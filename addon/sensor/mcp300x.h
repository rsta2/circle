//
// mcp300x.h
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
#ifndef _sensor_mcp300x_h
#define _sensor_mcp300x_h

#include <circle/spimaster.h>
#include <circle/spimasteraux.h>
#include <circle/types.h>

class CMCP300X		/// Driver for MCP3004/3008 DAC with SPI interface
{
public:
	static const unsigned Channels = 8;
	static const int MaxResultRaw = 1023;

	static constexpr float ResultSPIError = 1000.0f;	///< returned for SPI error

public:
	/// \param pSPIMaster Pointer to SPI master object to be used
	/// \param fVREF Reference voltage (e.g. 3.3)
	/// \param nChipSelect /CS line (0 or 1)
	/// \param nClockSpeed SPI clock speed (Hz)
	CMCP300X (CSPIMaster *pSPIMaster, float fVREF,
		  unsigned nChipSelect = 0, unsigned nClockSpeed = 1000000);

	/// \param pSPIMasterAUX Pointer to SPI AUX master object to be used
	/// \param fVREF Reference voltage (e.g. 3.3)
	/// \param nChipSelect /CS line (0, 1 or 2)
	/// \param nClockSpeed SPI clock speed (Hz)
	CMCP300X (CSPIMasterAUX *pSPIMasterAUX, float fVREF,
		  unsigned nChipSelect = 0, unsigned nClockSpeed = 1000000);

	~CMCP300X (void);

	/// \param nChannel Channel to be used (0..7, 0..3 for MCP3004)
	/// \return Measured voltage, or ResultSPIError on error
	float DoSingleEndedConversion (unsigned nChannel);

	/// \param nChannelPlus IN+ channel (0..7, 0..3 for MCP3004)
	/// \param nChannelMinus IN- channel (0..7, 0..3 for MCP3004)
	/// \return Measured differential voltage, or ResultSPIError on error
	/// \note See the MCP3004/3008 data sheet for supported channel combinations!
	float DoDifferentialConversion (unsigned nChannelPlus, unsigned nChannelMinus);

	/// \param nChannel Channel to be used (0..7, 0..3 for MCP3004)
	/// \return Measured raw value, or < 0 on error
	int DoSingleEndedConversionRaw (unsigned nChannel);

	/// \param nChannelPlus IN+ channel (0..7, 0..3 for MCP3004)
	/// \param nChannelMinus IN- channel (0..7, 0..3 for MCP3004)
	/// \return Measured differential raw value, or < 0 on error
	/// \note See the MCP3004/3008 data sheet for supported channel combinations!
	int DoDifferentialConversionRaw (unsigned nChannelPlus, unsigned nChannelMinus);

private:
	int DoConversion (u8 nControl);

private:
	CSPIMaster *m_pSPIMaster;
	CSPIMasterAUX *m_pSPIMasterAUX;
	float m_fVREF;
	unsigned m_nChipSelect;
	unsigned m_nClockSpeed;

	static u8 s_DifferentialMap[Channels][Channels];
};

#endif
