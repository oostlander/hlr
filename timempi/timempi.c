#include <mpi.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

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

	/* get current time */
	struct tm *Tm;
	struct timeval detail_time;
	time_t timer = time(NULL);
	Tm=localtime(&timer);
	gettimeofday(&detail_time,NULL);
	char timestamp[60];
	snprintf(timestamp,60,"%d %d %d, %d:%d:%d and %dms\n",
	       //Tm->tm_wday, /* Mon - Sun */
	       Tm->tm_mday,
	       Tm->tm_mon+1,
	       Tm->tm_year+1900,
	       Tm->tm_hour,
	       Tm->tm_min,
	       Tm->tm_sec,
	       (int) detail_time.tv_usec /1000);
	if (rank == 0)
	  {
	    number=-1;
	    MPI_Send(timestamp, 60, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
	  } else if (rank == 1)
	  {
	    MPI_Recv(timestamp, 60, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	    printf("Process 1 recieved number %s from process o\n",timestamp);
	  }
	/****** do some work ******/

	/* finalize the MPI environment */
	MPI_Finalize();
}
