//
// htmlscanner.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017  R. Stange <rsta2@o2online.de>
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
#include "htmlscanner.h"
#include <circle/util.h>
#include <assert.h>

CHtmlScanner::CHtmlScanner (const char *pContent)
:	m_pContent (pContent),
	m_nIndex (0),
	m_nWhiteSpace (1),
	m_nTag (0)
{
	assert (pContent != 0);
}

CHtmlScanner::~CHtmlScanner (void)
{
	m_pContent = 0;
}

THtmlItem CHtmlScanner::GetFirstItem (CString &rItem)
{
	m_nIndex = 0;

	return GetNextItem (rItem);
}

THtmlItem CHtmlScanner::GetNextItem (CString &rItem)
{
	char *p = m_Buffer;
	if (m_nTag)
	{
		m_nTag = 0;
		*p++ = '<';
	}

	int c;
	while ((c = GetChar ()) != -1)
	{
		if (p == m_Buffer && c == ' ')
		{
			continue;
		}

		if (p - m_Buffer >= HTMLSCAN_MAXIMUM_ITEM-1)
		{
			return HtmlItemError;
		}

		assert (c != 0);
		if (!(-128 <= c && c <= 127))
		{
			c = '?';
		}

		*p++ = (char) c;
		*p = '\0';

		if (m_Buffer[0] != '<' && c == '<')
		{
			m_nTag = 1;
			*--p = '\0';
			if (strlen (m_Buffer) > 0 && *(p-1) == ' ')
			{
				*--p = '\0';
			}
			rItem = m_Buffer;
			return HtmlItemText;
		}

		if (   !strncmp (m_Buffer, "<![CDATA[", 9)
		    && (p - m_Buffer) >= 12
		    && !strncmp (p - 3, "]]>", 3))
		{
			p[-3] = '\0';
			rItem = m_Buffer + 9;
			return HtmlItemCData;
		}

		if (   m_Buffer[0] == '<'
		    && strncmp (m_Buffer, "<![CDATA[", 9)
		    && strncmp (m_Buffer, "<!--", 4)
		    && c == '>')
		{
			rItem = m_Buffer;
			return HtmlItemTag;
		}

		if (   !strncmp (m_Buffer, "<!--", 4)
		    && (p - m_Buffer) >= 8
		    && !strncmp (p - 3, "-->", 3))
		{
			rItem = m_Buffer;
			return HtmlItemComment;
		}
	}

	return HtmlItemEOF;
}

THtmlItem CHtmlScanner::GetItem (int nItem, CString &rItem)
{
	assert (nItem >= 0);

	THtmlItem HtmlItem = GetFirstItem (rItem);
	if (HtmlItem == HtmlItemEOF)
	{
		return HtmlItem;
	}

	do
	{
		if (nItem-- == 0)
		{
			return HtmlItem;
		}
	}
	while ((HtmlItem = GetNextItem (rItem)) != HtmlItemEOF);

	return HtmlItem;
}

int CHtmlScanner::GetChar (void)
{
	assert (m_pContent != 0);

	int c;
	while ((c = m_pContent[m_nIndex]) != '\0')
	{
		m_nIndex++;

		switch (c)
		{
		case '\r':
			continue;

		case '\n':
		case '\t':
		case ' ':
			if (m_nWhiteSpace)
			{
				continue;
			}
			m_nWhiteSpace = 1;
			return ' ';

		case '\x1A':
			return -1;

		default:
			m_nWhiteSpace = 0;
			return c;
		}
	}

	return -1;
}
