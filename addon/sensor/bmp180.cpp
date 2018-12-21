//
// bmp180.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
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
#include <sensor/bmp180.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

// Configuration
#define OVERSAMPLE	3    		// 0..3

// Registers
#define CHIPID		0xD0
	#define CHIPID_BMP180	0x55
#define EEPROM		0xAA
#define MEASURE		0xF4
	#define MEASURE_TEMP	0x2E
	#define MEASURE_PRES	0x34
#define RESULT		0xF6

struct TBMP180EEPROM
{
	u16	AC1;
	u16	AC2;
	u16	AC3;
	u16	AC4;
	u16	AC5;
	u16	AC6;
	u16	B1;
	u16	B2;
	u16	MB;
	u16	MC;
	u16	MD;
}
PACKED;

static const char FromBMP180[] = "bmp180";

CBMP180::CBMP180 (CI2CMaster *pI2CMaster, unsigned nI2CClockHz, u8 ucSlaveAddress)
:	m_pI2CMaster (pI2CMaster),
	m_nI2CClockHz (nI2CClockHz),
	m_ucSlaveAddress (ucSlaveAddress),
	m_nTemperature (0),
	m_nPressure (0)
{
}

CBMP180::~CBMP180 (void)
{
	m_pI2CMaster = 0;
}

boolean CBMP180::Initialize (void)
{
	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	u8 ucChipID;
	if (!WriteRead (CHIPID, &ucChipID, sizeof ucChipID))
	{
		CLogger::Get ()->Write (FromBMP180, LogError, "Chip at 0x%X does not respond",
					(unsigned) m_ucSlaveAddress);

		return FALSE;
	}

	if (ucChipID != CHIPID_BMP180)
	{
		CLogger::Get ()->Write (FromBMP180, LogError,
					"Chip at 0x%x is not a BMP180 (id 0x%X)",
					(unsigned) m_ucSlaveAddress, (unsigned) ucChipID);

		return FALSE;
	}

	TBMP180EEPROM EEPROMData;
	if (!WriteRead (EEPROM, &EEPROMData, sizeof EEPROMData))
	{
		CLogger::Get ()->Write (FromBMP180, LogError, "Cannot read calibration data");

		return FALSE;
	}

	m_AC1 = (s16) bswap16 (EEPROMData.AC1);
	m_AC2 = (s16) bswap16 (EEPROMData.AC2);
	m_AC3 = (s16) bswap16 (EEPROMData.AC3);
	m_AC4 = bswap16 (EEPROMData.AC4);
	m_AC5 = bswap16 (EEPROMData.AC5);
	m_AC6 = bswap16 (EEPROMData.AC6);
	m_B1  = (s16) bswap16 (EEPROMData.B1);
	m_B2  = (s16) bswap16 (EEPROMData.B2);
	m_MB  = (s16) bswap16 (EEPROMData.MB);
	m_MC  = (s16) bswap16 (EEPROMData.MC);
	m_MD  = (s16) bswap16 (EEPROMData.MD);

	return TRUE;
}

boolean CBMP180::DoMeasurement (void)
{
	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	// Measure temperature

	s32 UT = DoMeasurement (MEASURE_TEMP, 5);
	if (UT == BMP180_I2C_ERROR)
	{
		return FALSE;
	}

	s32 X1 = ((UT - m_AC6) * m_AC5) >> 15;
	s32 X2 = (m_MC << 11) / (X1 + m_MD);
	s32 B5 = X1 + X2;
	m_nTemperature = (B5 + 8) >> 4;

	// Measure pressure

	s32 UP = DoMeasurement (MEASURE_PRES | (OVERSAMPLE << 6), 26, OVERSAMPLE);
	if (UP == BMP180_I2C_ERROR)
	{
		return FALSE;
	}

	s32 B6 = B5 - 4000;
	s32 B62 = B6 * B6 >> 12;
	X1 = (m_B2 * B62) >> 11;
	X2 = m_AC2 * B6 >> 11;
	s32 X3 = X1 + X2;
	s32 B3 = (((m_AC1 * 4 + X3) << OVERSAMPLE) + 2) >> 2;
	X1 = m_AC3 * B6 >> 13;
	X2 = (m_B1 * B62) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	s32 B4 = (m_AC4 * (X3 + 32768)) >> 15;
	u32 B7 = (UP - B3) * (50000 >> OVERSAMPLE);

	s32 P;
	if (B7 < 0x80000000)
	{
		P = (B7 * 2) / B4;
	}
	else
	{
		P = (B7 / B4) * 2;
	}

	X1 = (P >> 8) * (P >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * P) >> 16;
	m_nPressure = P + ((X1 + X2 + 3791) >> 4);

	return TRUE;
}

int CBMP180::GetTemperature (void)
{
	return m_nTemperature;
}

int CBMP180::GetPressure (void)
{
	return m_nPressure;
}

s32 CBMP180::DoMeasurement (u8 ucCmd, unsigned nDurationMs, unsigned nOversample)
{
	assert (m_pI2CMaster != 0);

	const u8 MeasureCmd[] = {MEASURE, ucCmd};
	int nResult = m_pI2CMaster->Write (m_ucSlaveAddress, MeasureCmd, sizeof MeasureCmd);
	if (nResult != sizeof MeasureCmd)
	{
		CLogger::Get ()->Write (FromBMP180, LogWarning,
					"I2C write failed (err %d)", nResult);

		return BMP180_I2C_ERROR;
	}

	CTimer::Get ()->MsDelay (nDurationMs);

	u8 ResultBuffer[3];
	if (!WriteRead (RESULT, ResultBuffer, sizeof ResultBuffer))
	{
		return BMP180_I2C_ERROR;
	}

	s32 nValue =   (int) ResultBuffer[0] << 16
		     | (int) ResultBuffer[1] << 8
		     | (int) ResultBuffer[2];

	assert (nOversample <= 3);
	nValue >>= 8 - nOversample;

	return nValue;
}

boolean CBMP180::WriteRead (u8 ucRegister, void *pBuffer, unsigned nCount)
{
	assert (m_pI2CMaster != 0);

	int nResult = m_pI2CMaster->Write (m_ucSlaveAddress, &ucRegister, sizeof ucRegister);
	if (nResult != sizeof ucRegister)
	{
		CLogger::Get ()->Write (FromBMP180, LogWarning,
					"I2C write failed (err %d)", nResult);

		return FALSE;
	}

	assert (pBuffer != 0);
	assert (nCount > 0);
	nResult = m_pI2CMaster->Read (m_ucSlaveAddress, pBuffer, nCount);
	if (nResult != (int) nCount)
	{
		CLogger::Get ()->Write (FromBMP180, LogWarning,
					"I2C read failed (err %d)", nResult);

		return FALSE;
	}

	return TRUE;
}
