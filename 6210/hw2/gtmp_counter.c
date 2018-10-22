#include <stdlib.h>
#include <omp.h>
#include "gtmp.h"

static unsigned int count;
static unsigned int num_threads;
static short *sense;

void gtmp_init(int n_threads)
{
  count = num_threads = n_threads;
  // To avoid false sharing, allocate a block equal to the chache line size.
  posix_memalign((void**)&sense, LEVEL1_DCACHE_LINESIZE, LEVEL1_DCACHE_LINESIZE);
  *sense = 0;
}

void gtmp_barrier()
{
  short *local_sense;
  // To avoid false sharing, allocate a block equal to the chache line size.
  posix_memalign((void**)&local_sense, LEVEL1_DCACHE_LINESIZE, LEVEL1_DCACHE_LINESIZE);
  *local_sense = *sense ^ 0x1;

  if (__sync_fetch_and_sub(&count, 1) == 1)
  {
    count = num_threads;
    *sense = *local_sense;
  }
  else
  {
    while (*sense != *local_sense);
  }

  free((void*)local_sense);
}

void gtmp_finalize()
{
  free((void*)sense);
}