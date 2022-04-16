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
#include <math.h>

#include "b_io.h"
#include "dir_func.c"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
{
	//holds the open file buffer
	char * buf;		

	// holds position in fcbArray	
	int file_descriptor; 		
	
	int len;
	int pos;

	int mode; // 1 is write and 0 is no write.

	int pos_in_parent;
	
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

		// ---- WARNING: THIS IS SUPPOSED TO CHECK IF FLAG is 'O_CREAT' ---- //
		// ---- replace with if (flag == O_CREAT) ---- //
		if (1)
		{
			// create this file in parent

			//find a free space to put file in parent directory
			for (int i = 0; i < 64; i++)
			{
				if (strcmp(temp_curr_dir[i].filename, "") == 0)
				{

					// marks this position in the parent as occupied
					strcpy(temp_curr_dir[i].filename, saved_filename);
					temp_curr_dir[i].is_file = 1;
					fcbArray[returnFd].pos_in_parent = i;
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
			printf("ERROR: file doesn't exist. Cannot make this file.\n");
			return -1;
		}
		
	}

	// file already exists
	else if (num_of_paths == -2)
	{
		printf("ERROR: file already exists in %s directory\n", temp_curr_dir[0].filename);
		return -1;
	}

	// invalid path given
	else 
	{
		printf("ERROR: invalid path.\n");
		return -1;
	}

	fcbArray[returnFd].buf = malloc(buffer_size + 1);
	
	if (fcbArray[returnFd].buf == NULL)
	{
		printf("ERROR: failed to malloc");
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


	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		printf("ERROR: cannot write this file.\n\n");
		return (-1);
	}

		//-------------------------- write to disk ----------------------//
		int free_space = buffer_size - fcbArray[fd].pos;

		/*
		len will grow as buffer grows. must update in parent directory, 
		and will be used to calculate blocks to allocate for writing to disk
		*/

		if (fcbArray[fd].pos > free_space)
		{
			// call realloc to grow buffer
			buffer_size += 512;

			char * resize = realloc(fcbArray[fd].buf, buffer_size);

			if (resize == NULL)
			{
				printf("ERROR: failed to reallocate.\n");
				return -1;
			}
			
			fcbArray[fd].buf = resize;
		}
		
		memcpy(fcbArray[fd].buf + fcbArray[fd].pos, buffer, count);

		// move position up to track how much freespace we have in buf
		fcbArray[fd].pos += count;
		fcbArray[fd].len = fcbArray[fd].pos;
		
		// 200 = BUFFLEN, implies that less then 200 is probably eof indicator
		if (count < 200)
		{
			// update temp_curr_directory with child's filesize
			int i = fcbArray[fd].pos_in_parent;
			temp_curr_dir[i].size = fcbArray[fd].len;

			// write entire file to disk
			float topnum = (float) fcbArray[fd].len;

			int blocks_to_allocate = ceil(topnum/512.0);

			printf("blocks_to_allocate: %d\n", blocks_to_allocate);

			temp_curr_dir[i].starting_block = allocate_space(blocks_to_allocate);
			LBAwrite(fcbArray[fd].buf, blocks_to_allocate, temp_curr_dir[i].starting_block);

			// update parent directory
			LBAwrite(temp_curr_dir, 6, temp_curr_dir[0].starting_block);

			//updated VCB
			LBAwrite(VCB, 1, 0);

			// update current directory if temp_curr_dir is same as current directory
			if (temp_curr_dir[0].starting_block == curr_dir[0].starting_block)
			{
				LBAread(curr_dir, 6, temp_curr_dir[0].starting_block);
			}
			
		}
		
		return 0;
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
			memset(fcbArray[i].buf, 0, sizeof(fcbArray[i].buf));
			free(fcbArray[i].buf);
			fcbArray[i].buf = NULL;
		}
	}

	free(temp_curr_dir);
	temp_curr_dir = NULL;

	startup = 0;
}