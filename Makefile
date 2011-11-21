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
	$(CC) -c $(CFLAGS) $*.c ./omp

# Targets ...
all: partdiff-seq

partdiff-openmp: $(OPENMP) Makefile
	$(CC) $(LFLAGS) -o $@ $(OPENMP) $(LIBS)

partdiff-seq: $(OBJS) Makefile
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	$(RM) *.o *~

partdiff-openmp.o : partdiff-seq.c Makefile

partdiff-seq.o: partdiff-seq.c Makefile

askparams.o: askparams.c Makefile

displaymatrix.o: displaymatrix.c Makefile
