/*	FILE			: DR.c
 *	PROJECT			: PROG1970 - Assignment #3
 *	PROGRAMMER		: Alex Kozak and Attila Katona
 *	FIRST VERSION	: 2018-07-14
 *	DESCRIPTION		: The data reader (DR) program’s purpose is to monitor its incoming message queue for the varying
 *					  statuses of the different Hoochamacallit machines on the shop floor. It will keep track of the number
 *					  of different machines present in the system, and each new message it gets from a machine, it reports
 *					  it findings to a data monitoring log file. The DR application is solely responsible for creating
 *					  its incoming message queue and when it detects that all of its feeding machines have gone off-line, the
 *					  DR application will free up the message queue, release its shared memory and the DR application will
 *					  terminate.
 */

#include "../../Common/inc/Common.h"

 // Prototypes
int  getSharedMemory(size_t);
int writeCharToBuffer(char* , CircularBuffer*);
int getSemaphore(int numSemaphores);
void INThandler(int sig);

int main(int argc, char *argv[])
{
	signal(SIGINT, INThandler);
	int sharedMemoryID, semaphoreID;										// Message ID, Shared Memory ID	
	// Replace circularBuffer with a struct containing a char array 
	// of size 256 and two ints representing write point and read point
	CircularBuffer *circularBuffer;									

	sharedMemoryID = getSharedMemory(sizeof(CircularBuffer) + 1);
	semaphoreID    = getSemaphore(1);
	semctl(semaphoreID, 0, SETVAL, 1);
	circularBuffer = (CircularBuffer*)shmat(sharedMemoryID, NULL, 0);

	circularBuffer->semaphoreID = semaphoreID;		
	circularBuffer->readIndex = 0;
	circularBuffer->writeIndex = 0;
	memset(circularBuffer->buffer, 0, sizeof(circularBuffer->buffer));

	// if the Shared Memory failed to create, exit the program with an error code
	if (sharedMemoryID == -1 || semaphoreID == -1) { return 1; }

	// ALSO REPLACE THIS MASTERLIST
	//circularBuffer = (CircularBuffer*)shmat(sharedMemoryID, NULL, 0);	

	char shmIDString[64];
	sprintf(shmIDString, "%d", sharedMemoryID);
	const char *args[64] = {"./DP-2", shmIDString , NULL};

    pid_t   my_pid;
    int     status;
    if (0 == (my_pid = fork())) {
            if (-1 == execve("./DP-2", (char **)args	 , NULL)) {
                    perror("child process execve failed [%m]");
                    return -1;
            }
    }

	// Spawn the DP-2 client and pass it the sharedMemoryID

	// MAIN BODY CODE
	while (1)
	{
		//generate random letter from 'A' to 'T'
		char randArray[21];
		for (int i = 0; i < 20; i++)
		{
			randArray[i] = rand() % 20 + 65;
		}
		randArray[20] = 0;
		writeCharToBuffer(randArray, circularBuffer);
		//sleep for 1/20 of a second
		sleep(2);
	}	
	// IN THIS AREA

	// Close the Shared Memory
	shmctl(sharedMemoryID, IPC_RMID, NULL);
	shmdt(circularBuffer);
	return 0;
}

void INThandler(int sig)
{
	signal(sig, SIG_IGN);
	exit(0);
}

int getSemaphore(int numSemaphores)
{
	// FUNCTION		: getSharedMemory
	// DESCRIPTION	: This function gets/opens a shared memory location given by shmget() using the constants
	//				  called SHARED_MEM_LOCATION and SHARED_MEM_KEY found in Common.h
	// PARAMETERS	:	
	//	  size_t	size	: The desired size for the Shared Memory segment
	// RETURNS		: 
	//    int				: The ID for accessing the Shared Memory location

	int semaphoreID;

	// Get the Shared Memory key
   	key_t semaphoreKey = ftok(SEMAPHORE_LOCATION, SEMAPHORE_KEY); // REPLACE WITH SOMETHING ELSE
   	// Check to see if Shared Memory already exists 
   	if ((semaphoreID = semget(semaphoreKey, numSemaphores, 0)) == -1)
   	{
   		// Shared Memory doesn't exist yet thus create it
     	semaphoreID = semget(semaphoreKey, numSemaphores, (IPC_CREAT | 0660));
   	}
   	//printf("semID: %d\nsemKey: %d\n", semaphoreID, semaphoreKey);
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
   	//printf("shmID: %d\nshmKey: %d\n", sharedMemoryID, sharedMemoryKey);
   	return sharedMemoryID;
}

int writeCharToBuffer(char* randArray, CircularBuffer *circularBuffer)
{
	int *rx = &(circularBuffer->readIndex);
	int *wx = &(circularBuffer->writeIndex);

	if (semop(circularBuffer->semaphoreID, &acquire_operation, 1) == -1)
	{
		printf("Error getting semaphore\n");
		usleep(50000);
		writeCharToBuffer(randArray, circularBuffer);
	}
	else
	{
		for (int i = 0; i < strlen(randArray); i++)
		{
			if (*wx + 1 == *rx || (*wx == 255 && *rx == 0))
			{
				printf("BUFFER FULL (DP-1), sleeping for 2 seconds (read: %d, write: %d)", *rx, *wx);
				sleep(2);
				writeCharToBuffer(&randArray[i], circularBuffer);
			}
			memmove(&(circularBuffer->buffer[*wx]), &randArray[i], sizeof(char));	
			if(*wx == 255)
			{
				*wx = 0;
			}
			else
			{
				(*wx)++;
			}
		}
		semop(circularBuffer->semaphoreID, &release_operation, 1);
	}
	// get the current read and write indecies
	// if the write isn't going to equal the read, set the current index to randCharacter and increment
	// if 256 after increment, set back to index 0.
	return 0;
}