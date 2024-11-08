//
// usbmsdgadget.h
//
// USB Mass Storage Gadget by Mike Messinides
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023-2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_gadget_usbmsdgadget_h
#define _circle_usb_gadget_usbmsdgadget_h

#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/gadget/usbmsdgadgetendpoint.h>
#include <circle/usb/usb.h>
#include <circle/interrupt.h>
#include <circle/device.h>
#include <circle/synchronize.h>
#include <circle/macros.h>
#include <circle/types.h>

#define BLOCK_SIZE 512

struct TUSBMSDCBW   //31 bytes
{
	u32 dCBWSignature;
	u32 dCBWTag;
	u32 dCBWDataTransferLength;
	unsigned char bmCBWFlags;
	unsigned char bCBWLUN;
	unsigned char bCBWCBLength;
	unsigned char CBWCB[16];
}
PACKED;

#define SIZE_CBW 31

#define VALID_CBW_SIG 0x43425355
#define CSW_SIG 0x53425355

struct TUSBMSDCSW   //13 bytes
{
	u32 dCSWSignature=CSW_SIG;
	u32 dCSWTag;
	u32 dCSWDataResidue=0;
	unsigned char bmCSWStatus=0; //0=ok 1=command fail 2=phase error
}
PACKED;

#define SIZE_CSW 13

#define MSD_CSW_STATUS_OK 0
#define MSD_CSW_STATUS_FAIL 1
#define MSD_CSW_STATUS_PHASE_ERR 2

//reply to SCSI Request Sense Command 0x3
struct TUSBMSDRequestSenseReply   //14 bytes
{
	unsigned char bErrCode;
	unsigned char bSegNum;
	unsigned char bSenseKey; //=5 command not supported
	unsigned char bInformation[4];
	unsigned char bAddlSenseLen;
	unsigned char bCmdSpecificInfo[4];
	unsigned char bAddlSenseCode;
	unsigned char bAddlSenseCodeQual;
	unsigned char bFieldReplaceUnitCode;
	unsigned char bSKSVetc;
	u16           sFieldPointer;
	u16           sReserved;
}
PACKED;
#define SIZE_RSR 14

//reply to SCSI Inquiry Command 0x12
struct TUSBMSDInquiryReply   //36 bytes
{
	unsigned char bPeriphQualDevType;
	unsigned char bRMB;
	unsigned char bVersion;
	unsigned char bRespDataFormatEtc;
	unsigned char bAddlLength;
	unsigned char bSCCS;
	unsigned char bBQUEetc;
	unsigned char bRELADRetc;
	unsigned char bVendorID[8];
	unsigned char bProdID[16];
	unsigned char bProdRev[4];
}
PACKED;
#define SIZE_INQR 36

//reply to SCSI Mode Sense(6) 0x1A
struct TUSBMSDModeSenseReply   //4 bytes
{
	unsigned char bModeDataLen;
	unsigned char bMedType;
	unsigned char bDevParam;
	unsigned char bBlockDecrLen;
}
PACKED;
#define SIZE_MODEREP 4

//reply to SCSI Read Capacity 0x25
struct TUSBMSDReadCapacityReply   //8 bytes
{
	u32 nLastBlockAddr=0x7F3E0000; //15999
	u8 nSectorSize[4];
}
PACKED;
#define SIZE_READCAPREP 8

struct TUSBMSDFormatCapacityReply   //10 bytes
{
	u8 reserved[3] {0,0,0};
	u8 capListLength = 8;
	u32 numBlocks =0x803E0000;
	u8 descriptor = 2; //formatted media
	u8 blockLen0 = 0;
	u8 blockLen[2];
}
PACKED;
#define SIZE_FORMATR 10



class CUSBMSDGadget : public CDWUSBGadget	/// USB mass storage device gadget
{
public:
	/// \param pInterruptSystem Pointer to the interrupt system object
	/// \param pDevice Pointer to the block device, to be controlled by this gadget
	/// \note pDevice must be initialized yet, when it is specified here.
	/// \note SetDevice() has to be called later, when pDevice is not specified here.
	CUSBMSDGadget (CInterruptSystem *pInterruptSystem, CDevice *pDevice = nullptr);

	~CUSBMSDGadget (void);

	/// \param pDevice Pointer to the block device, to be controlled by this gadget
	/// \note Call this, if pDevice has not been specified in the constructor.
	void SetDevice (CDevice *pDevice);

	/// \brief Call this periodically from TASK_LEVEL to allow I/O operations!
	void Update (void);

	/// \param nBlocks Capacity of the block device in number of blocks (a 512 bytes)
	/// \note Used when the block device does not report its size.
	void SetDeviceBlocks(u64 nBlocks);
	/// \return Capacity of the block device in number of blocks (a 512 bytes)
	u64 GetBlocks (void) const;

protected:
	/// \brief Get device-specific descriptor
	/// \param wValue Parameter from setup packet (descriptor type (MSB) and index (LSB))
	/// \param wIndex Parameter from setup packet (e.g. language ID for string descriptors)
	/// \param pLength Pointer to variable, which receives the descriptor size
	/// \return Pointer to descriptor or nullptr, if not available
	/// \note May override this to personalize device.
	const void *GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength) override;

	/// \brief Convert string to UTF-16 string descriptor
	/// \param pString Pointer to ASCII C-string
	/// \param pLength Pointer to variable, which receives the descriptor size
	/// \return Pointer to string descriptor in class-internal buffer
	const void *ToStringDescriptor (const char *pString, size_t *pLength);

private:
	void AddEndpoints (void) override;

	void CreateDevice (void) override;

	void OnSuspend (void) override;

	int OnClassOrVendorRequest (const TSetupData *pSetupData, u8 *pData) override;

private:
	friend class CUSBMSDGadgetEndpoint;

	void OnTransferComplete (boolean bIn, size_t nLength);

	void OnActivate(); //called from OUT ep

private:
	void HandleSCSICommand();

	void SendCSW();

	void InitDeviceSize(u64 blocks);

private:
	CDevice *m_pDevice;

	enum TEPNumber
	{
		EPIn = 1,
		EPOut  = 2,
		NumEPs
	};

	CUSBMSDGadgetEndpoint *m_pEP[NumEPs];

	u8 m_StringDescriptorBuffer[80];

private:
	static const TUSBDeviceDescriptor s_DeviceDescriptor;

	struct TUSBMSTGadgetConfigurationDescriptor
	{
		TUSBConfigurationDescriptor			Configuration;
		TUSBInterfaceDescriptor				Interface;
		TUSBEndpointDescriptor				EndpointIn;
		TUSBEndpointDescriptor				EndpointOut;
	}
	PACKED;

	static const TUSBMSTGadgetConfigurationDescriptor s_ConfigurationDescriptor;

	static const char *const s_StringDescriptor[];

	enum TMSDState
	{
		Init,
		ReceiveCBW,
		InvalidCBW,
		DataIn,
		DataOut,
		SentCSW,
		SendReqSenseReply,
		DataInRead,
		DataOutWrite
	};

	TMSDState m_nState=Init;

	TUSBMSDCBW m_CBW;
	TUSBMSDCSW m_CSW;

	TUSBMSDInquiryReply m_InqReply {0,0x80,0x04,0x02,0x1F,0,0,0,{'C','i','r','c','l','e',0,0},
					{'M','a','s','s',' ','S','t','o','r','a','g','e',0,0,0,0},
					{'0','0','0',0}};
	TUSBMSDModeSenseReply m_ModeSenseReply {3,0,0,0};
	TUSBMSDReadCapacityReply m_ReadCapReply {0x7F3E0000,{0,0,2,0}};	// last block =15999,
									// each block is 512 bytes
	TUSBMSDRequestSenseReply m_ReqSenseReply;
	TUSBMSDFormatCapacityReply m_FormatCapReply {{0,0,0},8,0x803E0000,2,0,{2,0}};

	static const size_t MaxOutMessageSize = 512;
	static const size_t MaxInMessageSize = 512;
	DMA_BUFFER (u8, m_OutBuffer, MaxOutMessageSize);
	DMA_BUFFER (u8, m_InBuffer, MaxInMessageSize);

	u32 m_nblock_address;
	u32 m_nnumber_blocks;
	u64 m_nDeviceBlocks=0;
	u32 m_nbyteCount;
	boolean m_MSDReady=false;
};

#endif
