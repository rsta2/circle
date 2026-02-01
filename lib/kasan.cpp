/*
 * Authors: Stephan Mühlstrasser and Rene Stange
 * KASan implementation for Circle.
 * Based on the code from https://github.com/androidoffsec/baremetal_kasan
 *
 * Original copyright notice:
 *
 * Copyright 2024 Google LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <circle/memory.h>
#include <circle/memorymap.h>
#include <circle/util.h>
#include <circle/kasan.h>
#include <circle/heapallocator.h>
#include <circle/logger.h>
#include <circle/string.h>

typedef u8 uint8_t;
typedef char int8_t;

#define KASAN_SHADOW_SHIFT 3
#define KASAN_SHADOW_GRANULE_SIZE (1UL << KASAN_SHADOW_SHIFT)
#define KASAN_SHADOW_MASK (KASAN_SHADOW_GRANULE_SIZE - 1)

#define ASAN_SHADOW_UNPOISONED_MAGIC 0x00
#define ASAN_SHADOW_RESERVED_MAGIC 0xff
#define ASAN_SHADOW_GLOBAL_REDZONE_MAGIC 0xf9
#define ASAN_SHADOW_HEAP_HEAD_REDZONE_MAGIC 0xfa
#define ASAN_SHADOW_HEAP_TAIL_REDZONE_MAGIC 0xfb
#define ASAN_SHADOW_HEAP_FREE_MAGIC 0xfd

#define CALLER_PC ((uintptr)__builtin_return_address(0))

// The head redzone size must be equal to Circle's HEAP_BLOCK_ALIGN
// value. This is necessary to fulfil Circle's guarantee that memory
// allocated on the heap is suitably aligned for being used as a
// DMA buffer.
#define KASAN_HEAP_HEAD_REDZONE_SIZE HEAP_BLOCK_ALIGN
#define KASAN_HEAP_TAIL_REDZONE_SIZE 0x20

#define KASAN_MEM_TO_SHADOW(addr) \
    (((addr) >> KASAN_SHADOW_SHIFT) + MEM_SHADOW_START)
#define KASAN_SHADOW_TO_MEM(shadow) \
    (((shadow) - MEM_SHADOW_START) << KASAN_SHADOW_SHIFT)

void __kasan_printf (const char *pFormat, ...);

extern "C"
{
    // The real memory functions implemented in Circle, called from
    // the corresponding KASan wrappers. These are not instrumented
    // by KASan, as they are directly implemented in assembly
    // in util_fast.S.
    void *__kasan_memset (void *pBuffer, int nValue, size_t nLength);
    void *__kasan_memcpy (void *pDest, const void *pSrc, size_t nLength);
}

namespace
{
    uintptr g_shadow_mem_end;
    uintptr g_low_mem_end;
    uintptr g_high_mem_end;

    bool g_kasan_initialized = false;
};

void kasan_bug_report(uintptr addr, size_t size,
                      uintptr buggy_shadow_address, uint8_t is_write,
                      uintptr ip);

static inline uintptr get_poisoned_shadow_address(uintptr addr,
                                                        size_t size)
{
    uintptr addr_shadow_start = KASAN_MEM_TO_SHADOW(addr);
    uintptr addr_shadow_end = KASAN_MEM_TO_SHADOW(addr + size - 1) + 1;
    uintptr non_zero_shadow_addr = 0;

    for (uintptr i = 0; i < addr_shadow_end - addr_shadow_start; i++)
    {
        if (*(uint8_t *)(addr_shadow_start + i) != 0)
        {
            non_zero_shadow_addr = addr_shadow_start + i;
            break;
        }
    }

    if (non_zero_shadow_addr)
    {
        uintptr last_byte = addr + size - 1;
        int8_t *last_shadow_byte = (int8_t *)KASAN_MEM_TO_SHADOW(last_byte);

        // Non-zero bytes in shadow memory may indicate either:
        //  1) invalid memory access (0xff, 0xfa, ...)
        //  2) access to a 8-byte region which isn't entirely accessible, i.e. only
        //     n bytes can be read/written in the 8-byte region, where n < 8
        //     (in this case shadow byte encodes how much bytes in an 8-byte region
        //     are accessible).
        // Thus, if there is a non-zero shadow byte we need to check if it
        // corresponds to the last byte in the checked region:
        //   not last - OOB memory access
        //   last - check if we don't access beyond what's encoded in the shadow
        //          byte.
        if (non_zero_shadow_addr != (uintptr)last_shadow_byte ||
            ((int8_t)(last_byte & KASAN_SHADOW_MASK) >= *last_shadow_byte))
            return non_zero_shadow_addr;
    }

    return 0;
}

// Both `address` and `size` must be 8-byte aligned.
static void poison_shadow(uintptr address, size_t size, uint8_t value)
{
    uintptr shadow_start, shadow_end;
    size_t shadow_length = 0;

    shadow_start = KASAN_MEM_TO_SHADOW(address);
    shadow_end = KASAN_MEM_TO_SHADOW(address + size - 1) + 1;
    shadow_length = shadow_end - shadow_start;

    __kasan_memset((void *)shadow_start, value, shadow_length);
}

// `address` must be 8-byte aligned
static void unpoison_shadow(uintptr address, size_t size)
{
    poison_shadow(address, size & (~KASAN_SHADOW_MASK),
                  ASAN_SHADOW_UNPOISONED_MAGIC);

    if (size & KASAN_SHADOW_MASK)
    {
        uint8_t *shadow = (uint8_t *)KASAN_MEM_TO_SHADOW(address + size);
        *shadow = size & KASAN_SHADOW_MASK;
    }
}

static inline int kasan_check_memory(uintptr addr, size_t size,
                                     uint8_t write, uintptr pc)
{
    if (!g_kasan_initialized || size == 0)
        return 1;

    uintptr addr_end = addr + size - 1;
    if (   (MEM_SHADOW_START <= addr && addr_end < g_shadow_mem_end)    // shadow region
        || (g_low_mem_end <= addr && addr_end < GIGABYTE)               // GPU and I/O region
        || g_high_mem_end <= addr_end)                                  // behind high heap
        return 1;

    uintptr const buggy_shadow_address = get_poisoned_shadow_address(addr, size);
    if (buggy_shadow_address == 0)
        return 1;

    kasan_bug_report(addr, size, buggy_shadow_address, write, pc);
    return 0;
}

// Implement necessary routines for KASan sanitization of globals.

// See struct __asan_global definition at
// https://github.com/llvm-mirror/compiler-rt/blob/master/lib/asan/asan_interface_internal.h.
struct kasan_global_info
{
    // Starting address of the variable
    const void *start;
    // Variable size
    size_t size;
    // 32-bit aligned size of global including the redzone
    size_t size_with_redzone;
    // Symbol name
    const void *name;
    const void *module_name;
    unsigned long has_dynamic_init;
    void *location;
    unsigned int odr_indicator;
};

static void asan_register_global(struct kasan_global_info *global)
{
    unpoison_shadow((uintptr)global->start, global->size);

    size_t aligned_size = (global->size + KASAN_SHADOW_MASK) & ~KASAN_SHADOW_MASK;
    poison_shadow((uintptr)global->start + aligned_size,
                  global->size_with_redzone - aligned_size,
                  ASAN_SHADOW_GLOBAL_REDZONE_MAGIC);
}

extern "C"
{
    void __asan_register_globals(struct kasan_global_info *globals, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            asan_register_global(&globals[i]);
    }

    void __asan_unregister_globals(void *globals, size_t size) {}

    // Empty placeholder implementation to supress linker error for undefined symbol
    void __asan_handle_no_return(void) {}

    // KASan memcpy/memset hooks.

    void *memcpy(void *dst, const void *src, size_t size)
    {
        kasan_check_memory((uintptr)dst, size, /*is_write*/ true, CALLER_PC);
        kasan_check_memory((uintptr)src, size, /*is_write*/ false, CALLER_PC);

        return __kasan_memcpy(dst, src, size);
    }

    void *memset(void *buf, int c, size_t size)
    {
        kasan_check_memory((uintptr)buf, size, /*is_write*/ true, CALLER_PC);

        return __kasan_memset(buf, c, size);
    }
}

// Implement KASan heap management hooks.

struct KASAN_HEAP_HEADER
{
    unsigned int aligned_size;
};

struct kasan_allocate_params
{
    size_t aligned_size;
    size_t total_size;
};

static
kasan_allocate_params kasan_get_allocate_params(size_t nSize)
{
    size_t const aligned_size = (nSize + KASAN_SHADOW_MASK) & (~KASAN_SHADOW_MASK);
    size_t const total_size = aligned_size + KASAN_HEAP_HEAD_REDZONE_SIZE +
                              KASAN_HEAP_TAIL_REDZONE_SIZE;
    return {aligned_size, total_size};
}

static
void *kasan_shadow_allocated(char *ptr, size_t nSize, size_t aligned_size)
{
    KASAN_HEAP_HEADER * const kasan_heap_hdr = reinterpret_cast<KASAN_HEAP_HEADER *>(ptr);
    kasan_heap_hdr->aligned_size = aligned_size;

    unpoison_shadow((uintptr)(ptr + KASAN_HEAP_HEAD_REDZONE_SIZE), nSize);
    poison_shadow((uintptr)ptr, KASAN_HEAP_HEAD_REDZONE_SIZE,
                  ASAN_SHADOW_HEAP_HEAD_REDZONE_MAGIC);
    poison_shadow(
        (uintptr)(ptr + KASAN_HEAP_HEAD_REDZONE_SIZE + aligned_size),
        KASAN_HEAP_TAIL_REDZONE_SIZE, ASAN_SHADOW_HEAP_TAIL_REDZONE_MAGIC);

    return ptr + KASAN_HEAP_HEAD_REDZONE_SIZE;
}

void *KasanAllocateHook (CHeapAllocator &rHeapAllocator, size_t nSize)
{
    kasan_allocate_params const allocate_params = kasan_get_allocate_params(nSize);

    char *const ptr = static_cast<char *>(rHeapAllocator.DoAllocate(allocate_params.total_size));
    if (ptr == nullptr)
        return nullptr;

    return kasan_shadow_allocated(ptr, nSize, allocate_params.aligned_size);
}

void KasanFreeHook (CHeapAllocator &rHeapAllocator, void *pBlock)
{
    if (pBlock == nullptr)
        return;

    char *const ptr = static_cast<char *>(pBlock);

    struct KASAN_HEAP_HEADER *const kasan_heap_hdr =
        reinterpret_cast<KASAN_HEAP_HEADER *>(ptr - KASAN_HEAP_HEAD_REDZONE_SIZE);
    unsigned int const aligned_size = kasan_heap_hdr->aligned_size;

    rHeapAllocator.DoFree(kasan_heap_hdr);
    poison_shadow((uintptr)ptr, aligned_size, ASAN_SHADOW_HEAP_FREE_MAGIC);
}

void *KasanReAllocateHook (CHeapAllocator &rHeapAllocator, void *pBlock, ssize_t nSize)
{
    if (pBlock == nullptr)
    {
        return KasanAllocateHook(rHeapAllocator, nSize);
    }

    char *const old_ptr = static_cast<char *>(pBlock);

    KASAN_HEAP_HEADER *const kasan_heap_hdr =
        reinterpret_cast<struct KASAN_HEAP_HEADER *>(old_ptr - KASAN_HEAP_HEAD_REDZONE_SIZE);

    poison_shadow((uintptr)old_ptr, kasan_heap_hdr->aligned_size, ASAN_SHADOW_HEAP_FREE_MAGIC);

    kasan_allocate_params const allocate_params = kasan_get_allocate_params(nSize);

    char *const new_ptr = static_cast<char *>(rHeapAllocator.DoReAllocate(kasan_heap_hdr, allocate_params.total_size));
    if (new_ptr == nullptr)
        return nullptr;

    return kasan_shadow_allocated(new_ptr, nSize, allocate_params.aligned_size);;
}

// Implement KAsan error reporting routines.

static void kasan_print_16_bytes_no_bug(uintptr address)
{
    CString logger_message;
    logger_message.Format("0x%X:", address);

    for (int i = 0; i < 16; i++)
    {
        CString byte_str;
        byte_str.Format(" %02X", *(uint8_t *)(address + i));
        logger_message += byte_str;
    }

    CLogger::Get ()->Write ("kasan", LogError, "%s", logger_message.c_str());
}

static void kasan_print_16_bytes_with_bug(uintptr address,
                                          int buggy_offset)
{
    CString logger_message;
    logger_message.Format("0x%X:", address);
    for (int i = 0; i < buggy_offset; i++)
    {
        CString byte_str;
        byte_str.Format(" %02X", *(uint8_t *)(address + i));
        logger_message += byte_str;
    }
    {
        CString buggy_byte_str;
        buggy_byte_str.Format("[%02X]", *(uint8_t *)(address + buggy_offset));
        logger_message += buggy_byte_str;
    }
    if (buggy_offset < 15)
    {
        CString byte_str;
        byte_str.Format("%02X", *(uint8_t *)(address + buggy_offset + 1));
        logger_message += byte_str;
    }
    for (int i = buggy_offset + 2; i < 16; i++)
    {
        CString byte_str;
        byte_str.Format(" %02X", *(uint8_t *)(address + i));
        logger_message += byte_str;
    }
    CLogger::Get ()->Write ("kasan", LogError, "%s", logger_message.c_str());
}

static void kasan_print_shadow_memory(uintptr address, int range_before,
                                      int range_after)
{
    uintptr shadow_address = KASAN_MEM_TO_SHADOW(address);
    uintptr aligned_shadow = shadow_address & 0xfffffff0;
    int buggy_offset = shadow_address - aligned_shadow;

    CLogger::Get ()->Write ("kasan", LogError,
        "Shadow bytes around the buggy address 0x%X (shadow 0x%X):",
        address, shadow_address);

    for (int i = range_before; i > 0; i--)
    {
        kasan_print_16_bytes_no_bug(aligned_shadow - i * 16);
    }

    kasan_print_16_bytes_with_bug(aligned_shadow, buggy_offset);

    for (int i = 1; i <= range_after; i++)
    {
        kasan_print_16_bytes_no_bug(aligned_shadow + i * 16);
    }
}

void kasan_bug_report(uintptr addr, size_t size,
                      uintptr buggy_shadow_address, uint8_t is_write,
                      uintptr ip)
{
    uintptr const buggy_address = KASAN_SHADOW_TO_MEM(buggy_shadow_address);
    CLogger::Get ()->Write ("kasan", LogError,
        "===================================================");
    CLogger::Get ()->Write ("kasan", LogError,
        "Invalid memory access: address 0x%X, size 0x%X, is_write %d, ip 0x%X",
        addr, size, is_write, ip);

    kasan_print_shadow_memory(buggy_address, 3, 3);
}

void KasanInitialize (void)
{
    size_t const shadow_mem_size = CMemorySystem::GetShadowMemSize();

    g_shadow_mem_end = MEM_SHADOW_START + shadow_mem_size;
    g_low_mem_end = CMemorySystem::GetLowMemSize();
    g_high_mem_end = GIGABYTE + CMemorySystem::GetHighMemSize();

    // The whole shadow memory is initialized to zero (unpoisoned).
    // The instrumented code does only explicitly poison and unpoison
    // the red zones.
    __kasan_memset(reinterpret_cast<void *>(MEM_SHADOW_START), 0x00, shadow_mem_size);

    // Mark shadow memory region not accessible by the sanitized code.
    poison_shadow(MEM_SHADOW_START, shadow_mem_size, ASAN_SHADOW_RESERVED_MAGIC);

    g_kasan_initialized = true;
}

// Define KASan handlers used by the compiler instrumentation.
extern "C"
{
    void __asan_loadN_noabort(unsigned int addr, unsigned int size)
    {
        kasan_check_memory(addr, size, /*is_write*/ false, CALLER_PC);
    }

    void __asan_storeN_noabort(unsigned int addr, size_t size)
    {
        kasan_check_memory(addr, size, /*is_write*/ true, CALLER_PC);
    }

#define DEFINE_KASAN_LOAD_STORE_ROUTINES(size)                         \
    void __asan_load##size##_noabort(uintptr addr)               \
    {                                                                  \
        kasan_check_memory(addr, size, /*is_write*/ false, CALLER_PC); \
    }                                                                  \
    void __asan_store##size##_noabort(uintptr addr)              \
    {                                                                  \
        kasan_check_memory(addr, size, /*is_write*/ true, CALLER_PC);  \
    }

    DEFINE_KASAN_LOAD_STORE_ROUTINES(1)
    DEFINE_KASAN_LOAD_STORE_ROUTINES(2)
    DEFINE_KASAN_LOAD_STORE_ROUTINES(4)
    DEFINE_KASAN_LOAD_STORE_ROUTINES(8)
    DEFINE_KASAN_LOAD_STORE_ROUTINES(16)

// Local variable KASan instrumentation
#define DEFINE_KASAN_SET_SHADOW_ROUTINE(byte)              \
    void __asan_set_shadow_##byte(void *addr, size_t size) \
    {                                                      \
        __kasan_memset(addr, 0x##byte, size);                      \
    }

    DEFINE_KASAN_SET_SHADOW_ROUTINE(00) // addressable memory
    DEFINE_KASAN_SET_SHADOW_ROUTINE(f1) // stack left redzone
    DEFINE_KASAN_SET_SHADOW_ROUTINE(f2) // stack mid redzone
    DEFINE_KASAN_SET_SHADOW_ROUTINE(f3) // stack right redzone

    /* Additions in Circle's kasan library because of unresolved externals */
    DEFINE_KASAN_SET_SHADOW_ROUTINE(f5) // guessed from current LLVM code in asan_poisoning.cc
    DEFINE_KASAN_SET_SHADOW_ROUTINE(f8) // guessed from current LLVM code in asan_poisoning.cc

    /* Further functions that appear with non-optimized code */
    void __asan_before_dynamic_init(const char *module_name)
    {
    }
    void __asan_after_dynamic_init()
    {
    }
}
