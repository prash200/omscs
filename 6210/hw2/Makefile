OMPFLAGS = -g -Wall -std=c99 -fopenmp -DLEVEL1_DCACHE_LINESIZE=`getconf LEVEL1_DCACHE_LINESIZE`
OMPLIBS = -lgomp
CC = gcc

#MPICH = /usr/lib64/openmpi/1.4-gcc
MPIFLAGS = -g -Wall -std=c99 #-I$(MPICH)/include
MPILIBS =
#MPICC = /opt/openmpi-1.4.3-gcc44/bin/mpicc
MPICC = mpicc

DEBUG ?= no



hello_openmp: hello_openmp.c
	$(CC) $(OMPFLAGS) -o $@ $^ $(OMPLIBS)

gtmp_counter: hello_gtmp.c gtmp_counter.c
	$(CC) $(OMPFLAGS) -o $@ $^ $(OMPLIBS)

gtmp_mcs: hello_gtmp.c gtmp_mcs.c
	$(CC) $(OMPFLAGS) -o $@ $^ $(OMPLIBS)

gtmp_tree: hello_gtmp.c gtmp_tree.c
	$(CC) $(OMPFLAGS) -o $@ $^ $(OMPLIBS)

hello_mpi: hello_mpi.c
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

gtmpi_dissemination: hello_gtmpi.c gtmpi_dissemination.c
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

gtmpi_tournament: hello_gtmpi.c gtmpi_tournament.c
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

gtmpi_counter: hello_gtmpi.c gtmpi_counter.c
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

clean:
	rm -rf *.o hello_openmp hello_mpi_* gtmp_* gtmpi_*
