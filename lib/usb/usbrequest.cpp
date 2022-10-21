//
// usbrequest.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbrequest.h>
#include <assert.h>

CUSBRequest::CUSBRequest (CUSBEndpoint *pEndpoint, void *pBuffer, u32 nBufLen, TSetupData *pSetupData)
:	m_pEndpoint (pEndpoint),
	m_pSetupData (pSetupData),
	m_pBuffer (pBuffer),
	m_nBufLen (nBufLen),
	m_bStatus (0),
	m_nResultLen (0),
	m_USBError (USBErrorUnknown),
	m_nNumIsoPackets (0),
	m_pCompletionRoutine (0),
	m_pCompletionParam (0),
	m_pCompletionContext (0),
	m_bCompleteOnNAK (FALSE)
{
	assert (m_pEndpoint != 0);
	assert (m_pBuffer != 0 || m_nBufLen == 0);
}

CUSBRequest::~CUSBRequest (void)
{
	m_pEndpoint = 0;
	m_pSetupData = 0;
	m_pBuffer = 0;
	m_pCompletionRoutine = 0;
}

CUSBEndpoint *CUSBRequest::GetEndpoint (void) const
{
	assert (m_pEndpoint != 0);
	return m_pEndpoint;
}

void CUSBRequest::SetStatus (int bStatus)
{
	m_bStatus = bStatus;
}

void CUSBRequest::SetResultLen (u32 nLength)
{
	m_nResultLen = nLength;
}

void CUSBRequest::SetUSBError (TUSBError Error)
{
	m_USBError = Error;
}

int CUSBRequest::GetStatus (void) const
{
	return m_bStatus;
}

u32 CUSBRequest::GetResultLength (void) const
{
	assert (m_bStatus);

	return m_nResultLen;
}

TUSBError CUSBRequest::GetUSBError (void) const
{
	assert (!m_bStatus);

	return m_USBError;
}

TSetupData *CUSBRequest::GetSetupData (void)
{
	assert (m_pEndpoint->GetType () == EndpointTypeControl);
	assert (m_pSetupData != 0);

	return m_pSetupData;
}

void *CUSBRequest::GetBuffer (void)
{
	assert (   m_pBuffer != 0
		|| m_nBufLen == 0);

	return m_pBuffer;
}

u32 CUSBRequest::GetBufLen (void) const
{
	return m_nBufLen;
}

void CUSBRequest::AddIsoPacket (u16 usPacketSize)
{
	assert (m_nNumIsoPackets < MaxIsoPackets);
	assert (usPacketSize > 0);
	assert (usPacketSize <= m_nBufLen);

	m_usIsoPacketSize[m_nNumIsoPackets++] = usPacketSize;
}

u32 CUSBRequest::GetNumIsoPackets (void) const
{
	assert (m_nNumIsoPackets > 0);

	return m_nNumIsoPackets;
}

u16 CUSBRequest::GetIsoPacketSize (unsigned nPacketIndex) const
{
	assert (nPacketIndex < m_nNumIsoPackets);

	return m_usIsoPacketSize[nPacketIndex];
}

void CUSBRequest::SetCompletionRoutine (TURBCompletionRoutine *pRoutine, void *pParam, void *pContext)
{
	m_pCompletionRoutine = pRoutine;
	m_pCompletionParam   = pParam;
	m_pCompletionContext = pContext;

	assert (m_pCompletionRoutine != 0);
}

void CUSBRequest::CallCompletionRoutine (void)
{
	assert (m_pCompletionRoutine != 0);
	
	(*m_pCompletionRoutine) (this, m_pCompletionParam, m_pCompletionContext);
}

void CUSBRequest::SetCompleteOnNAK (void)
{
	m_bCompleteOnNAK = TRUE;
}

boolean CUSBRequest::IsCompleteOnNAK (void) const
{
	return m_bCompleteOnNAK;
}

IMPLEMENT_CLASS_ALLOCATOR (CUSBRequest)
