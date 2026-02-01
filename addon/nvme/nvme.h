//
// nvme.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@gmx.net>
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
#ifndef _nvme_nvmedevice_h
#define _nvme_nvmedevice_h

#include <nvme/nvmesharedmemallocator.h>
#include <circle/device.h>
#include <circle/interrupt.h>
#include <circle/bcmpciehostbridge.h>
#include <circle/fs/partitionmanager.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/types.h>

#define NVME_LBA_SIZE	512	// The only supported NVMe LBA size format

enum NVME_STATUS : int
{
	NVME_STATUS_OK			= 0,		///< Success
	NVME_STATUS_ERROR_BAD_PARAM	= -1,		///< Invalid parameter given
	NVME_STATUS_ERROR_NO_RESOURCE	= -2,		///< Memory exhausted
	NVME_STATUS_ERROR_CONTROLLER	= -3,		///< Controller error
	NVME_STATUS_ERROR_TIMEOUT	= -4,		///< Command timed out
	NVME_STATUS_ERROR_READ_ONLY	= -5,		///< Write currently not supported
	NVME_STATUS_ERROR_LBA_RANGE	= -6		///< LBA out of range
};

class CNVMeDevice: public CDevice	/// Driver for PCIe NVMe Controller
{
public:
	/// \param pInterrupt Pointer to interrupt system object
	CNVMeDevice(CInterruptSystem *pInterrupt);

	~CNVMeDevice(void);

	/// \return Operation successful?
	bool Initialize(void);

	/// \param pBuffer Buffer, where read data will be placed
	/// \param ulCount Maximum number of bytes to be read
	/// \return Number of read bytes or < 0 on failure
	/// \note The buffer should be cache aligned for better performance.
	int Read (void *pBuffer, size_t ulCount) override;

	/// \param pBuffer Buffer, from which data will be fetched for write
	/// \param ulCount Number of bytes to be written
	/// \return Number of written bytes or < 0 on failure
	/// \note The buffer should be cache aligned for better performance.
	int Write (const void *pBuffer, size_t ulCount) override;

	/// \param ulOffset Byte offset from start
	/// \return The resulting offset, (u64) -1 on error
	u64 Seek (u64 ulOffset) override;

	/// \return Total byte size of the device, (u64) -1 on error
	u64 GetSize (void) const override;

	/// \param ulCmd The IOCtl command to invoke
	/// \param pData Depends on command, used to return command specific data
	/// \return Zero on success, or error code on failure
	/// \note Currently only supports DEVICE_IOCTL_SYNC.
	int IOCtl (unsigned long ulCmd, void *pData) override;

	void DumpStatus(void);

private:
	// Make contents of volatile write cache be made non-volatile
	// nNamespaceId: target namespace id (typically 1)
	int Flush(u32 nNamespaceId);

	// Send an Admin Identify command to get controller/namespace data.
	// pOutBuf must point to a buffer of at least 4096 bytes physically mapped.
	int Identify(u32 nCns, void *pOutBuf, u32 uNsId = 0);

	// Basic I/O Read/Write of logical blocks
	// nNamespaceId: target namespace id (typically 1)
	// nLba: starting LBA
	// nBlocks: number of logical blocks to transfer
	// pBuffer: virtual pointer to buffer (must be DMA/physically addressable by allocator)
	// bIsWrite: true for write, false for read
	int IoPassThrough(u32 nNamespaceId, u64 nLba, u32 nBlocks, void *pBuffer, bool bIsWrite);

	// Admin command submitter
	int AdminCommand(u8 nOpcode, u32 nNsId, u32 nCdw10, u32 nCdw11, u64 ulDataPhysAddr);

	// Internal helpers
	int CreateAdminQueues(void);
	int CreateIoQueue(u16 usQueueId, u16 usEntries);

	struct TQueue
	{
		const char *pName;	// queue name
		u16   usID;		// queue id
		u16   nEntries;		// queue entries

		void *pSqVirt;		// virtual pointer to Submission Queue array
		void *pCqVirt;		// virtual pointer to Completion Queue array
		u64   nSqPhys;		// physical address of Submission Queue array
		u64   nCqPhys;		// physical address of Completion Queue array

		u16   nSqTail;		// tail index for Submission Queue
		u16   nCqHead;		// head index for Completion Queue
		bool  bCqPhase;		// phase state of Completion Queue
	};

	// Submit command with opcode to queue with namespace id and parameters
	int SubmitCommand(TQueue *pQueue, u8 uchOpcode, u32 nNsId,
			  u32 nCdw10, u32 nCdw11, u32 nCdw12,
			  uintptr ulPrp1, uintptr ulPrp2);

	// Poll queue for command completion for up to nTimeoutHZ ticks
	int PollForCompletion(TQueue *pQueue, u16 usCid, unsigned nTimeoutHZ);

	// Wait for CSTS.RDY to equal target (true -> 1, false -> 0)
	bool WaitReady(bool bOn);

#ifdef NO_BUSY_WAIT
	static void InterruptHandler (void *pParam);
#endif

private:
	CBcmPCIeHostBridge m_PCIeExternal;
	CNVMeSharedMemAllocator m_Allocator;

#ifdef NO_BUSY_WAIT
	CInterruptSystem *m_pInterrupt;
	bool m_bIRQConnected;
#endif

	u32 m_nVersion;
	u64 m_ulCaps;
	unsigned m_nDoorbellStride;
	unsigned m_nTimeoutHZ;		// RDY timeout in HZ units

	TQueue m_AdminQueue;
	TQueue m_IoQueue;

	u64 m_ulNamespaceSize;
	u64 m_ulOffset;

	CPartitionManager *m_pPartitionManager;

#ifdef NO_BUSY_WAIT
	CSynchronizationEvent m_Event;
#endif
};

#endif
