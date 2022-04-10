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
#include "dir_func.c"

/*
This function is used for cleaning the blocks.
This is done to produce nice looking hexdumps.
*/

void flush_blocks(int numOfBlocks, uint64_t blockSize)
{
	uint64_t * clean_this_block = malloc(blockSize);

	for (int i = 0; i < numOfBlocks; i++)
	{
		LBAread(clean_this_block, 1, i);
		for (int j = 0; j < blockSize / 8; j++)
		{
			clean_this_block[j] = 0;
		}
		LBAwrite(clean_this_block, 1, i);
	}

	free(clean_this_block);
	clean_this_block = NULL;
}


int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */

	VCB = malloc(blockSize);
	
	//VCB is updated with whatever is at position 0
	LBAread(VCB, 1, 0);

	
	//checking if magic number is a match
	if (VCB->magic_num != 4)
	{
		flush_blocks(numberOfBlocks, blockSize);

		VCB->magic_num = 4;
		VCB->total_blocks = numberOfBlocks;
		VCB->block_size = blockSize;
		
		init_bitmap(numberOfBlocks, blockSize); 
		update_free_block_start(blockSize);

		// --------- INIT ROOT DIRECTORY ---------- //

		create_dir(".", 700);

		//---------- TESTING MAKE DIR -----------//

		// fs_mkdir("/home", 777);

		// simulating a reset to root directory
		// free(cur_dir);
		// cur_dir = NULL;

		// fs_mkdir("/home/Documents", 707);
	}
		
		return 0;
}
	
	
void exitFileSystem ()
{	
	
	if (VCB != NULL)
	{
		free(VCB);
		VCB = NULL;
	}
	
	if (buffer_bitmap != NULL)
	{
		free(buffer_bitmap);
		buffer_bitmap = NULL;
	}
	
	printf ("System exiting\n");
}