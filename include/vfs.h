#ifndef VFS_H
#define VFS_H

#include <stdbool.h>
#include <stdint.h>

#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct vfs_node;

typedef struct dirent {
  char name[128];
  uint32_t ino;
} dirent_t;

typedef uint32_t (*vfs_read_func)(struct vfs_node *, uint32_t, uint32_t,
                                  uint8_t *);
typedef uint32_t (*vfs_write_func)(struct vfs_node *, uint32_t, uint32_t,
                                   uint8_t *);
typedef void (*vfs_open_func)(struct vfs_node *);
typedef void (*vfs_close_func)(struct vfs_node *);
typedef struct dirent *(*vfs_readdir_func)(struct vfs_node *, uint32_t);
typedef struct vfs_node *(*vfs_finddir_func)(struct vfs_node *, char *);

typedef struct vfs_node {
  char name[128];
  uint32_t flags;
  uint32_t inode;
  uint32_t length;
  vfs_read_func read;
  vfs_write_func write;
  vfs_open_func open;
  vfs_close_func close;
  vfs_readdir_func readdir;
  vfs_finddir_func finddir;
} vfs_node_t;

typedef struct {
  bool in_use;
  vfs_node_t *node;
  uint32_t position;
  uint32_t flags;
} file_descriptor_t;

void vfs_init();
int vfs_mount(const char *path, vfs_node_t *fs_root);
vfs_node_t *vfs_get_root();
int vfs_open(const char *path, uint32_t flags);
void vfs_close(int fd);
int vfs_read(int fd, void *buffer, uint32_t size);
int vfs_write(int fd, const void *buffer, uint32_t size);
int vfs_seek(int fd, int offset, int whence);
vfs_node_t *vfs_opendir(const char *path);
dirent_t *vfs_readdir(vfs_node_t *node, uint32_t index);
vfs_node_t *vfs_finddir(vfs_node_t *node, char *name);
int vfs_stat(const char *path, vfs_node_t *stat_buf);
int vfs_mkdir(const char *path);
int vfs_create(const char *path);
int vfs_unlink(const char *path);

#endif
