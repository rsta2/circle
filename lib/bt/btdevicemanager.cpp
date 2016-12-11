//
// btdevicemanager.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/bt/btdevicemanager.h>
#include <circle/bt/bthcilayer.h>
#include <circle/bt/bcmvendor.h>
#include <circle/sched/scheduler.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

static const u8 Firmware[] =
{
#if RASPPI >= 2				// this takes 35 KByte and is not needed on RPi 1
	#include "BCM43430A1.h"
#endif
};

static const char FromDeviceManager[] = "btdev";

CBTDeviceManager::CBTDeviceManager (CBTHCILayer *pHCILayer, CBTQueue *pEventQueue,
				    u32 nClassOfDevice, const char *pLocalName)
:	m_pHCILayer (pHCILayer),
	m_pEventQueue (pEventQueue),
	m_nClassOfDevice (nClassOfDevice),
	m_State (BTDeviceStateUnknown),
	m_pBuffer (0)
{
	memset (m_LocalName, 0, sizeof m_LocalName);
	strncpy ((char *) m_LocalName, pLocalName, sizeof m_LocalName);
}

CBTDeviceManager::~CBTDeviceManager (void)
{
	delete [] m_pBuffer;
	m_pBuffer = 0;

	m_pHCILayer = 0;
	m_pEventQueue = 0;
}

boolean CBTDeviceManager::Initialize (void)
{
	assert (m_pHCILayer != 0);

	m_pBuffer = new u8[BT_MAX_HCI_EVENT_SIZE];
	assert (m_pBuffer != 0);

	TBTHCICommandHeader Cmd;
	Cmd.OpCode = OP_CODE_RESET;
	Cmd.ParameterTotalLength = PARM_TOTAL_LEN (Cmd);
	m_pHCILayer->SendCommand (&Cmd, sizeof Cmd);

	m_State = BTDeviceStateResetPending;

	return TRUE;
}

void CBTDeviceManager::Process (void)
{
	assert (m_pHCILayer != 0);
	assert (m_pEventQueue != 0);
	assert (m_pBuffer != 0);

	unsigned nLength;
	while ((nLength = m_pEventQueue->Dequeue (m_pBuffer)) > 0)
	{
		assert (nLength >= sizeof (TBTHCIEventHeader));
		TBTHCIEventHeader *pHeader = (TBTHCIEventHeader *) m_pBuffer;

		switch (pHeader->EventCode)
		{
		case EVENT_CODE_COMMAND_COMPLETE: {
			assert (nLength >= sizeof (TBTHCIEventCommandComplete));
			TBTHCIEventCommandComplete *pCommandComplete = (TBTHCIEventCommandComplete *) pHeader;

			m_pHCILayer->SetCommandPackets (pCommandComplete->NumHCICommandPackets);

			if (pCommandComplete->Status != BT_STATUS_SUCCESS)
			{
				CLogger::Get ()->Write (FromDeviceManager, LogError,
							"Command 0x%X failed (status 0x%X)",
							(unsigned) pCommandComplete->CommandOpCode,
							(unsigned) pCommandComplete->Status);

				m_State = BTDeviceStateFailed;

				continue;
			}

			switch (pCommandComplete->CommandOpCode)
			{
			case OP_CODE_RESET: {
				assert (m_State == BTDeviceStateResetPending);
#if RASPPI >= 2
				if (m_pHCILayer->GetTransportType () != BTTransportTypeUART)
				{
					goto NoFirmwareLoad;
				}

				TBTHCICommandHeader Cmd;
				Cmd.OpCode = OP_CODE_DOWNLOAD_MINIDRIVER;
				Cmd.ParameterTotalLength = PARM_TOTAL_LEN (Cmd);
				m_pHCILayer->SendCommand (&Cmd, sizeof Cmd);
				CScheduler::Get ()->MsSleep (50);

				m_nFirmwareOffset = 0;
				m_State = BTDeviceStateWriteRAMPending;
				} break;

			case OP_CODE_DOWNLOAD_MINIDRIVER:
			case OP_CODE_WRITE_RAM: {
				assert (m_State == BTDeviceStateWriteRAMPending);

				assert (m_nFirmwareOffset+3 <= sizeof Firmware);
				u16 nOpCode  = Firmware[m_nFirmwareOffset++];
				    nOpCode |= Firmware[m_nFirmwareOffset++] << 8;
				u8 nLength = Firmware[m_nFirmwareOffset++];

				TBTHCIBcmVendorCommand Cmd;
				Cmd.Header.OpCode = nOpCode;
				Cmd.Header.ParameterTotalLength = nLength;

				for (unsigned i = 0; i < nLength; i++)
				{
					assert (m_nFirmwareOffset < sizeof Firmware);
					Cmd.Data[i] = Firmware[m_nFirmwareOffset++];
				}

				m_pHCILayer->SendCommand (&Cmd, sizeof Cmd.Header + nLength);

				if (nOpCode == OP_CODE_LAUNCH_RAM)
				{
					m_State = BTDeviceStateLaunchRAMPending;
				}
				} break;

			case OP_CODE_LAUNCH_RAM: {
				assert (m_State == BTDeviceStateLaunchRAMPending);
				CScheduler::Get ()->MsSleep (250);

			NoFirmwareLoad:
#endif
				TBTHCICommandHeader Cmd;
				Cmd.OpCode = OP_CODE_READ_BD_ADDR;
				Cmd.ParameterTotalLength = PARM_TOTAL_LEN (Cmd);
				m_pHCILayer->SendCommand (&Cmd, sizeof Cmd);

				m_State = BTDeviceStateReadBDAddrPending;
				} break;

			case OP_CODE_READ_BD_ADDR: {
				assert (m_State == BTDeviceStateReadBDAddrPending);

				assert (nLength >= sizeof (TBTHCIEventReadBDAddrComplete));
				TBTHCIEventReadBDAddrComplete *pEvent = (TBTHCIEventReadBDAddrComplete *) pHeader;
				memcpy (m_LocalBDAddr, pEvent->BDAddr, BT_BD_ADDR_SIZE);

				CLogger::Get ()->Write (FromDeviceManager, LogNotice,
							"Local BD address is %02X:%02X:%02X:%02X:%02X:%02X",
							(unsigned) m_LocalBDAddr[5],
							(unsigned) m_LocalBDAddr[4],
							(unsigned) m_LocalBDAddr[3],
							(unsigned) m_LocalBDAddr[2],
							(unsigned) m_LocalBDAddr[1],
							(unsigned) m_LocalBDAddr[0]);

				TBTHCIWriteClassOfDeviceCommand Cmd;
				Cmd.Header.OpCode = OP_CODE_WRITE_CLASS_OF_DEVICE;
				Cmd.Header.ParameterTotalLength = PARM_TOTAL_LEN (Cmd);
				Cmd.ClassOfDevice[0] = m_nClassOfDevice       & 0xFF;
				Cmd.ClassOfDevice[1] = m_nClassOfDevice >> 8  & 0xFF;
				Cmd.ClassOfDevice[2] = m_nClassOfDevice >> 16 & 0xFF;
				m_pHCILayer->SendCommand (&Cmd, sizeof Cmd);

				m_State = BTDeviceStateWriteClassOfDevicePending;
				} break;

			case OP_CODE_WRITE_CLASS_OF_DEVICE: {
				assert (m_State == BTDeviceStateWriteClassOfDevicePending);

				TBTHCIWriteLocalNameCommand Cmd;
				Cmd.Header.OpCode = OP_CODE_WRITE_LOCAL_NAME;
				Cmd.Header.ParameterTotalLength = PARM_TOTAL_LEN (Cmd);
				memcpy (Cmd.LocalName, m_LocalName, sizeof Cmd.LocalName);
				m_pHCILayer->SendCommand (&Cmd, sizeof Cmd);

				m_State = BTDeviceStateWriteLocalNamePending;
				} break;

			case OP_CODE_WRITE_LOCAL_NAME: {
				assert (m_State == BTDeviceStateWriteLocalNamePending);

				TBTHCIWriteScanEnableCommand Cmd;
				Cmd.Header.OpCode = OP_CODE_WRITE_SCAN_ENABLE;
				Cmd.Header.ParameterTotalLength = PARM_TOTAL_LEN (Cmd);
				Cmd.ScanEnable = SCAN_ENABLE_BOTH_ENABLED;
				m_pHCILayer->SendCommand (&Cmd, sizeof Cmd);

				m_State = BTDeviceStateWriteScanEnabledPending;
				} break;

			case OP_CODE_WRITE_SCAN_ENABLE:
				assert (m_State == BTDeviceStateWriteScanEnabledPending);
				m_State = BTDeviceStateRunning;
				break;

			default:
				break;
			}
			} break;

		case EVENT_CODE_COMMAND_STATUS: {
			assert (nLength >= sizeof (TBTHCIEventCommandStatus));
			TBTHCIEventCommandStatus *pCommandStatus = (TBTHCIEventCommandStatus *) pHeader;

			m_pHCILayer->SetCommandPackets (pCommandStatus->NumHCICommandPackets);
			} break;

		default:
			assert (0);
			break;
		}
	}
}

boolean CBTDeviceManager::DeviceIsRunning (void) const
{
	return m_State == BTDeviceStateRunning ? TRUE : FALSE;
}
