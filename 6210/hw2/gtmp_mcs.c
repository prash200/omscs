#include <stdlib.h>
#include <omp.h>
#include "gtmp.h"

typedef struct _node_t
{
  unsigned short sense;
  unsigned short have_child;
  unsigned short child_not_ready;
} node_t;

static int count;
static node_t **nodes;

void gtmp_init(int n_threads)
{
  count = n_threads;

  nodes = (node_t**)malloc(count * sizeof(node_t*));
  for (int i = 0; i < count; i++)
  {
    // To avoid false sharing, allocate a block equal to the chache line size.
    posix_memalign((void**)&(nodes[i]), LEVEL1_DCACHE_LINESIZE, LEVEL1_DCACHE_LINESIZE);

    if (4 * i + 4 < count)
    {
      nodes[i].have_child = 15;
    }
    else if (4 * i + 3 < count)
    {
      nodes[i].have_child = 7;
    }
    else if (4 * i + 2 < count)
    {
      nodes[i].have_child = 3;
    }
    else if (4 * i + 1 < count)
    {
      nodes[i].have_child = 1;
    }
    else
    {
      nodes[i].have_child = 0;
    }

    nodes[i].child_not_ready = nodes[i].have_child;

    nodes[i].sense = 0;
  }
}

void gtmp_barrier()
{
  while (nodes[omp_get_thread_num()].child_not_ready != 0);

  nodes[omp_get_thread_num()].child_not_ready = nodes[omp_get_thread_num()].have_child;

  unsigned short *local_sense;
  // To avoid false sharing, allocate a block equal to the chache line size.
  posix_memalign((void**)&local_sense, LEVEL1_DCACHE_LINESIZE, LEVEL1_DCACHE_LINESIZE);
  *local_sense = nodes[omp_get_thread_num()].sense ^ 0x1;

  if (omp_get_thread_num() != 0)
  {
    nodes[(omp_get_thread_num() - 1) / 4].child_not_ready &= ~(1 << ((omp_get_thread_num() + 3) % 4));

    while (nodes[omp_get_thread_num()].sense != *local_sense);
  }
  else
  {
    nodes[0].sense = *local_sense;
  }

  if ((omp_get_thread_num() * 2 + 1) < count)
  {
    nodes[omp_get_thread_num() * 2 + 1].sense = *local_sense;
  }

  if ((omp_get_thread_num() * 2 + 2) < count)
  {
    nodes[omp_get_thread_num() * 2 + 2].sense = *local_sense;
  }

  free((void*)local_sense);
}

void gtmp_finalize()
{
  for (int i = 0; i < count; i++)
  {
    free((void*)nodes[i]);
  }

  free((void*)nodes);
}