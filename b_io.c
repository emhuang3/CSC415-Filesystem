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
	//holds the open file buffer
	char * buf;		

	// holds position in fcbArray	
	int file_descriptor; 		
	
	int len;
	int pos;

	int mode; // 1 is write and 0 is no write.

	dir_entr * parent_dir;
	
} b_fcb;

// this is probably used to make the proccess quicker for openning a buffer
b_fcb fcbArray[MAXFCBS];

//Indicates that this has not been initialized
int startup = 0;	

//Method to initialize our file system
int buffer_size;
void b_init ()
{
	buffer_size = VCB->block_size;

	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
			// indicates a free fcbArray
			fcbArray[i].buf = NULL;
			fcbArray[i].mode = 0;
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

b_io_fd b_open (char * pathname, int flags)
{
	b_io_fd returnFd;
		
	// initilizes our file interface
	if (startup == 0) b_init();
	
	// this will get a free space in the fcb array
	returnFd = b_getFCB();

	if (returnFd < 0)
	{
		return -1;
	}

	int ret = parse_pathname(pathname);

	// implies that we may have a working file
	if (ret == -1 && num_of_paths == 0) 
	{

		//check if the file already exists
		for (int i = 0; i < 63; i++)
		{
			if (strcmp(temp_curr_dir[i].filename, saved_filename) == 0)
			{
				printf("File already exists.\n");
				ret = 0;
			}
		}

		// implies that file does not exist
		if (ret == -1) 
		{
			if (flags == O_CREAT)
			{
				// create this file in parent

				//find a free space to put file in parent directory
				for (int i = 0; i < 64; i++)
				{
					if (strcmp(temp_curr_dir[i].filename, "") == 0)
					{

						// marks this position in the parent as occupied
						strcpy(temp_curr_dir[i].filename, saved_filename);
						temp_curr_dir[i].starting_block = VCB->free_block_start;
						i = 64;
					}
					else if (i == 63)
					{
						printf("ERROR: unable to find free space in the parent directory.\n");
						return -1;
					}
				}
			}
			else
			{
				// file doesn't exist. Cannot make this file

				return -1;
			}
		}
	}

	else
	{
		printf("invalid path.\n");
	}

	fcbArray[returnFd].parent_dir = temp_curr_dir;
	fcbArray[returnFd].buf = malloc(buffer_size + 1);
	
	if (fcbArray[returnFd].buf == NULL)
	{
		printf("failed to malloc");
		close (returnFd);	
		return -1;
	}

	// init to zero. will increment as we read the file
	fcbArray[returnFd].len = 0;
	fcbArray[returnFd].pos = 0;
	
	printf("opened %s in parent directory: %s, with fd %d.\n", saved_filename, temp_curr_dir[0].filename, returnFd);

	return (returnFd);
}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1);
	}

	if (fcbArray[fd].file_descriptor == -1)
	{
		return -1;
	}
	
	return (0);
}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
{
	int ret = 0;

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); 
	}

		//-------------------------- write to disk ----------------------//
		int free_space = buffer_size - fcbArray[fd].pos;

		return ret;
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
		return (-1); 		
	}

	int num_of_bytes = fcbArray[fd].len - fcbArray[fd].pos;

	if (num_of_bytes == 0)
	{
		/* code */
	}
	
		
	return (0);
}
	
// Interface to Close the file	
void b_close (b_io_fd fd)
{
	//--------------------------- clean up ------------------------//

	for (int i = 0; i < MAXFCBS; i++)
	{
		if (fcbArray[i].buf != NULL)
		{
			free(fcbArray[i].buf);
			fcbArray[i].buf = NULL;
		}
	}

	if (temp_curr_dir != NULL)
	{
		free(temp_curr_dir);
		temp_curr_dir = NULL;
	}
}