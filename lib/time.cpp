//
// time.cpp
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
#include <circle/time.h>
#include <assert.h>

const unsigned CTime::s_nDaysOfMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

const char *CTime::s_pMonthName[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
				       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

const char *CTime::s_pDaysOfWeek[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

CTime::CTime (void)
:	m_nSeconds (0),
	m_nMinutes (0),
	m_nHours (0),
	m_nMonthDay (1),
	m_nMonth (0),
	m_nYear (70),
	m_nWeekDay (4)
{
}

CTime::CTime (const CTime &rSource)
:	m_nSeconds (rSource.m_nSeconds),
	m_nMinutes (rSource.m_nMinutes),
	m_nHours (rSource.m_nHours),
	m_nMonthDay (rSource.m_nMonthDay),
	m_nMonth (rSource.m_nMonth),
	m_nYear (rSource.m_nYear),
	m_nWeekDay (rSource.m_nWeekDay)
{
}

CTime::~CTime (void)
{
}

void CTime::Set (time_t Time)
{
	m_nSeconds = Time % 60;
	Time /= 60;
	m_nMinutes = Time % 60;
	Time /= 60;
	m_nHours = Time % 24;
	Time /= 24;

	m_nWeekDay = (Time + 4) % 7;		// Jan 01 1970 was Thursday (4)

	unsigned nYear = 1970;
	while (1)
	{
		int nDaysOfYear = IsLeapYear (nYear) ? 366 : 365;
		if (Time < nDaysOfYear)
		{
			break;
		}

		Time -= nDaysOfYear;
		nYear++;
	}

	m_nYear = nYear-1900;

	unsigned nMonth = 0;
	while (1)
	{
		int nDaysOfMonth = GetDaysOfMonth (nMonth, nYear);
		if (Time < nDaysOfMonth)
		{
			break;
		}

		Time -= nDaysOfMonth;
		nMonth++;
	}

	m_nMonth = nMonth;
	m_nMonthDay = Time + 1;
}

boolean CTime::SetTime (unsigned nHours, unsigned nMinutes, unsigned nSeconds)
{
	if (   nSeconds > 59
	    || nMinutes > 59
	    || nHours   > 23)
	{
		return FALSE;
	}

	m_nSeconds = nSeconds;
	m_nMinutes = nMinutes;
	m_nHours   = nHours;

	return TRUE;
}

boolean CTime::SetDate (unsigned nMonthDay, unsigned nMonth, unsigned nYear)
{
	if (   nYear < 1970
	    || !(1 <= nMonth && nMonth <= 12)
	    || !(1 <= nMonthDay && nMonthDay <= GetDaysOfMonth (nMonth-1, nYear)))
	{
		return FALSE;
	}

	m_nMonthDay = nMonthDay;
	m_nMonth    = nMonth-1;
	m_nYear     = nYear-1900;

	// calculate day of the week
	unsigned nTotalDays = nMonthDay-1;

	while (--nMonth > 0)
	{
		nTotalDays += GetDaysOfMonth (nMonth-1, nYear);
	}

	while (--nYear >= 1970)
	{
		nTotalDays += IsLeapYear (nYear) ? 366 : 365;
	}

	m_nWeekDay = (nTotalDays + 4) % 7;	// Jan 01 1970 was Thursday (4)

	return TRUE;
}

time_t CTime::Get (void) const
{
	time_t Result = 0;

	for (unsigned nYear = 1970; nYear < m_nYear+1900; nYear++)
	{
		Result += IsLeapYear (nYear) ? 366 : 365;
	}

	for (unsigned nMonth = 0; nMonth < m_nMonth; nMonth++)
	{
		Result += GetDaysOfMonth (nMonth, m_nYear+1900);
	}

	Result += m_nMonthDay-1;
	Result *= 24;
	Result += m_nHours;
	Result *= 60;
	Result += m_nMinutes;
	Result *= 60;
	Result += m_nSeconds;

	return Result;
}

unsigned CTime::GetSeconds (void) const
{
	return m_nSeconds;
}

unsigned CTime::GetMinutes (void) const
{
	return m_nMinutes;
}

unsigned CTime::GetHours (void) const
{
	return m_nHours;
}

unsigned CTime::GetMonthDay (void) const
{
	return m_nMonthDay;
}

unsigned CTime::GetMonth (void) const
{
	return m_nMonth+1;
}

unsigned CTime::GetYear (void) const
{
	return m_nYear+1900;
}

unsigned CTime::GetWeekDay (void) const
{
	return m_nWeekDay;
}

const char *CTime::GetString (void)
{
	assert (m_nWeekDay <= 6);
	assert (m_nMonth <= 11);

	m_String.Format ("%s %s %2u %02u:%02u:%02u %04u",
			 s_pDaysOfWeek[m_nWeekDay], s_pMonthName[m_nMonth], m_nMonthDay,
			 m_nHours, m_nMinutes, m_nSeconds,
			 m_nYear+1900);

	return m_String;
}

boolean CTime::IsLeapYear (unsigned nYear)
{
	if (nYear % 100 == 0)
	{
		return nYear % 400 == 0;
	}

	return nYear % 4 == 0;
}

unsigned CTime::GetDaysOfMonth (unsigned nMonth, unsigned nYear)
{
	if (   nMonth == 1
	    && IsLeapYear (nYear))
	{
		return 29;
	}

	assert (nMonth <= 11);
	return s_nDaysOfMonth[nMonth];
}
