#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <mfs.h>
#include "freeAlloc.c"
/*
example of make dir with permissions
status = mkdir("/home/cnd/mod1", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
*/

dir_entr * dirent;
void create_dir(char * name, char * parentName, int parent_starting_block, int permissions) 
{
    if (dirent == NULL)
    {
        dirent = malloc(VCB->block_size*6);
    }
    
    strncpy(dirent[0].filename, name, sizeof(name));
	strncpy(dirent[1].filename, parentName, sizeof(parentName));

    dirent[0].size = dirent[1].size = sizeof(dir_entr) * 64;
    dirent[0].permissions = dirent[1].permissions = permissions;
	dirent[0].is_file = dirent[1].is_file = 0;

    dirent[1].starting_block = parent_starting_block;
    VCB->root_start = dirent[0].starting_block = allocate_space(5);

    printf("------------CREATED NEW DIRECTORY------------\n");
    printf("Size of new dir: %d \n", dirent[0].size);
	printf("dir[0].starting_block : %d \n", dirent[0].starting_block);
    printf("dir[1].starting_block : %d \n", dirent[1].starting_block);
	printf("dir[0].filename : %s \n", dirent[0].filename);
	printf("dir[1].filename : %s \n\n", dirent[1].filename);


    for (int i = 2; i < 64; i++)
	{
		// empty filename will imply that it is free to write to
		strncpy(dirent[i].filename, "", 0);
	}

    LBAwrite(dirent, 6, dirent[0].starting_block);

	LBAwrite(VCB, 1, 0);
}

// to be called in mk_dir()
int find_dir(char * pathnames, int starting_block, int counter) {
    if (dirent == NULL)
    {
        dirent = malloc(VCB->block_size*6);
    }

    LBAread(dirent, 6, starting_block);
    int my_count = counter++;


}

char * parse_path(char *pathname) 
{

}

int fs_mkdir(const char *pathname, mode_t mode) 
{
    // will return -1  if invalid, and 0 if success

    // call parse path to divide string into array of searchable strings

    // call find dir path. this will return a value depending on success or failure

    //call create_dir if success else return -1
}

int fs_rmdir(const char *pathname) 
{

}

