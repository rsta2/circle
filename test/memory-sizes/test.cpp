// Checks the sizes CMemorySystem takes from the board's RAM size, with the
// real lib/memory64.cpp as run.py prepares it. For each RAM size from 1 to
// 16 GB, the high heap is the RAM above 1 GB, limited to the high memory
// region. With KASAN_SUPPORTED, the shadow memory is an eighth of the RAM the
// board reports early, limited to 4 GB, and the low heap starts behind it.
#include <circle/memory.h>
#include <circle/bcmpropertytags.h>
#include <circle/machineinfo.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

THeapSetup s_HeapSetups[2];
unsigned s_nHeapSetups = 0;
unsigned s_nRAMSize = 0;

// The low memory the property tag reports in this test.
u32 s_nARMMemorySize = 948 * MEGABYTE;

void assertion_failed (const char *pExpr, const char *pFile, unsigned nLine)
{
	printf ("assertion failed: %s at %s:%u\n", pExpr, pFile, nLine);
	abort ();
}

void CMemorySystem::Destructor (void)
{
	if (s_pThis == this)
	{
		s_pThis = 0;
	}
}

void CMemorySystem::EnableMMU (void)
{
}

static const THeapSetup *FindHeapSetup (const char *pName)
{
	for (unsigned i = 0; i < s_nHeapSetups; i++)
	{
		if (strcmp (s_HeapSetups[i].pName, pName) == 0)
		{
			return &s_HeapSetups[i];
		}
	}

	return 0;
}

static unsigned s_nFailures = 0;

static void Check (unsigned nRAMSize, const char *pWhat, u64 nGot, u64 nExpected)
{
	printf ("%s: %5u MB RAM: %s is %llu MB, expected %llu MB\n", nGot == nExpected ? "pass" : "FAIL",
		nRAMSize, pWhat, (unsigned long long) (nGot / MEGABYTE),
		(unsigned long long) (nExpected / MEGABYTE));
	if (nGot != nExpected)
	{
		s_nFailures++;
	}
}

// The expected sizes in MB for each RAM size, per build. With KASAN_SUPPORTED,
// the RAM is what GetRAMSizeEarly () reports, at most 4 GB, for the high memory
// as well. NOT_CHECKED marks a size this build does not check.
#define NOT_CHECKED	(~0U)

struct TExpected
{
	unsigned nRAMSize;
	unsigned nShadowMem;
	unsigned nHighMem;
};

static const TExpected Expected[] =
{
#if RASPPI >= 5 && !defined (KASAN_SUPPORTED)
	{ 1024, NOT_CHECKED,    0},
	{ 2048, NOT_CHECKED, 1024},
	{ 4096, NOT_CHECKED, 3072},
	{ 8192, NOT_CHECKED, 7168},
	{16384, NOT_CHECKED, 7168},
#elif RASPPI == 4 && defined (KASAN_SUPPORTED)
	{ 1024, 128,    0},
	{ 2048, 256, 1024},
	{ 4096, 512, 2048},
	{ 8192, 512, 2048},
	{16384, 512, 2048},
#elif RASPPI >= 5 && defined (KASAN_SUPPORTED)
	// GetRAMSizeEarly () returns 2048 here, so the high memory does not follow the RAM size
	{ 1024, 256, NOT_CHECKED},
	{ 2048, 256, NOT_CHECKED},
	{ 4096, 256, NOT_CHECKED},
	{ 8192, 256, NOT_CHECKED},
	{16384, 256, NOT_CHECKED},
#else
	#error No expected sizes for this build
#endif
};

int main (void)
{
	for (const TExpected &Sizes : Expected)
	{
		s_nRAMSize = Sizes.nRAMSize;
		s_nHeapSetups = 0;

		CMemorySystem Memory (FALSE);
		Memory.SetupHighMem ();

#ifdef KASAN_SUPPORTED
		u64 nShadow = (u64) Sizes.nShadowMem * MEGABYTE;
		Check (Sizes.nRAMSize, "the shadow memory", CMemorySystem::GetShadowMemSize (), nShadow);
		const THeapSetup *pLow = FindHeapSetup ("heaplow");
		Check (Sizes.nRAMSize, "the low heap's start", pLow != 0 ? pLow->nBase : 0, MEM_SHADOW_START + nShadow);
#endif
		if (Sizes.nHighMem != NOT_CHECKED)
		{
			u64 nHigh = (u64) Sizes.nHighMem * MEGABYTE;
			Check (Sizes.nRAMSize, "the high memory", CMemorySystem::GetHighMemSize (), nHigh);
			const THeapSetup *pHigh = FindHeapSetup ("heaphigh");
			Check (Sizes.nRAMSize, "the high heap", pHigh != 0 ? pHigh->nSize : 0, nHigh);
		}
	}

	return s_nFailures == 0 ? 0 : 1;
}
