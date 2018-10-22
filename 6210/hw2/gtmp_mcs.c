#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "gtmp.h"

typedef struct treenode_s
{
  unsigned char parentsense;
  unsigned char *parentpointer;
  unsigned char *childpointers[2];
  unsigned char havechild[4];
  unsigned char childnotready[4];
  unsigned char dummy;
} treenode;

static treenode *nodes;

static unsigned int vpid_next;
static unsigned int sense_init;
static int num_procs;


void gtmp_init(int num_threads)
{
  int i, j;

  num_procs = num_threads;
  vpid_next = 0;
  sense_init = 1;
  nodes = (treenode *) malloc(num_procs * sizeof(treenode));

  for (i = 0; i < num_procs; i++)
  {
    for (j = 0; j < 4; j++)
    {
      if (4 * i + j + 1 < num_procs)
      {
        nodes[i].havechild[j] = 1;
      }
      else
      {
        nodes[i].havechild[j] = 0;
      }

      nodes[i].childnotready[j] = nodes[i].havechild[j];
    }

    if (i != 0)
    {
      nodes[i].parentpointer = &(nodes[(i-1)/4].childnotready[(i-1) % 4]);
    }
    else
    {
      nodes[i].parentpointer = &(nodes[i].dummy);
    }

    if (2 * i + 1 < num_procs)
    {
      nodes[i].childpointers[0] = &(nodes[2*i+1].parentsense);
    }
    else
    {
      nodes[i].childpointers[0] = &(nodes[i].dummy);
    }

    if (2 * i + 2 < num_procs)
    {
      nodes[i].childpointers[1] = &(nodes[2*i+2].parentsense);
    }
    else
    {
      nodes[i].childpointers[1] = &(nodes[i].dummy);
    }

    nodes[i].parentsense = 0;
  }
}

void gtmp_barrier()
{
  unsigned int vpid = __sync_fetch_and_add(&vpid_next, 1) % num_procs;
  unsigned char sense = sense_init;
  int i;

  while (nodes[vpid].childnotready[0] |
    nodes[vpid].childnotready[1] | 
    nodes[vpid].childnotready[2] |
    nodes[vpid].childnotready[3]);

  for (i = 0; i < 4; i++)
  {
    nodes[vpid].childnotready[i] = nodes[vpid].havechild[i];
  }

  *(nodes[vpid].parentpointer) = 0;

  if (vpid != 0)
  {
    while (nodes[vpid].parentsense != sense);
  }

  *nodes[vpid].childpointers[0] = sense;
  *nodes[vpid].childpointers[1] = sense;

  sense_init = !sense;
}

void gtmp_finalize()
{
  free(nodes);
}