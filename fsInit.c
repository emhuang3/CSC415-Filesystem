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
#include "freeAlloc.c"

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

	/*
	checking if magic number of block 0 is 3.
	if it is 3 then VCB is already formatted,
	and we don't need to initialize.
	*/

	if (VCB->magic_num != 3)
	{
		flush_blocks(numberOfBlocks, blockSize);
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
			
		// printf("Size of dir_entr: %ld \n", sizeof(dir_entr));
		// printf("blocks to allocate for root: %ld \n\n", (sizeof(dir_entr)* 64) / 512);

		strncpy(root_dir[0].filename, ".", 1);
		strncpy(root_dir[1].filename, "..", 2);

		root_dir[0].size = root_dir[1].size = sizeof(dir_entr) * 64;

		root_dir[0].permissions = root_dir[1].permissions = 700;

		root_dir[0].is_file = root_dir[1].is_file = 0;

		/*
		I want to allocate 6 blocks for the root. allocate_space() 
		will return the starting block in the freespace bitmap.
		VCB->dir_entr_start will represent the starting block of
		the root directory.
		*/

		VCB->dir_entr_start = root_dir[0].starting_block = allocate_space(5, numberOfBlocks);
		
		root_dir[1].starting_block = root_dir[0].starting_block;
		

		// printf("Size of root_dir: %d \n", root_dir[0].size);
		// printf("root_dir[0].starting_block : %d \n", root_dir[0].starting_block);
		// printf("root_dir[0].filename : %s \n", root_dir[0].filename);
		// printf("root_dir[1].filename : %s \n\n", root_dir[1].filename);

		for (int i = 2; i < 64; i++)
		{
			// empty filename will imply that it is free to write to
			strncpy(root_dir[i].filename, "", 0);
		}

		LBAwrite(root_dir, 6, root_dir[0].starting_block);

		LBAwrite(VCB, 1, 0);

		//---------- cleaning up malloc'd spaces ---------- //

		free(root_dir);
		root_dir = NULL;
	}

		free(VCB);
		VCB = NULL;
		
		return 0;
}
	
	
void exitFileSystem ()
{
	if (buffer_bitmap != NULL)
	{
		free(buffer_bitmap);
		buffer_bitmap = NULL;
	}
	
	printf ("System exiting\n");
}