#include <stdlib.h>
#include <omp.h>
#include "gtmp.h"

/** Things changed to optimize:
  (Put each node on its own cache line)
  1. Make nodes an array of node pointers instead of an array of nodes
  2. Put each element of nodes on different cache line to avoid false sharing
  3. Make num_nodes static for cleanup purposes
  4. Free up individual nodes as well as nodes array

  (Optimize how we do atomic test and decrement)
  5. Change OMP critical section to single fetch-and-dec call to reduce 
     overhead

  (Make single-line function inline)
  6. Make _gtmp_get_node an inline function

  (Reduce number of nodes, it need not be power of 2, multiple of 2 should be fine)
  7. Make v multiple of 2

  8. local_sense on its own cache line

  9. Use bitwise operator for negation
*/

typedef struct _node_t
{
  short k;
  short count;
  short sense;
  struct _node_t *parent;
} node_t;

static int num_leaves;
static int num_nodes;
static node_t **nodes;

void gtmp_barrier_aux(node_t* node, short* sense);

static inline node_t* _gtmp_get_node(int i)
{
  return nodes[i];
}

void gtmp_init (int n_threads)
{
  node_t *curr_node;

  int v;
  if (n_threads % 2 == 0)
  {
    v = n_threads;
  }
  else
  {
    v = n_threads + 1;
  }
  
  num_nodes = v - 1;
  num_leaves = v / 2;

  nodes = (node_t**)malloc(num_nodes * sizeof(node_t*));
  for (int i = 0; i < num_nodes; i++)
  {
    // To avoid false sharing, allocate a block equal to the chache line size.
    posix_memalign((void**)&(nodes[i]), LEVEL1_DCACHE_LINESIZE, LEVEL1_DCACHE_LINESIZE);

    curr_node = _gtmp_get_node(i);
    curr_node->k = i < n_threads - 1 ? 2 : 1;
    curr_node->count = curr_node->k;
    curr_node->sense = 0;
    curr_node->parent = i == 0 ? NULL : _gtmp_get_node((i - 1) / 2);
  }

  curr_node = _gtmp_get_node(0);
}

void gtmp_barrier()
{
  short *local_sense;
  // To avoid false sharing, allocate a block equal to the chache line size.
  posix_memalign((void**)&local_sense, LEVEL1_DCACHE_LINESIZE, LEVEL1_DCACHE_LINESIZE);
  *local_sense = _gtmp_get_node(num_leaves - 1 + (omp_get_thread_num() % num_leaves))->sense ^ 0x1;

  gtmp_barrier_aux(_gtmp_get_node(num_leaves - 1 + (omp_get_thread_num() % num_leaves)), local_sense);

  free((void*)local_sense);
}

void gtmp_barrier_aux(node_t* node, short* local_sense)
{
  if(__sync_fetch_and_sub(&node->count, 1) == 1)
  {
    if(node->parent != NULL)
    {
      gtmp_barrier_aux(node->parent, local_sense);
    }

    node->count = node->k;
    node->sense = *local_sense;
  }

  while (node->sense != *local_sense);
}

void gtmp_finalize()
{
  for (int i = 0; i < num_nodes; i++)
  {
    free((void*)nodes[i]);
  }

  free((void*)nodes);
}

