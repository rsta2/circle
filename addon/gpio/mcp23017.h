//
// mcp23017.h
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
#ifndef _gpio_mcp23017_h
#define _gpio_mcp23017_h

#include <circle/i2cmaster.h>
#include <circle/types.h>

/// MCP23017 register addresses (BANK=0, sequential mode)
enum TMCP23017Register
{
	MCP23017IODirA		= 0x00,		///< I/O direction A (1=input, 0=output)
	MCP23017IODirB		= 0x01,		///< I/O direction B
	MCP23017IPolA		= 0x02,		///< Input polarity A
	MCP23017IPolB		= 0x03,		///< Input polarity B
	MCP23017GPIntEnA	= 0x04,		///< Interrupt-on-change enable A
	MCP23017GPIntEnB	= 0x05,		///< Interrupt-on-change enable B
	MCP23017DefValA		= 0x06,		///< Default compare value A
	MCP23017DefValB		= 0x07,		///< Default compare value B
	MCP23017IntConA		= 0x08,		///< Interrupt control A
	MCP23017IntConB		= 0x09,		///< Interrupt control B
	MCP23017IOCon		= 0x0A,		///< Configuration register
	MCP23017GPPUA		= 0x0C,		///< Pull-up resistor A
	MCP23017GPPUB		= 0x0D,		///< Pull-up resistor B
	MCP23017IntFA		= 0x0E,		///< Interrupt flag A
	MCP23017IntFB		= 0x0F,		///< Interrupt flag B
	MCP23017IntCapA		= 0x10,		///< Interrupt capture A
	MCP23017IntCapB		= 0x11,		///< Interrupt capture B
	MCP23017GPIOA		= 0x12,		///< Port register A
	MCP23017GPIOB		= 0x13,		///< Port register B
	MCP23017OLATA		= 0x14,		///< Output latch A
	MCP23017OLATB		= 0x15		///< Output latch B
};

class CMCP23017		/// Driver for MCP23017 I2C 16-bit GPIO expander
{
public:
	/// \param pI2CMaster Pointer to I2C master object
	/// \param ucI2CAddress I2C slave address (0x20-0x27, default 0x20)
	CMCP23017 (CI2CMaster *pI2CMaster, u8 ucI2CAddress = 0x20);

	~CMCP23017 (void);

	/// \brief Initialize the MCP23017
	/// \return Operation successful?
	/// \note Sets BANK=0, all pins as input with pull-ups and interrupts enabled
	boolean Initialize (void);

	/// \brief Set I/O direction for port A
	/// \param uchDirection Bit mask (1=input, 0=output)
	void SetDirectionA (u8 uchDirection);
	/// \brief Set I/O direction for port B
	/// \param uchDirection Bit mask (1=input, 0=output)
	void SetDirectionB (u8 uchDirection);

	/// \brief Enable internal pull-ups for port A
	/// \param uchPullUp Bit mask (1=pull-up enabled)
	void SetPullUpA (u8 uchPullUp);
	/// \brief Enable internal pull-ups for port B
	/// \param uchPullUp Bit mask (1=pull-up enabled)
	void SetPullUpB (u8 uchPullUp);

	/// \brief Enable interrupt-on-change for port A pins
	/// \param uchPins Bit mask (1=interrupt enabled)
	void EnableInterruptA (u8 uchPins);
	/// \brief Enable interrupt-on-change for port B pins
	/// \param uchPins Bit mask (1=interrupt enabled)
	void EnableInterruptB (u8 uchPins);

	/// \brief Read current state of port A
	/// \return 8-bit port value
	u8 ReadPortA (void);
	/// \brief Read current state of port B
	/// \return 8-bit port value
	u8 ReadPortB (void);

	/// \brief Read interrupt flag register for port A
	/// \return Bit mask of pins that caused interrupt
	u8 ReadInterruptFlagA (void);
	/// \brief Read interrupt flag register for port B
	/// \return Bit mask of pins that caused interrupt
	u8 ReadInterruptFlagB (void);

	/// \brief Read captured pin state at time of interrupt for port A
	/// \return Port A value captured at interrupt time
	u8 ReadInterruptCaptureA (void);
	/// \brief Read captured pin state at time of interrupt for port B
	/// \return Port B value captured at interrupt time
	u8 ReadInterruptCaptureB (void);

	/// \brief Write to port A output latches
	/// \param uchValue 8-bit value to write
	void WritePortA (u8 uchValue);
	/// \brief Write to port B output latches
	/// \param uchValue 8-bit value to write
	void WritePortB (u8 uchValue);

private:
	void WriteRegister (u8 uchRegister, u8 uchValue);
	u8 ReadRegister (u8 uchRegister);

private:
	CI2CMaster *m_pI2CMaster;
	u8 m_ucAddress;
};

#endif
