//
// usbmassdevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <circle/usb/usbmassdevice.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/synchronize.h>
#include <circle/macros.h>
#include <circle/new.h>
#include <assert.h>

#define MAX_TRIES	8				// max. read / write attempts

// USB Mass Storage Bulk-Only Transport

// Class-specific requests
#define GET_MAX_LUN			0xFE
#define BULK_ONLY_MASS_STORAGE_RESET	0xFF

// Command Block Wrapper
struct TCBW
{
	u32		dCWBSignature,
#define CBWSIGNATURE		0x43425355
			dCWBTag,
			dCBWDataTransferLength;		// number of bytes
	u8		bmCBWFlags,
#define CBWFLAGS_DATA_IN	0x80
			bCBWLUN		: 4,
#define CBWLUN			0
			Reserved1	: 4,
			bCBWCBLength	: 5,		// valid length of the CBWCB in bytes
			Reserved2	: 3;
	u8		CBWCB[16];
}
PACKED;

// Command Status Wrapper
struct TCSW
{
	u32		dCSWSignature,
#define CSWSIGNATURE		0x53425355
			dCSWTag,
			dCSWDataResidue;		// difference in amount of data processed
	u8		bCSWStatus;
#define CSWSTATUS_PASSED	0x00
#define CSWSTATUS_FAILED	0x01
#define CSWSTATUS_PHASE_ERROR	0x02
}
PACKED;

// SCSI Transparent Command Set

#define SCSI_CONTROL		0x00

struct TSCSIInquiry
{
	u8		OperationCode,
#define SCSI_OP_INQUIRY		0x12
			LogicalUnitNumberEVPD,
			PageCode,
			Reserved,
			AllocationLength,
			Control;
}
PACKED;

struct TSCSIInquiryResponse
{
	u8		PeripheralDeviceType	: 5,
#define SCSI_PDT_DIRECT_ACCESS_BLOCK	0x00			// SBC-2 command set (or above)
#define SCSI_PDT_DIRECT_ACCESS_RBC	0x0E			// RBC command set
			PeripheralQualifier	: 3,		// 0: device is connected to this LUN
			DeviceTypeModifier	: 7,
			RMB			: 1,		// 1: removable media
			ANSIApprovedVersion	: 3,
			ECMAVersion		: 3,
			ISOVersion		: 2,
			Reserved1,
			AdditionalLength,
			Reserved2[3],
			VendorIdentification[8],
			ProductIdentification[16],
			ProductRevisionLevel[4];
}
PACKED;

struct TSCSITestUnitReady
{
	u8		OperationCode;
#define SCSI_OP_TEST_UNIT_READY		0x00
	u32		Reserved;
	u8		Control;
}
PACKED;

struct TSCSIRequestSense
{
	u8		OperationCode;
#define SCSI_REQUEST_SENSE		0x03
	u8		DescriptorFormat	: 1,		// set to 0
			Reserved1		: 7;
	u16		Reserved2;
	u8		AllocationLength;
	u8		Control;
}
PACKED;

struct TSCSIRequestSenseResponse7x
{
	u8		ResponseCode		: 7,
			Valid			: 1;
	u8		Obsolete;
	u8		SenseKey		: 4,
			Reserved		: 1,
			ILI			: 1,
			EOM			: 1,
			FileMark		: 1;
	u32		Information;				// big endian
	u8		AdditionalSenseLength;
	u32		CommandSpecificInformation;		// big endian
	u8		AdditionalSenseCode;
	u8		AdditionalSenseCodeQualifier;
	u8		FieldReplaceableUnitCode;
	u8		SenseKeySpecificHigh	: 7,
			SKSV			: 1;
	u16		SenseKeySpecificLow;
}
PACKED;

struct TSCSIReadCapacity10
{
	u8		OperationCode;
#define SCSI_OP_READ_CAPACITY10		0x25
	u8		Obsolete		: 1,
			Reserved1		: 7;
	u32		LogicalBlockAddress;			// set to 0
	u16		Reserved2;
	u8		PartialMediumIndicator	: 1,		// set to 0
			Reserved3		: 7;
	u8		Control;
}
PACKED;

struct TSCSIReadCapacityResponse
{
	u32		ReturnedLogicalBlockAddress;		// big endian
	u32		BlockLengthInBytes;			// big endian
}
PACKED;

struct TSCSIRead10
{
	u8		OperationCode,
#define SCSI_OP_READ		0x28
			Reserved1;
	u32		LogicalBlockAddress;			// big endian
	u8		Reserved2;
	u16		TransferLength;				// block count, big endian
	u8		Control;
#define SCSI_READ_CONTROL	0x00
}
PACKED;

struct TSCSIWrite10
{
	u8		OperationCode,
#define SCSI_OP_WRITE		0x2A
			Flags;
#define SCSI_WRITE_FUA		0x08
	u32		LogicalBlockAddress;			// big endian
	u8		Reserved;
	u16		TransferLength;				// block count, big endian
	u8		Control;
#define SCSI_WRITE_CONTROL	0x00
}
PACKED;

CNumberPool CUSBBulkOnlyMassStorageDevice::s_DeviceNumberPool (1);

static const char FromUmsd[] = "umsd";

CUSBBulkOnlyMassStorageDevice::CUSBBulkOnlyMassStorageDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pEndpointIn (0),
	m_pEndpointOut (0),
	m_nCWBTag (0),
	m_nBlockCount (0),
	m_ullOffset (0),
	m_pPartitionManager (0),
	m_nDeviceNumber (0)
{
}

CUSBBulkOnlyMassStorageDevice::~CUSBBulkOnlyMassStorageDevice (void)
{
	if (m_nDeviceNumber != 0)
	{
		CDeviceNameService::Get ()->RemoveDevice ("umsd", m_nDeviceNumber, TRUE);

		s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);

		m_nDeviceNumber = 0;
	}

	delete m_pPartitionManager;
	m_pPartitionManager = 0;

	delete m_pEndpointOut;
	m_pEndpointOut =  0;
	
	delete m_pEndpointIn;
	m_pEndpointIn = 0;
}

boolean CUSBBulkOnlyMassStorageDevice::Configure (void)
{
	if (GetNumEndpoints () < 2)
	{
		ConfigurationError (FromUmsd);

		return FALSE;
	}

	const TUSBEndpointDescriptor *pEndpointDesc;
	while ((pEndpointDesc = (TUSBEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT)) != 0)
	{
		if ((pEndpointDesc->bmAttributes & 0x3F) == 0x02)		// Bulk
		{
			if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x80)	// Input
			{
				if (m_pEndpointIn != 0)
				{
					ConfigurationError (FromUmsd);

					return FALSE;
				}

				m_pEndpointIn = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
			else							// Output
			{
				if (m_pEndpointOut != 0)
				{
					ConfigurationError (FromUmsd);

					return FALSE;
				}

				m_pEndpointOut = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
		}
	}

	if (   m_pEndpointIn  == 0
	    || m_pEndpointOut == 0)
	{
		ConfigurationError (FromUmsd);

		return FALSE;
	}

	if (!CUSBFunction::Configure ())
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Cannot set interface");

		return FALSE;
	}

	TSCSIInquiry SCSIInquiry;
	SCSIInquiry.OperationCode	  = SCSI_OP_INQUIRY;
	SCSIInquiry.LogicalUnitNumberEVPD = 0;
	SCSIInquiry.PageCode		  = 0;
	SCSIInquiry.Reserved		  = 0;
	SCSIInquiry.AllocationLength	  = sizeof (TSCSIInquiryResponse);
	SCSIInquiry.Control		  = SCSI_CONTROL;

	TSCSIInquiryResponse SCSIInquiryResponse;
	if (Command (&SCSIInquiry, sizeof SCSIInquiry,
		     &SCSIInquiryResponse, sizeof SCSIInquiryResponse,
		     TRUE) != (int) sizeof SCSIInquiryResponse)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Device does not respond");

		return FALSE;
	}

	if (SCSIInquiryResponse.PeripheralDeviceType != SCSI_PDT_DIRECT_ACCESS_BLOCK)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Unsupported device type: 0x%02X", (unsigned) SCSIInquiryResponse.PeripheralDeviceType);
		
		return FALSE;
	}

	unsigned nTries = 100;
	while (--nTries)
	{
		CTimer::Get ()->MsDelay (100);

		TSCSITestUnitReady SCSITestUnitReady;
		SCSITestUnitReady.OperationCode = SCSI_OP_TEST_UNIT_READY;
		SCSITestUnitReady.Reserved	= 0;
		SCSITestUnitReady.Control	= SCSI_CONTROL;

		if (Command (&SCSITestUnitReady, sizeof SCSITestUnitReady, 0, 0, FALSE) >= 0)
		{
			break;
		}

		TSCSIRequestSense SCSIRequestSense;
		SCSIRequestSense.OperationCode	  = SCSI_REQUEST_SENSE;
		SCSIRequestSense.DescriptorFormat = 0;
		SCSIRequestSense.Reserved1	  = 0;
		SCSIRequestSense.Reserved2	  = 0;
		SCSIRequestSense.AllocationLength = sizeof (TSCSIRequestSenseResponse7x);
		SCSIRequestSense.Control	  = SCSI_CONTROL;

		TSCSIRequestSenseResponse7x SCSIRequestSenseResponse7x;
		if (Command (&SCSIRequestSense, sizeof SCSIRequestSense,
			     &SCSIRequestSenseResponse7x, sizeof SCSIRequestSenseResponse7x,
			     TRUE) < 0)
		{
			CLogger::Get ()->Write (FromUmsd, LogError, "Request sense failed");

			return FALSE;
		}
	}

	if (nTries == 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Unit is not ready");

		return FALSE;
	}

	TSCSIReadCapacity10 SCSIReadCapacity;
	SCSIReadCapacity.OperationCode		= SCSI_OP_READ_CAPACITY10;
	SCSIReadCapacity.Obsolete		= 0;
	SCSIReadCapacity.Reserved1		= 0;
	SCSIReadCapacity.LogicalBlockAddress	= 0;
	SCSIReadCapacity.Reserved2		= 0;
	SCSIReadCapacity.PartialMediumIndicator	= 0;
	SCSIReadCapacity.Reserved3		= 0;
	SCSIReadCapacity.Control		= SCSI_CONTROL;

	TSCSIReadCapacityResponse SCSIReadCapacityResponse;
	if (Command (&SCSIReadCapacity, sizeof SCSIReadCapacity,
		     &SCSIReadCapacityResponse, sizeof SCSIReadCapacityResponse,
		     TRUE) != (int) sizeof SCSIReadCapacityResponse)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Read capacity failed");

		return FALSE;
	}

	unsigned nBlockSize = le2be32 (SCSIReadCapacityResponse.BlockLengthInBytes);
	if (nBlockSize != UMSD_BLOCK_SIZE)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Unsupported block size: %u", nBlockSize);

		return FALSE;
	}

	m_nBlockCount = le2be32 (SCSIReadCapacityResponse.ReturnedLogicalBlockAddress);
	if (m_nBlockCount == (u32) -1)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Unsupported disk size > 2TB");

		return FALSE;
	}

	m_nBlockCount++;

	CLogger::Get ()->Write (FromUmsd, LogDebug, "Capacity is %u MByte", m_nBlockCount / (0x100000 / UMSD_BLOCK_SIZE));

	unsigned nDeviceNumber = s_DeviceNumberPool.AllocateNumber (FALSE);
	if (nDeviceNumber == CNumberPool::Invalid)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Too many devices");

		return FALSE;
	}

	assert (m_nDeviceNumber == 0);
	m_nDeviceNumber = nDeviceNumber;

	CString DeviceName;
	DeviceName.Format ("umsd%u", m_nDeviceNumber);

	assert (m_pPartitionManager == 0);
	m_pPartitionManager = new CPartitionManager (this, DeviceName);
	assert (m_pPartitionManager != 0);
	if (!m_pPartitionManager->Initialize ())
	{
		s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);
		m_nDeviceNumber = 0;

		return FALSE;
	}

	CDeviceNameService::Get ()->AddDevice (DeviceName, this, TRUE);
	
	return TRUE;
}

int CUSBBulkOnlyMassStorageDevice::Read (void *pBuffer, size_t nCount)
{
	unsigned nTries = MAX_TRIES;

	int nResult;

	do
	{
		nResult = TryRead (pBuffer, nCount);

		if (nResult != (int) nCount)
		{
			int nStatus = Reset ();
			if (nStatus != 0)
			{
				return nStatus;
			}
		}
	}
	while (   nResult != (int) nCount
	       && --nTries > 0);

	return nResult;
}

int CUSBBulkOnlyMassStorageDevice::Write (const void *pBuffer, size_t nCount)
{
	unsigned nTries = MAX_TRIES;

	int nResult;

	do
	{
		nResult = TryWrite (pBuffer, nCount);

		if (nResult != (int) nCount)
		{
			int nStatus = Reset ();
			if (nStatus != 0)
			{
				return nStatus;
			}
		}
	}
	while (   nResult != (int) nCount
	       && --nTries > 0);

	return nResult;
}

u64 CUSBBulkOnlyMassStorageDevice::Seek (u64 ullOffset)
{
	m_ullOffset = ullOffset;

	return m_ullOffset;
}

u64 CUSBBulkOnlyMassStorageDevice::GetSize (void) const
{
	assert (m_nBlockCount > 0);
	assert (m_nBlockCount < (u32) -1);

	return (u64) m_nBlockCount << UMSD_BLOCK_SHIFT;
}

unsigned CUSBBulkOnlyMassStorageDevice::GetCapacity (void) const
{
	return m_nBlockCount;
}

int CUSBBulkOnlyMassStorageDevice::TryRead (void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);

	if (   (m_ullOffset & UMSD_BLOCK_MASK) != 0
	    || m_ullOffset > UMSD_MAX_OFFSET)
	{
		return -1;
	}
	u32 nBlockAddress = (u32) (m_ullOffset >> UMSD_BLOCK_SHIFT);

	if ((nCount & UMSD_BLOCK_MASK) != 0)
	{
		return -1;
	}
	u16 usTransferLength = (u16) (nCount >> UMSD_BLOCK_SHIFT);

	//CLogger::Get ()->Write (FromUmsd, LogDebug, "TryRead %u/0x%X/%u", nBlockAddress, (unsigned) pBuffer, (unsigned) usTransferLength);

	TSCSIRead10 SCSIRead;
	SCSIRead.OperationCode		= SCSI_OP_READ;
	SCSIRead.Reserved1		= 0;
	SCSIRead.LogicalBlockAddress	= le2be32 (nBlockAddress);
	SCSIRead.Reserved2		= 0;
	SCSIRead.TransferLength		= le2be16 (usTransferLength);
	SCSIRead.Control		= SCSI_CONTROL;

	if (Command (&SCSIRead, sizeof SCSIRead, pBuffer, nCount, TRUE) != (int) nCount)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "TryRead failed");

		return -1;
	}

	return nCount;
}

int CUSBBulkOnlyMassStorageDevice::TryWrite (const void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);

	if (   (m_ullOffset & UMSD_BLOCK_MASK) != 0
	    || m_ullOffset > UMSD_MAX_OFFSET)
	{
		return -1;
	}
	u32 nBlockAddress = (u32) (m_ullOffset >> UMSD_BLOCK_SHIFT);

	if ((nCount & UMSD_BLOCK_MASK) != 0)
	{
		return -1;
	}
	u16 usTransferLength = (u16) (nCount >> UMSD_BLOCK_SHIFT);

	//CLogger::Get ()->Write (FromUmsd, LogDebug, "TryWrite %u/0x%X/%u", nBlockAddress, (unsigned) pBuffer, (unsigned) usTransferLength);

	TSCSIWrite10 SCSIWrite;
	SCSIWrite.OperationCode		= SCSI_OP_WRITE;
	SCSIWrite.Flags			= SCSI_WRITE_FUA;
	SCSIWrite.LogicalBlockAddress	= le2be32 (nBlockAddress);
	SCSIWrite.Reserved		= 0;
	SCSIWrite.TransferLength	= le2be16 (usTransferLength);
	SCSIWrite.Control		= SCSI_CONTROL;

	if (Command (&SCSIWrite, sizeof SCSIWrite, (void *) pBuffer, nCount, FALSE) < 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "TryWrite failed");

		return -1;
	}

	return nCount;
}

int CUSBBulkOnlyMassStorageDevice::Command (void *pCmdBlk, size_t nCmdBlkLen,
					    void *pBuffer, size_t nBufLen, boolean bIn)
{
	assert (pCmdBlk != 0);
	assert (6 <= nCmdBlkLen && nCmdBlkLen <= 16);
	assert (nBufLen == 0 || pBuffer != 0);

	DMA_BUFFER (u8, CBWBuffer, sizeof (TCBW));
	TCBW *pCBW = (TCBW *) CBWBuffer;
	memset (pCBW, 0, sizeof *pCBW);

	pCBW->dCWBSignature	     = CBWSIGNATURE;
	pCBW->dCWBTag		     = ++m_nCWBTag;
	pCBW->dCBWDataTransferLength = nBufLen;
	pCBW->bmCBWFlags	     = bIn ? CBWFLAGS_DATA_IN : 0;
	pCBW->bCBWLUN		     = CBWLUN;
	pCBW->bCBWCBLength	     = (u8) nCmdBlkLen;

	memcpy (pCBW->CBWCB, pCmdBlk, nCmdBlkLen);

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	if (pHost->Transfer (m_pEndpointOut, pCBW, sizeof *pCBW) < 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "CBW transfer failed");

		return -1;
	}

	int nResult = 0;
	
	if (nBufLen > 0)
	{
		assert (pBuffer != 0);

		u8 *pDMABuffer = 0;
		if (!IS_CACHE_ALIGNED (pBuffer, nBufLen))
		{
			pDMABuffer = new (HEAP_DMA30) u8[nBufLen];
			assert (pDMABuffer != 0);

			if (!bIn)
			{
				memcpy (pDMABuffer, pBuffer, nBufLen);
			}
		}

		nResult = pHost->Transfer (bIn ? m_pEndpointIn : m_pEndpointOut,
					   pDMABuffer != 0 ? pDMABuffer : pBuffer, nBufLen);
		if (nResult < 0)
		{
			CLogger::Get ()->Write (FromUmsd, LogError, "Data transfer failed");

			delete [] pDMABuffer;

			return -1;
		}

		if (pDMABuffer != 0)
		{
			if (bIn)
			{
				memcpy (pBuffer, pDMABuffer, nBufLen);
			}

			delete [] pDMABuffer;
		}
	}

	DMA_BUFFER (u8, CSWBuffer, sizeof (TCSW));
	TCSW *pCSW = (TCSW *) CSWBuffer;

	if (pHost->Transfer (m_pEndpointIn, pCSW, sizeof *pCSW) != (int) sizeof *pCSW)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "CSW transfer failed");

		if (pHost->ControlMessage (GetEndpoint0 (),
					   REQUEST_TO_ENDPOINT | REQUEST_OUT, CLEAR_FEATURE,
					   ENDPOINT_HALT, m_pEndpointIn->GetNumber () | 0x80,
					   0, 0) < 0)
		{
			CLogger::Get ()->Write (FromUmsd, LogDebug,
						"Cannot clear halt on endpoint IN");

			return -1;
		}

		m_pEndpointIn->ResetPID ();

		if (pHost->Transfer (m_pEndpointIn, pCSW, sizeof *pCSW) != (int) sizeof *pCSW)
		{
			CLogger::Get ()->Write (FromUmsd, LogError, "CSW transfer failed twice");

			return -1;
		}
	}

	if (pCSW->dCSWSignature != CSWSIGNATURE)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "CSW signature is wrong");

		return -1;
	}

	if (pCSW->dCSWTag != m_nCWBTag)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "CSW tag is wrong");

		return -1;
	}

	if (pCSW->bCSWStatus != CSWSTATUS_PASSED)
	{
		return -1;
	}

	if (pCSW->dCSWDataResidue != 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Data residue is not 0");

		return -1;
	}

	return nResult;
}

int CUSBBulkOnlyMassStorageDevice::Reset (void)
{
	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);
	
	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_CLASS | REQUEST_TO_INTERFACE | REQUEST_OUT,
				   BULK_ONLY_MASS_STORAGE_RESET, 0, GetInterfaceNumber (), 0, 0) < 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogDebug, "Cannot reset device");

		return -1;
	}

	CTimer::Get ()->MsDelay (100);

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_TO_ENDPOINT | REQUEST_OUT, CLEAR_FEATURE,
				   ENDPOINT_HALT, m_pEndpointIn->GetNumber () | 0x80, 0, 0) < 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogDebug, "Cannot clear halt on endpoint IN");

		return -1;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_TO_ENDPOINT | REQUEST_OUT, CLEAR_FEATURE,
				   ENDPOINT_HALT, m_pEndpointOut->GetNumber (), 0, 0) < 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogDebug, "Cannot clear halt on endpoint OUT");

		return -1;
	}

	m_pEndpointIn->ResetPID ();
	m_pEndpointOut->ResetPID ();

	return 0;
}
