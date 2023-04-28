//
// zxutil.cpp
//
#include <zxutil/zxutil.h>


/* Definitions */
#define RANDOM_SEED_OFFSET        0x7FB0C16A      // A little more secure
#define RANDOM_NUM_OFFSET         0xB782D0A2

#define random_32() (*--random_32_p ? *random_32_p : cycle_random_32())


/* Internal Function Prototypes */
static unsigned long cycle_random_32( void );
static void seed_random_32( unsigned long );

/* Internal Variables */
static int seeded = 0;
const unsigned long *random_32_p;

/**
 * Seed the PNRG
 */
void rand_seed(unsigned long seed) {
  seed_random_32(seed);
  seeded = 1;
}

/**
 * Get a pseudo-random number (32-bit)
 */
unsigned long rand_32() {
  if (!seeded) rand_seed(0);

  return random_32();
}

/************************************************** *********/
/* The method employed is based on a lagged fibonacci PRNG */
/* given in Knuth's "Graph Base" book. */
/************************************************** *********/

#define VALUES_N (55u)
#define VALUES_D (24u)
/* Other reasonable values: 33,20 17,11 9,5 */


#define VALUES_INDEX_LIMIT (VALUES_N + 1)

static unsigned long values[VALUES_INDEX_LIMIT];
const unsigned long *random_32_p = values + VALUES_INDEX_LIMIT;
static const unsigned long mask32 = 0xffffffffUL;


void seed_random_32( unsigned long seed_value ){
  unsigned i;

  values[1] = seed_value & mask32;
  for( i = 2; i < VALUES_INDEX_LIMIT; i++ ) values[i] = 0;

  for( i = 0; i < VALUES_INDEX_LIMIT * 2; i++ ){
    random_32_p = &values[0];
    (void) cycle_random_32();
  }
}

unsigned long cycle_random_32(){
  unsigned i;

  if( random_32_p == &values[0] ){
    for( i = 1; i + VALUES_D < VALUES_INDEX_LIMIT; i++ ){
      values[i] -= values[ i+VALUES_D ];
      values[i] &= mask32;
    }
    for( i = i; i < VALUES_INDEX_LIMIT; i++ ){
      values[i] -= values[ i - (VALUES_N - VALUES_D) ];
      values[i] &= mask32;
    }

    for( i = 1; i < VALUES_INDEX_LIMIT; i++ ){
      values[i] ^= (mask32 ^ values[i-1]) >> 1;
    }

    random_32_p = &values[VALUES_INDEX_LIMIT - 1];

  }
  else if( random_32_p == &values[VALUES_INDEX_LIMIT - 1] ){
    seed_random_32( 0 );
  }

  return *random_32_p;
}

/***** End of Random number generator *****/