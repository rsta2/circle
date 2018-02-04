//
// htmlscanner.h
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
#ifndef _htmlscanner_h
#define _htmlscanner_h

#include <circle/string.h>

#define HTMLSCAN_MAXIMUM_ITEM	0x2000

enum THtmlItem
{
	HtmlItemTag,
	HtmlItemText,
	HtmlItemCData,
	HtmlItemComment,
	HtmlItemEOF,
	HtmlItemError,
	HtmlItemUnknown
};

class CHtmlScanner
{
public:
	CHtmlScanner (const char *pContent);
	~CHtmlScanner (void);

	THtmlItem GetFirstItem (CString &rItem);
	THtmlItem GetNextItem (CString &rItem);

	THtmlItem GetItem (int nItem, CString &rItem);

private:
	int GetChar (void);

private:
	const char *m_pContent;
	int m_nIndex;

	char m_Buffer[HTMLSCAN_MAXIMUM_ITEM];

	int m_nWhiteSpace;
	int m_nTag;
};

#endif
