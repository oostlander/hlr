/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**  	      	   TU Muenchen - Institut fuer Informatik                  **/
/**                                                                        **/
/** Copyright: Prof. Dr. Thomas Ludwig                                     **/
/**            Thomas A. Zochler, Andreas C. Schmidt                       **/
/**                                                                        **/
/** File:      partdiff-seq.h                                              **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/* *********************************** */
/* Include some standard header files. */
/* *********************************** */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

/* ************* */
/* Some defines. */
/* ************* */
#ifndef PI
#define PI 			3.141592653589793
#endif
#define TWO_PI_SQUARE 		(2 * PI * PI)
#define MAX_ITERATION  		200000
#define METH_GAUSS_SEIDEL 	1
#define METH_JACOBI 		2
#define FUNC_F0			1
#define FUNC_FPISIN		2
#define TERM_PREC		1
#define TERM_ITER		2

struct options
{
	int     number;         /* Number of threads                              */
	int     method;         /* Gauss Seidel or Jacobi method of iteration     */
	int     interlines;     /* matrix size = interlines*8+9                   */
	int     inf_func;       /* inference function                             */
	int     termination;    /* termination condition                          */
	int     term_iteration; /* terminate if iteration number reached          */
	double  term_precision; /* terminate if precision reached                 */
};

/* *************************** */
/* Some function declarations. */
/* *************************** */
/* Documentation in files      */
/* - askparams.c               */
/* - displaymatrix.c           */
/* *************************** */
void AskParams( struct options*, int, char** );

void DisplayMatrix ( char*, double*, int );

void DisplayMatrixAddr ( char*, double***, int, int );
