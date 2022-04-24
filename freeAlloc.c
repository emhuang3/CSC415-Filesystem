#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "vc_block.c"

/*
This is a buffer for the freespace bitmap,
which will stay in memory and update the 
freespace bitmap in the LBA.
*/

uint * buffer_bitmap;

/*
 this helper function allows us to check if a particular 
 bit is occupied in the bitmap. The function takes an index representing
 a block in the LBA, to check if the associated bit is allocated or not.
*/

int is_allocated(int index)
{
    return ((buffer_bitmap[index / 32] & (1 << (index % 32))) != 0);
}

/*
 this helper function allows us to allocate a bit inside an int,
 at an index representing a block in the LBA.
*/
void allocate_bit(int index)
{
    buffer_bitmap[index / 32] |= 1 << (index % 32);
}

/*
 this helper function clears a bit at an index representing a block
 in the LBA.
*/

void clear_bit(int index)
{
    buffer_bitmap[index / 32] &= ~(1 << (index % 32));
}

/*
 this helper function converts a byte size to a number 
 of blocks that could store the bytes
*/

int convert_size_to_blocks(int byte_size, int block_size) 
{
	float top_num = (float) byte_size;
	float bottom_num = (float) block_size;
	int block_count = ceil(top_num/bottom_num);

	return block_count;
}

// this function finds the first free block in the freespace bitmap
void update_free_block_start() 
{
	for (int i = 0; i < VCB->total_blocks; i++)
	{
		if (!is_allocated(i))
		{	
			VCB->free_block_start = i;
			LBAwrite(VCB, 1, 0);
			break;
		}
	}
}

// this function clears space when removing or overwriting a file.
void reallocate_space(dir_entr * directory, int index, int save_state)
{
	// this stores the number of blocks the directory occupies
	int block_count = convert_size_to_blocks(directory[index].size, VCB->block_size);

	// this clears the bits the directory currently occupies.
	for (int i = directory[index].starting_block; i < directory[index].starting_block + block_count; i++)
	{
		clear_bit(i);
	}

	/*
	 This provides the caller the option to prevent updating disk during volatile writing process, if
	 something outside of this function fails, which will require the caller to update the bitmap.
	*/

	if (!save_state)
	{
		LBAwrite(buffer_bitmap, 5, 1);

		// updating free block start in VCB
		update_free_block_start();
	}
}

/* this function moves a file/dir to the next conguously available freespace. directory represents 
the parent and index points to the child file/directory with the associated start block. */

int move_on_disk(dir_entr * directory, int index, int count)
{
	int amount_to_alloc = convert_size_to_blocks(directory[index].size, VCB->block_size);
	int starting_index = count + directory[index].starting_block;
	int j = 0;
	int new_starting_block = 0;

	for (int i = starting_index; i < VCB->total_blocks; i++)
	{
		/* if this bit is free, then check that the next 'amount_to_alloc' is free, before writing to 
		the contiguous space. This will move the file/directory on disk. */

		if (!is_allocated(i))
		{
			j++;
		}

		// reset j to 0 if next block is occupied. This is done to find contiguous space for the moving file/directory on disk.
		else
		{
			j = 0;
		}

		// if j is equal to the amount to allocate, then we found contiguous space and can write it to the buffered bitmap.
		if (j == amount_to_alloc)
		{
			new_starting_block = i - j;
			for (int k  = i - j; k < i; k++)
			{
				allocate_bit(k);
			}
			i = VCB->total_blocks;
		}
	}
	
	// reallocate_space will remove old position of directory.
	reallocate_space(directory, index, 0);

	int child_block_count = convert_size_to_blocks(directory[index].size, VCB->block_size);
	int parent_block_count = convert_size_to_blocks(directory[0].size, VCB->block_size);
	dir_entr * child_dir = malloc(child_block_count * VCB->block_size);

	if (child_dir == NULL)
	{
		printf("failed to malloc while trying to move a file or directory on disk.\n");
		return -1;
	}
	
	LBAread(child_dir, child_block_count, directory[index].starting_block);

	if (directory[index].is_file == 0)
	{
		// update metadata in child
		child_dir[0].starting_block = new_starting_block;
	}

	// this writes the moving child to the new sapce on disk
	LBAwrite(child_dir, child_block_count, new_starting_block);

	// update metadata in parent, and child if child is a directory.
	directory[index].starting_block = new_starting_block;
	
	// this writes updated parent to disk.
	LBAwrite(directory, parent_block_count, directory[0].starting_block);

	free(child_dir);
	child_dir = NULL;

	return 0;
}


/* this function find a directory associated with the given starting block, 
and moves it by calling move_on_disk(). */

int find_on_disk(dir_entr * directory, int find_this_start_block, int count)
{
	int ret = 0;
	if (find_this_start_block == VCB->root_start)
	{
		// this will probably never occur.
		printf("cannot move root.\n");
		return -1;
	}
	
	int index = 2;
	while (strcmp(directory[index].filename, "") != 0) 
	{
		if (directory[index].starting_block == find_this_start_block)
		{
			ret = move_on_disk(directory, index, count);
			if (ret == 0)
			{
				return 0;
			}
		}

 		/* call a find_on_disk() to search the children for the starting block, but only if the child is a 
		 directory, since files cannot contain children. */

		else if (directory[index].is_file == 0)
		{
			int child_block_count = convert_size_to_blocks(directory[index].size, VCB->block_size);
			dir_entr * child_dir = malloc(child_block_count * VCB->block_size);
			LBAread(child_dir, child_block_count, directory[index].starting_block);

			ret = find_on_disk(child_dir, find_this_start_block, count);
			if (ret == 0)
			{
				if (child_dir == NULL)
				{
					free(child_dir);
					child_dir = NULL;
				}

				// this implies that directory with associated starting block was found and moved.
				return 0;
			}
			
			if (child_dir == NULL)
			{
				free(child_dir);
				child_dir = NULL;
			}
		}

		// check that next index is not over the size of the directory.
		if (index + 1 > directory[0].size / sizeof(dir_entr))
		{
			return -1;
		}
		index++;
	}
	return -1;
}


// resets bitmap in the event that allocation fails
void reset_bitmap()
{
	if (buffer_bitmap != NULL)
	{
		free(buffer_bitmap);
		buffer_bitmap = NULL;
	}

	buffer_bitmap = calloc(5, VCB->block_size);

	if (buffer_bitmap == NULL)
	{
		printf("failed to malloc bitmap.\n");
		exit(-1);
	}
	
	LBAread(buffer_bitmap, 5, 1);;
}

/*
 This function will allocate contiguous space in the LBA by counting 
 the number of blocks to allocate from the first available block and 
 calling move_on_disk() for all files that are in the way.
*/

int allocate_space(int amount_to_alloc)
{
	dir_entr * temp_root_dir;
	int root_block_count = convert_size_to_blocks(VCB->root_size, VCB->block_size);
	temp_root_dir = malloc(root_block_count * VCB->block_size);
	LBAread(temp_root_dir, root_block_count, VCB->root_start);

	/*
	 this number will be returned to the caller to provide a starting 
	 number for thier allocated space
	*/

	int previous_free_block_start = VCB->free_block_start;
	
	// j will iterate as space is being allocated for the caller
	int j = 0; 
	int ret = 0;

	/*
	this iterates through the buffer_bitmap to see if block is free or occupied.
	If it is occupued, move_on_disk() will move the contents of the block to the
	next contiguously available space.
	*/

	for (int i = VCB->free_block_start; i < VCB->total_blocks; i++)
	{
		
		if (j < amount_to_alloc)
		{
						
			// block is free, therefore we can mark this as occupied for the caller
			if (!is_allocated(i))
			{
				allocate_bit(i);
				j++;
			}
			else
			{
				ret = find_on_disk(temp_root_dir, i, amount_to_alloc - j + 1);

				if (ret == -1)
				{
					printf("Not enough space on disk.\n");
					reset_bitmap();
					return -1;
				}
				
				allocate_bit(i);
				j++;
			}
		}

		// this implies there is no free space left.
		else if (i == VCB->total_blocks - 1) 
		{
			printf("Not enough space on disk.\n");
			reset_bitmap();
			return -1;
		}

		// exits if no more blocks need to be allocated.
		else
		{
			i = VCB->total_blocks;
		}

	}

	// push updated bitmap to disk
	LBAwrite(buffer_bitmap, 5, 1);

	// updates VCB with new first free block position.
	update_free_block_start();

	free(temp_root_dir);
	temp_root_dir = NULL;

	return previous_free_block_start;
}

// this function initilizes a bitmap if the VCB needs to be formatted.
void init_bitmap()
{

	int first_free_block = 0;

	// creating bitmap for free space for the first time
	if (buffer_bitmap == NULL)
	{
		buffer_bitmap = malloc(VCB->block_size * 5);

		// checking if malloc was successful
		if (buffer_bitmap == NULL)
		{
			printf("ERROR: failed to malloc.\n");
			exit(-1);
		}
	}

	/*
	This is initializing dedicated space for VCB and freespace
	bitmap in the LBA.
	*/

	for (int i = 0; i < 6; i++)
	{
		allocate_bit(i);
	}

	LBAwrite(buffer_bitmap, 5, 1);

	update_free_block_start();
}

/* 
this helper function pushes children of parent leftward 
after an empty space was created in parent's entry list.
*/

void move_child_left(dir_entr * parent, int index)
{
    // index refers to the index that was just freed up.
    int fill_index = index; // this is the position that needs to be filled by rightmost child
    int iterator = index;   // this will be used to iterate through the parent.
	int is_full = 0;

    // this checks if we've reached the end of the entry list
    if ((iterator + 1) >= (parent[0].size/sizeof(dir_entr)))
    {
		// if removed/moved child was already the rightmost position, then we will just return.
		if (strcmp(parent[iterator + 1].filename, "") == 0)
		{
			return;
		}
        is_full = 1;
    }
    
	if (is_full == 0)
	{
		while (strcmp(parent[iterator + 1].filename, "") != 0)
    	{
        	iterator++;
        	if ((iterator + 1) > (parent[0].size/sizeof(dir_entr)))
        	{
				break;
        	}
    	}
	}

    // this will move the rightmost child's metadata to the fill position
    strcpy(parent[fill_index].filename, parent[iterator].filename);
    parent[fill_index].group_ID = parent[iterator].group_ID;
    parent[fill_index].is_file = parent[iterator].is_file;
    parent[fill_index].mode = parent[iterator].mode;
    parent[fill_index].size = parent[iterator].size;
    parent[fill_index].starting_block = parent[iterator].starting_block;
    parent[fill_index].time = parent[iterator].time;
    parent[fill_index].user_ID = parent[iterator].user_ID;

    // this will free the rightmost child
    memset(parent[iterator].filename, 0, sizeof(parent[iterator].filename));
}