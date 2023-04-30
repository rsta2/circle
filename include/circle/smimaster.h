//
/// \file smimaster.h
//
// Driver for the Second Memory Interface
// Original development by Sebastien Nicolas <seba1978@gmx.de>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_smimaster_h
#define _circle_smimaster_h

#include <circle/dmachannel.h>
#include <circle/gpiopin.h>

#define SMI_NUM_ADDRESS_LINES		6
#define SMI_NUM_DATA_LINES			18
#define SMI_ALL_DATA_LINES_MASK		0b111111111111111111

#define GPIO_FOR_SAx(line) (5 - line) // SA0 is on GPIO0, SA1 on GPIO4, etc until SA0 on GPIO5
#define GPIO_FOR_SDx(line) (line + 8) // SD0 is on GPIO8, SD1 on GPIO9, etc until SD17 on GPIO25

enum TSMIDataWidth
{
	SMI8Bits = 0,
	SMI16Bits = 1,
	SMI18Bits = 2,
	SMI9Bits = 3
};

/// \class CSMIMaster
/// \brief Driver for the Second Memory Interface.
///
/// \details Supported features
/// - Drives any combination of SMI Data lines (GPIO8 to GPIO25)
/// - May also drive SMI Address lines (GPIO0 to GPIO5)
/// - Does not use SOE/SWE on GPIO6/GPIO7
/// - Read/Write operation in Direct mode or Write in DMA mode
///
/// \details Operations
/// One must first call SetupTiming() with suitable timing information.
/// The Device bank to use and the address to assert on the SAx lines may then optionally be set with SetDeviceAndAddress()
/// Then Direct mode may then be used with Read() / Write().
/// Or for DMA mode, one must first call SetupDMA() with a suitable internal buffer, then WriteDMA() to flush the buffer into SMI.


class CSMIMaster
{
public:
	/// \param nSDLinesMask		mask determining which SDx lines should be driven. For example (1 << 0) | (1 << 5) for SD0 (GPIO8) and SD5 (GPIO13)
	/// \param bUseAddressPins	enable use of address pins GPIO0 to GPIO5
	CSMIMaster (unsigned nSDLinesMask = SMI_ALL_DATA_LINES_MASK, boolean bUseAddressPins = TRUE);

	~CSMIMaster (void);

	/// \brief Returns nSDLinesMask given in the constructor
	unsigned GetSDLinesMask ();

	/// \brief Sets up the SMI cycle
	/// \param nWidth		the length of the data bus
	/// \param nCycle_ns	the clock period for the setup/strobe/hold cycle, in nanoseconds
	/// \param nSetup		the setup time that is used to decode the address value, in units of nCycle_ns
	/// \param nStrobe		the width of the strobe pulse that triggers the transfer, in units of nCycle_ns
	/// \param nHold		the hold time that keeps the signals stable after the transfer, in units of nCycle_ns
	/// \param nPace		the pace time in between two cycles, in units of nCycle_ns
	/// \param nDevice		the settings bank to use between 0 and 3
	void SetupTiming (TSMIDataWidth nWidth, unsigned nCycle_ns, unsigned nSetup, unsigned nStrobe, unsigned nHold, unsigned nPace, unsigned nDevice = 0);

	/// \brief Sets up DMA for (potentially multiple) SMI cycles of data from the given buffer
	/// \param pDMABuffer	the buffer (make sure it's DMA-aligned)
	/// \param nLength		length of the buffer in bytes
	void SetupDMA (void *pDMABuffer, unsigned nLength);

	/// \brief Defines the device and address to use for the next Read/Write operation
	/// \param nDevice	the settings bank to use between 0 and 3
	/// \param nAddr	the value to be asserted on the address pins
	void SetDeviceAndAddress (unsigned nDevice, unsigned nAddr);

	/// \brief Reads a single SMI cycle from the SDx lines
	unsigned Read ();

	/// \brief Issues a single SMI cycle, i.e. writes the value to the SDx lines
	/// \param nValue	the value to be written to the enabled SDx lines
	void Write (unsigned nValue);

	/// \brief Triggers the DMA transfer of a few cycles with the buffer/length specified in SetupDMA
	/// \param bWaitForCompletion	Whether to wait for DMA completion
	void WriteDMA (boolean bWaitForCompletion);

protected:
	unsigned m_nSDLinesMask;
	boolean m_bUseAddressPins;
	CDMAChannel m_txDMA;
	CGPIOPin m_dataGpios[SMI_NUM_DATA_LINES];
	CGPIOPin m_addressGpios[SMI_NUM_ADDRESS_LINES];
	void *m_pDMABuffer;
	unsigned m_nLength;
};

#endif
