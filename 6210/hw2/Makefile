OMPFLAGS = -g -Wall -std=c99 -fopenmp -DLEVEL1_DCACHE_LINESIZE=`getconf LEVEL1_DCACHE_LINESIZE` -D_POSIX_C_SOURCE=200112L
OMPLIBS = -lgomp
CC = gcc

MPI_PROCESSORS ?= 2
MPIFLAGS = -g -Wall -std=c99 -DPROCESSORS=$(MPI_PROCESSORS)
MPICC = mpicc

rebuild: clean build

build: hello_openmp gtmp_counter gtmp_mcs gtmp_tree hello_mpi gtmpi_dissemination gtmpi_tournament gtmpi_counter

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

gtmpi_dissemination: clean gtmpi.c gtmpi_dissemination.c
	$(MPICC) $(MPIFLAGS) -o $@ $^
	@echo "Usage: mpiexec -n $(MPI_PROCESSORS) ./gtmpi_dissemination"

gtmpi_tournament: clean gtmpi.c gtmpi_tournament.c
	$(MPICC) $(MPIFLAGS) -o $@ $^
	@echo "Usage: mpiexec -n $(MPI_PROCESSORS) ./gtmpi_tournament"

gtmpi_counter: clean gtmpi.c gtmpi_counter.c
	$(MPICC) $(MPIFLAGS) -o $@ $^
	@echo "Usage: mpiexec -n $(MPI_PROCESSORS) ./gtmpi_counter"

clean:
	rm -rf *.o hello_openmp gtmp_counter gtmp_mcs gtmp_tree hello_mpi gtmpi_dissemination gtmpi_tournament gtmpi_counter