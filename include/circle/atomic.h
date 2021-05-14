//
/// \file atomic.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_atomic_h
#define _circle_atomic_h

#include <circle/sysconfig.h>
#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARM_ALLOW_MULTI_CORE
	#define __CIRCLE_ATOMIC_MEMMODEL	__ATOMIC_SEQ_CST
#else
	#define __CIRCLE_ATOMIC_MEMMODEL	__ATOMIC_RELAXED
#endif

static inline int AtomicGet (const volatile int *pVar)
{
	return __atomic_load_n (pVar, __CIRCLE_ATOMIC_MEMMODEL);
}

static inline int AtomicSet (volatile int *pVar, int nValue)
{
	__atomic_store_n (pVar, nValue, __CIRCLE_ATOMIC_MEMMODEL);

	return nValue;
}

/// \return Previous value
static inline int AtomicExchange (volatile int *pVar, int nValue)
{
	return __atomic_exchange_n (pVar, nValue, __CIRCLE_ATOMIC_MEMMODEL);
}

/// \brief Set value "nValue", if previous value is "nCompare"
/// \return previous value
static inline int AtomicCompareExchange (volatile int *pVar, int nCompare, int nValue)
{
	__atomic_compare_exchange_n (pVar, &nCompare, nValue, false,
				     __CIRCLE_ATOMIC_MEMMODEL, __CIRCLE_ATOMIC_MEMMODEL);

	return nCompare;
}

static inline int AtomicAdd (volatile int *pVar, int nValue)
{
	return __atomic_add_fetch (pVar, nValue, __CIRCLE_ATOMIC_MEMMODEL);
}

static inline int AtomicSub (volatile int *pVar, int nValue)
{
	return __atomic_sub_fetch (pVar, nValue, __CIRCLE_ATOMIC_MEMMODEL);
}

static inline int AtomicIncrement (volatile int *pVar)
{
	return AtomicAdd (pVar, 1);
}

static inline int AtomicDecrement (volatile int *pVar)
{
	return AtomicSub (pVar, 1);
}

#ifdef __cplusplus
}
#endif

#endif
