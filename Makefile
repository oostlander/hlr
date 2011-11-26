# Common definitions
CC = gcc

# Compiler flags, paths and libraries
CFLAGS = -std=c99 -fopenmp -g -pthread -pedantic -Wall -Wextra -O1
LFLAGS = $(CFLAGS)
LIBS   = -lm
OPENMP = partdiff-openmp.o askparams.o displaymatrix.o
OBJS   = partdiff-seq.o askparams.o displaymatrix.o

# Rule to create *.o from *.c
.c.o:
	$(CC) -c $(CFLAGS) $*.c

# Targets ...
all: partdiff-seq

partdiff-openmp: $(OPENMP) Makefile
	$(CC) $(LFLAGS) -o $@ $(OPENMP) $(LIBS)

partdiff-seq: $(OBJS) Makefile
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	$(RM) *.o *~

clean-script:
	$(RM) -r *.out p-omp*
clean-all:
	$(RM) -r *.out p-omp* *.o *~ partdiff-seq partdiff-openmp omp/partdiff-seq omp/*.out omp/p-omp* omp/*.o omp/*~

partdiff-openmp.o : partdiff-openmp.c Makefile

partdiff-seq.o: partdiff-seq.c Makefile

askparams.o: askparams.c Makefile

displaymatrix.o: displaymatrix.c Makefile
