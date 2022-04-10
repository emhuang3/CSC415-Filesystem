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
dir_entr * cur_dir; // this will be updated in the validate_path() function
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
        dirent[1].starting_block = cur_dir->starting_block;
        dirent[0].permissions = permissions;
        dirent[1].permissions = cur_dir->permissions;
        strncpy(dirent[1].filename, cur_dir->filename, sizeof(cur_dir->filename));
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

    // free(cur_dir);
    // cur_dir = NULL;
}

char * saved_filename;
int ret;
int num_of_paths;

// to be called in parse_path(). This function will update current working directory
int validate_path(char * name) {

    // this represents the paths remaining to be searched
    num_of_paths--;

    if (cur_dir == NULL)
    {
        cur_dir = malloc(VCB->block_size*6);
        LBAread(cur_dir, 6, VCB->root_start);
    }

    //iterate through dir_entries of current directory
    for (int i = 2; i < 64; i++)
    {
        if (strcmp(cur_dir[i].filename, name) == 0 && num_of_paths > 0)
        {
            printf("found path\n");

            // updating current directory with found path
            LBAread(cur_dir, 6, cur_dir[i].starting_block);

            printf("Current directory: %s\n", cur_dir[0].filename);
            return 0;
        }
        else if (i == 63 && num_of_paths == 0)
        {
            printf("path does not exist\n");
            printf("able to create this directory\n");

            // save name
            saved_filename = name;
            return 0;
        }
        else if (strcmp(cur_dir[i].filename, name) == 0 && num_of_paths == 0)
        {
            printf("path already exists\n");
            printf("NOT able to create this directory\n");

            // updating current directory with found path
            LBAread(cur_dir, 6, cur_dir[i].starting_block);

            printf("Current directory: %s\n", cur_dir[0].filename);
            return -1;
        }
        else if (i == 63 && num_of_paths > 0)
        {
            // this else-container implies that path was not found while there are still paths left to search
            printf("path does not exist\n");
            printf("NOT able to create this directory\n");

            return -1;
        }
    }
}


int parse_pathname(const char * pathname) 
{
    char buffer_pathname[20];
    char count_slashes[20];
    
    strncpy(count_slashes, pathname, strlen(pathname));
    strncpy(buffer_pathname, pathname, strlen(pathname));

    printf("buffer_pathname: %s\n", buffer_pathname);

    for (int i = 0; i < strlen(count_slashes); i++)
    {
        if (count_slashes[i] == '/')
        {
            num_of_paths++;
        }
    }

    printf("num of paths: %d\n", num_of_paths);

    //just in case there is a newline character
    char * newLine = strchr(buffer_pathname, '\n');
    if (newLine)
    {
        * newLine = ' ';
    } 

    // call str_tok to divide pathname into an array
    char * name = strtok(buffer_pathname, "/");
    
    while (name != NULL)
    {

        printf("Pathname: %s : ", name);
        ret = validate_path(name);
        if (ret == -1) 
        {
            printf("failed to validate path\n");
            return ret;
        }

        name = strtok(NULL, "/");
    }

    return ret;
}

int fs_mkdir(const char * pathname, mode_t mode) 
{
    // will return -1  if invalid, and 0 if success
    int ret = parse_pathname(pathname);

    if (ret == 0)
    {

        //searching for a free directory entry to write to in current directory
        for (int i = 2; i < 64; i++)
        {
            if (strcmp(cur_dir[i].filename, "") == 0)
            {
                //creating new directory
                strncpy(cur_dir[i].filename, saved_filename, sizeof(saved_filename));
                cur_dir[i].starting_block = VCB->free_block_start;

                printf("Current Directory: %s\n", cur_dir[0].filename);
                printf("%s added to index %d in %s Directory\n", cur_dir[i].filename, i, cur_dir[0].filename);
                printf("%s starting block: %d\n\n", cur_dir[i].filename, VCB->free_block_start);

                create_dir(saved_filename, mode);

                // update current directory on disk
                LBAwrite(cur_dir, 6, cur_dir[0].starting_block);

                // reset current working directory if needed
                free(cur_dir);
                cur_dir = NULL;
                
                break;
            }
            else if (i == 63)
            {
                printf("Unable to find free directory entry inside /%s\n", cur_dir[0].filename);
            }
        }
    }
    return ret;
}

int fs_rmdir(const char *pathname) 
{

}
