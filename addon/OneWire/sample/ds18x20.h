//
// ds18x20.h
//
#ifndef _ds18x20_h
#define _ds18x20_h

#include <OneWire/OneWire.h>
#include <circle/types.h>

class CDS18x20
{
public:
	CDS18x20 (OneWire *pOneWire);
	~CDS18x20 (void);

	boolean Initialize (void);

	boolean StartConversion (void);

	int GetCelsius (void) const;
	int GetCelsiusFract (void) const;		// 0..9

	int GetFahrenheit (void) const;
	int GetFahrenheitFract (void) const;		// 0..9

private:
	OneWire *m_pOneWire;

	u8	 m_Address[8];
	boolean  m_bIs18S20;

	int	 m_nCelsius;
	int	 m_nCelsiusFract;

	unsigned m_nFahrenheit;
	unsigned m_nFahrenheitFract;
};

#endif
