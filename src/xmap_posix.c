#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)

#include "xmap/xmap.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif

struct xmap_t {
  void *data;
  size_t size;
  int fd;
};

#if defined(_MSC_VER)
__declspec(thread) static char tls_error_buf[256] = "No error";
#else
static __thread char tls_error_buf[256] = "No error";
#endif

XMAP_API const char *xmap_last_error(void) {
  return tls_error_buf;
}

static __thread int tls_sys_error = 0;

static void set_last_error(const char *msg) {
  tls_sys_error = errno;
  snprintf(tls_error_buf, sizeof(tls_error_buf), "%s (System Code: %d)", msg, tls_sys_error);
}

XMAP_API int xmap_last_system_error(void) {
  return tls_sys_error;
}

xmap_t *xmap_open(const char *filepath, xmap_mode_t mode) {
  return xmap_open_ext(filepath, mode, XMAP_FLAG_NONE);
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
  int flags = async ? MS_ASYNC : MS_SYNC;
  return msync(map->data, map->size, flags) == 0;
}

xmap_t *xmap_open_shared(const char *name, size_t size, xmap_mode_t mode, xmap_ipc_flags_t flags) {
  if (!name || size == 0) {
    return NULL;
  }

  int oflags = (mode == XMAP_READ_WRITE) ? O_RDWR : O_RDONLY;
  bool needs_truncate = false;

  if (flags & XMAP_IPC_CREATE_IF_MISSING) {
    oflags |= (O_CREAT | O_EXCL);
    needs_truncate = true;
  }

  int fd = shm_open(name, oflags, 0666);

  if (fd == -1) {
    if (errno == EEXIST && (flags & XMAP_IPC_CREATE_IF_MISSING)) {
      fd = shm_open(name, (mode == XMAP_READ_WRITE) ? O_RDWR : O_RDONLY, 0666);
      needs_truncate = false;
    } else {
      set_last_error("shm_open failed due to POSIX permissions existence mismatch");
      return NULL;
    }
  }

  if (fd == -1) {
    return NULL;
  }

  if (flags & XMAP_IPC_OPEN_EXISTING) {
    struct stat sb;
    if (fstat(fd, &sb) == 0) {
      if ((size_t)sb.st_size < size) {
        set_last_error("Requested IPC segment size is larger than the existing segment.");
        close(fd);
        return NULL;
      }
      size = sb.st_size;
    }
  }

  if (needs_truncate && mode == XMAP_READ_WRITE) {
    if (ftruncate(fd, size) == -1) {
      set_last_error("ftruncate failed on shared memory segment.");
      close(fd);
      return NULL;
    }
  }

  int prot_flags = PROT_READ;
  if (mode == XMAP_READ_WRITE) {
    prot_flags |= PROT_WRITE;
  }

  void *data = mmap(NULL, size, prot_flags, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    set_last_error("mmap failed for shared memory.");
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

xmap_t *xmap_open_ext(const char *filepath, xmap_mode_t mode, xmap_flags_t flags) {
  if (!filepath)
    return NULL;

  int open_flags = (mode == XMAP_READ_WRITE) ? O_RDWR : O_RDONLY;
  int fd = open(filepath, open_flags);
  if (fd == -1)
    return NULL;

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    close(fd);
    return NULL;
  }

  size_t size = sb.st_size;
  if (size == 0) {
    close(fd);
    return NULL;
  }

  int prot_flags = PROT_READ;
  if (mode == XMAP_READ_WRITE)
    prot_flags |= PROT_WRITE;

  int mmap_flags = MAP_SHARED;

  if (flags & XMAP_FLAG_HUGE_PAGES) {
#ifdef MAP_HUGETLB
    mmap_flags |= MAP_HUGETLB;
#else
    set_last_error(
        "Warning: HugePages requested but not supported by OS. Falling back to standard pages.");
#endif
  }

  if (flags & XMAP_FLAG_POPULATE) {
#ifdef __linux__
    mmap_flags |= MAP_POPULATE;
#endif
  }

  void *data = mmap(NULL, size, prot_flags, mmap_flags, fd, 0);
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

#endif
