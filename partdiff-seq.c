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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <sys/time.h>
#include "partdiff-seq.h"


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

/* ************************************************************************ */
/* Global variables                                                         */
/* ************************************************************************ */

/* time measurement variables */
struct timeval start_time;       /* time when program started                      */
struct timeval comp_time;        /* time when calculation completed                */


/* ************************************************************************ */
/* initVariables: Initializes some global variables                         */
/* ************************************************************************ */
static
void
initVariables (struct calculation_arguments* arguments, struct calculation_results* results, struct options* options)
{
	arguments->N = options->interlines * 8 + 9 - 1;
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
	int i, j;

	int N = arguments->N;

	arguments->M = allocateMemory(arguments->num_matrices * (N + 1) * (N + 1) * sizeof(double)); 
	arguments->Matrix = allocateMemory(arguments->num_matrices * sizeof(double**));

	for (i = 0; i < arguments->num_matrices; i++)
	{
		arguments->Matrix[i] = allocateMemory((N + 1) * sizeof(double*)); /* Elementzugriff über Zeiger */

		for (j = 0; j <= N; j++)
		{
			arguments->Matrix[i][j] = (double*)(arguments->M + (i * (N + 1) * (N + 1)) + (j * (N + 1)));
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
	double h = arguments->h;
	double*** Matrix = arguments->Matrix;

	/* initialize matrix/matrices with zeros */
	for (g = 0; g < arguments->num_matrices; g++)
	{
		for (i = 0; i <= N; i++)
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
		for(i = 0; i <= N; i++)
		{
			for (j = 0; j < arguments->num_matrices; j++)
			{
				Matrix[j][i][0] = 1 - (h * i);
				Matrix[j][i][N] = h * i;
				Matrix[j][0][i] = 1 - (h * i);
				Matrix[j][N][i] = h * i;
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

	int N = arguments->N;
	double h = arguments->h;
	double*** Matrix = arguments->Matrix;

	/* initialize m1 and m2 depending on algorithm */
	if (options->method == METH_GAUSS_SEIDEL)
	{
		m1=0; m2=0;
	}
	else
	{
		m1=0; m2=1;
	}

	while (options->term_iteration > 0)
	{
		maxresiduum = 0;
		/* Pointer auf Threads */
		pthread_t threads[options->number]; /* Anzahl der Threads wird festgelegt durch User */
		/* pthread create */
		/* over all rows */
		/* TODO ab hier muss es eine funktion werden die "in place" auf die Variablen zugreift */
		threadCalculate(1,1,N,N,&maxresiduum, );
		for (i = 1; i < N; i++)
		{
			/* over all columns */
			for (j = 1; j < N; j++)
			{
				star = (Matrix[m2][i-1][j] + Matrix[m2][i][j-1] + Matrix[m2][i][j+1] + Matrix[m2][i+1][j]) * 0.25;

				if (options->inf_func == FUNC_FPISIN)
				{
					star = (TWO_PI_SQUARE * sin((double)(j) * PI * h) * sin((double)(i) * PI * h) * h * h * 0.25) + star;
				}

				residuum = Matrix[m2][i][j] - star; /* TODO residuum muss pro Thread gemacht werden */
				residuum = (residuum < 0) ? -residuum : residuum; /* Durch abs ersetzen (weil Prozessor befehle) */
				maxresiduum = (residuum < maxresiduum) ? maxresiduum : residuum; /* TODO maxresiduum muss zu mutex werden */

				Matrix[m1][i][j] = star;
			}
		}
                /* maxresiduum muss gesetzt werden */
		/* pthreadjoin */
		results->stat_iteration++;
		results->stat_precision = maxresiduum;

		/* exchange m1 and m2 */
		//i=m1; m1=m2; m2=i; /* normal swap */
		m1 ^= m2 ^= m2 ^= m1; /* XOR swap */
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
			options->term_iteration--;
		}
	}

	results->m = m2;
}
/* **************************************************************************************************** */
/* Does the parralel part of the work calculate does. Needs to get the parameters as Call by Reference. */
/* **************************************************************************************************** */
static
void
threadCalculate (int i, int j, int max_i, int max_j, double* maxresiduum, int* m1, int* m2, struct options* options, struct calculation_arguments* arguments)
{
  double star;
  double residuum;
  double t_maxresiduum;
  double*** Matrix = arguments->Matrix;
  for (i = 1; i < max_i; i++)
    {
      /* over all columns */
      for (j = 1; j < max_j; j++)
	{
	  star = (Matrix[m2][i-1][j] + Matrix[m2][i][j-1] + Matrix[m2][i][j+1] + Matrix[m2][i+1][j]) * 0.25;

	  if (options->inf_func == FUNC_FPISIN)
	    {
	      star = (TWO_PI_SQUARE * sin((double)(j) * PI * h) * sin((double)(i) * PI * h) * h * h * 0.25) + star;
	    }

	  residuum = Matrix[m2][i][j] - star; /* TODO residuum muss pro Thread gemacht werden */
	  residuum = (residuum < 0) ? -residuum : residuum; /* Durch abs ersetzen (weil Prozessor befehle) */
	  t_maxresiduum = (residuum < t_maxresiduum) ? t_maxresiduum : residuum; 
	  Matrix[m1][i][j] = star;
	}
    }
  /* TODO maxresiduum muss zu mutex werden */
  *maxresiduum = (t_maxresiduum < *maxresiduum) ? *maxresiduum : t_maxresiduum; /* kann man so machen weil max gesucht wird nicht min */
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

/* ************************************************************************ */
/*  main                                                                    */
/* ************************************************************************ */
int
main (int argc, char** argv)
{
	struct options options;
	struct calculation_arguments arguments;
	struct calculation_results results; 

	/* get parameters */
	AskParams(&options, argc, argv);              /* ************************* */

	initVariables(&arguments, &results, &options);           /* ******************************************* */

	allocateMatrices(&arguments);        /*  get and initialize variables and matrices  */
	initMatrices(&arguments, &options);            /* ******************************************* */

	gettimeofday(&start_time, NULL);                   /*  start timer         */
	calculate(&arguments, &results, &options);                                      /*  solve the equation  */
	gettimeofday(&comp_time, NULL);                   /*  stop timer          */

	displayStatistics(&arguments, &results, &options);                                  /* **************** */
	DisplayMatrix("Matrix:",                              /*  display some    */
			arguments.Matrix[results.m][0], options.interlines);            /*  statistics and  */

	freeMatrices(&arguments);                                       /*  free memory     */

	return 0;
}
