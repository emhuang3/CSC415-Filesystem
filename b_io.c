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
	// holds the open file buffer.
	char * buf;		

	// holds position in fcbArray.
	int file_descriptor; 

	// holds the size of the buffer.
	int len;

	// holds an index to the current byte in the buffer.
	int pos;

	// holds a position in the 'dest' folder we are copying to.
	int pos_in_dest_parent;

	// holds a position in the 'src' folder we are copying from.
	int pos_in_src_parent;

	// holds the parent directory of the file.
	dir_entr * parent_dir;
	
} b_fcb;

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
			return i;
		}
	}
	return (-1);
}
	

int overwrite = 0;

// Interface to open a buffered file
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

	// this mallocs for the fcb's parent directory to later be stored.
	if (fcbArray[returnFd].parent_dir == NULL)
	{	
		fcbArray[returnFd].parent_dir = calloc(6, VCB->block_size);
		if (fcbArray[returnFd].parent_dir == NULL)
		{
			printf("malloc failed for openning a file\n");
			return -1;
		}
	}
	
	int ret = parse_pathname(pathname);

	// This if statement means we can neither create nor open an existing file.
	if (ret == INVALID && paths_remaining > 0)
	{
		return -1;
	}

	// this if statement implies that the head path is invalid, meaning we can create a file here.
	if (ret == INVALID && paths_remaining == 0) 
	{
		/* temp_curr_dir will be updated to the files 'dest' parent, so we will store that information 
		into the fcb's 'parent_dir' buffer to have a reference to its own parent. */

		LBAread(fcbArray[returnFd].parent_dir, 6, temp_curr_dir[0].starting_block);

		// fcbArray[returnFd].pos_in_src_parent = temp_child_index;

		if (temp_curr_dir != NULL)
    	{
        	free(temp_curr_dir);
        	temp_curr_dir = NULL;
    	}

		// if flag contains O_CREAT, then we will create this file in 'dest' parent
		if (flags & O_CREAT)
		{
			// this for loop finds a free space to put file in 'dest' parent directory
			for (int i = 0; i < 32; i++)
			{

				if (strcmp(fcbArray[returnFd].parent_dir[i].filename, "") == 0)
				{

					/* marks this position in the parent as occupied by copying the created 
					filename into the 'dest' parent of the file. saved_filename is updated in
					validate_path(). */

					strcpy(fcbArray[returnFd].parent_dir[i].filename, saved_filename);
					fcbArray[returnFd].parent_dir[i].is_file = 1;
					fcbArray[returnFd].pos_in_dest_parent = i;
					update_time(fcbArray[returnFd].parent_dir[i].create_time);
					update_time(fcbArray[returnFd].parent_dir[i].modify_time);
					update_time(fcbArray[returnFd].parent_dir[i].access_time);
					update_time(fcbArray[returnFd].parent_dir[0].modify_time);

					i = 32;
				}
				else if (i == 31)
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
	else if (ret == FOUND_FILE)
	{

		// this will store a reference to the file's parent in the fcb's 'parent_dir' buffer.
		LBAread(fcbArray[returnFd].parent_dir, 6, temp_curr_dir[0].starting_block);

		// this will save the index of the file's metadata that is stored in its parent directory.
		fcbArray[returnFd].pos_in_src_parent = temp_child_index;

		if (temp_curr_dir != NULL)
    	{
        	free(temp_curr_dir);
        	temp_curr_dir = NULL;
   	 	}

		if (flags & O_CREAT)
		{
			/* if parse_pathname() found a file and a create flag was given to b_open(), then the 
			user probably wants to overwrite this file, so we will mark 'overwrite' to '1' to begin 
			that process. */

			overwrite = 1;
		}
		
		// this is getting the block count of the openned file.
		int file_index = fcbArray[returnFd].pos_in_src_parent;
		int filesize = fcbArray[returnFd].parent_dir[file_index].size;
		int block_count = convert_size_to_blocks(filesize, VCB->block_size);

		fcbArray[returnFd].buf = calloc(block_count, VCB->block_size);

		if (fcbArray[returnFd].buf == NULL)
		{
			printf("failed to malloc while trying to open a file\n");
			return -1;
		}

		fcbArray[returnFd].len = filesize;
		fcbArray[returnFd].pos = 0;

		LBAread(fcbArray[returnFd].buf, block_count, fcbArray[returnFd].parent_dir[file_index].starting_block);

		printf("openned '%s' in parent directory: '%s'.\n", 
		fcbArray[returnFd].parent_dir[file_index].filename, fcbArray[returnFd].parent_dir[0].filename);
		return (returnFd);
	}

	// invalid path was given, so we will return -1 to shell to terminate the writing process.
	else 
	{
		if (temp_curr_dir != NULL)
    	{
        	free(temp_curr_dir);
        	temp_curr_dir = NULL;
    	}

		printf("ERROR: invalid path.\n");
		return -1;
	}

	fcbArray[returnFd].buf = calloc(1, buffer_size + 1); // might through an error for realloc
	
	if (fcbArray[returnFd].buf == NULL)
	{
		printf("failed to malloc while trying to open a file\n");
		return -1;
	}

	fcbArray[returnFd].len = 0;
	fcbArray[returnFd].pos = 0;

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

	// this if statement begins the overwrite process
	if (overwrite == 1)
	{
		overwrite = 2;
		char input[2];
		printf("file already exists.\n");
		printf("would you like to overwrite this file? y or n\n");

		// provide yes or no option before proceeding with file overwriting
		while (strcmp(input, "y") != 0 && strcmp(input, "n") != 0)
		{
			scanf("%2s", input);
		}
		
		if (strcmp(input, "n") == 0 )
		{
			printf("did not copy file.\n\n");
			return -1;
		}

		else
		{
			// ------ begin overwrite process ------ //

			fcbArray[fd].len = 0;
			fcbArray[fd].pos = 0;

			/* this frees the blocks that the file occupies in freespace bitmap. 3rd arg '1' prevents 
			bitmap from writing to disk until the end of the overwrite process to ensure the bitmap on 
			disk is safe from an interrupted overwrite process.*/

			reallocate_space(fcbArray[fd].parent_dir, fcbArray[fd].pos_in_dest_parent, 1);
		}
	}

		//-------------------------- write to disk ----------------------//

		int free_space = buffer_size - fcbArray[fd].pos;

		/* len will grow as buffer grows. This will be updated in the file's parent directory, 
		and will be used to calculate blocks to allocate for writing to disk. */

		/* if the pointer to a byte in the fcb's buffer (pos) exceeds the amount of space left 
		inside the buffer (free_space), then we will grow the buffer by adding another block to it. */

		if (fcbArray[fd].pos > free_space)
		{
			buffer_size += VCB->block_size;
			char * resize = realloc(fcbArray[fd].buf, buffer_size);
			if (resize == NULL)
			{
				printf("ERROR: failed to reallocate.\n\n");
				return -1;
			}
			fcbArray[fd].buf = resize;
		}
		
		// this copies a number of bytes (count) from 'buffer' into the fcb's 'buf'.
		memcpy(fcbArray[fd].buf + fcbArray[fd].pos, buffer, count);

		/* this iterates pos to reflect the current byte we are looking at in order 
		to track how much freespace we have in the fcb buf. */

		fcbArray[fd].pos += count;
		fcbArray[fd].len = fcbArray[fd].pos;
		
		// 200 = BUFFLEN, implies that less then 200 is an eof indicator.
		if (count < 200)
		{
			/* this updates the fcb buffer for the 'dest' parent directory with newly 
			created/overwritten filesize. */

			int index = fcbArray[fd].pos_in_dest_parent;
			fcbArray[fd].parent_dir[index].size = fcbArray[fd].len;

			// this gets a block count to write to disk
			int block_count = convert_size_to_blocks(fcbArray[fd].len, VCB->block_size);

			// allocates space to the created file.
			fcbArray[fd].parent_dir[index].starting_block = allocate_space(block_count);

			if (fcbArray[fd].parent_dir[index].starting_block == -1)
			{
				// this if statment implies that we could not allocate freespace in the bitmap.
				reset_bitmap();
				return -1;
			}
			
			if (overwrite == 2)
			{
				/* overwriting should be succesfully finished at this point, so we will update bitmap. 
				we are updating bitmap here to ensure bitmap isn't falsely updated during an interrupted
				overwrite process. */

				LBAwrite(buffer_bitmap, 5, 1);
				update_free_block_start();

				//resets overwrite for next use.
				overwrite = 0;
			}

			// writes the file to disk.
			LBAwrite(fcbArray[fd].buf, block_count, fcbArray[fd].parent_dir[index].starting_block);

			// updates parent directory.
			LBAwrite(fcbArray[fd].parent_dir, 6, fcbArray[fd].parent_dir[0].starting_block);

			// updates VCB.
			LBAwrite(VCB, 1, 0);

			printf("finished writing file.\n");
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
		// read contents of fcb into buffer.
		memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].pos, count);
		fcbArray[fd].pos += count;
		return (count);
	}
	else
	{	
		// implies that we are at the last bit of the buffer
		memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].pos, bytes_remaining);
		printf("finished reading file.\n");
		return bytes_remaining;
	}
}
	
// Interface to Close the file	
void b_close (b_io_fd fd)
{
	
	//--------------------------- clean up ------------------------//

	if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }

	for (int i = 0; i < MAXFCBS; i++)
	{
		if (fcbArray[i].buf != NULL)
		{
			free(fcbArray[i].parent_dir);
			fcbArray[i].parent_dir = NULL;

			free(fcbArray[i].buf);
			fcbArray[i].buf = NULL;
		}
	}
}