#ifndef GTMP_H
#define GTMP_H

void gtmp_init(int n_threads);
void gtmp_barrier();
void gtmp_finalize();
#endif