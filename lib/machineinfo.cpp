//
// machineinfo.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/machineinfo.h>
#include <circle/gpioclock.h>
#include <circle/sysconfig.h>
#include <circle/startup.h>
#include <assert.h>

static struct
{
	unsigned	nRevisionRaw;
	TMachineModel	Model;
	unsigned	nRevision;
	unsigned	nRAMSize;
}
s_OldInfo[] 
{
	{0x02, MachineModelBRelease1MB256, 1, 256},
	{0x03, MachineModelBRelease1MB256, 1, 256},
	{0x04, MachineModelBRelease2MB256, 2, 256},
	{0x05, MachineModelBRelease2MB256, 2, 256},
	{0x06, MachineModelBRelease2MB256, 2, 256},
	{0x07, MachineModelA,              2, 256},
	{0x08, MachineModelA,              2, 256},
	{0x09, MachineModelA,              2, 256},
	{0x0D, MachineModelBRelease2MB512, 2, 512},
	{0x0E, MachineModelBRelease2MB512, 2, 512},
	{0x0F, MachineModelBRelease2MB512, 2, 512},
	{0x10, MachineModelBPlus,          1, 512},
	{0x11, MachineModelCM,             1, 512},
	{0x12, MachineModelAPlus,          1, 256},
	{0x13, MachineModelBPlus,          2, 512},
	{0x14, MachineModelCM,             1, 512},
	{0x15, MachineModelAPlus,          1, 256}
};

static struct
{
	unsigned	nType;
	TMachineModel	Model;
	unsigned	nMajor;
}
s_NewInfo[] 
{
	{0, MachineModelA,		1},
	{1, MachineModelBRelease2MB512,	1},	// can be other revision
	{2, MachineModelAPlus,		1},
	{3, MachineModelBPlus,		1},
	{4, MachineModel2B,		2},
	{6, MachineModelCM,		1},
	{8, MachineModel3B,		3},
	{9, MachineModelZero,		1},
	{10, MachineModelCM3,		3},
	{12, MachineModelZeroW,		1},
	{13, MachineModel3BPlus,	3},
	{14, MachineModel3APlus,	3},
	{16, MachineModelCM3Plus,	3},
	{17, MachineModel4B,		4},
	{18, MachineModelZero2W,	3},
	{19, MachineModel400,		4},
	{20, MachineModelCM4,		4},
	{21, MachineModelCM4S,		4}
};

static const char *s_MachineName[] =		// must match TMachineModel
{
	"Raspberry Pi Model A",
	"Raspberry Pi Model B R1",
	"Raspberry Pi Model B R2",
	"Raspberry Pi Model B R2",
	"Raspberry Pi Model A+",
	"Raspberry Pi Model B+",
	"Raspberry Pi Zero",
	"Raspberry Pi Zero W",
	"Raspberry Pi Zero 2 W",
	"Raspberry Pi 2 Model B",
	"Raspberry Pi 3 Model B",
	"Raspberry Pi 3 Model A+",
	"Raspberry Pi 3 Model B+",
	"Compute Module",
	"Compute Module 3",
	"Compute Module 3+",
	"Raspberry Pi 4 Model B",
	"Raspberry Pi 400",
	"Compute Module 4",
	"Compute Module 4S",
	"Unknown"
};

static const char *s_SoCName[] =		// must match TSoCType
{
	"BCM2835",
	"BCM2836",
	"BCM2837",
	"BCM2711",
	"Unknown"
};

static unsigned s_ActLEDInfo[] =		// must match TMachineModel
{
	16 | ACTLED_ACTIVE_LOW,		// A
	16 | ACTLED_ACTIVE_LOW,		// B R1
	16 | ACTLED_ACTIVE_LOW,		// B R2 256MB
	16 | ACTLED_ACTIVE_LOW,		// B R2 512MB
	47,				// A+
	47,				// B+
	47 | ACTLED_ACTIVE_LOW,		// Zero
	47 | ACTLED_ACTIVE_LOW,		// Zero W
	29 | ACTLED_ACTIVE_LOW,		// Zero 2 W
	47,				// 2B
	0 | ACTLED_VIRTUAL_PIN,		// 3B
	29,				// 3A+
	29,				// 3B+
	47,				// CM
	0 | ACTLED_VIRTUAL_PIN,		// CM3
	0 | ACTLED_VIRTUAL_PIN,		// CM3+
	42,				// 4B
	42,				// 400
	42,				// CM4
	0 | ACTLED_VIRTUAL_PIN,		// CM4S

	ACTLED_UNKNOWN			// Unknown
};

CMachineInfo *CMachineInfo::s_pThis = 0;

CMachineInfo::CMachineInfo (void)
:	m_nRevisionRaw (0),
	m_MachineModel (MachineModelUnknown),
	m_nModelMajor (0),
	m_nModelRevision (0),
	m_SoCType (SoCTypeUnknown),
	m_nRAMSize (0),
#if RASPPI <= 3
	m_usDMAChannelMap (0x1F35)	// default mapping
#else
	m_usDMAChannelMap (0x71F5),	// default mapping
	m_pDTB (0)
#endif
{
	if (s_pThis != 0)
	{
		m_nRevisionRaw	 = s_pThis->m_nRevisionRaw;
		m_MachineModel	 = s_pThis->m_MachineModel;
		m_nModelMajor	 = s_pThis->m_nModelMajor;
		m_nModelRevision = s_pThis->m_nModelRevision;
		m_SoCType	 = s_pThis->m_SoCType;
		m_nRAMSize	 = s_pThis->m_nRAMSize;

		return;
	}
	s_pThis = this;

	CBcmPropertyTags Tags (TRUE);
	TPropertyTagSimple DMAChannels;
	if (Tags.GetTag (PROPTAG_GET_DMA_CHANNELS, &DMAChannels, sizeof DMAChannels))
	{
		m_usDMAChannelMap = (u16) DMAChannels.nValue;
	}

	TPropertyTagSimple BoardRevision;
	if (!Tags.GetTag (PROPTAG_GET_BOARD_REVISION, &BoardRevision, sizeof BoardRevision))
	{
		return;
	}

	m_nRevisionRaw = BoardRevision.nValue;
	if (m_nRevisionRaw & (1 << 23))		// new revision scheme?
	{
		unsigned nType = (m_nRevisionRaw >> 4) & 0xFF;
		unsigned i;
		for (i = 0; i < sizeof s_NewInfo / sizeof s_NewInfo[0]; i++)
		{
			if (s_NewInfo[i].nType == nType)
			{
				break;
			}
		}

		if (i >= sizeof s_NewInfo / sizeof s_NewInfo[0])
		{
			return;
		}
		
		m_MachineModel   = s_NewInfo[i].Model;
		m_nModelMajor    = s_NewInfo[i].nMajor;
		m_nModelRevision = (m_nRevisionRaw & 0xF) + 1;
		m_SoCType        = (TSoCType) ((m_nRevisionRaw >> 12) & 0xF);
		m_nRAMSize       = 256 << ((m_nRevisionRaw >> 20) & 7);

		if (m_SoCType >= SoCTypeUnknown)
		{
			m_SoCType = SoCTypeUnknown;
		}

		if (   m_MachineModel == MachineModelBRelease2MB512
		    && m_nRAMSize     == 256)
		{
			m_MachineModel =   m_nModelRevision == 1
					 ? MachineModelBRelease1MB256
					 : MachineModelBRelease2MB256;
		}
	}
	else
	{
		unsigned i;
		for (i = 0; i < sizeof s_OldInfo / sizeof s_OldInfo[0]; i++)
		{
			if (s_OldInfo[i].nRevisionRaw == m_nRevisionRaw)
			{
				break;
			}
		}

		if (i >= sizeof s_OldInfo / sizeof s_OldInfo[0])
		{
			return;
		}

		m_MachineModel	 = s_OldInfo[i].Model;
		m_nModelMajor	 = 1;
		m_nModelRevision = s_OldInfo[i].nRevision;
		m_SoCType	 = SoCTypeBCM2835;
		m_nRAMSize	 = s_OldInfo[i].nRAMSize;
	}
}

CMachineInfo::~CMachineInfo (void)
{
	m_MachineModel = MachineModelUnknown;

#if RASPPI >= 4
	delete m_pDTB;
	m_pDTB = 0;
#endif

	if (s_pThis == this)
	{
		s_pThis = 0;
	}
}

TMachineModel CMachineInfo::GetMachineModel (void) const
{
	return m_MachineModel;
}

const char *CMachineInfo::GetMachineName (void) const
{
	return s_MachineName[m_MachineModel];
}

unsigned CMachineInfo::GetModelMajor (void) const
{
	return m_nModelMajor;
}

unsigned CMachineInfo::GetModelRevision (void) const
{
	return m_nModelRevision;
}

TSoCType CMachineInfo::GetSoCType (void) const
{
	return m_SoCType;
}

unsigned CMachineInfo::GetRAMSize (void) const
{
	return m_nRAMSize;
}

const char *CMachineInfo::GetSoCName (void) const
{
	return s_SoCName[m_SoCType];
}

u32 CMachineInfo::GetRevisionRaw (void) const
{
	return m_nRevisionRaw;
}

unsigned CMachineInfo::GetActLEDInfo (void) const
{
	return s_ActLEDInfo[m_MachineModel];
}

unsigned CMachineInfo::GetClockRate (u32 nClockId) const
{
	CBcmPropertyTags Tags;
	TPropertyTagClockRate TagClockRate;
	TagClockRate.nClockId = nClockId;
	if (   Tags.GetTag (PROPTAG_GET_CLOCK_RATE, &TagClockRate, sizeof TagClockRate, 4)
	    && TagClockRate.nRate != 0)
	{
		return TagClockRate.nRate;
	}

	TagClockRate.nClockId = nClockId;
	if (   Tags.GetTag (PROPTAG_GET_CLOCK_RATE_MEASURED, &TagClockRate, sizeof TagClockRate, 4)
	    && TagClockRate.nRate != 0)
	{
		return TagClockRate.nRate;
	}

	// if clock rate can not be requested, use a default rate
	unsigned nResult = 0;

	switch (nClockId)
	{
	case CLOCK_ID_EMMC:
	case CLOCK_ID_EMMC2:
		nResult = 100000000;
		break;

	case CLOCK_ID_UART:
		nResult = 48000000;
		break;

	case CLOCK_ID_CORE:
		if (m_nModelMajor < 3)
		{
			nResult = 250000000;
		}
		else
		{
			nResult = 300000000;		// TODO
		}
		break;

	case CLOCK_ID_PIXEL_BVB:
		nResult = 75000000;
		break;

	default:
		assert (0);
		break;
	}

	return nResult;
}

unsigned CMachineInfo::GetGPIOPin (TGPIOVirtualPin Pin) const
{
	unsigned nResult = 0;

	switch (Pin)
	{
	case GPIOPinAudioLeft:
#ifdef USE_PWM_AUDIO_ON_ZERO
		if (   m_MachineModel == MachineModelZero
		    || m_MachineModel == MachineModelZeroW
		    || m_MachineModel == MachineModelZero2W)
		{
#ifdef USE_GPIO18_FOR_LEFT_PWM_ON_ZERO
			return 18;
#else
			return 12;
#endif // USE_GPIO18_FOR_LEFT_PWM_ON_ZERO
		}
#endif
		if (m_MachineModel <= MachineModelBRelease2MB512)
		{
			nResult = 40;
		}
		else
		{
			if (m_nModelMajor < 3)
			{
				nResult = 45;
			}
			else
			{
				nResult = 41;
			}
		}
		break;

	case GPIOPinAudioRight:
#ifdef USE_PWM_AUDIO_ON_ZERO
		if (   m_MachineModel == MachineModelZero
		    || m_MachineModel == MachineModelZeroW
		    || m_MachineModel == MachineModelZero2W)
		{
#ifdef USE_GPIO19_FOR_RIGHT_PWM_ON_ZERO
			return 19;
#else
			return 13;
#endif // USE_GPIO19_FOR_RIGHT_PWM_ON_ZERO
		}
#endif
		if (m_MachineModel <= MachineModelBRelease2MB512)
		{
			nResult = 45;
		}
		else
		{
			nResult = 40;
		}
		break;

	default:
		assert (0);
		break;
	}

	return nResult;
}

unsigned CMachineInfo::GetGPIOClockSourceRate (unsigned nSourceId)
{
	if (m_nModelMajor <= 3)
	{
		switch (nSourceId)
		{
		case GPIOClockSourceOscillator:
			return 19200000;

		case GPIOClockSourcePLLD:
			return 500000000;

		default:
			break;
		}
	}
	else
	{
		switch (nSourceId)
		{
		case GPIOClockSourceOscillator:
			return 54000000;

		case GPIOClockSourcePLLD:
			return 750000000;

		default:
			break;
		}
	}

	return GPIO_CLOCK_SOURCE_UNUSED;
}

unsigned CMachineInfo::GetDevice (TDeviceId DeviceId) const
{
	unsigned nResult = 0;

	switch (DeviceId)
	{
	case DeviceI2CMaster:
		if (m_MachineModel == MachineModelBRelease1MB256)
		{
			nResult = 0;
		}
		else
		{
			nResult = 1;
		}
		break;

	default:
		assert (0);
		break;
	}

	return nResult;
}

boolean CMachineInfo::ArePWMChannelsSwapped (void) const
{
	return    m_MachineModel >= MachineModelAPlus
	       && m_MachineModel != MachineModelZero
	       && m_MachineModel != MachineModelZeroW
	       && m_MachineModel != MachineModelZero2W;
}

unsigned CMachineInfo::AllocateDMAChannel (unsigned nChannel)
{
	assert (s_pThis != 0);
	if (s_pThis != this)
	{
		return s_pThis->AllocateDMAChannel (nChannel);
	}

	if (!(nChannel & ~DMA_CHANNEL__MASK))
	{
		// explicit channel allocation
		assert (nChannel <=  DMA_CHANNEL_MAX);
		if (m_usDMAChannelMap & (1 << nChannel))
		{
			m_usDMAChannelMap &= ~(1 << nChannel);

			return nChannel;
		}
	}
	else
	{
		// arbitrary channel allocation
		int i = nChannel == DMA_CHANNEL_NORMAL ? 6 : DMA_CHANNEL_MAX;
		int nMin = 0;
#if RASPPI >= 4
		if (nChannel == DMA_CHANNEL_EXTENDED)
		{
			i = DMA_CHANNEL_EXT_MAX;
			nMin = DMA_CHANNEL_EXT_MIN;
		}
#endif
		for (; i >= nMin; i--)
		{
			if (m_usDMAChannelMap & (1 << i))
			{
				m_usDMAChannelMap &= ~(1 << i);

				return (unsigned) i;
			}
		}
	}

	return DMA_CHANNEL_NONE;
}

void CMachineInfo::FreeDMAChannel (unsigned nChannel)
{
	assert (s_pThis != 0);
	if (s_pThis != this)
	{
		s_pThis->FreeDMAChannel (nChannel);

		return;
	}

	assert (nChannel <= DMA_CHANNEL_MAX);
	assert (!(m_usDMAChannelMap & (1 << nChannel)));
	m_usDMAChannelMap |= 1 << nChannel;
}

#if RASPPI >= 4

void CMachineInfo::FetchDTB (void)
{
	u32 * volatile pDTBPtr = (u32 * volatile) ARM_DTB_PTR32;

	const void *pDTB = (const void *) (uintptr) *pDTBPtr;
	if (pDTB != 0)
	{
		assert (m_pDTB == 0);
		m_pDTB = new CDeviceTreeBlob (pDTB);
		assert (m_pDTB != 0);

		*pDTBPtr = 0;		// does not work with chain boot, disable it
	}
}

TMemoryWindow CMachineInfo::GetPCIeDMAMemory (void) const
{
	assert (s_pThis != 0);
	if (s_pThis != this)
	{
		return s_pThis->GetPCIeDMAMemory ();
	}

	TMemoryWindow Result;

	if (m_pDTB != 0)
	{
		const TDeviceTreeNode *pPCIe = m_pDTB->FindNode ("/scb/pcie@7d500000");
		if (pPCIe != 0)
		{
			const TDeviceTreeProperty *pDMA = m_pDTB->FindProperty (pPCIe, "dma-ranges");
			if (   pDMA != 0
			    && m_pDTB->GetPropertyValueLength (pDMA) == sizeof (u32)*7)
			{
				Result.BusAddress = (u64) m_pDTB->GetPropertyValueWord (pDMA, 1) << 32
							| m_pDTB->GetPropertyValueWord (pDMA, 2);
				Result.CPUAddress = (u64) m_pDTB->GetPropertyValueWord (pDMA, 3) << 32
							| m_pDTB->GetPropertyValueWord (pDMA, 4);
				Result.Size	  = (u64) m_pDTB->GetPropertyValueWord (pDMA, 5) << 32
							| m_pDTB->GetPropertyValueWord (pDMA, 6);

				return Result;
			}
		}
	}

	// use default setting, if DTB is not available
	Result.BusAddress = MEM_PCIE_DMA_RANGE_PCIE_START;
	Result.CPUAddress = MEM_PCIE_DMA_RANGE_START;
	Result.Size = (u64) m_nRAMSize * MEGABYTE;

	if (   m_MachineModel != MachineModel4B			// not for BCM2711B0
	    || m_nModelRevision >= 5)
	{
		if (m_nRAMSize >= 4096)
		{
			Result.BusAddress = 0x400000000ULL;
		}
	}

	return Result;
}

#endif

CMachineInfo *CMachineInfo::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}
