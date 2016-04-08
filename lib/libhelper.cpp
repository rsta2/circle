//
// libhelper.cpp
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
#include <assert.h>
#include <circle/exceptionhandler.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned __Divide (unsigned nDividend, unsigned nDivisor, unsigned *pRest)
{
	if (nDivisor == 0)
	{
		assert (0);
		CExceptionHandler::Get ()->Throw (EXCEPTION_DIVISION_BY_ZERO);
	}
	
	unsigned long long ullDivisor = nDivisor;

	unsigned nCount = 1;
	while (nDividend > ullDivisor)
	{
		ullDivisor <<= 1;
		nCount++;
	}

	unsigned nQuotient = 0;
	while (nCount--)
	{
		nQuotient <<= 1;

		if (nDividend >= ullDivisor)
		{
			nQuotient |= 1;
			nDividend -= ullDivisor;
		}

		ullDivisor >>= 1;
	}
	
	if (pRest != 0)
	{
		*pRest = nDividend;
	}
	
	return nQuotient;
}

int __DivideInteger (int nDividend, int nDivisor)
{
	if (nDividend < 0)
	{
		if (nDivisor < 0)
		{
			return __Divide (-nDividend, -nDivisor, 0);
		}
		else
		{
			return -__Divide (-nDividend, nDivisor, 0);
		}
	}
	else
	{
		if (nDivisor < 0)
		{
			return -__Divide (nDividend, -nDivisor, 0);
		}
		else
		{
			return __Divide (nDividend, nDivisor, 0);
		}
	}
}

#ifdef __cplusplus
}
#endif
