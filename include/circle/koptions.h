//
// koptions.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_koptions_h
#define _circle_koptions_h

#include <circle/bcmpropertytags.h>
#include <circle/cputhrottle.h>

class CKernelOptions
{
public:
	CKernelOptions (void);
	~CKernelOptions (void);

	unsigned GetWidth (void) const;
	unsigned GetHeight (void) const;

	const char *GetLogDevice (void) const;
	unsigned GetLogLevel (void) const;

	const char *GetKeyMap (void) const;

	unsigned GetUSBPowerDelay (void) const;

	TCPUSpeed GetCPUSpeed (void) const;
	unsigned GetSoCMaxTemp (void) const;

	static CKernelOptions *Get (void);

private:
	char *GetToken (void);				// returns next "option=value" pair, 0 if nothing follows

	static char *GetOptionValue (char *pOption);	// returns value and terminates option with '\0'

	static unsigned GetDecimal (char *pString);	// returns decimal value, -1 on error

private:
	TPropertyTagCommandLine m_TagCommandLine;
	char *m_pOptions;

	unsigned m_nWidth;
	unsigned m_nHeight;

	char m_LogDevice[20];
	unsigned m_nLogLevel;

	char m_KeyMap[3];

	unsigned m_nUSBPowerDelay;

	TCPUSpeed m_CPUSpeed;
	unsigned m_nSoCMaxTemp;

	static CKernelOptions *s_pThis;
};

#endif
