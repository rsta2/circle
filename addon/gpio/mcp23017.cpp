//
// mcp23017.cpp
//
// Driver for MCP23017 I2C GPIO expander
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  T. Nelissen
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
#include "mcp23017.h"
#include <assert.h>

CMCP23017::CMCP23017 (CI2CMaster *pI2CMaster, u8 ucI2CAddress)
:	m_pI2CMaster (pI2CMaster),
	m_ucAddress (ucI2CAddress)
{
	assert (m_pI2CMaster != 0);
	assert (m_ucAddress >= 0x20 && m_ucAddress <= 0x27);
}

CMCP23017::~CMCP23017 (void)
{
}

boolean CMCP23017::Initialize (void)
{
	// Configure IOCON:
	// BANK=0   (sequential register addresses)
	// MIRROR=0 (separate INTA/INTB)
	// SEQOP=0  (sequential operation enabled)
	// DISSLW=0 (SDA slew rate control enabled)
	// HAEN=0   (hardware address pins disabled)
	// ODR=0    (active driver output for INT)
	// INTPOL=0 (interrupt output active-low)
	WriteRegister (MCP23017IOCon, 0x00);

	// Set all pins as inputs by default
	WriteRegister (MCP23017IODirA, 0xFF);
	WriteRegister (MCP23017IODirB, 0xFF);

	// Enable pull-ups on all pins
	WriteRegister (MCP23017GPPUA, 0xFF);
	WriteRegister (MCP23017GPPUB, 0xFF);

	// Enable interrupts on all pins
	WriteRegister (MCP23017GPIntEnA, 0xFF);
	WriteRegister (MCP23017GPIntEnB, 0xFF);

	// Compare against previous value (not DEFVAL)
	WriteRegister (MCP23017IntConA, 0x00);
	WriteRegister (MCP23017IntConB, 0x00);

	// Clear any pending interrupts by reading capture registers
	ReadRegister (MCP23017IntCapA);
	ReadRegister (MCP23017IntCapB);

	return TRUE;
}

void CMCP23017::SetDirectionA (u8 uchDirection)
{
	WriteRegister (MCP23017IODirA, uchDirection);
}

void CMCP23017::SetDirectionB (u8 uchDirection)
{
	WriteRegister (MCP23017IODirB, uchDirection);
}

void CMCP23017::SetPullUpA (u8 uchPullUp)
{
	WriteRegister (MCP23017GPPUA, uchPullUp);
}

void CMCP23017::SetPullUpB (u8 uchPullUp)
{
	WriteRegister (MCP23017GPPUB, uchPullUp);
}

void CMCP23017::EnableInterruptA (u8 uchPins)
{
	WriteRegister (MCP23017GPIntEnA, uchPins);
}

void CMCP23017::EnableInterruptB (u8 uchPins)
{
	WriteRegister (MCP23017GPIntEnB, uchPins);
}

u8 CMCP23017::ReadPortA (void)
{
	return ReadRegister (MCP23017GPIOA);
}

u8 CMCP23017::ReadPortB (void)
{
	return ReadRegister (MCP23017GPIOB);
}

u8 CMCP23017::ReadInterruptFlagA (void)
{
	return ReadRegister (MCP23017IntFA);
}

u8 CMCP23017::ReadInterruptFlagB (void)
{
	return ReadRegister (MCP23017IntFB);
}

u8 CMCP23017::ReadInterruptCaptureA (void)
{
	return ReadRegister (MCP23017IntCapA);
}

u8 CMCP23017::ReadInterruptCaptureB (void)
{
	return ReadRegister (MCP23017IntCapB);
}

void CMCP23017::WritePortA (u8 uchValue)
{
	WriteRegister (MCP23017OLATA, uchValue);
}

void CMCP23017::WritePortB (u8 uchValue)
{
	WriteRegister (MCP23017OLATB, uchValue);
}

void CMCP23017::WriteRegister (u8 uchRegister, u8 uchValue)
{
	assert (m_pI2CMaster != 0);

	u8 Buffer[] = {uchRegister, uchValue};
	m_pI2CMaster->Write (m_ucAddress, Buffer, sizeof Buffer);
}

u8 CMCP23017::ReadRegister (u8 uchRegister)
{
	assert (m_pI2CMaster != 0);

	u8 Buffer[] = {uchRegister};
	m_pI2CMaster->Write (m_ucAddress, Buffer, sizeof Buffer);
	m_pI2CMaster->Read (m_ucAddress, Buffer, sizeof Buffer);

	return Buffer[0];
}
