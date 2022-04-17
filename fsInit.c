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
#include "b_io.c"

/*
This function is used for cleaning the blocks.
This is done to produce nice looking hexdumps.
*/

void flush_blocks(int numOfBlocks, uint64_t blockSize)
{
	uint64_t * clean_this_block = malloc(blockSize);

	// checking if malloc was successful
	if (clean_this_block == NULL)
	{
		printf("ERROR: failed to malloc.\n");
		exit(-1);
	}

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

	// checking if malloc was successful
	if (VCB == NULL)
	{
		printf("ERROR: failed to malloc.\n");
		exit(-1);
	}
	
	
	//VCB is updated with whatever is at position 0
	LBAread(VCB, 1, 0);

	
	//checking if magic number is a match
	if (VCB->magic_num != 5)
	{
		flush_blocks(numberOfBlocks, blockSize);

		VCB->magic_num = 5;
		VCB->total_blocks = numberOfBlocks;
		VCB->block_size = blockSize;
		
		init_bitmap(); 
		update_free_block_start(blockSize);

		// --------- INIT ROOT DIRECTORY ---------- //

		create_dir(".", 700);

		// temp placement

		// setting current working directory to root
		curr_dir = malloc(VCB->block_size * 6);
		
		// checking if malloc was successful
		if (curr_dir == NULL)
		{
			printf("ERROR: failed to malloc.\n");
			exit(-1);
		}

		LBAread(curr_dir, 6, VCB->root_start);

		/// --------------- TEST DIRECTORIES --------------- //

		fs_mkdir("/school", 511);
		fs_mkdir("/work", 511);
		fs_mkdir("/personal", 511);
	}

	if (curr_dir == NULL)
	{
		// setting current working directory to root
		curr_dir = malloc(VCB->block_size * 6);
		
		// checking if malloc was successful
		if (curr_dir == NULL)
		{
			printf("ERROR: failed to malloc.\n");
			exit(-1);
		}

		LBAread(curr_dir, 6, VCB->root_start);
	}

	return 0;
}
	
	
void exitFileSystem ()
{	
	if (curr_dir != NULL)
	{
		free(curr_dir);
		curr_dir = NULL;
	}
	
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