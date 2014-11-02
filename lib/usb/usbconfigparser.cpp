//
// usbconfigparser.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbconfigparser.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <assert.h>

#define SKIP_BYTES(pDesc, nBytes)	((TUSBDescriptor *) ((u8 *) (pDesc) + (nBytes)))

CUSBConfigurationParser::CUSBConfigurationParser (const void *pBuffer, unsigned nBufLen)
:	m_pBuffer ((TUSBDescriptor *) pBuffer),
	m_nBufLen (nBufLen),
	m_bValid (FALSE),
	m_pEndPosition (SKIP_BYTES (m_pBuffer, nBufLen)),
	m_pCurrentPosition (m_pBuffer),
	m_pErrorPosition (m_pBuffer)
{
	assert (m_pBuffer != 0);
	
	if (   m_nBufLen < 4		// wTotalLength must exist
	    || m_nBufLen > 512)		// best guess
	{
		return;
	}

	if (   m_pBuffer->Configuration.bLength         != sizeof (TUSBConfigurationDescriptor)
	    || m_pBuffer->Configuration.bDescriptorType != DESCRIPTOR_CONFIGURATION
	    || m_pBuffer->Configuration.wTotalLength    >  nBufLen)
	{
		return;
	}

	if (m_pBuffer->Configuration.wTotalLength < nBufLen)
	{
		m_pEndPosition = SKIP_BYTES (m_pBuffer, m_pBuffer->Configuration.wTotalLength);
	}

	const TUSBDescriptor *pCurrentPosition = m_pBuffer;
	u8 ucLastDescType = 0;
	while (SKIP_BYTES (pCurrentPosition, 2) < m_pEndPosition)
	{
		u8 ucDescLen  = pCurrentPosition->Header.bLength;
		u8 ucDescType = pCurrentPosition->Header.bDescriptorType;

		TUSBDescriptor *pDescEnd = SKIP_BYTES (pCurrentPosition, ucDescLen);
		if (pDescEnd > m_pEndPosition)
		{
			m_pErrorPosition = pCurrentPosition;
			return;
		}

		u8 ucExpectedLen = 0;
		switch (ucDescType)
		{
		case DESCRIPTOR_CONFIGURATION:
			if (ucLastDescType != 0)
			{
				m_pErrorPosition = pCurrentPosition;
				return;
			}
			ucExpectedLen = sizeof (TUSBConfigurationDescriptor);
			break;

		case DESCRIPTOR_INTERFACE:
			if (ucLastDescType == 0)
			{
				m_pErrorPosition = pCurrentPosition;
				return;
			}
			ucExpectedLen = sizeof (TUSBInterfaceDescriptor);
			break;

		case DESCRIPTOR_ENDPOINT:
			if (   ucLastDescType == 0
			    || ucLastDescType == DESCRIPTOR_CONFIGURATION)
			{
				m_pErrorPosition = pCurrentPosition;
				return;
			}
			ucExpectedLen = sizeof (TUSBEndpointDescriptor);
			break;

		default:
			break;
		}

		if (   ucExpectedLen != 0
		    && ucDescLen != ucExpectedLen)
		{
			m_pErrorPosition = pCurrentPosition;
			return;
		}

		ucLastDescType = ucDescType;
		pCurrentPosition = pDescEnd;
	}

	if (pCurrentPosition != m_pEndPosition)
	{
		m_pErrorPosition = pCurrentPosition;
		return;
	}

	m_bValid = TRUE;
}

CUSBConfigurationParser::~CUSBConfigurationParser (void)
{
	m_pBuffer = 0;
}

boolean CUSBConfigurationParser::IsValid (void) const
{
	return m_bValid;
}

const TUSBDescriptor *CUSBConfigurationParser::GetDescriptor (u8 ucType)
{
	assert (m_bValid);

	const TUSBDescriptor *pResult = 0;
	
	while (m_pCurrentPosition < m_pEndPosition)
	{
		u8 ucDescLen  = m_pCurrentPosition->Header.bLength;
		u8 ucDescType = m_pCurrentPosition->Header.bDescriptorType;

		TUSBDescriptor *pDescEnd = SKIP_BYTES (m_pCurrentPosition, ucDescLen);
		assert (pDescEnd <= m_pEndPosition);

		if (   ucType     == DESCRIPTOR_ENDPOINT
		    && ucDescType == DESCRIPTOR_INTERFACE)
		{
			break;
		}

		if (ucDescType == ucType)
		{
			pResult = m_pCurrentPosition;
			m_pCurrentPosition = pDescEnd;
			break;
		}

		m_pCurrentPosition = pDescEnd;
	}

	if (pResult != 0)
	{
		m_pErrorPosition = pResult;
	}

	return pResult;
}

void CUSBConfigurationParser::Error (const char *pSource) const
{
	assert (pSource != 0);
	CLogger::Get ()->Write (pSource, LogError,
				"Invalid configuration descriptor (offset 0x%X)",
				(unsigned) m_pErrorPosition - (unsigned) m_pBuffer);
#ifndef NDEBUG
	debug_hexdump (m_pBuffer, m_nBufLen, pSource);
#endif
}
