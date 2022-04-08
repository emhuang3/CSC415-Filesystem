#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

typedef struct vcb
{
	int block_size;
	int total_blocks;
	int free_block_start;
	int dir_entr_start;
	int magic_num;
} vcb;

// VCB is declared globally here
vcb * VCB;

typedef struct dir_entr
{

	int starting_block;
	int size;

	// used tell if this is a file or directory
	int is_file;

	// points to the next file. if null then it is free
	int next; 

	int permissions;
	char filename[20];
	uid_t user_ID;
	gid_t group_ID;

} dir_entr;