/**************************************************************
* Class:  CSC-415-03 Spring 2022
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
* File: globals.c
*
* Description: This is a place for the most used global structs
* variable and function.
*
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

typedef struct vcb
{
	int magic_num;
	int block_size;
	int total_blocks;
	int free_block_start;
	int root_start;
	int root_size;
} vcb;

// VCB is declared globally here
vcb * VCB;

typedef struct dir_entr
{
	char filename[20];
	int starting_block;
	int size;
	int is_file;
	int mode;

	char create_time[20];
	char access_time[20];
	char modify_time[20];
} dir_entr;

void update_time(char buffer[20])
{
	time_t timestamp = time(NULL);
	struct tm *get_time = gmtime(&timestamp);
	char temp_buf[256];
	strftime(temp_buf, sizeof(temp_buf), "%F %T", get_time);
	strncpy(buffer, temp_buf, 20);
}