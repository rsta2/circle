//
// devicetreeblob.h
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
#ifndef _circle_devicetreeblob_h
#define _circle_devicetreeblob_h

#include <circle/types.h>

struct TDeviceTreeNode;
struct TDeviceTreeProperty;

class CDeviceTreeBlob	/// Simple Devicetree blob parser
{
public:
	/// \param pBuffer Pointer to the begin of the DTB in memory
	CDeviceTreeBlob (const void *pBuffer);
	~CDeviceTreeBlob (void);
	
	/// \param pPath Path string to the node (e.g. "/node1/node2")
	/// \param pParentNode Search inside this node, 0 for root node
	/// \return Opaque pointer to the node, or 0 if not found
	const TDeviceTreeNode *FindNode (const char *pPath,
					 const TDeviceTreeNode *pParentNode = 0) const;

	/// \param pNode Pointer to the node
	/// \param pName Name of the property
	/// \return Opaque pointer to the property, or 0 if not found
	const TDeviceTreeProperty *FindProperty (const TDeviceTreeNode *pNode,
						 const char *pName) const;
	
	/// \param pProperty Pointer to the property
	/// \return Size of the property value in bytes (0 for an empty property) 
	size_t GetPropertyValueLength (const TDeviceTreeProperty *pProperty) const;

	/// \param pProperty Pointer to the property
	/// \return Pointer to the first byte of the property value
	const u8 *GetPropertyValue (const TDeviceTreeProperty *pProperty) const;

	/// \param pProperty Pointer to the property
	/// \param nIndex 0-based index of the requested word
	/// \return (nIndex+1)'th word for the property value
	u32 GetPropertyValueWord (const TDeviceTreeProperty *pProperty,
				  unsigned nIndex) const;

private:
	const TDeviceTreeNode *FindNodeInternal (const char *pPath,
						 const TDeviceTreeNode *pNode,
						 const TDeviceTreeNode **ppNextNode) const;

private:
	u8 *m_pFTD;
};

#endif
