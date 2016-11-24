//
// koptions.cpp
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
#include <circle/koptions.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <circle/sysconfig.h>

#define INVALID_VALUE	((unsigned) -1)

CKernelOptions *CKernelOptions::s_pThis = 0;

CKernelOptions::CKernelOptions (void)
:	m_nWidth (0),
	m_nHeight (0),
	m_nLogLevel (LogDebug),
	m_nUSBPowerDelay (0),
	m_CPUSpeed (CPUSpeedLow),
	m_nSoCMaxTemp (60)
{
	strcpy (m_LogDevice, "tty1");
	strcpy (m_KeyMap, DEFAULT_KEYMAP);

	s_pThis = this;

	CBcmPropertyTags Tags;
	if (!Tags.GetTag (PROPTAG_GET_COMMAND_LINE, &m_TagCommandLine, sizeof m_TagCommandLine))
	{
		return;
	}

	if (m_TagCommandLine.Tag.nValueLength >= sizeof m_TagCommandLine.String)
	{
		return;
	}
	m_TagCommandLine.String[m_TagCommandLine.Tag.nValueLength] = '\0';
	
	m_pOptions = (char *) m_TagCommandLine.String;

	char *pOption;
	while ((pOption = GetToken ()) != 0)
	{
		char *pValue = GetOptionValue (pOption);

		if (strcmp (pOption, "width") == 0)
		{
			unsigned nValue;
			if (   (nValue = GetDecimal (pValue)) != INVALID_VALUE
			    && 640 <= nValue && nValue <= 1980)
			{
				m_nWidth = nValue;
			}
		}
		else if (strcmp (pOption, "height") == 0)
		{
			unsigned nValue;
			if (   (nValue = GetDecimal (pValue)) != INVALID_VALUE
			    && 480 <= nValue && nValue <= 1080)
			{
				m_nHeight = nValue;
			}
		}
		else if (strcmp (pOption, "logdev") == 0)
		{
			strncpy (m_LogDevice, pValue, sizeof m_LogDevice-1);
			m_LogDevice[sizeof m_LogDevice-1] = '\0';
		}
		else if (strcmp (pOption, "loglevel") == 0)
		{
			unsigned nValue;
			if (   (nValue = GetDecimal (pValue)) != INVALID_VALUE
			    && nValue <= LogDebug)
			{
				m_nLogLevel = nValue;
			}
		}
		else if (strcmp (pOption, "keymap") == 0)
		{
			strncpy (m_KeyMap, pValue, sizeof m_KeyMap-1);
			m_KeyMap[sizeof m_KeyMap-1] = '\0';
		}
		else if (strcmp (pOption, "usbpowerdelay") == 0)
		{
			unsigned nValue;
			if (   (nValue = GetDecimal (pValue)) != INVALID_VALUE
			    && 200 <= nValue && nValue <= 8000)
			{
				m_nUSBPowerDelay = nValue;
			}
		}
		else if (strcmp (pOption, "fast") == 0)
		{
			if (strcmp (pValue, "true") == 0)
			{
				m_CPUSpeed = CPUSpeedMaximum;
			}
		}
		else if (strcmp (pOption, "socmaxtemp") == 0)
		{
			unsigned nValue;
			if (   (nValue = GetDecimal (pValue)) != INVALID_VALUE
			    && 40 <= nValue && nValue <= 78)
			{
				m_nSoCMaxTemp = nValue;
			}
		}
	}
}

CKernelOptions::~CKernelOptions (void)
{
	s_pThis = 0;
}

unsigned CKernelOptions::GetWidth (void) const
{
	return m_nWidth;
}

unsigned CKernelOptions::GetHeight (void) const
{
	return m_nHeight;
}

const char *CKernelOptions::GetLogDevice (void) const
{
	return m_LogDevice;
}

unsigned CKernelOptions::GetLogLevel (void) const
{
	return m_nLogLevel;
}

const char *CKernelOptions::GetKeyMap (void) const
{
	return m_KeyMap;
}

unsigned CKernelOptions::GetUSBPowerDelay (void) const
{
	return m_nUSBPowerDelay;
}

TCPUSpeed CKernelOptions::GetCPUSpeed (void) const
{
	return m_CPUSpeed;
}

unsigned CKernelOptions::GetSoCMaxTemp (void) const
{
	return m_nSoCMaxTemp;
}

CKernelOptions *CKernelOptions::Get (void)
{
	return s_pThis;
}

char *CKernelOptions::GetToken (void)
{
	while (*m_pOptions != '\0')
	{
		if (*m_pOptions != ' ')
		{
			break;
		}

		m_pOptions++;
	}

	if (*m_pOptions == '\0')
	{
		return 0;
	}

	char *pToken = m_pOptions;

	while (*m_pOptions != '\0')
	{
		if (*m_pOptions == ' ')
		{
			*m_pOptions++ = '\0';

			break;
		}

		m_pOptions++;
	}

	return pToken;
}

char *CKernelOptions::GetOptionValue (char *pOption)
{
	while (*pOption != '\0')
	{
		if (*pOption == '=')
		{
			break;
		}

		pOption++;
	}

	if (*pOption == '\0')
	{
		return 0;
	}

	*pOption++ = '\0';

	return pOption;
}

unsigned CKernelOptions::GetDecimal (char *pString)
{
	if (   pString == 0
	    || *pString == '\0')
	{
		return INVALID_VALUE;
	}

	unsigned nResult = 0;

	char chChar;
	while ((chChar = *pString++) != '\0')
	{
		if (!('0' <= chChar && chChar <= '9'))
		{
			return INVALID_VALUE;
		}

		unsigned nPrevResult = nResult;

		nResult = nResult * 10 + (chChar - '0');
		if (   nResult < nPrevResult
		    || nResult == INVALID_VALUE)
		{
			return INVALID_VALUE;
		}
	}

	return nResult;
}
