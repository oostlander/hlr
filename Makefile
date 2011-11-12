# Common definitions
CC = gcc

# Compiler flags, paths and libraries
CFLAGS = -std=c99 -g -pthread -pedantic -Wall -Wextra -O1
LFLAGS = $(CFLAGS)
LIBS   = -lm

OBJS = partdiff-seq.o askparams.o displaymatrix.o

# Rule to create *.o from *.c
.c.o:
	$(CC) -c $(CFLAGS) $*.c

# Targets ...
all: partdiff-seq

partdiff-seq: $(OBJS) Makefile
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	$(RM) *.o *~

partdiff-seq.o: partdiff-seq.c Makefile

askparams.o: askparams.c Makefile

displaymatrix.o: displaymatrix.c Makefile
