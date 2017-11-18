//
// time.h
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
#ifndef _circle_time_h
#define _circle_time_h

#include <circle/string.h>
#include <circle/types.h>

typedef signed long time_t;

class CTime			/// Holds, makes and breaks the time
{
public:
	CTime (void);
	CTime (const CTime &rSource);
	~CTime (void);

	/// \param Time		seconds since Jan 01 1970
	void Set (time_t Time);

	/// \param nHours	0-23
	/// \param nMinutes	0-59
	/// \param nSeconds	0-59
	/// \return Time valid?
	boolean SetTime (unsigned nHours, unsigned nMinutes, unsigned nSeconds);

	/// \param nMonthDay	1-31
	/// \param nMonth	1-12
	/// \param nYear	1970-
	/// \return Date valid?
	boolean SetDate (unsigned nMonthDay, unsigned nMonth, unsigned nYear);

	/// \return		seconds since Jan 01 1970
	time_t Get (void) const;

	/// \return 0-59
	unsigned GetSeconds (void) const;
	/// \return 0-59
	unsigned GetMinutes (void) const;
	/// \return 0-23
	unsigned GetHours (void) const;
	/// \return 1-31
	unsigned GetMonthDay (void) const;
	/// \return 1-12
	unsigned GetMonth (void) const;
	/// \return 1970-
	unsigned GetYear (void) const;
	/// \return 0-6 (Sun-Sat)
	unsigned GetWeekDay (void) const;

	/// \return "WWW MMM DD HH:MM:SS YYYY"
	const char *GetString (void);

private:
	static boolean IsLeapYear (unsigned nYear);
	static unsigned GetDaysOfMonth (unsigned nMonth, unsigned nYear);

private:
	// see mktime(3)
	unsigned m_nSeconds;	// 0-59
	unsigned m_nMinutes;	// 0-59
	unsigned m_nHours;	// 0-23
	unsigned m_nMonthDay;	// 1-31
	unsigned m_nMonth;	// 0-11
	unsigned m_nYear;	// year-1900
	unsigned m_nWeekDay;	// 0-6 (Sun-Sat)

	CString m_String;

	static const unsigned s_nDaysOfMonth[];
	static const char *s_pMonthName[];
	static const char *s_pDaysOfWeek[];
};

#endif
