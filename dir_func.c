#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <mfs.h>
//#include "vc_block.c"
#include "freeAlloc.c"

/*
example of make dir with permissions
status = mkdir("/home/cnd/mod1", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
*/

dir_entr * dirent;        // this will be used to make a new child directory
dir_entr * temp_curr_dir; // this will represent parent_dir and be reset at the end of mk_dir()
dir_entr * curr_dir;      // this will be kept in memory 
void create_dir(char * name, int permissions) 
{
    if (dirent == NULL)
    {
        dirent = malloc(VCB->block_size*6);
    }
    
    strncpy(dirent[0].filename, name, strlen(name));

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
        dirent[1].starting_block = temp_curr_dir->starting_block;
        dirent[0].permissions = permissions;
        dirent[1].permissions = temp_curr_dir->permissions;
        strncpy(dirent[1].filename, temp_curr_dir->filename, strlen(temp_curr_dir->filename));
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
}

char saved_filename[20];
int ret;
int num_of_paths;

// to be called in parse_path(). This function will update current working directory
int validate_path(char * name) {

    // this represents the paths remaining to be searched
    num_of_paths--;

    if (curr_dir == NULL)
    {
        curr_dir = malloc(VCB->block_size*6);
        temp_curr_dir = malloc(VCB->block_size*6);
        temp_curr_dir = curr_dir;

        // starting from root directory
        LBAread(temp_curr_dir, 6, VCB->root_start);
    }
    
    else
    {
        temp_curr_dir = malloc(VCB->block_size*6);
        temp_curr_dir = curr_dir;

        // starting from the current working directory
        LBAread(temp_curr_dir, 6, curr_dir[0].starting_block);
    }

    //iterate through dir_entries of current directory
    for (int i = 2; i < 64; i++)
    {
        if (strcmp(temp_curr_dir[i].filename, name) == 0 && num_of_paths > 0)
        {
            printf("found path\n");

            // updating current directory with found path
            LBAread(temp_curr_dir, 6, temp_curr_dir[i].starting_block);

            printf("Current directory: %s\n\n", temp_curr_dir[0].filename);
            return 0;
        }
        else if (strcmp(temp_curr_dir[i].filename, name) == 0 && num_of_paths == 0)
        {
            printf("path already exists\n");
            printf("NOT able to create this directory\n");

            // updating current directory with found path
            LBAread(temp_curr_dir, 6, temp_curr_dir[i].starting_block);
            LBAread(curr_dir, 6, temp_curr_dir[i].starting_block);

            printf("Current directory: %s\n\n", curr_dir[0].filename);
            return -1;
        }
        else if (i == 63 && num_of_paths > 0)
        {
            // this else-container implies that path was not found while there are still paths left to search
            printf("path does not exist\n");
            printf("NOT able to create this directory\n");

            return -1;
        }
        else if (i == 63 && num_of_paths == 0)
        {
            printf("path does not exist\n");
            printf("able to create this directory\n");

            // save name
            strncpy(saved_filename, name, strlen(name));
            // saved_filename = name;
            return 0;
        }
    }
}


int parse_pathname(const char * pathname)
{
    char count_slashes[20];
    
    strncpy(count_slashes, pathname, strlen(pathname));
    char * buffer_pathname = strdup(pathname);

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

    // call str_tok to divide pathname into tokens
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
            if (strcmp(temp_curr_dir[i].filename, "") == 0)
            {
                //creating new directory
                strncpy(temp_curr_dir[i].filename, saved_filename, strlen(saved_filename));
                temp_curr_dir[i].starting_block = VCB->free_block_start;

                printf("Current Directory: %s\n", temp_curr_dir[0].filename);
                printf("%s added to index %d in %s Directory\n", temp_curr_dir[i].filename, i, temp_curr_dir[0].filename);
                printf("%s starting block: %d\n\n", temp_curr_dir[i].filename, VCB->free_block_start);

                create_dir(saved_filename, mode);

                // update current directory on disk
                LBAwrite(temp_curr_dir, 6, temp_curr_dir[0].starting_block);

                // reset current working directory if needed
                free(temp_curr_dir);
                temp_curr_dir = NULL;
                
                break;
            }
            else if (i == 63)
            {
                printf("Unable to find free directory entry inside /%s\n", temp_curr_dir[0].filename);
            }
        }
    }
    else 
    {
        //reset current working directory
        free(temp_curr_dir);
        temp_curr_dir = NULL;
    }
    return ret;
}

int fs_rmdir(const char *pathname) 
{

}

dir_entr * currentDir;
int dir_ent_position;
int load_Dir(char * token){
    dir_ent_position = -1;
    for(int i = 0; i<64; i++){
        if(strcmp(token, currentDir[i].filename)==0){
            dir_ent_position = i;
            printf("MATCH token : %s\n", token);
            LBAread(currentDir, 6, currentDir[i].starting_block);
            break;
        }
        else if(i ==63){
            printf("This does not exist: %s\n", token);
        }
    }
    return dir_ent_position;
    
}

int parsePath(const char * pathname){
    //first load the entire root directory
    currentDir = malloc(512*6);
    LBAread(currentDir, 6, 6);

    //store path name into char copy to use in strtok
    char * copy = strdup(pathname);
    
    char * token = strtok(copy, "/");
    
    while(token != NULL){
        //printf("%s\n", token);
        dir_ent_position = load_Dir(token);
        token = strtok(NULL, "/");
    }
    
    //printf("%d", dir_ent_position);
    return dir_ent_position;
}



//used in ls
fdDir * dirp;
fdDir * fs_opendir(const char * name){
    //load directory stream to iterate through
    /*Parse through each directory to check if it is valid and then load
    each as it is confirmed to be valid*/
    dir_ent_position = parsePath(name);
    if(dir_ent_position == -1){
        printf("File path does not exist.\n");
    }
    
    printf("%d\n", dir_ent_position);
    //make a pointer for the directory steam
    dirp = malloc(sizeof(fdDir));
    //always begin at file pos 0 in directory stream
    dirp->dirEntryPosition = dir_ent_position;
    //Location of the directory in relation to the LBA
    dirp->directoryStartLocation = currentDir[dir_ent_position].starting_block;
    printf("%s\n", currentDir[dir_ent_position].filename);
    // //file path
    // strcpy(dirp->filepath, name);
    return dirp;
}

//used in displayFiles()
struct fs_diriteminfo * dirItem;
struct fs_diriteminfo *fs_readdir(fdDir *dirp){
    /*every time readdir is called move the dirpointer 
    to the next item in the directory stream*/
    dirp->dirEntryPosition += 1;
    //assign filetype
    if(fs_isDir(dirp->filepath)){
        dirItem->fileType = FT_DIRECTORY;
    }
    if(fs_isFile(dirp->filepath)){
        dirItem->fileType = FT_REGFILE;
    }
}

//used in displayFiles()
int fs_closedir(fdDir *dirp){
    //frees up the memory you allocated for opendir
    free(currentDir);
    free(dirp);
    free(dirItem);
    return 0;
}

char * fs_getcwd(char *buf, size_t size){

}

int fs_isFile(char * path){
    return 0;
}

int fs_isDir(char * path){
    return 1;
}

// int fs_stat(const char *path, struct fs_stat *buf){
    
// }