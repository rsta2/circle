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
#include <OneWire/ds18x20.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>

static const char FromDS18x20[] = "ds18x20";

CDS18x20::CDS18x20 (OneWire *pOneWire, const u8 *pAddress)
:	m_pOneWire (pOneWire),
	m_bSearch (TRUE),
	m_fCelsius (-100.0),
	m_fFahrenheit (-100.0)
{
	assert (m_pOneWire != 0);

	if (pAddress != 0)
	{
		memcpy (m_Address, pAddress, sizeof m_Address);

		m_bSearch = FALSE;
	}
}

CDS18x20::~CDS18x20 (void)
{
	m_pOneWire = 0;
}

boolean CDS18x20::Initialize (void)
{
	assert (m_pOneWire != 0);

	if (m_bSearch)
	{
		if (!m_pOneWire->search (m_Address))
		{
			CLogger::Get ()->Write (FromDS18x20, LogError, "Sensor not found");

			return FALSE;
		}
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

	// read power supply
	if (!m_pOneWire->reset ())
	{
		CLogger::Get ()->Write (FromDS18x20, LogError, "Device does not respond");

		return FALSE;
	}

	m_pOneWire->select (m_Address);
	m_pOneWire->write (0xB4);

	switch (m_pOneWire->read ())
	{
	case 0x00:
		m_bParasitePower = TRUE;
		break;

	case 0xFF:
		m_bParasitePower = FALSE;
		break;

	default:
		CLogger::Get ()->Write (FromDS18x20, LogError, "Cannot read power supply");
		return FALSE;
	}

	CString HexAddress;
	for (unsigned i = 0; i < 8; i++)
	{
		CString Number;
		Number.Format ("%02X", (unsigned) m_Address[i]);
		HexAddress.Append (Number);
	}

	assert (pChip != 0);
	CLogger::Get ()->Write (FromDS18x20, LogNotice, "%s with %s found (%s)", pChip,
				m_bParasitePower ? "parasite power" : "external supply",
				(const char *) HexAddress);

	return TRUE;
}

boolean CDS18x20::DoMeasurement (void)
{
	if (!m_pOneWire->reset ())
	{
		CLogger::Get ()->Write (FromDS18x20, LogWarning, "Device does not respond");

		return FALSE;
	}

	m_pOneWire->select (m_Address);
	m_pOneWire->write (0x44, m_bParasitePower ? 1 : 0);	// start conversion
  
	CTimer::Get ()->MsDelay (750);		// for 12 bit resolution (default)

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

	int nCelsius = nRaw * 10 / 16;
	unsigned nFahrenheit = (nCelsius * 18) / 10 + 320;

	m_fCelsius = nCelsius / 10.0;
	m_fFahrenheit = nFahrenheit / 10.0;

	return TRUE;
}

float CDS18x20::GetCelsius (void) const
{
	return m_fCelsius;
}

float CDS18x20::GetFahrenheit (void) const
{
	return m_fFahrenheit;
}
