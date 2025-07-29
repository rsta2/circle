//
// machineinfo.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2025  R. Stange <rsta2@gmx.net>
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
#include <circle/bcm2712.h>
#include <circle/memio.h>
#include <circle/util.h>
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
	{21, MachineModelCM4S,		4},
	{23, MachineModel5,		5},
	{24, MachineModelCM5,		5},
	{25, MachineModel500,		5},
	{26, MachineModelCM5Lite,	5}
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
	"Raspberry Pi 5",
	"Raspberry Pi 500",
	"Compute Module 5",
	"Compute Module 5 Lite",
	"Unknown"
};

static const char *s_SoCName[] =		// must match TSoCType
{
	"BCM2835",
	"BCM2836",
	"BCM2837",
	"BCM2711",
	"BCM2712",
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
	9 | ACTLED_ACTIVE_LOW,		// 5	 (at GPIO chip #2)
	9 | ACTLED_ACTIVE_LOW,		// 500
	9 | ACTLED_ACTIVE_LOW,		// CM5
	9 | ACTLED_ACTIVE_LOW,		// CM5 Lite

	ACTLED_UNKNOWN			// Unknown
};

CMachineInfo *CMachineInfo::s_pThis = 0;

CMachineInfo::CMachineInfo (void)
:	m_nRevisionRaw (0),
	m_MachineModel (MachineModelUnknown),
	m_nModelMajor (0),
	m_nModelRevision (0),
	m_SoCType (SoCTypeUnknown),
	m_SoCStepping (SoCSteppingUnknown),
	m_nRAMSize (0),
#if RASPPI <= 3
	m_usDMAChannelMap (0x1F35)	// default mapping
#elif RASPPI == 4
	m_usDMAChannelMap (0x71F5),	// default mapping
	m_pDTB (0)
#else
	m_usDMAChannelMap (0x0FF5),	// default mapping
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
		m_SoCStepping	 = s_pThis->m_SoCStepping;
		m_nRAMSize	 = s_pThis->m_nRAMSize;
#if RASPPI >= 4
		m_pDTB		 = s_pThis->m_pDTB;
#endif

		return;
	}
	s_pThis = this;

#if RASPPI >= 4
	FetchDTB ();
#endif

	CBcmPropertyTags Tags;
#if RASPPI <= 4
	TPropertyTagSimple DMAChannels;
	if (Tags.GetTag (PROPTAG_GET_DMA_CHANNELS, &DMAChannels, sizeof DMAChannels))
	{
		m_usDMAChannelMap = (u16) DMAChannels.nValue;
	}
#else
	const TDeviceTreeNode *pDMANode;
	const TDeviceTreeProperty *pChannelMask;
	if (   m_pDTB
	    && (pDMANode = m_pDTB->FindNode ("/axi/dma@10000"))		// DMA32 (6 channels)
	    && (pChannelMask = m_pDTB->FindProperty (pDMANode, "brcm,dma-channel-mask")))
	{
		m_usDMAChannelMap &= ~0x3F;
		m_usDMAChannelMap |= (u16) m_pDTB->GetPropertyValueWord (pChannelMask, 0) & 0x3F;
	}

	if (   m_pDTB
	    && (pDMANode = m_pDTB->FindNode ("/axi/dma@10600"))		// DMA40 (6 channels)
	    && (pChannelMask = m_pDTB->FindProperty (pDMANode, "brcm,dma-channel-mask")))
	{
		m_usDMAChannelMap &= ~0xFC0;
		m_usDMAChannelMap |= (u16) m_pDTB->GetPropertyValueWord (pChannelMask, 0) & 0xFC0;
	}
#endif

	TPropertyTagSimple BoardRevision;
	if (!Tags.GetTag (PROPTAG_GET_BOARD_REVISION, &BoardRevision, sizeof BoardRevision))
	{
#if RASPPI >= 4
		const TDeviceTreeNode *pSystemNode;
		const TDeviceTreeProperty *pRevision;

		if (   !m_pDTB
		    || !(pSystemNode = m_pDTB->FindNode ("/system"))
		    || !(pRevision = m_pDTB->FindProperty (pSystemNode, "linux,revision")))
		{
			return;
		}

		BoardRevision.nValue = m_pDTB->GetPropertyValueWord (pRevision, 0);
#else
		return;
#endif
	}

#if RASPPI == 5
	// See: https://forums.raspberrypi.com/viewtopic.php?p=2247906#p2247856
	u32 nStepping = read32 (ARM_SOC_STEPPING);
	if ((nStepping >> 16) == 0x2712)
	{
		m_SoCStepping = static_cast<TSoCStepping> (nStepping & 0xFF);
	}
#endif

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

	if (s_pThis == this)
	{
#if RASPPI >= 4
		delete m_pDTB;
		m_pDTB = 0;
#endif

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

TSoCStepping CMachineInfo::GetSoCStepping (void) const
{
	return m_SoCStepping;
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
#if RASPPI <= 4
		nResult = 100000000;
#else
		nResult = 200000000;
#endif
		break;

	case CLOCK_ID_UART:
#if RASPPI <= 4
		nResult = 48000000;
#else
		nResult = 44000000;
#endif
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
#if defined (USE_PWM_AUDIO_ON_ZERO) || RASPPI >= 5
		if (   m_MachineModel == MachineModelZero
		    || m_MachineModel == MachineModelZeroW
		    || m_MachineModel == MachineModelZero2W
		    || m_MachineModel == MachineModel5)
		{
#if defined (USE_GPIO18_FOR_LEFT_PWM_ON_ZERO) || defined (USE_GPIO18_FOR_LEFT_PWM)
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
#if defined (USE_PWM_AUDIO_ON_ZERO) || RASPPI >= 5
		if (   m_MachineModel == MachineModelZero
		    || m_MachineModel == MachineModelZeroW
		    || m_MachineModel == MachineModelZero2W
		    || m_MachineModel == MachineModel5)
		{
#if defined (USE_GPIO19_FOR_RIGHT_PWM_ON_ZERO) || defined (USE_GPIO19_FOR_RIGHT_PWM)
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
#if RASPPI <= 4
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
#else
	switch (nSourceId)
	{
	case GPIOClockSourceXOscillator:	return 50000000;

	case GPIOClockSourcePLLSys:		return 200000000;
	case GPIOClockSourcePLLSysSec:		return 125000000;
	case GPIOClockSourcePLLSysPriPh:	return 100000000;

	case GPIOClockSourceClkSys:		return 200000000;

	default:
		break;
	}
#endif

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
	       && m_MachineModel != MachineModelZero2W
	       && m_MachineModel != MachineModel5;
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
#if RASPPI <= 3
		assert (nChannel <= DMA_CHANNEL_MAX);
#else
		assert (nChannel <= DMA_CHANNEL_EXT_MAX);
#endif
		if (m_usDMAChannelMap & (1 << nChannel))
		{
			m_usDMAChannelMap &= ~(1 << nChannel);

			return nChannel;
		}
	}
	else
	{
		// arbitrary channel allocation
#if RASPPI <= 4
		int i = nChannel == DMA_CHANNEL_NORMAL ? 6 : DMA_CHANNEL_MAX;
#else
		int i = DMA_CHANNEL_MAX;
#endif
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

#if RASPPI <= 3
	assert (nChannel <= DMA_CHANNEL_MAX);
#else
	assert (nChannel <= DMA_CHANNEL_EXT_MAX);
#endif
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

const CDeviceTreeBlob *CMachineInfo::GetDTB (void) const
{
	return m_pDTB;
}

TMemoryWindow CMachineInfo::GetPCIeMemory (unsigned nBus) const
{
	assert (s_pThis != 0);
	if (s_pThis != this)
	{
		return s_pThis->GetPCIeMemory (nBus);
	}

	TMemoryWindow Result;

	if (nBus == PCIE_BUS_ONBOARD)
	{
		Result.CPUAddress = MEM_PCIE_RANGE_START;
		Result.BusAddress = MEM_PCIE_RANGE_PCIE_START;
		Result.Size = MEM_PCIE_RANGE_SIZE;
	}
	else
	{
#if RASPPI >= 5
		Result.CPUAddress = MEM_PCIE_EXT_RANGE_START;
		Result.BusAddress = MEM_PCIE_EXT_RANGE_PCIE_START;
		Result.Size = MEM_PCIE_EXT_RANGE_SIZE;
#else
		assert (0);
#endif
	}

	return Result;
}

TMemoryWindow CMachineInfo::GetPCIeDMAMemory (unsigned nBus) const
{
	assert (s_pThis != 0);
	if (s_pThis != this)
	{
		return s_pThis->GetPCIeDMAMemory (nBus);
	}

	TMemoryWindow Result;

	if (m_pDTB != 0)
	{
		const char *pPCIePath;
		unsigned n, i;
#if RASPPI == 4
		assert (nBus == PCIE_BUS_ONBOARD);
		// there is one inbound window only
		pPCIePath = "/scb/pcie@7d500000";
		n = 1*7;
		i = 0;
#else
		if (nBus == PCIE_BUS_ONBOARD)
		{
			// TODO: for now we map only the second inbound window
			pPCIePath = "/axi/pcie@1000120000";
			n = 2*7;
			i = 7;
		}
		else
		{
			assert (nBus == PCIE_BUS_EXTERNAL);
			// there is one inbound window only
			pPCIePath = "/axi/pcie@1000110000";
			n = 1*7;
			i = 0;
		}
#endif
		const TDeviceTreeNode *pPCIe = m_pDTB->FindNode (pPCIePath);
		if (pPCIe != 0)
		{
			const TDeviceTreeProperty *pDMA = m_pDTB->FindProperty (pPCIe, "dma-ranges");
			if (   pDMA != 0
			    && m_pDTB->GetPropertyValueLength (pDMA) == sizeof (u32)*n)
			{
				Result.BusAddress = (u64) m_pDTB->GetPropertyValueWord (pDMA, i+1) << 32
							| m_pDTB->GetPropertyValueWord (pDMA, i+2);
				Result.CPUAddress = (u64) m_pDTB->GetPropertyValueWord (pDMA, i+3) << 32
							| m_pDTB->GetPropertyValueWord (pDMA, i+4);
				Result.Size	  = (u64) m_pDTB->GetPropertyValueWord (pDMA, i+5) << 32
							| m_pDTB->GetPropertyValueWord (pDMA, i+6);

				return Result;
			}
		}
	}

	// use default setting, if DTB is not available
	Result.BusAddress = MEM_PCIE_DMA_RANGE_PCIE_START;
	Result.CPUAddress = MEM_PCIE_DMA_RANGE_START;
	Result.Size = (u64) m_nRAMSize * MEGABYTE;

#if RASPPI == 4
	if (   m_MachineModel != MachineModel4B			// not for BCM2711B0
	    || m_nModelRevision >= 5)
	{
		if (m_nRAMSize >= 4096)
		{
			Result.BusAddress = 0x400000000ULL;
		}
	}
#endif

	return Result;
}

#endif

CMachineInfo *CMachineInfo::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}
