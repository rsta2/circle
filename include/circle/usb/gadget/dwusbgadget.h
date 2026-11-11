//
// dwusbgadget.h
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
#ifndef _circle_usb_gadget_dwusbgadget_h
#define _circle_usb_gadget_dwusbgadget_h

#include <circle/usb/usbcontroller.h>
#include <circle/usb/gadget/dwusbgadgetendpoint.h>
#include <circle/usb/dwhciregister.h>
#include <circle/usb/usb.h>
#include <circle/interrupt.h>
#include <circle/types.h>

class CDWUSBGadget : public CUSBController	/// DW USB gadget on Raspberry Pi (3)A(+), Zero (2) (W), 4B
{
public:
	enum TDeviceSpeed
	{
		FullSpeed,
		HighSpeed,
		DeviceSpeedUnknown
	};

public:
	/// \param pInterruptSystem Pointer to the interrupt system
	/// \param DeviceSpeed Enumeration speed of this USB gadget
	CDWUSBGadget (CInterruptSystem *pInterruptSystem, TDeviceSpeed DeviceSpeed);

	virtual ~CDWUSBGadget (void);

	/// \return Operation successful?
	/// \param bScanDevices Parameter has no function here
	/// \note May override this for device-specific initialization.
	boolean Initialize (boolean bScanDevices = TRUE) override;

	/// \return TRUE if connection status has been changed (always TRUE on first call)
	boolean UpdatePlugAndPlay (void) override;

	/// \brief Get device-specific descriptor
	/// \param wValue Parameter from setup packet (descriptor type (MSB) and index (LSB))
	/// \param wIndex Parameter from setup packet (e.g. language ID for string descriptors)
	/// \param pLength Pointer to variable, which receives the descriptor size
	/// \return Pointer to descriptor or nullptr, if not available
	virtual const void *GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength) = 0;

	/// \brief Create device-specific EPs
	virtual void AddEndpoints (void) = 0;

	/// \brief Create application interface device (API)
	virtual void CreateDevice (void) = 0;

	/// \brief Get the speed, which has been negotiated with the host
	/// \return Negotiated speed (FullSpeed or HighSpeed) or DeviceSpeedUnknown
	TDeviceSpeed GetNegotiatedUSBSpeed (void) const;

	/// \brief A speed has been negotiated with the host (on enumeration done)
	/// \param Speed Negotiated speed (FullSpeed or HighSpeed)
	/// \note May override this to update EP parameters (e.g. max. packet size).
	virtual void OnNegotiatedSpeed (TDeviceSpeed Speed) {}

	/// \brief Device connection has been suspended / removed
	/// \note Have to undo AddEndpoints() and CreateDevice() here.
	virtual void OnSuspend (void) = 0;

	/// \brief Opt in to bus resume without a USB reset or re-enumeration
	/// \return TRUE to retain configuration, endpoint transfers and EP0 phase
	/// \note Default FALSE preserves the existing reset-only suspend policy.
	/// \note Called at IRQ level; the policy must remain constant while suspended.
	virtual boolean RetainTransfersOnSuspend (void) const { return FALSE; }

	/// \brief The bus suspended with transfers retained
	/// \note Called at IRQ level only when RetainTransfersOnSuspend() is TRUE.
	/// \note Must not reconfigure endpoints or release/reuse their DMA buffers.
	/// \note Pending endpoint completion callbacks may still run while suspended.
	/// \note Stop scheduling application traffic until OnBusResume() or reset.
	virtual void OnBusSuspend (void) {}

	/// \brief The bus resumed without reset; the previous device state is restored
	/// \note Called at IRQ level only after a retained suspend, not after reset.
	/// \note Must not recreate endpoints or restart already armed transfers.
	virtual void OnBusResume (void) {}

	/// \return TRUE while an opted-in gadget is suspended with transfers retained
	boolean IsBusSuspended (void) const { return m_bRetainedSuspend; }

	/// Alternate-interface requests; called in IRQ context.
	/// Override for gadgets with alternate settings or multiple interfaces.
	virtual boolean SetInterface (u16 nInterface, u16 nAlternate);
	virtual int GetInterface (u16 nInterface);

	/// \brief A class or vendor request has been received on EP0
	/// \param pSetupData Contents of the Setup packet
	/// \param pData Received data (host-to-device), or buffer for data to be sent
	/// \return Number of bytes in pData to be sent, or < 0 on error (STALL the request)
	/// \note By default host-to-device requests will be ignored and other requests will be STALLed
	virtual int OnClassOrVendorRequest (const TSetupData *pSetupData, u8 *pData);

private:
	boolean PowerOn (void);
	boolean InitCore (void);
	void InitCoreDevice (void);

	boolean Reset (void);
	void EnableCommonInterrupts (void);
	void EnableDeviceInterrupts (void);
	boolean FlushTxFIFO (unsigned nFIFO, unsigned nUsPollDelay = 1000);
	void FlushRxFIFO (void);

	void HandleUSBSuspend (void);
	void HandleUSBResume (void);
	void HandleUSBReset (void);
	void HandleEnumerationDone (void);
	void HandleInEPInterrupt (void);
	void HandleOutEPInterrupt (void);
	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	boolean WaitForBit (CDWHCIRegister *pRegister, u32 nMask, boolean bWaitUntilSet,
			    unsigned nMsTimeout, unsigned nUsPollDelay = 1000);

	void AssignEndpoint (unsigned nEP, CDWUSBGadgetEndpoint *pEP);
	void RemoveEndpoint (unsigned nEP);
	void SetDeviceAddress (u8 uchAddress);
	boolean SetConfiguration (u8 uchConfiguration);
	u8 GetConfiguration (void) const;
	friend class CDWUSBGadgetEndpoint;
	friend class CDWUSBGadgetEndpoint0;

private:
	static const unsigned NumberOfEPs	= 7;	// without EP0
	static const unsigned NumberOfInEPs	= 7;
	static const unsigned NumberOfOutEPs	= 7;

private:
	CInterruptSystem *m_pInterruptSystem;
	TDeviceSpeed m_DeviceSpeed;

	enum TState
	{
		StatePowered,
		StateSuspended,
		StateResetDone,
		StateEnumDone,
		StateConfigured
	};

	TState m_State;
	TState m_PreSuspendState;
	boolean m_bRetainedSuspend;

	enum TPnPEvent
	{
		PnPEventConfigured,
		PnPEventSuspend,
		PnPEventUnknown
	};

	boolean m_bPnPEvent[PnPEventUnknown];

	CDWUSBGadgetEndpoint *m_pEP[NumberOfEPs+1];
};

#endif
