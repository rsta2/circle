#pragma once
#include <circle/usb/usb.h>
#include <vector>
class CDWUSBGadget
{
};
class CDWUSBGadgetEndpoint
{
public:
	enum TDirection
	{
		DirectionOut,
		DirectionIn
	};
	enum TTransferMode
	{
		TransferDataOut,
		TransferDataIn
	};
	CDWUSBGadgetEndpoint (const TUSBEndpointDescriptor *p, CDWUSBGadget *)
	    : m_In (p->bEndpointAddress & 0x80)
	{
	}
	virtual ~CDWUSBGadgetEndpoint () = default;
	virtual void OnUSBReset ()
	{
		Armed = false;
		++Resets;
	}
	virtual void OnActivate () = 0;
	virtual void OnDeactivate () = 0;
	virtual void OnSuspend ()
	{
	}
	virtual void OnTransferComplete (boolean, size_t) = 0;
	unsigned Starts = 0, Cancels = 0, Resets = 0;
	bool Armed = false;
	std::vector<std::vector<u8>> Packets;

protected:
	TDirection GetDirection () const
	{
		return m_In ? DirectionIn : DirectionOut;
	}
	void BeginTransfer (TTransferMode mode, void *p, size_t n)
	{
		++Starts;
		Armed = true;
		if (mode == TransferDataIn)
		{
			Packets.emplace_back (static_cast<u8 *> (p), static_cast<u8 *> (p) + n);
		}
	}
	void CancelTransfer ()
	{
		Armed = false;
		++Cancels;
	}

private:
	bool m_In;
};
