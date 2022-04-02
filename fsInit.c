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

typedef struct vcb{
	int block_size; 	//1
	int total_blocks;	//2
	int total_free_blocks;	//3
	int fat_start;	//4	
	int fat_len;	//5
	int free_block_start;	//6
	int dir_entr_start;	//7
	int dir_entr_len;	//8
	int magic_num;	//9
}vcb;


int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	vcb * vcb_buffer = malloc(blockSize);	//default blockSize is 512
	//vcb * buffer = malloc(blockSize);
	LBAread(vcb_buffer, 1, 0);	//
	
	
	if(vcb_buffer->magic_num != 3){
		vcb_buffer->block_size = 512;
		vcb_buffer->total_blocks = 10000000;
		//vcb_buffer->total_free_blocks = 5;
		vcb_buffer->fat_start = 1;
		vcb_buffer->fat_len = 6;
		vcb_buffer->free_block_start = 2;
		vcb_buffer->dir_entr_start = 1;
		//vcb_buffer->dir_entr_len;
		vcb_buffer->magic_num = 3;
	}
	

	LBAwrite(vcb_buffer, 1, 0);
	printf("Block Size: %d\n",vcb_buffer->block_size);
	printf("Total Blocks: %d\n", vcb_buffer->total_blocks);
	//printf("TotalFreeBlocks: %d\n", vcb_buffer->total_free_blocks);
	printf("Fat Starts at: %d\n", vcb_buffer->fat_start);
	printf("Fat Length: %d\n", vcb_buffer->fat_len);
	printf("Free Block Starts at: %d\n", vcb_buffer->free_block_start);
	printf("Directory Entry Starts at : %d\n", vcb_buffer->dir_entr_start);
	//printf("Dir Entry Length: %d\n", vcb_buffer->dir_entr_len);
	printf("Magic Num: %d\n",vcb_buffer->magic_num);
	



	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}