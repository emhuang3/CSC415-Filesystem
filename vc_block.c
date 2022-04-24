#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

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

	// used tell if this is a file or directory
	int is_file;

	// will replace time, user_ID, group_ID with modified, created, and accessed timestamps
	int time;

	int mode;
	
	uid_t user_ID;
	gid_t group_ID;

} dir_entr;