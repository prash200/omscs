#include <stdlib.h>
#include <omp.h>
#include "gtmp.h"

typedef struct _treenode_t
{
  unsigned short sense;
  unsigned short have_child;
  unsigned short child_not_ready;
} treenode_t;

static int count;
static treenode_t *nodes;

void gtmp_init(int n_threads)
{
  count = n_threads;
  nodes = (treenode_t*)malloc(count * sizeof(treenode_t));

  for (int i = 0; i < count; i++)
  {
    if (4 * i + 4 + 1 < count)
    {
      nodes[i].have_child = 15;
    }
    else if (4 * i + 3 + 1 < count)
    {
      nodes[i].have_child = 7;
    }
    else if (4 * i + 2 + 1 < count)
    {
      nodes[i].have_child = 3;
    }
    else if (4 * i + 1 + 1 < count)
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
  int thread_num = omp_get_thread_num();

  while (nodes[thread_num].child_not_ready != 0);

  nodes[thread_num].child_not_ready = have_child;

  nodes[(thread_num - 1) / 4].child_not_ready |= (1 << (thread_num % 4));

  unsigned short local_sense = nodes[thread_num].sense ^ 0x1;
  if (thread_num != 0)
  {
    while (nodes[thread_num].sense != local_sense);
  }

  if (thread_num * 2 + 1 < count)
  {
    nodes[thread_num * 2 + 1].sense = local_sense;
  }

  if (thread_num * 2 + 2 < count)
  {
    nodes[thread_num * 2 + 2].sense = local_sense;
  }
}

void gtmp_finalize()
{
  free(nodes);
}