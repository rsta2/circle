//
// smsc951x.cpp
//
// Information to implement this driver was taken
// from the Linux smsc95xx driver which is:
// 	Copyright (C) 2007-2008 SMSC
// and the Embedded Xinu SMSC LAN9512 USB driver which is:
//	Copyright (c) 2008, Douglas Comer and Dennis Brylow
// See the file lib/usb/README for details!
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/smsc951x.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/bcmpropertytags.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <circle/debug.h>
#include <assert.h>

// USB vendor requests
#define WRITE_REGISTER			0xA0
#define READ_REGISTER			0xA1

// Registers
#define ID_REV				0x00
#define INT_STS				0x08
#define RX_CFG				0x0C
#define TX_CFG				0x10
	#define TX_CFG_ON			0x00000004
#define HW_CFG				0x14
	#define HW_CFG_BIR			0x00001000
#define RX_FIFO_INF			0x18
#define PM_CTRL				0x20
#define LED_GPIO_CFG			0x24
	#define LED_GPIO_CFG_SPD_LED		0x01000000
	#define LED_GPIO_CFG_LNK_LED		0x00100000
	#define LED_GPIO_CFG_FDX_LED		0x00010000
#define GPIO_CFG			0x28
#define AFC_CFG				0x2C
#define E2P_CMD				0x30
#define E2P_DATA			0x34
#define BURST_CAP			0x38
#define GPIO_WAKE			0x64
#define INT_EP_CTL			0x68
#define BULK_IN_DLY			0x6C
#define MAC_CR				0x100
	#define MAC_CR_RCVOWN			0x00800000
	#define MAC_CR_MCPAS			0x00080000
	#define MAC_CR_PRMS			0x00040000
	#define MAC_CR_BCAST			0x00000800
	#define MAC_CR_TXEN			0x00000008
	#define MAC_CR_RXEN			0x00000004
#define ADDRH				0x104
#define ADDRL				0x108
#define HASHH				0x10C
#define HASHL				0x110
#define MII_ADDR			0x114
	#define MII_BUSY			0x01
	#define MII_WRITE			0x02
	#define PHY_ID_MASK			0x1F
	#define PHY_ID_INTERNAL			0x01
	#define REG_NUM_MASK			0x1F
#define MII_DATA			0x118
#define FLOW				0x11C
#define VLAN1				0x120
#define VLAN2				0x124
#define WUFF				0x128
#define WUCSR				0x12C
#define COE_CR				0x130

// TX commands (first two 32-bit words in buffer)
#define TX_CMD_A_DATA_OFFSET		0x001F0000
#define TX_CMD_A_FIRST_SEG		0x00002000
#define TX_CMD_A_LAST_SEG		0x00001000
#define TX_CMD_A_BUF_SIZE		0x000007FF

#define TX_CMD_B_CSUM_ENABLE		0x00004000
#define TX_CMD_B_ADD_CRC_DISABLE	0x00002000
#define TX_CMD_B_DISABLE_PADDING	0x00001000
#define TX_CMD_B_PKT_BYTE_LENGTH	0x000007FF

// RX status (first 32-bit word in buffer)
#define RX_STS_FF			0x40000000	// Filter Fail
#define RX_STS_FL			0x3FFF0000	// Frame Length
	#define RX_STS_FRAMELEN(reg)	(((reg) & RX_STS_FL) >> 16)
#define RX_STS_ES			0x00008000	// Error Summary
#define RX_STS_BF			0x00002000	// Broadcast Frame
#define RX_STS_LE			0x00001000	// Length Error
#define RX_STS_RF			0x00000800	// Runt Frame
#define RX_STS_MF			0x00000400	// Multicast Frame
#define RX_STS_TL			0x00000080	// Frame too long
#define RX_STS_CS			0x00000040	// Collision Seen
#define RX_STS_FT			0x00000020	// Frame Type
#define RX_STS_RW			0x00000010	// Receive Watchdog
#define RX_STS_ME			0x00000008	// Mii Error
#define RX_STS_DB			0x00000004	// Dribbling
#define RX_STS_CRC			0x00000002	// CRC Error

#define RX_STS_ERROR			(  RX_STS_FF	\
					 | RX_STS_ES	\
					 | RX_STS_LE	\
					 | RX_STS_TL	\
					 | RX_STS_CS	\
					 | RX_STS_RW	\
					 | RX_STS_ME	\
					 | RX_STS_DB	\
					 | RX_STS_CRC)

static const char FromSMSC951x[] = "smsc951x";

CSMSC951xDevice::CSMSC951xDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pEndpointBulkIn (0),
	m_pEndpointBulkOut (0)
{
}

CSMSC951xDevice::~CSMSC951xDevice (void)
{
	delete m_pEndpointBulkOut;
	m_pEndpointBulkOut = 0;

	delete m_pEndpointBulkIn;
	m_pEndpointBulkIn = 0;
}

boolean CSMSC951xDevice::Configure (void)
{
	CBcmPropertyTags Tags;
	TPropertyTagMACAddress MACAddress;
	if (Tags.GetTag (PROPTAG_GET_MAC_ADDRESS, &MACAddress, sizeof MACAddress))
	{
		m_MACAddress.Set (MACAddress.Address);
	}
	else
	{
		CLogger::Get ()->Write (FromSMSC951x, LogError, "Cannot get MAC address");

		return FALSE;
	}
	CString MACString;
	m_MACAddress.Format (&MACString);
	CLogger::Get ()->Write (FromSMSC951x, LogDebug, "MAC address is %s", (const char *) MACString);

	if (GetNumEndpoints () != 3)
	{
		ConfigurationError (FromSMSC951x);

		return FALSE;
	}

	const TUSBEndpointDescriptor *pEndpointDesc;
	while ((pEndpointDesc = (TUSBEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT)) != 0)
	{
		if ((pEndpointDesc->bmAttributes & 0x3F) == 0x02)		// Bulk
		{
			if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x80)	// Input
			{
				if (m_pEndpointBulkIn != 0)
				{
					ConfigurationError (FromSMSC951x);

					return FALSE;
				}

				m_pEndpointBulkIn = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
			else							// Output
			{
				if (m_pEndpointBulkOut != 0)
				{
					ConfigurationError (FromSMSC951x);

					return FALSE;
				}

				m_pEndpointBulkOut = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
		}
	}

	if (   m_pEndpointBulkIn  == 0
	    || m_pEndpointBulkOut == 0)
	{
		ConfigurationError (FromSMSC951x);

		return FALSE;
	}

	if (!CUSBFunction::Configure ())
	{
		CLogger::Get ()->Write (FromSMSC951x, LogError, "Cannot set interface");

		return FALSE;
	}

	u8 MACAddressBuffer[MAC_ADDRESS_SIZE];
	m_MACAddress.CopyTo (MACAddressBuffer);
	u16 usMACAddressHigh =   (u16) MACAddressBuffer[4]
			       | (u16) MACAddressBuffer[5] << 8;
	u32 nMACAddressLow   =   (u32) MACAddressBuffer[0]
			       | (u32) MACAddressBuffer[1] << 8
			       | (u32) MACAddressBuffer[2] << 16
			       | (u32) MACAddressBuffer[3] << 24;
	if (   !WriteReg (ADDRH, usMACAddressHigh)
	    || !WriteReg (ADDRL, nMACAddressLow))
	{
		CLogger::Get ()->Write (FromSMSC951x, LogError, "Cannot set MAC address");

		return FALSE;
	}

	if (   !WriteReg (LED_GPIO_CFG,   LED_GPIO_CFG_SPD_LED
					| LED_GPIO_CFG_LNK_LED
					| LED_GPIO_CFG_FDX_LED)
	    || !WriteReg (MAC_CR,  MAC_CR_RCVOWN
				 //| MAC_CR_PRMS		// promiscous mode
				 | MAC_CR_TXEN
				 | MAC_CR_RXEN)
	    || !WriteReg (TX_CFG, TX_CFG_ON))
	{
		CLogger::Get ()->Write (FromSMSC951x, LogError, "Cannot start device");

		return FALSE;
	}

	AddNetDevice ();

	return TRUE;
}

const CMACAddress *CSMSC951xDevice::GetMACAddress (void) const
{
	return &m_MACAddress;
}

boolean CSMSC951xDevice::SendFrame (const void *pBuffer, unsigned nLength)
{
	if (nLength > FRAME_BUFFER_SIZE)
	{
		return FALSE;
	}

	DMA_BUFFER (u8, TxBuffer, FRAME_BUFFER_SIZE+8);
	assert (pBuffer != 0);
	memcpy (TxBuffer+8, pBuffer, nLength);

	u32 *pTxHeader = (u32 *) TxBuffer;
	pTxHeader[0] = TX_CMD_A_FIRST_SEG | TX_CMD_A_LAST_SEG | nLength;
	pTxHeader[1] = nLength;
	
	assert (m_pEndpointBulkOut != 0);
	return GetHost ()->Transfer (m_pEndpointBulkOut, TxBuffer, nLength+8) >= 0;
}

boolean CSMSC951xDevice::ReceiveFrame (void *pBuffer, unsigned *pResultLength)
{
	assert (m_pEndpointBulkIn != 0);
	assert (pBuffer != 0);
	CUSBRequest URB (m_pEndpointBulkIn, pBuffer, FRAME_BUFFER_SIZE);

	if (!GetHost ()->SubmitBlockingRequest (&URB))
	{
		return FALSE;
	}

	u32 nResultLength = URB.GetResultLength ();
	if (nResultLength < 4)				// should not happen with HW_CFG_BIR set
	{
		return FALSE;
	}

	u32 nRxStatus = *(u32 *) pBuffer;
	if (nRxStatus & RX_STS_ERROR)
	{
		CLogger::Get ()->Write (FromSMSC951x, LogWarning, "RX error (status 0x%X)", nRxStatus);

		return FALSE;
	}
	
	u32 nFrameLength = RX_STS_FRAMELEN (nRxStatus);
	assert (nFrameLength == nResultLength-4);
	assert (nFrameLength > 4);
	if (nFrameLength <= 4)
	{
		return FALSE;
	}
	nFrameLength -= 4;	// ignore CRC

	//CLogger::Get ()->Write (FromSMSC951x, LogDebug, "Frame received (status 0x%X)", nRxStatus);

	memcpy (pBuffer, (u8 *) pBuffer + 4, nFrameLength);	// overwrite RX status

	assert (pResultLength != 0);
	*pResultLength = nFrameLength;
	
	return TRUE;
}

boolean CSMSC951xDevice::IsLinkUp (void)
{
	u16 usPHYModeStatus;
	if (!PHYRead (0x01, &usPHYModeStatus))
	{
		return FALSE;
	}

	return usPHYModeStatus & (1 << 2) ? TRUE : FALSE;
}

TNetDeviceSpeed CSMSC951xDevice::GetLinkSpeed (void)
{
	u16 usPHYSpecialControlStatus;
	if (!PHYRead (0x1F, &usPHYSpecialControlStatus))
	{
		return NetDeviceSpeedUnknown;
	}

	// check for Auto-negotiation done
	if (!(usPHYSpecialControlStatus & (1 << 12)))
	{
		return NetDeviceSpeedUnknown;
	}

	switch ((usPHYSpecialControlStatus >> 2) & 7)
	{
	case 0b001:	return NetDeviceSpeed10Half;
	case 0b010:	return NetDeviceSpeed100Half;
	case 0b101:	return NetDeviceSpeed10Full;
	case 0b110:	return NetDeviceSpeed100Full;
	default:	return NetDeviceSpeedUnknown;
	}
}

boolean CSMSC951xDevice::PHYWrite (u8 uchIndex, u16 usValue)
{
	assert (uchIndex <= 31);

	if (   !PHYWaitNotBusy ()
	    || !WriteReg (MII_DATA, usValue))
	{
		return FALSE;
	}

	u32 nMIIAddress = (PHY_ID_INTERNAL << 11) | ((u32) uchIndex << 6);
	if (!WriteReg (MII_ADDR, nMIIAddress | MII_WRITE | MII_BUSY))
	{
		return FALSE;
	}

	return PHYWaitNotBusy ();
}

boolean CSMSC951xDevice::PHYRead (u8 uchIndex, u16 *pValue)
{
	assert (uchIndex <= 31);

	if (!PHYWaitNotBusy ())
	{
		return FALSE;
	}

	u32 nMIIAddress = (PHY_ID_INTERNAL << 11) | ((u32) uchIndex << 6);
	u32 nValue;
	if (   !WriteReg (MII_ADDR, nMIIAddress | MII_BUSY)
	    || !PHYWaitNotBusy ()
	    || !ReadReg (MII_DATA, &nValue))
	{
		return FALSE;
	}

	assert (pValue != 0);
	*pValue = nValue & 0xFFFF;

	return TRUE;
}

boolean CSMSC951xDevice::PHYWaitNotBusy (void)
{
	CTimer *pTimer = CTimer::Get ();
	assert (pTimer != 0);

	unsigned nStartTicks = pTimer->GetTicks ();

	u32 nValue;
	do
	{
		if (pTimer->GetTicks () - nStartTicks >= HZ)
		{
			return FALSE;
		}

		if (!ReadReg (MII_ADDR, &nValue))
		{
			return FALSE;
		}
	}
	while (nValue & MII_BUSY);

	return TRUE;
}

boolean CSMSC951xDevice::WriteReg (u32 nIndex, u32 nValue)
{
	return GetHost ()->ControlMessage (GetEndpoint0 (),
					   REQUEST_OUT | REQUEST_VENDOR, WRITE_REGISTER,
					   0, nIndex, &nValue, sizeof nValue) >= 0;
}

boolean CSMSC951xDevice::ReadReg (u32 nIndex, u32 *pValue)
{
	return GetHost ()->ControlMessage (GetEndpoint0 (),
					   REQUEST_IN | REQUEST_VENDOR, READ_REGISTER,
					   0, nIndex, pValue, sizeof *pValue) == (int) sizeof *pValue;
}

#ifndef NDEBUG

void CSMSC951xDevice::DumpReg (const char *pName, u32 nIndex)
{
	u32 nValue;
	if (!ReadReg (nIndex, &nValue))
	{
		CLogger::Get ()->Write (FromSMSC951x, LogError, "Cannot read register 0x%X", nIndex);

		return;
	}

	CLogger::Get ()->Write (FromSMSC951x, LogDebug, "%08X %s", nValue, pName);
}

void CSMSC951xDevice::DumpRegs (void)
{
	DumpReg ("ID_REV",       ID_REV);
	DumpReg ("INT_STS",      INT_STS);
	DumpReg ("RX_CFG",       RX_CFG);
	DumpReg ("TX_CFG",       TX_CFG);
	DumpReg ("HW_CFG",       HW_CFG);
	DumpReg ("RX_FIFO_INF",  RX_FIFO_INF);
	DumpReg ("PM_CTRL",      PM_CTRL);
	DumpReg ("LED_GPIO_CFG", LED_GPIO_CFG);
	DumpReg ("GPIO_CFG",     GPIO_CFG);
	DumpReg ("AFC_CFG",      AFC_CFG);
	DumpReg ("BURST_CAP",    BURST_CAP);
	DumpReg ("INT_EP_CTL",   INT_EP_CTL);
	DumpReg ("BULK_IN_DLY",  BULK_IN_DLY);
	DumpReg ("MAC_CR",       MAC_CR);
	DumpReg ("ADDRH",        ADDRH);
	DumpReg ("ADDRL",        ADDRL);
	DumpReg ("HASHH",        HASHH);
	DumpReg ("HASHL",        HASHL);
	DumpReg ("FLOW",         FLOW);
	DumpReg ("WUCSR",        WUCSR);
}

#endif
