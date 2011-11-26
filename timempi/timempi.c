#include "mpi.h"
#include <stdio.h>

int main(argc,argv)
int argc;
char *argv[];
{
	int numtasks, rank, rc;
	
	/* initialize MPI and check for success*/
	rc = MPI_Init(&argc,&argv);
	if (rc != MPI_SUCCESS)
	{
		printf ("Error starting MPI programm. Termianting.\n");
		MPI_Abort(MPI_COMM_WORLD, rc);
	}
	
	/* get size of comm and rank in that comm */
	MPI_Comm_size(MPI_COMM_WORLD,&numtasks );
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	printf ("Number of tasks= %d Myrank= %d\n", numtasks,rank);
	
	/****** do some work ******/
	
	MPI_Finalize();
}
