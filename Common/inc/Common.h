/*	FILE			: DC.c
 *	PROJECT			: PROG1970 - Assignment #3
 *	PROGRAMMER		: Alex Kozak and Attila Katona
 *	FIRST VERSION	: 2018-07-14
 *  DESCRIPTION 	: This file holds all the #includes, #define constants and structs used in the
 *  				  Hoochamacallit System. This file is included in DC.c, DX.c and DR.c
 */


#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>

// Maximum number of DCs running at one time
#define MAX_DC_ROLES			10

// The number of valid messageIDs
#define NUM_MESSAGE_IDS		    7

// Possible valid messageIDs
#define EVERYTHING_OKAY			0
#define HYDRAULIC_FAILURE		1
#define SAFETY_BUTTON_FAILURE	2
#define NO_RAW_MATERIAL			3
#define TEMP_OUT_OF_RANGE		4
#define OPERATOR_ERROR			5
#define MACHINE_OFFLINE			6

// WOD indexes which close the Message Queue
#define DELETE_MSGQ_10			10
#define DELETE_MSGQ_17          17

// Sleep Length
#define DC_SLEEP_LENGTH    		10
#define DR_SLEEP_LENGTH			15

// Log filepaths
#define DC_LOG_FILE_PATH		"/tmp/dataCorruptor.log"
#define DR_LOG_FILE_PATH		"/tmp/dataMonitor.log"

// Shared Memory pathing information
#define SHARED_MEM_KEY			11111
#define SHARED_MEM_LOCATION		"."

// Memory Queue pathing information
#define SEMAPHORE_KEY			12345
#define SEMAPHORE_LOCATION		".."

struct sembuf acquire_operation = { 0, -1, SEM_UNDO }; 
struct sembuf release_operation = { 0, 1, SEM_UNDO }; 

typedef struct tagCircularBuffer
{
	int semaphoreID;
	int readIndex;
	int writeIndex;
	char buffer[256];
} CircularBuffer;