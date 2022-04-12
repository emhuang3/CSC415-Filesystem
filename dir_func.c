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

dir_entr * temp_dir;      // this is a temp dir for making/removing directories
dir_entr * temp_curr_dir; // this will represent a temp pointer for pathname validation and directory manipulation
dir_entr * curr_dir;      // this will be kept in memory as a pointer to a current working directory

void create_dir(char * name, int permissions) 
{
    if (temp_dir == NULL)
    {
        temp_dir = malloc(VCB->block_size*6);

        // checking if malloc was successful
	    if (temp_dir == NULL)
	    {
		    printf("ERROR: failed to malloc.\n");
		    exit(-1);
	    }
    }

    
    
    strncpy(temp_dir[0].filename, name, strlen(name));

    // these two will lines will always be true so no need to put them in if statments
    temp_dir[0].size = temp_dir[1].size = sizeof(dir_entr) * 64;
	temp_dir[0].is_file = temp_dir[1].is_file = 0;

    // if root
    if (strcmp(temp_dir[0].filename, ".") == 0)
    {
        VCB->root_start = temp_dir[0].starting_block = 
        temp_dir[1].starting_block = allocate_space(5);
        temp_dir[0].permissions = temp_dir[1].permissions = permissions;
        strncpy(temp_dir[1].filename, "..", 2);
        printf("------------CREATED ROOT DIRECTORY------------\n");
    }
    else // if not root
    {
        temp_dir[0].starting_block = allocate_space(5);
        temp_dir[1].starting_block = temp_curr_dir->starting_block;
        temp_dir[0].permissions = permissions;
        temp_dir[1].permissions = temp_curr_dir->permissions;
        strncpy(temp_dir[1].filename, temp_curr_dir->filename, strlen(temp_curr_dir->filename));
        printf("------------CREATED NEW DIRECTORY------------\n");
    }

    printf("Size of dir: %d \n", temp_dir[0].size);
	printf("dir[0].starting_block : %d \n", temp_dir[0].starting_block);
    printf("dir[1].starting_block : %d \n", temp_dir[1].starting_block);
	printf("dir[0].filename : %s \n", temp_dir[0].filename);
	printf("dir[1].filename : %s \n", temp_dir[1].filename);
    printf("dir[0].permissions : %d \n", temp_dir[0].permissions);
    printf("dir[1].permissions : %d \n\n", temp_dir[1].permissions);


    for (int i = 2; i < 64; i++)
	{
		// empty filename will imply that it is free to write to
		strncpy(temp_dir[i].filename, "", 0);
	}

    LBAwrite(temp_dir, 6, temp_dir[0].starting_block);

	LBAwrite(VCB, 1, 0);

    free(temp_dir);
	temp_dir = NULL;
}

char saved_filename[20];
int ret;
int num_of_paths;

// to be called in parse_path(). This function will update temp current working directory
int validate_path(char * name) {
    // return -1 if it exits, and 0 if it does not.

    // this represents the paths remaining to be searched
    num_of_paths--;
    
    // init temp current working directory if NULL
    if (temp_curr_dir == NULL)
    {
        temp_curr_dir = malloc(VCB->block_size * 6);

        // checking if malloc was successful
	    if (temp_curr_dir == NULL)
	    {
		    printf("ERROR: failed to malloc.\n");
		    exit(-1);
	    }

        // starting temp from the current working directory
        LBAread(temp_curr_dir, 6, curr_dir[0].starting_block);
    }

    /*
    iterate through dir_entries of temp directory to see if path is valid, 
    before updating current working directory with a valid path.
    */

    for (int i = 2; i < 64; i++)
    {
        // this path was found while more paths still need to be searched
        if (strcmp(temp_curr_dir[i].filename, name) == 0 && num_of_paths > 0)
        {
            printf("found path\n");

            // updating temp with found path
            LBAread(temp_curr_dir, 6, temp_curr_dir[i].starting_block);

            printf("temp directory set to: %s\n\n", temp_curr_dir[0].filename);
            return 0;
        }

        // last path was found
        else if (strcmp(temp_curr_dir[i].filename, name) == 0 && num_of_paths == 0)
        {
            printf("last path already exists\n");
            printf("NOT able to create this directory\n");

            // updating temp directory with found path
            LBAread(temp_curr_dir, 6, temp_curr_dir[i].starting_block);
            

            printf("temp directory set to: %s\n\n", temp_curr_dir[0].filename);
            return 0;
        }

        /*
         this path does not exist while more paths still need to be searched.
         In this case we can break out of the while loop by returning -1
        */

        else if (i == 63 && num_of_paths > 0)
        {
            // this else-container implies that path was not found while there are still paths left to search
            printf("path %s does not exist in %s Directory\n", name, temp_curr_dir[0].filename);
            printf("NOT able to create this directory\n");

            return -1;
        }

        // last path does not exist. This implies that we can make a new directory
        else if (i == 63 && num_of_paths == 0)
        {
            printf("last path does not exist\n");
            printf("able to create this directory\n");

            // save name
            memset(saved_filename,0, sizeof(saved_filename));
            strncpy(saved_filename, name, strlen(name));
            // saved_filename = name;
            return -1;
        }
    }
}


int parse_pathname(const char * pathname)
{
    char * count_slashes = strdup(pathname);
    char * buffer_pathname = strdup(pathname);
    num_of_paths = 0;

    printf("buffer_pathname: %s\n", buffer_pathname);

    //check that beginning of pathname is '\'
    if (count_slashes[0] != '\\')
    {
        printf("ERROR: path must begin with '\\'\n\n");
        num_of_paths = -1;
        return -1;
    }
    
    // counting forward slashes to determine num of input
    for (int i = 0; i < strlen(count_slashes); i++)
    {
        if (count_slashes[i] == '\\' && i == strlen(count_slashes) - 1)
        {
            printf("ERROR: empty head path after '\\'\n\n");
            num_of_paths = -1;
            return -1;
        }

        else if (count_slashes[i] == '\\' && count_slashes[i + 1] == '\\')
        {
            printf("ERROR: invalid double '\\'\n\n");
            num_of_paths = -1;
            return -1;
        }

        else if (count_slashes[i] == '\\')
        {
            num_of_paths++;
        }
    }

    memset(count_slashes, 0, sizeof(count_slashes));

    printf("num of paths: %d\n", num_of_paths);

    //just in case there is a newline character, this will remove \n
    char * newLine = strchr(buffer_pathname, '\n');
    if (newLine)
    {
        * newLine = ' ';
    } 

    // call str_tok to divide pathname into tokens
    char * name = strtok(buffer_pathname, "\\");
    
    while (name != NULL)
    {

        printf("Pathname: %s -> ", name);
        ret = validate_path(name);
        if (ret == -1) 
        {
            printf("invalid path\n");
            return ret;
        }

        name = strtok(NULL, "\\");
    }

    return ret;
}

int fs_mkdir(const char * pathname, mode_t mode) 
{
    // will return -1 if path is invalid, and 0 if it is a hit
    int ret = parse_pathname(pathname);

    if (ret == -1 && num_of_paths == 0)
    {

        //searching for a free directory entry to write to in temp current directory
        for (int i = 2; i < 64; i++)
        {
            if (strcmp(temp_curr_dir[i].filename, "") == 0)
            {
                //adding to entry in parent directory
                strncpy(temp_curr_dir[i].filename, saved_filename, strlen(saved_filename));
                temp_curr_dir[i].starting_block = VCB->free_block_start;
                temp_curr_dir[i].permissions = mode;
                temp_curr_dir[i].size = 3072; // size will always start here and then dynamically grow when entries fill up
               
                // creating the child directory
                create_dir(saved_filename, mode);

                // update current directory on disk
                LBAwrite(temp_curr_dir, 6, temp_curr_dir[0].starting_block);
                ret = 0;
                i = 64;
            }
            else if (i == 63)
            {
                printf("Unable to find free directory entry inside /%s\n", temp_curr_dir[0].filename);

                // ------------- later should add a function to grow num of entries if full ----------- //
            }
        }
    }

    /*
    if temp dir was used, then reset temp curr 
    working directory for next use.
    */

    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }

    return ret;
}

int fs_rmdir(const char *pathname) 
{
    // int ret = parse_pathname(pathname);
    int ret = 0;

    //checking that this directory is not root
    if (strcmp(temp_curr_dir[0].filename, ".") == 0)
    {
        ret = -1;
        printf("ERROR: cannot remove root directory.\n");
    }

    //checking if path is valid and if temp curr working dir is directory
    if (ret == 0)
    {
        // check that the directory is empty
        for (int i = 2; i < 64; i++)
        {
            if (strcmp(temp_curr_dir[i].filename, "") != 0)
            {
                ret = -1;
                printf("ERROR: directory is not empty.\n");
                i = 64;
            }
        }

        if (ret == 0)
        {   
            // getting the parent dir from child
            temp_dir = malloc(VCB->block_size*6);
                
            // checking if malloc was successful
	        if (temp_dir == NULL)
	        {
		        printf("ERROR: failed to malloc.\n");
		        exit(-1);
	        }

            LBAread(temp_dir, 6, temp_curr_dir[1].starting_block);

            //looking for this directory in parent directory, then erasing it from parent directory
            for (int i = 2; i < 64; i++)
            {
                if (strcmp(temp_dir[i].filename, temp_curr_dir[0].filename) == 0)
                {
                    printf("found child [%s] in parent [%s] at index %d.\n", 
                    temp_curr_dir[0].filename, temp_dir[i].filename, i);

                    // clearing filename in parent will mark this entry as free to write to
                    memset(temp_dir[i].filename, 0, sizeof(temp_dir[i].filename));

                    // free up space in freespace bitmap
                    reallocate_space(temp_curr_dir);

                    i = 64;
                }

                else if (i == 63)
                {
                    ret = -1;
                    printf("ERROR: cannot find child [%s] in parent [%s].\n", 
                    temp_curr_dir[0].filename, temp_dir[i].filename);
                }
            }
        }

        // update parent directory to disk (i.e. temp_dir).
        if (ret == 0)
        {
            LBAwrite(temp_dir, 6, temp_dir[0].starting_block);
            printf("-- updated disk --\n\n");
        }
        else
        {

            printf("fatal error: -- did not update disk --\n\n");
        }
    }

    free(temp_dir);
    temp_dir = NULL;

    free(temp_curr_dir);
    temp_curr_dir = NULL;

    return ret;
}

//return 1 if head path is directory is true and 0 if false
int fs_isDir(char * path){
    int ret = parse_pathname(path); 

    printf("ret: %d\n", ret);
    printf("is_file: %d\n", temp_curr_dir[0].is_file);

    if(ret == 0){
        
        /*
         temp_curr_dir is updated to head path 
         after successful return on parse_path()
        */

        if (temp_curr_dir[0].is_file == 0){
            ret = 1;
        }
    }
    
    return ret;
}

//return 0 if path is file, -1 otherwise
int fs_isFile(char * path){
    //basically the same code as fs_isDir, just change the if ondition to (directory.is_file == 1) instead
    //the is_file val can be 0(dir), 1(file), -1(invalid)
    return 0;
}

int fs_delete(char* filename){
    char* pathname = strcat("./", filename);
    // check if file exists with parse_pathname(pathname);
    // if DNE, return -1 and throw error.
    // now how do we delete it?
    return 0;
}