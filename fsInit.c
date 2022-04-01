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

typedef struct fat
{
	int occupied;
	int eof;
	int next_block;
} fat;

typedef struct dir_entr
{
	int starting_block;
	int size;
	int file_type;
	int permissions;
	char filename[20];
	uid_t user_ID;
	gid_t group_ID;

} dir_entr;

// char * filename;
// uint64_t volume_size;
// uint64_t block_size;

char buffer[128];
vcb * VCB;

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	
		VCB = malloc(sizeof(VCB)*blockSize);

	//buffer is updated with whatever is at position 0
		LBAread(buffer, 1, 0); 

	// check if magic numbers match

		VCB->magic_num = 3;
		VCB->total_blocks = 10;
		VCB->block_size = 512;
		// VCB->free_block_start = find_free_block();
		VCB->dir_entr_start = 2; // this might be where the root directory is positioned.
		VCB->fat_start = 1;		// this is where the fat is positioned.

		// LBAwrite(/*bitmap*/, 5, VCB->free_block_start);
		// init_bitmap();
	
		LBAwrite(VCB, 1, 0);

	//  if (buffer == NULL)
	// {
	// 	VCB->magic_num = 3;
	// 	VCB->total_blocks = 1000000000;
	// 	VCB->block_size = 512;
	// 	// VCB->free_block_start = find_free_block();
	// 	VCB->dir_entr_start = 2; // this might be where the root directory is positioned.
	// 	VCB->fat_start = 1;		// this is where the fat is positioned.

	// 	// LBAwrite(/*bitmap*/, 5, VCB->free_block_start);
	// 	// init_bitmap();
	
	// LBAwrite(VCB, 1, 0);
	// }
	
	return 0;
	}
	
	
void exitFileSystem ()
	{
		//clean up vcb
		free(VCB);
		VCB = NULL;

		//clean up free space bitmap

		printf ("System exiting\n");
	}