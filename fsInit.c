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

#include <math.h>

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

typedef struct dir_entr{
	int start_block;
	int size;
	int file_type;
	int persmissions;
	char filename[20];
	uid_t user_ID;
	gid_t group_ID;
}dir_entr;

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	vcb * vcb_buffer = malloc(blockSize);	//default blockSize is 512
	//vcb * buffer = malloc(blockSize);
	LBAread(vcb_buffer, 1, 0);	//
	
	// printf("%llu", LBAread(vcb_buffer,1,0));
	// printf("---%s---\n", vcb_buffer);
	
	if(vcb_buffer->magic_num != 3){
		vcb_buffer->block_size = blockSize;
		vcb_buffer->total_blocks = numberOfBlocks;
		vcb_buffer->total_free_blocks = 8;
		vcb_buffer->fat_start = 1;
		vcb_buffer->fat_len = 6;
		//vcb_buffer->free_block_start = 1;
		vcb_buffer->dir_entr_start = 1;
		vcb_buffer->dir_entr_len = 50;
		vcb_buffer->magic_num = 3;
	}
	
	LBAwrite(vcb_buffer, 1, 0);

	
	//-------------------------------------------------------
	
	/*x # of blocks = x # of bits in default case 19531 blocks
	so this means 19531 bits*/
	printf("-------------\nNumber of Bits: %d\n",vcb_buffer->total_blocks);
	/*To find # of bytes divide total blocks by 8.0 
	a float because we want the ceiling value*/
	int byte_num = (int)ceil(vcb_buffer->total_blocks/8.0);	
	printf("Number Of Bytes: %d\n", byte_num);
	/*To find the number of blocks in free space divide bytes 
	by blockSize; default block size is 512. Then read 
	vcb buffer and then write the total num of free blocks to it*/
	LBAread(vcb_buffer, 1, 0);
	vcb_buffer->total_free_blocks = (int)ceil((float)byte_num/blockSize);
	LBAwrite(vcb_buffer,1,0);
	//int num_of_freespace_block = (int)ceil((float)byte_num/blockSize);
	printf("Number Of Free Space Blocks: %d\n", vcb_buffer->total_free_blocks);

	/*Now malloc freespace */
	int * free_space = malloc(vcb_buffer->total_free_blocks*blockSize);
	LBAwrite(free_space, 5, 1);
	//mark the first 6 bits as used
	for(int i = 0; i<6; i++){
		free_space[i] = 1;
		//printf("%d", free_space[i]);
	}
	
	//write to vcb where free blocks start
	LBAread(vcb_buffer, 1, 0);
	vcb_buffer->free_block_start = 1;
	LBAwrite(vcb_buffer,1,0);

	//--------------------------------------------------------

	printf("-------------\nBlock Size: %d\n",vcb_buffer->block_size);
	printf("Total Blocks: %d\n", vcb_buffer->total_blocks);
	printf("TotalFreeBlocks: %d\n", vcb_buffer->total_free_blocks);
	printf("Fat Starts at: %d\n", vcb_buffer->fat_start);
	printf("Fat Length: %d\n", vcb_buffer->fat_len);
	printf("Free Block Starts at: %d\n", vcb_buffer->free_block_start);
	printf("Directory Entry Starts at : %d\n", vcb_buffer->dir_entr_start);
	printf("Directory Entry Length: %d\n", vcb_buffer->dir_entr_len);
	printf("Magic Num: %d\n",vcb_buffer->magic_num);

	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}

	