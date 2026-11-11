// SPDX-License-Identifier: GPL-3.0-or-later
#include <circle/usb/gadget/usbstringdescriptor.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>

void StringTests ()
{
	u8 Buffer[260];
	size_t nLength = 99;
	memset (Buffer, 0xA5, sizeof Buffer);
	assert (USBStringToDescriptor ("Circle", Buffer+1, 14, &nLength) == Buffer+1);
	const u8 Expected[] = {14, 3, 'C', 0, 'i', 0, 'r', 0, 'c', 0, 'l', 0, 'e', 0};
	assert (nLength == sizeof Expected && !memcmp (Buffer+1, Expected, nLength));
	assert (Buffer[0] == 0xA5 && Buffer[15] == 0xA5);
	assert (USBStringToDescriptor ("", Buffer, 2, &nLength) == Buffer);
	assert (nLength == 2 && Buffer[0] == 2 && Buffer[1] == 3);

	std::string Maximum (126, 'x');
	assert (USBStringToDescriptor (Maximum.c_str (), Buffer, sizeof Buffer, &nLength));
	assert (nLength == 254 && Buffer[0] == 254);
	Maximum += 'x';
	for (const char *p : {Maximum.c_str (), "\xC3\xA9", "\xFF"})
	{
		memset (Buffer, 0xA5, sizeof Buffer);
		assert (!USBStringToDescriptor (p, Buffer, sizeof Buffer, &nLength));
		assert (!nLength);
		for (u8 Byte : Buffer) assert (Byte == 0xA5);
	}
	for (size_t nSize = 0; nSize < 14; nSize++)
	{
		memset (Buffer, 0xA5, sizeof Buffer);
		assert (!USBStringToDescriptor ("Circle", Buffer, nSize, &nLength));
		assert (!nLength);
		for (u8 Byte : Buffer) assert (Byte == 0xA5);
	}
	assert (!USBStringToDescriptor (nullptr, Buffer, sizeof Buffer, &nLength) && !nLength);
	assert (!USBStringToDescriptor ("Circle", nullptr, sizeof Buffer, &nLength) && !nLength);
	assert (!USBStringToDescriptor ("Circle", Buffer, sizeof Buffer, nullptr));
	puts ("PASS shared USB strings: UTF-16LE bytes, empty/exact/max lengths, bounds, ASCII validation and null arguments");
}
