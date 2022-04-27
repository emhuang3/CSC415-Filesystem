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

/* This function is used for cleaning the blocks.
This is done to produce nice looking hexdumps,
and to make debugging easier. */

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

	/* This malloc'd VCB will bring block 0 into memory to see 
	if the existing volume's magic number is a match. This
	buffer will stay in memory and update the filesys's 
	dedicated VCB block 0. */

	VCB = malloc(blockSize);

	/*
	whenevery a malloc fails, we will either exit the program,
	or a particular cmd depending on the severity of the failure.
	In this case we will exit the program, since a VCB buffer is
	crucial to the functionality of our filesys.
	*/

	if (VCB == NULL)
	{
		printf("ERROR: failed to malloc VCB.\n");
		exit(-1);
	}
	
	/* The VCB buffer is updated with position 0 in the LBA.
	 this is where we store our VCB, so we want to know if
	 the sys has already been initilized by checking the 
	 magic number. */

	LBAread(VCB, 1, 0);

	
	//checking if magic number is a match

	if (VCB->magic_num != 3)
	{
		// THIS IS TEMPORARY
		flush_blocks(numberOfBlocks, blockSize);

		printf("\nformatting volume control block...\n");

		VCB->magic_num = 3;
		VCB->total_blocks = numberOfBlocks;
		VCB->block_size = blockSize;
		
		/* This function writes a bitmap into the LBA,
		 which tracks which blocks are free to write
		 to, and which blocks are already allocated. */

		printf("initializing freespace bitmap...\n");
		init_bitmap(); 

		/* This function will update the VCB's free_block_start 
		 to the available block to write to in the LBA. */

		update_free_block_start();

		// this function initializes a root directory
		
		printf("initializing root directory...\n\n");
		create_dir(".", 700);

		// -------------- TEMP PLACEMENT ----------- //

		// setting current working directory to root
		curr_dir = calloc(6, VCB->block_size);
		
		// checking if malloc was successful
		if (curr_dir == NULL)
		{
			printf("ERROR: failed to malloc.\n");
			exit(-1);
		}

		LBAread(curr_dir, 6, VCB->root_start);

		printf("formatting complete.\n\n");

		/// --------------- TEST DIRECTORIES --------------- //

		// fs_mkdir("/school", 511);
		// fs_mkdir("/work", 511);
		// fs_mkdir("/personal", 511);
		// fs_mkdir("/home", 511);
		// fs_mkdir("/home/student", 511);
		// fs_mkdir("/home/student/Docs", 511);
		// fs_mkdir("/school", 511);
		// fs_mkdir("/personal", 511);
		// fs_mkdir("/games", 511);

		// fs_mkdir("/personal/games", 511);

		// fs_mkdir("/personal/games/elden_ring", 511);
		// fs_mkdir("/personal/games/forza_5", 511);
		// fs_mkdir("/personal/games/among_us", 511);
	}

	// if VCB is already formatted, then we will bring existing freespace into memory.
	if (buffer_bitmap == NULL)
	{
		reset_bitmap();
	}

	if (curr_dir == NULL)
	{
		/* This is setting the root to be the current working directory.
		 This will stay in memory, as curr_dir will always point to
		 the current working directory. */

		curr_dir = malloc(VCB->block_size * 6);
		
		if (curr_dir == NULL)
		{
			printf("ERROR: failed to malloc a current working directory.\n");
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