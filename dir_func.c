#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <mfs.h>
#include "vc_block.c"

int fs_mkdir(const char *pathname, mode_t mode) {
    
}

int fs_rmdir(const char *pathname) {

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

int parsePathIter(const char * pathname){
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
    dir_ent_position = parsePathIter(name);
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

int fs_stat(const char *path, struct fs_stat *buf){
    
}
