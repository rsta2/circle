//
// ds18x20.cpp
//
// Taken from the:
// 	OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// 	http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// 	The DallasTemperature library can do all this work for you!
// 	http://milesburton.com/Dallas_Temperature_Control_Library
//
#include "ds18x20.h"
#include <circle/timer.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromDS18x20[] = "ds18x20";

CDS18x20::CDS18x20 (OneWire *pOneWire)
:	m_pOneWire (pOneWire),
	m_nCelsius (-100),
	m_nFahrenheit (-100)
{
	assert (m_pOneWire != 0);
}

CDS18x20::~CDS18x20 (void)
{
	m_pOneWire = 0;
}

boolean CDS18x20::Initialize (void)
{
	assert (m_pOneWire != 0);

	if (!m_pOneWire->search (m_Address))
	{
		CLogger::Get ()->Write (FromDS18x20, LogError, "Sensor not found");

		return FALSE;
	}

	if (OneWire::crc8 (m_Address, 7) != m_Address[7])
	{
		CLogger::Get ()->Write (FromDS18x20, LogError, "CRC is not valid");

		return FALSE;
	}

	const char *pChip = 0;

	// the first ROM byte indicates which chip
	switch (m_Address[0])
	{
	case 0x10:
		pChip = "DS18S20";	// or old DS1820
		m_bIs18S20 = TRUE;
		break;

	case 0x28:
		pChip = "DS18B20";
		m_bIs18S20 = FALSE;
		break;

	case 0x22:
		pChip = "DS1822";
		m_bIs18S20 = FALSE;
		break;

	default:
		CLogger::Get ()->Write (FromDS18x20, LogError, "Not a DS18x20 family device");
		return FALSE;
	}

	assert (pChip != 0);
	CLogger::Get ()->Write (FromDS18x20, LogNotice, "%s found", pChip);

	return TRUE;
}

boolean CDS18x20::StartConversion (void)
{
	m_pOneWire->reset ();
	m_pOneWire->select (m_Address);
	m_pOneWire->write (0x44, 1);		// start conversion, with parasite power on at the end
  
	CTimer::Get ()->MsDelay (1000);		// maybe 750ms is enough, maybe not

	// we might do a m_pOneWire->depower() here, but the reset will take care of it.
	if (!m_pOneWire->reset ())
	{
		CLogger::Get ()->Write (FromDS18x20, LogWarning, "Device does not respond");

		return FALSE;
	}

	m_pOneWire->select (m_Address);
	m_pOneWire->write (0xBE);		// read Scratchpad

	u8 Data[9];
	for (unsigned i = 0; i < 9; i++)
	{
		Data[i] = m_pOneWire->read ();
	}

	if (OneWire::crc8 (Data, 8) != Data[8])
	{
		CLogger::Get ()->Write (FromDS18x20, LogWarning, "CRC is not valid");

		return FALSE;
	}
	
	// Convert the data to actual temperature
	// because the result is a 16 bit signed integer, it should
	// be stored to an "int16_t" type, which is always 16 bits
	// even when compiled on a 32 bit processor.
	short nRaw = (Data[1] << 8) | Data[0];

	if (m_bIs18S20)
	{
		nRaw = nRaw << 3;		// 9 bit resolution default

		if (Data[7] == 0x10)
		{
			// "count remain" gives full 12 bit resolution
			nRaw = (nRaw & 0xFFF0) + 12 - Data[6];
		}
	}
	else
	{
		u8 ucConfig = Data[4] & 0x60;

		// at lower res, the low bits are undefined, so let's zero them
		if (ucConfig == 0x00)
		{
			nRaw = nRaw & ~7;	// 9 bit resolution, 93.75 ms
		}
		else if (ucConfig == 0x20)
		{
			nRaw = nRaw & ~3;	// 10 bit, 187.5 ms
		}
		else if (ucConfig == 0x40)
		{
			nRaw = nRaw & ~1;	// 11 bit, 375 ms
		}

		// default is 12 bit resolution, 750 ms conversion time
	}

	m_nCelsius = nRaw * 10 / 16;
	m_nFahrenheit = (m_nCelsius * 18) / 10 + 320;

	m_nCelsiusFract = m_nCelsius % 10;
	m_nCelsius /= 10;

	m_nFahrenheitFract = m_nFahrenheit % 10;
	m_nFahrenheit /= 10;

	return TRUE;
}

int  CDS18x20::GetCelsius (void) const
{
	return m_nCelsius;
}

int CDS18x20::GetCelsiusFract (void) const
{
	return m_nCelsiusFract;
}

int CDS18x20::GetFahrenheit (void) const
{
	return m_nFahrenheit;
}

int CDS18x20::GetFahrenheitFract (void) const
{
	return m_nFahrenheitFract;
}
