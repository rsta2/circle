//
// usbmassdevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

// USB Mass Storage Bulk-Only Transport

// Command Block Wrapper
struct TCBW
{
	unsigned int	dCWBSignature,
#define CBWSIGNATURE		0x43425355
			dCWBTag,
			dCBWDataTransferLength;		// number of bytes
	unsigned char	bmCBWFlags,
#define CBWFLAGS_DATA_IN	0x80
			bCBWLUN		: 4,
#define CBWLUN			0
			Reserved1	: 4,
			bCBWCBLength	: 5,		// valid length of the CBWCB in bytes
			Reserved2	: 3;
	// CBWCB follows
}
PACKED;

// Command Status Wrapper
struct TCSW
{
	unsigned int	dCSWSignature,
#define CSWSIGNATURE		0x53425355
			dCSWTag,
			dCSWDataResidue;		// difference in amount of data processed
	unsigned char	bCSWStatus;
#define CSWSTATUS_PASSED	0x00
#define CSWSTATUS_FAILED	0x01
#define CSWSTATUS_PHASE_ERROR	0x02
}
PACKED;

// SCSI Transparent Command Set

struct TSCSIInquiry
{
	unsigned char	OperationCode,
#define SCSI_OP_INQUIRY		0x12
			LogicalUnitNumberEVPD,
			PageCode,
			Reserved,
			AllocationLength,
			Control;
#define SCSI_INQUIRY_CONTROL	0x00
}
PACKED;

struct TCBWInquiry
{
	TCBW		CBW;
	TSCSIInquiry	SCSIInquiry;
	unsigned char	Padding[10];
}
PACKED;

struct TSCSIInquiryResponse
{
	unsigned char	PeripheralDeviceType	: 5,
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

struct TSCSIRead10
{
	unsigned char	OperationCode,
#define SCSI_OP_READ		0x28
			Reserved1;
	unsigned int	LogicalBlockAddress;			// big endian
	unsigned char	Reserved2;
	unsigned short	TransferLength;				// block count, big endian
	unsigned char	Control;
#define SCSI_READ_CONTROL	0x00
}
PACKED;

struct TCBWRead
{
	TCBW		CBW;
	TSCSIRead10	SCSIRead;
	unsigned char	Padding[6];
}
PACKED;

struct TSCSIWrite10
{
	unsigned char	OperationCode,
#define SCSI_OP_WRITE		0x2A
			Flags;
#define SCSI_WRITE_FUA		0x08
	unsigned int	LogicalBlockAddress;			// big endian
	unsigned char	Reserved;
	unsigned short	TransferLength;				// block count, big endian
	unsigned char	Control;
#define SCSI_WRITE_CONTROL	0x00
}
PACKED;

struct TCBWWrite
{
	TCBW		CBW;
	TSCSIWrite10	SCSIWrite;
	unsigned char	Padding[6];
}
PACKED;

unsigned CUSBBulkOnlyMassStorageDevice::s_nDeviceNumber = 1;

static const char FromUmsd[] = "umsd";

CUSBBulkOnlyMassStorageDevice::CUSBBulkOnlyMassStorageDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pEndpointIn (0),
	m_pEndpointOut (0),
	m_nCWBTag (0),
	m_ullOffset (0)
{
}

CUSBBulkOnlyMassStorageDevice::~CUSBBulkOnlyMassStorageDevice (void)
{
	delete m_pEndpointOut;
	m_pEndpointOut =  0;
	
	delete m_pEndpointIn;
	m_pEndpointIn = 0;
}

int CUSBBulkOnlyMassStorageDevice::Configure (void)
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

	TCBWInquiry CBWInquiry;

	CBWInquiry.CBW.dCWBSignature		= CBWSIGNATURE;
	CBWInquiry.CBW.dCWBTag			= ++m_nCWBTag;
	CBWInquiry.CBW.dCBWDataTransferLength	= sizeof (TSCSIInquiryResponse);
	CBWInquiry.CBW.bmCBWFlags		= CBWFLAGS_DATA_IN;
	CBWInquiry.CBW.bCBWLUN			= CBWLUN;
	CBWInquiry.CBW.Reserved1		= 0;
	CBWInquiry.CBW.bCBWCBLength		= sizeof (TSCSIInquiry);
	CBWInquiry.CBW.Reserved2		= 0;

	CBWInquiry.SCSIInquiry.OperationCode		= SCSI_OP_INQUIRY;
	CBWInquiry.SCSIInquiry.LogicalUnitNumberEVPD	= 0;
	CBWInquiry.SCSIInquiry.PageCode			= 0;
	CBWInquiry.SCSIInquiry.Reserved			= 0;
	CBWInquiry.SCSIInquiry.AllocationLength		= sizeof (TSCSIInquiryResponse);
	CBWInquiry.SCSIInquiry.Control			= SCSI_INQUIRY_CONTROL;

	TSCSIInquiryResponse SCSIInquiryResponse;
	TCSW CSW;

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	if (   pHost->Transfer (m_pEndpointOut, &CBWInquiry, sizeof CBWInquiry) < 0
	    || pHost->Transfer (m_pEndpointIn, &SCSIInquiryResponse, sizeof SCSIInquiryResponse) != (int) sizeof SCSIInquiryResponse
	    || pHost->Transfer (m_pEndpointIn, &CSW, sizeof CSW) != (int) sizeof CSW)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Device does not respond");

		return FALSE;
	}

	if (   CSW.dCSWSignature != CSWSIGNATURE
	    || CSW.dCSWTag       != m_nCWBTag)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Invalid inquiry response");

		return FALSE;
	}

	if (CSW.bCSWStatus != CSWSTATUS_PASSED)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Inquiry failed (status 0x%02X)", (unsigned) CSW.bCSWStatus);

		return FALSE;
	}

	if (CSW.dCSWDataResidue != 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Invalid data residue on inquiry");

		return FALSE;
	}
	
	if (SCSIInquiryResponse.PeripheralDeviceType != SCSI_PDT_DIRECT_ACCESS_BLOCK)
	{
		CLogger::Get ()->Write (FromUmsd, LogError, "Unsupported device type: 0x%02X", (unsigned) SCSIInquiryResponse.PeripheralDeviceType);
		
		return FALSE;
	}

	CString DeviceName;
	DeviceName.Format ("umsd%u", s_nDeviceNumber++);
	CDeviceNameService::Get ()->AddDevice (DeviceName, this, TRUE);
	
	return TRUE;
}

int CUSBBulkOnlyMassStorageDevice::Read (void *pBuffer, unsigned nCount)
{
	unsigned nTries = 4;

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

int CUSBBulkOnlyMassStorageDevice::Write (const void *pBuffer, unsigned nCount)
{
	unsigned nTries = 4;

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

unsigned long long CUSBBulkOnlyMassStorageDevice::Seek (unsigned long long ullOffset)
{
	m_ullOffset = ullOffset;

	return m_ullOffset;
}

int CUSBBulkOnlyMassStorageDevice::TryRead (void *pBuffer, unsigned nCount)
{
	assert (pBuffer != 0);

	if (   (m_ullOffset & UMSD_BLOCK_MASK) != 0
	    || m_ullOffset > UMSD_MAX_OFFSET)
	{
		return -1;
	}
	unsigned nBlockAddress = (unsigned) (m_ullOffset >> UMSD_BLOCK_SHIFT);

	if ((nCount & UMSD_BLOCK_MASK) != 0)
	{
		return -1;
	}
	unsigned short usTransferLength = (unsigned short) (nCount >> UMSD_BLOCK_SHIFT);

	//CLogger::Get ()->Write (FromUmsd, LogDebug, "TryRead %u/0x%X/%u", nBlockAddress, (unsigned) pBuffer, (unsigned) usTransferLength);

	TCBWRead CBWRead;

	CBWRead.CBW.dCWBSignature		= CBWSIGNATURE;
	CBWRead.CBW.dCWBTag			= ++m_nCWBTag;
	CBWRead.CBW.dCBWDataTransferLength	= nCount;
	CBWRead.CBW.bmCBWFlags			= CBWFLAGS_DATA_IN;
	CBWRead.CBW.bCBWLUN			= CBWLUN;
	CBWRead.CBW.Reserved1			= 0;
	CBWRead.CBW.bCBWCBLength		= sizeof (TSCSIRead10);
	CBWRead.CBW.Reserved2			= 0;

	CBWRead.SCSIRead.OperationCode		= SCSI_OP_READ;
	CBWRead.SCSIRead.Reserved1		= 0;
	CBWRead.SCSIRead.LogicalBlockAddress	= le2be32 (nBlockAddress);
	CBWRead.SCSIRead.Reserved2		= 0;
	CBWRead.SCSIRead.TransferLength		= le2be16 (usTransferLength);
	CBWRead.SCSIRead.Control		= SCSI_READ_CONTROL;

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);
	
	TCSW CSW;

	if (   pHost->Transfer (m_pEndpointOut, &CBWRead, sizeof CBWRead) < 0
	    || pHost->Transfer (m_pEndpointIn, pBuffer, nCount) != (int) nCount
	    || pHost->Transfer (m_pEndpointIn, &CSW, sizeof CSW) != (int) sizeof CSW)
	{
		return -1;
	}

	if (   CSW.dCSWSignature   != CSWSIGNATURE
	    || CSW.dCSWTag         != m_nCWBTag
	    || CSW.bCSWStatus      != CSWSTATUS_PASSED
	    || CSW.dCSWDataResidue != 0)
	{
		return -1;
	}

	return nCount;
}

int CUSBBulkOnlyMassStorageDevice::TryWrite (const void *pBuffer, unsigned nCount)
{
	assert (pBuffer != 0);

	if (   (m_ullOffset & UMSD_BLOCK_MASK) != 0
	    || m_ullOffset > UMSD_MAX_OFFSET)
	{
		return -1;
	}
	unsigned nBlockAddress = (unsigned) (m_ullOffset >> UMSD_BLOCK_SHIFT);

	if ((nCount & UMSD_BLOCK_MASK) != 0)
	{
		return -1;
	}
	unsigned short usTransferLength = (unsigned short) (nCount >> UMSD_BLOCK_SHIFT);

	//CLogger::Get ()->Write (FromUmsd, LogDebug, "TryWrite %u/0x%X/%u", nBlockAddress, (unsigned) pBuffer, (unsigned) usTransferLength);

	TCBWWrite CBWWrite;

	CBWWrite.CBW.dCWBSignature		= CBWSIGNATURE;
	CBWWrite.CBW.dCWBTag			= ++m_nCWBTag;
	CBWWrite.CBW.dCBWDataTransferLength	= nCount;
	CBWWrite.CBW.bmCBWFlags			= 0;
	CBWWrite.CBW.bCBWLUN			= CBWLUN;
	CBWWrite.CBW.Reserved1			= 0;
	CBWWrite.CBW.bCBWCBLength		= sizeof (TSCSIWrite10);
	CBWWrite.CBW.Reserved2			= 0;

	CBWWrite.SCSIWrite.OperationCode	= SCSI_OP_WRITE;
	CBWWrite.SCSIWrite.Flags		= SCSI_WRITE_FUA;
	CBWWrite.SCSIWrite.LogicalBlockAddress	= le2be32 (nBlockAddress);
	CBWWrite.SCSIWrite.Reserved		= 0;
	CBWWrite.SCSIWrite.TransferLength	= le2be16 (usTransferLength);
	CBWWrite.SCSIWrite.Control		= SCSI_WRITE_CONTROL;

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);
	
	TCSW CSW;

	if (   pHost->Transfer (m_pEndpointOut, &CBWWrite, sizeof CBWWrite) < 0
	    || pHost->Transfer (m_pEndpointOut, (void *) pBuffer, nCount) < 0
	    || pHost->Transfer (m_pEndpointIn, &CSW, sizeof CSW) != (int) sizeof CSW)
	{
		return -1;
	}

	if (   CSW.dCSWSignature   != CSWSIGNATURE
	    || CSW.dCSWTag         != m_nCWBTag
	    || CSW.bCSWStatus      != CSWSTATUS_PASSED
	    || CSW.dCSWDataResidue != 0)
	{
		return -1;
	}

	return nCount;
}

int CUSBBulkOnlyMassStorageDevice::Reset (void)
{
	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);
	
	if (pHost->ControlMessage (GetEndpoint0 (), 0x21, 0xFF, 0, 0x00, 0, 0) < 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogDebug, "Cannot reset device");

		return -1;
	}

	if (pHost->ControlMessage (GetEndpoint0 (), 0x02, 1, 0, 1, 0, 0) < 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogDebug, "Cannot clear halt on endpoint 1");

		return -1;
	}

	if (pHost->ControlMessage (GetEndpoint0 (), 0x02, 1, 0, 2, 0, 0) < 0)
	{
		CLogger::Get ()->Write (FromUmsd, LogDebug, "Cannot clear halt on endpoint 2");

		return -1;
	}

	m_pEndpointIn->ResetPID ();
	m_pEndpointOut->ResetPID ();

	return 0;
}
