//
// dwusbgadgetendpoint0.h
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
#ifndef _circle_usb_gadget_dwusbgadgetendpoint0_h
#define _circle_usb_gadget_dwusbgadgetendpoint0_h

#include <circle/usb/gadget/dwusbgadgetendpoint.h>
#include <circle/synchronize.h>
#include <circle/types.h>

class CDWUSBGadgetEndpoint0 : public CDWUSBGadgetEndpoint	/// Endpoint 0 of a DW USB gadget
{
public:
	CDWUSBGadgetEndpoint0 (size_t nMaxPacketSize, CDWUSBGadget *pGadget);
	~CDWUSBGadgetEndpoint0 (void);

	void OnActivate (void) override;

	void OnControlMessage (void) override;

	void OnTransferComplete (boolean bIn, size_t nLength) override;

private:
	enum TState
	{
		StateDisconnect,
		StateIdle,
		StateInDataPhase,
		StateOutDataPhase,
		StateInStatusPhase,
		StateOutStatusPhase,
		StateStall
	};

	TState m_State;

	size_t m_nBytesLeft;
	u8 *m_pBufPtr;

	static const size_t BufferSize = 512;
	DMA_BUFFER (u8, m_OutBuffer, BufferSize);
	DMA_BUFFER (u8, m_InBuffer, BufferSize);
};

#endif
