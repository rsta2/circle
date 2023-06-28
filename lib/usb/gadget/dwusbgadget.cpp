//
// dwusbgadget.cpp
//
// Limits:
//	Does only support Control EP0 and Bulk EPs.
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/gadget/dwusbgadgetendpoint0.h>
#include <circle/usb/usb.h>
#include <circle/bcmpropertytags.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <assert.h>

// FIFO sizes, in 32-bit words
#define TOTAL_FIFO_SIZE		4080
#define RX_FIFO_SIZE		512
#define NPER_TX_FIFO_SIZE	32
#define DEDICATED_TX_FIFO_SIZE	384	// for each IN EP (7 times)

LOGMODULE ("dwgadget");

CDWUSBGadget::CDWUSBGadget (CInterruptSystem *pInterruptSystem, TDeviceSpeed DeviceSpeed)
:	m_pInterruptSystem (pInterruptSystem),
	m_DeviceSpeed (DeviceSpeed),
	m_State (StatePowered),
	m_bPnPEvent {FALSE, FALSE}
{
	for (unsigned i = 0; i <= NumberOfEPs; i++)
	{
		m_pEP[i] = nullptr;
	}
}

CDWUSBGadget::~CDWUSBGadget (void)
{
	assert (0);
}

boolean CDWUSBGadget::Initialize (boolean bScanDevices)
{
	// Check Vendor ID
	size_t nDescLength;
	const TUSBDeviceDescriptor *pDeviceDesc =
		reinterpret_cast<const TUSBDeviceDescriptor *> (
			GetDescriptor (DESCRIPTOR_DEVICE << 8, 0, &nDescLength));
	assert (pDeviceDesc);
	if (!pDeviceDesc->idVendor)
	{
		LOGERR ("You have to define a unique USB Vendor ID");
		return FALSE;
	}

	PeripheralEntry ();

	CDWHCIRegister VendorId (DWHCI_CORE_VENDOR_ID);
	if (VendorId.Read () != 0x4F54280A)
	{
		LOGERR ("Unknown vendor 0x%0X", VendorId.Get ());
		return FALSE;
	}

	if (!PowerOn ())
	{
		LOGERR ("Cannot power on");
		return FALSE;
	}

	// Disable all interrupts
	CDWHCIRegister AHBConfig (DWHCI_CORE_AHB_CFG);
	AHBConfig.Read ();
	AHBConfig.And (~DWHCI_CORE_AHB_CFG_GLOBALINT_MASK);
	AHBConfig.Write ();

	assert (m_pInterruptSystem != 0);
	m_pInterruptSystem->ConnectIRQ (ARM_IRQ_USB, InterruptStub, this);

	if (!InitCore ())
	{
		LOGERR ("Cannot initialize core");
		return FALSE;
	}

	// Create EPs
	new CDWUSBGadgetEndpoint0 (pDeviceDesc->bMaxPacketSize0, this);

	AddEndpoints ();

	m_State = StateSuspended;

	// Enable all interrupts
	AHBConfig.Read ();
	AHBConfig.Or (DWHCI_CORE_AHB_CFG_GLOBALINT_MASK);
	AHBConfig.Write ();

	PeripheralExit ();

	return TRUE;
}

boolean CDWUSBGadget::UpdatePlugAndPlay (void)
{
	boolean bResult = FALSE;

	if (m_bPnPEvent[PnPEventSuspend])
	{
		m_bPnPEvent[PnPEventSuspend] = FALSE;
		bResult = TRUE;

		// Remove EPs
		OnSuspend ();

		delete m_pEP[0];

		// Disable all interrupts
		CDWHCIRegister AHBConfig (DWHCI_CORE_AHB_CFG);
		AHBConfig.Read ();
		AHBConfig.And (~DWHCI_CORE_AHB_CFG_GLOBALINT_MASK);
		AHBConfig.Write ();

		if (!InitCore ())
		{
			LOGPANIC ("Cannot initialize core");
		}

		// Re-create EPs
		size_t nDescLength;
		const TUSBDeviceDescriptor *pDeviceDesc =
			reinterpret_cast<const TUSBDeviceDescriptor *> (
				GetDescriptor (DESCRIPTOR_DEVICE << 8, 0, &nDescLength));
		assert (pDeviceDesc);
		new CDWUSBGadgetEndpoint0 (pDeviceDesc->bMaxPacketSize0, this);

		AddEndpoints ();

		m_State = StateSuspended;

		// Enable all interrupts
		AHBConfig.Read ();
		AHBConfig.Or (DWHCI_CORE_AHB_CFG_GLOBALINT_MASK);
		AHBConfig.Write ();
	}
	else if (m_bPnPEvent[PnPEventConfigured])
	{
		m_bPnPEvent[PnPEventConfigured] = FALSE;
		bResult = TRUE;

		CreateDevice ();
	}

	return bResult;
}

boolean CDWUSBGadget::PowerOn (void)
{
	CBcmPropertyTags Tags;
	TPropertyTagPowerState PowerState;
	PowerState.nDeviceId = DEVICE_ID_USB_HCD;
	PowerState.nState = POWER_STATE_ON | POWER_STATE_WAIT;
	if (   !Tags.GetTag (PROPTAG_SET_POWER_STATE, &PowerState, sizeof PowerState)
	    ||  (PowerState.nState & POWER_STATE_NO_DEVICE)
	    || !(PowerState.nState & POWER_STATE_ON))
	{
		return FALSE;
	}

	return TRUE;
}

boolean CDWUSBGadget::InitCore (void)
{
	if (!Reset ())
	{
		LOGERR ("Reset failed");
		return FALSE;
	}

	CDWHCIRegister USBConfig (DWHCI_CORE_USB_CFG);
	USBConfig.Read ();
	USBConfig.And (~DWHCI_CORE_USB_CFG_ULPI_UTMI_SEL);	// select UTMI+
	USBConfig.And (~DWHCI_CORE_USB_CFG_PHYIF);		// UTMI width is 8
	USBConfig.Write ();

	CDWHCIRegister AHBConfig (DWHCI_CORE_AHB_CFG);
	AHBConfig.Read ();
	AHBConfig.Or (DWHCI_CORE_AHB_CFG_DMAENABLE);
	AHBConfig.Or (DWHCI_CORE_AHB_CFG_WAIT_AXI_WRITES);
	AHBConfig.And (~DWHCI_CORE_AHB_CFG_MAX_AXI_BURST__MASK);
	//AHBConfig.Or (0 << DWHCI_CORE_AHB_CFG_MAX_AXI_BURST__SHIFT);	// max. AXI burst length 4
	AHBConfig.Write ();

	// HNP and SRP are not used
	USBConfig.Read ();
	USBConfig.And (~DWHCI_CORE_USB_CFG_HNP_CAPABLE);
	USBConfig.And (~DWHCI_CORE_USB_CFG_SRP_CAPABLE);

	// force device mode
	USBConfig.Or (DWHCI_CORE_USB_CFG_FORCE_DEV_MODE);
	USBConfig.Write ();

	CTimer::Get ()->MsDelay (25);

	InitCoreDevice ();

	return TRUE;
}

void CDWUSBGadget::InitCoreDevice (void)
{
	// Restart the PHY clock
	CDWHCIRegister Power (ARM_USB_POWER, 0);
	Power.Write ();

	CTimer::Get ()->MsDelay (40);

	// Set device speed
	CDWHCIRegister DeviceConfig (DWHCI_DEV_CFG);
	DeviceConfig.Read ();
	DeviceConfig.And (~DWHCI_DEV_CFG_DEV_SPEED__MASK);
	DeviceConfig.Or (   (  m_DeviceSpeed == FullSpeed
			     ? DWHCI_DEV_CFG_DEV_SPEED_FS
			     : DWHCI_DEV_CFG_DEV_SPEED_HS)
			 << DWHCI_DEV_CFG_DEV_SPEED__SHIFT);
	DeviceConfig.Write ();

	// Configure data FIFO sizes (dynamic FIFO sizing enabled)
	ASSERT_STATIC (   RX_FIFO_SIZE + NPER_TX_FIFO_SIZE + DEDICATED_TX_FIFO_SIZE*NumberOfInEPs
		       <= TOTAL_FIFO_SIZE);

	CDWHCIRegister RxFIFOSize (DWHCI_CORE_RX_FIFO_SIZ, RX_FIFO_SIZE);
	RxFIFOSize.Write ();

	CDWHCIRegister NonPeriodicTxFIFOSize (DWHCI_CORE_NPER_TX_FIFO_SIZ, 0);
	NonPeriodicTxFIFOSize.Or (RX_FIFO_SIZE);
	NonPeriodicTxFIFOSize.Or (NPER_TX_FIFO_SIZE << 16);
	NonPeriodicTxFIFOSize.Write ();

	for (unsigned i = 0; i < NumberOfInEPs; i++)
	{
		CDWHCIRegister TxFIFOSize (DWHCI_CORE_DEV_TX_FIFO (i), 0);
		TxFIFOSize.Or (RX_FIFO_SIZE + NPER_TX_FIFO_SIZE + DEDICATED_TX_FIFO_SIZE*i);
		TxFIFOSize.Or (DEDICATED_TX_FIFO_SIZE << 16);
		TxFIFOSize.Write ();
		TxFIFOSize.Read ();
	}

	CDWHCIRegister DataFIFOConfig (DWHCI_CORE_DFIFO_CFG, 0);
	DataFIFOConfig.Or (TOTAL_FIFO_SIZE << DWHCI_CORE_DFIFO_CFG_FIFO_CFG__SHIFT);
	DataFIFOConfig.Or (   (  RX_FIFO_SIZE + NPER_TX_FIFO_SIZE
			       + DEDICATED_TX_FIFO_SIZE*NumberOfInEPs)
			   << DWHCI_CORE_DFIFO_CFG_EPINFO_BASE__SHIFT);
	DataFIFOConfig.Write ();

	FlushTxFIFO (0x10);	 	// Flush all TX FIFOs
	FlushRxFIFO ();

	// Flush the learning queue
	CDWHCIRegister Reset (DWHCI_CORE_RESET, DWHCI_CORE_RESET_IN_TOKEN_QUEUE_FLUSH);
	Reset.Write ();

	// Clear all device interrupts (multiprocessing support is not used)
	for (unsigned i = 0; i < NumberOfInEPs; i++)
	{
		CDWHCIRegister EachInEPIntMask (DWHCI_DEV_EACH_IN_EP_INT_MASK (i), 0);
		EachInEPIntMask.Write ();
	}

	for (unsigned i = 0; i < NumberOfOutEPs; i++)
	{
		CDWHCIRegister EachOutEPIntMask (DWHCI_DEV_EACH_OUT_EP_INT_MASK (i), 0);
		EachOutEPIntMask.Write ();
	}

	CDWHCIRegister EachEPInterrupt (DWHCI_DEV_EACH_EP_INT, ~0U);
	EachEPInterrupt.Write ();
	CDWHCIRegister EachEPInterruptMask (DWHCI_DEV_EACH_EP_INT, 0);
	EachEPInterruptMask.Write ();

	// Initialize all IN EP registers
	for (unsigned i = 0; i <= NumberOfInEPs; i++)
	{
		CDWHCIRegister InEPCtrl (DWHCI_DEV_IN_EP_CTRL (i), 0);
		if (InEPCtrl.Read () & DWHCI_DEV_EP_CTRL_EP_ENABLE)
		{
			InEPCtrl.Or (DWHCI_DEV_EP_CTRL_EP_DISABLE);
			InEPCtrl.Or (DWHCI_DEV_EP_CTRL_SET_NAK);
		}
		InEPCtrl.Write ();

		CDWHCIRegister InEPXferSize (DWHCI_DEV_IN_EP_XFER_SIZ (i), 0);
		InEPXferSize.Write ();

		CDWHCIRegister InEPDMAAddress (DWHCI_DEV_IN_EP_DMA_ADDR (i), 0);
		InEPDMAAddress.Write ();

		CDWHCIRegister InEPInterrupt (DWHCI_DEV_IN_EP_INT (i), 0xFF);
		InEPInterrupt.Write ();
	}

	// Initialize all OUT EP registers
	for (unsigned i = 0; i <= NumberOfOutEPs; i++)
	{
		CDWHCIRegister OutEPCtrl (DWHCI_DEV_OUT_EP_CTRL (i), 0);
		if (OutEPCtrl.Read () & DWHCI_DEV_EP_CTRL_EP_ENABLE)
		{
			CDWHCIRegister DeviceCtrl (DWHCI_DEV_CTRL);
			DeviceCtrl.Read ();
			DeviceCtrl.Set (DWHCI_DEV_CTRL_SET_GLOBAL_OUT_NAK);
			DeviceCtrl.Write ();

			CDWHCIRegister IntStatus (DWHCI_CORE_INT_STAT);
			WaitForBit (&IntStatus, DWHCI_CORE_INT_STAT_GLOBAL_OUT_NAK_EFF, TRUE, 1000);
			IntStatus.Set (DWHCI_CORE_INT_STAT_GLOBAL_OUT_NAK_EFF);
			IntStatus.Write ();

			OutEPCtrl.ClearAll ();
			OutEPCtrl.Or (DWHCI_DEV_EP_CTRL_EP_DISABLE);
			OutEPCtrl.Or (DWHCI_DEV_EP_CTRL_SET_NAK);
			OutEPCtrl.Write ();

			CDWHCIRegister OutEPInterrupt (DWHCI_DEV_OUT_EP_INT (i));
			WaitForBit (&OutEPInterrupt, DWHCI_DEV_OUT_EP_INT_EP_DISABLED, TRUE, 1000);
			OutEPInterrupt.Set (DWHCI_DEV_OUT_EP_INT_EP_DISABLED);
			OutEPInterrupt.Write ();

			DeviceCtrl.Read ();
			DeviceCtrl.Set (DWHCI_DEV_CTRL_CLEAR_GLOBAL_OUT_NAK);
			DeviceCtrl.Write ();
		}
		OutEPCtrl.Write ();

		CDWHCIRegister OutEPXferSize (DWHCI_DEV_OUT_EP_XFER_SIZ (i), 0);
		OutEPXferSize.Write ();

		CDWHCIRegister OutEPDMAAddress (DWHCI_DEV_OUT_EP_DMA_ADDR (i), 0);
		OutEPDMAAddress.Write ();

		CDWHCIRegister OutEPInterrupt (DWHCI_DEV_OUT_EP_INT (i), 0xFF);
		OutEPInterrupt.Write ();
	}

	EnableDeviceInterrupts ();

	CDWHCIRegister InEPCommonIntMask (DWHCI_DEV_IN_EP_COMMON_INT_MASK);
	InEPCommonIntMask.Read ();
	InEPCommonIntMask.Or (DWHCI_DEV_IN_EP_COMMON_INT_MASK_FIFO_UNDERRUN);
	InEPCommonIntMask.Write ();
}

boolean CDWUSBGadget::Reset (void)
{
	CDWHCIRegister Reset (DWHCI_CORE_RESET, 0);

	// wait for AHB master IDLE state
	if (!WaitForBit (&Reset, DWHCI_CORE_RESET_AHB_IDLE, TRUE, 100))
	{
		return FALSE;
	}

	// core soft reset
	Reset.Or (DWHCI_CORE_RESET_SOFT_RESET);
	Reset.Write ();

	if (!WaitForBit (&Reset, DWHCI_CORE_RESET_SOFT_RESET, FALSE, 10))
	{
		return FALSE;
	}

	CTimer::Get ()->MsDelay (100);

	return TRUE;
}

void CDWUSBGadget::EnableDeviceInterrupts (void)
{
	CDWHCIRegister IntMask (DWHCI_CORE_INT_MASK, 0);	// Disable all interrupts
	IntMask.Write ();
	IntMask.Read ();

	CDWHCIRegister IntStatus (DWHCI_CORE_INT_STAT);		// Clear any pending interrupts
	IntStatus.SetAll ();
	IntStatus.Write ();

	// Enable interrupts
	IntMask.Or (DWHCI_CORE_INT_MASK_USB_SUSPEND);
	IntMask.Or (DWHCI_CORE_INT_MASK_USB_RESET_INTR);
	IntMask.Or (DWHCI_CORE_INT_MASK_ENUM_DONE);
	IntMask.Or (DWHCI_CORE_INT_MASK_IN_EP_INTR);
	IntMask.Or (DWHCI_CORE_INT_MASK_OUT_EP_INTR);
	IntMask.Or (DWHCI_CORE_INT_MASK_IN_EP_MISMATCH);
	IntMask.Write ();
}

void CDWUSBGadget::FlushTxFIFO (unsigned nFIFO)
{
	CDWHCIRegister Reset (DWHCI_CORE_RESET, 0);
	Reset.Or (DWHCI_CORE_RESET_TX_FIFO_FLUSH);
	Reset.And (~DWHCI_CORE_RESET_TX_FIFO_NUM__MASK);
	Reset.Or (nFIFO << DWHCI_CORE_RESET_TX_FIFO_NUM__SHIFT);
	Reset.Write ();

	if (!WaitForBit (&Reset, DWHCI_CORE_RESET_TX_FIFO_FLUSH, FALSE, 10))
	{
		return;
	}

	CTimer::Get ()->usDelay (1);		// Wait for 3 PHY clocks
}

void CDWUSBGadget::FlushRxFIFO (void)
{
	CDWHCIRegister Reset (DWHCI_CORE_RESET, 0);
	Reset.Or (DWHCI_CORE_RESET_RX_FIFO_FLUSH);
	Reset.Write ();

	if (!WaitForBit (&Reset, DWHCI_CORE_RESET_RX_FIFO_FLUSH, FALSE, 10))
	{
		return;
	}

	CTimer::Get ()->usDelay (1);		// Wait for 3 PHY clocks
}

boolean CDWUSBGadget::WaitForBit (CDWHCIRegister *pRegister,
				  u32		  nMask,
				  boolean	  bWaitUntilSet,
				  unsigned	  nMsTimeout)
{
	assert (pRegister != 0);
	assert (nMask != 0);
	assert (nMsTimeout > 0);

	while ((pRegister->Read () & nMask) ? !bWaitUntilSet : bWaitUntilSet)
	{
		CTimer::Get ()->MsDelay (1);

		if (--nMsTimeout == 0)
		{
			LOGWARN ("Timeout");
#ifndef NDEBUG
			pRegister->Dump ();
#endif
			return FALSE;
		}
	}

	return TRUE;
}

void CDWUSBGadget::HandleUSBSuspend (void)
{
#ifdef USB_GADGET_DEBUG
	LOGDBG ("USB suspend");
#endif

	m_bPnPEvent[PnPEventSuspend] = TRUE;

	for (unsigned i = 0; i <= NumberOfEPs; i++)
	{
		if (m_pEP[i])
		{
			m_pEP[i]->OnSuspend ();
		}
	}

	CDWHCIRegister IntStatus (DWHCI_CORE_INT_STAT, DWHCI_CORE_INT_MASK_USB_SUSPEND);
	IntStatus.Write ();
}

void CDWUSBGadget::HandleUSBReset (void)
{
#ifdef USB_GADGET_DEBUG
	LOGDBG ("USB reset");
#endif

	// Set NAK for all OUT EPs
	for (unsigned i = 0; i <= NumberOfOutEPs; i++)
	{
		CDWHCIRegister OutEPCtrl (DWHCI_DEV_OUT_EP_CTRL (i), 0);
		OutEPCtrl.Or (DWHCI_DEV_EP_CTRL_SET_NAK);
		OutEPCtrl.Write ();
	}

	FlushTxFIFO (0x10);

	// Flush learning queue
	CDWHCIRegister Reset (DWHCI_CORE_RESET, DWHCI_CORE_RESET_IN_TOKEN_QUEUE_FLUSH);
	Reset.Write ();

	// Initialize next EP sequence
	CDWUSBGadgetEndpoint::HandleUSBReset ();

	CDWHCIRegister InEP0Ctrl (DWHCI_DEV_IN_EP_CTRL (0));
	InEP0Ctrl.Read ();
	InEP0Ctrl.And (~DWHCI_DEV_IN_EP_CTRL_NEXT_EP__MASK);
	InEP0Ctrl.Write ();

	// Update IN EP mismatch count by active IN NP EP count + 1
	CDWHCIRegister DeviceConfig (DWHCI_DEV_CFG);
	DeviceConfig.Read ();
	DeviceConfig.And (~DWHCI_DEV_CFG_EP_MISMATCH_COUNT__MASK);
	DeviceConfig.Or (2 << DWHCI_DEV_CFG_EP_MISMATCH_COUNT__SHIFT);
	DeviceConfig.Write ();

	// Enable common OUT EP interrupts
	CDWHCIRegister OutEPCommonIntMask (DWHCI_DEV_OUT_EP_COMMON_INT_MASK,
					     DWHCI_DEV_OUT_EP_INT_SETUP_DONE
					   | DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE
					   | DWHCI_DEV_OUT_EP_INT_AHB_ERROR
					   | DWHCI_DEV_OUT_EP_INT_EP_DISABLED);
	OutEPCommonIntMask.Write ();

	// Enable common IN EP interrupts
	CDWHCIRegister InEPCommonIntMask (DWHCI_DEV_IN_EP_COMMON_INT_MASK,
					    DWHCI_DEV_IN_EP_INT_XFER_COMPLETE
					  | DWHCI_DEV_IN_EP_INT_TIMEOUT
					  | DWHCI_DEV_IN_EP_INT_AHB_ERROR
					  | DWHCI_DEV_IN_EP_INT_EP_DISABLED);
	InEPCommonIntMask.Write ();

	SetDeviceAddress (0);

	// Notify all EPs about USB reset
	for (unsigned i = 0; i <= NumberOfEPs; i++)
	{
		if (m_pEP[i])
		{
			m_pEP[i]->OnUSBReset ();
		}
	}

	m_State = StateResetDone;

	CDWHCIRegister IntStatus (DWHCI_CORE_INT_STAT, DWHCI_CORE_INT_MASK_USB_RESET_INTR);
	IntStatus.Write ();
}

void CDWUSBGadget::HandleEnumerationDone (void)
{
#ifdef USB_GADGET_DEBUG
	LOGDBG ("Enumeration done");
#endif

	if (m_State != StateResetDone)
	{
		return;
	}

	CDWHCIRegister USBConfig (DWHCI_CORE_USB_CFG);
	USBConfig.Read ();
	USBConfig.And (~DWHCI_CORE_USB_CFG_TURNAROUND_TIME__MASK);
	USBConfig.Or (9 << DWHCI_CORE_USB_CFG_TURNAROUND_TIME__SHIFT);
	USBConfig.Write ();

	assert (m_pEP[0]);
	m_pEP[0]->OnActivate ();

	m_State = StateEnumDone;

	CDWHCIRegister IntStatus (DWHCI_CORE_INT_STAT, DWHCI_CORE_INT_MASK_ENUM_DONE);
	IntStatus.Write ();
}

void CDWUSBGadget::HandleInEPInterrupt (void)
{
#ifdef USB_GADGET_DEBUG
	LOGDBG ("In EP interrupt");
#endif

	assert (   m_State == StateEnumDone
		|| m_State == StateConfigured);

	CDWHCIRegister AllEPsIntStat (DWHCI_DEV_ALL_EPS_INT_STAT);
	CDWHCIRegister AllEPsIntMask (DWHCI_DEV_ALL_EPS_INT_MASK);
	u32 nInEPStat = (AllEPsIntStat.Read () & AllEPsIntMask.Read ()) & 0xFFFF;

	for (unsigned nEP = 0; nInEPStat; nInEPStat >>= 1, nEP++)
	{
		if (nInEPStat & 1)
		{
			assert (nEP <= NumberOfEPs);
			assert (m_pEP[nEP]);
			m_pEP[nEP]->HandleInInterrupt ();
		}
	}
}

void CDWUSBGadget::HandleOutEPInterrupt (void)
{
#ifdef USB_GADGET_DEBUG
	LOGDBG ("Out EP interrupt");
#endif

	assert (   m_State == StateEnumDone
		|| m_State == StateConfigured);

	CDWHCIRegister AllEPsIntStat (DWHCI_DEV_ALL_EPS_INT_STAT);
	CDWHCIRegister AllEPsIntMask (DWHCI_DEV_ALL_EPS_INT_MASK);
	u32 nOutEPStat = (AllEPsIntStat.Read () & AllEPsIntMask.Read ()) >> 16;

	for (unsigned nEP = 0; nOutEPStat; nOutEPStat >>= 1, nEP++)
	{
		if (nOutEPStat & 1)
		{
			assert (nEP <= NumberOfEPs);
			assert (m_pEP[nEP]);
			m_pEP[nEP]->HandleOutInterrupt ();
		}
	}
}

void CDWUSBGadget::InterruptHandler (void)
{
	PeripheralEntry ();

	CDWHCIRegister IntStatus (DWHCI_CORE_INT_STAT);
	CDWHCIRegister IntMask (DWHCI_CORE_INT_MASK);
	u32 nIntStatus = IntStatus.Read () & IntMask.Read ();

#ifdef USB_GADGET_DEBUG
	LOGDBG ("IRQ (status 0x%08X)", nIntStatus);
#endif

	if (nIntStatus & DWHCI_CORE_INT_MASK_USB_SUSPEND)
	{
		HandleUSBSuspend ();
	}

	if (nIntStatus & DWHCI_CORE_INT_MASK_USB_RESET_INTR)
	{
		HandleUSBReset ();
	}

	if (nIntStatus & DWHCI_CORE_INT_MASK_ENUM_DONE)
	{
		HandleEnumerationDone ();
	}

	if (nIntStatus & DWHCI_CORE_INT_MASK_OUT_EP_INTR)
	{
		HandleOutEPInterrupt ();
	}

	if (nIntStatus & DWHCI_CORE_INT_MASK_IN_EP_INTR)
	{
		HandleInEPInterrupt ();
	}

	assert (!(nIntStatus & DWHCI_CORE_INT_MASK_IN_EP_MISMATCH));

	PeripheralExit ();
}

void CDWUSBGadget::InterruptStub (void *pParam)
{
	CDWUSBGadget *pThis = static_cast<CDWUSBGadget *> (pParam);
	assert (pThis != 0);

	pThis->InterruptHandler ();
}

void CDWUSBGadget::AssignEndpoint (unsigned nEP, CDWUSBGadgetEndpoint *pEP)
{
	assert (nEP <= NumberOfEPs);

	if (m_pEP[nEP])
	{
		LOGPANIC ("An endpoint with number %u already exists", nEP);
	}

	m_pEP[nEP] = pEP;
	assert (m_pEP[nEP]);
}

void CDWUSBGadget::RemoveEndpoint (unsigned nEP)
{
	assert (nEP <= NumberOfEPs);
	assert (m_pEP[nEP]);
	m_pEP[nEP] = nullptr;
}

void CDWUSBGadget::SetDeviceAddress (u8 uchAddress)
{
#ifdef USB_GADGET_DEBUG
	LOGDBG ("Set device address to %u", uchAddress);
#endif

	CDWHCIRegister DeviceConfig (DWHCI_DEV_CFG);
	DeviceConfig.Read ();
	DeviceConfig.And (~DWHCI_DEV_CFG_DEV_ADDR__MASK);
	DeviceConfig.Or (uchAddress << DWHCI_DEV_CFG_DEV_ADDR__SHIFT);
	DeviceConfig.Write ();
}

boolean CDWUSBGadget::SetConfiguration (u8 uchConfiguration)
{
	if (uchConfiguration != 1)
	{
		return FALSE;
	}

	assert (m_State == StateEnumDone);
	m_State = StateConfigured;

	unsigned nEPCount = 0;
	for (unsigned i = 1; i <= NumberOfEPs; i++)
	{
		if (m_pEP[i])
		{
			m_pEP[i]->OnActivate ();

			nEPCount++;
		}
	}

	LOGNOTE ("%u Endpoint(s) activated", nEPCount);

	m_bPnPEvent[PnPEventConfigured] = TRUE;

	return TRUE;
}
