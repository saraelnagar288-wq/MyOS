#include <myos/fs.h>
#include <myos/kheap.h>
#include <myos/kernel.h>

static vfs_node_t* g_vfs_root = NULL;

void vfs_init(void) {
    g_vfs_root = ramfs_init();
    LOG_INFO("VFS", "Virtual File System initialized with RAMFS root");
}

vfs_node_t* vfs_get_root(void) {
    return g_vfs_root;
}

uint32_t vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (node && node->read) {
        return node->read(node, offset, size, buffer);
    }
    return 0;
}

uint32_t vfs_write(vfs_node_t* node, uint32_t offset, uint32_t size, const uint8_t* buffer) {
    if (node && node->write) {
        return node->write(node, offset, size, buffer);
    }
    return 0;
}

void vfs_open(vfs_node_t* node) {
    if (node && node->open) {
        node->open(node);
    }
}

void vfs_close(vfs_node_t* node) {
    if (node && node->close) {
        node->close(node);
    }
}

vfs_dirent_t* vfs_readdir(vfs_node_t* node, uint32_t index) {
    if (node && (node->flags & FS_DIRECTORY) && node->readdir) {
        return node->readdir(node, index);
    }
    return NULL;
}

vfs_node_t* vfs_finddir(vfs_node_t* node, const char* name) {
    if (node && (node->flags & FS_DIRECTORY) && node->finddir) {
        return node->finddir(node, name);
    }
    return NULL;
}

vfs_node_t* vfs_lookup(const char* path) {
    if (!path || !g_vfs_root) return NULL;
    if (kstrcmp(path, "/") == 0) return g_vfs_root;

    /* Skip leading slash */
    if (*path == '/') path++;

    char element[VFS_MAX_NAME];
    vfs_node_t* current = g_vfs_root;

    while (*path) {
        int i = 0;
        while (*path && *path != '/' && i < VFS_MAX_NAME - 1) {
            element[i++] = *path++;
        }
        element[i] = '\0';
        if (*path == '/') path++;

        vfs_node_t* next = vfs_finddir(current, element);
        if (!next) return NULL;
        current = next;
    }

    return current;
}
