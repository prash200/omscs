#include <stdlib.h>
#include <omp.h>
#include "gtmp.h"

typedef struct _treenode_t
{
  unsigned char parent_sense;
  unsigned char *parent_pointer;
  unsigned char *child_pointers[2];
  unsigned char have_child[4];
  unsigned char child_not_ready[4];
  unsigned char dummy;
} treenode_t;

static treenode_t *nodes;
static unsigned int vpid_next;
static unsigned int sense_init;
static int num_procs;

void gtmp_init(int num_threads)
{
  num_procs = num_threads;
  vpid_next = 0;
  sense_init = 1;
  nodes = (treenode_t*) malloc(num_procs * sizeof(treenode_t));

  for (int i = 0; i < num_procs; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      if (4 * i + j + 1 < num_procs)
      {
        nodes[i].have_child[j] = 1;
      }
      else
      {
        nodes[i].have_child[j] = 0;
      }

      nodes[i].child_not_ready[j] = nodes[i].have_child[j];
    }

    if (i != 0)
    {
      nodes[i].parent_pointer = &(nodes[(i-1)/4].child_not_ready[(i-1) % 4]);
    }
    else
    {
      nodes[i].parent_pointer = &(nodes[i].dummy);
    }

    if (2 * i + 1 < num_procs)
    {
      nodes[i].child_pointers[0] = &(nodes[2*i+1].parent_sense);
    }
    else
    {
      nodes[i].child_pointers[0] = &(nodes[i].dummy);
    }

    if (2 * i + 2 < num_procs)
    {
      nodes[i].child_pointers[1] = &(nodes[2*i+2].parent_sense);
    }
    else
    {
      nodes[i].child_pointers[1] = &(nodes[i].dummy);
    }

    nodes[i].parent_sense = 0;
  }
}

void gtmp_barrier()
{
  unsigned int vpid = __sync_fetch_and_add(&vpid_next, 1) % num_procs;
  unsigned char sense = sense_init;
  int i;

  while (nodes[vpid].child_not_ready[0] |
    nodes[vpid].child_not_ready[1] | 
    nodes[vpid].child_not_ready[2] |
    nodes[vpid].child_not_ready[3]);

  for (i = 0; i < 4; i++)
  {
    nodes[vpid].child_not_ready[i] = nodes[vpid].have_child[i];
  }

  *(nodes[vpid].parent_pointer) = 0;

  if (vpid != 0)
  {
    while (nodes[vpid].parent_sense != sense);
  }

  *nodes[vpid].child_pointers[0] = sense;
  *nodes[vpid].child_pointers[1] = sense;

  sense_init = !sense;
}

void gtmp_finalize()
{
  free(nodes);
}