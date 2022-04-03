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
	int dir_entr_start; // -> points to the root directory
	int dir_entr_len;
	int magic_num;

} vcb;

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


int find_free_block(int numOfBlocks, uint64_t blockSize)
{
	int first_free_block = 0;
	
	//creating buffer to read if a block is empty or not
	dir_entr * buffer = malloc(blockSize);

	// creating bitmap for free space
	uint8_t * bitmap = malloc(sizeof(bitmap) * numOfBlocks);

	//initializing index 0 to end-of-free-space bitmap, since block 0 should hold our VCB
	int numOfFreeSpaceBlocks = 5;
	printf("blocks for bitmap: %d \n", numOfFreeSpaceBlocks);
	for (int i = 0; i <= numOfFreeSpaceBlocks + 1; i++)
	{
		/*
		 WARNING: will probably have to call a move() function for 
		 file/directory if they are occupying block space for the 
		 freespace bitmap
		 */

		bitmap[i] = 1;
	}
	
	//iterating through bitmap and starting after the dedicate space for vcb and freespace bitmap
	for (int i = numOfFreeSpaceBlocks + 1; i < numOfBlocks; i++)
	{
		LBAread(buffer, 1, i);

		// checking if block is empty, and updating bitmap
		if (buffer->occupied != 1)
		{
			bitmap[i] = 0;
		}
		else 
		{
			bitmap[i] = 1;
		}
	}

	for (int i = 1; i < numOfBlocks; i++)
	{
		if (bitmap[i] == 0)
		{
			first_free_block = i;
			break;
		}
	}

	LBAwrite(bitmap, numOfFreeSpaceBlocks, 1);

	return first_free_block;
}

typedef struct fat
{
	int occupied;
	int eof;
	int next_block;
} fat;

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	
		vcb * VCB = malloc(blockSize);
		
		//VCB is updated with whatever is at position 0
		LBAread(VCB, 1, 0);
		printf("debug: %d \n", VCB->magic_num);

		/*
	 	checking if magic number of block 0 is 3.
	 	if it is 3 then VCB is already formatted,
	 	and we don't need to initialize.
		*/

		if (VCB->magic_num != 3)
		{
			VCB->magic_num = 3;
			VCB->total_blocks = 10;
			VCB->block_size = 512;
			VCB->free_block_start = find_free_block(numberOfBlocks, blockSize);
			VCB->dir_entr_start = 2;  // this might be where the root directory is positioned.
			VCB->fat_start = 1;		  // this is where the fat is positioned.
		}

		printf("entered\n");
		
		return 0;
	}
	
	
void exitFileSystem ()
	{
		printf ("System exiting\n");
	}