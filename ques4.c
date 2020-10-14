#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

//  function to send and recieve data
int master_io(MPI_Comm master_comm, MPI_Comm comm);
int slave_io(MPI_Comm master_comm, MPI_Comm comm);

// tags for 3 different types of message
#define MSG_EXIT 1
#define MSG_PRINT_ORDERED 2
#define MSG_PRINT_UNORDERED 3


int main(int argc, char **argv)
{
    int rank;
    MPI_Comm new_comm;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    MPI_Comm_split( MPI_COMM_WORLD,rank == 0, 0, &new_comm);// creating master and slave
    if (rank == 0) 
	master_io( MPI_COMM_WORLD, new_comm );
    else
	slave_io( MPI_COMM_WORLD, new_comm );
    MPI_Finalize();
    return 0;
}

/* This is the master */
int master_io(MPI_Comm master_comm, MPI_Comm comm)
{
    int        i, size, nslave, firstmsg;
    char       buf[256], buf2[256];
    MPI_Status status;

    MPI_Comm_size( master_comm, &size );
    nslave = size - 1;
    
    
    while (nslave > 0) {  // run till we get exit from all processes
	MPI_Recv( buf, 256, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, 
		  master_comm, &status );  // recieve any message
	switch (status.MPI_TAG) {   // check type of message
	case MSG_EXIT: nslave--; break;
	case MSG_PRINT_UNORDERED:
	    fputs( buf, stdout );  // if we got unordered message just print
	    break;
	case MSG_PRINT_ORDERED:     // if message is ordered store the current message and get message from all other process and print in order 
	    
	    firstmsg = status.MPI_SOURCE;
	    for (i=1; i<size; i++) {
		if (i == firstmsg) // displaying the message we got when it is its turn
		    fputs( buf, stdout );
		else {
		    MPI_Recv( buf2, 256, MPI_CHAR, i, MSG_PRINT_ORDERED, // recieve all other ordered message
			  master_comm, &status );
		    fputs( buf2, stdout );
		}
	    }
	    break;
	}
    }
    return 0;
}

/* This is the slave */
int slave_io(MPI_Comm master_comm, MPI_Comm comm)
{
     char buf[256];
    int  rank;
    
    MPI_Comm_rank( comm, &rank );  // ordered message1
    sprintf( buf, "Hello from slave %d\n", rank );
    MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, 0, MSG_PRINT_ORDERED, 
	      master_comm );
    
    sprintf( buf, "Goodbye from slave %d\n", rank );// ordered message2
    MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, 0, MSG_PRINT_ORDERED, 
	      master_comm );

    sprintf( buf, "I'm exiting (%d)\n", rank );// unordered message 
    MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, 0, MSG_PRINT_UNORDERED, 
	      master_comm );

    MPI_Send( buf, 0, MPI_CHAR, 0, MSG_EXIT, master_comm );// exit signal
    return 0;
}
