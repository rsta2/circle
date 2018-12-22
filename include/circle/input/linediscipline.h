//
// linediscipline.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2018  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_input_linediscipline_h
#define _circle_input_linediscipline_h

#include <circle/device.h>
#include <circle/types.h>

#define LD_MAXLINE	(160+1)

enum TLineMode
{
	LineModeInput,
	LineModeOutput,
	LineModeRaw,
	LineModeUnknown
};

class CLineDiscipline : public CDevice
{
public:
	CLineDiscipline (CDevice *pInputDevice, CDevice *pOutputDevice);
	~CLineDiscipline (void);

	int Read (void *pBuffer, size_t nCount);

	void SetOptionRawMode (boolean bEnable);
	void SetOptionEcho (boolean bEnable);

private:
	int GetChar (void);
	void PutChar (int nChar);

private:
	CDevice *m_pInputDevice;
	CDevice *m_pOutputDevice;

	TLineMode m_Mode;
	boolean m_bEcho;

	char  m_Buffer[LD_MAXLINE];
	char *m_pInPtr;
	char *m_pOutPtr;
};

#endif
