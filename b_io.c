/**************************************************************
* Class:  CSC-415-03 Fall 2021
* 
* Names: Kilian Kistenbroker, Emily Huang, Sean Locklar, 
* Shauhin Pourshayegan
*
* Student IDs: 920723372, 920499746, 920506337, 920447681
* 
* GitHub Name: KilianKistenbroker
* 
* Group Name: Team Poke
*
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "dir_func.c"
//#include "vc_block.c"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

// dir_entr * currentDir;
// int dir_ent_position;
// int load_Dir(char * token){
//     dir_ent_position = -1;
//     for(int i = 0; i<64; i++){
//         if(strcmp(token, currentDir[i].filename)==0){
//             dir_ent_position = i;
//             //printf("MATCH token : %s\n", token);
//             LBAread(currentDir, 6, currentDir[i].starting_block);
//             //found so reset i to 0 and set currentDir 
//             //printf("%s\n", currentDir->filename);
//             break;
//         }
//         else if(i ==63){
//             //printf("This does not exist: %s\n", token);
//             return -1;
//         }
		
//     }
//     return dir_ent_position;
    
// }

// char * filetoken;
// //parsePath for opendir ret
// int parsePath(const char * pathname){
//     //first load the entire root directory
//     currentDir = malloc(512*6);
//     LBAread(currentDir, 6, 6);

//     //store path name into char copy to use in strtok
//     char * copy = strdup(pathname);
    
//     char * token = strtok(copy, "\\");
    
//     while(token != NULL){
//         filetoken = token;
// 		//printf("%s\n", filename);
//         dir_ent_position = load_Dir(token);
//         token = strtok(NULL, "\\");
//     }
//     //printf("%d", dir_ent_position);
//     return dir_ent_position;
// }

typedef struct b_fcb
	{
	/** TODO add all the information you need in the file control block **/
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_fcb * fcb;
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//if(parsePath(filename) < 0 && flags == O_CREAT){
		//create file
	//}
	fcb = malloc(sizeof(b_fcb));
	//printf("%s\n", filetoken);
	fcb->buflen = 0;
	fcb->index = 0;
	printf("%d\n", sizeof(b_fcb));
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();				// get our own file descriptor
										// check for error - all used FCB's
	
	return (returnFd);						// all set
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
	return (0);	//Change this
	}
	
// Interface to Close the file	
void b_close (b_io_fd fd)
	{
		free(fcb);
		fcb = NULL;
	}
