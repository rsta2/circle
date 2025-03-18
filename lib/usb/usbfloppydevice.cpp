//
// usbfloppydevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbfloppydevice.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/synchronize.h>
#include <circle/macros.h>
#include <circle/new.h>
#include <assert.h>

#define BLOCK_SIZE		512
#define BLOCK_MASK		(BLOCK_SIZE-1)
#define BLOCK_SHIFT		9

#define MAX_OFFSET		0x1FFFFFFFFFFULL	// 2TB

#define MAX_TRIES		5			// max. attempts

// USB Mass Storage Control/Bulk/Interrupt (CBI) Transport

// Command Block Status
#define STATUS_PASS			0x00
#define STATUS_FAIL			0x01
#define STATUS_PHASE_ERROR		0x02
#define STATUS_PERSISTENT_FAILURE	0x03

// UFI Command Set

struct TSCSIInquiry
{
	u8		OperationCode,
#define SCSI_OP_INQUIRY		0x12
			LogicalUnitNumberEVPD,
			PageCode,
			Reserved1,
			AllocationLength,
			Reserved2[7];
}
PACKED;

struct TSCSIInquiryResponse
{
	u8		PeripheralDeviceType	: 5,
#define SCSI_PDT_DIRECT_ACCESS_FLOPPY	0x00			// Direct access device (floppy)
#define SCSI_PDT_NONE			0x1F			// No FDD connected to this unit
			Reserved1		: 3,
			Reserved2		: 7,
			RMB			: 1,		// 1: removable media
			ANSIApprovedVersion	: 3,		// 0
			ECMAVersion		: 3,		// 0
			ISOVersion		: 2,		// 0
			ResponseDataFormat	: 4,		// 1: UFI device
			Reserved3		: 4,
			AdditionalLength,			// 31
			Reserved4[3],
			VendorIdentification[8],
			ProductIdentification[16],
			ProductRevisionLevel[4];
}
PACKED;

struct TSCSITestUnitReady
{
	u8		OperationCode,
#define SCSI_OP_TEST_UNIT_READY		0x00
			Reserved1		: 5,
			LogicalUnitNumber	: 3,
			Reserved2[10];
}
PACKED;

struct TSCSIRequestSense
{
	u8		OperationCode,
#define SCSI_REQUEST_SENSE		0x03
			Reserved1		: 5,
			LogicalUnitNumber	: 3,
			Reserved2[2],
			AllocationLength,
			Reserved3[7];
}
PACKED;

struct TSCSIRequestSenseResponse
{
	u8		ErrorCode		: 7,		// 0x70
			Valid			: 1,
			Reserved1,
			SenseKey		: 4,
#define SCSI_SENSE_KEY_NOT_READY	0x02
			Reserved2		: 4,
			Information[4],				// big endian
			AdditionalSenseLength,			// 10
			Reserved3[4],
			AdditionalSenseCode,
			AdditionalSenseCodeQualifier,
			Reserved4[4];
}
PACKED;

struct TSCSIReadCapacity
{
	u8		OperationCode,
#define SCSI_OP_READ_CAPACITY		0x25
			RelAdr			: 1,
			Reserved1		: 4,
			LogicalUnitNumber	: 3;
	u32		LogicalBlockAddress;			// set to 0
	u16		Reserved2;
	u8		PartialMediumIndicator	: 1,		// set to 0
			Reserved3		: 7,
			Reserved4[3];
}
PACKED;

struct TSCSIReadCapacityResponse
{
	u32		LastLogicalBlockAddress;		// big endian
	u32		BlockLengthInBytes;			// big endian
}
PACKED;

struct TSCSIRead10
{
	u8		OperationCode,
#define SCSI_OP_READ		0x28
			RelAdr			: 1,
			Reserved1		: 2,
			FUA			: 1,
			DPO			: 1,
			LogicalUnitNumber	: 3;
	u32		LogicalBlockAddress;			// big endian
	u8		Reserved2;
	u16		TransferLength;				// block count, big endian
	u8		Reserved3[3];
}
PACKED;

struct TSCSIWrite10
{
	u8		OperationCode,
#define SCSI_OP_WRITE		0x2A
			RelAdr			: 1,
			Reserved1		: 2,
			FUA			: 1,
			DPO			: 1,
			LogicalUnitNumber	: 3;
	u32		LogicalBlockAddress;			// big endian
	u8		Reserved2;
	u16		TransferLength;				// block count, big endian
	u8		Reserved3[3];
}
PACKED;

struct TSCSISendDiagnostic
{
	u8		OperationCode,
#define SCSI_OP_SEND_DIAGNOSTIC	0x1D
			UnitOfl			: 1,
			DefOfl			: 1,
			SelfTest		: 1,
			Reserved1		: 1,
			PF			: 1,
			LogicalUnitNumber	: 3,
			Reserved2[10];
}
PACKED;

CNumberPool CUSBFloppyDiskDevice::s_DeviceNumberPool (1);

LOGMODULE ("ufd");

static const char DevicePrefix[] = "ufd";

CUSBFloppyDiskDevice::CUSBFloppyDiskDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pEndpointIn (0),
	m_pEndpointOut (0),
	m_pEndpointInterrupt (0),
	m_nBlockCount (0),
	m_ullOffset (0),
	m_nDeviceNumber (0)
{
}

CUSBFloppyDiskDevice::~CUSBFloppyDiskDevice (void)
{
	if (m_nDeviceNumber != 0)
	{
		CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, TRUE);

		s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);

		m_nDeviceNumber = 0;
	}
	delete m_pEndpointInterrupt;
	m_pEndpointInterrupt = 0;

	delete m_pEndpointOut;
	m_pEndpointOut =  0;
	
	delete m_pEndpointIn;
	m_pEndpointIn = 0;
}

boolean CUSBFloppyDiskDevice::Configure (void)
{
	if (GetNumEndpoints () < 2)
	{
		ConfigurationError (From);

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
					ConfigurationError (From);

					return FALSE;
				}

				m_pEndpointIn = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
			else							// Output
			{
				if (m_pEndpointOut != 0)
				{
					ConfigurationError (From);

					return FALSE;
				}

				m_pEndpointOut = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
		}
		else if ((pEndpointDesc->bmAttributes & 0x3F) == 0x03)		// Interrupt
		{
			if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x80)	// Input
			{
				if (m_pEndpointInterrupt != 0)
				{
					ConfigurationError (From);

					return FALSE;
				}

				m_pEndpointInterrupt = new CUSBEndpoint (GetDevice (),
									 pEndpointDesc);
			}
		}
	}

	if (   m_pEndpointIn == 0
	    || m_pEndpointOut == 0
	    || (   GetInterfaceProtocol () == 0
	        && m_pEndpointInterrupt == 0))
	{
		ConfigurationError (From);

		return FALSE;
	}

	if (!CUSBFunction::Configure ())
	{
		LOGERR ("Cannot set interface");

		return FALSE;
	}

	TSCSIInquiry SCSIInquiry;
	memset (&SCSIInquiry, 0, sizeof SCSIInquiry);
	SCSIInquiry.OperationCode = SCSI_OP_INQUIRY;
	SCSIInquiry.AllocationLength = sizeof (TSCSIInquiryResponse);

	TSCSIInquiryResponse SCSIInquiryResponse;
	if (Command (&SCSIInquiry, sizeof SCSIInquiry,
		     &SCSIInquiryResponse, sizeof SCSIInquiryResponse,
		     TRUE) != (int) sizeof SCSIInquiryResponse)
	{
		LOGERR ("Device does not respond");

		return FALSE;
	}

	if (SCSIInquiryResponse.PeripheralDeviceType != SCSI_PDT_DIRECT_ACCESS_FLOPPY)
	{
		LOGERR ("Unsupported device type: 0x%02X",
			(unsigned) SCSIInquiryResponse.PeripheralDeviceType);
		
		return FALSE;
	}

	if (WaitUnitReady () < 0)
	{
		return FALSE;
	}

	TSCSIReadCapacityResponse SCSIReadCapacityResponse;
	unsigned nTries;
	for (nTries = MAX_TRIES; nTries; nTries--)
	{
		TSCSIReadCapacity SCSIReadCapacity;
		memset (&SCSIReadCapacity, 0, sizeof SCSIReadCapacity);
		SCSIReadCapacity.OperationCode = SCSI_OP_READ_CAPACITY;

		if (Command (&SCSIReadCapacity, sizeof SCSIReadCapacity,
			     &SCSIReadCapacityResponse, sizeof SCSIReadCapacityResponse,
			     TRUE) == (int) sizeof SCSIReadCapacityResponse)
		{
			break;
		}
	}

	if (nTries == 0)
	{
		LOGERR ("Read capacity failed");

		return FALSE;
	}

	unsigned nBlockSize = le2be32 (SCSIReadCapacityResponse.BlockLengthInBytes);
	if (nBlockSize != BLOCK_SIZE)
	{
		LOGERR ("Unsupported block size: %u", nBlockSize);

		return FALSE;
	}

	m_nBlockCount = le2be32 (SCSIReadCapacityResponse.LastLogicalBlockAddress);
	if (m_nBlockCount == (u32) -1)
	{
		LOGERR ("Unsupported disk size > 2TB");

		return FALSE;
	}

	m_nBlockCount++;

	LOGDBG ("Capacity is %u KBytes", m_nBlockCount / (0x400 / BLOCK_SIZE));

	unsigned nDeviceNumber = s_DeviceNumberPool.AllocateNumber (FALSE);
	if (nDeviceNumber == CNumberPool::Invalid)
	{
		LOGERR ("Too many devices");

		return FALSE;
	}

	assert (m_nDeviceNumber == 0);
	m_nDeviceNumber = nDeviceNumber;

	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, TRUE);
	
	return TRUE;
}

int CUSBFloppyDiskDevice::Read (void *pBuffer, size_t nCount)
{
	int nResult = WaitUnitReady ();
	if (nResult < 0)
	{
		return nResult;
	}

	unsigned nTries = MAX_TRIES;

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

int CUSBFloppyDiskDevice::Write (const void *pBuffer, size_t nCount)
{
	int nResult = WaitUnitReady ();
	if (nResult < 0)
	{
		return nResult;
	}

	unsigned nTries = MAX_TRIES;

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

u64 CUSBFloppyDiskDevice::Seek (u64 ullOffset)
{
	m_ullOffset = ullOffset;

	return m_ullOffset;
}

u64 CUSBFloppyDiskDevice::GetSize (void) const
{
	assert (m_nBlockCount > 0);
	assert (m_nBlockCount < (u32) -1);

	return (u64) m_nBlockCount << BLOCK_SHIFT;
}

int CUSBFloppyDiskDevice::WaitUnitReady (void)
{
	unsigned nTicks = CTimer::Get ()->GetTicks ();
	while (CTimer::Get ()->GetTicks () - nTicks < 4*HZ)
	{
		TSCSITestUnitReady SCSITestUnitReady;
		memset (&SCSITestUnitReady, 0, sizeof SCSITestUnitReady);
		SCSITestUnitReady.OperationCode = SCSI_OP_TEST_UNIT_READY;

		if (Command (&SCSITestUnitReady, sizeof SCSITestUnitReady, 0, 0, FALSE) >= 0)
		{
			return 0;
		}

#if RASPPI >= 4
		if (!GetEndpoint0 ()->GetXHCIEndpoint ()->ResetFromHalted ())
		{
			LOGERR ("Endpoint reset failed");

			return -1;
		}

		// TODO: Send CLEAR_TT_BUFFER for EP0 to the hub, this device is connected to.
#endif

		TSCSIRequestSense SCSIRequestSense;
		memset (&SCSIRequestSense, 0, sizeof SCSIRequestSense);
		SCSIRequestSense.OperationCode = SCSI_REQUEST_SENSE;
		SCSIRequestSense.AllocationLength = sizeof (TSCSIRequestSenseResponse);

		TSCSIRequestSenseResponse SCSIRequestSenseResponse;
		if (Command (&SCSIRequestSense, sizeof SCSIRequestSense,
			     &SCSIRequestSenseResponse, sizeof SCSIRequestSenseResponse,
			     TRUE) < 0)
		{
			LOGERR ("Request sense failed");

			return -1;
		}

		if (SCSIRequestSenseResponse.SenseKey == SCSI_SENSE_KEY_NOT_READY)
		{
			break;
		}
	}

	LOGERR ("Unit is not ready");

	return -1;
}

int CUSBFloppyDiskDevice::TryRead (void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);

	if (   (m_ullOffset & BLOCK_MASK) != 0
	    || m_ullOffset > MAX_OFFSET)
	{
		return -1;
	}
	u32 nBlockAddress = (u32) (m_ullOffset >> BLOCK_SHIFT);

	if ((nCount & BLOCK_MASK) != 0)
	{
		return -1;
	}
	u16 usTransferLength = (u16) (nCount >> BLOCK_SHIFT);

	// LOGDBG ("TryRead %u/0x%lX/%u", nBlockAddress, (uintptr) pBuffer, (unsigned) usTransferLength);

	TSCSIRead10 SCSIRead;
	memset (&SCSIRead, 0, sizeof SCSIRead);
	SCSIRead.OperationCode		= SCSI_OP_READ;
	SCSIRead.LogicalBlockAddress	= le2be32 (nBlockAddress);
	SCSIRead.TransferLength		= le2be16 (usTransferLength);

	if (Command (&SCSIRead, sizeof SCSIRead, pBuffer, nCount, TRUE) != (int) nCount)
	{
		LOGERR ("TryRead failed");

		return -1;
	}

	return nCount;
}

int CUSBFloppyDiskDevice::TryWrite (const void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);

	if (   (m_ullOffset & BLOCK_MASK) != 0
	    || m_ullOffset > MAX_OFFSET)
	{
		return -1;
	}
	u32 nBlockAddress = (u32) (m_ullOffset >> BLOCK_SHIFT);

	if ((nCount & BLOCK_MASK) != 0)
	{
		return -1;
	}
	u16 usTransferLength = (u16) (nCount >> BLOCK_SHIFT);

	// LOGDBG ("TryWrite %u/0x%lX/%u", nBlockAddress, (uintptr) pBuffer, (unsigned) usTransferLength);

	TSCSIWrite10 SCSIWrite;
	memset (&SCSIWrite, 0, sizeof SCSIWrite);
	SCSIWrite.OperationCode		= SCSI_OP_WRITE;
	SCSIWrite.LogicalBlockAddress	= le2be32 (nBlockAddress);
	SCSIWrite.TransferLength	= le2be16 (usTransferLength);

	if (Command (&SCSIWrite, sizeof SCSIWrite, (void *) pBuffer, nCount, FALSE) < 0)
	{
		LOGERR ("TryWrite failed");

		return -1;
	}

	return nCount;
}

int CUSBFloppyDiskDevice::Command (void *pCmdBlk, size_t nCmdBlkLen,
				   void *pBuffer, size_t nBufLen, boolean bIn)
{
	assert (pCmdBlk != 0);
	assert (nCmdBlkLen == 12);
	assert (nBufLen == 0 || pBuffer != 0);

	DMA_BUFFER (u8, CmdBuffer, nCmdBlkLen);
	memcpy (CmdBuffer, pCmdBlk, nCmdBlkLen);

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
				   0, 0, GetInterfaceNumber (), CmdBuffer, nCmdBlkLen) < 0)
	{
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
			LOGERR ("Data transfer failed");

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

	if (GetInterfaceProtocol () == 0)
	{
		DMA_BUFFER (u8, Status, 2);

		assert (m_pEndpointInterrupt != 0);
		if (pHost->Transfer (m_pEndpointInterrupt, Status, 2) != 2)
		{
			LOGERR ("Status transfer failed");

			return -1;
		}

		if (   Status[0] != 0
		    || Status[1] != STATUS_PASS)
		{
			return -1;
		}
	}

	return nResult;
}

int CUSBFloppyDiskDevice::Reset (void)
{
	TSCSISendDiagnostic SCSISendDiagnostic;
	memset (&SCSISendDiagnostic, 0, sizeof SCSISendDiagnostic);
	SCSISendDiagnostic.OperationCode = SCSI_OP_SEND_DIAGNOSTIC;
	SCSISendDiagnostic.SelfTest = 1;

	return Command (&SCSISendDiagnostic, sizeof SCSISendDiagnostic, 0, 0, FALSE);
}
