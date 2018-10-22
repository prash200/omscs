OMPFLAGS = -g -Wall -std=c99 -fopenmp -DLEVEL1_DCACHE_LINESIZE=`getconf LEVEL1_DCACHE_LINESIZE`
OMPLIBS = -lgomp
CC = gcc

#MPICH = /usr/lib64/openmpi/1.4-gcc
MPIFLAGS = -g -Wall -std=c99 #-I$(MPICH)/include
#MPICC = /opt/openmpi-1.4.3-gcc44/bin/mpicc
MPICC = mpicc

all: hello_openmp gtmp_counter gtmp_mcs gtmp_tree hello_mpi gtmpi_dissemination gtmpi_tournament gtmpi_tournament

hello_openmp: hello_openmp.c
	$(CC) $(OMPFLAGS) -o $@ $^ $(OMPLIBS)

gtmp_counter: gtmp.c gtmp_counter.c
	$(CC) $(OMPFLAGS) -o $@ $^ $(OMPLIBS)

gtmp_mcs: gtmp.c gtmp_mcs.c
	$(CC) $(OMPFLAGS) -o $@ $^ $(OMPLIBS)

gtmp_tree: gtmp.c gtmp_tree.c
	$(CC) $(OMPFLAGS) -o $@ $^ $(OMPLIBS)

hello_mpi: hello_mpi.c
	$(MPICC) $(MPIFLAGS) -o $@ $^

gtmpi_dissemination: gtmpi.c gtmpi_dissemination.c
	$(MPICC) $(MPIFLAGS) -o $@ $^

gtmpi_tournament: gtmpi.c gtmpi_tournament.c
	$(MPICC) $(MPIFLAGS) -o $@ $^

gtmpi_tournament: gtmpi.c gtmpi_counter.c
	$(MPICC) $(MPIFLAGS) -o $@ $^

clean:
	rm -rf *.o hello_openmp hello_mpi_* gtmp_* gtmpi_*
