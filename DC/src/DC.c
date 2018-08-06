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

void convertIntToSymbol(int num);
void readBuffer();
void printHistogram();
void INThandler(int sig);

int pidDP1, pidDP2;
CircularBuffer *circularBuffer;	
int frequenyCount[20], sharedMemoryID, secondsPassed = 0;

int main(int argc, char *argv[])
{
	signal(SIGINT, INThandler);
	memset(frequenyCount, 0, sizeof(frequenyCount));
	//SsaredMemoryID is passed from DP-2
	sharedMemoryID = atoi(argv[1]);									// Message ID
	pidDP1 = atoi(argv[2]);
	pidDP2 = atoi(argv[3]);

	// confirm this is successful otherwise rinse and repeat after waiting 10 seconds
	while (*((int*)(circularBuffer = (CircularBuffer*)shmat(sharedMemoryID, NULL, 0))) == -1){ sleep(10); }	
	signal(SIGALRM, readBuffer);
	alarm(2);
	// MAIN BODY CODE
	while (1)
	{
		if (secondsPassed == 10)
		{	
			printHistogram();
			secondsPassed = 0;
		}
	}
	return 0;
}

void INThandler(int sig)
{
    signal(sig, SIG_IGN);
	kill(pidDP1, SIGINT);
	kill(pidDP2, SIGINT);

	readBuffer();
	printHistogram();
	printf("Shazam !!\n");
	exit(0);
}

void readBuffer()
{
	secondsPassed += 2;
	int* rx = &circularBuffer->readIndex;
	int* wx = &circularBuffer->writeIndex;
	while (*rx != *wx)
	{
		frequenyCount[circularBuffer->buffer[*rx] - 65]++;
		if(*rx == 255)
		{
			*rx = 0;
		}
		else
		{
			(*rx)++;
		}
	}
	signal(SIGALRM, readBuffer);
	alarm(2);
}

void printHistogram()
{
	printf("\n==================================================\n");		
	for (int i = 0; i < 20; i++)
	{
		printf("%c: ", i + 65);
		convertIntToSymbol(frequenyCount[i]);
	}
	printf("==================================================\n");	
}

void convertIntToSymbol(int num)
{
	for (int i = 0; i < (num % 1000) / 100; i++)
		printf("*");
	for (int i = 0; i < (num % 100) / 10; i++)
		printf("+");
	for (int i = 0; i < num % 10; i++)
		printf("-");
	printf("\n");
}