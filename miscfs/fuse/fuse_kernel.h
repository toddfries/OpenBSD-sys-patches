/* $OpenBSD$ */
/*
    Copyright (C) 2012-2013 Sylvestre Gallon <ccna.syl@gmail.com>
    Copyright (C) 2001-2007 Miklos Szeredi. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
#ifndef __FUSE_KERNEL_H__
#define __FUSE_KERNEL_H__

#define FUSE_KERNEL_VERSION 7
#define FUSE_KERNEL_MINOR_VERSION 8

/** The node ID of the root inode */
#define FUSE_ROOT_ID 1

struct fuse_kstatfs {
	uint64_t blocks;
	uint64_t bfree;
	uint64_t bavail;
	uint64_t files;
	uint64_t ffree;
	uint32_t bsize;
	uint32_t namelen;
	uint32_t frsize;
	uint32_t padding;
	uint32_t spare[6];
};

enum fuse_opcode {
	FUSE_LOOKUP =		1,
	FUSE_FORGET =		2,  /* no reply */
	FUSE_GETATTR =		3,
	FUSE_SETATTR =		4,
	FUSE_READLINK =		5,
	FUSE_SYMLINK =		6,
	FUSE_MKNOD =		8,
	FUSE_MKDIR =		9,
	FUSE_UNLINK =		10,
	FUSE_RMDIR =		11,
	FUSE_RENAME =		12,
	FUSE_LINK =		13,
	FUSE_OPEN =		14,
	FUSE_READ =		15,
	FUSE_WRITE =		16,
	FUSE_STATFS =		17,
	FUSE_RELEASE =		18,
	FUSE_FSYNC =		20,
	FUSE_SETXATTR =		21,
	FUSE_GETXATTR =		22,
	FUSE_LISTXATTR =	23,
	FUSE_REMOVEXATTR =	24,
	FUSE_FLUSH =		25,
	FUSE_INIT =		26,
	FUSE_OPENDIR =		27,
	FUSE_READDIR =		28,
	FUSE_RELEASEDIR =	29,
	FUSE_FSYNCDIR =		30,
	FUSE_GETLK =		31,
	FUSE_SETLK =		32,
	FUSE_SETLKW =		33,
	FUSE_ACCESS =		34,
	FUSE_CREATE =		35,
	FUSE_INTERRUPT =	36,
	FUSE_BMAP =		37,
	FUSE_DESTROY =		38,
};

struct fuse_in_header {
	uint32_t len;
	uint32_t opcode;
	uint64_t unique;
	uint64_t nodeid;
	uint32_t uid;
	uint32_t gid;
	uint32_t pid;
	uint32_t padding;
};

struct fuse_init_in {
	uint32_t major;
	uint32_t minor;
	uint32_t max_readahead;
	uint32_t flags;
};

struct fuse_out_header {
	uint32_t len;
	uint32_t error;
	uint64_t unique;
};

struct fuse_init_out {
	uint32_t major;
	uint32_t minor;
	uint32_t max_readahead;
	uint32_t flags;
	uint32_t unused;
	uint32_t max_write;
};

struct fuse_statfs_out {
	struct fuse_kstatfs st;
};

struct fuse_attr {
	uint64_t ino;
	uint64_t size;
	uint64_t blocks;
	uint64_t atime;
	uint64_t mtime;
	uint64_t ctime;
	uint32_t atimensec;
	uint32_t mtimensec;
	uint32_t ctimensec;
	uint32_t mode;
	uint32_t nlink;
	uint32_t uid;
	uint32_t gid;
	uint32_t rdev;
	uint32_t blksize;
	uint32_t padding;
};

struct fuse_attr_in {
	uint32_t getattr_flags;
	uint32_t dummy;
	uint32_t fh;
};

struct fuse_attr_out {
	uint64_t attr_valid;
	uint32_t attr_valid_nsec;
	uint32_t dummy;
	struct fuse_attr attr;
};

struct fuse_access_in {
	uint32_t mask;
	uint32_t padding;
};

struct fuse_open_in {
	uint32_t flags;
	uint32_t mode;
};

struct fuse_open_out {
	uint64_t fh;
	uint32_t open_flags;
	uint32_t padding;
};

struct fuse_release_in {
	uint64_t fh;
	uint32_t flags;
	uint32_t release_flags;
	uint64_t lock_owner;
};

struct fuse_read_in {
	uint64_t fh;
	uint64_t offset;
	uint32_t size;
	uint32_t padding;
};

struct fuse_dirent {
	uint64_t ino;
	uint64_t off;
	uint32_t namelen;
	uint32_t type;
	char name[0];
};

struct pseudo_dirent {
	uint32_t d_namlen;
};

struct fuse_entry_out {
	uint64_t nodeid;
	uint64_t generation;

	uint64_t entry_valid;	/* Cache timeout for the name */
	uint64_t attr_valid;	/* Cache timeout for the attributes */
	uint32_t entry_valid_nsec;
	uint32_t attr_valid_nsec;
	struct fuse_attr attr;
};

struct fuse_mkdir_in {
	uint32_t mode;
	uint32_t padding;
};

struct fuse_link_in {
	uint64_t oldnodeid;
};

struct fuse_write_in {
	uint64_t fh;
	uint64_t offset;
	uint32_t size;
	uint32_t write_flags;
};

struct fuse_write_out {
	uint32_t size;
	uint32_t padding;
};

/**
 * Bitmasks for fuse_setattr_in.valid
 */
#define FATTR_MODE	(1 << 0)
#define FATTR_UID	(1 << 1)
#define FATTR_GID	(1 << 2)
#define FATTR_SIZE	(1 << 3)
#define FATTR_ATIME	(1 << 4)
#define FATTR_MTIME	(1 << 5)
#define FATTR_FH	(1 << 6)

struct fuse_setattr_in {
	uint32_t valid;
	uint32_t padding;
	uint64_t fh;
	uint64_t size;
	uint64_t unused1;
	uint64_t atime;
	uint64_t mtime;
	uint64_t unused2;
	uint32_t atimensec;
	uint32_t mtimensec;
	uint32_t unused3;
	uint32_t mode;
	uint32_t unused4;
	uint32_t uid;
	uint32_t gid;
	uint32_t unused5;
};

#define FUSE_NAME_OFFSET offsetof(struct fuse_dirent, name)
#define FUSE_DIRENT_ALIGN(x) (((x)+sizeof(uint64_t)-1) & ~(sizeof(uint64_t)-1))
#define FUSE_DIRENT_SIZE(d) \
	FUSE_DIRENT_ALIGN(FUSE_NAME_OFFSET + (d)->namelen)

#endif /* __FUSEFS_H__ */
