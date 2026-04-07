#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)

#include "xmap/xmap.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct xmap_t {
  void *data;
  size_t size;
  int fd;
};

xmap_t *xmap_open(const char *filepath, xmap_mode_t mode) {
  if (!filepath) {
    return NULL;
  }

  int open_flags = (mode == XMAP_READ_WRITE) ? O_RDWR : O_RDONLY;
  int fd = open(filepath, open_flags);
  if (fd == -1) {
    return NULL;
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    close(fd);
    return NULL;
  }

  size_t size = sb.st_size;
  if (size == 0) {
    close(fd);
    return NULL; // Can't map empty file securely across all OS
  }

  int prot_flags = PROT_READ;
  if (mode == XMAP_READ_WRITE) {
    prot_flags |= PROT_WRITE;
  }

  void *data = mmap(NULL, size, prot_flags, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    close(fd);
    return NULL;
  }

  xmap_t *map = (xmap_t *)malloc(sizeof(xmap_t));
  if (!map) {
    munmap(data, size);
    close(fd);
    return NULL;
  }

  map->data = data;
  map->size = size;
  map->fd = fd;
  return map;
}

void xmap_close(xmap_t *map) {
  if (map) {
    if (map->data) {
      munmap(map->data, map->size);
    }
    if (map->fd != -1) {
      close(map->fd);
    }
    free(map);
  }
}

void *xmap_data(const xmap_t *map) {
  return map ? map->data : NULL;
}

size_t xmap_size(const xmap_t *map) {
  return map ? map->size : 0;
}

bool xmap_flush(xmap_t *map, bool async) {
  if (!map || !map->data) {
    return false;
  }
  int flags = (int)async ? MS_ASYNC : MS_ASYNC;
  return msync(map->data, map->size, flags) == 0;
}

#endif
