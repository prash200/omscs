#include <omp.h>
#include "gtmp.h"

static unsigned int count;
static unsigned short sense = 0;
static unsigned int num_threads;
static const cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);

void gtmp_init(int num_threads)
{
  count = num_threads;
  num_threads = num_threads;
}

void gtmp_barrier()
{
  short *local_sense;
  posix_memalign((void**)&local_sense, cache_line_size, cache_line_size);
  *local_sense = sense ^ 0x1;

  if (__sync_fetch_and_sub(&count, 1) == 1)
  {
    count = num_threads;
    sense = *local_sense;
  }
  else
  {
    while (sense != *local_sense);
  }

  free((void*)local_sense);
}

void gtmp_finalize()
{
}