//
// bcmpropertytags.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
};

CBcmPropertyTags::CBcmPropertyTags (void)
:	m_MailBox (BCM_MAILBOX_PROP_OUT)
{
}

CBcmPropertyTags::~CBcmPropertyTags (void)
{
}

boolean CBcmPropertyTags::GetTag (u32 nTagId, void *pTag, unsigned nTagSize, unsigned nRequestParmSize)
{
	assert (pTag != 0);
	assert (nTagSize >= sizeof (TPropertyTagSimple));
	unsigned nBufferSize = sizeof (TPropertyBuffer) + nTagSize + sizeof (u32);
	assert ((nBufferSize & 3) == 0);

	// cannot use "new" here because this is used before mem_init() is called
	u8 Buffer[nBufferSize + 15];
	TPropertyBuffer *pBuffer = (TPropertyBuffer *) (((u32) Buffer + 15) & ~15);
	
	pBuffer->nBufferSize = nBufferSize;
	pBuffer->nCode = CODE_REQUEST;
	memcpy (pBuffer->Tags, pTag, nTagSize);
	
	TPropertyTag *pHeader = (TPropertyTag *) pBuffer->Tags;
	pHeader->nTagId = nTagId;
	pHeader->nValueBufSize = nTagSize - sizeof (TPropertyTag);
	pHeader->nValueLength = nRequestParmSize & ~VALUE_LENGTH_RESPONSE;

	u32 *pEndTag = (u32 *) (pBuffer->Tags + nTagSize);
	*pEndTag = PROPTAG_END;

	CleanDataCache ();
	DataSyncBarrier ();

	u32 nBufferAddress = GPU_MEM_BASE + (u32) pBuffer;
	if (m_MailBox.WriteRead (nBufferAddress) != nBufferAddress)
	{
		return FALSE;
	}
	
	InvalidateDataCache ();
	DataSyncBarrier ();

	if (pBuffer->nCode != CODE_RESPONSE_SUCCESS)
	{
		return FALSE;
	}
	
	if (!(pHeader->nValueLength & VALUE_LENGTH_RESPONSE))
	{
		return FALSE;
	}
	
	pHeader->nValueLength &= ~VALUE_LENGTH_RESPONSE;
	if (pHeader->nValueLength == 0)
	{
		return FALSE;
	}

	memcpy (pTag, pBuffer->Tags, nTagSize);

	return TRUE;
}
