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
int writeCharToBuffer(char randChar, CircularBuffer *circularBuffer);
void INThandler(int sig);

int main(int argc, char *argv[], char *envp[])
{
	signal(SIGINT, INThandler);
	srand(time(NULL));
	CircularBuffer *circularBuffer;
	int sharedMemoryID = atoi(argv[1]);									// Message ID

	// Spawn the DC client and pass it the DP-1 and DP-2 PID's
	char ppid[64];
	sprintf(ppid, "%d", getppid());

	char pid[64];
	sprintf(pid, "%d", getpid());

	const char *args[64] = {"./DC", argv[1], ppid, pid, NULL};
    pid_t   my_pid;
    int     status;
    if (0 == (my_pid = fork())) {
            if (-1 == execve(args[0], (char **)args , NULL)) {
                    perror("child process execve failed [%m]");
                    return -1;
            }
    }

	circularBuffer = (CircularBuffer*)shmat(sharedMemoryID, NULL, 0);		

	while (1)
	{
		//generate random letter from 'A' to 'T'
		char randChar = rand() % 20 + 65;

		//Add it to the circular buffer
		writeCharToBuffer(randChar, circularBuffer);

		//sleep for 1/20 of a second
		usleep(50000);
	}	
	
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

int writeCharToBuffer(char randChar, CircularBuffer *circularBuffer)
{
	int *rx = &(circularBuffer->readIndex);
	int *wx = &(circularBuffer->writeIndex);


	if (semop(circularBuffer->semaphoreID, &acquire_operation, 1) == -1)
	{
		printf("Error getting semaphore\n");
		usleep(50000);
		writeCharToBuffer(randChar, circularBuffer);
	}
	else
	{
		if (*wx + 1 == *rx || (*wx == 255 && *rx == 0))
		{
			printf("BUFFER FULL (DP-2), sleeping for 2 seconds (read: %d, write: %d)", *rx, *wx);
			sleep(2);
			writeCharToBuffer(randChar, circularBuffer);
		}
		memmove(&(circularBuffer->buffer[*wx]), &randChar, sizeof(randChar));	
		if(*wx == 255)
		{
			*wx = 0;
		}
		else
		{
			(*wx)++;
		}
		semop(circularBuffer->semaphoreID, &release_operation, 1);
	}

	//printf("currentBuffer: %s\n", &circularBuffer->buffer[*rx]);

	// get the current read and write indecies
	// if the write isn't going to equal the read, set the current index to randCharacter and increment
	// if 256 after increment, set back to index 0.
	return 0;
}