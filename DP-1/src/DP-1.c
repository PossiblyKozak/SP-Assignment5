/*	FILE			: DP-1.c
 *	PROJECT			: PROG1970 - Assignment #5
 *	PROGRAMMER		: Alex Kozak
 *	FIRST VERSION	: 2018-08-06
 *	DESCRIPTION		: The DP-1 program is designed to create a block of shared memory to contain a CircularBuffer object
 *				      to be used by all associated programs. It also creates a semaphore meant to allow for writing blocking
 *					  and spin up its own system to add to the circular buffer a block of 20 (DP1_NUM_CHARACTERS) characters 
 * 					  every 2 (DP1_DELAY) seconds. This starts the DP-2 process with the sharedMemoryID as the only command
 * 					  line argument.
 */

#include "../../Common/inc/Common.h"

 // Prototypes
int  getSemaphore(int numSemaphores);
int  getSharedMemory(size_t);
void INThandler(int sig);
void writeArrayToBuffer(char* , CircularBuffer*);

int getSemaphore(int numSemaphores)
{
	// FUNCTION		: getSemaphore
	// DESCRIPTION	: This function gets/opens a Semaphore given by semget() using the constants
	//				  SEMAPHORE_LOCATION and SEMAPHORE_KEY found in Common.h
	// PARAMETERS	:	
	//	  int numSemaphores	: The number of Semaphores to be created
	// RETURNS		: 
	//    int				: The ID for accessing the Semaphore

	int semaphoreID;

	// Get the Semaphore key
   	key_t semaphoreKey = ftok(SEMAPHORE_LOCATION, SEMAPHORE_KEY); // REPLACE WITH SOMETHING ELSE
   	// Check to see if Semaphore already exists 
   	if ((semaphoreID = semget(semaphoreKey, numSemaphores, 0)) == -1)
   	{
   		// Semaphore doesn't exist yet thus create it
     	semaphoreID = semget(semaphoreKey, numSemaphores, (IPC_CREAT | 0660));
   	}
   	return semaphoreID;
}

int getSharedMemory(size_t size)
{
	// FUNCTION		: getSharedMemory
	// DESCRIPTION	: This function gets/opens a shared memory location given by shmget() using the constants
	//				  called SHARED_MEM_LOCATION and SHARED_MEM_KEY found in Common.h
	// PARAMETERS	:	
	//	  size_t	size	: The desired size for the Shared Memory segment
	// RETURNS		: 
	//    int				: The ID for accessing the Shared Memory location

	int sharedMemoryID;

	// Get the Shared Memory key
   	key_t sharedMemoryKey = ftok(".", 'A'); // REPLACE WITH SOMETHING ELSE
   	// Check to see if Shared Memory already exists 
   	if ((sharedMemoryID = shmget(sharedMemoryKey, sizeof(CircularBuffer), 0)) == -1)
   	{
   		// Shared Memory doesn't exist yet thus create it
     	sharedMemoryID = shmget(sharedMemoryKey, sizeof(CircularBuffer), (IPC_CREAT | 0660));
   	}
   	return sharedMemoryID;
}

void INThandler(int sig)
{
	// FUNCTION		: INThandler
	// DESCRIPTION	: This function handles the SIGINT signal by closing the program
	// PARAMETERS	:	
	//	  int sig 	: The signal given
	// RETURNS		: 
	//    VOID

	signal(sig, SIG_IGN);
	exit(0);
}

void writeArrayToBuffer(char* randArray, CircularBuffer *circularBuffer)
{
	// FUNCTION		: writeArrayToBuffer
	// DESCRIPTION	: This function writes a given char array to the buffer of the passed in
	//				  CircularBuffer pointer
	// PARAMETERS	:	
	//	  char* randArray					: The array of characters to be added to the buffer 
	// 	  CircularBuffer *circularBuffer 	: The struct containing the buffer
	// RETURNS		: 
	//    VOID

	// Get the circularBuffer's read and write indecies into more easily ruser pointers
	int *rx = &(circularBuffer->readIndex);
	int *wx = &(circularBuffer->writeIndex);

	// Attempt to claim the semaphore for use
	if (semop(circularBuffer->semaphoreID, &acquire_operation, 1) == -1)
	{
		// In the case that it is being used, print an error message and retry
		printf("Error getting semaphore, retrying in %0.2f seconds\n", SEMAPHORE_DELAY/10000);
		usleep(SEMAPHORE_DELAY);
		writeArrayToBuffer(randArray, circularBuffer);
	}
	else
	{
		// Succesfully gained access to the semaphore
		for (int i = 0; i < strlen(randArray); i++)
		{
			// Confirm there is space in the buffer
			if (!(*wx + 1 == *rx || (*wx == BUFFER_SIZE - 1 && *rx == 0)))
			{
				// Set the character at the write index to randArray
				memmove(&(circularBuffer->buffer[*wx]), &randArray, sizeof(randArray));	

				// check to see if the write index requires wrapping back to the start of the list or not
				if(*wx == BUFFER_SIZE - 1)
				{
					// If yes, back to the 0 index
					*wx = 0;
				}
				else
				{
					// If not, continue one furthur down the buffer
					(*wx)++;
				}			
			}
		}
		// free up the semaphore for future use
		//printf("DP-1 Releasing Semaphore...\n");
		semop(circularBuffer->semaphoreID, &release_operation, 1);
	}
}

int main(int argc, char *argv[])
{
	// Set the event handler for the SIGINT signal to the program and seed the random number generator
	signal(SIGINT, INThandler);
	srand(time(NULL) + 1);
	
	int sharedMemoryID, semaphoreID;	// Shared Memory ID, Semaphore ID	
	CircularBuffer *circularBuffer;		// CircularBuffer object stored in shared memory							

	// get the Shared Memory and Semaphore IDs
	sharedMemoryID = getSharedMemory(sizeof(CircularBuffer) + 1);
	semaphoreID    = getSemaphore(1);

	// if the Shared Memory or Semaphore failed to create, exit the program with an error code
	if (sharedMemoryID == -1 || semaphoreID == -1) { return 1; }

	// Get the shared memory space for the CircularBuffer object
	circularBuffer = (CircularBuffer*)shmat(sharedMemoryID, NULL, 0);

	// Set the value of the semaphore to unused
	semctl(semaphoreID, 0, SETVAL, 1);

	// Set the default values of the CircularBuffer object and clear any data associated
	circularBuffer->semaphoreID = semaphoreID;		
	circularBuffer->readIndex = 0;
	circularBuffer->writeIndex = 0;
	memset(circularBuffer->buffer, 0, sizeof(circularBuffer->buffer));

	// Generate a string to pass through the command line to the DP-2 program
	char shmIDString[ARGS_BUFFER_SIZE];
	sprintf(shmIDString, "%d", sharedMemoryID);

	// Build the argument list
	const char *args[ARGS_BUFFER_SIZE] = {"./DP-2", shmIDString , NULL};

	// fork and run the DP-2 process with the arguments given
    if (0 == fork()) 
    {
        if (-1 == execve(args[0], (char **)args	 , NULL)) 
        {
            perror("child process execve failed");
            return -1;
        }
    } 

	while (1)
	{
		//generate random letter from 'A' to 'T'
		char randArray[DP1_NUM_CHARACTERS + 1];
		memset(randArray,0, sizeof(randArray));
		for (int i = 0; i < DP1_NUM_CHARACTERS; i++)
		{
			randArray[i] = (rand() % NUM_LETTERS) + START_LETTER;
		}
		randArray[DP1_NUM_CHARACTERS] = 0;

		// Add the array to the circular buffer
		writeArrayToBuffer(randArray, circularBuffer);

		//sleep for 2 (DP1_DELAY) seconds
		sleep(DP1_DELAY);
	}	

	// Close the Shared Memory
	shmctl(sharedMemoryID, IPC_RMID, NULL);
	shmdt(circularBuffer);
	return 0;
}