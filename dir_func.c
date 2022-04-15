#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <mfs.h>
#include "freeAlloc.c"


dir_entr * temp_dir;      // this is a temp dir for making/removing directories and getting cwd
dir_entr * temp_curr_dir; // this will represent a temp pointer to current working directory.
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

    
    
    strcpy(temp_dir[0].filename, name);

    // these two will lines will always be true so no need to put them in if statments
    temp_dir[0].size = temp_dir[1].size = sizeof(dir_entr) * 64;
	temp_dir[0].is_file = temp_dir[1].is_file = 0;

    // if root
    if (strcmp(temp_dir[0].filename, ".") == 0)
    {
        VCB->root_start = temp_dir[0].starting_block = 
        temp_dir[1].starting_block = allocate_space(6);
        temp_dir[0].permissions = temp_dir[1].permissions = permissions;
        strcpy(temp_dir[1].filename, "..");
        printf("------------CREATED ROOT DIRECTORY------------\n");
    }
    else // if not root
    {
        temp_dir[0].starting_block = allocate_space(6);
        temp_dir[1].starting_block = temp_curr_dir->starting_block;
        temp_dir[0].permissions = permissions;
        temp_dir[1].permissions = temp_curr_dir->permissions;
        strcpy(temp_dir[1].filename, temp_curr_dir->filename);
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

    memset(temp_dir, 0, sizeof(temp_dir));
    free(temp_dir);
	temp_dir = NULL;
}

char saved_filename[20];
int ret;
int num_of_paths;

/* 
to be called in parse_path(). This function will update 
temp current working directory to check if a valide path was given
*/

int validate_path(char * name) 
{
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

        // initializing temp from the current working directory
        LBAread(temp_curr_dir, 6, curr_dir[0].starting_block);
    }

    /*
    iterate through dir_entries of temp directory to see if path is valid, 
    before updating current working directory with a valid path.
    */

    for (int i = 2; i < 64; i++)
    {
        //checking if this is a file
        if (temp_curr_dir[i].is_file)
        {
            if (num_of_paths == 0)
            {
                // save name for file creation
                memset(saved_filename,0, sizeof(saved_filename));
                strncpy(saved_filename, name, strlen(name));
            }
            
            printf("cannot set file as current directory.\n");
            return -1;
        }

        // this path was found while more paths still need to be searched
        else if (strcmp(temp_curr_dir[i].filename, name) == 0 && num_of_paths > 0)
        {
            
            // updating temp with found path
            LBAread(temp_curr_dir, 6, temp_curr_dir[i].starting_block);
            return 0;
        }

        // last path was found
        else if (strcmp(temp_curr_dir[i].filename, name) == 0 && num_of_paths == 0)
        {

            // updating temp with found path
            LBAread(temp_curr_dir, 6, temp_curr_dir[i].starting_block);
            return 0;
        }

        /*
         this path does not exist while more paths still need to be searched.
         In this case we can break out of the while loop by returning -1
        */

        // this else-if container implies that path was not found while there are still paths left to search
        else if (i == 63 && num_of_paths > 0)
        {
            return -1;
        }

        // last path does not exist. This implies that we can make a new directory
        else if (i == 63 && num_of_paths == 0)
        {

            // save name
            memset(saved_filename,0, sizeof(saved_filename));
            strncpy(saved_filename, name, strlen(name));
            return -1;
        }
    }
}

int parse_pathname(const char * pathname)
{
    char * count_slashes = strdup(pathname);
    char * buffer_pathname = strdup(pathname);
    num_of_paths = 0;

    //just in case there is a newline character, this will remove \n
    char * newLine = strchr(buffer_pathname, '\n');
    if (newLine)
    {
        * newLine = '\0';
    } 

    //check that beginning of pathname is '\'
    if (count_slashes[0] != '/')
    {
        printf("ERROR: path must begin with '/'\n\n");
        num_of_paths = -1;
        return -1;
    }
    
    // counting forward slashes to determine num of input
    for (int i = 0; i < strlen(count_slashes); i++)
    {
        if (count_slashes[i] == '/' && i == strlen(count_slashes) - 1)
        {
            printf("ERROR: empty head path after '/'\n\n");
            num_of_paths = -1;
            return -1;
        }

        else if (count_slashes[i] == '/' && count_slashes[i + 1] == '/')
        {
            printf("ERROR: invalid double '/'\n\n");
            num_of_paths = -1;
            return -1;
        }

        else if (count_slashes[i] == '/')
        {
            num_of_paths++;
        }
    }

    // calling str_tok to divide pathname into tokens
    char * name = strtok(buffer_pathname, "/");
    
    while (name != NULL)
    {

        // validating that each path is valid
        ret = validate_path(name);
        if (ret == -1) 
        {
            return ret;
        }

        name = strtok(NULL, "/");
    }

    return ret;
}

int fs_mkdir(const char * pathname, mode_t mode) 
{
    // will return -1 if path is invalid, and 0 if it is valid
    int ret = parse_pathname(pathname);

    /*
     this if statement implies that the last path does not exist, 
     which means we can make this directory
    */

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
    else if (ret == 0 && num_of_paths == 0)
    {
        printf("path already exists.\n");
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
    temp_dir = malloc(VCB->block_size*6);
                
    // checking if malloc was successful
	if (temp_dir == NULL)
	{
		printf("ERROR: failed to malloc.\n");
		exit(-1);
	}

    int ret = parse_pathname(pathname);

    //checking that this directory is not root
    if (strcmp(temp_curr_dir[0].filename, ".") == 0)
    {
        ret = -1;
        printf("ERROR: cannot remove root directory.\n");
    }

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

    // if directory is empty and not root
    if (ret == 0)
    {   
        // getting the parent dir from child
        LBAread(temp_dir, 6, temp_curr_dir[1].starting_block);

        //looking for this directory in parent directory, then erasing it from parent directory
        for (int i = 2; i < 64; i++)
        {
            if (strcmp(temp_dir[i].filename, temp_curr_dir[0].filename) == 0)
            {

                // clearing filename in parent will mark this entry as free to write to
                memset(temp_dir[i].filename, 0, sizeof(temp_dir[i].filename));

                // free up space in freespace bitmap
                reallocate_space(temp_curr_dir);

                i = 64;
            }

            // cannot find this entry in parent
            else if (i == 63)
            {
                ret = -1;
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
    
    // --------------- clean up --------------- //
    memset(temp_dir, 0, sizeof(temp_dir));
    free(temp_dir);
    temp_dir = NULL;

    free(temp_curr_dir);
    temp_curr_dir = NULL;

    return ret;
}

//return 1 if head path is directory is true and 0 if false
int fs_isDir(char * path)
{
    int ret = parse_pathname(path); 

    if(ret == 0)
    {
        if (temp_curr_dir[0].is_file == 0)
        {
            ret = 1;
        }
    }
    else
    {
        ret = 0; // convert ret from -1 to 0 to return 'false'
    }

    free(temp_curr_dir);
    temp_curr_dir = NULL;
    
    return ret;
}

//return 1 if path is file, 0 otherwise
int fs_isFile(char * path){
    
    // int ret = parse_pathname(path);

    // if(ret == 0)
    // {
    //     for (int i = 2; i < 64; i++)
    //     {
    //         if(temp_curr_dir[i].filename == saved_filename && temp_curr_dir[i].is_file == 1)
    //         {
    //             ret = 1;
    //             i = 64;
    //         }
    //     }
    // }
    

    // free(temp_curr_dir);
    // temp_curr_dir = NULL;
    // return ret;
    return 0;
}

int fs_delete(char* filename)
{
    char * path = strcat("./", filename);
    int ret = parse_pathname(path);
    
    if (ret == -1 || temp_curr_dir->is_file != 1)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
        return ret;
    }
    else if (temp_curr_dir->is_file != 1){
        free(temp_curr_dir);
        temp_curr_dir = NULL;
        printf("%s is not a file", temp_curr_dir->filename);
        return -1;
    }
    
    free(curr_dir);
    curr_dir = NULL;
    return ret;
}

char * fs_getcwd(char * buf, size_t size) 
{

    // travel backwords from curr_dir and populate an absolute path to display
    temp_dir = malloc(VCB->block_size * 6);
    LBAread(temp_dir, 6, curr_dir[0].starting_block);

    char * tail_path = malloc(VCB->block_size * 6);
    char * head_path = malloc(VCB->block_size * 6);
    char * slash = malloc(VCB->block_size * 6);

    strcpy(slash, "/");
   
    strcpy(tail_path, temp_dir[0].filename);

    // -------- concatinating paths into one complete pathname ------ //
    while (strcmp(tail_path, ".") != 0)
    {
        strcat(tail_path, head_path);
        strcpy(head_path, strcat(slash, tail_path));
        LBAread(temp_dir, 6, temp_dir[1].starting_block);

        strcpy(tail_path, temp_dir[0].filename);
        memset(slash, 0, sizeof(slash));
        strcpy(slash, "/");
    }
    
    strcat(tail_path, head_path);
    strcpy(buf, strcat(slash, tail_path));

    // --------- clean and free up temp buffers -------- //
    memset(tail_path, 0, sizeof(tail_path));
    memset(head_path, 0, sizeof(head_path));
    memset(slash, 0, sizeof(slash));
    memset(temp_dir, 0, sizeof(temp_dir));

    free(temp_dir);
    temp_dir = NULL;

    free(tail_path);
    tail_path = NULL;

    free(head_path);
    head_path = NULL;

    free(slash);
    slash = NULL;

    return buf;
}


int fs_setcwd(char * buf) 
{
    if (strcmp(buf, "..") == 0)
    {
        LBAread(curr_dir, 6, curr_dir[1].starting_block);
        return 0;
    }
    
    int ret = parse_pathname(buf);

    if (ret == 0) 
    {
        LBAread(curr_dir, 6, temp_curr_dir[0].starting_block);
    }

    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }

    return ret;
}

//used in ls
fdDir * dirp;
char * name_check;
fdDir * fs_opendir(const char * name)
{
    
    int ret = 0;

    name_check = malloc (4097);

    if (name_check == NULL)
    {
        printf("ERROR: malloc failed.\n");
        exit(-1);
    }
    
    if (strcmp(name, fs_getcwd(name_check, 4097)) != 0)
    {
        ret = parse_pathname(name);
    }
    
    if(ret == -1)
    {
        return dirp;
    }
    
    if (temp_curr_dir == NULL)
    {
        temp_curr_dir = malloc(VCB->block_size*6);
        LBAread(temp_curr_dir, 6, curr_dir[0].starting_block);
    }

    //make a pointer for the directory steam
    dirp = malloc(sizeof(fdDir));

    //always begin at file pos 0 in directory stream
    dirp->dirEntryPosition = 0;

    //Location of the directory in relation to the LBA
    dirp->directoryStartLocation = temp_curr_dir[0].starting_block;

    return dirp;
}

//used in displayFiles()
struct fs_diriteminfo * dirItem;
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{

    dirItem = malloc(sizeof(struct fs_diriteminfo));

    /*
     temp_curr_dir will always be updated to head of entire path, 
     so we can just check inside temp_curr_dir if it is a file or not
    */

    if (temp_curr_dir[0].is_file == 0)
    {
        dirItem->fileType = FT_DIRECTORY;
    }

    if (temp_curr_dir[0].is_file == 1)
    {
        dirItem->fileType = FT_REGFILE;
    }

    if (dirp->dirEntryPosition >= 63)
    {
        return NULL;
    }
    else if(strcmp(temp_curr_dir[dirp->dirEntryPosition].filename, "")!=0)
    {
       strcpy(dirItem->d_name, temp_curr_dir[dirp->dirEntryPosition].filename);

       dirp->dirEntryPosition += 1;
    }
    else if(strcmp(temp_curr_dir[dirp->dirEntryPosition].filename, "")==0)
    {
        while(strcmp(temp_curr_dir[dirp->dirEntryPosition].filename, "")==0 && dirp->dirEntryPosition <63)
        {

            dirp->dirEntryPosition += 1;
            if(strcmp(temp_curr_dir[dirp->dirEntryPosition].filename, "")!=0)
            {
                strcpy(dirItem->d_name, temp_curr_dir[dirp->dirEntryPosition].filename);
            }
            else
            {
                return NULL;
            }
        }
        
        dirp->dirEntryPosition += 1;
    }
    
    return dirItem;
}

//used in displayFiles()
int fs_closedir(fdDir *dirp)
{
    // clean and free up the memory you allocated for opendir
    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }
        
    memset(name_check, 0, sizeof(name_check));
    free(name_check);
    name_check = NULL;
    
    free(dirp);
    dirp = NULL;

    free(dirItem);
    dirItem = NULL;
    
    return 0;
}

//everything except block size seems to be optional
int fs_stat(const char *path, struct fs_stat *buf)
{
    // off_t     st_size;    		/* total size, in bytes */
    buf->st_size = sizeof(dir_entr) * 64;

	// blksize_t st_blksize; 		/* blocksize for file system I/O */
    buf->st_blksize = 512;

	// blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
    buf->st_blocks = 6;
    
	// time_t    st_accesstime;   	/* time of last access */

	// time_t    st_modtime;   	/* time of last modification */

	// time_t    st_createtime;   	/* time of last status change */

    return 0;
}