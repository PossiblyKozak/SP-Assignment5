/*	FILE			: DP-2.c
 *	PROJECT			: PROG1970 - Assignment #5
 *	PROGRAMMER		: Alex Kozak
 *	FIRST VERSION	: 2018-08-06
 *	DESCRIPTION		: The DP-2 program is designed to be started by the DP-1 program, and is similar in nature aside from 
 * 					  a few key points. The DP-2 program adds a single character to the circular buffer every 0.05 (DP2_DELAY)
 * 					  seconds. This program also spawns the DC application, with the sharedMemoryID aswell as the pid and ppid
 * 					  of the process as command line arguments.
 */

#include "../../Common/inc/Common.h"

// Prototypes
int runProgram(char** args);
void INThandler(int sig);
void writeCharToBuffer(char randChar, CircularBuffer *circularBuffer);

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

void writeCharToBuffer(char randChar, CircularBuffer *circularBuffer)
{
	// FUNCTION		: writeCharToBuffer
	// DESCRIPTION	: This function writes a given char to the buffer of the passed in
	//				  CircularBuffer pointer
	// PARAMETERS	:	
	//	  char randChar						: The character to be added to the buffer 
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
		printf("Error getting semaphore\n");
		usleep(RETRY_DELAY);
		writeCharToBuffer(randChar, circularBuffer);
	}
	else	// Succesfully gained access to the semaphore
	{		
		// Confirm there is space in the buffer
		if (!(*wx + 1 == *rx || (*wx == BUFFER_SIZE - 1 && *rx == 0)))
		{
			// Set the character at the write index to randChar
			memmove(&(circularBuffer->buffer[*wx]), &randChar, sizeof(randChar));	

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
		// free up the semaphore for future use
		//printf("DP-2 Releasing Semaphore...\n");
		semop(circularBuffer->semaphoreID, &release_operation, 1);
	}
}

int main(int argc, char *argv[], char *envp[])
{
	printf("Running DP-2\n");

	// Declare variables
	CircularBuffer *circularBuffer;

	// Set the event handler for the SIGINT signal to the program and seed the random number generator
	signal(SIGINT, INThandler);
	srand(time(NULL));

	// Parse the sharedMemoryID from the command arguments passed from DP-1
	int sharedMemoryID = atoi(argv[1]);

	// Get the parent PID and own PID and store it in a string
	char ppid[ARGS_BUFFER_SIZE], pid[ARGS_BUFFER_SIZE];
	sprintf(ppid, "%d", getppid());
	sprintf(pid, "%d", getpid());

	// Build the arguments array to spawn the DC program
	const char *args[ARGS_BUFFER_SIZE] = {"./DC", argv[1], ppid, pid, NULL};

	// fork and run the DC process with the arguments given
    if (0 == fork()) 
    {
        if (-1 == execve(args[0], (char **)args	 , NULL)) 
        {
            perror("child process execve failed");
            return -1;
        }
    }   

	// attach to the shared memory space given from DP-1
	circularBuffer = (CircularBuffer*)shmat(sharedMemoryID, NULL, 0);		
	
	while (1)
	{
		//generate random letter from 'A' to 'T'
		char randChar = (rand() % NUM_LETTERS) + START_LETTER;

		//Add it to the circular buffer
		writeCharToBuffer(randChar, circularBuffer);

		//sleep for 1/20 (DP2_DELAY) of a second
		usleep(DP2_DELAY);
	}

	// Close the Shared Memory
	shmctl(sharedMemoryID, IPC_RMID, NULL);
	return 0;
}