// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <circle/sound/i2ssoundbasedevice.h>
#include <circle/usb/gadget/usbaudiogadgetcapture.h>
#include <circle/i2cmaster.h>
#include <circle/logger.h>
#include <circle/atomic.h>

class CADCSource : public CI2SSoundBaseDevice, public CUSBAudioCaptureBuffer
{
public:
	explicit CADCSource (CInterruptSystem *pInterrupt,
			     CI2CMaster *pI2CMaster = nullptr,
			     boolean bSlave = TRUE,
			     TDeviceMode Mode = DeviceModeRXOnly)
	: CI2SSoundBaseDevice (pInterrupt, 48000, 128, bSlave, pI2CMaster, 0, Mode) {}

	void SetRecording (boolean bActive) override
	{
		CUSBAudioCaptureBuffer::SetRecording (bActive);
		AtomicSet (&m_nReportPending, !bActive);
	}

	void Report (CLogger &Logger)
	{
		if (!AtomicExchange (&m_nReportPending, 0)) return;
		TStatistics Stats;
		GetStatistics (&Stats);
		Logger.Write ("adc", LogNotice, "produced %u sent %u queued %u over %u under %u",
			      Stats.nProduced, Stats.nSent, Stats.nQueued,
			      Stats.nOverruns, Stats.nUnderruns);
	}

private:
	void PutChunk (const u32 *pData, unsigned nWords) override
	{
		s16 Samples[128];
		while (nWords >= 2)
		{
			unsigned nFrames = nWords > 128 ? 64 : nWords / 2;
			for (unsigned i = 0; i < nFrames * 2; ++i)
			{
				Samples[i] = (s32) *pData++ >> 8;
			}
			WriteFrames (Samples, nFrames);
			nWords -= nFrames * 2;
		}
	}

	volatile int m_nReportPending = 0;
};
