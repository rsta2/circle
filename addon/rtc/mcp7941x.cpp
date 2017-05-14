//
// mcp7941x.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
//
// Portions by Arjan van Vught <info@raspberrypi-dmx.nl>
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
#include <rtc/mcp7941x.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

// Time and Configuration Registers (TCR)
#define MCP7941X_RTCC_TCR_SECONDS	0x00
	#define MCP7941X_RTCC_BIT_ST		0x80
#define MCP7941X_RTCC_TCR_MINUTES	0x01
#define MCP7941X_RTCC_TCR_HOURS		0x02
	#define MCP7941X_RTCC_BIT_12		0x40
#define MCP7941X_RTCC_TCR_DAY		0x03
	#define MCP7941X_RTCC_BIT_VBATEN	0x08
#define MCP7941X_RTCC_TCR_DATE		0x04
#define MCP7941X_RTCC_TCR_MONTH		0x05
#define MCP7941X_RTCC_TCR_YEAR		0x06

#define MCP7941X_RTCC_SRAM_START	0x20
#define MCP7941X_RTCC_SRAM_END		0x5F
#define MCP7941X_RTCC_SRAM_SIZE		0x40

#define BCD2DEC(val)			(((val) & 0x0F) + ((val) >> 4) * 10)
#define DEC2BCD(val)			((((val) / 10) << 4) + (val) % 10)

const char FromMCP7941X[] = "mcp7941x";

CMCP7941X::CMCP7941X (CI2CMaster *pI2CMaster, unsigned nI2CClockHz, u8 ucSlaveAddress)
:	m_pI2CMaster (pI2CMaster),
	m_nI2CClockHz (nI2CClockHz),
	m_ucSlaveAddress (ucSlaveAddress)
{
}

CMCP7941X::~CMCP7941X (void)
{
	m_pI2CMaster = 0;
}

boolean CMCP7941X::Initialize (void)
{
	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	if (m_pI2CMaster->Write (m_ucSlaveAddress, 0, 0) != 0)
	{
		CLogger::Get ()->Write (FromMCP7941X, LogError, "Device is not present (addr 0x%02X)",
					(unsigned) m_ucSlaveAddress);

		return FALSE;
	}

	return TRUE;
}

boolean CMCP7941X::Get (CTime *pTime)
{
	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	u8 Cmd[] = {MCP7941X_RTCC_TCR_SECONDS};
	int nResult = m_pI2CMaster->Write (m_ucSlaveAddress, Cmd, sizeof Cmd);
	if (nResult != sizeof Cmd)
	{
		CLogger::Get ()->Write (FromMCP7941X, LogError, "I2C write failed (err %d)", nResult);

		return FALSE;
	}

	u8 Reg[7];
	nResult = m_pI2CMaster->Read (m_ucSlaveAddress, Reg, sizeof Reg);
	if (nResult != sizeof Reg)
	{
		CLogger::Get ()->Write (FromMCP7941X, LogError, "I2C read failed (err %d)", nResult);

		return FALSE;
	}

	if (Reg[MCP7941X_RTCC_TCR_HOURS] & MCP7941X_RTCC_BIT_12)
	{
		CLogger::Get ()->Write (FromMCP7941X, LogError, "RTC runs in 12 hours mode");

		return FALSE;
	}

	assert (pTime != 0);
	if (!pTime->SetTime (BCD2DEC (Reg[MCP7941X_RTCC_TCR_HOURS]   & 0x3F),
			     BCD2DEC (Reg[MCP7941X_RTCC_TCR_MINUTES] & 0x7F),
			     BCD2DEC (Reg[MCP7941X_RTCC_TCR_SECONDS] & 0x7F)))
	{
		CLogger::Get ()->Write (FromMCP7941X, LogError, "Invalid time read");

		return FALSE;
	}

	if (!pTime->SetDate (BCD2DEC (Reg[MCP7941X_RTCC_TCR_DATE]  & 0x3F),
			     BCD2DEC (Reg[MCP7941X_RTCC_TCR_MONTH] & 0x1F),
			     BCD2DEC (Reg[MCP7941X_RTCC_TCR_YEAR]) + 2000))
	{
		CLogger::Get ()->Write (FromMCP7941X, LogError, "Invalid date read");

		return FALSE;
	}

	return TRUE;
}

boolean CMCP7941X::Set (const CTime &Time)
{
	u8 Reg[7];
	Reg[MCP7941X_RTCC_TCR_SECONDS] = DEC2BCD ((u8) Time.GetSeconds ()     & 0x7F);
	Reg[MCP7941X_RTCC_TCR_MINUTES] = DEC2BCD ((u8) Time.GetMinutes ()     & 0x7F);
	Reg[MCP7941X_RTCC_TCR_HOURS]   = DEC2BCD ((u8) Time.GetHours ()       & 0x1F);
	Reg[MCP7941X_RTCC_TCR_DAY]     = DEC2BCD ((u8) (Time.GetWeekDay ()+1) & 0x07);
	Reg[MCP7941X_RTCC_TCR_DATE]    = DEC2BCD ((u8) Time.GetMonthDay ()    & 0x3F);
	Reg[MCP7941X_RTCC_TCR_MONTH]   = DEC2BCD ((u8) Time.GetMonth ()       & 0x1F);
	Reg[MCP7941X_RTCC_TCR_YEAR]    = DEC2BCD ((u8) (Time.GetYear ()-2000) & 0xFF);

	Reg[MCP7941X_RTCC_TCR_SECONDS] |= MCP7941X_RTCC_BIT_ST;
	Reg[MCP7941X_RTCC_TCR_DAY]     |= MCP7941X_RTCC_BIT_VBATEN;

	u8 Data[8] = {MCP7941X_RTCC_TCR_SECONDS};
	memcpy (Data+1, Reg, sizeof Reg);

	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	unsigned nResult = m_pI2CMaster->Write (m_ucSlaveAddress, Data, sizeof Data);
	if (nResult != sizeof Data)
	{
		CLogger::Get ()->Write (FromMCP7941X, LogError, "I2C write failed (err %d)", nResult);

		return FALSE;
	}

	return TRUE;
}

boolean CMCP7941X::ReadSRAM (u8 ucAddress, void *pBuffer, unsigned nCount)
{
	assert (ucAddress < MCP7941X_RTCC_SRAM_SIZE);
	assert (pBuffer != 0);
	assert (1 < nCount && nCount <= MCP7941X_RTCC_SRAM_SIZE);

	ucAddress += MCP7941X_RTCC_SRAM_START;
	assert (ucAddress+nCount <= MCP7941X_RTCC_SRAM_END+1);

	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	u8 Cmd[] = {ucAddress};
	int nResult = m_pI2CMaster->Write (m_ucSlaveAddress, Cmd, sizeof Cmd);
	if (nResult != sizeof Cmd)
	{
		CLogger::Get ()->Write (FromMCP7941X, LogError, "I2C write failed (err %d)", nResult);

		return FALSE;
	}

	nResult = m_pI2CMaster->Read (m_ucSlaveAddress, pBuffer, nCount);
	if (nResult != (int) nCount)
	{
		CLogger::Get ()->Write (FromMCP7941X, LogError, "I2C read failed (err %d)", nResult);

		return FALSE;
	}

	return TRUE;
}

boolean CMCP7941X::WriteSRAM (u8 ucAddress, const void *pBuffer, unsigned nCount)
{
	assert (ucAddress < MCP7941X_RTCC_SRAM_SIZE);
	assert (pBuffer != 0);
	assert (1 < nCount && nCount <= MCP7941X_RTCC_SRAM_SIZE);

	ucAddress += MCP7941X_RTCC_SRAM_START;
	assert (ucAddress+nCount <= MCP7941X_RTCC_SRAM_END+1);

	u8 Data[MCP7941X_RTCC_SRAM_SIZE+1] = {ucAddress};
	memcpy (Data+1, pBuffer, nCount);

	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	unsigned nResult = m_pI2CMaster->Write (m_ucSlaveAddress, Data, sizeof Data);
	if (nResult != sizeof Data)
	{
		CLogger::Get ()->Write (FromMCP7941X, LogError, "I2C write failed (err %d)", nResult);

		return FALSE;
	}

	return TRUE;
}
