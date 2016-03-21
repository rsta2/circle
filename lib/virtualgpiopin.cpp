//
// virtualgpiopin.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
// 
// This file contains code taken from Linux:
//	from file drivers/gpio/gpio-bcm-virt.c
//	Copyright (C) 2012,2013 Dom Cobley <popcornmix@gmail.com>
//	Based on gpio-clps711x.c by Alexander Shiyan <shc_work@mail.ru>
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
#include <circle/virtualgpiopin.h>
#include <circle/bcmpropertytags.h>
#include <circle/memio.h>

u32 CVirtualGPIOPin::s_nGPIOBaseAddress = 0;

CSpinLock CVirtualGPIOPin::s_SpinLock (FALSE);

CVirtualGPIOPin::CVirtualGPIOPin (unsigned nPin)
:	m_nPin (nPin),
	m_nEnableCount (0),
	m_nDisableCount (0)
{
	if (m_nPin >= VIRTUAL_GPIO_PINS)
	{
		return;
	}

	s_SpinLock.Acquire ();

	if (s_nGPIOBaseAddress == 0)
	{
		CBcmPropertyTags Tags;
		TPropertyTagSimple TagSimple;
		if (Tags.GetTag (PROPTAG_GET_GPIO_VIRTBUF, &TagSimple, sizeof TagSimple))
		{
			s_nGPIOBaseAddress = TagSimple.nValue & ~0xC0000000;
		}
	}

	s_SpinLock.Release ();
	
	Write (LOW);
}

CVirtualGPIOPin::~CVirtualGPIOPin (void)
{
}

void CVirtualGPIOPin::Write (unsigned nValue)
{
	if (   m_nPin >= VIRTUAL_GPIO_PINS
	    || s_nGPIOBaseAddress == 0
	    || (nValue != LOW && nValue != HIGH))
	{
		return;
	}

	s_SpinLock.Acquire ();

	m_nValue = nValue;

	if ((signed short) (m_nEnableCount - m_nDisableCount) > 0)
	{
		if (m_nValue)
		{
			s_SpinLock.Release ();

			return;
		}
	}
	else
	{
		if (!m_nValue)
		{
			s_SpinLock.Release ();

			return;
		}
	}

	if (m_nValue)
	{
		m_nEnableCount++;
	}
	else
	{
		m_nDisableCount++;
	}

	write32 (s_nGPIOBaseAddress + m_nPin * 4, ((u32) m_nEnableCount << 16) | m_nDisableCount);

	s_SpinLock.Release ();
}

void CVirtualGPIOPin::Invert (void)
{
	Write (m_nValue ^ 1);
}
