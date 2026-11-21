// Stands in for the barriers, which the memory system needs only with the MMU
// enabled.
#pragma once

#define DataSyncBarrier()		((void) 0)
#define InstructionSyncBarrier()	((void) 0)
