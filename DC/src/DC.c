/*	FILE			: DC.c
 *	PROJECT			: PROG1970 - Assignment #5
 *	PROGRAMMER		: Alex Kozak
 *	FIRST VERSION	: 2018-08-06
 *	DESCRIPTION		: The purpose of the DC is to read from and display the data creaded by the 
 *					  DP-1 and DP-2 programs in a histogram, and interpret the SIGINT signal to exit the 
 * 					  program after finishing a read of the buffer. The DC is started by the DP-2 program
 * 					  with command line arguents containing the sharedMemoryID for access to the
 *					  circularBuffer, and the PID's of both DP-1 and DP-2.
 */

#include "../../Common/inc/Common.h"

// Prototypes
void convertIntToSymbol(int num);
void INThandler(int sig);
void printHistogram();
void readBuffer();

// Global variables
CircularBuffer *circularBuffer;	
int frequenyCount[NUM_LETTERS], sharedMemoryID, secondsPassed, pidDP1, pidDP2;

void convertIntToSymbol(int num)
{
	// FUNCTION		: convertIntToSymbol
	// DESCRIPTION	: This function converts a given integer to histogram symbols and prints it to the screen
	// PARAMETERS	:	
	//	  int num 		: The number to be converted to symbols
	// RETURNS		: 
	//    VOID

	// Print '~' for every 1000
	for (int i = 0; i < (num % 10000) / 1000; i++)
		printf("~");

	// Print '*' for every 100
	for (int i = 0; i < (num % 1000) / 100; i++)
		printf("*");
	
	// Print '+' for every 10
	for (int i = 0; i < (num % 100) / 10; i++)
		printf("+");
	
	// Print '-' for every 1
	for (int i = 0; i < num % 10; i++)
		printf("-");
	
	// print a newline to end the symbol reperesentation
	printf("\n");
}

void INThandler(int sig)
{
	// FUNCTION		: INThandler
	// DESCRIPTION	: This function handles the SIGINT signal by closing the program,
	//				  reading from the buffer, printing one last histogram, and killing 
	// 				  the DP-1 and DP-2 processes
	// PARAMETERS	:	
	//	  int sig 	: The signal given
	// RETURNS		: 
	//    VOID

	// Ignore the signal
    signal(sig, SIG_IGN);

    // Kill both DP-1 and DP-2 with the SIGINT signal
	kill(pidDP1, SIGINT);
	kill(pidDP2, SIGINT);

	// read what's still to be read in the buffer and print the final results
	readBuffer();
	printHistogram();

	// Print "Shazam !!" as a final statement
	printf("Shazam !!\n");

	// Exit the program
	exit(0);
}

void printHistogram()
{
	// FUNCTION		: printHistogram
	// DESCRIPTION	: This function prints the histogram from the frequencyCount array
	// PARAMETERS	:	
	//	  VOID
	// RETURNS		: 
	//    VOID

	printf("\n==================================================\n");		// Print a dividing line header
	for (int i = 0; i < NUM_LETTERS; i++) 		// For every letter in the range
	{
		printf("%c: ", i + START_LETTER);		// Print the letter itself
		convertIntToSymbol(frequenyCount[i]);	// call the convertIntToSymbol function to print the formatted int
	}
	printf("==================================================\n");			// Print a dividing line footer
}

void readBuffer()
{
	// FUNCTION		: readBuffer
	// DESCRIPTION	: This function reads from the circular buffer starting at the 
	//				  read index, and reading until it reaches the writing index before
	// 				  finishing its read. It is called on an alarm by SIGALRM, and it also
	//				  resets said alarm so it can read again in future.
	// PARAMETERS	:	
	//	  VOID
	// RETURNS		: 
	//    VOID

	// increment secondsPassed by DC_READ_DELAY which is defined in Common.h as 2
	secondsPassed += DC_READ_DELAY;

	// Get the circularBuffer's read and write indecies into more easily ruser pointers
	int* rx = &circularBuffer->readIndex;
	int* wx = &circularBuffer->writeIndex;

	// Attempt to claim the semaphore for use
	if (semop(circularBuffer->semaphoreID, &acquire_operation, 1) == -1)
	{
		// In the case that it is being used, print an error message and retry
		printf("Error getting semaphore, retrying in %0.2f seconds\n", SEMAPHORE_DELAY/10000);
		usleep(SEMAPHORE_DELAY);
		readBuffer();
	}
	else
	{
		while (*rx != *wx) // until the write catches up to the read, repeat
		{
			// frequencyCount is an array of size 20 (NUM_LETTERS) which corresponds to the letters in the buffer
			// and will increment at each instance of the letter in the buffer.
			frequenyCount[circularBuffer->buffer[*rx] - START_LETTER]++;
			
			// check to see if the read index requires wrapping back to the start of the list or not
			if(*rx == BUFFER_SIZE - 1)
			{
				// If yes, back to the 0 index
				*rx = 0;
			}
			else
			{
				// If not, continue one furthur down the buffer
				(*rx)++;
			}
		}
		// free up the semaphore for future use
		semop(circularBuffer->semaphoreID, &release_operation, 1);	
		
		// Reset the signal to point to readBuffer again, and set an alarm for 2 (DC_READ_DELAY) seconds in the future
		signal(SIGALRM, readBuffer);
		alarm(DC_READ_DELAY);
	}	
}

int main(int argc, char *argv[])
{
	printf("Running DC\n");

	// Set the event handler for the SIGINT signal to the program
	signal(SIGINT, INThandler);

	// Initialize globals required for signal use
	secondsPassed 	= 0;
	sharedMemoryID 	= atoi(argv[1]);
	pidDP1 			= atoi(argv[2]);
	pidDP2 			= atoi(argv[3]);
	memset(frequenyCount, 0, sizeof(frequenyCount));

	// Confirm shared memory is valid, if not repeat after waiting 10(DC_SEMAPHORE_RETRY) seconds
	while (*((int*)(circularBuffer = (CircularBuffer*)shmat(sharedMemoryID, NULL, 0))) == -1) { sleep(DC_SEMAPHORE_RETRY); }	
	
	// Set the signal for reading the buffer every 2 seconds
	signal(SIGALRM, readBuffer);
	alarm(DC_READ_DELAY);

	while (1)	// Repeat until the program exits through SIGINT
	{
		if (secondsPassed >= DC_HISTOGRAM_DELAY)
		{	
			// Print the histogram every 10 (DC_HISTOGRAM_DELAY) seconds
			printHistogram();
			secondsPassed -= DC_HISTOGRAM_DELAY;
		}
	}
	return 0;
}