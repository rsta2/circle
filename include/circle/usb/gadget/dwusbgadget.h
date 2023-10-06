//
// dwusbgadget.h
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
#ifndef _circle_usb_gadget_dwusbgadget_h
#define _circle_usb_gadget_dwusbgadget_h

#include <circle/usb/usbcontroller.h>
#include <circle/usb/gadget/dwusbgadgetendpoint.h>
#include <circle/usb/dwhciregister.h>
#include <circle/interrupt.h>
#include <circle/types.h>

class CDWUSBGadget : public CUSBController	/// DW USB gadget on Raspberry Pi (3)A(+), Zero (2) (W), 4B
{
public:
	enum TDeviceSpeed
	{
		FullSpeed,
		HighSpeed
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

	/// \brief Device connection has been suspended / removed
	/// \note Have to undo AddEndpoints() and CreateDevice() here.
	virtual void OnSuspend (void) = 0;

private:
	boolean PowerOn (void);
	boolean InitCore (void);
	void InitCoreDevice (void);

	boolean Reset (void);
	void EnableCommonInterrupts (void);
	void EnableDeviceInterrupts (void);
	void FlushTxFIFO (unsigned nFIFO);
	void FlushRxFIFO (void);

	void HandleUSBSuspend (void);
	void HandleUSBReset (void);
	void HandleEnumerationDone (void);
	void HandleInEPInterrupt (void);
	void HandleOutEPInterrupt (void);
	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	boolean WaitForBit (CDWHCIRegister *pRegister, u32 nMask, boolean bWaitUntilSet,
			    unsigned nMsTimeout);

	void AssignEndpoint (unsigned nEP, CDWUSBGadgetEndpoint *pEP);
	void RemoveEndpoint (unsigned nEP);
	void SetDeviceAddress (u8 uchAddress);
	boolean SetConfiguration (u8 uchConfiguration);
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
