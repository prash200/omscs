#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "gtmp.h"

/** Things changed to optimize:
  (1-4: put each node on its own cache line)
  1. Make nodes an array of node pointers instead of an array of nodes
  2. Put each element of nodes on different cache line to avoid false sharing
  3. Make num_nodes static for cleanup purposes
  4. Free up individual nodes as well as nodes array

  (5: optimize how we do atomic test and decrement)
  5. Change OMP critical section to single fetch-and-dec call to reduce 
     overhead

  (6: make single-line function inline)
  6. Make _gtmp_get_node an inline function
*/

typedef struct _node_t
{
  int k;
  int count;
  int lock_sense;
  struct _node_t* parent;
} node_t;

static int num_leaves;
static int num_nodes;
static node_t **nodes;
static const cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);

void gtmp_barrier_aux(node_t* node, int sense);

inline node_t* _gtmp_get_node(int i)
{
  return nodes[i];
}

void gtmp_init (int num_threads)
{
  node_t* curnode;
  
  int v = 1;
  while ( v < num_threads)
  {
    v *= 2;
  }
  
  num_nodes = v - 1;
  num_leaves = v/2;

  nodes = (node_t**)malloc(num_nodes * sizeof(node_t*));

  for (int i = 0; i < num_nodes; i++)
  {
    posix_memalign((void**)&(nodes[i]), cache_line_size, cache_line_size);

    curnode = _gtmp_get_node(i);
    curnode->k = i < num_threads - 1 ? 2 : 1;
    curnode->count = curnode->k;
    curnode->lock_sense = 0;
    curnode->parent = _gtmp_get_node((i - 1) / 2);
  }
  
  curnode = _gtmp_get_node(0);
  curnode->parent = NULL;
}

void gtmp_barrier()
{
  node_t* mynode;
  int sense;

  mynode = _gtmp_get_node(num_leaves - 1 + (omp_get_thread_num() % num_leaves));
  
  sense = !mynode->lock_sense;
  
  gtmp_barrier_aux(mynode, sense);
}

void gtmp_barrier_aux(node_t* node, int sense)
{
  if(__sync_fetch_and_sub(&node->count, 1) == 1)
  {
    if(node->parent != NULL)
    {
      gtmp_barrier_aux(node->parent, sense);
    }

    node->count = node->k;
    node->lock_sense = !node->lock_sense;
  }

  while (node->lock_sense != sense);
}

void gtmp_finalize()
{
  int i;

  for (i = 0; i < num_nodes; i++)
  {
    free(nodes[i]);
  } 

  free(nodes);
}

