//
// propertiesbasefile.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2018  R. Stange <rsta2@o2online.de>
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
#include <Properties/propertiesbasefile.h>
#include <assert.h>

enum TParseState
{
	StateWaitForName,
	StateCopyName,
	StateWaitForValue,
	StateCopyValue,
	StateIgnoreComment,
	StateUnknown
};
 
CPropertiesBaseFile::CPropertiesBaseFile (void)
:	m_nState (StateWaitForName),
	m_nIndex (0),
	m_nLine (1)
{
}

CPropertiesBaseFile::~CPropertiesBaseFile (void)
{
}

unsigned CPropertiesBaseFile::GetErrorLine (void) const
{
	return m_nLine;
}

void CPropertiesBaseFile::ResetErrorLine (void)
{
	m_nLine = 1;
}

boolean CPropertiesBaseFile::Parse (char chChar)
{
	if (chChar == '\r')
	{
		return TRUE;
	}

	switch (m_nState)
	{
	case StateWaitForName:
		switch (chChar)
		{
		case ' ':
		case '\t':
			break;

		case '\n':
			m_nLine++;
			break;

		case '#':
			m_nState = StateIgnoreComment;
			break;

		default:
			if (IsValidNameChar (chChar))
			{
				if (m_nIndex >= MAX_PROPERTY_NAME_LENGTH-2)
				{
					return FALSE;
				}

				m_PropertyName[m_nIndex++] = chChar;
				m_PropertyName[m_nIndex]   = '\0';

				m_nState = StateCopyName;
			}
			else
			{
				return FALSE;
			}
			break;
		}
		break;

	case StateCopyName:
		if (IsValidNameChar (chChar))
		{
			if (m_nIndex >= MAX_PROPERTY_NAME_LENGTH-2)
			{
				return FALSE;
			}

			m_PropertyName[m_nIndex++] = chChar;
			m_PropertyName[m_nIndex]   = '\0';
		}
		else if (chChar == '=' || chChar == ' ' || chChar == '\t')
		{
			m_nState = StateWaitForValue;
			m_nIndex = 0;
		}
		else if (chChar == '\n')
		{
			AddProperty (m_PropertyName, "");
			m_nState = StateWaitForName;
			m_nIndex = 0;
			m_nLine++;
		}
		else
		{
			return FALSE;
		}
		break;

	case StateWaitForValue:
		switch (chChar)
		{
		case ' ':
		case '\t':
		case '=':
			break;

		case '\n':
			AddProperty (m_PropertyName, "");
			m_nState = StateWaitForName;
			m_nIndex = 0;
			m_nLine++;
			break;

		default:
			if (!(' ' < chChar && chChar <= '~'))
			{
				return FALSE;
			}

			assert (m_nIndex == 0);
			m_PropertyValue[m_nIndex++] = chChar;
			m_PropertyValue[m_nIndex]   = '\0';

			m_nState = StateCopyValue;
			break;
		}
		break;

	case StateCopyValue:
		if (chChar == '\n')
		{
			// remove trailing whitespaces
			for (unsigned i = m_nIndex-1; i > 0; i--)
			{
				if (m_PropertyValue[i] != ' ' && m_PropertyValue[i] != '\t')
				{
					break;
				}

				m_PropertyValue[i] = '\0';
			}
			
			AddProperty (m_PropertyName, m_PropertyValue);
			m_nState = StateWaitForName;
			m_nIndex = 0;
			m_nLine++;
		}
		else if (!(' ' <= chChar && chChar <= '~'))
		{
			return FALSE;
		}
		else
		{
			if (m_nIndex >= MAX_PROPERTY_VALUE_LENTGH-2)
			{
				return FALSE;
			}

			m_PropertyValue[m_nIndex++] = chChar;
			m_PropertyValue[m_nIndex]   = '\0';
		}
		break;

	case StateIgnoreComment:
		if (chChar == '\n')
		{
			m_nState = StateWaitForName;
			m_nIndex = 0;
			m_nLine++;
		}
		break;

	default:
		assert (0);
		break;
	}

	return TRUE;
}

boolean CPropertiesBaseFile::IsValidNameChar (char chChar)
{
	if (   ('A' <= chChar && chChar <= 'Z')
	    || ('a' <= chChar && chChar <= 'z')
	    || ('0' <= chChar && chChar <= '9')
	    || chChar == '_' || chChar == '-' || chChar == '.')
	{
		return TRUE;
	}

	return FALSE;
}
