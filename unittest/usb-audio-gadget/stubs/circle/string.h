// SPDX-License-Identifier: GPL-3.0-or-later
// Host substitute for diagnostic formatting only, not descriptor parsing.
#pragma once
#include <string>
#include <cstdio>
class CString
{
public:
 CString (const char *p = "") : m_String (p) {}
 operator const char * () const { return m_String.c_str (); }
 void Format (const char *pFormat, unsigned n)
 { char Buffer[32]; std::snprintf (Buffer, sizeof Buffer, pFormat, n); m_String = Buffer; }
 void Append (const char *p) { m_String += p; }
 size_t GetLength () const { return m_String.size (); }
 CString &operator = (const char *p) { m_String = p; return *this; }
private:
 std::string m_String;
};
