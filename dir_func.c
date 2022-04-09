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
dir_entr * parent; // this will be updated in the validate_path() function
void create_dir(char * name, int permissions) 
{
    if (dirent == NULL)
    {
        dirent = malloc(VCB->block_size*6);
    }
    
    strncpy(dirent[0].filename, name, sizeof(name));

    // these two will lines will always be true so no need to put them in if statments
    dirent[0].size = dirent[1].size = sizeof(dir_entr) * 64;
	dirent[0].is_file = dirent[1].is_file = 0;

    // if root
    if (strcmp(dirent[0].filename, ".") == 0)
    {
        VCB->root_start = dirent[0].starting_block = 
        dirent[1].starting_block = allocate_space(5);
        dirent[0].permissions = dirent[1].permissions = permissions;
        strncpy(dirent[1].filename, "..", 2);
        printf("------------CREATED ROOT DIRECTORY------------\n");
    }
    else // if not root
    {
        dirent[0].starting_block = allocate_space(5);
        dirent[1].starting_block = parent->starting_block;
        dirent[0].permissions = permissions;
        dirent[1].permissions = parent->permissions;
        strncpy(dirent[1].filename, parent->filename, sizeof(parent->filename));
        printf("------------CREATED NEW DIRECTORY------------\n");
    }

    printf("Size of dir: %d \n", dirent[0].size);
	printf("dir[0].starting_block : %d \n", dirent[0].starting_block);
    printf("dir[1].starting_block : %d \n", dirent[1].starting_block);
	printf("dir[0].filename : %s \n", dirent[0].filename);
	printf("dir[1].filename : %s \n", dirent[1].filename);
    printf("dir[0].permissions : %d \n", dirent[0].permissions);
    printf("dir[1].permissions : %d \n\n", dirent[1].permissions);


    for (int i = 2; i < 64; i++)
	{
		// empty filename will imply that it is free to write to
		strncpy(dirent[i].filename, "", 0);
	}

    LBAwrite(dirent, 6, dirent[0].starting_block);

	LBAwrite(VCB, 1, 0);

    free(dirent);
	dirent = NULL;

    free(parent);
    parent = NULL;
}

// to be called in mk_dir()
int validate_path(char * pathnames, int starting_block, int counter) {
    if (parent == NULL)
    {
        parent = malloc(VCB->block_size*6);
    }

    LBAread(dirent, 6, starting_block);
    int my_count = counter++;

    // check that parent contains path

    //if counter == pathnames.size, then check that path does not exist in latest directory
}

char * parse_path(char * pathname) 
{
    // call str_tok to divide pathname into an array

    // return array
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

