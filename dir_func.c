#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <mfs.h>
#include "freeAlloc.c"

/* This is a temp dir for making/removing/moving directories 
and getting cwd. */

dir_entr * temp_dir;      

/* This will represent a temp pointer to current working directory 
after parsing the pathname. */

dir_entr * temp_curr_dir; 

/* This will be kept in memory as a pointer to a current working 
directory. */

dir_entr * curr_dir;      

/* Saved data will be used to transfer information during a move
process. */

dir_entr saved_data;

/* This will store an number representing the index of a child.
this is done to have easy access to a parent's child after 
parsing a pathname. */

int temp_child_index;

// these are return values for after parsing a pathname.
enum {VALID, INVALID, SELF, FOUND_FILE};

// this works with fs_makedir to create a directory.
int create_dir(char * name, int mode) 
{
    if (temp_dir == NULL)
    {
        temp_dir = calloc(6, VCB->block_size);

        // checking if malloc was successful. failure doesn't require an exit
	    if (temp_dir == NULL)
	    {
		    printf("ERROR: failed to malloc for creating a directory.\n");
		    return -1;
	    }
    }

    strcpy(temp_dir[0].filename, name);

    // size and is file will always be constant for creating a directory
    temp_dir[0].size = temp_dir[1].size = sizeof(dir_entr) * 64;
	temp_dir[0].is_file = temp_dir[1].is_file = 0;

    // if root, then we will create a root directory here.
    if (strcmp(temp_dir[0].filename, ".") == 0)
    {
        VCB->root_start = temp_dir[0].starting_block = 
        temp_dir[1].starting_block = allocate_space(6);
        temp_dir[0].mode = temp_dir[1].mode = mode;
        strcpy(temp_dir[1].filename, "..");
        printf("------------CREATED ROOT DIRECTORY------------\n");
    }
    else // if not root, then we will make a directory here.
    {
        temp_dir[0].starting_block = allocate_space(6);
        if (temp_dir[0].starting_block == -1)
        {
            free(temp_dir);
	        temp_dir = NULL;
            return -1;
        }
        
        temp_dir[1].starting_block = temp_curr_dir->starting_block;
        temp_dir[0].mode = mode;
        temp_dir[1].mode = temp_curr_dir->mode;
        strcpy(temp_dir[1].filename, temp_curr_dir->filename);
        printf("------------CREATED NEW DIRECTORY------------\n");
    }

    // TEMP INFORMATION FOR NOW
    printf("Size of dir: %d \n", temp_dir[0].size);
	printf("dir[0].starting_block : %d \n", temp_dir[0].starting_block);
    printf("dir[1].starting_block : %d \n", temp_dir[1].starting_block);
	printf("dir[0].filename : %s \n", temp_dir[0].filename);
	printf("dir[1].filename : %s \n", temp_dir[1].filename);
    printf("dir[0].mode : %d \n", temp_dir[0].mode);
    printf("dir[1].mode : %d \n\n", temp_dir[1].mode);


    for (int i = 2; i < 64; i++)
	{
		// empty filename will imply that it is free to write to this entry position
		strncpy(temp_dir[i].filename, "", 0);
	}

    // updates created directory and updated VCB's freespace starting position to disk.
    LBAwrite(temp_dir, 6, temp_dir[0].starting_block);
	LBAwrite(VCB, 1, 0);

    free(temp_dir);
	temp_dir = NULL;

    return 0;
}

// these attributes are for the path parsing implimentation.
char saved_filename[20]; // saves a name for creating a directory.
int ret;                 // stores the return value from various functions.
int paths_remaining;     // stores the number of paths that remain to be validated.
int total_paths;         // represents the total number of paths given by the user.

/* This function is called in parse_path(). This function will store either a root directory or cwd into
temp_curr_dir, and compare the given path to each of its children to find a matching file or directory.
if it is a directory, then temp_curr_dir will be updated to store the new directory and continue searching 
its children for the next path until thier are no more paths remaining. */

int validate_path(char * name) 
{
    paths_remaining--;
    
    if (temp_curr_dir == NULL)
    {
        temp_curr_dir = calloc(6, VCB->block_size);

        // checking if malloc was successful
	    if (temp_curr_dir == NULL)
	    {
		    printf("failed to malloc for validating path.\n");
		    return -1;
	    }

        // this indicates user is passing in an absolute path, so we will start parsing from the root.
        if (strcmp(name, ".") == 0)
        {
            LBAread(temp_curr_dir, 6, VCB->root_start);
            return VALID;
        }

        // this indicates user is passing a relative path, so we will start parsing from the cwd.
        else
        {
            LBAread(temp_curr_dir, 6, curr_dir[0].starting_block);
        }
    }

    // if user is trying to refer to cwd for a moving process, then we will check that here
    if (total_paths == 1 && strcmp(temp_curr_dir[0].filename, name) == 0)
    {
        temp_child_index = 0;
        
        /* in case the user is trying to make a child of the same name as the parent, 
        we will save the given name */

        memset(saved_filename,0, sizeof(saved_filename));
        strcpy(saved_filename, name);

        /* return SELF indicates that we can either make this as directory 
        or refer to curr dir for moving files. */

        return SELF; 
    }
    
    /* This for loop iterates through directory entries of temp_curr_dir to see if a path is valid, 
    before updating temp_curr_dir with the valid path. */

    for (int i = 2; i < 64; i++)
    {   
        // this is checking if this path is a file.
        if (strcmp(temp_curr_dir[i].filename, name) == 0 && temp_curr_dir[i].is_file == 1)
        {
            // if this was the head path.
            if (paths_remaining == 0)
            {
                // temp_child_index stores a reference to this file.
                temp_child_index = i;
                return FOUND_FILE;
            }
            else
            {
                printf("ERROR: cannot set file as current directory.\n");
                return INVALID;
            }
        }

        // this path was found while more paths still need to be searched
        else if (strcmp(temp_curr_dir[i].filename, name) == 0 && paths_remaining > 0)
        {
            // updating temp_curr_dir with found path
            temp_child_index = i;
            LBAread(temp_curr_dir, 6, temp_curr_dir[i].starting_block);
            return VALID;
        }

        // last path was found
        else if (strcmp(temp_curr_dir[i].filename, name) == 0 && paths_remaining == 0)
        {
            // updating temp_curr_dir with last path
            temp_child_index = i;
            LBAread(temp_curr_dir, 6, temp_curr_dir[i].starting_block);
            return VALID;
        
        }

        /* This path does not exist while more paths still need to be searched.
         In this case we can break out of the while loop by returning INVALID. */

        else if (i == 63 && paths_remaining > 0)
        {
            return INVALID;
        }

        // last path does not exist. This implies that we can make a new directory
        else if (i == 63 && paths_remaining == 0)
        {
            // saves a name for creating a directory.
            memset(saved_filename,0, sizeof(saved_filename));
            strncpy(saved_filename, name, strlen(name));
            return INVALID;
        }
    }
}
 
/* this function works in conjunction with validate_path(), and uses str_tok 
with a '/' delim to tokenize the pathname into individual paths to validate. */

int parse_pathname(const char * pathname)
{
    // this will be used to count the number of slashes.
    char * count_slashes = strdup(pathname);

    // this will be tokenized by str_tok().
    char * buffer_pathname = strdup(pathname);
    paths_remaining = 0;
    total_paths = 0;

    // just in case there is a newline character, this will remove '\n'.
    char * newLine = strchr(buffer_pathname, '\n');
    if (newLine)
    {
        * newLine = '\0';
    } 

    // checks that beginning of pathname is '/'
    if (count_slashes[0] != '/')
    {
        printf("ERROR: path must begin with '/'\n");

        // setting paths to -1 will prevent md from creating a directory.
        paths_remaining = -1;
        return INVALID;
    }
    
    // counting forward slashes to determine number of paths to search.
    for (int i = 0; i < strlen(count_slashes); i++)
    {
        if (count_slashes[i] == '/' && i == strlen(count_slashes) - 1)
        {
            printf("ERROR: empty head path after '/'\n");
            paths_remaining = -1;
            return INVALID;
        }

        else if (count_slashes[i] == '/' && count_slashes[i + 1] == '/')
        {
            printf("ERROR: invalid double '/'\n");
            paths_remaining = -1;
            return INVALID;
        }

        else if (count_slashes[i] == '/')
        {
            paths_remaining++;
        }
    }

    total_paths = paths_remaining;

    // calling str_tok to divide pathname into tokens
    char * name = strtok(buffer_pathname, "/");
    
    while (name != NULL)
    {
        /* validating that each tokenized path is valid by passing in 
        each path as 'name' to validate_path() */

        ret = validate_path(name);

        // if path is invalid, then there is no need to continue, so we will return.
        if (ret == INVALID) 
        {
            return ret;
        }

        name = strtok(NULL, "/");
    }

    return ret;
}

/* this function begins the process of creating a directory. Calling  parse_pathname will 
update temp_curr_dir to the rightmost valid path, which gives us a parent to insert a new
directory into, if only the head path is invalid. */

int fs_mkdir(const char * pathname, mode_t mode) 
{
    // will return -1 if path is invalid, and 0 if it is valid
    int ret = parse_pathname(pathname);

    // this ensures that user is not naming a directory as root.
    if (strcmp(saved_filename, ".") == 0 || strcmp(saved_filename, "..") == 0)
    {
       printf("cannot name directory %s\n", saved_filename);
    }

    /* This if statement implies that the last path does not exist, 
     which means we can make this directory */

    else if (ret == INVALID && paths_remaining == 0 || ret == SELF && paths_remaining == 0)
    {

        // searching for a free directory entry to write to in temp current directory
        for (int i = 2; i < 64; i++)
        {
            if (strcmp(temp_curr_dir[i].filename, "") == 0)
            {
                //adding to entry in parent directory
                strncpy(temp_curr_dir[i].filename, saved_filename, strlen(saved_filename));
                temp_curr_dir[i].starting_block = VCB->free_block_start;
                temp_curr_dir[i].mode = mode;
                temp_curr_dir[i].size = 3072;
                temp_curr_dir[i].is_file = 0;

                // creating the child directory
                ret = create_dir(saved_filename, mode);
                if (ret == -1)
                {
                    /* this means that create_dir failed to make a directory, which would 
                    happen if we couldn't allocate space. */

                    free(temp_curr_dir);
                    temp_curr_dir = NULL;
                    return -1;
                }
                
                // updating parent with new metadata of created children on disk
                LBAwrite(temp_curr_dir, 6, temp_curr_dir[0].starting_block);
                ret = 0; // returns 0 to shell
                i = 64;
            }
            else if (i == 63)
            {
                printf("Unable to find free directory entry inside /%s\n", temp_curr_dir[0].filename);
                ret = -1;
            }
        }
    }
    else if (ret == VALID && paths_remaining == 0)
    {
        printf("path already exists.\n");
        ret = -1;
    }

    /* If temp_curr_dir was used, then we will reset 
    temp_curr_dir for next use. */

    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }

    return ret;
}

/* This function removes a directory by updating temp_curr_dir to the head path after
calling parse_pathname() and checking that it is empty. If it is empty then it will be 
cleared from both it's parent and from the freespace bitmap. */

int fs_rmdir(const char * pathname) 
{
    // temp_dir will bring the parent of the head path into memory.
    temp_dir = calloc(6, VCB->block_size);
                
	if (temp_dir == NULL)
	{
		printf("ERROR: failed to malloc.\n");
		return -1;
	}

    int ret = parse_pathname(pathname);

    // malloc failure in parse_pathname() occured so we will exit function.
    if (ret == -1)
    {
        return -1;
    }

    // checking that this directory is not root, since we cannot remove root.
    if (strcmp(temp_curr_dir[0].filename, ".") == 0)
    {
        printf("ERROR: cannot remove root directory.\n");
        ret = INVALID; 
    }

    // the directory must be empty to use the rm cmd, so we will check that here.
    for (int i = 2; i < 64; i++)
    {
        if (strcmp(temp_curr_dir[i].filename, "") != 0)
        {
            printf("ERROR: directory is not empty.\n");
            ret = INVALID;
            i = 64;
        }
    }

    // if directory is empty and not root, then everything is VALID so far.
    if (ret == VALID)
    {
        // getting the parent directory from child and storing it into temp_dir.
        LBAread(temp_dir, 6, temp_curr_dir[1].starting_block);

        /* This is searching for the 'removed' directory (temp_curr_dir) in the 
        parent directory (temp_dir), then freeing child from parent directory by 
        clearing the filename */

        for (int i = 2; i < 64; i++)
        {
            if (strcmp(temp_dir[i].filename, temp_curr_dir[0].filename) == 0)
            {
                // this is freeing space in freespace bitmap that was previously occupied by the removed directory.
                reallocate_space(temp_curr_dir, 0, 0);

                // clearing filename in parent will mark this entry as free to write to in the parent.
                memset(temp_dir[i].filename, 0, sizeof(temp_dir[i].filename));
                move_child_left(temp_dir, i);

                i = 64;
            }
        }
    }
        // this updates parent directory to disk.
        if (ret == VALID)
        {
            LBAwrite(temp_dir, 6, temp_dir[0].starting_block);
            printf("-- updated disk --\n\n");
        }
        else
        {
            printf("-- did not update disk --\n\n");
        }

    free(temp_dir);
    temp_dir = NULL;

    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }

    return ret;
}

/* This will check if the head path is a directory by calling parse_pathname() and checking that the
updated temp_curr_dir[0].is_file is equal to 0. if true, we will return 1, else return 0. */

int fs_isDir(char * path)
{
    int ret = 0;
    ret = parse_pathname(path); 

    // VALID implies that temp_curr_dir is updated to the head of the path.
    if(ret == VALID)
    {
        if (temp_curr_dir[0].is_file == 0)
        {
            ret = 1; // returns true to shell
        }
    }
    else
    {
        ret = 0; //returns false to the shell
    }

    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }
    
    return ret;
}

/* This function does the same thing as fs_isDir(), except parse_pathname() will return FOUND_FILE if 
head path is a file. */

int fs_isFile(char * path)
{
    int ret = 0;
    ret = parse_pathname(path);

    if (ret == FOUND_FILE)
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }

    return ret;
}

// this function deletes a file.
int fs_delete(char * pathname)
{
    int ret = 0;
    ret = parse_pathname(pathname);

    // failed to malloc in parse_pathname() so we will return -1 to shell
    if (ret == -1)
    {
        return -1;
    }
    
    // temp_child_index will have the file position in the parent after calling parse_pathname().
    int index = temp_child_index;

    // frees the bits previosuly allocated to the file in the freespace bitmap.
    reallocate_space(temp_curr_dir, index, 0);

    // frees the file from the parents child entry list.
    memset(temp_curr_dir[index].filename, 0, sizeof(temp_curr_dir[index].filename));
    
    // fills the freed position in parent with the rightmost child.
    move_child_left(temp_curr_dir, index);

    // updates parent of deleted file
    LBAwrite(temp_curr_dir, 6, temp_curr_dir[0].starting_block);

    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }

    return 0;
}

/* This prints the entire current working path by iterating backwards from the 
current directory (curr_dir) to get the filenames, until we reach the root. */

char * fs_getcwd(char * buf, size_t size) 
{
    /* temp_dir starts from curr_dir and travels backwards to all parents
    and populate a pathname to display. */

    temp_dir = calloc(6, VCB->block_size);
    LBAread(temp_dir, 6, curr_dir[0].starting_block);

    int count_paths = 0;                           // counts the num of paths.
    char * tail_path = calloc(6, VCB->block_size); // stores the tail end of path.
    char * head_path = calloc(6, VCB->block_size); // stores the head of a path.
    char * slash = calloc(6, VCB->block_size);     // stores a '/' character.

    strcpy(slash, "/");
   
    // copy's current directory's filename to  tail_path.
    strncpy(tail_path, temp_dir[0].filename, strlen(temp_dir[0].filename));

    // This while loop concatinating paths into one complete pathname
    while (strcmp(tail_path, ".") != 0)
    {
        count_paths++;

        strcat(tail_path, head_path);                     // head path is added to tail-end of tail path
        strcpy(head_path, strcat(slash, tail_path));      // head path becomes updated tail path
        LBAread(temp_dir, 6, temp_dir[1].starting_block); // new parent is retieved

        strcpy(tail_path, temp_dir[0].filename);          // tail path becomes parent filename
        memset(slash, 0, sizeof(slash));                  // slash is reset to '/'
        strcpy(slash, "/");
    }
    
    if (count_paths > 0)
    {
        // at this point tail_path should be root '/.' and head_path is all the concatinated paths after root.
        strcat(tail_path, head_path);
    }
    
    /* tail_path will contain the full path at this point and be stored in the buf 
    to be returned to the shell for displaying to the user. */

    strcpy(buf, strcat(slash, tail_path));

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

/* This function sets current working directory (curr_dir) to temp_curr_dir after a VALID return from 
parse_pathname. a VALID return will update temp_curr_dir to the directory at the head path. */

int fs_setcwd(char * buf) 
{
    // this if statement will set cwd to the parent if cwd.
    if (strcmp(buf, "..") == 0)
    {
        LBAread(curr_dir, 6, curr_dir[1].starting_block);
        return 0;
    }
    
    int ret = parse_pathname(buf);

    if (ret == VALID) 
    {
        LBAread(curr_dir, 6, temp_curr_dir[0].starting_block);
        ret = 0;
    }

    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }

    return ret;
}


fdDir * dirp;

/* This function stores the head path directory into temp_curr_dir after a 
 VALID parse for displaying the children of the directory. */

fdDir * fs_opendir(const char * name)
{
    int ret = 0;
    
    if (name != NULL)
    {
        ret = parse_pathname(name);
    }
    
    if(ret == INVALID)
    {
        return dirp;
    }
    
    /* if this is null, then it is implied that no path was given by user so, 
    we will display the children of the cwd. */

    if (temp_curr_dir == NULL)
    {
        temp_curr_dir = calloc(6, VCB->block_size);
        LBAread(temp_curr_dir, 6, curr_dir[0].starting_block);
    }

    // make a pointer for the directory steam
    dirp = malloc(sizeof(fdDir));

    // always begin at file pos 0 in directory stream
    dirp->dirEntryPosition = 0;

    // Location of the directory in relation to the LBA
    dirp->directoryStartLocation = temp_curr_dir[0].starting_block;

    return dirp;
}

/* this function will update dirItem with metadata stored in temp_curr_dir when
 displaying its children for the 'ls' cmd. */

struct fs_diriteminfo * dirItem;
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{

    dirItem = malloc(sizeof(struct fs_diriteminfo));

    /*
     temp_curr_dir will always be updated to head of entire path, 
     so we can just check inside temp_curr_dir if it is a file or not
    */

    if (temp_curr_dir[dirp->dirEntryPosition].is_file == 0)
    {
        dirItem->fileType = 0;
    }
    if (temp_curr_dir[dirp->dirEntryPosition].is_file == 1)
    {
        dirItem->fileType = 1;
    }

    dirItem->size = temp_curr_dir[dirp->dirEntryPosition].size;
    if (dirp->dirEntryPosition >= 63)
    {
        return NULL;
    }
    else if(strcmp(temp_curr_dir[dirp->dirEntryPosition].filename, "") !=0 )
    {
       strcpy(dirItem->d_name, temp_curr_dir[dirp->dirEntryPosition].filename);
       dirp->dirEntryPosition += 1;
    }
    else if(strcmp(temp_curr_dir[dirp->dirEntryPosition].filename, "") == 0)
    {
        while(strcmp(temp_curr_dir[dirp->dirEntryPosition].filename, "") == 0 && dirp->dirEntryPosition < 63)
        {
            dirp->dirEntryPosition += 1;
            if(strcmp(temp_curr_dir[dirp->dirEntryPosition].filename, "") != 0)
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

// this function cleans all buffers used for the 'ls' cmd.
int fs_closedir(fdDir *dirp)
{
    printf("\n");
    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }
    
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

// this will be used for the 'mv' cmd in move().
dir_entr * child_dir; 

/* This helper function uses recursion to free/delete an entire branch of directories/files 
from freespace bitmap if the user chooses to overwrite a directory */

void delete_this_branch(dir_entr * directory, int index)
{
    // directory[index] refers to the child being deleted.
    int block_count = convert_size_to_blocks(directory[index].size, VCB->block_size);
    dir_entr * branch = malloc(block_count * VCB->block_size);

    // here we are getting this child into memory
    LBAread(branch, block_count, directory[index].starting_block);

    /* If we are just deleting a file, then we don't need a recursive call 
     since there is no branch for a file */

    if (directory[index].is_file)
    {
        reallocate_space(directory, index, 0);
        memset(directory[index].filename, 0, sizeof(directory[index].filename));
        move_child_left(directory, index);

        if (branch != NULL)
        {
            free(branch);
            branch = NULL;
        }
        return;
    }
    
    /* this for loop checks for dir/files. it will directly remove files from the freespace bitmap, 
    and use a recursive call to traverse through the children of a directory for removal of directories
    from the freespace bitmap. */

    for (int i = 2; i < 64; i++)
    {
        if (strcmp(branch[i].filename, "") != 0 && directory[i].is_file == 0)
        {
            int child_block_count = convert_size_to_blocks(branch[i].size, VCB->block_size);
            dir_entr * child_branch = malloc(child_block_count * VCB->block_size);

            LBAread(child_branch, child_block_count, branch[i].starting_block);
            delete_this_branch(child_branch, child_block_count);

            if (child_branch != NULL)
            {
                free(child_branch);
                child_branch = NULL;
            }
        }

        // if it is a file, then we could just delete it
        else if (strcmp(directory[i].filename, "") != 0 && directory[i].is_file == 1)
        {
            reallocate_space(branch, i, 1);
        }

        /* Directory should be empty at this point, so we will delete self, 
         update bitmap and return 0. */

        else if (i == 63)
        {
            // free/delete child name from parent
            memset(directory[index].filename, 0, sizeof(directory[index].filename));
            move_child_left(directory, index);

            /* We passed a save state arg '1' to prevent the bitmap from being written 
            to disk until this entire process was successfully complete, so we must 
            update bitmap here. */
            
            reallocate_space(branch, 0, 1);

            LBAwrite(buffer_bitmap, 5, 1);

		    update_free_block_start();


            if (branch != NULL)
            {
                free(branch);
                branch = NULL;
            }
        }
    }
}

/* This function provides an option for a user to overwrite a file or directory when 
moving files/directories into a folder containing a child of the same filetype and name 
of the moving file/directory. parameter 'index' represents the starting block of the 
child we want to replace. */
 
int overwrite_dir(int index)
{
    int ret;
    char input[2];
    printf("directory already exists.\n");
    printf("replace existing directory? y or n\n");
    
    while (strcmp(input, "y") != 0 && strcmp(input, "n") != 0)
	{
		scanf("%2s", input);
	}

    if (strcmp(input, "n") == 0 )
    {
        return -1;
    }

    else
    {
        // this begins the overwrite process by calling this function.
        delete_this_branch(temp_curr_dir, index);
        return 0;
    }
}

/* This function is called when a user passes the 'mv' cmd. Parameter 'src' will be passed 
into parse_pathname() until temp_curr_dir is updated to store the file/directory the user 
wants to move. the moving child's parent will be stored into a 'temp_dir' buffer. The moving
file/directory will then be stored into a 'child_dir' buffer. Afterwards, the parameter 'dest'
will be passed into parse_pathname() for updating temp_curr_dir to represent the destination 
for the moving 'child_dir'. The moving 'child_dir' buffer will be stored into the updated 
temp_curr_dir, and both the 'src' and 'dest' parents metadata will be updated to reflect that
the child was moved. */

int move(char * src, char * dest)
{
    if (strcmp(src, dest) == 0)
    {
        printf("ERROR: cannot move file/directory into itself.\n");
        return -1;
    }

    if (strcmp(src, "/.") == 0)
    {
        printf("ERROR: cannot move root.\n");
        return -1;
    }

    int ret = parse_pathname(src);

    // this will be used for changing filename in same directory
    int saved_index = temp_child_index; 

    /* if src path is invalid or refering to current working directory, then we will exit 
    this function since we cannot move an invalid or cwd. */

    if (ret == INVALID || ret == SELF)
    {
        printf("ERROR: src not a valid path.\n");
        if (temp_curr_dir != NULL)
        {
            free(temp_curr_dir);
            temp_curr_dir = NULL;
        }
        return -1;
    }

    // this will store the parent of the moving file/directory.
    temp_dir = calloc(6, VCB->block_size);

    if (temp_dir == NULL)
    {
        printf("failed to malloc.\n");
        ret = -1;
    }

    else
    {
        /* if we are moving a file, then parse_pathname will update temp_curr_dir to store the parent of the file, 
        rather than the head path. Because if this must seperate how we read our blocks into memory between files 
        and directories. */

        if (ret == FOUND_FILE)
        {
            int child_block_count = convert_size_to_blocks(temp_curr_dir[temp_child_index].size, VCB->block_size);
            child_dir = calloc(child_block_count, VCB->block_size);

            //reads parent into memory
            LBAread(temp_dir, 6, temp_curr_dir[0].starting_block);

            //reads child into memory
            LBAread(child_dir, (int) child_block_count, temp_curr_dir[temp_child_index].starting_block);
        }
        else
        {
            int child_block_count = convert_size_to_blocks(temp_curr_dir[0].size, VCB->block_size);
            child_dir = calloc(child_block_count, VCB->block_size);

            //reads parent into memory
            LBAread(temp_dir, 6, temp_curr_dir[1].starting_block);

            //reads child into memory
            LBAread(child_dir, (int) child_block_count, temp_curr_dir[0].starting_block);
        }

        // this is saving the moving child's information to move to the 'dest' directory
        memset(saved_data.filename, 0, sizeof(saved_data.filename));
        strcpy(saved_data.filename, temp_dir[temp_child_index].filename);
        saved_data.starting_block = temp_dir[temp_child_index].starting_block;
        saved_data.size = temp_dir[temp_child_index].size;
        saved_data.is_file = temp_dir[temp_child_index].is_file;
        saved_data.mode = temp_dir[temp_child_index].mode;
        saved_data.user_ID = temp_dir[temp_child_index].user_ID;
	    saved_data.group_ID = temp_dir[temp_child_index].group_ID;

        // clear child's name in src parent to mark it free
        memset(temp_dir[temp_child_index].filename, 0, sizeof(temp_dir[temp_child_index].filename));
        move_child_left(temp_dir, temp_child_index);
    }

    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }

    // this gets the 'dest' path into memory
    ret = parse_pathname(dest);
    if (ret == -1)
    {
        printf("failed to malloc in move.\n");
        ret = -1;
    }

    /* this if statement implies that the final path is invalid after calling parse_pathname().
    In this case we will rename the moving the file to the filename of the last path given by the user. */

    if (ret == INVALID && paths_remaining == 0)
    {
        // change name of moving child in parent dir
        memset(saved_data.filename, 0, sizeof(saved_data.filename));
        strcpy(saved_data.filename, saved_filename);

        // change name in child
        memset(child_dir[0].filename, 0, sizeof(child_dir[0].filename));
        strcpy(child_dir[0].filename, saved_filename);

        // if src and dest folders are the same
        if (temp_dir[0].starting_block == temp_curr_dir[0].starting_block)
        {
            // clear old name when changing name in same directory . temp_child_index doesn't work here
            memset(temp_curr_dir[saved_index].filename, 0, sizeof(temp_curr_dir[saved_index].filename));
            move_child_left(temp_curr_dir, saved_index);
        }
        ret = VALID;
    }
    
    if (ret == INVALID)
    {
        printf("ERROR: dest not a valid path.\n");
    }

    /* this else statement implies that parse_pathname() successfully found a 'dest' folder and updated 
    temp_curr_dir to store the 'dest' folder. */

    else
    {
        /* This for loop checks if a file/directory with the same filename of the moving child already 
        exists in the dest directory. */

        for (int i = 2; i < 64; i++)
        {
            if (strcmp(saved_data.filename, temp_curr_dir[i].filename) == 0)
            {
                /* In the case that a matching filename and filetype exists between the moving child and 
                a child in the 'dest' directory, we will call overwrite_dir() to provide the user an option 
                to overwrite the existing 'dest' child. */

                if (saved_data.is_file == temp_curr_dir[i].is_file)
                {
                    ret = overwrite_dir(i);
                }
                else
                {
                    printf("ERROR: file or directory with the same name already exists in '%s'.\n", temp_dir[0].filename);
                    printf("ERROR: cannot overwrite a different filetype.\n");
                    ret = -1;
                }
            }
        }
        
        if (ret == VALID || ret == SELF)
        {
            // this for loop will try to find a free slot in the 'dest' parent.
            for (int i = 2; i < 64; i++)
            {

                /* free slot was found, so we will copy the saved data from the moving child over to the new 
                'dest' parent. */

                if (strcmp(temp_curr_dir[i].filename, "") == 0)
                {
                    strcpy(temp_curr_dir[i].filename, saved_data.filename);
                    temp_curr_dir[i].starting_block = saved_data.starting_block;
                    temp_curr_dir[i].size = saved_data.size;
                    temp_curr_dir[i].is_file = saved_data.is_file;
                    temp_curr_dir[i].mode = saved_data.mode;
                    temp_curr_dir[i].user_ID = saved_data.user_ID;
                    temp_curr_dir[i].group_ID = saved_data.group_ID;

                    i = 64;
                }
                else if (i == 63)
                {
                    printf("ERROR: no more space left in this directory.\n");
                    ret = -1;
                }
            }
        }
    }

    if (ret == VALID || ret == SELF)
    {
        /* this updates parent info in the moving child if we are moving a directory. This will not be done for moving files 
        because the 'child_dir' buffer will store the actual bytes of the file for moving files, which cannot contain its 
        parent info. */

        if (saved_data.is_file == 0)
        {
            memset(child_dir[1].filename, 0, sizeof(child_dir[1].filename));
            strcpy(child_dir[1].filename, temp_curr_dir[0].filename);
            child_dir[1].size = temp_curr_dir[0].size;
            child_dir[1].starting_block = temp_curr_dir[0].starting_block;

        }

        LBAwrite(child_dir, 6, child_dir[0].starting_block);

        // this if statment prevents 'src' parent from updating disk if 'src' parent is the same as 'dest' parent.
        if (temp_dir[0].starting_block != temp_curr_dir[0].starting_block)
        {
            // updates src parent directory
            LBAwrite(temp_dir, 6, temp_dir[0].starting_block);
        }
        
        // updates dest parent directory
        LBAwrite(temp_curr_dir, 6, temp_curr_dir[0].starting_block);
    }

    if (temp_dir != NULL)
    {
        free(temp_dir);
        temp_dir = NULL;
    }

    if (temp_curr_dir != NULL)
    {
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }

    if (child_dir != NULL)
    {
        free(child_dir);
        child_dir = NULL;
    }
    
    return ret;
}