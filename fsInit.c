/**************************************************************
* Class:  CSC-415-0# Fall 2021
* Names: 
* Student IDs:
* GitHub Name:
* Group Name:
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "fsLow.h"
#include "mfs.h"

typedef struct vcb
{
	int block_size;
	int total_blocks;
	// int total_free_blocks;
	// int fat_start;
	// int fat_len;
	int free_block_start;
	int dir_entr_start;
	// int dir_entr_len;
	int magic_num;
} vcb;

// VCB is declared globally here
vcb * VCB;

typedef struct dir_entr
{
	int starting_block;
	int size;

	// used tell if this is a file or directory
	int file_type;

	// used to check if this dir_entr occupies a block during initial format
	int occupied;

	// points to the next file. if null then it is free
	int next; 

	int permissions;
	char filename[20];
	uid_t user_ID;
	gid_t group_ID;

} dir_entr;

//probably don't need this for now
typedef struct fat
{
	int occupied;
	int eof;
	int next_block;
} fat;

uint8_t * buffer_bitmap;


void update_free_block_start(int total_blocks) 
{

	/*
	 reading free space into buffer from disk, 
	 that way I can iterate through the freespace bitmap
	 */

	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(sizeof(buffer_bitmap) * total_blocks);
	}
	
	LBAread(buffer_bitmap, 5, 1);

	//finding the first free block in the freespace bitmap
	for (int i = 0; i < total_blocks; i++)
	{
		if (buffer_bitmap[i] == 0)
		{
			VCB->free_block_start = i;
			printf("updated free_space_start: %d \n\n", VCB->free_block_start);
			break;
		}
	}
}

//This function will allocate space by moving occupied blocks out of the way
int allocate_space(int amount_to_alloc, int total_blocks)
{
	int previous_free_block_start = VCB->free_block_start;
	int j = 0; // j is an index for alloc_block_array

	// creating buffer_bitmap to read the freespace from disk
	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(sizeof(buffer_bitmap) * total_blocks);
	}

	LBAread(buffer_bitmap, 5, 1); // blocks 1 -> 5 represent our freespace bitmap

	/*
	this iterates through the buffer_bitmap to see if block is free or occupied.
	If it is occupued, move() will move the contents of the block somewhere else.
	*/

	printf("These positions are given to caller and written to disk: \n");
	for (int i = VCB->free_block_start ; i < total_blocks; i++)
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
			i = total_blocks;
		}
		else if (i == total_blocks - 1) // implies their is no free space left.
		{
			printf("no more space available\n");
		}
	}
	printf("\n");

	// push updated bitmap to disk
	LBAwrite(buffer_bitmap, 5, 1);

	// updates VCB with new first free block position.
	update_free_block_start(total_blocks);

	return previous_free_block_start;
}

void init_bitmap(int numOfBlocks, uint64_t blockSize)
{

	int first_free_block = 0;
	
	//creating buffer to read if a block is empty or not
	dir_entr * buffer_block = malloc(blockSize);

	// creating bitmap for free space
	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(sizeof(buffer_bitmap) * numOfBlocks);
	}
	

	//initializing dedicated block space for VCB and freespace bitmap
	for (int i = 0; i <= 5 + 1; i++)
	{
		/*
		 WARNING: will probably have to call a move() function for 
		 file/directory if they are occupying block space for the 
		 freespace bitmap
		 */

		buffer_bitmap[i] = 1;
	}
	
	/*
	iterating through bitmap and starting after the dedicate space 
	for vcb and freespace bitmap (e.g 1 block for + 5 blocks for freespace)
	*/

	for (int i = 6; i < numOfBlocks; i++)
	{
		LBAread(buffer_block, 1, i);

		// checking if block is empty, and updating bitmap
		if (buffer_block->occupied != 1)
		{
			buffer_bitmap[i] = 0;
		}
		else 
		{
			buffer_bitmap[i] = 1;
		}
	}

	LBAwrite(buffer_bitmap, 5, 1);

	update_free_block_start(numOfBlocks);
}

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	
		VCB = malloc(blockSize);
		
		//VCB is updated with whatever is at position 0
		LBAread(VCB, 1, 0);

		/*
	 	checking if magic number of block 0 is 3.
	 	if it is 3 then VCB is already formatted,
	 	and we don't need to initialize.
		*/

		if (VCB->magic_num != 3)
		{
			VCB->magic_num = 3;
			VCB->total_blocks = numberOfBlocks;
			VCB->block_size = blockSize;

			//init_bitmap() also returns the first free block in freespace bitmap
			init_bitmap(numberOfBlocks, blockSize); 
			update_free_block_start(blockSize);

			// --------- INIT ROOT DIRECTORY ---------- //

			/*
			 creating an array of 51 dir_entries
			 changed sizeof(dir_entr) -> sizeof(root_dir) * 5
			 since sizeof(dir_entr) was producing weird hexdump error
			 */

			dir_entr * root_dir = malloc(blockSize * 6); 
			
			printf("Size of dir_entr: %ld \n", sizeof(dir_entr));
			printf("blocks to allocate for root: %ld \n\n", 1 + (sizeof(dir_entr)* 59) / 512);

			strncpy(root_dir[0].filename, ".", 1);
			root_dir[0].size = sizeof(dir_entr) * 59;

			// VCB->free_block_start will always be up to date
			root_dir[0].starting_block = VCB->free_block_start; 
			strncpy(root_dir[1].filename, "..", 2);

			printf("Size of root_dir: %d \n", root_dir[0].size);
			printf("root_dir[0].starting_block : %d \n", root_dir[0].starting_block);
			printf("root_dir[0].filename : %s \n", root_dir[0].filename);
			printf("root_dir[1].filename : %s \n\n", root_dir[1].filename);

			for (int i = 0; i < 59; i++)
			{
				// cannot set to null, so 0 will imply that this dir_entry is free
				root_dir[i].next = 0;
			}

			//-------ALLOC SPACE FOR ROOT DIRECTORY-------//
		
			/*
			 I want to allocate 6. VCB->free_block_start will
			 be updated after calling allocate_space() so I will 
			 retrieve the previous free_start
			 */

			int previous_free_block_start = allocate_space(5, numberOfBlocks); 
		
			for (int i = 0; i < 6; i++)
			{
				LBAwrite(root_dir, 6, previous_free_block_start);
			}

			// this is the starting block of the root directory
			VCB->dir_entr_start = previous_free_block_start; 

			LBAwrite(VCB, 1, 0);
		}


		return 0;
	}
	
	
void exitFileSystem ()
	{
		printf ("System exiting\n");
	}