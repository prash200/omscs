#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <sys/utsname.h>
#include "gtmp.h"

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "Usage: ./hello_world [NUM_THREADS]\n");
    exit(1);
  }

  printf("This is the serial section\n");

  int num_threads = strtol(argv[1], NULL, 10);
  omp_set_dynamic(0);
  if (omp_get_dynamic())
  {
    printf("Warning: dynamic adjustment of threads has been set\n");
  }

  omp_set_num_threads(num_threads);
  gtmp_init(num_threads);

#pragma omp parallel
  {
    int thread_num = omp_get_thread_num();
    struct utsname ugnm;
    uname(&ugnm);
    printf("Hello World from thread %d of %d, running on %s.\n", thread_num, num_threads, ugnm.nodename);

    gtmp_barrier();
  }

#pragma omp parallel
  {    
    int thread_num = omp_get_thread_num();
    struct utsname ugnm;
    uname(&ugnm);
    printf("Goodbye World from thread %d of %d, running on %s.\n", thread_num, num_threads, ugnm.nodename);

    gtmp_barrier();
  }

  gtmp_finalize();

  printf("Back in the serial section again\n");
  return 0;
}

