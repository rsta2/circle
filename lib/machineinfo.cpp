//
// machineinfo.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
	{9, MachineModelZero,		1}
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
	"Raspberry Pi 2 Model B",
	"Raspberry Pi 3 Model B",
	"Compute Module",
	"Unknown"
};

static const char *s_SoCName[] =		// must match TSoCType
{
	"BCM2835",
	"BCM2836",
	"BCM2837",
	"Unknown"
};

CMachineInfo *CMachineInfo::s_pThis = 0;

CMachineInfo::CMachineInfo (void)
:	m_nRevisionRaw (0),
	m_MachineModel (MachineModelUnknown),
	m_nModelMajor (0),
	m_nModelRevision (0),
	m_SoCType (SoCTypeUnknown),
	m_nRAMSize (0)
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

	CBcmPropertyTags Tags;
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

unsigned CMachineInfo::GetClockRate (u32 nClockId) const
{
	CBcmPropertyTags Tags;
	TPropertyTagClockRate TagClockRate;
	TagClockRate.nClockId = nClockId;
	if (Tags.GetTag (PROPTAG_GET_CLOCK_RATE, &TagClockRate, sizeof TagClockRate, 4))
	{
		return TagClockRate.nRate;
	}

	// if clock rate can not be requested, use a default rate
	unsigned nResult = 0;

	switch (nClockId)
	{
	case CLOCK_ID_EMMC:
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

CMachineInfo *CMachineInfo::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}
