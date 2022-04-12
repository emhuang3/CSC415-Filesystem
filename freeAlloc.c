#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "vc_block.c"

uint8_t * buffer_bitmap;

void update_free_block_start() 
{

	/*
	 reading free space into buffer from disk, 
	 that way I can iterate through the freespace bitmap
	 */

	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(sizeof(buffer_bitmap) * VCB->total_blocks);
		
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
		if (buffer_bitmap[i] == 0)
		{
			VCB->free_block_start = i;
			// printf("updated free_space_start: %d \n\n", VCB->free_block_start);
			
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
		buffer_bitmap = malloc(sizeof(buffer_bitmap) * VCB->total_blocks);

		// checking if malloc was successful
		if (buffer_bitmap == NULL)
		{
			printf("ERROR: failed to malloc.\n");
			exit(-1);
		}

		LBAread(buffer_bitmap, 5, 1); // blocks 1 -> 5 represent our freespace bitmap
	}

	

	/*
	this iterates through the buffer_bitmap to see if block is free or occupied.
	If it is occupued, move() will move the contents of the block somewhere else.
	*/

	printf("These positions are given to caller and written to disk: \n");
	for (int i = VCB->free_block_start ; i < VCB->total_blocks; i++)
	{
		
		if (j <= amount_to_alloc)
		{
			
			// block is free. Nothing to move
			if (buffer_bitmap[i] == 0)
			{
				printf("block %d freely written to \n", i);
				buffer_bitmap[i] = 1;
				j++;
			}
			else
			{
				printf("block %d was moved then written to \n", i);
				//run a move() function to move stuff in block somewhere else.
				buffer_bitmap[i] = 1;
				j++;
			}
		}
		else if (j == amount_to_alloc + 1) // exits if no more blocks need to be allocated.
		{
			i = VCB->total_blocks;
		}
		else if (i == VCB->total_blocks - 1) // implies their is no free space left.
		{
			printf("ERROR: no more space available on disk\n");
		}
	}
	printf("\n");

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
		buffer_bitmap = malloc(sizeof(buffer_bitmap) * VCB->total_blocks);

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
		buffer_bitmap[i] = 1;
	}

	LBAwrite(buffer_bitmap, 5, 1);

	update_free_block_start();
}

void reallocate_space(dir_entr * directory)
{
	// get num of blocks this directory occupies
	int count = ceil(directory[0].size/512);
	
	// update bitmap buffer
	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(sizeof(buffer_bitmap) * VCB->total_blocks);

		// check if malloc was successful
		if (buffer_bitmap == NULL)
		{
			printf("ERROR: failed to malloc.\n");
			exit(-1);
		}

		LBAread(buffer_bitmap, 5, 1);
	}

	
	

	printf("Blocks freed: ");
	for (int i = directory[0].starting_block; i < directory[0].starting_block + count; i++)
	{
		buffer_bitmap[i] = 0;
		printf("%d ", i);
	}
	printf("\n");

	LBAwrite(buffer_bitmap, 5, 1);

	// updating free block start in VCB
	update_free_block_start();

	/*
	 metadata in index 0 & 1 does not need to be updated 
	 since those will be overwritten anyways as they are
	 marked free to overwrite by freespace reallocation.
	*/
}