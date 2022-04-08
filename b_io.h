/**************************************************************
* Class:  CSC-415-03  Fall 2021
* 
* Names: Kilian Kistenbroker, Emily Huang, Sean Locklar, 
* Shauhin Pourshayegan
*
* Student IDs: 920723372, 920499746, 920506337, 920447681
* 
* GitHub Name: KilianKistenbroker
* 
* Group Name: Team Poke
*
* Project: Basic File System
*
* File: b_io.h
*
* Description: Interface of basic I/O functions
*
**************************************************************/

#ifndef _B_IO_H
#define _B_IO_H
#include <fcntl.h>

typedef int b_io_fd;

b_io_fd b_open (char * filename, int flags);
int b_read (b_io_fd fd, char * buffer, int count);
int b_write (b_io_fd fd, char * buffer, int count);
int b_seek (b_io_fd fd, off_t offset, int whence);
void b_close (b_io_fd fd);

#endif

