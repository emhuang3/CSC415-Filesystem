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
	int total_free_blocks;
	int fat_start;
	int fat_len;
	int free_block_start;
	int dir_entr_start;
	int dir_entr_len;
	int magic_num;

} vcb;

// VCB is declared globally here
vcb * VCB;

typedef struct dir_entr
{
	int starting_block;
	int size;
	int file_type;
	int occupied;
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
			printf("updated free_space_start: %d \n", VCB->free_block_start);
			break;
		}
	}
}

//This function will retrun an array of free positions to the caller

void allocate_space(int amount_to_alloc, int total_blocks, int * alloc_block_array)
{
	int j = 0; // j is an index for alloc_block_array
	int to_be_allocated[amount_to_alloc];

	// creating buffer_bitmap to read the freespace from disk
	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(sizeof(buffer_bitmap) * total_blocks);
	}

	LBAread(buffer_bitmap, 5, 1); // blocks 1 -> 5 represent our freespace bitmap

	/*
	this iterates through the buffer_bitmap and populates alloc_block_array
	with the position of free blocks to return to caller. caller could use this
	array of free positions to populate blocks using something like  
	LBAwrite(FILE, 1, alloc_block_array[i]) inside a loop.
	*/

	for (int i = VCB->free_block_start ; i < total_blocks; i++)
	{
		
		if (buffer_bitmap[i] == 0 && j <= amount_to_alloc)
		{
			(alloc_block_array)[j] = i;
			buffer_bitmap[i] = 1;
			j++;
		}
		else if (j == amount_to_alloc + 1) // exits if no more blocks need to be allocated.
		{
			i = total_blocks;
		}
		else if (i == total_blocks - 1) // implies their is no free space left.
		{
			printf("no more space available");
		}
	}

	// push updated bitmap to disk
	LBAwrite(buffer_bitmap, 5, 1);

	// updates VCB with new first free block position.
	update_free_block_start(total_blocks);
}

void init_bitmap(int numOfBlocks, uint64_t blockSize)
{

	/*
	 Testing that blocks are being read properly 
	 by adding a temporary directory to block 10, to 
	 see if it shows up in hexdump.
	 */
	dir_entr * test_dir = malloc(blockSize);
	test_dir->occupied = 1;
	LBAwrite(test_dir, 1, 10);

	/*-----------------END TEST----------------*/



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
		printf("debug: %d \n", VCB->magic_num);

		/*
	 	checking if magic number of block 0 is 3.
	 	if it is 3 then VCB is already formatted,
	 	and we don't need to initialize.
		*/

		if (VCB->magic_num != 0)
		{
			VCB->magic_num = 0;
			VCB->total_blocks = numberOfBlocks;
			VCB->block_size = blockSize;

			//init_bitmap() also returns the first free block in freespace bitmap
			init_bitmap(numberOfBlocks, blockSize); 
			update_free_block_start(blockSize);
			VCB->dir_entr_start = 6;  // this might be where the root directory is positioned.
			//VCB->fat_start;		  // this is where the fat is positioned.
		}


		//------------TESTING allocate_space() function----------//

		int * allocated_spaces = malloc(sizeof(int) * 4); // this is size of 5
		
		// I want to allocate 5 blocks (e.g. 4 == 5)
		allocate_space(4, numberOfBlocks, allocated_spaces); 
		
		printf("These positions are given to caller: ");
		for (int i = 0; i < 5; i++)
		{
			printf("%d ", allocated_spaces[i]);
		}
		printf("\n");
		

		//hexdump for freespace should update to reflect occupied

		/*-----------------END TEST----------------*/
		
		return 0;
	}
	
	
void exitFileSystem ()
	{
		printf ("System exiting\n");
	}