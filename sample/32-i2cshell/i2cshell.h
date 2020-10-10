//
// i2cshell.h
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
#ifndef _i2cshell_h
#define _i2cshell_h

#include <circle/input/console.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/i2cmaster.h>
#include <circle/string.h>
#include <circle/types.h>

#define I2CSHELL_MAX_LINE	500

class CI2CShell
{
public:
	CI2CShell (CConsole *pConsole, CUSBHCIDevice *pUSBHCI = 0);
	~CI2CShell (void);

	void Run (void);

private:
	boolean Master (void);
	boolean Slave (void);
	boolean Clock (void);
	boolean Detect (void);
	boolean Read (void);
	boolean Write (void);
	boolean Delay (void);
	boolean Help (void);

	unsigned GetNumber (const char *pName, unsigned nMinimum, unsigned nMaximum,
			    boolean bOptional = FALSE);

	void PrintI2CError (int nStatus);

private:
	void ReadLine (void);
	boolean GetToken (CString *pString);
	void UnGetToken (const CString &rString);

	static unsigned ConvertNumber (const CString &rString);
#define INVALID_NUMBER	((unsigned) -1)

	void Print (const char *pFormat, ...);

private:
	CConsole *m_pConsole;
	CUSBHCIDevice *m_pUSBHCI;

	boolean m_bContinue;

	CI2CMaster *m_pI2CMaster;
	unsigned m_nMasterDevice;
	unsigned m_nMasterConfig;

	unsigned m_nI2CClockHz;
	u8 m_ucSlaveAddress;
#define INVALID_SLAVE	0

	char m_LineBuffer[I2CSHELL_MAX_LINE+1];
	boolean m_bFirstToken;
	CString m_UnGetToken;
	char *m_pSavePtr;

	static const char HelpMsg[];
	static const char GPIOHelpMsg[];
};

#endif
