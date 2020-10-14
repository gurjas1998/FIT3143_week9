/* Gets the neighbors in a cartesian communicator
* Orginally written by Mary Thomas
* - Updated Mar, 2015
* Link: https://edoras.sdsu.edu/~mthomas/sp17.605/lectures/MPI-Cart-Comms-and-Topos.pdf
* Modifications to fix bugs, include an async send and receive and to revise print output
*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1 // DISP == displacement, ie. DISP = 1 then immediate neighbour right next to current rank

// Function prototype
void WriteToFile(char *pFilename, char *pMessage);

int main(int argc, char *argv[]) {
	// ndims=2 is a 2D ...grid?
	int ndims=2, size, my_rank, reorder, my_cart_rank, ierr;
	int nrows, ncols;
	int nbr_i_lo, nbr_i_hi; // nbr = neighbourhood (does he just mean neighbour), i = row, lo = left? hi = right
	int nbr_j_lo, nbr_j_hi; // j = column, lo/hi is top/bottom but idk which one
	MPI_Comm comm2D;
	int dims[ndims],coord[ndims];
	int wrap_around[ndims];
	
	/* start up initial MPI environment */
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	
	/* process command line arguments*/
	if (argc == 3) {
		nrows = atoi (argv[1]); // these are command line arguments
		ncols = atoi (argv[2]); // so user can specify dimensions
		dims[0] = nrows; /* number of rows */ 
		dims[1] = ncols; /* number of columns */
		if( (nrows*ncols) != size) {
			if( my_rank ==0) printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", nrows, ncols, nrows*ncols,size);
			MPI_Finalize(); 
			return 0;
		}
	} else {
		nrows=ncols=(int)sqrt(size);
		dims[0]=dims[1]=0;
	}
	
	/*************************************************************/
	/* create cartesian topology for processes */
	/*************************************************************/
	MPI_Dims_create(size, ndims, dims); // creates a grid with dimensions we've specified
	if(my_rank==0)
		printf("Root Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d] \n",my_rank,size,dims[0],dims[1]);
	
	/* create cartesian mapping */
	wrap_around[0] = 0;
	wrap_around[1] = 0; /* periodic shift is .false. not circular. this means the grid is 'flat'
                        ie. a rank on very top of the grid is NOT neighbours with one on the bottom. 
                    	To make it circular, set to 1 (or, True)*/
	reorder = 1; /* if false (0), rank of each process in new group is identical to 
	            its rank in the old group. by default we should put 1? */
	ierr =0; // what is the point of this line lol
	ierr = MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, wrap_around, reorder, &comm2D);
	if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);
	
	/* find my coordinates in the cartesian communicator group */
	MPI_Cart_coords(comm2D, my_rank, ndims, coord); // coordinated is returned into the coord array
	/* use my cartesian coordinates to find my rank in cartesian group*/
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);
	/* get my neighbors; axis is coordinate dimension of shift */
	/* axis=0 ==> shift along the rows: P[my_row-1]: P[me] : P[my_row+1] */
	/* axis=1 ==> shift along the columns P[my_col-1]: P[me] : P[my_col+1] */
	
	// Cart shift 'returns the shifted source and destination ranks, given a shift direction and amount
	// vishnu says 'it tells us who our neighbourhood nodes are'
	MPI_Cart_shift( comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi ); // note to pass in comm2D and NOT MPI_COMM_WORLD
	MPI_Cart_shift( comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi ); // not sure why shift_row is 0 and shift_col is 1
                                                                     // DISP == displacement, ie. DISP = 1 then immediate neighbour right next to current rank
	
	//printf("Global rank: %d. Cart rank: %d. Coord: (%d, %d). Left: %d. Right: %d. Top: %d. Bottom: %d\n", my_rank, my_cart_rank, coord[0], coord[1], nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi);
	//fflush(stdout);
	
	/*
	int bCastVal = -1;
	if(my_cart_rank == 4){
		bCastVal = 400;
	}
	MPI_Bcast(&bCastVal, 1, MPI_INT, 4, comm2D);
	printf("Global rank: %d. Cart rank: %d. Coord: (%d, %d). BCast Value: %d\n", my_rank, my_cart_rank, coord[0], coord[1], bCastVal);
	fflush(stdout);
	*/
	
	// the following seems to be for question 2
	
    MPI_Request send_request[4];
    MPI_Request receive_request[4];
    MPI_Status send_status[4];
    MPI_Status receive_status[4];
	
	sleep(my_rank); // puts a sleep because if they all run at once, they'll generate the same number due to the time(NULL)
    unsigned int seed = time(NULL);
	bool isPrime = false;
	int randomVal;
	int k;
	while (!isPrime) { // run this loop til it generates a prime number
	    int i = rand_r(&seed) % 4 + 1;
	    //printf("Rank: %d, i = %d\n", my_rank, i);
	    if(i > 1){
            int sqrt_i = sqrt(i) + 1;
            for (int j = 2; j <=sqrt_i; j ++) {
                k = j;    
                //printf("Rank: %d, k = %d\n", my_rank, k);
                if(i%j == 0) {
                    break;
                }
            }
            if(k >= sqrt_i){
                isPrime = true;
                randomVal = i;
            }
	    }
	}
	
	/* each rank will now send its random generated value to its neighbours */
	// vishnu says we should put this into a for loop if we can, but it's okay to just leave it like this
	MPI_Isend(&randomVal, 1, MPI_INT, nbr_i_lo, 0, comm2D, &send_request[0]);
	MPI_Isend(&randomVal, 1, MPI_INT, nbr_i_hi, 0, comm2D, &send_request[1]);
	MPI_Isend(&randomVal, 1, MPI_INT, nbr_j_lo, 0, comm2D, &send_request[2]);
	MPI_Isend(&randomVal, 1, MPI_INT, nbr_j_hi, 0, comm2D, &send_request[3]);
	
	/* initialise variables to store numbers a rank will receive from each of its neighbours.
	since the grid is not circular, it'll not receive from circular neighbours */
	int recvValL = -1, recvValR = -1, recvValT = -1, recvValB = -1;
	MPI_Irecv(&recvValT, 1, MPI_INT, nbr_i_lo, 0, comm2D, &receive_request[0]);
	MPI_Irecv(&recvValB, 1, MPI_INT, nbr_i_hi, 0, comm2D, &receive_request[1]);
	MPI_Irecv(&recvValL, 1, MPI_INT, nbr_j_lo, 0, comm2D, &receive_request[2]);
	MPI_Irecv(&recvValR, 1, MPI_INT, nbr_j_hi, 0, comm2D, &receive_request[3]);
	
	MPI_Waitall(4, send_request, send_status);
	MPI_Waitall(4, receive_request, receive_status);

	printf("Global rank: %d. Cart rank: %d. Coord: (%d, %d). Random Val: %d. Recv Top: %d. Recv Bottom: %d. Recv Left: %d. Recv Right: %d.\n", my_rank, my_cart_rank, coord[0], coord[1], randomVal, recvValT, recvValB, recvValL, recvValR);
	
	/* Now we compare all received numbers with its own. According to the lab specs, "if ALL prime numbers are the same, the process logs this info". Going with this assumption,
	   a process' generated prime number must match ALL the numbers it receives. Otherwise, it will not log anything, but a text file will still be created. */
	char message[200];
	char file_name[20];
	if((recvValL == randomVal || recvValL == -1) && (recvValR == randomVal || recvValR == -1) && (recvValT == randomVal || recvValT == -1) && (recvValB == randomVal || recvValB == -1)) {
	    snprintf(message, 200, "All prime numbers received match my generated number, which is %d.\n My rank: %d. Received %d from rank: %d, %d from rank: %d, %d from rank: %d and %d from rank: %d.", randomVal, my_rank, recvValT, nbr_i_lo, recvValB, nbr_i_hi, recvValL, nbr_j_lo, recvValR, nbr_j_hi);
	    printf("Rank: %d has all matches with its neighbours.\n", my_rank);
	}
	else{
	    snprintf(message, 200, "Didn't receive matching prime numbers.\n My rank: %d and number: %d. Received %d from rank: %d, %d from rank: %d, %d from rank: %d and %d from rank: %d.", my_rank, randomVal, recvValT, nbr_i_lo, recvValB, nbr_i_hi, recvValL, nbr_j_lo, recvValR, nbr_j_hi);

	}
    snprintf(file_name, 20, "process_%d.txt", my_rank);
    WriteToFile(file_name, message);
    
    
	MPI_Comm_free( &comm2D );
	MPI_Finalize();
	return 0;
}

void WriteToFile(char *pFilename, char *pMessage)
{
	FILE *pFile = fopen(pFilename, "w");
	fprintf(pFile, "%s", pMessage);
	fclose(pFile);
}
