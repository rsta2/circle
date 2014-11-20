//
// usbconfigparser.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _usbconfigparser_h
#define _usbconfigparser_h

#include <circle/usb/usb.h>
#include <circle/types.h>

class CUSBConfigurationParser
{
public:
	CUSBConfigurationParser (const void *pBuffer, unsigned nBufLen);
	CUSBConfigurationParser (CUSBConfigurationParser *pParser);		// copy constructor
	~CUSBConfigurationParser (void);

	boolean IsValid (void) const;

	const TUSBDescriptor *GetDescriptor (u8 ucType);	// returns 0 if not found
	const TUSBDescriptor *GetCurrentDescriptor (void);

	void Error (const char *pSource) const;

private:
	const TUSBDescriptor	*m_pBuffer;
	unsigned		 m_nBufLen;
	boolean			 m_bValid;
	const TUSBDescriptor	*m_pEndPosition;
	const TUSBDescriptor	*m_pNextPosition;
	const TUSBDescriptor	*m_pCurrentDescriptor;
	const TUSBDescriptor	*m_pErrorPosition;
};

#endif
