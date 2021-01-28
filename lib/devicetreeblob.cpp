//
// devicetreeblob.cpp
//
// Reference:
//	https://github.com/devicetree-org/devicetree-specification/releases/
//		download/v0.3/devicetree-specification-v0.3.pdf
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#include <circle/devicetreeblob.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

#define DTB_MAX_SIZE		0x100000		// best guess

#define DTB_ALIGN(n)		(((n) + 3) & ~3)

// all values big endian

struct TDeviceTreeBlobHeader
{
	u32	magic;
#define DTB_MAGIC		0xD00DFEED
	u32	totalsize;
	u32	off_dt_struct;
	u32	off_dt_strings;
	u32	off_mem_rsvmap;
	u32	version;
#define DTB_VERSION		17
	u32	last_comp_version;
#define DTB_LAST_COMP_VERSION	16
	u32	boot_cpuid_phys;
	u32	size_dt_strings;
	u32	size_dt_struct;
}
PACKED;

enum TFTDToken
{
	FDT_BEGIN_NODE	= 1,
	FDT_END_NODE,
	FDT_PROP,
	FDT_NOP,
	FDT_END		= 9
};

struct TDeviceTreeNode
{
	u32	token;
	u8	data[];		// name followed by properties
}
PACKED;

struct TDeviceTreeProperty
{
	u32	token;
	u32	len;
	u32	nameoff;
	u8	data[];
}
PACKED;

union TDeviceTreePiece
{
	u32			token;
	TDeviceTreeNode		node;
	TDeviceTreeProperty	property;
}
PACKED;

static const char From[] = "dtb";

CDeviceTreeBlob::CDeviceTreeBlob (const void *pBuffer)
:	m_pFTD (0)
{
	const TDeviceTreeBlobHeader *pHeader = (const TDeviceTreeBlobHeader *) pBuffer;
	if (   pHeader == 0
	    || be2le32 (pHeader->magic) != DTB_MAGIC
	    || be2le32 (pHeader->last_comp_version) != DTB_LAST_COMP_VERSION)
	{
		return;
	}

	u32 nTotalSize = be2le32 (pHeader->totalsize);
	if (   nTotalSize < sizeof (TDeviceTreeBlobHeader)
	    || nTotalSize > DTB_MAX_SIZE)
	{
		return;
	}

	m_pFTD = new u8[nTotalSize];
	if (m_pFTD == 0)
	{
		return;
	}

	memcpy (m_pFTD, pHeader, nTotalSize);
}

CDeviceTreeBlob::~CDeviceTreeBlob (void)
{
	delete [] m_pFTD;
	m_pFTD = 0;
}

const TDeviceTreeNode *CDeviceTreeBlob::FindNode (const char *pPath,
						  const TDeviceTreeNode *pNode) const
{
	return FindNodeInternal (pPath, pNode, 0);
}

const TDeviceTreeNode *CDeviceTreeBlob::FindNodeInternal (const char *pPath,
							  const TDeviceTreeNode *pNode,
							  const TDeviceTreeNode **ppNextNode) const
{
	assert (pPath != 0);

	if (m_pFTD == 0)
	{
		CLogger::Get ()->Write (From, LogWarning, "DTB not available");

		return 0;
	}

	if (pNode == 0)
	{
		const TDeviceTreeBlobHeader *pHeader = (const TDeviceTreeBlobHeader *) m_pFTD;

		pNode = (const TDeviceTreeNode *) (m_pFTD + be2le32 (pHeader->off_dt_struct));

		if (pPath[0] != '/')
		{
			CLogger::Get ()->Write (From, LogWarning, "Invalid path: %s", pPath);

			return 0;
		}

		pPath++;
	}

	while (be2le32 (pNode->token) == FDT_NOP)
	{
		pNode = (TDeviceTreeNode *) ((u8 *) pNode + sizeof (u32));
	}

	if (be2le32 (pNode->token) != FDT_BEGIN_NODE)
	{
		CLogger::Get ()->Write (From, LogWarning, "FDT_BEGIN_NODE expected (0x%X)",
					be2le32 (pNode->token));

		return 0;
	}

	if (pPath[0] == '\0')		// root node
	{
		return pNode;
	}

	size_t nLen;
	const char *pEnd = strchr (pPath, '/');
	if (pEnd != 0)
	{
		nLen = pEnd - pPath;
	}
	else
	{
		nLen = strlen (pPath);
	}

	if (nLen == 0)
	{
		CLogger::Get ()->Write (From, LogWarning, "Zero length path component");

		return 0;
	}

	char Name[nLen+1];
	memcpy (Name, pPath, nLen);
	Name[nLen] = '\0';

	const char *pNodeName = (const char *) pNode->data;
	if (strcmp (pNodeName, Name) == 0)
	{
		pPath += nLen;
		if (pPath[0] == '/')
		{
			return FindNodeInternal (pPath+1, pNode, 0);
		}

		return pNode;
	}

	nLen = DTB_ALIGN (strlen (pNodeName) + 1);

	TDeviceTreePiece *pPiece = (TDeviceTreePiece *) (pNode->data + nLen);
	u32 nToken = be2le32 (pPiece->token);
	while (   nToken == FDT_NOP
	       || nToken == FDT_PROP)
	{
		if (nToken == FDT_NOP)
		{
			nLen = sizeof (u32);
		}
		else
		{
			nLen =   sizeof (TDeviceTreeProperty)
			       + DTB_ALIGN (be2le32 (pPiece->property.len));
		}

		pPiece = (TDeviceTreePiece *) ((u8 *) pPiece + nLen);
		nToken = be2le32 (pPiece->token);
	}

	while (nToken == FDT_BEGIN_NODE)
	{
		const TDeviceTreeNode *pNextNode = 0;
		const TDeviceTreeNode *pNode = FindNodeInternal (pPath, &pPiece->node, &pNextNode);
		if (pNode != 0)
		{
			return pNode;
		}

		if (pNextNode == 0)
		{
			return 0;
		}

		pPiece = (TDeviceTreePiece *) pNextNode;
		nToken = be2le32 (pPiece->token);
	}

	while (nToken == FDT_NOP)
	{
		pPiece = (TDeviceTreePiece *) ((u8 *) pPiece + sizeof (u32));
		nToken = be2le32 (pPiece->token);
	}

	if (   nToken == FDT_END_NODE
	    && ppNextNode != 0)
	{
		*ppNextNode = (TDeviceTreeNode *) ((u8 *) pPiece + sizeof (u32));
	}

	return 0;
}


const TDeviceTreeProperty *CDeviceTreeBlob::FindProperty (const TDeviceTreeNode *pNode,
							  const char *pName) const
{
	assert (pNode != 0);
	assert (pName != 0);

	if (m_pFTD == 0)
	{
		return 0;
	}

	if (be2le32 (pNode->token) != FDT_BEGIN_NODE)
	{
		CLogger::Get ()->Write (From, LogWarning, "FDT_BEGIN_NODE expected (0x%X)",
					be2le32 (pNode->token));
		return 0;
	}

	size_t nLen = sizeof (u32) + DTB_ALIGN (strlen ((const char *) pNode->data) + 1);

	TDeviceTreePiece *pPiece = (TDeviceTreePiece *) ((u8 *) pNode + nLen);
	u32 nToken = be2le32 (pPiece->token);
	while (   nToken == FDT_PROP
	       || nToken == FDT_NOP)
	{
		if (nToken == FDT_NOP)
		{
			nLen = sizeof (u32);
		}
		else
		{
			const TDeviceTreeBlobHeader *pHeader = (const TDeviceTreeBlobHeader *) m_pFTD;
			const char *pPropName = (const char *) (  m_pFTD
								+ be2le32 (pHeader->off_dt_strings)
								+ be2le32 (pPiece->property.nameoff));

			if (strcmp (pPropName, pName) == 0)
			{
				return &pPiece->property;
			}

			nLen =   sizeof (TDeviceTreeProperty)
			       + DTB_ALIGN (be2le32 (pPiece->property.len));
		}

		pPiece = (TDeviceTreePiece *) ((u8 *) pPiece + nLen);
		nToken = be2le32 (pPiece->token);
	}

	CLogger::Get ()->Write (From, LogWarning, "Property not found: %s", pName);

	return 0;
}

size_t CDeviceTreeBlob::GetPropertyValueLength (const TDeviceTreeProperty *pProperty) const
{
	assert (pProperty != 0);

	if (m_pFTD == 0)
	{
		return 0;
	}

	return be2le32 (pProperty->len);
}

const u8 *CDeviceTreeBlob::GetPropertyValue (const TDeviceTreeProperty *pProperty) const
{
	assert (pProperty != 0);

	if (m_pFTD == 0)
	{
		return 0;
	}

	return pProperty->data;
}

u32 CDeviceTreeBlob::GetPropertyValueWord (const TDeviceTreeProperty *pProperty,
					   unsigned nIndex) const
{
	assert (pProperty != 0);

	if (m_pFTD == 0)
	{
		return 0;
	}

	if (be2le32 (pProperty->len) < (nIndex+1) * sizeof (u32))
	{
		CLogger::Get ()->Write (From, LogWarning, "Invalid data length (%u)",
					be2le32 (pProperty->len));

		return 0;
	}

	return be2le32 (*((u32 *) pProperty->data + nIndex));
}
