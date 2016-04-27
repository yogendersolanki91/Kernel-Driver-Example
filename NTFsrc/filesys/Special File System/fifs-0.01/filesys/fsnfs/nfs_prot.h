#include <rpc/types.h>
#define NFS_PORT 2049
#define NFS_MAXDATA 8192
#define NFS_MAXPATHLEN 1024
#define NFS_MAXNAMLEN 255
#define NFS_FHSIZE 32
#define NFS_COOKIESIZE 4
#define NFS_FIFO_DEV -1
#define NFSMODE_FMT 0170000
#define NFSMODE_DIR 0040000
#define NFSMODE_CHR 0020000
#define NFSMODE_BLK 0060000
#define NFSMODE_REG 0100000
#define NFSMODE_LNK 0120000
#define NFSMODE_SOCK 0140000
#define NFSMODE_FIFO 0010000

enum nfsstat {
	NFS_OK = 0,
	NFSERR_PERM = 1,
	NFSERR_NOENT = 2,
	NFSERR_IO = 5,
	NFSERR_NXIO = 6,
	NFSERR_ACCES = 13,
	NFSERR_EXIST = 17,
	NFSERR_NODEV = 19,
	NFSERR_NOTDIR = 20,
	NFSERR_ISDIR = 21,
	NFSERR_FBIG = 27,
	NFSERR_NOSPC = 28,
	NFSERR_ROFS = 30,
	NFSERR_NAMETOOLONG = 63,
	NFSERR_NOTEMPTY = 66,
	NFSERR_DQUOT = 69,
	NFSERR_STALE = 70,
	NFSERR_WFLUSH = 99,
};
typedef enum nfsstat nfsstat;
#ifdef __cplusplus
extern "C" {
bool_t xdr_nfsstat(...);
}
#else
bool_t xdr_nfsstat();
#endif


enum ftype {
	NFNON = 0,
	NFREG = 1,
	NFDIR = 2,
	NFBLK = 3,
	NFCHR = 4,
	NFLNK = 5,
	NFSOCK = 6,
	NFBAD = 7,
	NFFIFO = 8,
};
typedef enum ftype ftype;
#ifdef __cplusplus
extern "C" {
bool_t xdr_ftype(...);
}
#else
bool_t xdr_ftype();
#endif


struct nfs_fh {
	char data[NFS_FHSIZE];
};
typedef struct nfs_fh nfs_fh;
#ifdef __cplusplus
extern "C" {
bool_t xdr_nfs_fh(...);
}
#else
bool_t xdr_nfs_fh();
#endif


struct nfstime {
	u_int seconds;
	u_int useconds;
};
typedef struct nfstime nfstime;
#ifdef __cplusplus
extern "C" {
bool_t xdr_nfstime(...);
}
#else
bool_t xdr_nfstime();
#endif


struct fattr {
	ftype type;
	u_int mode;
	u_int nlink;
	u_int uid;
	u_int gid;
	u_int size;
	u_int blocksize;
	u_int rdev;
	u_int blocks;
	u_int fsid;
	u_int fileid;
	nfstime atime;
	nfstime mtime;
	nfstime ctime;
};
typedef struct fattr fattr;
#ifdef __cplusplus
extern "C" {
bool_t xdr_fattr(...);
}
#else
bool_t xdr_fattr();
#endif


struct sattr {
	u_int mode;
	u_int uid;
	u_int gid;
	u_int size;
	nfstime atime;
	nfstime mtime;
};
typedef struct sattr sattr;
#ifdef __cplusplus
extern "C" {
bool_t xdr_sattr(...);
}
#else
bool_t xdr_sattr();
#endif


typedef char *filename;
#ifdef __cplusplus
extern "C" {
bool_t xdr_filename(...);
}
#else
bool_t xdr_filename();
#endif


typedef char *nfspath;
#ifdef __cplusplus
extern "C" {
bool_t xdr_nfspath(...);
}
#else
bool_t xdr_nfspath();
#endif


struct attrstat {
	nfsstat status;
	union {
		fattr attributes;
	} attrstat_u;
};
typedef struct attrstat attrstat;
#ifdef __cplusplus
extern "C" {
bool_t xdr_attrstat(...);
}
#else
bool_t xdr_attrstat();
#endif


struct sattrargs {
	nfs_fh file;
	sattr attributes;
};
typedef struct sattrargs sattrargs;
#ifdef __cplusplus
extern "C" {
bool_t xdr_sattrargs(...);
}
#else
bool_t xdr_sattrargs();
#endif


struct diropargs {
	nfs_fh dir;
	filename name;
};
typedef struct diropargs diropargs;
#ifdef __cplusplus
extern "C" {
bool_t xdr_diropargs(...);
}
#else
bool_t xdr_diropargs();
#endif


struct diropokres {
	nfs_fh file;
	fattr attributes;
};
typedef struct diropokres diropokres;
#ifdef __cplusplus
extern "C" {
bool_t xdr_diropokres(...);
}
#else
bool_t xdr_diropokres();
#endif


struct diropres {
	nfsstat status;
	union {
		diropokres diropres;
	} diropres_u;
};
typedef struct diropres diropres;
#ifdef __cplusplus
extern "C" {
bool_t xdr_diropres(...);
}
#else
bool_t xdr_diropres();
#endif


struct readlinkres {
	nfsstat status;
	union {
		nfspath data;
	} readlinkres_u;
};
typedef struct readlinkres readlinkres;
#ifdef __cplusplus
extern "C" {
bool_t xdr_readlinkres(...);
}
#else
bool_t xdr_readlinkres();
#endif


struct readargs {
	nfs_fh file;
	u_int offset;
	u_int count;
	u_int totalcount;
};
typedef struct readargs readargs;
#ifdef __cplusplus
extern "C" {
bool_t xdr_readargs(...);
}
#else
bool_t xdr_readargs();
#endif


struct readokres {
	fattr attributes;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct readokres readokres;
#ifdef __cplusplus
extern "C" {
bool_t xdr_readokres(...);
}
#else
bool_t xdr_readokres();
#endif


struct readres {
	nfsstat status;
	union {
		readokres reply;
	} readres_u;
};
typedef struct readres readres;
#ifdef __cplusplus
extern "C" {
bool_t xdr_readres(...);
}
#else
bool_t xdr_readres();
#endif


struct writeargs {
	nfs_fh file;
	u_int beginoffset;
	u_int offset;
	u_int totalcount;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct writeargs writeargs;
#ifdef __cplusplus
extern "C" {
bool_t xdr_writeargs(...);
}
#else
bool_t xdr_writeargs();
#endif


struct createargs {
	diropargs where;
	sattr attributes;
};
typedef struct createargs createargs;
#ifdef __cplusplus
extern "C" {
bool_t xdr_createargs(...);
}
#else
bool_t xdr_createargs();
#endif


struct renameargs {
	diropargs from;
	diropargs to;
};
typedef struct renameargs renameargs;
#ifdef __cplusplus
extern "C" {
bool_t xdr_renameargs(...);
}
#else
bool_t xdr_renameargs();
#endif


struct linkargs {
	nfs_fh from;
	diropargs to;
};
typedef struct linkargs linkargs;
#ifdef __cplusplus
extern "C" {
bool_t xdr_linkargs(...);
}
#else
bool_t xdr_linkargs();
#endif


struct symlinkargs {
	diropargs from;
	nfspath to;
	sattr attributes;
};
typedef struct symlinkargs symlinkargs;
#ifdef __cplusplus
extern "C" {
bool_t xdr_symlinkargs(...);
}
#else
bool_t xdr_symlinkargs();
#endif


typedef char nfscookie[NFS_COOKIESIZE];
#ifdef __cplusplus
extern "C" {
bool_t xdr_nfscookie(...);
}
#else
bool_t xdr_nfscookie();
#endif


struct readdirargs {
	nfs_fh dir;
	nfscookie cookie;
	u_int count;
};
typedef struct readdirargs readdirargs;
#ifdef __cplusplus
extern "C" {
bool_t xdr_readdirargs(...);
}
#else
bool_t xdr_readdirargs();
#endif


struct entry {
	u_int fileid;
	filename name;
	nfscookie cookie;
	struct entry *nextentry;
};
typedef struct entry entry;
#ifdef __cplusplus
extern "C" {
bool_t xdr_entry(...);
}
#else
bool_t xdr_entry();
#endif


struct dirlist {
	entry *entries;
	bool_t eof;
};
typedef struct dirlist dirlist;
#ifdef __cplusplus
extern "C" {
bool_t xdr_dirlist(...);
}
#else
bool_t xdr_dirlist();
#endif


struct readdirres {
	nfsstat status;
	union {
		dirlist reply;
	} readdirres_u;
};
typedef struct readdirres readdirres;
#ifdef __cplusplus
extern "C" {
bool_t xdr_readdirres(...);
}
#else
bool_t xdr_readdirres();
#endif


struct statfsokres {
	u_int tsize;
	u_int bsize;
	u_int blocks;
	u_int bfree;
	u_int bavail;
};
typedef struct statfsokres statfsokres;
#ifdef __cplusplus
extern "C" {
bool_t xdr_statfsokres(...);
}
#else
bool_t xdr_statfsokres();
#endif


struct statfsres {
	nfsstat status;
	union {
		statfsokres reply;
	} statfsres_u;
};
typedef struct statfsres statfsres;
#ifdef __cplusplus
extern "C" {
bool_t xdr_statfsres(...);
}
#else
bool_t xdr_statfsres();
#endif


#define NFS_PROGRAM ((u_long)100003)
#define NFS_VERSION ((u_long)2)
#define NFSPROC_NULL ((u_long)0)
#ifdef __cplusplus
extern "C" {
extern void *nfsproc_null_2(...);
}
#else
extern void *nfsproc_null_2();
#endif /* __cplusplus */
#define NFSPROC_GETATTR ((u_long)1)
#ifdef __cplusplus
extern "C" {
extern attrstat *nfsproc_getattr_2(...);
}
#else
extern attrstat *nfsproc_getattr_2();
#endif /* __cplusplus */
#define NFSPROC_SETATTR ((u_long)2)
#ifdef __cplusplus
extern "C" {
extern attrstat *nfsproc_setattr_2(...);
}
#else
extern attrstat *nfsproc_setattr_2();
#endif /* __cplusplus */
#define NFSPROC_ROOT ((u_long)3)
#ifdef __cplusplus
extern "C" {
extern void *nfsproc_root_2(...);
}
#else
extern void *nfsproc_root_2();
#endif /* __cplusplus */
#define NFSPROC_LOOKUP ((u_long)4)
#ifdef __cplusplus
extern "C" {
extern diropres *nfsproc_lookup_2(...);
}
#else
extern diropres *nfsproc_lookup_2();
#endif /* __cplusplus */
#define NFSPROC_READLINK ((u_long)5)
#ifdef __cplusplus
extern "C" {
extern readlinkres *nfsproc_readlink_2(...);
}
#else
extern readlinkres *nfsproc_readlink_2();
#endif /* __cplusplus */
#define NFSPROC_READ ((u_long)6)
#ifdef __cplusplus
extern "C" {
extern readres *nfsproc_read_2(...);
}
#else
extern readres *nfsproc_read_2();
#endif /* __cplusplus */
#define NFSPROC_WRITECACHE ((u_long)7)
#ifdef __cplusplus
extern "C" {
extern void *nfsproc_writecache_2(...);
}
#else
extern void *nfsproc_writecache_2();
#endif /* __cplusplus */
#define NFSPROC_WRITE ((u_long)8)
#ifdef __cplusplus
extern "C" {
extern attrstat *nfsproc_write_2(...);
}
#else
extern attrstat *nfsproc_write_2();
#endif /* __cplusplus */
#define NFSPROC_CREATE ((u_long)9)
#ifdef __cplusplus
extern "C" {
extern diropres *nfsproc_create_2(...);
}
#else
extern diropres *nfsproc_create_2();
#endif /* __cplusplus */
#define NFSPROC_REMOVE ((u_long)10)
#ifdef __cplusplus
extern "C" {
extern nfsstat *nfsproc_remove_2(...);
}
#else
extern nfsstat *nfsproc_remove_2();
#endif /* __cplusplus */
#define NFSPROC_RENAME ((u_long)11)
#ifdef __cplusplus
extern "C" {
extern nfsstat *nfsproc_rename_2(...);
}
#else
extern nfsstat *nfsproc_rename_2();
#endif /* __cplusplus */
#define NFSPROC_LINK ((u_long)12)
#ifdef __cplusplus
extern "C" {
extern nfsstat *nfsproc_link_2(...);
}
#else
extern nfsstat *nfsproc_link_2();
#endif /* __cplusplus */
#define NFSPROC_SYMLINK ((u_long)13)
#ifdef __cplusplus
extern "C" {
extern nfsstat *nfsproc_symlink_2(...);
}
#else
extern nfsstat *nfsproc_symlink_2();
#endif /* __cplusplus */
#define NFSPROC_MKDIR ((u_long)14)
#ifdef __cplusplus
extern "C" {
extern diropres *nfsproc_mkdir_2(...);
}
#else
extern diropres *nfsproc_mkdir_2();
#endif /* __cplusplus */
#define NFSPROC_RMDIR ((u_long)15)
#ifdef __cplusplus
extern "C" {
extern nfsstat *nfsproc_rmdir_2(...);
}
#else
extern nfsstat *nfsproc_rmdir_2();
#endif /* __cplusplus */
#define NFSPROC_READDIR ((u_long)16)
#ifdef __cplusplus
extern "C" {
extern readdirres *nfsproc_readdir_2(...);
}
#else
extern readdirres *nfsproc_readdir_2();
#endif /* __cplusplus */
#define NFSPROC_STATFS ((u_long)17)
#ifdef __cplusplus
extern "C" {
extern statfsres *nfsproc_statfs_2(...);
}
#else
extern statfsres *nfsproc_statfs_2();
#endif /* __cplusplus */

