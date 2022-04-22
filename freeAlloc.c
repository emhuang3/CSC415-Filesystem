#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "vc_block.c"

uint * buffer_bitmap;

int is_allocated(int index)
{
    return ((buffer_bitmap[index / 32] & (1 << (index % 32))) != 0);
}

void allocate_bit(int index)
{
    buffer_bitmap[index / 32] |= 1 << (index % 32);
}

void clear_bit(int index)
{
    buffer_bitmap[index / 32] &= ~(1 << (index % 32));
}

int convert_size_to_blocks(int byte_size, int block_size) 
{
	// write entire file to disk
	float top_num = (float) byte_size;
	float bottom_num = (float) block_size;
	int block_count = ceil(top_num/bottom_num);

	return block_count;
}

void update_free_block_start() 
{

	/*
	 reading free space into buffer from disk, 
	 that way I can iterate through the freespace bitmap
	 */

	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(VCB->block_size * 5);
		
		// checking if malloc was successful
		if (buffer_bitmap == NULL)
		{
			printf("ERROR: failed to malloc.\n");
			exit(-1);
		}

		LBAread(buffer_bitmap, 5, 1);
	}

	//finding the first free block in the freespace bitmap
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

//This function will allocate space by moving occupied blocks out of the way
int allocate_space(int amount_to_alloc)
{
	int previous_free_block_start = VCB->free_block_start;
	int j = 0; // j is an index for alloc_block_array

	// creating buffer_bitmap to read the freespace from disk
	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(VCB->block_size * 5);
		

		// checking if malloc was successful
		if (buffer_bitmap == NULL)
		{
			printf("ERROR: failed to malloc.\n");
			exit(-1);
		}

		// blocks 1 -> 5 represent our freespace bitmap
		LBAread(buffer_bitmap, 5, 1); 
	}

	/*
	this iterates through the buffer_bitmap to see if block is free or occupied.
	If it is occupued, move() will move the contents of the block somewhere else.
	*/

	// printf("These positions are given to caller and written to disk: \n");
	for (int i = VCB->free_block_start; i < VCB->total_blocks; i++)
	{
		
		if (j < amount_to_alloc)
		{
						
			// block is free. Nothing to move
			if (!is_allocated(i))
			{
				// printf("block %d freely written to \n", i);
				allocate_bit(i);
				j++;
			}
			else
			{
				// printf("block %d was moved then written to \n", i);

				//run a move() function to move stuff in block somewhere else.
				allocate_bit(i);
				j++;
			}
		}

		// implies their is no free space left.
		else if (i == VCB->total_blocks - 1) 
		{
			printf("ERROR: no more space available on disk\n\n");

			//reset buffer_bitmap
			free(buffer_bitmap);
			buffer_bitmap = NULL;

			return VCB->free_block_start;
		}

		// exits if no more blocks need to be allocated.
		else
		{
			i = VCB->total_blocks;
		}

	}
	// printf("\n");

	// push updated bitmap to disk
	LBAwrite(buffer_bitmap, 5, 1);

	// updates VCB with new first free block position.
	update_free_block_start();

	return previous_free_block_start;
}

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


	//initializing dedicated block space for VCB and freespace bitmap
	for (int i = 0; i < 6; i++)
	{
		allocate_bit(i);
	}

	LBAwrite(buffer_bitmap, 5, 1);

	update_free_block_start();
}

void reallocate_space(dir_entr * directory, int index, int save_state)
{
	// get num of blocks this directory occupies
	int block_count = convert_size_to_blocks(directory[index].size, VCB->block_size);
	
	// update bitmap buffer
	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(VCB->block_size * 5);

		// check if malloc was successful
		if (buffer_bitmap == NULL)
		{
			printf("ERROR: failed to malloc.\n");
			exit(-1);
		}

		LBAread(buffer_bitmap, 5, 1);
	}

	for (int i = directory[index].starting_block; i < directory[index].starting_block + block_count; i++)
	{
		clear_bit(i);
	}

	// prevents updating disk during volatile writing process
	if (!save_state)
	{
		LBAwrite(buffer_bitmap, 5, 1);

		// updating free block start in VCB
		update_free_block_start();
	}
	


	/*
	 metadata in index 0 & 1 does not need to be updated 
	 since those will be overwritten anyways as they are
	 marked free to overwrite by freespace reallocation.
	*/
}

/* 
this helper function pushes children of parent leftward 
after an empty space was created in parent list
*/

void move_child_left(dir_entr * parent, int index)
{
    // index refers to the index that was just freed up.
    int fill_index = index; // this is the position that needs to be filled by rightmost child
    int iterator = index;   // this will be used to iterate through the parent.

    // check to see if (iterator + 1) is greater than (parent.size / block size)
    if ((iterator + 1) >= (parent[0].size/sizeof(dir_entr)))
    {
        printf("parent is full.\n");
        return;
    }
    
    while (strcmp(parent[iterator + 1].filename, "") != 0)
    {
        iterator++;
        if ((iterator + 1) >= (parent[0].size/sizeof(dir_entr)))
        {
            printf("parent is full.\n");
            return;
        }
    }

    // move the rightmost child to the fill position
    strcpy(parent[fill_index].filename, parent[iterator].filename);
    parent[fill_index].group_ID = parent[iterator].group_ID;
    parent[fill_index].is_file = parent[iterator].is_file;
    parent[fill_index].permissions = parent[iterator].permissions;
    parent[fill_index].size = parent[iterator].size;
    parent[fill_index].starting_block = parent[iterator].starting_block;
    parent[fill_index].time = parent[iterator].time;
    parent[fill_index].user_ID = parent[iterator].user_ID;

    // free the rightmost child
    memset(parent[iterator].filename, 0, sizeof(parent[iterator].filename));
}