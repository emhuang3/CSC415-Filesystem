/**************************************************************
* Class:  CSC-415-02/03 Fall 2021
* Names: Kilian Kistenbroker, Emily Huang, Sean Locklar, 
	Shauhin Pourshayegan
* Student IDs: , 920499746, 920506337, 920447681
* GitHub Name: KilianKistenbroker
* Group Name: Team Poke
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
	int next;
	int start_block;
	int size;
	int file_type;
	int persmissions;
	char filename[20];
	uid_t user_ID;
	gid_t group_ID;
}dir_entr;

vcb * vcb_buffer;


int freeSpaceStart(u_int8_t * free_space, int blocks){
	//free_space[10] =1;
	//free_space[7] =1;
	//printf("%ld", sizeof(free_space));
	//free_space = realloc(free_space,sizeof(free_space) + (sizeof(free_space) * blocks));
	int i = 0;
	//just finds first space that's empty
	while(free_space[i]!=0){
		//printf("%d", i);
		i++;
	}
	int free_space_start = i;	

	int end = i+blocks;
	for(int j = i; j<end;j++){
		if(free_space[j]==0){
			free_space[j] = 1;
		}
		else{
			end = end + 1;
			//printf("already occupied");
		}
		
		LBAwrite(free_space,  blocks, 1);
	}
	vcb_buffer->free_block_start = free_space_start;
	LBAwrite(vcb_buffer, 1, 0);
	
	return free_space_start;
}

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	vcb_buffer = malloc(blockSize);	//default blockSize is 512
	LBAread(vcb_buffer, 1, 0);	
	
	if(vcb_buffer->magic_num != 3){
		vcb_buffer->block_size = blockSize;
		vcb_buffer->total_blocks = numberOfBlocks;
		vcb_buffer->total_free_blocks = 8;	//8 is random value for testing
		vcb_buffer->fat_start = 1;
		vcb_buffer->fat_len = 6;	//6 is random value for testing
		vcb_buffer->free_block_start = 9;	//9 is random value for testing
		vcb_buffer->dir_entr_start = 1;
		vcb_buffer->dir_entr_len = 50;
		vcb_buffer->magic_num = 3;		
	}
	
	LBAwrite(vcb_buffer, 1, 0);

	
	//-------------------------------------------------------

	/*x # of blocks = x # of bits in default case 19531 blocks
	so this means 19531 bits To find # of bytes divide total blocks by 8.0 
	a float because we want the ceiling value*/
	int byte_num = (int)ceil(vcb_buffer->total_blocks/8.0);	
	/*To find the number of blocks in free space divide bytes 
	by blockSize; default block size is 512. Then read 
	vcb buffer and then write the total num of free blocks to it*/
	//LBAread(vcb_buffer, 1, 0);
	vcb_buffer->total_free_blocks = (int)ceil((float)byte_num/blockSize);
	LBAwrite(vcb_buffer,1,0);

	/*Now malloc freespace */
	u_int8_t * free_space = malloc(vcb_buffer->total_free_blocks*blockSize);
	LBAwrite(free_space, 5, 1);

	//mark the first 6 bits as used
	for(int i = 0; i<6; i++){
		free_space[i] = 1;
		
	}
	LBAwrite(free_space, 5, 1);
	
	//write to vcb where free blocks start
	vcb_buffer->free_block_start = 1;
	LBAwrite(vcb_buffer,1,0);

	//--------------------------------------------------------
	
	
	/*Lets say we want 50 direcotry entries. The size of a single 
	directory entry is 48. So we need enough blocks to fit 
	50*48 = 2400 bytes. For this we need at least 5 blocks.
	But 5 blocks mean 5*512 = 2560 bytes, meaning we have 
	room for for 8 more directory entries. So now we want 
	53 directory entries. This will be defined up top*/
	
	dir_entr * dir_entr_array = malloc(5*blockSize);
	for(int i =0; i<53;i++){
		//Initialize each directory structure to be in a free state?
		dir_entr_array[i].next = 0;
		
	}
	/*call freeSpaceStart to find the next free space to be the
	starting block for the next directory entry*/
	int dir_entr_starting_block = freeSpaceStart(free_space, 5);
	LBAread(dir_entr_array, 5,dir_entr_starting_block);
	

	//set values for directory entry 0
	dir_entr_array[0].next = 1;
	dir_entr_array[0].start_block = dir_entr_starting_block;
	/*2544 because that's the max number of files that
	can fit into the 5 allocated blocks(2560)*/
	dir_entr_array[0].size = 2544;
	dir_entr_array[0].file_type = 1;
	//only user will have read, write,execute permissions
	dir_entr_array[0].persmissions = 700;
	strcpy(dir_entr_array[0].filename, ".");
	//user will have access to root
	dir_entr_array[0].user_ID = 1;
	//groups will not have access to root
	dir_entr_array[0].group_ID = 0;

	//set values for directory entry 1;
	dir_entr_array[1].next = 1;
	dir_entr_array[1].start_block = dir_entr_starting_block;
	/*2544 because that's the max number of files that
	can fit into the 5 allocated blocks(2560)*/
	dir_entr_array[1].size = 2544;
	dir_entr_array[1].file_type = 1;
	//only user will have read, write,execute permissions
	dir_entr_array[1].persmissions = 700;
	strcpy(dir_entr_array[1].filename, "..");
	//user will have access to root
	dir_entr_array[1].user_ID = 1;
	//groups will not have access to root
	dir_entr_array[1].group_ID = 0;
	LBAwrite(dir_entr_array, 5, dir_entr_starting_block);

	//return starting block to vcb
	vcb_buffer->dir_entr_start = dir_entr_starting_block;
	LBAwrite(vcb_buffer, 1, 0);

	//printf("Free Block starts at: %d", vcb_buffer->free_block_start);
	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}

	