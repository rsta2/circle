//
// swdloader.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021  R. Stange <rsta2@o2online.de>
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
#include <pico/swdloader.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

//
// References:
//
// [1] ARM Debug Interface Architecture Specification ADIv5.0 to ADIv5.2, IHI 0031E
// [2] ARM v6-M Architecture Reference Manual, DDI 0419E
//

// Debug Port v2

#define TURN_CYCLES		1

// SWD-DP Requests
#define WR_DP_ABORT		0x81
	#define DP_ABORT_STKCMPCLR		BIT(1)
	#define DP_ABORT_STKERRCLR		BIT(2)
	#define DP_ABORT_WDERRCLR		BIT(3)
	#define DP_ABORT_ORUNERRCLR		BIT(4)
#define RD_DP_CTRL_STAT		0x8D
#define WR_DP_CTRL_STAT		0xA9
	#define DP_CTRL_STAT_ORUNDETECT		BIT(0)
	#define DP_CTRL_STAT_STICKYERR		BIT(5)
	#define DP_CTRL_STAT_CDBGPWRUPREQ	BIT(28)
	#define DP_CTRL_STAT_CDBGPWRUPACK	BIT(29)
	#define DP_CTRL_STAT_CSYSPWRUPREQ	BIT(30)
	#define DP_CTRL_STAT_CSYSPWRUPACK	BIT(31)
#define RD_DP_DPIDR		0xA5
	#define DP_DPIDR_SUPPORTED			0x0BC12477
#define RD_DP_RDBUFF		0xBD
#define WR_DP_SELECT		0xB1
	#define DP_SELECT_DPBANKSEL__SHIFT	0
	#define DP_SELECT_APBANKSEL__SHIFT	4
	#define DP_SELECT_APSEL__SHIFT		24
		#define DP_SELECT_DEFAULT		0	// DP bank 0, AP 0, AP bank 0
#define WR_DP_TARGETSEL		0x99
	#define DP_TARGETSEL_CPUAPID_SUPPORTED		0x01002927
	#define DP_TARGETSEL_TINSTANCE__SHIFT	28
		#define DP_TARGETSEL_TINSTANCE_CORE0	0
		#define DP_TARGETSEL_TINSTANCE_CORE1	1

// SW-DP response
#define DP_OK			0b001
#define DP_WAIT			0b010
#define DP_FAULT		0b100

// SWD MEM-AP Requests
#define WR_AP_CSW		0xA3
	#define AP_CSW_SIZE__SHIFT		0
		#define AP_CSW_SIZE_32BITS		2
	#define AP_CSW_ADDR_INC__SHIFT		4
		#define AP_CSW_SIZE_INCREMENT_SINGLE	1
	#define AP_CSW_DEVICE_EN		BIT(6)
	#define AP_CSW_PROT__SHIFT		24
		#define AP_CSW_PROT_DEFAULT		0x22
	#define AP_CSW_DBG_SW_ENABLE		BIT(31)
#define RD_AP_DRW		0x9F
#define WR_AP_DRW		0xBB
#define WR_AP_TAR		0x8B

// ARMv6-M Debug System Registers
#define DHCSR			0xE000EDF0
	#define DHCSR_C_DEBUGEN			BIT(0)
	#define DHCSR_C_HALT			BIT(1)
	#define DHCSR_DBGKEY__SHIFT		16
		#define DHCSR_DBGKEY_KEY		0xA05F
#define DCRSR			0xE000EDF4
	#define DCRSR_REGSEL__SHIFT		0
		#define DCRSR_REGSEL_R15		15	// PC register
	#define DCRSR_REGW_N_R			BIT(16)
#define DCRDR			0xE000EDF8

LOGMODULE ("swdloader");

CSWDLoader::CSWDLoader (unsigned nClockPin, unsigned nDataPin, unsigned nResetPin,
			unsigned nClockRateKHz)
:	m_bResetAvailable (nResetPin != 0),
	m_nDelayNanos (1000000U / nClockRateKHz / 2),
	m_ClockPin (nClockPin, GPIOModeOutput),
	m_DataPin (nDataPin, GPIOModeOutput),
	m_pTimer (CTimer::Get ())
{
	if (m_bResetAvailable)
	{
		m_ResetPin.AssignPin (nResetPin);

		m_ResetPin.SetMode (GPIOModeInput, FALSE);	// suppress LOW spike
		m_ResetPin.Write (HIGH);
		m_ResetPin.SetMode (GPIOModeOutput, FALSE);
	}

	m_DataPin.SetPullMode (GPIOPullModeUp);
}

CSWDLoader::~CSWDLoader (void)
{
	m_DataPin.SetMode (GPIOModeInput);
	m_ClockPin.SetMode (GPIOModeInput);

	if (m_bResetAvailable)
	{
		m_ResetPin.SetMode (GPIOModeInput);
	}
}

boolean CSWDLoader::Initialize (void)
{
	if (m_bResetAvailable)
	{
		m_pTimer->MsDelay (10);
		m_ResetPin.Write (LOW);
		m_pTimer->MsDelay (10);
		m_ResetPin.Write (HIGH);
		m_pTimer->MsDelay (10);
	}

	BeginTransaction ();

	Dormant2SWD ();
	WriteIdle ();
	LineReset ();

	// core 1 remains halted after reset
	SelectTarget (DP_TARGETSEL_CPUAPID_SUPPORTED, DP_TARGETSEL_TINSTANCE_CORE0);

	u32 nIDCode;
	if (!ReadData (RD_DP_DPIDR, &nIDCode))
	{
		LOGERR ("Target does not respond");

		return FALSE;
	}

	if (nIDCode != DP_DPIDR_SUPPORTED)
	{
		EndTransaction ();

		LOGERR ("Debug target not supported (ID code 0x%X)", nIDCode);

		return FALSE;
	}

	if (!PowerOn ())
	{
		LOGERR ("Target connect failed");

		return FALSE;
	}

	EndTransaction ();

	return TRUE;
}

boolean CSWDLoader::Load (const void *pProgram, size_t nProgSize, u32 nAddress)
{
	if (!Halt ())
	{
		return FALSE;
	}

	unsigned nStartTicks = m_pTimer->GetClockTicks ();

	if (!LoadChunk (pProgram, nProgSize, nAddress))
	{
		return FALSE;
	}

	unsigned nEndTicks = m_pTimer->GetClockTicks ();
	double fDuration = (double) (nEndTicks - nStartTicks) / CLOCKHZ;

	LOGNOTE ("%u bytes loaded in %.2f seconds (%.1f KBytes/s)",
		 nProgSize, fDuration, nProgSize / fDuration / 1024.0);

	return Start (nAddress);
}

boolean CSWDLoader::Halt (void)
{
	BeginTransaction ();

	if (   !WriteData (WR_AP_CSW,   (AP_CSW_SIZE_32BITS << AP_CSW_SIZE__SHIFT)
				      | (AP_CSW_SIZE_INCREMENT_SINGLE << AP_CSW_ADDR_INC__SHIFT)
				      | AP_CSW_DEVICE_EN
				      | (AP_CSW_PROT_DEFAULT << AP_CSW_PROT__SHIFT)
				      | AP_CSW_DBG_SW_ENABLE)
	    || !WriteMem (DHCSR,   DHCSR_C_DEBUGEN
				 | DHCSR_C_HALT
				 | (DHCSR_DBGKEY_KEY << DHCSR_DBGKEY__SHIFT)))
	{
		LOGERR ("Target halt failed");

		return FALSE;
	}

	EndTransaction ();

	return TRUE;
}

boolean CSWDLoader::LoadChunk (const void *pChunk, size_t nChunkSize, u32 nAddress)
{
	const u32 *pChunk32 = (const u32 *) pChunk;
	assert (pChunk32 != 0);

	u32 nFirstWord = *pChunk32;
	u32 nAddressCopy = nAddress;

	assert ((nChunkSize & 3) == 0);
	while (nChunkSize > 0)
	{
		BeginTransaction ();

		if (!WriteData (WR_AP_TAR, nAddress))
		{
			LOGERR ("Cannot write TAR (0x%X)", nAddress);

			return FALSE;
		}

		const size_t BlockSize = 1024;
		for (unsigned i = 0; i < BlockSize; i += 4)
		{
			if (!WriteData (WR_AP_DRW, *pChunk32++))
			{
				LOGERR ("Memory write failed (0x%X)", nAddress);

				return FALSE;
			}

			if (nChunkSize > 4)
			{
				nChunkSize -= 4;
			}
			else
			{
				nChunkSize = 0;
				break;
			}
		}

		nAddress += BlockSize;

		EndTransaction ();
	}

	BeginTransaction ();

	u32 nFirstWordRead;
	if (!ReadMem (nAddressCopy, &nFirstWordRead))
	{
		LOGERR ("Memory read failed (0x%X)", nAddressCopy);
	}

	EndTransaction ();

	if (nFirstWord != nFirstWordRead)
	{
		LOGERR ("Data mismatch (0x%X != 0x%X)", nFirstWord, nFirstWordRead);

		return FALSE;
	}

	return TRUE;
}

boolean CSWDLoader::Start (u32 nAddress)
{
	BeginTransaction ();

	if (   !WriteMem (DCRDR, nAddress)
	    || !WriteMem (DCRSR,   (DCRSR_REGSEL_R15 << DCRSR_REGSEL__SHIFT)
				 | DCRSR_REGW_N_R)
	    || !WriteMem (DHCSR,   DHCSR_C_DEBUGEN
				 | (DHCSR_DBGKEY_KEY << DHCSR_DBGKEY__SHIFT)))
	{
		LOGERR ("Target start failed");

		return FALSE;
	}

	EndTransaction ();

	return TRUE;
}

boolean CSWDLoader::PowerOn (void)
{
	if (!WriteData (WR_DP_ABORT,   DP_ABORT_STKCMPCLR
				     | DP_ABORT_STKERRCLR
				     | DP_ABORT_WDERRCLR
				     | DP_ABORT_ORUNERRCLR))
	{
		return FALSE;
	}

	if (!WriteData (WR_DP_SELECT, DP_SELECT_DEFAULT))
	{
		return FALSE;
	}

	if (!WriteData (WR_DP_CTRL_STAT,   DP_CTRL_STAT_ORUNDETECT
					 | DP_CTRL_STAT_STICKYERR
					 | DP_CTRL_STAT_CDBGPWRUPREQ
					 | DP_CTRL_STAT_CSYSPWRUPREQ))
	{
		return FALSE;
	}

	u32 nCtrlStat;
	if (!ReadData (RD_DP_CTRL_STAT, &nCtrlStat))
	{
		return FALSE;
	}

	if (   !(nCtrlStat & DP_CTRL_STAT_CDBGPWRUPACK)
	    || !(nCtrlStat & DP_CTRL_STAT_CSYSPWRUPACK))
	{
		EndTransaction ();

		return FALSE;
	}

	return TRUE;
}

boolean CSWDLoader::WriteMem (u32 nAddress, u32 nData)
{
	return    WriteData (WR_AP_TAR, nAddress)
	       && WriteData (WR_AP_DRW, nData);
}

boolean CSWDLoader::ReadMem (u32 nAddress, u32 *pData)
{
	return    WriteData (WR_AP_TAR, nAddress)
	       && ReadData (RD_AP_DRW, pData)
	       && ReadData (RD_DP_RDBUFF, pData);
}

boolean CSWDLoader::WriteData (u8 nRequest, u32 nData)
{
	WriteBits (nRequest, 7);

	assert (nRequest & 0x80);
	ReadBits (1 + TURN_CYCLES);	// park bit (not driven) and turn cycle

	u32 nResponse = ReadBits (3);

	ReadBits (TURN_CYCLES);

	if (nResponse != DP_OK)
	{
		EndTransaction ();

		LOGWARN ("Cannot write (req 0x%02X, data 0x%X, resp %u)",
			 (unsigned) nRequest, nData, nResponse);

		return FALSE;
	}

	WriteBits (nData, 32);
	WriteBits (parity32 (nData), 1);

	return TRUE;
}

boolean CSWDLoader::ReadData (u8 nRequest, u32 *pData)
{
	WriteBits (nRequest, 7);

	assert (nRequest & 0x80);
	ReadBits (1 + TURN_CYCLES);	// park bit (not driven) and turn cycle

	u32 nResponse = ReadBits (3);

	if (nResponse != DP_OK)
	{
		ReadBits (TURN_CYCLES);

		EndTransaction ();

		LOGWARN ("Cannot read (req 0x%02X, resp %u)", (unsigned) nRequest, nResponse);

		return FALSE;
	}

	u32 nData = ReadBits (32);

	u32 nParity = ReadBits (1);
	if (nParity != parity32 (nData))
	{
		ReadBits (TURN_CYCLES);

		EndTransaction ();

		LOGWARN ("Parity error (req 0x%02X)", (unsigned) nRequest);

		return FALSE;
	}

	assert (pData != 0);
	*pData = nData;

	ReadBits (TURN_CYCLES);

	return TRUE;
}

void CSWDLoader::SelectTarget (u32 nCPUAPID, u8 uchInstanceID)
{
	u32 nWData = nCPUAPID | ((u32) uchInstanceID << DP_TARGETSEL_TINSTANCE__SHIFT);

	WriteBits (WR_DP_TARGETSEL, 7);

	ReadBits (1 + 5);	// park bit and 5 bits not driven

	WriteBits (nWData, 32);
	WriteBits (parity32 (nWData), 1);
}

void CSWDLoader::BeginTransaction (void)
{
	EnterCritical ();

	WriteIdle ();
}

void CSWDLoader::EndTransaction (void)
{
	WriteIdle ();

	LeaveCritical ();
}

// Leaving dormant state and switch to SW-DP ([1] section B5.3.4)
void CSWDLoader::Dormant2SWD (void)
{
	WriteBits (0xFF, 8);		// 8 cycles high

	WriteBits (0x6209F392U, 32);	// selection alert sequence
	WriteBits (0x86852D95U, 32);
	WriteBits (0xE3DDAFE9U, 32);
	WriteBits (0x19BC0EA2U, 32);

	WriteBits (0x0, 4);		// 4 cycles low

	WriteBits (0x1A, 8);		// activation code
}

void CSWDLoader::LineReset (void)
{
	WriteBits (0xFFFFFFFFU, 32);
	WriteBits (0x00FFFFFU, 28);
}

void CSWDLoader::WriteIdle (void)
{
	WriteBits (0, 8);

	m_ClockPin.Write (LOW);

	m_DataPin.SetMode (GPIOModeOutput, FALSE);
	m_DataPin.Write (LOW);
}

void CSWDLoader::WriteBits (u32 nBits, unsigned nBitCount)
{
	m_DataPin.SetMode (GPIOModeOutput, FALSE);

	while (nBitCount--)
	{
		m_DataPin.Write (nBits & 1);

		WriteClock ();

		nBits >>= 1;
	}
}

u32 CSWDLoader::ReadBits (unsigned nBitCount)
{
	m_DataPin.SetMode (GPIOModeInput, FALSE);

	u32 nBits = 0;
	unsigned nRemaining = nBitCount--;
	while (nRemaining--)
	{
		unsigned nLevel = m_DataPin.Read ();

		WriteClock ();

		nBits >>= 1;
		nBits |= nLevel << nBitCount;
	}

	return nBits;
}

void CSWDLoader::WriteClock (void)
{
	m_ClockPin.Write (LOW);
	m_pTimer->nsDelay (m_nDelayNanos);

	m_ClockPin.Write (HIGH);
	m_pTimer->nsDelay (m_nDelayNanos);
}
