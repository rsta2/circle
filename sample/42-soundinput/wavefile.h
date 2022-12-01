//
// wavefile.h
//
#ifndef _wavefile_h
#define _wavefile_h

#include <circle/macros.h>
#include <circle/types.h>

struct TWAVEFileHeader
{
	static const u32 RIFF = 0x46464952U;
	static const u32 WAVE = 0x45564157U;
	static const u32 fmt  = 0x20746D66U;
	static const u32 data = 0x61746164U;

	u32	RIFFHeader;		// "RIFF"
	u32	RIFFChunkSize;		// total file size in bytes - 8

	u32	WAVEHeader;		// "WAVE"

	u32	fmtHeader;		// "fmt "
	u32	fmtChunkSize;		// 16
	u16	SampleFormat;		// 1 (PCM)
	u16	NumberOfChannels;	// 2
	u32	SampleRate;		// e.g. 44100
	u32	BytesPerSecond;		// (SampleRate * BitsPerSample * NumberOfChannels) / 8
	u16	BytesPerFrame;		// (BitsPerSample * NumberOfChannels) / 8
	u16	BitsPerSample;		// e.g. 16

	u32	dataHeader;		// "data"
	u32	dataChunkSize;		// size of following data in bytes
}
PACKED;

// chans	Number of channels
// bits		Bits per sample
// rate		Sample rate
// size		Total size of the audio data
#define WAVE_FILE_HEADER(chans, bits, rate, size)	\
{							\
	TWAVEFileHeader::RIFF,				\
	(size) + (u32) sizeof (TWAVEFileHeader) - 8,	\
	TWAVEFileHeader::WAVE,				\
	TWAVEFileHeader::fmt,				\
	16,						\
	1,						\
	(chans),					\
	(rate),						\
	(rate) * (bits) * (chans) / 8,			\
	(bits) * (chans) / 8,				\
	(bits),						\
	TWAVEFileHeader::data,				\
	(size)						\
}

#endif
