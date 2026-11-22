#pragma once
#include <circle/types.h>
#include <circle/usb/usb.h>
class CDWHCIRegister;
class CDWUSBGadgetEndpoint;
class CDWUSBGadget
{
      public:
	static constexpr unsigned NumberOfInEPs = 7, NumberOfOutEPs = 7, NumberOfEPs = 7;
	enum TState
	{
		StatePowered,
		StateSuspended,
		StateResetDone,
		StateEnumDone,
		StateConfigured
	};
	TState m_State = StatePowered, m_PreSuspendState = StatePowered;
	bool m_bRetainedSuspend = false, retain = false;
	unsigned suspends = 0, resumes = 0;
	bool RetainTransfersOnSuspend () const
	{
		return retain;
	}
	void OnBusSuspend ()
	{
		++suspends;
	}
	void OnBusResume ()
	{
		++resumes;
	}
	void EnableDeviceInterrupts ();
	bool IsBusSuspended () const
	{
		return m_bRetainedSuspend;
	}
	void HandleUSBSuspend ();
	void HandleUSBResume ();
	void InterruptHandler ();
	void HandleEnumerationDone ()
	{
		m_State = StateEnumDone;
	}
	CDWUSBGadgetEndpoint *m_pEP[8] = {};
	void AssignEndpoint (unsigned ep, CDWUSBGadgetEndpoint *p)
	{
		m_pEP[ep] = p;
	}
	void RemoveEndpoint (unsigned ep)
	{
		m_pEP[ep] = nullptr;
	}
	boolean WaitForBit (CDWHCIRegister *, u32, boolean, unsigned, unsigned = 1000);
	boolean FlushTxFIFO (unsigned, unsigned = 1000);
	void HandleUSBReset ();
	void HandleInEPInterrupt ();
	void HandleOutEPInterrupt ();
	unsigned deconfigurations = 0;
	void SetDeviceAddress (u8)
	{
	}
	u8 GetConfiguration (void) const;
	boolean SetConfiguration (u8 n)
	{
		if (!n)
		{
			++deconfigurations;
		}
		return true;
	}
	boolean SetInterface (u16, u16)
	{
		return true;
	}
	int GetInterface (u16)
	{
		return 0;
	}
	size_t descriptorSize = 18;
	const void *GetDescriptor (u16, u16, size_t *size)
	{
		static const u8 desc[274] = {18, 1};
		*size = descriptorSize;
		return desc;
	}
	int OnClassOrVendorRequest (const TSetupData *, u8 *)
	{
		return 0;
	}
};
