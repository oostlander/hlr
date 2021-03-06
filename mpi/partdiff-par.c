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
  int ghostlines;			/* Number of ghostlines (overlapping lines) */
  int localN;
  int *counts;
  int *displ;
  int *glines;
  int bignode;           /* Boolean! Does the Node calculate an extra line. */
};

/* ************************************************************************ */
/* Global variables                                                         */
/* ************************************************************************ */

/* time measurement variables */
struct timeval start_time;       /* time when program started                      */
struct timeval comp_time;        /* time when calculation completed                */
struct mpi_stats mpis;		     /* mpi values of specific node and etire com*/

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
/* freeMPI: frees memory for matrices of MPIstats                           */
/* ************************************************************************ */
static
void
freeMPI (struct mpi_stats* mpis)
{
  free(mpis->counts);
  free(mpis->displ);
  free(mpis->glines);
}

/* ********************************************************************************* */
/* isBignode: returns as boolean wether the given node has to calculate a line more. */
/* ********************************************************************************* */
static int isBignode(int node, struct calculation_arguments* arguments)
{
  int result = 0;
  if (((arguments->N % mpis.worldsize) >= (node+1))&&(1 != mpis.worldsize))
  {
    result = 1;
  }else if(((arguments->N % mpis.worldsize) < (node+1))||(1 == mpis.worldsize))
  {
    result = 0;
  }
  return result;
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
  int i, j;
  int N = arguments->N;
  //On the master-node the complete matrix is beeing allocated!
  if (0 == mpis.rank)
  {
    arguments->M = allocateMemory(arguments->num_matrices * (N + 1) * (N + 1) * sizeof(double));
    arguments->Matrix = allocateMemory(arguments->num_matrices * sizeof(double**));
    for (i = 0; i < arguments->num_matrices; i++)
    {
      arguments->Matrix[i] = allocateMemory((N + 1) * sizeof(double*)); /* element wise acess through pointers */
      for (j = 0; j <= N; j++)
      {
	arguments->Matrix[i][j] = (double*)(arguments->M + (i * (N + 1) * (N + 1)) + (j * (N + 1)));
      }
    }
  }else
  /* allocate memory on the nodes. */
  {
    arguments->M = allocateMemory(arguments->num_matrices * (mpis.localN + 1) * (N + 1) * sizeof(double));
    arguments->Matrix = allocateMemory(arguments->num_matrices * sizeof(double**));
    /* Debug
    printf("Prozess%i hat %i Zeilen allokiert und eine weitere Zeile belegt.\n", mpis.rank, (N / mpis.worldsize));
    * Debug */
    for (i = 0; i < arguments->num_matrices; i++)
    {
      arguments->Matrix[i] = allocateMemory((N + 1) * sizeof(double*)); /* element wise acess through pointers */
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
  int g, i, j;                                /*  local variables for loops   */
  int N = arguments->N;
  int lN = mpis.localN;
  double h = arguments->h;
  double*** Matrix = arguments->Matrix;
  
  /* initialize matrix/matrices with zeros */
  for (g = 0; g < arguments->num_matrices; g++)
  {
    for (i = 0; i <= ((0 == mpis.rank) ? N : lN); i++)
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
     /* Quick Debug
     double test,test1;
     test = ((0 == mpis.rank) ? N : (mpis.displ[mpis.rank]+lN));
     test1 = mpis.displ[mpis.rank];
     printf("rank%i is initializing from %f to %f. Displacement is %i; lN is %i\n",mpis.rank,test1,test,mpis.displ[mpis.rank],lN);
      Debug */
    
    for(i = mpis.displ[mpis.rank]; i <= ((0 == mpis.rank) ? N : (mpis.displ[mpis.rank]+lN)); i++)
    {
      for (j = 0; j < arguments->num_matrices; j++)
      {
		  Matrix[j][(i-mpis.displ[mpis.rank])][0] = 1 - (h * i);
		  Matrix[j][(i-mpis.displ[mpis.rank])][N] = h * i;
	  }
    }
    // initialize the top and bottom line
    for(i = 0; i <= N; i++)
    {
		for (j = 0; j < arguments->num_matrices; j++)
		{
			if (0 == mpis.rank)
		  {
			  Matrix[j][0][i] = 1 - (h * i);
			  Matrix[j][N][i] = h * i;
		  }
		  if (mpis.rank == (mpis.worldsize - 1))
		  {
			  Matrix[j][lN][i] = h * i;
		  }
		}
	}
    // set the corners to zero
    for (j = 0; j < arguments->num_matrices; j++)
    {
		if (0 == mpis.rank)
		{
			Matrix[j][N][0] = 0;
			Matrix[j][0][N] = 0;
		}
		if (mpis.rank == (mpis.worldsize-1))
		{
			Matrix[j][lN][0] = 0;
		}
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
	residuum = (residuum < 0) ? -residuum : residuum;
	/* temporal calculation of maxresiduum per thread */
	maxresiduum = (residuum < maxresiduum) ? maxresiduum : residuum;
	
	Matrix[m1][i][j] = star;
      }
    }
    
    results->stat_iteration++;
    results->stat_precision = maxresiduum;
    /* exchange m1 and m2 */
    i=m1; m1=m2; m2=i;
    // Send the ghostlines to the neigbours
    //(first and last node are treated differently)
    if(0 == mpis.rank)
    {
      MPI_Send(Matrix[m2][lN-1-1],N,MPI_DOUBLE,mpis.rank + 1, 1,MPI_COMM_WORLD);
      MPI_Recv(Matrix[m2][lN-1],N,MPI_DOUBLE,mpis.rank + 1, 1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    } else
    if(mpis.rank < (mpis.worldsize-1))
    {
      MPI_Send(Matrix[m2][1],N,MPI_DOUBLE,mpis.rank - 1,1,MPI_COMM_WORLD);
      MPI_Recv(Matrix[m2][0],N,MPI_DOUBLE,mpis.rank - 1, 1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      
      MPI_Recv(Matrix[m2][lN-1],N,MPI_DOUBLE,mpis.rank + 1, 1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Send(Matrix[m2][lN-1-1],N,MPI_DOUBLE,mpis.rank + 1,1,MPI_COMM_WORLD);
    } else
    if (mpis.rank == (mpis.worldsize-1))
    {
		MPI_Send(Matrix[m2][1],N,MPI_DOUBLE,mpis.rank - 1,1,MPI_COMM_WORLD);
		MPI_Recv(Matrix[m2][0],N,MPI_DOUBLE,mpis.rank - 1, 1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
	}
    /* Jacobi: One Iteration on many nodes.
     * Gauss-Seidel: Many iterations handed from node to node. */
    
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
      options->term_iteration--;
    }
  }
  /* Collecting the results frm the nodes. Nodes have different amounts of
   * lines to send (see calculation of localN).
   * Number of lines to send is in counts[], displacement in lines from 0
   * in displ[]. 
   * MPI_Gatherv is not recommendable since send and recieve-buffer cant be
   * the same and all processes (including 0) are sending.*/
   if (0 != mpis.rank)
  {
    MPI_Send(Matrix[m2][1], (lN - mpis.ghostlines) * N, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  }else
  {
    for (i = 1; i < mpis.worldsize; i++)
    {
      MPI_Recv(Matrix[m2][(mpis.displ[i] + 1)],(mpis.counts[i] - mpis.glines[i]) * N,
			MPI_DOUBLE, i,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }
  }
  results->m = m2;
}
/* ************************************************************************ */
/*  displayStatistics: displays some statistics about the calculation       */
/* ************************************************************************ */
static void displayStatistics (struct calculation_arguments* arguments, struct calculation_results *results, struct options* options)
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
/* ************************************************************************************ */
/* initMPI: reads and calculates values related to MPI and using it througout the prog. */
/* ************************************************************************************ */
static void initMPI(struct mpi_stats* mpis, struct calculation_arguments* arguments)
{
  int j = 0;
  MPI_Comm_size(MPI_COMM_WORLD,&mpis->worldsize);
  MPI_Comm_rank(MPI_COMM_WORLD,&mpis->rank);
  int ghostlines = ((0 == mpis->rank)||(mpis->rank == (mpis->worldsize -1)) ? 1 : 2);
  mpis->ghostlines = ghostlines;
  if (1 == isBignode(mpis->rank,arguments))
  {
    mpis->bignode = 1;
    //1 extra for big node 2 extra for ghostlines
    mpis->localN = ((arguments->N / mpis->worldsize)+1+ghostlines);
  }
  else if (0 == isBignode(mpis->rank,arguments))
  {
    mpis->bignode = 0;
    //2 extra for ghostlines
    mpis->localN = ((arguments->N / mpis->worldsize)+ghostlines);
  }
  mpis->counts = allocateMemory(mpis->worldsize * sizeof(int));
  mpis->displ = allocateMemory(mpis->worldsize * sizeof(int));
  mpis->glines = allocateMemory(mpis->worldsize * sizeof(int));
  int lines_per_node = arguments->N / mpis->worldsize;
  int number_big_nodes = arguments->N % mpis->worldsize;
  
  for (j = 0; j < mpis->worldsize; j++)
  {
	  ghostlines = ((0 == j)||(j == (mpis->worldsize -1)) ? 1 : 2);
	  mpis->glines[j] = ghostlines;
    if (1 == isBignode(j,arguments))
    {
      mpis->counts[j] = ((arguments->N / mpis->worldsize)+ 1 + ghostlines);
      //(number of nodes so far (j) times the lines per node) plus 1 for every node so far (big nodes)
      //minus 1 for extra line in calculation
      mpis->displ[j] = ((j * (arguments->N / mpis->worldsize)) + j) - ((0 == j) ? 0 : 1);
    }else if (0 == isBignode(j,arguments))
    {
      mpis->counts[j] = (arguments->N / mpis->worldsize) + ghostlines;
      // all bigger nodes + all small nodes so far - 1 (see above)
      // all bigger nodes = (arguments->N % mpis.worldsize)*(arguments->N + 1)
      // all small nodes so far = ((j + 1) - (arguments->N % mpis.worldsize))*(arguments->N + 1)
      mpis->displ[j] = ((((number_big_nodes * lines_per_node) + number_big_nodes)
      + ((j - number_big_nodes) * lines_per_node)) - ((0 == j) ? 0 : 1));
    }
  }
  if (mpis->worldsize == 1)
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
  AskParams(&options, argc, argv);                    /* get parameters */   
  initVariables(&arguments, &results, &options);           /* ******************************************* */
  initMPI(&mpis, &arguments);	                     /* initalize MPI */
  allocateMatrices(&arguments);                            /*  get and initialize variables and matrices  */
  initMatrices(&arguments, &options);                      /* ******************************************* */
  
  gettimeofday(&start_time, NULL);                   /*  start timer         */
  calculate(&arguments, &results, &options);         /*  solve the equation  */
  gettimeofday(&comp_time, NULL);                    /*  stop timer          */
  if (0 == mpis.rank)
  {
    displayStatistics(&arguments, &results, &options);               /* **************** */
    DisplayMatrix("Matrix:",                                         /*  display some    */
		  arguments.Matrix[results.m][0], options.interlines);       /*  statistics and  */
  }
  freeMatrices(&arguments);
  freeMPI(&mpis);                                                      /*  free memory     */
  /* **************** */
  //MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
