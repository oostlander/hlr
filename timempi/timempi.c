#include <mpi.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

int main(int argc,char* argv[])
{
  int numtasks, rank, rc;
  int micros=35;
	
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
	
	/* Make sure we have at least 2 processes(need at least that much). */
	if (numtasks < 2) 
	  {
	    fprintf(stderr, "World size must be at least two for %s to run properly!\n", argv[0]);
	    MPI_Abort(MPI_COMM_WORLD, 1); 
	  }
	/* get hostname */
	char hostname[MPI_MAX_PROCESSOR_NAME];
	int resultlength=0;
	MPI_Get_processor_name(hostname,&resultlength);
	/* get current time */
	struct tm *Tm;
	struct timeval detail_time;
	time_t timer = time(NULL);
	Tm=localtime(&timer);
	gettimeofday(&detail_time,NULL);
	micros = detail_time.tv_usec;
	/* workernodes do */
	if (rank != 0)
	  {
	    /* make formatted string from time */
	    char timestamp[60];
	    snprintf(timestamp,60,"%s(%d):%d %d %d, %d:%d:%d and %ldms\n",
		     //Tm->tm_wday, /* Mon - Sun */
		     hostname,
		     rank,
		     Tm->tm_mday,
		     Tm->tm_mon+1,
		     Tm->tm_year+1900,
		     Tm->tm_hour,
		     Tm->tm_min,
		     Tm->tm_sec,
		     (long) detail_time.tv_usec); /* /1000 for ms */

	    /* send timestamp to Master */
	    MPI_Send(timestamp, 60, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
	  }else if (rank == 0)
	  {
	    printf("The masternode recieved the following timestamps:\n");
	    /* print recieved messages */
	    char buf[60];
	    for (int i = 1; i < numtasks; i++)
	      {
		MPI_Recv(buf, 60, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		fprintf(stdout, "%s", buf);
	      }
	  }
      	MPI_Barrier(MPI_COMM_WORLD);
	fprintf(stdout,"Rang %d beendet jetzt!\n",rank);
	/* finalize the MPI environment */
	MPI_Finalize();
}
