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
	// holds the open file buffer
	char * buf;		

	// holds position in fcbArray	
	int file_descriptor; 		
	
	int len;
	int pos;

	int pos_in_parent;
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
			fcbArray[i].parent_dir = NULL;
			fcbArray[i].buf = NULL;
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

	if (fcbArray[returnFd].parent_dir == NULL)
	{
		fcbArray[returnFd].parent_dir = malloc(VCB->block_size * 6);

		if (fcbArray[returnFd].parent_dir == NULL)
		{
			printf("malloc failed\n");
			return -1;
		}
	}
	
	int ret = parse_pathname(pathname);

	// could not read path
	if (num_of_paths == -1)
	{
		return -1;
	}
	
	// set fcbArray[returnFd].parent_dir to have a refererence its own parent.
	LBAread(fcbArray[returnFd].parent_dir, 6, temp_curr_dir[0].starting_block);

	// copy temp_file_index into fcb's parent_dir
	fcbArray[returnFd].parent_dir[0].temp_file_index = temp_curr_dir[0].temp_file_index;

	free(temp_curr_dir);
	temp_curr_dir = NULL;

	// implies that we may have a working file
	if (ret == -1 && num_of_paths == 0) 
	{

		// ---- WARNING: THIS IS SUPPOSED TO CHECK IF FLAG is 'O_CREAT' ---- //
		// ---- replace with if (flag == O_CREAT) ---- //
		if (flags & O_CREAT)
		{
			// create this file in parent

			//find a free space to put file in parent directory
			for (int i = 0; i < 64; i++)
			{
				if (strcmp(fcbArray[returnFd].parent_dir[i].filename, "") == 0)
				{

					// marks this position in the parent as occupied
					strcpy(fcbArray[returnFd].parent_dir[i].filename, saved_filename);
					fcbArray[returnFd].parent_dir[i].is_file = 1;
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

	// This will open the existing file
	else if (num_of_paths == -2)
	{
		int file_index = fcbArray[returnFd].parent_dir[0].temp_file_index;
		int filesize = fcbArray[returnFd].parent_dir[file_index].size;
		int block_count = convert_size_to_blocks(filesize, VCB->block_size);

		fcbArray[returnFd].buf = malloc(filesize + 1);

		if (fcbArray[returnFd].buf == NULL)
		{
			printf("ERROR: failed to malloc\n");
			close (returnFd);	
			return -1;
		}

		fcbArray[returnFd].len = filesize;
		fcbArray[returnFd].pos = 0;

		LBAread(fcbArray[returnFd].buf, block_count, fcbArray[returnFd].parent_dir[file_index].starting_block);

		printf("opened %s in parent directory: %s with fd %d.\n", 
		fcbArray[returnFd].parent_dir[file_index].filename, fcbArray[returnFd].parent_dir[0].filename, returnFd);
		return (returnFd);
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
		printf("ERROR: failed to malloc\n");
		close (returnFd);	
		return -1;
	}

	// init to zero. will increment as we read the file
	fcbArray[returnFd].len = 0;
	fcbArray[returnFd].pos = 0;
	
	printf("opened %s in parent directory: %s, with fd %d.\n", saved_filename, fcbArray[returnFd].parent_dir[0].filename, returnFd);

	return (returnFd);
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

	// implies that user might want to overwrite this file
	if (num_of_paths == -2)
	{
		char input[2];
		printf("file already exists.\n");
		printf("would you like to overwrite this file? y or n\n");
		// provide yes or no option before proceeding

		while (strcmp(input, "y") != 0 && strcmp(input, "n") != 0)
		{
			scanf("%2s", input);
		}
		
		if (strcmp(input, "y") == 0 )
		{
			// begin overwrite process

			// clear/reset fileinfo in fcbArray[fd]

			// free blocks that file occupies in freespace bitmap
		}

		else
		{
			printf("did not copy file.\n\n");
			return -1;
		}
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
				printf("ERROR: failed to reallocate.\n\n");
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
			fcbArray[fd].parent_dir[i].size = fcbArray[fd].len;

			// write entire file to disk
			int block_count = convert_size_to_blocks(fcbArray[fd].len, VCB->block_size);

			printf("blocks_to_allocate: %d\n", block_count);

			fcbArray[fd].parent_dir[i].starting_block = allocate_space(block_count);
			LBAwrite(fcbArray[fd].buf, block_count, fcbArray[fd].parent_dir[i].starting_block);

			// update parent directory
			LBAwrite(fcbArray[fd].parent_dir, 6, fcbArray[fd].parent_dir[0].starting_block);

			//updated VCB
			LBAwrite(VCB, 1, 0);

			// update current directory if temp_curr_dir is same as current directory
			if (fcbArray[fd].parent_dir[0].starting_block == curr_dir[0].starting_block)
			{
				LBAread(curr_dir, 6, fcbArray[fd].parent_dir[0].starting_block);
			}

			printf("finished copying file.\n\n");
		}
		
		return 0;
}

int b_read (b_io_fd fd, char * buffer, int count)
{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); 		
	}

	// count is capped at 200
	int bytes_remaining = fcbArray[fd].len - fcbArray[fd].pos;

	if (bytes_remaining > count)
	{
		// read contents of fbcArray[fd] into buffer
		memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].pos, count);
		fcbArray[fd].pos += count;
		return (count);
	}

	// implies that we are at the last bit of the buffer
	else
	{	
		memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].pos, bytes_remaining);
		printf("finished copying file.\n\n");

		return bytes_remaining;
	}
}
	
// Interface to Close the file	
void b_close (b_io_fd fd)
{
	
	//--------------------------- clean up ------------------------//

	startup = 0;

	for (int i = 0; i < MAXFCBS; i++)
	{
		free(fcbArray[i].parent_dir);
		fcbArray[i].parent_dir = NULL;
	}
}