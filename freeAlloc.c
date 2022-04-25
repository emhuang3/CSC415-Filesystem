#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "vc_block.c"

/* This is a buffer for the freespace bitmap,
which will stay in memory and update the 
freespace bitmap in the LBA. */

uint * buffer_bitmap;

/* This helper function allows us to check if a particular 
 bit is occupied in the bitmap. The function takes an index representing
 a block in the LBA, to check if the associated bit is allocated or not. */

int is_allocated(int index)
{
    return ((buffer_bitmap[index / 32] & (1 << (index % 32))) != 0);
}

/* This helper function allows us to allocate a bit inside an int,
 at an index representing a block in the LBA. */
void allocate_bit(int index)
{
    buffer_bitmap[index / 32] |= 1 << (index % 32);
}

/* This helper function clears a bit at an index representing a block
 in the LBA. */

void clear_bit(int index)
{
    buffer_bitmap[index / 32] &= ~(1 << (index % 32));
}

/* This helper function converts a byte size to a number 
 of blocks that could store the bytes. */

int convert_size_to_blocks(int byte_size, int block_size) 
{
	float top_num = (float) byte_size;
	float bottom_num = (float) block_size;
	int block_count = ceil(top_num/bottom_num);

	return block_count;
}

// this function finds the first free block in the freespace bitmap
void update_free_block_start() 
{
	for (int i = 0; i < VCB->total_blocks; i++)
	{
		if (!is_allocated(i))
		{	
			VCB->free_block_start = i;
			LBAwrite(VCB, 1, 0);
			break;
		}
	}
}

// this function clears space when removing or overwriting a file.
void reallocate_space(dir_entr * directory, int index, int save_state)
{
	// this stores the number of blocks the directory occupies
	int block_count = convert_size_to_blocks(directory[index].size, VCB->block_size);

	// this clears the bits the directory currently occupies.
	for (int i = directory[index].starting_block; i < directory[index].starting_block + block_count; i++)
	{
		clear_bit(i);
	}

	/* This provides the caller the option to prevent updating disk during volatile writing process, if
	 something outside of this function fails, which will require the caller to update the bitmap. */

	if (!save_state)
	{
		LBAwrite(buffer_bitmap, 5, 1);

		// updating free block start in VCB
		update_free_block_start();
	}
}

// resets bitmap in the event that allocation fails
void reset_bitmap()
{
	if (buffer_bitmap != NULL)
	{
		free(buffer_bitmap);
		buffer_bitmap = NULL;
	}

	buffer_bitmap = calloc(5, VCB->block_size);

	if (buffer_bitmap == NULL)
	{
		printf("failed to malloc bitmap.\n");
		exit(-1);
	}
	
	LBAread(buffer_bitmap, 5, 1);;
}

/* This function will allocate contiguous space in the LBA by counting 
 the number of blocks to allocate from the first available block and 
 calling move_on_disk() for all files that are in the way. */

int allocate_space(int amount_to_alloc)
{
	/* This number will be returned to the caller to provide a starting 
	 number for thier allocated space. */

	int previous_free_block_start = VCB->free_block_start;
	
	// j will iterate as space is being allocated for the caller
	int j = 0; 
	int ret = 0;

	/* This iterates through the buffer_bitmap to see if block is free or occupied.
	If it is occupued, move_on_disk() will move the contents of the block to the
	next contiguously available space. */

	for (int i = VCB->free_block_start; i < VCB->total_blocks; i++)
	{
						
		// block is free, therefore we can mark this as occupied for the caller
		if (!is_allocated(i))
		{
			j++;
		}
		else
		{
			j = 0;
		}

		if (j == amount_to_alloc)
		{
			for (int k  = i - j + 1; k <= i; k++)
			{
				allocate_bit(k);
			}
			i = VCB->total_blocks;
		}

		// this implies there is no free space left.
		else if (i == VCB->total_blocks - 1) 
		{
			printf("Not enough space on disk.\n");
			reset_bitmap();
			return -1;
		}
	}

	// push updated bitmap to disk
	LBAwrite(buffer_bitmap, 5, 1);

	// updates VCB with new first free block position.
	update_free_block_start();

	return previous_free_block_start;
}

// this function initilizes a bitmap if the VCB needs to be formatted.
void init_bitmap()
{

	int first_free_block = 0;

	// creating bitmap for free space for the first time
	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(VCB->block_size * 5);

		// checking if malloc was successful
		if (buffer_bitmap == NULL)
		{
			printf("ERROR: failed to malloc.\n");
			exit(-1);
		}
	}

	/* This is initializing dedicated space for VCB and freespace
	bitmap in the LBA. */

	for (int i = 0; i < 6; i++)
	{
		allocate_bit(i);
	}

	LBAwrite(buffer_bitmap, 5, 1);

	update_free_block_start();
}

/* This helper function pushes children of parent leftward 
after an empty space was created in parent's entry list. */

void move_child_left(dir_entr * parent, int index)
{
    // index refers to the index that was just freed up.
    int fill_index = index; // this is the position that needs to be filled by rightmost child
    int iterator = index;   // this will be used to iterate through the parent.
	int is_full = 0;

    // this checks if we've reached the end of the entry list
    if ((iterator + 1) >= (parent[0].size/sizeof(dir_entr)))
    {
		// if removed/moved child was already the rightmost position, then we will just return.
		if (strcmp(parent[iterator + 1].filename, "") == 0)
		{
			return;
		}
        is_full = 1;
    }
    
	if (is_full == 0)
	{
		while (strcmp(parent[iterator + 1].filename, "") != 0)
    	{
        	iterator++;
        	if ((iterator + 1) > (parent[0].size/sizeof(dir_entr)))
        	{
				break;
        	}
    	}
	}

    // this will move the rightmost child's metadata to the fill position
    strcpy(parent[fill_index].filename, parent[iterator].filename);
    // parent[fill_index].group_ID = parent[iterator].group_ID;
    parent[fill_index].is_file = parent[iterator].is_file;
    parent[fill_index].mode = parent[iterator].mode;
    parent[fill_index].size = parent[iterator].size;
    parent[fill_index].starting_block = parent[iterator].starting_block;
    // parent[fill_index].count = parent[iterator].count;
    // parent[fill_index].user_ID = parent[iterator].user_ID;

    // this will free the rightmost child
    memset(parent[iterator].filename, 0, sizeof(parent[iterator].filename));
}