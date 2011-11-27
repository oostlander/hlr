#include <mpi.h>
#include <stdio.h>
#include <time.h>

int main(int argc,char* argv[])
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
	//printf ("Number of tasks= %d Myrank= %d\n", numtasks,rank);
	
	// We are assuming at least 2 processes for this task
	if (numtasks != 2) 
	  {
	    fprintf(stderr, "World size must be at least two for %s to run properly!\n", argv[0]);
	    MPI_Abort(MPI_COMM_WORLD, 1); 
	  }
	int namelength=MPI_MAX_PROCESSOR_NAME;
	char hostname[namelength];
	int resultlength=0;
	int number;
	MPI_Get_processor_name(hostname,&resultlength);
	time_t timer = time(NULL);
	if (rank == 0)
	  {
	    number=-1;
	    MPI_Send(&number, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
	  } else if (rank == 1)
	  {
	    MPI_Recv(&number, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	    printf("Process 1 recieved number %d from process o\n",number);
	  }
	/****** do some work ******/

	/* finalize the MPI environment */
	MPI_Finalize();
}
