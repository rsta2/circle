//
// mphi.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#include <circle/mphi.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/atomic.h>
#include <circle/synchronize.h>
#include <circle/macros.h>
#include <circle/new.h>
#include <assert.h>

#define MPHI_OUTDDB_TENDINT	BIT (29)

#define MPHI_CTRL_REQ_SOFT_RST	BIT (16)
#define MPHI_CTRL_SOFT_RST_DNE	BIT (17)
#define MPHI_CTRL_ENABLE	BIT (31)

#define MPHI_INTSTAT_TXEND	BIT (16)
#define MPHI_INTSTAT_IMFOFLW	BIT (29)

CMPHIDevice::CMPHIDevice (CInterruptSystem *pInterrupt)
:	m_pInterrupt (pInterrupt),
	m_pHandler (nullptr),
	m_pDummyDMABuffer (nullptr),
	m_nTriggerCount (0),
	m_nIntCount (0)
{
}

CMPHIDevice::~CMPHIDevice (void)
{
	if (m_pHandler)
	{
		DisconnectHandler ();
	}

	m_pInterrupt = 0;
}

void CMPHIDevice::ConnectHandler (TIRQHandler *pHandler, void *pParam)
{
	assert (!m_pHandler);
	m_pHandler = pHandler;
	m_pParam = pParam;
	assert (m_pHandler);

	assert (!m_pDummyDMABuffer);
	m_pDummyDMABuffer = new (HEAP_DMA30) u8[16];
	assert (m_pDummyDMABuffer);

	assert (m_pInterrupt);
	m_pInterrupt->ConnectIRQ (ARM_IRQ_HOSTPORT, IRQHandler, this);

	PeripheralEntry ();

	write32 (ARM_MPHI_CTRL, MPHI_CTRL_ENABLE);
	assert (read32 (ARM_MPHI_CTRL) & MPHI_CTRL_ENABLE);

	PeripheralExit ();
}

void CMPHIDevice::DisconnectHandler (void)
{
	assert (m_pHandler);

	assert (m_pInterrupt);
	m_pInterrupt->DisconnectIRQ (ARM_IRQ_HOSTPORT);

	m_pHandler = nullptr;

	PeripheralEntry ();

	write32 (ARM_MPHI_CTRL, MPHI_CTRL_ENABLE | MPHI_CTRL_REQ_SOFT_RST);
	while (!(read32 (ARM_MPHI_CTRL) & MPHI_CTRL_SOFT_RST_DNE))
	{
		// just wait
	}

	PeripheralExit ();

	delete m_pDummyDMABuffer;
	m_pDummyDMABuffer = 0;
}

void CMPHIDevice::TriggerIRQ (void)
{
	assert (m_pHandler);
	assert (m_pDummyDMABuffer);

	AtomicIncrement (&m_nTriggerCount);

	PeripheralEntry ();

	write32 (ARM_MPHI_INTSTAT, MPHI_INTSTAT_IMFOFLW);	// clear before re-trigger

	write32 (ARM_MPHI_OUTDDA, BUS_ADDRESS ((uintptr) m_pDummyDMABuffer));
	write32 (ARM_MPHI_OUTDDB, MPHI_OUTDDB_TENDINT);

	PeripheralExit ();
}

void CMPHIDevice::IRQHandler (void *pParam)
{
	CMPHIDevice *pThis = static_cast<CMPHIDevice *> (pParam);
	assert (pThis);

	do
	{
		assert (pThis->m_pHandler);
		(*pThis->m_pHandler) (pThis->m_pParam);

		PeripheralEntry ();

		write32 (ARM_MPHI_INTSTAT, MPHI_INTSTAT_TXEND);		// acknowledge interrupt

		// The device stops generating interrupts after 124 attempts,
		// that's why we have to restart it in time.
		if (++pThis->m_nIntCount >= 50)
		{
			write32 (ARM_MPHI_CTRL, MPHI_CTRL_ENABLE | MPHI_CTRL_REQ_SOFT_RST);
			while (!(read32 (ARM_MPHI_CTRL) & MPHI_CTRL_SOFT_RST_DNE))
			{
				// just wait
			}

			write32 (ARM_MPHI_CTRL, MPHI_CTRL_ENABLE);

			pThis->m_nIntCount = 0;
		}

		PeripheralExit ();
	}
	while (AtomicDecrement (&pThis->m_nTriggerCount));
}
