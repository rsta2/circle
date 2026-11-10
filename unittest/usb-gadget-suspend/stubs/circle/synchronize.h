#pragma once
#include <circle/types.h>
#define IRQ_LEVEL 1
#define DMA_BUFFER(type, name, count) alignas (64) type name[count]
void EnterCritical (unsigned);
void LeaveCritical ();
inline void CleanAndInvalidateDataCacheRange (uintptr, size_t)
{
}

inline void PeripheralEntry ()
{
}
inline void PeripheralExit ()
{
}
