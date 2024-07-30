//
// usmstgadgetendpoint.cpp
//
// USB Mass Storage Gadget by Mike Messinides
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023-2024  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/gadget/usbmsdgadgetendpoint.h>
#include <circle/usb/gadget/usbmsdgadget.h>
#include <circle/logger.h>
#include <assert.h>

#define MLOGNOTE(From,...)		//CLogger::Get ()->Write (From, LogNotice, __VA_ARGS__)

CUSBMSDGadgetEndpoint::CUSBMSDGadgetEndpoint (const TUSBEndpointDescriptor *pDesc,
					      CUSBMSDGadget *pGadget)
:	CDWUSBGadgetEndpoint (pDesc, pGadget),
	m_pGadget (pGadget)
{

}

CUSBMSDGadgetEndpoint::~CUSBMSDGadgetEndpoint (void)
{

}

//the following methods forward to the gadget class to facilitate
//unified state management of the device

void CUSBMSDGadgetEndpoint::OnActivate (void)
{
	if (GetDirection () == DirectionOut)
	{
		m_pGadget->OnActivate();
	}
}

void CUSBMSDGadgetEndpoint::OnTransferComplete (boolean bIn, size_t nLength)
{
	MLOGNOTE("MSDEndpoint","Transfer complete nlen= %i",nLength);
	m_pGadget->OnTransferComplete(bIn, nLength);
}


void CUSBMSDGadgetEndpoint::BeginTransfer (TMSDTransferMode Mode, void *pBuffer, size_t nLength)
{
	switch (Mode)
	{
	case TMSDTransferMode::TransferCBWOut:
	case TMSDTransferMode::TransferDataOut:
		MLOGNOTE("MSDEndpoint","Begin Transfer Out  nlen= %i",nLength);
		CDWUSBGadgetEndpoint::BeginTransfer (TTransferMode::TransferDataOut, pBuffer, nLength);
		break;
	case TMSDTransferMode::TransferDataIn:
	case TMSDTransferMode::TransferCSWIn:
		MLOGNOTE("MSDEndpoint","Begin Transfer In  nlen= %i",nLength);
		CDWUSBGadgetEndpoint::BeginTransfer (TTransferMode::TransferDataIn, pBuffer, nLength);
		break;
	default:
		assert(0);
		break;
	}
}

void CUSBMSDGadgetEndpoint::StallRequest(boolean bIn)
{
	CDWUSBGadgetEndpoint::Stall(bIn);

}
