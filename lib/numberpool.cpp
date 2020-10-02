//
// numberpool.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#include <circle/numberpool.h>
#include <circle/logger.h>
#include <assert.h>

CNumberPool::CNumberPool (unsigned nMin, unsigned nMax)
:	m_nMin (nMin),
	m_nMax (nMax),
	m_nMap (0)
{
	assert (nMax <= Limit);
	assert (nMin <= nMax);
}

CNumberPool::~CNumberPool (void)
{
}

unsigned CNumberPool::AllocateNumber (boolean bMustSucceed, const char *pFrom)
{
	unsigned i;
	for (i = m_nMin; i <= m_nMax; i++)
	{
		if (!(m_nMap & (1 << i)))
		{
			break;
		}
	}

	if (i > m_nMax)
	{
		if (bMustSucceed)
		{
			assert (pFrom != 0);
			CLogger::Get ()->Write (pFrom, LogPanic, "Number pool exhausted");
		}

		return Invalid;
	}

	m_nMap |= 1 << i;

	return i;
}

void CNumberPool::FreeNumber (unsigned nNumber)
{
	assert (m_nMin <= nNumber && nNumber <= m_nMax);
	assert (m_nMap & (1 << nNumber));

	m_nMap &= ~(1 << nNumber);
}
