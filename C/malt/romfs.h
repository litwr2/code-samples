#ifndef MALT_ROMFS_H
#define MALT_ROMFS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef __MALT
#include <libmalt.h>
#endif

//#define ROMFS_VAR _binary_romfs_bin_start  //taken from an object file
#define FD_TABLE_SIZE 16

struct FD {
    unsigned size, current_offset;
    char *data_ptr_begin;
};

int romfs_close(int fd);
int romfs_open(const char *path, int flags, ...);
int romfs_read(int fd, void *buf, size_t nbytes);
off_t romfs_lseek(int fd, off_t offset, int whence);

#endif

