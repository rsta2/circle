//
// ds18x20.h
//
#ifndef _OneWire_ds18x20_h
#define _OneWire_ds18x20_h

#include <OneWire/OneWire.h>
#include <circle/types.h>

/// \note The driver automatically determines the power mode (parasite / external supply).

class CDS18x20		/// Driver for DS18B20, DS18S20 and DS1822 temperature sensor
{
public:
	/// \param pOneWire Pointer to OneWire bus object
	/// \param pAddress Pointer to 8-byte address array (or 0 to search for first device)
	CDS18x20 (OneWire *pOneWire, const u8 *pAddress = 0);

	~CDS18x20 (void);

	boolean Initialize (void);

	boolean DoMeasurement (void);

	float GetCelsius (void) const;
	float GetFahrenheit (void) const;

private:
	OneWire *m_pOneWire;
	boolean  m_bSearch;

	u8	 m_Address[8];
	boolean  m_bIs18S20;
	boolean  m_bParasitePower;

	float	 m_fCelsius;
	float	 m_fFahrenheit;
};

#endif
