//
// bcmpropertytags.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#include <circle/bcmpropertytags.h>
#include <circle/util.h>
#include <circle/synchronize.h>
#include <circle/bcm2835.h>
#include <circle/memory.h>
#include <circle/macros.h>
#include <assert.h>

struct TPropertyBuffer
{
	u32	nBufferSize;			// bytes
	u32	nCode;
	#define CODE_REQUEST		0x00000000
	#define CODE_RESPONSE_SUCCESS	0x80000000
	#define CODE_RESPONSE_FAILURE	0x80000001
	u8	Tags[0];
	// end tag follows
}
PACKED;

CBcmPropertyTags::CBcmPropertyTags (boolean bEarlyUse)
:	m_MailBox (BCM_MAILBOX_PROP_OUT, bEarlyUse)
{
}

CBcmPropertyTags::~CBcmPropertyTags (void)
{
}

boolean CBcmPropertyTags::GetTag (u32 nTagId, void *pTag, unsigned nTagSize, unsigned nRequestParmSize)
{
	assert (pTag != 0);
	assert (nTagSize >= sizeof (TPropertyTagSimple));

	TPropertyTag *pHeader = (TPropertyTag *) pTag;
	pHeader->nTagId = nTagId;
	pHeader->nValueBufSize = nTagSize - sizeof (TPropertyTag);
	pHeader->nValueLength = nRequestParmSize & ~VALUE_LENGTH_RESPONSE;

	if (!GetTags (pTag, nTagSize))
	{
		return FALSE;
	}

	pHeader->nValueLength &= ~VALUE_LENGTH_RESPONSE;
	if (pHeader->nValueLength == 0)
	{
		return FALSE;
	}

	return TRUE;
}

boolean CBcmPropertyTags::GetTags (void *pTags, unsigned nTagsSize)
{
	assert (pTags != 0);
	assert (nTagsSize >= sizeof (TPropertyTagSimple));
	unsigned nBufferSize = sizeof (TPropertyBuffer) + nTagsSize + sizeof (u32);
	assert ((nBufferSize & 3) == 0);

	TPropertyBuffer *pBuffer =
		(TPropertyBuffer *) CMemorySystem::GetCoherentPage (COHERENT_SLOT_PROP_MAILBOX);

	pBuffer->nBufferSize = nBufferSize;
	pBuffer->nCode = CODE_REQUEST;
	memcpy (pBuffer->Tags, pTags, nTagsSize);

	u32 *pEndTag = (u32 *) (pBuffer->Tags + nTagsSize);
	*pEndTag = PROPTAG_END;

	DataSyncBarrier ();

	u32 nBufferAddress = BUS_ADDRESS ((uintptr) pBuffer);
	if (m_MailBox.WriteRead (nBufferAddress) != nBufferAddress)
	{
		return FALSE;
	}

	DataMemBarrier ();

	if (pBuffer->nCode != CODE_RESPONSE_SUCCESS)
	{
		return FALSE;
	}

	memcpy (pTags, pBuffer->Tags, nTagsSize);

	return TRUE;
}
