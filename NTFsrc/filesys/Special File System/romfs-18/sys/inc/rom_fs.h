/*
    This is a romfs file system driver for Windows NT/2000/XP.
    Copyright (C) 1999, 2000, 2001 Bo Brantén.
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _ROM_FS_
#define _ROM_FS_

#define SECTOR_SIZE         512

#define BLOCK_SIZE          PAGE_SIZE

#define ROMFS_MAGIC         "-rom1fs-"

#define ROMFS_MAGIC_OFFSET  0

#define SIZEOF_ROMFS_INODE(x) \
    ((sizeof(struct romfs_inode) + strnlen(x->name, ROMFS_MAXFN) + ROMFH_SIZE) \
    & ROMFH_MASK)

//
// Types used by Linux
//
#include "ltypes.h"

//
// Use 1 byte packing of on-disk structures
//
#include <pshpack1.h>

//
// The following is a subset of linux/include/linux/romfs_fs.h from
// version 2.2.14
//

/* The basic structures of the romfs filesystem */

#define ROMBSIZE    BLOCK_SIZE

#define ROMFS_MAXFN 128

/* On-disk "super block" */

struct romfs_super_block {
    __u32 word0;
    __u32 word1;
    __u32 size;
    __u32 checksum;
    char  name[0];  /* volume name */
};

/* On disk inode */

struct romfs_inode {
    __u32 next;     /* low 4 bits see ROMFH_ */
    __u32 spec;
    __u32 size;
    __u32 checksum;
    char  name[0];
};

#define ROMFH_TYPE  7
#define ROMFH_HRD   0
#define ROMFH_DIR   1
#define ROMFH_REG   2
#define ROMFH_SYM   3
#define ROMFH_BLK   4
#define ROMFH_CHR   5
#define ROMFH_SCK   6
#define ROMFH_FIF   7
#define ROMFH_EXEC  8

/* Alignment */

#define ROMFH_SIZE  16
#define ROMFH_PAD   (ROMFH_SIZE-1)
#define ROMFH_MASK  (~ROMFH_PAD)

//
// End of subset of linux/include/linux/romfs_fs.h
//

//
// Use default packing of structures
//
#include <poppack.h>

#endif
