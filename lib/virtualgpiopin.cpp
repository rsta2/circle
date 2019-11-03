//
// virtualgpiopin.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2019  R. Stange <rsta2@o2online.de>
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
#include <circle/memory.h>
#include <circle/bcm2835.h>

uintptr CVirtualGPIOPin::s_nGPIOBaseAddress = 0;

CSpinLock CVirtualGPIOPin::s_SpinLock (TASK_LEVEL);

CVirtualGPIOPin::CVirtualGPIOPin (unsigned nPin, boolean bSafeMode)
:	m_bSafeMode (bSafeMode),
	m_nPin (nPin),
	m_nEnableCount (0),
	m_nDisableCount (0)
{
	if (m_bSafeMode)
	{
		return;
	}

	if (m_nPin >= VIRTUAL_GPIO_PINS)
	{
		return;
	}

	s_SpinLock.Acquire ();

	if (s_nGPIOBaseAddress == 0)
	{
		s_nGPIOBaseAddress = CMemorySystem::GetCoherentPage (COHERENT_SLOT_GPIO_VIRTBUF);

		CBcmPropertyTags Tags;
		TPropertyTagSimple TagSimple;
		TagSimple.nValue = BUS_ADDRESS (s_nGPIOBaseAddress);
		if (!Tags.GetTag (PROPTAG_SET_GPIO_VIRTBUF, &TagSimple, sizeof TagSimple, 4))
		{
			if (Tags.GetTag (PROPTAG_GET_GPIO_VIRTBUF, &TagSimple, sizeof TagSimple))
			{
				s_nGPIOBaseAddress = TagSimple.nValue & ~0xC0000000;
			}
			else
			{
				s_nGPIOBaseAddress = 0;
			}
		}
	}

	if (s_nGPIOBaseAddress != 0)
	{
		write32 (s_nGPIOBaseAddress + m_nPin * 4, 0);
	}

	s_SpinLock.Release ();
	
	Write (LOW);
}

CVirtualGPIOPin::~CVirtualGPIOPin (void)
{
}

void CVirtualGPIOPin::Write (unsigned nValue)
{
	if (m_bSafeMode)
	{
		assert (m_nPin == 0);		// only Act LED is supported

		CBcmPropertyTags Tags;
		TPropertyTagGPIOState GPIOState;
		GPIOState.nGPIO = EXP_GPIO_BASE + 2;
		GPIOState.nState = nValue;
		Tags.GetTag (PROPTAG_SET_SET_GPIO_STATE, &GPIOState, sizeof GPIOState, 8);

		return;
	}

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
