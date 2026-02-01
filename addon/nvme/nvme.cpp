//
// nvme.cpp
//
// Driver for PCIe NVMe Controller
//
// Support for:
//	Tested with NVMe v1.4 only, v1.3 may also work
//	One I/O queue only
//	512 Byte LBA size format only
//	4KB page size only
//	Namespace with NSID 1 only
//	Controller Identifier (CNTID) 0 only
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
#include <nvme/nvme.h>
#include <nvme/nvmeprp.h>
#include <nvme/nvmehelper.h>
#include <circle/devicenameservice.h>
#include <circle/sysconfig.h>
#include <circle/memory.h>
#include <circle/memio.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

#ifdef NO_BUSY_WAIT
	#include <circle/sched/scheduler.h>
#endif

//#define NVME_DEBUG
//#define NVME_READ_ONLY

#define PCIE_SLOT			0
#define PCIE_FUNC			0
#define PCIE_CLASS_CODE			0x010802	// NVM Express

// NVMe register offsets
#define NVME_REG_CAP			0x0000
	#define NVME_REG_CAP_NSSRS		BIT(36)
	#define NVME_REG_CAP_DSTRD__SHIFT	32
	#define NVME_REG_CAP_DSTRD__MASK	(0x0FUL << 32)
	#define NVME_REG_CAP_TO__SHIFT		24
	#define NVME_REG_CAP_TO__MASK		(0xFF << 24)
#define NVME_REG_VER			0x0008
	#define NVME_REG_VER_MJR__SHIFT		16
	#define NVME_REG_VER_MJR__MASK		(0xFFFF << 16)
	#define NVME_REG_VER_MNR__SHIFT		8
	#define NVME_REG_VER_MNR__MASK		(0xFF << 8)
	#define NVME_REG_VER_TER__SHIFT		0
	#define NVME_REG_VER_TER__MASK		(0xFF << 0)
#define NVME_REG_INTMS			0x000C
#define NVME_REG_INTMC			0x0010
	#define NVME_REG_INTM_ALL_VECTORS	0xFFFFFFFFU
	#define NVME_REG_INTM_VECTOR0		BIT(0)
#define NVME_REG_CC			0x0014
	#define NVME_REG_CC_IOCQES__SHIFT	20
	#define NVME_REG_CC_IOCQES__MASK	(0xF << 20)
		#define NVME_REG_CC_IOCQES_16B	4
	#define NVME_REG_CC_IOSQES__SHIFT	16
	#define NVME_REG_CC_IOSQES__MASK	(0xF << 16)
		#define NVME_REG_CC_IOSQES_64B	6
	#define NVME_REG_CC_EN			BIT(0)
#define NVME_REG_CSTS			0x001C
	#define NVME_REG_CSTS_RDY		BIT(0)
#define NVME_REG_NSSR			0x0020
	#define NVME_REG_NSSR_RESET		0x4E564D65
#define NVME_REG_AQA			0x0024
#define NVME_REG_ASQ			0x0028
#define NVME_REG_ACQ			0x0030

#define DOORBELL_STRIDE(dstrd)		(1 << ((dstrd) + 2))
#define NVME_REG_DOORBELL_BASE		0x1000
#define NVME_DOORBELL_SQ_OFFSET(index)	(NVME_REG_DOORBELL_BASE + ((index) * m_nDoorbellStride*2))
#define NVME_DOORBELL_CQ_OFFSET(index)	(NVME_REG_DOORBELL_BASE + ((index) * m_nDoorbellStride*2) + 4)

// Admin Command Opcodes
#define NVME_ADMIN_OPC_DELETE_IO_SQ	0x00
#define NVME_ADMIN_OPC_CREATE_IO_SQ	0x01
#define NVME_ADMIN_OPC_CREATE_IO_CQ	0x05
#define NVME_ADMIN_OPC_IDENTIFY		0x06

// NVM Command Opcodes
#define NVME_IO_OPC_FLUSH		0x00
#define NVME_IO_OPC_WRITE 		0x01
#define NVME_IO_OPC_READ		0x02

// Identifier
#define NSID				1	// ID of our only supported namespace

#define AQID				0	// ID of Admin queue (fixed)
#define IOQID				1	// ID of our only I/O queue (submission / completion)

// Constants for queue sizes
#define NVME_ADMIN_QUEUE_ENTRIES	64
#define NVME_IO_QUEUE_ENTRIES		64

#define POLL_TIMEOUT_HZ			MSEC2HZ(5000)

// We need this, because the library is build with strict-align for accessing coherent memory.
#define ATOMIC_READ(ptr)		__atomic_load_n (ptr, __ATOMIC_RELAXED)
#define ATOMIC_WRITE(ptr, val)		__atomic_store_n (ptr, val, __ATOMIC_RELAXED)

struct TNVMeCommand	// Submission queue entry
{
	u8  opc;
	u8  fuse;
	u16 cid;
	u32 nsid;
	u64 reserved;
	u64 mptr;
	u64 prp1;
	u64 prp2;
	u32 cdw10;
	u32 cdw11;
	u32 cdw12;
	u32 cdw13;
	u32 cdw14;
	u32 cdw15;
};

// Cannot use PACKED here, because Clang shows warning. Check if it's packed here:
ASSERT_STATIC (sizeof (TNVMeCommand) == 64);

struct TNVMeCompletion	// Completion queue entry
{
	u32 dw0;
	u32 dw1;	// reserved
	u16 sqhead;
	u16 sqid;
	u16 cid;
	u16 status;
#define CQE_STATUS_SCT__SHIFT	9
#define CQE_STATUS_SCT__MASK	(7 << 9)
#define CQE_STATUS_SC__SHIFT	1
#define CQE_STATUS_SC__MASK	(0xFF << 1)
#define CQE_STATUS_PHASE_BIT	BIT(0)
};

ASSERT_STATIC (sizeof (TNVMeCompletion) == 16);

LOGMODULE ("nvme");

static const char DeviceName[] = "nvme1";

static inline u64 MmioRead64(unsigned nOffset)
{
	return read64 (MEM_PCIE_EXT_RANGE_START + nOffset);
}

static inline u32 MmioRead32(unsigned nOffset)
{
	return read32 (MEM_PCIE_EXT_RANGE_START + nOffset);
}

static inline void MmioWrite64(unsigned nOffset, u64 ulValue)
{
	write64 (MEM_PCIE_EXT_RANGE_START + nOffset, ulValue);
}

static inline void MmioWrite32(unsigned nOffset, u32 nValue)
{
	write32 (MEM_PCIE_EXT_RANGE_START + nOffset, nValue);
}

CNVMeDevice::CNVMeDevice(CInterruptSystem *pInterrupt)
: 	m_PCIeExternal (PCIE_BUS_NVME, pInterrupt),
	m_Allocator (CMemorySystem::GetCoherentPage (COHERENT_SLOT_NVME),
		     CMemorySystem::GetCoherentPage (COHERENT_SLOT_NVME + 1)),
#ifdef NO_BUSY_WAIT
	m_pInterrupt(pInterrupt),
	m_bIRQConnected(false),
#endif
	m_ulOffset(0),
	m_pPartitionManager(nullptr)
{
	memset(&m_AdminQueue, 0, sizeof m_AdminQueue);
	m_AdminQueue.pName = "Admin";
	m_AdminQueue.usID = AQID;
	m_AdminQueue.nEntries = NVME_ADMIN_QUEUE_ENTRIES;

	memset(&m_IoQueue, 0, sizeof m_IoQueue);
	m_IoQueue.pName = "I/O";
	m_IoQueue.usID = IOQID;
	m_IoQueue.nEntries = NVME_IO_QUEUE_ENTRIES;
}

CNVMeDevice::~CNVMeDevice(void)
{
	delete m_pPartitionManager;
	m_pPartitionManager = nullptr;

#ifdef NO_BUSY_WAIT
	if (m_bIRQConnected)
	{
		MmioWrite32(NVME_REG_INTMS, NVME_REG_INTM_ALL_VECTORS);

		assert(m_pInterrupt);
		m_pInterrupt->DisconnectIRQ(ARM_IRQ_PCIE_EXT_HOST_INTA);

		m_bIRQConnected = false;
	}
#endif

        // Reset controller
        MmioWrite32(NVME_REG_CC, MmioRead32(NVME_REG_CC) & ~NVME_REG_CC_EN);
        WaitReady(false);

	if (m_AdminQueue.pSqVirt)
	{
		m_Allocator.Free(m_AdminQueue.pSqVirt);
		m_AdminQueue.pSqVirt = nullptr;
	}

	if (m_AdminQueue.pCqVirt)
	{
		m_Allocator.Free(m_AdminQueue.pCqVirt);
		m_AdminQueue.pCqVirt = nullptr;
	}

	if (m_IoQueue.pSqVirt)
	{
		m_Allocator.Free(m_IoQueue.pSqVirt);
		m_IoQueue.pSqVirt = nullptr;
	}

	if (m_IoQueue.pCqVirt)
	{
		m_Allocator.Free(m_IoQueue.pCqVirt);
		m_IoQueue.pCqVirt = nullptr;
	}
}

bool CNVMeDevice::Initialize(void)
{
	if (!m_PCIeExternal.Initialize())
	{
		LOGERR ("Cannot init external PCIe");

		return false;
	}

	if (!m_PCIeExternal.EnableDevice (PCIE_CLASS_CODE, PCIE_SLOT, PCIE_FUNC))
	{
		LOGERR ("Cannot enable PCIe device");

		return false;
	}

	// Check controller version
	m_nVersion = MmioRead32(NVME_REG_VER);
	u32 nMjr = (m_nVersion & NVME_REG_VER_MJR__MASK) >> NVME_REG_VER_MJR__SHIFT;
	u32 nMnr = (m_nVersion & NVME_REG_VER_MNR__MASK) >> NVME_REG_VER_MNR__SHIFT;
#ifdef NVME_DEBUG
	u32 nTer = (m_nVersion & NVME_REG_VER_TER__MASK) >> NVME_REG_VER_TER__SHIFT;
#endif

	if (   nMjr != 1
	    || (nMnr != 3 && nMnr != 4))
	{
		LOGERR("NVMe version not supported (0x%X)", m_nVersion);

		return false;
	}

	// Check capabilities
	m_ulCaps = MmioRead64(NVME_REG_CAP);

#ifdef NVME_DEBUG
	LOGDBG("NVMe controller found (ver %u.%u.%u)", nMjr, nMnr, nTer);

	LOGDBG("Capabilities are 0x%lX", m_ulCaps);
#endif

	m_nDoorbellStride = DOORBELL_STRIDE(   (m_ulCaps & NVME_REG_CAP_DSTRD__MASK)
					    << NVME_REG_CAP_DSTRD__SHIFT);

	m_nTimeoutHZ = MSEC2HZ(((m_ulCaps & NVME_REG_CAP_TO__MASK) >> NVME_REG_CAP_TO__SHIFT) * 500);
	if (!m_nTimeoutHZ)
	{
		LOGDBG("Timeout adjusted");

		m_nTimeoutHZ = 5 * HZ;
	}

        // Reset controller
        MmioWrite32(NVME_REG_CC, MmioRead32(NVME_REG_CC) & ~NVME_REG_CC_EN);
        if (!WaitReady(false))
	{
		LOGERR("Cannot reset controller");

		return false;
	}

#ifdef NO_BUSY_WAIT
	// Connect IRQ
	assert(!m_bIRQConnected);
	m_bIRQConnected = true;

	MmioWrite32(NVME_REG_INTMS, NVME_REG_INTM_ALL_VECTORS);

	assert(m_pInterrupt);
	m_pInterrupt->ConnectIRQ (ARM_IRQ_PCIE_EXT_HOST_INTA, InterruptHandler, this);
#endif

	// Create admin queues
	u32 nRet = CreateAdminQueues();
	if (nRet != NVME_STATUS_OK)
	{
		LOGERR("Cannot create admin queues");

		return false;
	}

        // Choose SQ/CQ entry sizes and enable controller
        u32 nCC = MmioRead32(NVME_REG_CC);
	nCC &= ~(NVME_REG_CC_IOCQES__MASK | NVME_REG_CC_IOCQES__MASK);
	nCC |=   NVME_REG_CC_IOSQES_64B << NVME_REG_CC_IOSQES__SHIFT
	       | NVME_REG_CC_IOCQES_16B << NVME_REG_CC_IOCQES__SHIFT
	       | NVME_REG_CC_EN;
        MmioWrite32(NVME_REG_CC, nCC);

	if (!WaitReady(true))
	{
		LOGERR("Cannot enable controller");

		return false;
	}

	// Create a single I/O queue
	nRet = CreateIoQueue(IOQID, NVME_IO_QUEUE_ENTRIES);
	if (nRet != NVME_STATUS_OK)
	{
		LOGERR("Cannot create I/O queue");

		return false;
	}

	// Identify namespace and controller
	char ModelNumber[40+1] = "";

	u8 *pIdBuf = static_cast<u8 *> (m_Allocator.Allocate(4096, NVME_PAGE_SIZE));
	if (!pIdBuf)
	{
		LOGERR("Allocation failed");

		return false;
	}

	// CNS is 0 (Namespace) or 1 (Controller)
	for (unsigned nCNS = 0; nCNS <= 1; nCNS++)
	{
		nRet = Identify(nCNS, pIdBuf, !nCNS ? NSID : 0);
		if (nRet != NVME_STATUS_OK)
		{
			LOGERR("Identify failed (cns %u, err %d)", nCNS, nRet);

			return false;
		}

		if (!nCNS)
		{
			// Namespace
			u8 uchFLBAS = pIdBuf[26];
			u32 nLBAFormat = *(reinterpret_cast<u32 *>(&pIdBuf[128]) + (uchFLBAS & 0xF));
			u8 uchLBADS = nLBAFormat >> 16 & 0xFF;
			unsigned nLBASize = 1 << uchLBADS;

			if (nLBASize != NVME_LBA_SIZE)
			{
				LOGERR("LBA size not supported (%u)", nLBASize);

				m_Allocator.Free(pIdBuf);

				return false;
			}

			m_ulNamespaceSize = *(u64 *) &pIdBuf[0] * nLBASize;

			u16 usMS = nLBAFormat & 0xFFFF;		// Check Metadata size field
			if (usMS)
			{
				LOGERR("Metadata not supported (%u)", (unsigned) usMS);

				m_Allocator.Free(pIdBuf);

				return false;
			}
		}
		else
		{
			// Controller
			memcpy (ModelNumber, &pIdBuf[24], sizeof ModelNumber-1);
			ModelNumber[sizeof ModelNumber-1] = '\0';
		}
	}

	m_Allocator.Free(pIdBuf);

	LOGNOTE("%luGB NVMe Model %s", m_ulNamespaceSize / GIGABYTE, ModelNumber);

	// Create partion devices and device names
	assert(m_pPartitionManager == 0);
	m_pPartitionManager = new CPartitionManager(this, DeviceName);
	assert(m_pPartitionManager != 0);
	if (!m_pPartitionManager->Initialize())
	{
		return false;
	}

	CDeviceNameService::Get ()->AddDevice (DeviceName, this, TRUE);

	return true;
}

int CNVMeDevice::Read (void *pBuffer, size_t ulCount)
{
#ifdef NVME_DEBUG
	LOGDBG("Read(%p, %lu)", pBuffer, ulCount);
#endif

	assert (pBuffer);

	if (m_ulOffset & (NVME_LBA_SIZE-1))
	{
		return NVME_STATUS_ERROR_BAD_PARAM;
	}
        unsigned nLBA = m_ulOffset / NVME_LBA_SIZE;

	if (!ulCount || (ulCount & (NVME_LBA_SIZE-1)))
	{
		return NVME_STATUS_ERROR_BAD_PARAM;
	}

	// Use temporary DMA buffer, if pBuffer is not cache aligned
	void *pTransferBuffer = pBuffer;
	u8 *pDMABuffer = nullptr;
	if (!IS_CACHE_ALIGNED(pBuffer, ulCount))
	{
		pDMABuffer = new u8[ulCount];
		if (!pDMABuffer)
		{
			return NVME_STATUS_ERROR_NO_RESOURCE;
		}

		pTransferBuffer = pDMABuffer;
	}

	InvalidateDataCacheRange (reinterpret_cast<uintptr>(pTransferBuffer), ulCount);

	int nRet = IoPassThrough(NSID, nLBA, ulCount / NVME_LBA_SIZE, pTransferBuffer, false);
        if (nRet != NVME_STATUS_OK)
	{
		delete [] pDMABuffer;

		return nRet;
        }

	InvalidateDataCacheRange (reinterpret_cast<uintptr>(pTransferBuffer), ulCount);

	if (pDMABuffer)
	{
		memcpy (pBuffer, pDMABuffer, ulCount);

		delete [] pDMABuffer;
	}

        return ulCount;
}

int CNVMeDevice::Write (const void *pBuffer, size_t ulCount)
{
#ifdef NVME_DEBUG
	LOGDBG("Write(%p, %lu)", pBuffer, ulCount);
#endif

	assert (pBuffer);

	if (m_ulOffset & (NVME_LBA_SIZE-1))
	{
		return NVME_STATUS_ERROR_BAD_PARAM;
	}
        unsigned nLBA = m_ulOffset / NVME_LBA_SIZE;

	if (!ulCount || (ulCount & (NVME_LBA_SIZE-1)))
	{
		return NVME_STATUS_ERROR_BAD_PARAM;
	}

#ifdef NVME_READ_ONLY
	return NVME_STATUS_ERROR_READ_ONLY;
#endif

	// Use temporary DMA buffer, if pBuffer is not cache aligned
	void *pTransferBuffer = const_cast<void *> (pBuffer);
	u8 *pDMABuffer = nullptr;
	if (!IS_CACHE_ALIGNED(pBuffer, ulCount))
	{
		pDMABuffer = new u8[ulCount];
		if (!pDMABuffer)
		{
			return NVME_STATUS_ERROR_NO_RESOURCE;
		}

		memcpy (pDMABuffer, pBuffer, ulCount);

		pTransferBuffer = pDMABuffer;
	}

	CleanDataCacheRange (reinterpret_cast<uintptr>(pTransferBuffer), ulCount);

	int nRet = IoPassThrough(NSID, nLBA, ulCount / NVME_LBA_SIZE, pTransferBuffer, true);

	delete [] pDMABuffer;

        if (nRet != NVME_STATUS_OK)
	{

		return nRet;
        }

	return ulCount;
}

u64 CNVMeDevice::Seek (u64 ulOffset)
{
#ifdef NVME_DEBUG
	LOGDBG("Seek(%lu)", ulOffset);
#endif

	m_ulOffset = ulOffset;

	return ulOffset;
}

u64 CNVMeDevice::GetSize (void) const
{
	return m_ulNamespaceSize;
}

int CNVMeDevice::IOCtl (unsigned long ulCmd, void *pData)
{
#ifdef NVME_DEBUG
	LOGDBG("IOCtl(0x%lX)", ulCmd);
#endif

	if (ulCmd == DEVICE_IOCTL_SYNC)
	{
		return Flush(NSID);
	}

	return NVME_STATUS_ERROR_BAD_PARAM;
}

int CNVMeDevice::CreateAdminQueues(void)
{
	// Allocate space for queues. ASQ is submission queue (array of commands)
	size_t uSqSize = sizeof(TNVMeCommand) * NVME_ADMIN_QUEUE_ENTRIES;
	size_t uCqSize = sizeof(TNVMeCompletion) * NVME_ADMIN_QUEUE_ENTRIES;

	void *pSq = m_Allocator.Allocate(uSqSize, NVME_PAGE_SIZE);
	if (!pSq) return NVME_STATUS_ERROR_NO_RESOURCE;
	void *pCq = m_Allocator.Allocate(uCqSize, NVME_PAGE_SIZE);
	if (!pCq) return NVME_STATUS_ERROR_NO_RESOURCE;

	memset(pSq, 0, uSqSize);
	memset(pCq, 0, uCqSize);

	m_AdminQueue.pSqVirt = pSq;
	m_AdminQueue.pCqVirt = pCq;
	m_AdminQueue.nSqPhys = PhysicalOf(pSq);
	m_AdminQueue.nCqPhys = PhysicalOf(pCq);

	u32 uAqa = ((NVME_ADMIN_QUEUE_ENTRIES - 1) << 16) | (NVME_ADMIN_QUEUE_ENTRIES - 1);
	MmioWrite32(NVME_REG_AQA, uAqa);
	MmioWrite64(NVME_REG_ASQ, m_AdminQueue.nSqPhys);
	MmioWrite64(NVME_REG_ACQ, m_AdminQueue.nCqPhys);

	m_AdminQueue.nSqTail = 0;
	m_AdminQueue.nCqHead = 0;
	m_AdminQueue.bCqPhase = true;

	return NVME_STATUS_OK;
}

int CNVMeDevice::CreateIoQueue(u16 uQueueId, u16 uEntries)
{
	// For I/O queues, we need to send Admin Create I/O Completion Queue and
	// Create Submission Queue commands. We allocate SQ/CQ memory and then use
	// AdminCommand() to create queues.
	size_t uSqSize = sizeof(TNVMeCommand) * uEntries;
	size_t uCqSize = sizeof(TNVMeCompletion) * uEntries;

	void *pSq = m_Allocator.Allocate(uSqSize, NVME_PAGE_SIZE);
	if (!pSq) return NVME_STATUS_ERROR_NO_RESOURCE;
	void *pCq = m_Allocator.Allocate(uCqSize, NVME_PAGE_SIZE);
	if (!pCq) return NVME_STATUS_ERROR_NO_RESOURCE;

	memset(pSq, 0, uSqSize);
	memset(pCq, 0, uCqSize);

	m_IoQueue.pSqVirt = pSq;
	m_IoQueue.pCqVirt = pCq;
	m_IoQueue.nSqPhys = PhysicalOf(pSq);
	m_IoQueue.nCqPhys = PhysicalOf(pCq);

	// Build Create CQ
	u32 uCdw10 = (uQueueId & 0xffff) | ((uEntries - 1) << 16);
	// cdw11: PC=1(phys contig) | IEN=1 | PRIO=0 | IRQ vector=0
	u32 uCdw11 = BIT(0) | BIT(1) | 0 << 16;
	// Data pointer: PRP1 = CQ physical base, PRP2 = 0
	u32 nRet = AdminCommand(NVME_ADMIN_OPC_CREATE_IO_CQ, 0, uCdw10, uCdw11, m_IoQueue.nCqPhys);
	if (nRet != NVME_STATUS_OK) return nRet;

	uCdw10 = (uQueueId & 0xffff) | ((uEntries - 1) << 16);
	// cdw11: CQid << 16, PC=1
	uCdw11 = (static_cast<u32>(uQueueId) << 16) | 1;
	nRet = AdminCommand(NVME_ADMIN_OPC_CREATE_IO_SQ, 0, uCdw10, uCdw11, m_IoQueue.nSqPhys);
	if (nRet != NVME_STATUS_OK) return nRet;

	m_IoQueue.nSqTail = 0;
	m_IoQueue.nCqHead = 0;
	m_IoQueue.bCqPhase = true;

	return NVME_STATUS_OK;
}

int CNVMeDevice::Flush(u32 nNsId)
{
	return SubmitCommand(&m_IoQueue, NVME_IO_OPC_FLUSH, nNsId, 0, 0, 0, 0, 0);
}

int CNVMeDevice::Identify(u32 uCns, void *pOutBuf, u32 uNsId)
{
	// Ensure buffer is physically contiguous and acquire physical address
	u64 uPhys = PhysicalOf(pOutBuf);

	// Build Admin Identify - CNS selects controller or namespace
	u32 uCdw10 = uCns;

	return AdminCommand(NVME_ADMIN_OPC_IDENTIFY, uNsId, uCdw10, 0, uPhys);
}

int CNVMeDevice::IoPassThrough(u32 nNsId, u64 nLba, u32 nBlocks, void *pBuffer, bool bIsWrite)
{
	assert (pBuffer);
	assert (nBlocks);

	CNVMePRP PrpBuilder(&m_Allocator);
	if (!PrpBuilder.BuildForBuffer(pBuffer, static_cast<size_t>(nBlocks) * NVME_LBA_SIZE))
	{
		return NVME_STATUS_ERROR_NO_RESOURCE;
	}

	return SubmitCommand(&m_IoQueue,
			     bIsWrite ? NVME_IO_OPC_WRITE : NVME_IO_OPC_READ,
			     nNsId,
			     static_cast<u32>(nLba & 0xffffffff),
			     static_cast<u32>((nLba >> 32) & 0xffffffff),
			     nBlocks - 1,
			     PrpBuilder.Prp1(),
			     PrpBuilder.Prp2());
}

int CNVMeDevice::AdminCommand(u8 uchOpcode, u32 nNsId, u32 uCdw10, u32 uCdw11, u64 ulDataPhysAddr)
{
	assert(ulDataPhysAddr);

	return SubmitCommand(&m_AdminQueue, uchOpcode, nNsId, uCdw10, uCdw11, 0, ulDataPhysAddr, 0);
}

int CNVMeDevice::SubmitCommand(TQueue *pQueue, u8 uchOpcode, u32 nNsId,
			       u32 nCdw10, u32 nCdw11, u32 nCdw12,
			       uintptr ulPrp1, uintptr ulPrp2)
{
	assert (pQueue);

#ifdef NVME_DEBUG
	LOGDBG("%s command (opcode 0x%02X, cdw 0x%X 0x%X 0x%X)",
	       pQueue->pName, uchOpcode, nCdw10, nCdw11, nCdw12);
#endif

	TNVMeCommand *pSq = reinterpret_cast<TNVMeCommand *>(pQueue->pSqVirt);

	u16 usCid = static_cast<u16>((pQueue->nSqTail) & 0xffff);
	TNVMeCommand *pCmd = &pSq[pQueue->nSqTail];
	memset(pCmd, 0, sizeof(TNVMeCommand));
	pCmd->opc = uchOpcode;
	pCmd->cid = usCid;
	pCmd->nsid = nNsId;
	pCmd->prp1 = ulPrp1;
	pCmd->prp2 = ulPrp2;
	pCmd->cdw10 = nCdw10;
	pCmd->cdw11 = nCdw11;
	pCmd->cdw12 = nCdw12;

#ifdef NO_BUSY_WAIT
	m_Event.Clear();

	MmioWrite32(NVME_REG_INTMC, NVME_REG_INTM_VECTOR0);
#endif

	// Doorbell write for submission queue
	pQueue->nSqTail = (pQueue->nSqTail + 1) % pQueue->nEntries;
	DataSyncBarrier();
	MmioWrite32(NVME_DOORBELL_SQ_OFFSET(pQueue->usID), pQueue->nSqTail);

	return PollForCompletion(pQueue, usCid, POLL_TIMEOUT_HZ);
}

int CNVMeDevice::PollForCompletion(TQueue *pQueue, u16 usCid, unsigned nTimeoutHZ)
{
	assert (pQueue);
	assert (usCid < pQueue->nEntries);

	// Poll queue for matching CID
	TNVMeCompletion *pCq = reinterpret_cast<TNVMeCompletion *>(pQueue->pCqVirt);
	unsigned nStart = CTimer::Get()->GetTicks();

#ifdef NVME_DEBUG
	unsigned nStartClockTicks = CTimer::Get()->GetClockTicks();
#endif

#ifdef NO_BUSY_WAIT
	if (m_Event.WaitWithTimeout(1000000UL * nTimeoutHZ / HZ))
	{
#ifdef NVME_DEBUG
		LOGDBG("%s command timed out", pQueue->pName);
#endif

		return NVME_STATUS_ERROR_TIMEOUT;
	}
#endif

	while (true)
	{
		DataMemBarrier();

		TNVMeCompletion &ce = pCq[pQueue->nCqHead];
		u16 usStatus = ATOMIC_READ(&ce.status);

		if (   !!(usStatus & CQE_STATUS_PHASE_BIT) == pQueue->bCqPhase
		    && ATOMIC_READ(&ce.cid) == usCid
		    && ATOMIC_READ(&ce.sqid) == pQueue->usID)
		{
			// Advance head
			pQueue->nCqHead = (pQueue->nCqHead + 1) % pQueue->nEntries;
			if (pQueue->nCqHead == 0) pQueue->bCqPhase = !pQueue->bCqPhase;
			DataSyncBarrier ();
			MmioWrite32(NVME_DOORBELL_CQ_OFFSET(pQueue->usID), pQueue->nCqHead);

			unsigned nSCT = (usStatus & CQE_STATUS_SCT__MASK) >> CQE_STATUS_SCT__SHIFT;
			unsigned nSC = (usStatus & CQE_STATUS_SC__MASK) >> CQE_STATUS_SC__SHIFT;
			if (nSCT || nSC)
			{
				LOGDBG("%s command failed (sct %u, sc 0x%X)",
				       pQueue->pName, nSCT, nSC);

				if (!nSCT && nSC == 0x80)
				{
					return NVME_STATUS_ERROR_LBA_RANGE;
				}

				return NVME_STATUS_ERROR_CONTROLLER;
			}

			break;
		}

#ifdef NO_BUSY_WAIT
#ifdef NVME_DEBUG
		else
		{
			LOGDBG("Interrupt without completion");
		}
#endif
#endif

		if (CTimer::Get()->GetTicks() - nStart > nTimeoutHZ)
		{
#ifdef NVME_DEBUG
			LOGDBG("%s command timed out", pQueue->pName);
#endif

			return NVME_STATUS_ERROR_TIMEOUT;
		}

#ifndef NO_BUSY_WAIT
		CTimer::Get()->usDelay(1);
#endif
	}

#ifdef NVME_DEBUG
	LOGDBG("%s command completed after %uus",
	       pQueue->pName, CTimer::Get()->GetClockTicks() - nStartClockTicks);
#endif

	return NVME_STATUS_OK;
}

bool CNVMeDevice::WaitReady(bool bOn)
{
	unsigned nStart = CTimer::Get()->GetTicks();

	while (true)
	{
		if (!!(MmioRead32(NVME_REG_CSTS) & NVME_REG_CSTS_RDY) == bOn)
		{
			return true;
		}

		if (CTimer::Get()->GetTicks() - nStart >= m_nTimeoutHZ)
		{
			break;
		}

#ifdef NO_BUSY_WAIT
		CScheduler::Get()->MsSleep(1);
#else
		CTimer::Get()->MsDelay(1);
#endif
	}

#ifdef NVME_DEBUG
	LOGDBG("Timeout");
#endif

	return false;
}

void CNVMeDevice::DumpStatus(void)
{
	for (u32 nOffset = 0; nOffset <= 0x3F; nOffset += 4)
	{
		LOGDBG("%04X: %08X", nOffset, MmioRead32 (nOffset));
	}

	m_PCIeExternal.DumpStatus (PCIE_SLOT, PCIE_FUNC);

	LOGDBG("%lu bytes shared memory free", m_Allocator.GetFreeSpace ());
}

#ifdef NO_BUSY_WAIT

void CNVMeDevice::InterruptHandler (void *pParam)
{
	CNVMeDevice *pThis = static_cast<CNVMeDevice *> (pParam);
	assert (pThis);

	MmioWrite32(NVME_REG_INTMS, NVME_REG_INTM_VECTOR0);

#ifdef NVME_DEBUG
	//LOGDBG("IRQ");
#endif

	pThis->m_Event.Set();
}

#endif
