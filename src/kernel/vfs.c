#include "vfs.h"
#include "kmalloc.h"
#include "string.h"
#include <stddef.h>

#define MAX_FILE_DESCRIPTORS 256

// File descriptor table
static file_descriptor_t fd_table[MAX_FILE_DESCRIPTORS];

// Root file system node
static vfs_node_t *vfs_root = NULL;

// Initialize VFS
void vfs_init() {
  // Clear file descriptor table
  for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
    fd_table[i].in_use = false;
    fd_table[i].node = NULL;
    fd_table[i].position = 0;
    fd_table[i].flags = 0;
  }
}

// Set the root file system
int vfs_mount(const char *path, vfs_node_t *fs_root) {
  // For now, we only support mounting at root "/"
  if (strcmp(path, "/") == 0) {
    vfs_root = fs_root;
    return 0;
  }
  return -1;
}

// Get root node
vfs_node_t *vfs_get_root() { return vfs_root; }

// Allocate a file descriptor
static int allocate_fd() {
  for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
    if (!fd_table[i].in_use) {
      fd_table[i].in_use = true;
      return i;
    }
  }
  return -1; // No free descriptors
}

// Parse path and return the node
static vfs_node_t *vfs_resolve_path(const char *path) {
  if (!vfs_root)
    return NULL;
  if (!path || path[0] == '\0')
    return NULL;

  // Handle root directory
  if (strcmp(path, "/") == 0) {
    return vfs_root;
  }

  // Start from root
  vfs_node_t *current = vfs_root;

  // Skip leading slash
  if (path[0] == '/')
    path++;

  char component[128];
  int comp_idx = 0;

  while (*path) {
    if (*path == '/' || *path == '\0') {
      if (comp_idx > 0) {
        component[comp_idx] = '\0';

        // Find this component in current directory
        if (current->finddir) {
          current = current->finddir(current, component);
          if (!current)
            return NULL;
        } else {
          return NULL;
        }

        comp_idx = 0;
      }
      if (*path == '/')
        path++;
    } else {
      if (comp_idx < 127) {
        component[comp_idx++] = *path;
      }
      path++;
    }
  }

  // Handle last component
  if (comp_idx > 0) {
    component[comp_idx] = '\0';
    if (current->finddir) {
      current = current->finddir(current, component);
    } else {
      return NULL;
    }
  }

  return current;
}

// Open a file
int vfs_open(const char *path, uint32_t flags) {
  vfs_node_t *node = vfs_resolve_path(path);
  if (!node)
    return -1;

  int fd = allocate_fd();
  if (fd < 0)
    return -1;

  fd_table[fd].node = node;
  fd_table[fd].position = 0;
  fd_table[fd].flags = flags;

  if (node->open) {
    node->open(node);
  }

  return fd;
}

// Close a file
void vfs_close(int fd) {
  if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS)
    return;
  if (!fd_table[fd].in_use)
    return;

  if (fd_table[fd].node && fd_table[fd].node->close) {
    fd_table[fd].node->close(fd_table[fd].node);
  }

  fd_table[fd].in_use = false;
  fd_table[fd].node = NULL;
  fd_table[fd].position = 0;
}

// Read from a file
int vfs_read(int fd, void *buffer, uint32_t size) {
  if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS)
    return -1;
  if (!fd_table[fd].in_use)
    return -1;

  vfs_node_t *node = fd_table[fd].node;
  if (!node || !node->read)
    return -1;

  uint32_t bytes_read =
      node->read(node, fd_table[fd].position, size, (uint8_t *)buffer);
  fd_table[fd].position += bytes_read;

  return bytes_read;
}

// Write to a file
int vfs_write(int fd, const void *buffer, uint32_t size) {
  if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS)
    return -1;
  if (!fd_table[fd].in_use)
    return -1;

  vfs_node_t *node = fd_table[fd].node;
  if (!node || !node->write)
    return -1;

  uint32_t bytes_written =
      node->write(node, fd_table[fd].position, size, (uint8_t *)buffer);
  fd_table[fd].position += bytes_written;

  return bytes_written;
}

// Seek in a file
int vfs_seek(int fd, int offset, int whence) {
  if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS)
    return -1;
  if (!fd_table[fd].in_use)
    return -1;

  vfs_node_t *node = fd_table[fd].node;
  if (!node)
    return -1;

  uint32_t new_pos = 0;

  switch (whence) {
  case SEEK_SET:
    new_pos = offset;
    break;
  case SEEK_CUR:
    new_pos = fd_table[fd].position + offset;
    break;
  case SEEK_END:
    new_pos = node->length + offset;
    break;
  default:
    return -1;
  }

  fd_table[fd].position = new_pos;
  return new_pos;
}

// Open directory
vfs_node_t *vfs_opendir(const char *path) { return vfs_resolve_path(path); }

// Read directory entry
dirent_t *vfs_readdir(vfs_node_t *node, uint32_t index) {
  if (!node || !(node->flags & VFS_DIRECTORY))
    return NULL;
  if (!node->readdir)
    return NULL;

  return node->readdir(node, index);
}

// Find directory entry by name
vfs_node_t *vfs_finddir(vfs_node_t *node, char *name) {
  if (!node || !(node->flags & VFS_DIRECTORY))
    return NULL;
  if (!node->finddir)
    return NULL;

  return node->finddir(node, name);
}

// Get file stats
int vfs_stat(const char *path, vfs_node_t *stat_buf) {
  vfs_node_t *node = vfs_resolve_path(path);
  if (!node)
    return -1;

  // Copy node information to stat buffer
  *stat_buf = *node;
  return 0;
}

// Create directory (stub for now)
int vfs_mkdir(const char *path) {
  // TODO: Implement directory creation
  return -1;
}

// Create file (stub for now)
int vfs_create(const char *path) {
  // TODO: Implement file creation
  return -1;
}

// Delete file (stub for now)
int vfs_unlink(const char *path) {
  // TODO: Implement file deletion
  return -1;
}
