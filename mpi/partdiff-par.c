/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**                TU Muenchen - Institut fuer Informatik                  **/
/**                                                                        **/
/** Copyright: Prof. Dr. Thomas Ludwig                                     **/
/**            Andreas C. Schmidt                                          **/
/**            JK und andere  besseres Timing, FLOP Berechnung             **/
/**                                                                        **/
/** File:      partdiff-seq.c                                              **/
/**                                                                        **/
/** Purpose:   Partial differential equation solver for Gauss-Seidel and   **/
/**            Jacobi method.                                              **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/* ************************************************************************ */
/* Include standard header file.                                            */
/* ************************************************************************ */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <sys/time.h>
#include "partdiff-par.h"
//#include <omp.h>
#include <mpi.h>

struct calculation_arguments
{
  int     N;              /* number of spaces between lines (lines=N+1)     */
  int     num_matrices;   /* number of matrices                             */
  double  ***Matrix;      /* index matrix used for addressing M             */
  double  *M;             /* two matrices with real values                  */
  double  h;              /* length of a space between two lines            */
};

struct calculation_results
{
  int     m;
  int     stat_iteration; /* number of current iteration                    */
  double  stat_precision; /* actual precision of all slaves in iteration    */
};

struct mpi_stats
{
  int worldsize;			/* Size of Comm_WORLD */
  int rank;                             /* Rank of Node in Comm_WORLD */
  int jblocksize;			/* Size of Block per node for Jacobi Method */
  int localN;
};

/* ************************************************************************ */
/* Global variables                                                         */
/* ************************************************************************ */

/* time measurement variables */
struct timeval start_time;       /* time when program started                      */
struct timeval comp_time;        /* time when calculation completed                */
struct mpi_stats mpis;		 /* mpi values of specific node */
/* ************************************************************************ */
/* initVariables: Initializes some global variables                         */
/* ************************************************************************ */
static
void
initVariables (struct calculation_arguments* arguments, struct calculation_results* results, struct options* options)
{
  arguments->N = options->interlines * 8 + 9 - 1; /* magic numbers... why "* 8 + 9 - 1" */
  arguments->num_matrices = (options->method == METH_JACOBI) ? 2 : 1;
  arguments->h = (float)( ( (float)(1) ) / (arguments->N));
  
  results->m = 0;
  results->stat_iteration = 0;
  results->stat_precision = 0;
}

/* ************************************************************************ */
/* freeMatrices: frees memory for matrices                                  */
/* ************************************************************************ */
static
void
freeMatrices (struct calculation_arguments* arguments)
{
  int i;
  
  for (i = 0; i < arguments->num_matrices; i++)
    {
      free(arguments->Matrix[i]);
    }
  
  free(arguments->Matrix);
  free(arguments->M);
}

/* ************************************************************************ */
/* allocateMemory ()                                                        */
/* allocates memory and quits if there was a memory allocation problem      */
/* ************************************************************************ */
static
void*
allocateMemory (size_t size)
{
  void *p;
  
  if ((p = malloc(size)) == NULL)
    {
      printf("\n\nSpeicherprobleme!\n");
      /* exit program */
      exit(1);
    }
  
  return p;
}

/* ************************************************************************ */
/* allocateMatrices: allocates memory for matrices                          */
/* ************************************************************************ */
static
void
allocateMatrices (struct calculation_arguments* arguments)
{
  /* Bei Gauss Seidel muss nicht die gesamte Matrix im Speicher gehalten werden sondern nur der jeweils zum Rechnen benötigte Teil. */
  /* Es muss nur N durch Anzahl Knoten belegt werden. Ist dabei ein Rest vorhanden wird dieser auf
die Knoten verteilt (bei 64 Knoten und Rest 63 bekommen 63 Knoten eine Zeile mehr nicht ein Knoten
63 Zeilen mehr */
  /* I-Probe Ansatz: Es wird mittels Tag geprüft, ob die gewünschte Genauigkeit schon erreicht wurde.
   Dann AllReduce */
   int i, j;
   //Folge: Matrix[matr] [rows] [collums]
   int N = arguments->N;
   //On the master-node the complete matrix is allocated!
   if (0 == mpis.rank)
   {
	   arguments->M = allocateMemory(arguments->num_matrices * (N + 1) * (N + 1) * sizeof(double));
	   arguments->Matrix = allocateMemory(arguments->num_matrices * sizeof(double**));
	   for (i = 0; i < arguments->num_matrices; i++)
	   {
		   arguments->Matrix[i] = allocateMemory((N + 1) * sizeof(double*)); /* Elementzugriff über Zeiger */
		   for (j = 0; j <= N; j++)
		   {
			   arguments->Matrix[i][j] = (double*)(arguments->M + (i * (mpis.localN + 1) * (N + 1)) + (j * (N + 1)));
		   }
	   }
   }else
   {
      arguments->M = allocateMemory(arguments->num_matrices * (mpis.localN + 1) * (N + 1) * sizeof(double));
      arguments->Matrix = allocateMemory(arguments->num_matrices * sizeof(double**));
      //printf("Prozess%i hat %i Zeilen allokiert und eine weitere Zeile belegt.\n", mpis.rank, (N / mpis.worldsize));
  for (i = 0; i < arguments->num_matrices; i++)
  {
      arguments->Matrix[i] = allocateMemory((N + 1) * sizeof(double*)); /* Elementzugriff über Zeiger */
      for (j = 0; j <= N; j++)
      {
		  arguments->Matrix[i][j] = (double*)(arguments->M + (i * (mpis.localN + 1) * (N + 1)) + (j * (N + 1)));
	  }
  }
}
}

/* ************************************************************************ */
/* initMatrices: Initialize matrix/matrices and some global variables       */
/* ************************************************************************ */
static
void
initMatrices (struct calculation_arguments* arguments, struct options* options)
{
	// Es reicht nicht local N zu verwenden, denn es muss nach Zeile und Spalte unterschieden werden
	// sonst wird auch generell eine längere Zeile angnommen (gilt auch für calculate)
      int g, i, j;                                /*  local variables for loops   */
      int N = arguments->N;
      int lN = mpis.localN;
      double h = arguments->h;
      double*** Matrix = arguments->Matrix;
      
      /* initialize matrix/matrices with zeros */
      for (g = 0; g < arguments->num_matrices; g++)
	{
	  for (i = 0; i <= lN; i++)
	    {
	      for (j = 0; j <= N; j++)
		{
		  Matrix[g][i][j] = 0;
		}
	    }
	}
      
      /* initialize borders, depending on function (function 2: nothing to do) */
      if (options->inf_func == FUNC_F0)
	{
	  for(i = 0; i <= lN; i++)
	    {
	      for (j = 0; j < arguments->num_matrices; j++)
	      {
			  Matrix[j][i][0] = 1 - (h * i);
			  Matrix[j][i][N] = h * i;
			  if (0 == mpis.rank)
			  {
				  Matrix[j][0][i] = 1 - (h * i);
			  }else{Matrix[j][0][i] = 0;}
			  if (mpis.rank == (mpis.worldsize - 1))
			  {
				  Matrix[j][N][i] = h * i;
			  }else{Matrix[j][N][i] = 0;}
		  }
	    }
	  
	  for (j = 0; j < arguments->num_matrices; j++)
	    {
	      Matrix[j][N][0] = 0;
	      Matrix[j][0][N] = 0;
	    }
	}
}


/* ************************************************************************ */
/* calculate: solves the equation                                           */
/* ************************************************************************ */
static
void
calculate (struct calculation_arguments* arguments, struct calculation_results *results, struct options* options)
{
  int i, j;                                   /* local variables for loops  */
  int m1, m2;                                 /* used as indices for old and new matrices       */
  double star;                                /* four times center value minus 4 neigh.b values */
  double residuum;                            /* residuum of current iteration                  */
  double maxresiduum;                         /* maximum residuum value of a slave in iteration */
  //double n_maxresiduum = 0;					/* temporal value of maxresiduum on one Node */
  int N = arguments->N;
  int lN = mpis.localN;
  double h = arguments->h;
  double*** Matrix = arguments->Matrix;
  //MPI_Status status;
  //printf("h is:%f\n",arguments->h);
  //printf("N is:%d\n",arguments->N);
  //	omp_set_num_threads(options->number); /* setting number of openMP Threads */
  
  /* Print the number of available processors on the system and the
   * maximum number of threads useable */
  //printf("Anzahl Threads: %d\n", omp_get_max_threads());
  //printf("Anzahl Prozessoren: %d\n", omp_get_num_procs());
   /*if (0 == mpis.rank)
    {
      printf("I am the Master\n");
    }*/
  /* initialize m1 and m2 depending on algorithm */
  if (options->method == METH_GAUSS_SEIDEL)
    {
      m1=0; m2=0;
    }
  else			/* Jacobi */
    {
      m1=0; m2=1;
    }
  
  while (options->term_iteration > 0)
    {
      maxresiduum = 0;
      /* schemat */
      /* Matrix muss beim Masterknoten in den Speicher gelegt werden (nur Master) */
      /* Datenstruktur für die im Speicher vorgehaltenen entsprechend der Knotenaufteilung*/
      /* senden der nötigen Daten an die Knoten */
      /* berechnen */
      /* senden der nötigen Daten an den Nachfolgeknoten */
      /* start parallel part of the program */
      /* variables are declared either private, firstprivate, lastprivate or shared */
      /* default clause sets this for all variables not mentioned !!!!!*/    
      /* over all rows */
      for (i = 1; i < lN; i++)
	{
	  /* over all columns */
	  for (j = 1; j < N; j++)
	    {
	      star = (Matrix[m2][i-1][j] + Matrix[m2][i][j-1] + Matrix[m2][i][j+1] + Matrix[m2][i+1][j]) * 0.25;
	      
	      if (options->inf_func == FUNC_FPISIN)
		{
		  star = (TWO_PI_SQUARE * sin((double)(j) * PI * h) * sin((double)(i) * PI * h) * h * h * 0.25) + star;
		}
	      
	      residuum = Matrix[m2][i][j] - star;
	      residuum = (residuum < 0) ? -residuum : residuum; /* Durch abs ersetzen (weil Prozessor befehle) */	
	      /* temporal calculation of maxresiduum per thread */
	      maxresiduum = (residuum < maxresiduum) ? maxresiduum : residuum;
	      
	      Matrix[m1][i][j] = star;
	    }
	  /* collect all temporal values of maxresiduum (critical needed to prevent race condition) */
	}
      results->stat_iteration++;
      results->stat_precision = maxresiduum;
      /*if(mpis.rank < (mpis.worldsize-1))
      {
		  printf("snd rk:%i\n",mpis.rank);
		  MPI_Send(&(Matrix[m2][lN][0]),N+1,MPI_DOUBLE,mpis.rank + 1,1,MPI_COMM_WORLD);
		  //MPI_Send(&Matrix[m2][N][0],N+1,MPI_DOUBLE,mpis.rank + 1,1,MPI_COMM_WORLD);
	  }
  if(mpis.rank != 0)
  {
      MPI_Recv(&(Matrix[m2][0][0]),N+1,MPI_DOUBLE,mpis.rank - 1,1,MPI_COMM_WORLD,&status);
      printf("rcv rk:%i\n",mpis.rank);
  }*/
      
      /* exchange m1 and m2 */
      i=m1; m1=m2; m2=i;
      /* *********************************************************************** */
      /* !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! */
      /* Es kann also nur jeweils eine Iteration überhaupt parallelisiert werden */
      /* !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! !! */
      /* *********************************************************************** */
      
      /* check for stopping calculation, depending on termination method */
      if (options->termination == TERM_PREC)
	{
	  if (maxresiduum < options->term_precision) 
	    {
	      options->term_iteration = 0;
	    }
	}
      else if (options->termination == TERM_ITER)
	{
		//MPI_Barrier;
	  options->term_iteration--;
	}
    }
    /* calculate the length of the rows to be sent per node and saves them
     * in counts in the correct order.*/
    int counts[mpis.worldsize];
    int displ[mpis.worldsize];
    for (j = 0; j < mpis.worldsize; j++)
    {
		if (((arguments->N % mpis.worldsize) >= (mpis.rank+1))&&(1 != mpis.worldsize))
		{
			counts[j] = ((arguments->N / mpis.worldsize)+1)*(N + 1);
			// all bigger nodes so far
			displ[j] = j * (N + 1) * sizeof(double);
		}else if (((arguments->N % mpis.worldsize) < (mpis.rank+1))||(1 == mpis.worldsize))
		{
			counts[j] = (arguments->N / mpis.worldsize)*(N + 1);
			// all bigger nodes + all small nodes so far
			// all bigger nodes = (arguments->N % mpis.worldsize)*(N + 1)
			// all small nodes so far = ((j + 1) - (arguments->N % mpis.worldsize))*(N + 1)
			displ[j] = ((arguments->N % mpis.worldsize)*(N + 1))+(((j + 1) - (arguments->N % mpis.worldsize))*(N + 1))*sizeof(double);
		}
	}
    /* Einsammeln der Ergebnisse von den Knoten. Dabei senden die unterschied-
     * lichen Prozesse auch unterschiedliche viele Zeilen (siehe Berechnung 
     * von localN). Dementsprechend ist die Anzahl der zu sendenden Daten im
     * Array counts und die entsprechend berechneten Displacements in displs
     * hinterlegt.*/
    MPI_Gatherv(&Matrix[m2][0][0], lN*(N+1), MPI_DOUBLE, &Matrix[m2][0][0], counts, displ, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  
  results->m = m2;
}
/* ************************************************************************ */
/*  displayStatistics: displays some statistics about the calculation       */
/* ************************************************************************ */
static
void
displayStatistics (struct calculation_arguments* arguments, struct calculation_results *results, struct options* options)
{
  int N = arguments->N;
  
  double time = (comp_time.tv_sec - start_time.tv_sec) + (comp_time.tv_usec - start_time.tv_usec) * 1e-6;
  printf("Berechnungszeit:    %f s \n", time);
  
  //Calculate Flops
  // star op = 5 ASM ops (+1 XOR) with -O3, matrix korrektur = 1
  double q = 6;
  double mflops;
  
  if (options->inf_func == FUNC_F0)
    {
      // residuum: checked 1 flop in ASM, verified on Nehalem architecture.
      q += 1.0;
    }
  else
    {
      // residuum: 11 with O0, but 10 with "gcc -O3", without counting sin & cos
      q += 10.0;
    }
  
  /* calculate flops  */
  mflops = (q * (N - 1) * (N - 1) * results->stat_iteration) * 1e-6;
  printf("Executed float ops: %f MFlop\n", mflops);
  printf("Speed:              %f MFlop/s\n", mflops / time);
  
  printf("Berechnungsmethode: ");
  
  if (options->method == METH_GAUSS_SEIDEL)
    {
      printf("Gauss-Seidel");
    }
  else if (options->method == METH_JACOBI)
    {
      printf("Jacobi");
    }
  
  printf("\n");
  printf("Interlines:         %d\n",options->interlines);
  printf("Stoerfunktion:      ");
  
  if (options->inf_func == FUNC_F0)
    {
      printf("f(x,y)=0");
    }
  else if (options->inf_func == FUNC_FPISIN)
    {
      printf("f(x,y)=2pi^2*sin(pi*x)sin(pi*y)");
    }
  
  printf("\n");
  printf("Terminierung:       ");
  
  if (options->termination == TERM_PREC)
    {
      printf("Hinreichende Genaugkeit");
    }
  else if (options->termination == TERM_ITER)
    {
      printf("Anzahl der Iterationen");
    }
  
  printf("\n");
  printf("Anzahl Iterationen: %d\n", results->stat_iteration);
  printf("Norm des Fehlers:   %e\n", results->stat_precision);
}

static void initMPI(struct mpi_stats* mpis, struct calculation_arguments* arguments)
{
  MPI_Comm_size(MPI_COMM_WORLD,&mpis->worldsize);
  MPI_Comm_rank(MPI_COMM_WORLD,&mpis->rank);
  if (((arguments->N % mpis->worldsize) >= (mpis->rank+1))&&(1 != mpis->worldsize))
  {
  mpis->localN = ((arguments->N / mpis->worldsize)+1);
  }else if (((arguments->N % mpis->worldsize) < (mpis->rank+1))||(1 == mpis->worldsize))
  {
	mpis->localN = (arguments->N / mpis->worldsize);
	}
  if (mpis->worldsize < 2)
    {
      printf("Given worldsize does not allow for MPI parallelization. Trying OpenMP...?\n");
    }
}

/* ************************************************************************ */
/*  main                                                                    */
/* ************************************************************************ */
int
main (int argc, char** argv)
{
  struct options options;
  struct calculation_arguments arguments;
  struct calculation_results results; 
    int rc;
  
  rc = MPI_Init(&argc,&argv);
  if (rc != MPI_SUCCESS) 
  {
      printf("Error initializing MPI. Terminating.\n");
      MPI_Abort(MPI_COMM_WORLD, rc);
  }
     AskParams(&options, argc, argv); /* get parameters */   
      initVariables(&arguments, &results, &options);           /* ******************************************* */
      initMPI(&mpis, &arguments);	/* initalize MPI */
      allocateMatrices(&arguments);                            /*  get and initialize variables and matrices  */
      initMatrices(&arguments, &options);                      /* ******************************************* */
      
      gettimeofday(&start_time, NULL);                   /*  start timer         */
      calculate(&arguments, &results, &options);  /*  solve the equation  */
      gettimeofday(&comp_time, NULL);                    /*  stop timer          */
      if (0 == mpis.rank)
      {
		  displayStatistics(&arguments, &results, &options);                      /* **************** */
		  DisplayMatrix("Matrix:",                                                /*  display some    */
				arguments.Matrix[results.m][0], options.interlines);      /*  statistics and  */
	  }
      freeMatrices(&arguments);                                                   /*  free memory     */
                                                                        /* **************** */
  MPI_Barrier(MPI_COMM_WORLD);
  return 0;
}
