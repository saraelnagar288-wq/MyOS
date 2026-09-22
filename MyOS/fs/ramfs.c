#include <myos/fs.h>
#include <myos/kheap.h>
#include <myos/kernel.h>

#define RAMFS_MAX_CHILDREN 32

typedef struct ramfs_entry {
    char name[VFS_MAX_NAME];
    bool is_dir;
    uint8_t* data;
    size_t size;
    size_t capacity;
    struct ramfs_entry* children[RAMFS_MAX_CHILDREN];
    uint32_t child_count;
    struct ramfs_entry* parent;
    vfs_node_t node;
} ramfs_entry_t;

static ramfs_entry_t* g_ramfs_root = NULL;
static vfs_dirent_t   g_shared_dirent;

static uint32_t ramfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    ramfs_entry_t* entry = (ramfs_entry_t*)node->ptr;
    if (!entry || entry->is_dir || !entry->data) return 0;

    if (offset >= entry->size) return 0;
    if (offset + size > entry->size) {
        size = entry->size - offset;
    }

    kmemcpy(buffer, entry->data + offset, size);
    return size;
}

static uint32_t ramfs_write(vfs_node_t* node, uint32_t offset, uint32_t size, const uint8_t* buffer) {
    ramfs_entry_t* entry = (ramfs_entry_t*)node->ptr;
    if (!entry || entry->is_dir) return 0;

    if (offset + size > entry->capacity) {
        size_t new_cap = (offset + size) * 2;
        entry->data = (uint8_t*)krealloc(entry->data, new_cap);
        entry->capacity = new_cap;
    }

    kmemcpy(entry->data + offset, buffer, size);
    if (offset + size > entry->size) {
        entry->size = offset + size;
        node->length = entry->size;
    }
    return size;
}

static vfs_dirent_t* ramfs_readdir(vfs_node_t* node, uint32_t index) {
    ramfs_entry_t* entry = (ramfs_entry_t*)node->ptr;
    if (!entry || !entry->is_dir || index >= entry->child_count) return NULL;

    ramfs_entry_t* child = entry->children[index];
    kstrcpy(g_shared_dirent.name, child->name);
    g_shared_dirent.inode = child->node.inode;
    g_shared_dirent.type  = child->is_dir ? FS_DIRECTORY : FS_FILE;
    g_shared_dirent.size  = child->size;

    return &g_shared_dirent;
}

static vfs_node_t* ramfs_finddir(vfs_node_t* node, const char* name) {
    ramfs_entry_t* entry = (ramfs_entry_t*)node->ptr;
    if (!entry || !entry->is_dir || !name) return NULL;

    for (uint32_t i = 0; i < entry->child_count; i++) {
        if (kstrcmp(entry->children[i]->name, name) == 0) {
            return &entry->children[i]->node;
        }
    }
    return NULL;
}

static ramfs_entry_t* ramfs_create_entry(const char* name, bool is_dir, ramfs_entry_t* parent) {
    ramfs_entry_t* entry = (ramfs_entry_t*)kzalloc(sizeof(ramfs_entry_t));
    kstrcpy(entry->name, name);
    entry->is_dir = is_dir;
    entry->parent = parent;
    entry->child_count = 0;

    static uint32_t s_next_inode = 1;
    entry->node.inode = s_next_inode++;
    kstrcpy(entry->node.name, name);
    entry->node.flags = is_dir ? FS_DIRECTORY : FS_FILE;
    entry->node.read = ramfs_read;
    entry->node.write = ramfs_write;
    entry->node.readdir = ramfs_readdir;
    entry->node.finddir = ramfs_finddir;
    entry->node.ptr = (struct vfs_node*)entry;

    if (parent && parent->child_count < RAMFS_MAX_CHILDREN) {
        parent->children[parent->child_count++] = entry;
    }

    return entry;
}

bool ramfs_create_file(const char* path, const uint8_t* data, size_t size) {
    /* For simplicity, extract dir and filename */
    char dir_path[VFS_MAX_PATH];
    char file_name[VFS_MAX_NAME];

    const char* last_slash = NULL;
    const char* p = path;
    while (*p) {
        if (*p == '/') last_slash = p;
        p++;
    }

    ramfs_entry_t* parent = g_ramfs_root;
    if (last_slash && last_slash != path) {
        size_t dlen = last_slash - path;
        kstrncpy(dir_path, path, dlen);
        dir_path[dlen] = '\0';
        vfs_node_t* dnode = vfs_lookup(dir_path);
        if (dnode) {
            parent = (ramfs_entry_t*)dnode->ptr;
        }
        kstrcpy(file_name, last_slash + 1);
    } else {
        kstrcpy(file_name, (last_slash == path) ? path + 1 : path);
    }

    ramfs_entry_t* file = ramfs_create_entry(file_name, false, parent);
    if (size > 0 && data) {
        file->data = (uint8_t*)kmalloc(size);
        file->capacity = size;
        file->size = size;
        file->node.length = size;
        kmemcpy(file->data, data, size);
    }
    return true;
}

bool ramfs_create_dir(const char* path) {
    /* Create directory under root or parent */
    const char* name = (*path == '/') ? path + 1 : path;
    ramfs_create_entry(name, true, g_ramfs_root);
    return true;
}

vfs_node_t* ramfs_init(void) {
    g_ramfs_root = ramfs_create_entry("/", true, NULL);

    /* Create Standard File System Layout:
       /system, /bin, /sbin, /lib, /etc, /dev, /proc, /tmp, /home, /var */
    ramfs_entry_t* dir_system = ramfs_create_entry("system", true, g_ramfs_root);
    ramfs_entry_t* dir_bin    = ramfs_create_entry("bin",    true, g_ramfs_root);
    ramfs_create_entry("sbin",   true, g_ramfs_root);
    ramfs_create_entry("lib",    true, g_ramfs_root);
    ramfs_entry_t* dir_etc    = ramfs_create_entry("etc",    true, g_ramfs_root);
    ramfs_create_entry("dev",    true, g_ramfs_root);
    ramfs_create_entry("proc",   true, g_ramfs_root);
    ramfs_create_entry("tmp",    true, g_ramfs_root);
    ramfs_entry_t* dir_home   = ramfs_create_entry("home",   true, g_ramfs_root);
    ramfs_create_entry("var",    true, g_ramfs_root);

    /* Populate standard files */
    const char* motd = "Welcome to MyOS 0.1.0 x86-64!\nA lightweight, modern, modular operating system.\n";
    ramfs_entry_t* f_motd = ramfs_create_entry("motd", false, dir_system);
    f_motd->size = kstrlen(motd);
    f_motd->data = (uint8_t*)kmalloc(f_motd->size);
    kmemcpy(f_motd->data, motd, f_motd->size);
    f_motd->node.length = f_motd->size;

    const char* os_release = "NAME=\"MyOS\"\nVERSION=\"0.1.0\"\nID=myos\nPRETTY_NAME=\"MyOS 0.1.0 x86-64\"\n";
    ramfs_entry_t* f_osrel = ramfs_create_entry("os-release", false, dir_etc);
    f_osrel->size = kstrlen(os_release);
    f_osrel->data = (uint8_t*)kmalloc(f_osrel->size);
    kmemcpy(f_osrel->data, os_release, f_osrel->size);
    f_osrel->node.length = f_osrel->size;

    const char* hostname = "myos-station\n";
    ramfs_entry_t* f_host = ramfs_create_entry("hostname", false, dir_etc);
    f_host->size = kstrlen(hostname);
    f_host->data = (uint8_t*)kmalloc(f_host->size);
    kmemcpy(f_host->data, hostname, f_host->size);
    f_host->node.length = f_host->size;

    /* Binary stubs in /bin */
    ramfs_create_entry("init", false, dir_bin);
    ramfs_create_entry("mysh", false, dir_bin);

    /* User directory */
    ramfs_create_entry("user", true, dir_home);

    return &g_ramfs_root->node;
}
