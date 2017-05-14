//
// routecache.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_routecache_h
#define _circle_net_routecache_h

#include <circle/ptrarray.h>
#include <circle/types.h>

class CRouteCache
{
public:
	CRouteCache (void);
	~CRouteCache (void);

	void Flush (void);

	void AddRoute (const u8 *pDestIP, const u8 *pGatewayIP);

	const u8 *GetRoute (const u8 *pDestIP) const;

public:
	CPtrArray m_Cache;
};

#endif
