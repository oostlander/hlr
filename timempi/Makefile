# Common definitions
CC = mpicc

# Compiler flags, paths and libraries
CFLAGS = -std=c99 -fopenmp -g -pthread -pedantic -Wall -Wextra -O1
LFLAGS = $(CFLAGS)
LIBS   = -lm
TIMEMPI = timempi.o
TIMEMPI2 = timempi2.o
BIN = timempi timempi2

# Rule to create *.o from *.c
.c.o:
	$(CC) -c $(CFLAGS) $*.c
	
# Targets ...
all: timempi timempi2

timempi : $(TIMEMPI) Makefile
	$(CC) $(LFLAGS) -o $@ $(TIMEMPI) $(LIBS)
	
timempi2 : $(TIMEMPI2) Makefile
	$(CC) $(LFLAGS) -o $@ $(TIMEMPI2) $(LIBS)
	
clean : 
	$(RM) *.o *~ $(BIN)
