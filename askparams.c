/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**  	      	   TU Muenchen - Institut fuer Informatik                  **/
/**                                                                        **/
/** Copyright: Dr. Thomas Ludwig                                           **/
/**            Thomas A. Zochler                                           **/
/**                                                                        **/
/** File:      askparams.c                                                 **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/** Beschreibung der Funktion AskParams():                                 **/
/**                                                                        **/
/** Die Funktion AskParams liest sechs Parameter (Erkl"arung siehe unten)  **/
/** entweder von der Standardeingabe oder von Kommandozeilenoptionen ein.  **/
/**                                                                        **/
/** Ziel dieser Funktion ist es, die Eingabe der Parameter sowohl inter-   **/
/** aktiv als auch als Kommandozeilenparameter zu erm"oglichen.            **/
/**                                                                        **/
/** F"ur die Parameter argc und argv k"onnen direkt die vom System         **/
/** gelieferten Variablen der Funktion main verwendet werden.              **/
/**                                                                        **/
/** Beispiel:                                                              **/
/**                                                                        **/
/** int main ( int argc, char **argv )                                     **/
/** {                                                                      **/
/**   ...                                                                  **/
/**   AskParams( ..., argc, argv );                                        **/
/**   ...                                                                  **/
/** }                                                                      **/
/**                                                                        **/
/** Dabei wird argv[0] ignoriert und weiter eingegebene Parameter der      **/
/** Reihe nach verwendet.                                                  **/
/**                                                                        **/
/** Falls bei Aufruf von AskParams() argc < 2 "ubergeben wird, werden      **/
/** die Parameter statt dessen von der Standardeingabe gelesen.            **/
/****************************************************************************/
/** int *method;                                                           **/
/**         Bezeichnet das bei der L"osung der Poissongleichung zu         **/
/**         verwendende Verfahren ( Gauss-Seidel oder Jacobi ).            **/
/** Werte:  METH_GAUSS_SEIDEL  oder METH_JACOBI (definierte Konstanten)    **/
/****************************************************************************/
/** int *interlines:                                                       **/
/**         Gibt die Zwischenzeilen zwischen den auszugebenden             **/
/**         neun Zeilen an. Die Gesamtanzahl der Zeilen ergibt sich als    **/
/**         lines = 8 * (*interlines) + 9. Diese Art der Berechnung der    **/
/**         Problemgr"o"se (auf dem Aufgabenblatt mit N bezeichnet)        **/
/**         wird benutzt, um mittels der Ausgaberoutine DisplayMatrix()    **/
/**         immer eine "ubersichtliche Ausgabe zu erhalten.                **/
/** Werte:  0 < *interlines                                                **/
/****************************************************************************/
/** int *func:                                                             **/
/**         Bezeichnet die St"orfunktion ( I oder II ) und damit auch      **/
/**         die Randbedingungen.                                           **/
/** Werte:  FUNC_F0: f(x,y)=0, 0<x<1, 0<y<1                                 **/
/**         FUNC_FPISIN: f(x,y)=2pi^2*sin(pi*x)sin(pi*y), 0<x<1, 0<y<1          **/
/****************************************************************************/
/** int *termination:                                                      **/
/**         Gibt die Art der Abbruchbedingung an.                          **/
/** Werte:  TERM_PREC: Abbruchbedingung ist die Genauigkeit der bereits       **/
/**                 berechneten N"aherung. Diese soll unter die            **/
/**                 Grenze term_precision kommen.                          **/
/**         TERM_ITER: Abbruchbedingung ist die Anzahl der Iterationen. Diese **/
/**                 soll gr"o"ser als term_iteration sein.                 **/
/****************************************************************************/
/** double *term_precision:                                                **/
/** int t*erm_iteration:                                                   **/
/**         Es wird jeweils nur einer der beiden Parameter f"ur die        **/
/**         Abbruchbedingung eingelesen.                                   **/
/****************************************************************************/

#include "partdiff-seq.h"
#include <string.h>

void AskParams( struct options* options, int argc, char** argv )
{
	printf ( "\n");
	printf ( "============================================================\n"  );
	printf ( "Program for calculation of partial differential equations.  \n" );
	printf ( "============================================================\n"  );
	printf ( "(c) Dr. Thomas Ludwig, TU München.\n");
	printf ( "    Thomas A. Zochler, TU München.\n");
	printf ( "    Andreas C. Schmidt, TU München.\n");
	printf ( "============================================================\n"  );

	if( argc < 2 )
	{
		/* ----------------------------------------------- */
		/* Get input: method, interlines, func, precision. */
		/* ----------------------------------------------- */
		int ret = 0;
		do
		{
			printf ( "\n" );
			printf("Select number of threads:\n");
			printf("Number> ");
			fflush( stdout );
			ret = scanf("%d", &(options->number));
		}
		while ( (options->number < 0) );
		do
		{
			printf ( "\n" );
			printf( "Select calculationmethod:\n");
			printf( "  %1d: Gauss-Seidel.\n", METH_GAUSS_SEIDEL);
			printf( "  %1d: Jacobi.\n",       METH_JACOBI);
			printf( "method> ");
			fflush( stdout );
			ret = scanf("%d", &(options->method));
		}
		while ( (options->method < METH_GAUSS_SEIDEL) || (options->method > METH_JACOBI) );
		do
		{
			printf ( "\n" );
			printf("Matrixsize = Interlines*8+9\n");
			printf("Interlines> ");
			fflush( stdout );
			ret = scanf("%d", &(options->interlines));
		}
		while ( (options->interlines < 0) || (options->interlines > 1000) );
		do
		{
			printf ( "\n" );
			printf("Select interferencefunction:\n");
			printf(" %1d: f(x,y)=0.\n",                        FUNC_F0);
			printf(" %1d: f(x,y)=2pi^2*sin(pi*x)sin(pi*y).\n", FUNC_FPISIN);
			printf("interferencefunction> ");
			fflush( stdout );
			ret = scanf("%d", &(options->inf_func));
		}
		while ( (options->inf_func < FUNC_F0) || (options->inf_func > FUNC_FPISIN) );
		do
		{
			printf ( "\n" );
			printf("Select termination:\n");
			printf(" %1d: sufficient precision.\n",  TERM_PREC);
			printf(" %1d: number of iterationes.\n", TERM_ITER);
			printf("termination> ");
			fflush( stdout );
			ret = scanf("%d", &(options->termination));
		}
		while ( (options->termination < TERM_PREC) || (options->termination > TERM_ITER) );

		if (options->termination == TERM_PREC)
		{
			do
			{
				printf ( "\n" );
				printf("Select precision:\n");
				printf("  Range: 1e-4 .. 1e-20.\n");
				printf("precision> ");
				fflush( stdout );
				ret = scanf("%lf", &(options->term_precision));
			}
			while ( (options->term_precision < 1e-20) || (options->term_precision > 1e-4));

			options->term_iteration = MAX_ITERATION;
		}
		else if (options->termination == TERM_ITER)
		{
			do
			{
				printf ( "\n" );
				printf("Select number of iterations:\n");
				printf("  Range: 1 .. %d.\n", MAX_ITERATION );
				printf("Iterationes> ");
				fflush( stdout );
				ret = scanf("%d", &(options->term_iteration));
			}
			while ( (options->term_iteration < 1) || (options->term_iteration > MAX_ITERATION ));

			options->term_precision = 0;
		}
	}
	else
	{
		if (strcmp("help",(char *)argv[1])==0 || strcmp("-h",(char *)argv[1])==0 ||
		    strcmp("-?",  (char *)argv[1])==0 || strcmp("?", (char *)argv[1])==0 ||
		    argc < 7)
		{
			printf("\nUsage:\n");
			printf("partdiff [num] [method] [lines] [func] [term] [prec/iter]\n");
			printf("  - num:    number of threads to use\n");
			printf("  - method: %1d: Gauss-Seidel.\n", METH_GAUSS_SEIDEL);
			printf("            %1d: Jacobi.\n",       METH_JACOBI);
			printf("  - lines:  (lines=interlines) matrixsize = interlines*8+9\n");
			printf("  - func:   %1d: f(x,y)=0.\n",                        FUNC_F0);
			printf("            %1d: f(x,y)=2pi^2*sin(pi*x)sin(pi*y).\n", FUNC_FPISIN);
			printf("  - term:   %1d: sufficient precision.\n",  TERM_PREC);
			printf("            %1d: number of iterationes.\n", TERM_ITER);
			printf("  - prec/iter: depending on term:\n");
			printf("            precision:  Range: 1e-4 .. 1e-20.\n");
			printf("            iterations: Range: 1 .. %d.\n", MAX_ITERATION );
			printf("\n");
			printf("Example: %s 1 2 100 1 2 100 \n", argv[0]);
			exit(0);
		}

		sscanf( argv[1],"%d", &(options->number));
		sscanf( argv[2],"%d", &(options->method));
		sscanf( argv[3],"%d", &(options->interlines));
		sscanf( argv[4],"%d", &(options->inf_func));
		sscanf( argv[5],"%d", &(options->termination));

		if (options->termination == TERM_PREC)
		{
			sscanf( argv[6],"%lf", &(options->term_precision));
			options->term_iteration = MAX_ITERATION;
		}
		else
		{
			sscanf( argv[6],"%d", &(options->term_iteration));
			options->term_precision = 0;
		}
	}
}
