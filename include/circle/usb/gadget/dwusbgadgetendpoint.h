//
// dwusbgadgetendpoint.h
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
#ifndef _circle_usb_gadget_dwusbgadgetendpoint_h
#define _circle_usb_gadget_dwusbgadgetendpoint_h

#include <circle/usb/usb.h>
#include <circle/usb/dwhci.h>
#include <circle/synchronize.h>
#include <circle/types.h>

class CDWUSBGadget;

/// \note Derive your own endpoint class(es) from this one.

class CDWUSBGadgetEndpoint	/// Endpoint of a DW USB gadget
{
public:
	/// \brief Create EP0
	/// \param nMaxPacketSize Maximum packet size for EP0
	/// \param pGadget Pointer to USB gadget object
	CDWUSBGadgetEndpoint (size_t nMaxPacketSize, CDWUSBGadget *pGadget);
	/// \brief Create other EPs
	/// \param pDesc Pointer to descriptor, which describes this endpoint
	/// \param pGadget Pointer to USB gadget object
	CDWUSBGadgetEndpoint (const TUSBEndpointDescriptor *pDesc, CDWUSBGadget *pGadget);

	virtual ~CDWUSBGadgetEndpoint (void);

	/// \brief Called on USB reset for each EP
	/// \note May override this for EP-specific processing.
	virtual void OnUSBReset (void);

	/// \brief Called to activate/start EP processing.
	/// \note Override this to start first transfer.
	virtual void OnActivate (void) = 0;

	/// \brief Called, when the current transfer completes.
	/// \param bIn Was it an IN transfer?
	/// \param nLength Number of transferred bytes
	/// \note Override this to process received data and/or restart transfer.
	virtual void OnTransferComplete (boolean bIn, size_t nLength) = 0;

	/// \brief Device connection has been suspended / removed
	/// \note Override this to abort pending transfers.
	virtual void OnSuspend (void) {}

protected:
	/// \return Endpoint number (0-15)
	unsigned GetEPNumber (void) const
	{
		return m_nEP;
	}

	enum TDirection
	{
		DirectionOut,		///< From host to device
		DirectionIn,		///< From device to host
		DirectionInOut		///< Bidirectional
	};

	/// \return Endpoint direction
	TDirection GetDirection (void) const
	{
		return m_Direction;
	}

	enum TType
	{
		TypeControl,
		TypeBulk,
		//TypeInterrupt,
		//TypeIsochronous
	};

	/// \return Endpoint type
	TType GetType (void) const
	{
		return m_Type;
	}

	enum TTransferMode
	{
		TransferSetupOut,	///< From host to device, setup packet
		TransferDataOut,	///< From host to device
		TransferDataIn,		///< From device to host
		TransferUnknown
	};

	/// \brief Begin a transfer
	/// \param Mode Transfer mode
	/// \param pBuffer Points to buffer with data to be sent, or which receives the data
	/// \param nLength Number of bytes to be sent, or max. bytes to be received
	/// \note pBuffer may be nullptr and nLength zero to transfer empty packets.
	/// \note The buffer must be declared as DMA_BUFFER
	void BeginTransfer (TTransferMode Mode, void *pBuffer, size_t nLength);

	/// \brief Send STALL response
	/// \param bIn STALL next IN request, or OUT otherwise?
	void Stall (boolean bIn);

private:
	size_t FinishTransfer (void);
	void InitTransfer (void);

	virtual void OnControlMessage (void);

	virtual void HandleOutInterrupt (void);
	virtual void HandleInInterrupt (void);

	static void HandleUSBReset (void);

	friend class CDWUSBGadget;
	friend class CDWUSBGadgetEndpoint0;

private:
	CDWUSBGadget *m_pGadget;

	TDirection m_Direction;
	TType m_Type;
	unsigned m_nEP;				// EP number

	size_t m_nMaxPacketSize;

	TTransferMode m_TransferMode;
	void *m_pTransferBuffer;
	size_t m_nTransferLength;

	DMA_BUFFER (u32, m_DummyBuffer, 1);	// for empty packet transfers

	// next EP sequence including EP0
	// (EP if non-periodic and active, 0xFF otherwise)
	static u8 s_NextEPSeq[DWHCI_MAX_EPS_CHANNELS];
	static u8 s_uchFirstInNextEPSeq;
};

#endif
