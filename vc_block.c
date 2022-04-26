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

void update_time(char my_time[20])
{
	time_t timestamp = time(NULL);
	struct tm *ptm = gmtime(&timestamp);
	char buf[256];
	strftime(buf, sizeof buf, "%F %T", ptm);
	strncpy(my_time, buf, 20);
}