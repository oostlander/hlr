# Common definitions
CC = mpicc

# Compiler flags, paths and libraries
CFLAGS = -std=c99 -fopenmp -g -pthread -pedantic -Wall -Wextra -O0
LFLAGS = $(CFLAGS)
LIBS   = -lm

OBJS = partdiff-par.o askparams.o displaymatrix.o

# Rule to create *.o from *.c
.c.o:
	$(CC) -c $(CFLAGS) $*.c

# Targets ...
all: partdiff-par

partdiff-par: $(OBJS) Makefile
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	$(RM) -r *.o *~ .ddt* *.error *.output
clean-script:
	$(RM) -r *.out p-omp*
partdiff-par.o: partdiff-par.c Makefile

askparams.o: askparams.c Makefile

displaymatrix.o: displaymatrix.c Makefile
