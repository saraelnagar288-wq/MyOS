#ifndef MYOS_FS_H
#define MYOS_FS_H

#include <myos/types.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08

#define VFS_MAX_PATH 256
#define VFS_MAX_NAME 64

struct vfs_node;

typedef struct vfs_dirent {
    char name[VFS_MAX_NAME];
    uint32_t inode;
    uint32_t type;
    size_t size;
} vfs_dirent_t;

typedef uint32_t (*read_type_t)(struct vfs_node*, uint32_t offset, uint32_t size, uint8_t* buffer);
typedef uint32_t (*write_type_t)(struct vfs_node*, uint32_t offset, uint32_t size, const uint8_t* buffer);
typedef void (*open_type_t)(struct vfs_node*);
typedef void (*close_type_t)(struct vfs_node*);
typedef vfs_dirent_t* (*readdir_type_t)(struct vfs_node*, uint32_t index);
typedef struct vfs_node* (*finddir_type_t)(struct vfs_node*, const char* name);

typedef struct vfs_node {
    char name[VFS_MAX_NAME];
    uint32_t mask;
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;
    uint32_t inode;
    uint32_t length;
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    readdir_type_t readdir;
    finddir_type_t finddir;
    struct vfs_node* ptr; /* mount point or fs-specific data */
} vfs_node_t;

void vfs_init(void);
vfs_node_t* vfs_get_root(void);
vfs_node_t* vfs_lookup(const char* path);
uint32_t vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t vfs_write(vfs_node_t* node, uint32_t offset, uint32_t size, const uint8_t* buffer);
void vfs_open(vfs_node_t* node);
void vfs_close(vfs_node_t* node);
vfs_dirent_t* vfs_readdir(vfs_node_t* node, uint32_t index);
vfs_node_t* vfs_finddir(vfs_node_t* node, const char* name);

/* RAMFS Initialization */
vfs_node_t* ramfs_init(void);
bool ramfs_create_file(const char* path, const uint8_t* data, size_t size);
bool ramfs_create_dir(const char* path);

#endif /* MYOS_FS_H */
