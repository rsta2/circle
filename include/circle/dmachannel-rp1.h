//
// dmachannel-rp1.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_dmachannel_rp1_h
#define _circle_dmachannel_rp1_h

#include <circle/interrupt.h>
#include <circle/spinlock.h>
#include <circle/macros.h>
#include <circle/types.h>

class CDMAChannelRP1	/// RP1 platform DMA controller support
{
public:
	enum TDREQ
	{
		DREQSourceSPI0RX	= 12,
		DREQSourceSPI0TX	= 13,
		DREQSourceSPI1RX	= 14,
		DREQSourceSPI1TX	= 15,
		DREQSourceSPI2RX	= 16,
		DREQSourceSPI2TX	= 17,
		DREQSourceSPI3RX	= 18,
		DREQSourceSPI3TX	= 19,
		DREQSourceSPI5RX	= 22,
		DREQSourceSPI5TX	= 23,

		DREQSourcePWM0		= 24,

		DREQSourceI2S0RX	= 31,
		DREQSourceI2S0TX	= 32,
		DREQSourceI2S1RX	= 33,
		DREQSourceI2S1TX	= 34,

		DREQSourceNone		= 64
	};

	/// \brief Transfer completion routine
	/// \param nChannel Number of DMA channel (0-7)
	/// \param nBuffer  Number of cyclic buffer (0-N, 0 if not cyclic)
	/// \param bStatus  TRUE for successful transfer, FALSE on error
	/// \param pParam   User parameter
	typedef void TCompletionRoutine (unsigned nChannel, unsigned nBuffer,
					 boolean bStatus, void *pParam);

public:
	/// \param nChannel Explicit channel number (0-7)
	/// \param pInterruptSystem Pointer to the interrupt system object
	CDMAChannelRP1 (unsigned nChannel, CInterruptSystem *pInterruptSystem);

	~CDMAChannelRP1 (void);

	/// \brief Prepare a memory copy transfer
	/// \param pDestination Pointer to the destination buffer
	/// \param pSource	Pointer to the source buffer
	/// \param ulLength	Number of bytes to be transferred
	/// \param bCached	Are the destination and source buffers in cached memory regions
	void SetupMemCopy (void *pDestination, const void *pSource, size_t ulLength,
			   boolean bCached = TRUE);

	/// \brief Prepare an I/O read transfer
	/// \param pDestination Pointer to the destination buffer
	/// \param ulIOAddress	I/O address to be read from (ARM-side or bus address)
	/// \param ulLength	Number of bytes to be transferred
	/// \param DREQ		DREQ line for pacing the transfer
	/// \param nIORegWidth	Width of the I/O register, 1 or 4 byte(s) (default)
	void SetupIORead (void *pDestination, uintptr ulIOAddress, size_t ulLength, TDREQ DREQ,
			  unsigned nIORegWidth = 4);

	/// \brief Prepare a cyclic I/O read transfer
	/// \param ppDestinations Pointer to an array with the destination buffer pointers
	/// \param nBuffers	Number of destination buffers
	/// \param ulIOAddress	I/O address to be read from (ARM-side or bus address)
	/// \param ulLength	Number of bytes to be transferred per buffer
	/// \param DREQ		DREQ line for pacing the transfer
	/// \param nIORegWidth	Width of the I/O register, 1 or 4 byte(s) (default)
	/// \note Transfer starts from first buffer again, when last buffer has been filled.
	void SetupCyclicIORead (void *ppDestinations[], uintptr ulIOAddress, unsigned nBuffers,
				size_t ulLength, TDREQ DREQ, unsigned nIORegWidth = 4);

	/// \brief Prepare an I/O write transfer
	/// \param ulIOAddress	I/O address to be written (ARM-side or bus address)
	/// \param pSource	Pointer to the source buffer
	/// \param ulLength	Number of bytes to be transferred
	/// \param DREQ		DREQ line for pacing the transfer
	/// \param nIORegWidth	Width of the I/O register, 1 or 4 byte(s) (default)
	void SetupIOWrite (uintptr ulIOAddress, const void *pSource, size_t ulLength, TDREQ DREQ,
			   unsigned nIORegWidth = 4);

	/// \brief Prepare a cyclic I/O write transfer
	/// \param ulIOAddress	I/O address to be written (ARM-side or bus address)
	/// \param ppSources	Pointer to an array with the source buffer pointers
	/// \param nBuffers	Number of source buffers
	/// \param ulLength	Number of bytes to be transferred per buffer
	/// \param DREQ		DREQ line for pacing the transfer
	/// \param nIORegWidth	Width of the I/O register, 1 or 4 byte(s) (default)
	/// \note Transfer starts from first buffer again, when last buffer has been sent.
	void SetupCyclicIOWrite (uintptr ulIOAddress, const void *ppSources[], unsigned nBuffers,
				 size_t ulLength, TDREQ DREQ, unsigned nIORegWidth = 4);

	/// \brief Set completion routine to be called, when the transfer is finished
	/// \param pRoutine Pointer to the completion routine
	/// \param pParam   User parameter
	void SetCompletionRoutine (TCompletionRoutine *pRoutine, void *pParam);

	/// \brief Start the DMA transfer
	void Start (void);

	/// \brief Cancel running DMA transfer and wait for termination
	void Cancel (void);

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	void ChannelInterruptHandler (boolean bStatus);
	static void InterruptHandler (void *pParam);

private:
	static const int Channels = 8;

	static const int MaxCyclicBuffers = 4;

	enum TDirection
	{
		DirectionMem2Mem,
		DirectionMem2Dev,
		DirectionDev2Mem,
		DirectionUnknown
	};

	struct TLinkedListItem
	{
		u64	sar;
		u64	dar;
		u32	block_ts_lo;
		u32	block_ts_hi;
		u64	llp;
		u32	ctl_lo;
		u32	ctl_hi;
		u32	sstat;
		u32	dstat;
		u32	status_lo;
		u32	status_hi;
		u32	reserved_lo;
		u32	reserved_hi;
	}
	PACKED;

private:
	unsigned m_nChannel;

	TLinkedListItem *m_pLLI[MaxCyclicBuffers];

	unsigned m_nBuffers;
	volatile unsigned m_nCurrentBuffer;

	size_t m_ulLength;
	TDirection m_Direction;
	TDREQ m_DREQ;

	TCompletionRoutine *m_pCompletionRoutine;
	void *m_pCompletionParam;

	static unsigned s_nInstanceCount;
	static CInterruptSystem *s_pInterruptSystem;

	static CSpinLock s_SpinLock;

	static CDMAChannelRP1 *s_pThis[Channels];
};

#endif
