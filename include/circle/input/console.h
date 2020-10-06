//
// console.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_input_console_h
#define _circle_input_console_h

#include <circle/device.h>
#include <circle/input/keyboardbuffer.h>
#include <circle/input/linediscipline.h>
#include <circle/types.h>

// console options
#define CONSOLE_OPTION_ICANON	(1 << 0)	// canonic input using line editor (default)
#define CONSOLE_OPTION_ECHO	(1 << 1)	// echo input to output (default)

class CConsole : public CDevice		/// Console using screen/USB keyboard or alternate device(s)
{
public:
	/// \param pAlternateDevice Alternate device to be used (if USB keyboard is not attached)
	/// \param bPlugAndPlay Enable USB plug-and-play?
	/// \note This constructor is mandatory for USB plug-and-play operation.
	CConsole (CDevice *pAlternateDevice = 0, boolean bPlugAndPlay = FALSE);
	/// \param pInputDevice Device used for input (instead of USB keyboard)
	/// \param pOutputDevice Device used for output (instead of screen)
	CConsole (CDevice *pInputDevice, CDevice *pOutputDevice);

	~CConsole (void);

	/// \return Operation successful?
	boolean Initialize (void);

	/// \brief Update USB plug-and-play configuration
	/// \note Must call this continuously for USB-plug-and-play operation only!
	void UpdatePlugAndPlay (void);

	/// \return Is alternate device used instead of screen/USB keyboard?
	boolean IsAlternateDeviceUsed (void) const;

	/// \param pBuffer Pointer to buffer for read data
	/// \param nCount Maximum number of bytes to be read
	/// \return Number of bytes read (0 no data available, < 0 on error)
	/// \note This method does not block! It has to be called until != 0 is returned.
	int Read (void *pBuffer, size_t nCount);

	/// \param pBuffer Pointer to data to be written
	/// \param nCount Number of bytes to be written
	/// \return Number of bytes successfully written (< 0 on error)
	int Write (const void *pBuffer, size_t nCount);

	/// \return Console options mask (see console options)
	unsigned GetOptions (void) const;
	/// \param nOptions Console options mask (see console options)
	void SetOptions (unsigned nOptions);

private:
	static void KeyboardRemovedHandler (CDevice *pDevice, void *pContext);

private:
	boolean  m_bPlugAndPlay;
	CDevice *m_pAlternateDevice;
	CDevice *m_pInputDevice;
	CDevice *m_pOutputDevice;
	boolean  m_bAlternateDeviceUsed;

	CKeyboardBuffer *m_pKeyboardBuffer;
	CLineDiscipline *m_pLineDiscipline;

	unsigned m_nOptions;
};

#endif
