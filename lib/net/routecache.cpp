//
// routecache.cpp
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
#include <circle/net/routecache.h>
#include <circle/net/ipaddress.h>
#include <circle/util.h>
#include <assert.h>

struct TRouteCacheEntry
{
	u8	DestIP[IP_ADDRESS_SIZE];
	u8	GatewayIP[IP_ADDRESS_SIZE];
};

CRouteCache::CRouteCache (void)
{
}

CRouteCache::~CRouteCache (void)
{
	Flush ();
}

void CRouteCache::Flush (void)
{
	unsigned nCount = m_Cache.GetCount ();
	while (nCount-- > 0)
	{
		delete (TRouteCacheEntry *) m_Cache[nCount];

		m_Cache.RemoveLast ();
	}
}

void CRouteCache::AddRoute (const u8 *pDestIP, const u8 *pGatewayIP)
{
	assert (pDestIP != 0);
	assert (pGatewayIP != 0);

	TRouteCacheEntry *pDestEntry = 0;

	unsigned nCount = m_Cache.GetCount ();
	for (unsigned i = 0; i < nCount; i++)
	{
		TRouteCacheEntry *pEntry = (TRouteCacheEntry *) m_Cache[i];
		assert (pEntry != 0);

		if (memcmp (pEntry->DestIP, pDestIP, IP_ADDRESS_SIZE) == 0)
		{
			pDestEntry = pEntry;

			break;
		}
	}

	if (pDestEntry == 0)
	{
		pDestEntry = new TRouteCacheEntry;
		assert (pDestEntry != 0);

		memcpy (pDestEntry->DestIP, pDestIP, IP_ADDRESS_SIZE);

		m_Cache.Append (pDestEntry);
	}

	memcpy (pDestEntry->GatewayIP, pGatewayIP, IP_ADDRESS_SIZE);
}

const u8 *CRouteCache::GetRoute (const u8 *pDestIP) const
{
	assert (pDestIP != 0);

	unsigned nCount = m_Cache.GetCount ();
	for (unsigned i = 0; i < nCount; i++)
	{
		const TRouteCacheEntry *pEntry = (const TRouteCacheEntry *) m_Cache[i];
		assert (pEntry != 0);

		if (memcmp (pEntry->DestIP, pDestIP, IP_ADDRESS_SIZE) == 0)
		{
			return pEntry->GatewayIP;
		}
	}

	return 0;
}
