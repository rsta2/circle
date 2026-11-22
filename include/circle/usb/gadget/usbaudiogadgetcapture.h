// SPDX-License-Identifier: GPL-3.0-or-later
//
// Circle - A C++ bare metal environment for Raspberry Pi
//
#ifndef _circle_usb_gadget_usbaudiogadgetcapture_h
#define _circle_usb_gadget_usbaudiogadgetcapture_h

#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/spinlock.h>

/// Full-speed UAC1 isochronous IN stream, 48 kHz interleaved stereo PCM16.
/// Append Terminals to the AC topology and Streaming after the AC interface.
/// Include the streaming interface in the enclosing AC header's collection.
struct TUSBAudioCaptureDescriptors
{
	enum TClock { Asynchronous, Synchronous };
	TUSBAudioCaptureDescriptors (u8 nInterface, u8 nEndpoint, u8 nInputTerminal,
				     u8 nOutputTerminal, u16 usTerminalType,
				     TClock Clock = Asynchronous);

	TUSBEndpointDescriptor Endpoint;
	u8 Terminals[21];
	u8 Streaming[52];
};

class CUSBAudioCaptureSource
{
public:
	static const unsigned Channels = 2;
	static const unsigned NominalFrames = 48;
	static const unsigned MaxFrames = NominalFrames + 1;

	/// Callbacks can run at IRQ level. Implementations must never block.
	virtual void SetRecording (boolean bActive) = 0;
	/// Fill up to MaxFrames stereo frames. Return number of initialized frames.
	virtual unsigned ReadPacket (s16 *pSamples) = 0;
protected:
	~CUSBAudioCaptureSource (void) = default;
};

/// Reference source for an independently clocked producer.
class CUSBAudioCaptureBuffer : public CUSBAudioCaptureSource
{
public:
	struct TStatistics
	{
		unsigned nProduced, nSent, nQueued, nOverruns, nUnderruns;
	};

	void SetRecording (boolean bActive) override;
	unsigned ReadPacket (s16 *pSamples) override;
	/// Submit interleaved stereo PCM16 frames. Inactive input is discarded.
	void WriteFrames (const s16 *pSamples, unsigned nFrames);
	void GetStatistics (TStatistics *pStatistics);

private:
	static const unsigned Capacity = 1024;
	static const unsigned Reserve = 256;
	CSpinLock m_Lock;
	s16 m_Buffer[Capacity * Channels];
	boolean m_bRecording = FALSE, m_bPrimed = FALSE;
	unsigned m_nRead = 0, m_nWrite = 0, m_nPackets = 0;
	unsigned m_nProduced = 0, m_nSent = 0, m_nOverruns = 0, m_nUnderruns = 0;
};

class CUSBAudioCaptureEndpoint : public CDWUSBGadgetEndpoint
{
public:
	CUSBAudioCaptureEndpoint (const TUSBAudioCaptureDescriptors &Descriptors,
				  CDWUSBGadget *pGadget, CUSBAudioCaptureSource *pSource);
	boolean SetInterface (u16 nInterface, u16 nAlternate);
	int GetInterface (u16 nInterface) const;
	void OnUSBReset (void) override;
	void OnActivate (void) override;
	void OnDeactivate (void) override;
	void OnSuspend (void) override;
	void OnTransferComplete (boolean bIn, size_t nLength) override;
private:
	void Arm (void);
	CUSBAudioCaptureSource *m_pSource;
	unsigned m_nInterface;
	unsigned m_nMaxFrames;
	boolean m_bConfigured = FALSE, m_bStreaming = FALSE;
	DMA_BUFFER (s16, m_Samples, CUSBAudioCaptureSource::MaxFrames * 2);
};

#endif
