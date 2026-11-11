// SPDX-License-Identifier: GPL-3.0-or-later
//
// usbstringdescriptor.h
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
#ifndef _circle_usb_gadget_usbstringdescriptor_h
#define _circle_usb_gadget_usbstringdescriptor_h

#include <circle/types.h>

/// Convert a NUL-terminated ASCII string to a USB UTF-16LE string descriptor.
/// Caller owns the output buffer; input and output must not overlap.
/// Returns nullptr and sets *pLength to zero on invalid input or insufficient
/// space. No partial descriptor is written. Maximum length is 126 characters.
const void *USBStringToDescriptor (const char *pString, void *pBuffer,
				  size_t nBufferSize, size_t *pLength);

#endif
