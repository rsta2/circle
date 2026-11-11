// SPDX-License-Identifier: GPL-3.0-or-later
//
// usbstringdescriptor.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
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
#include <circle/usb/gadget/usbstringdescriptor.h>
#include <circle/usb/usb.h>

const void *USBStringToDescriptor (const char *pString, void *pBuffer,
				  size_t nBufferSize, size_t *pLength)
{
	if (!pLength)
	{
		return nullptr;
	}
	*pLength = 0;
	if (!pString || !pBuffer || nBufferSize < 2)
	{
		return nullptr;
	}

	// bLength is one byte and a UTF-16 descriptor has an even byte length.
	size_t nLength = 2;
	for (const char *p = pString; *p; p++)
	{
		if ((u8) *p > 0x7F || nLength >= 254 || nBufferSize-nLength < 2)
		{
			return nullptr;
		}
		nLength += 2;
	}

	u8 *p = static_cast<u8 *> (pBuffer);
	*p++ = (u8) nLength;
	*p++ = DESCRIPTOR_STRING;
	while (*pString)
	{
		*p++ = (u8) *pString++;
		*p++ = 0;
	}
	*pLength = nLength;
	return pBuffer;
}
