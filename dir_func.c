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

fdDir * dirp;
dir_entr * root;
fdDir * fs_opendir(const char * name){
    //load directory stream to iterate through
    root = malloc(512*6);
    LBAread(root, 6, 5);
    //make a pointer for the directory steam
    dirp = malloc(512);
    //always begin at file pos 0 in directory stream
    dirp->dirEntryPosition = 0;
    //Location of the directory in relation to the LBA
    dirp->directoryStartLocation = root->starting_block;
    //file path
    strcpy(dirp->filepath, name);
    return dirp;
}

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

int fs_closedir(fdDir *dirp){
    //frees up the memory you allocated for opendir
    free(root);
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
