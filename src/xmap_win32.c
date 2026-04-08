#if defined(_WIN32)

#include "xmap/xmap.h"
#include <windows.h>

static bool win32_enable_large_page_privilege(void) {
  HANDLE hToken;
  TOKEN_PRIVILEGES tp;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
    return false;
  }
  if (!LookupPrivilegeValueA(NULL, "SeLockMemoryPrivilege", &tp.Privileges[0].Luid)) {
    CloseHandle(hToken);
    return false;
  }
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  bool result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
  CloseHandle(hToken);
  return result && (GetLastError() == ERROR_SUCCESS);
}

struct xmap_t {
  void *data;
  size_t size;
  HANDLE file_handle;
  HANDLE map_handle;
};

xmap_t *xmap_open(const char *filepath, xmap_mode_t mode) {
  return xmap_open_ext(filepath, mode, XMAP_FLAG_NONE);
}

void xmap_close(xmap_t *map) {
  if (map) {
    if (map->data)
      UnmapViewOfFile(map->data);
    if (map->map_handle)
      CloseHandle(map->map_handle);
    if (map->file_handle && map->file_handle != INVALID_HANDLE_VALUE)
      CloseHandle(map->file_handle);
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
  if (!map || !map->data)
    return false;
  // Windows FlushViewOfFile is inherently synchronous regarding starting the I/O,
  // but we execute it regardless.
  if (!FlushViewOfFile(map->data, 0))
    return false;
  if (!async) {
    FlushFileBuffers(map->file_handle);
  }
  return true;
}

xmap_t *xmap_open_shared(const char *name, size_t size, xmap_mode_t mode) {
  if (!name || size == 0)
    return NULL;

  DWORD protect = (mode == XMAP_READ_WRITE) ? PAGE_READWRITE : PAGE_READONLY;

  HANDLE map_handle =
      CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, protect, (DWORD)((size >> 32) & 0xFFFFFFFF),
                         (DWORD)(size & 0xFFFFFFFF), name);

  if (map_handle == NULL)
    return NULL;

  DWORD map_access = (mode == XMAP_READ_WRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
  void *data = MapViewOfFile(map_handle, map_access, 0, 0, size);
  if (data == NULL) {
    CloseHandle(map_handle);
    return NULL;
  }

  xmap_t *map = (xmap_t *)malloc(sizeof(xmap_t));
  if (!map) {
    UnmapViewOfFile(data);
    CloseHandle(map_handle);
    return NULL;
  }

  map->data = data;
  map->size = size;
  map->file_handle = INVALID_HANDLE_VALUE;
  map->map_handle = map_handle;
  return map;
}

xmap_t *xmap_open_ext(const char *filepath, xmap_mode_t mode, xmap_flags_t flags) {
  if (!filepath)
    return NULL;

  DWORD access = (mode == XMAP_READ_WRITE) ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;
  DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;

  HANDLE file_handle =
      CreateFileA(filepath, access, share, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file_handle == INVALID_HANDLE_VALUE)
    return NULL;

  LARGE_INTEGER file_size;
  if (!GetFileSizeEx(file_handle, &file_size) || file_size.QuadPart == 0) {
    CloseHandle(file_handle);
    return NULL;
  }

  DWORD protect = (mode == XMAP_READ_WRITE) ? PAGE_READWRITE : PAGE_READONLY;

  if (flags & XMAP_FLAG_HUGE_PAGES) {
    if (win32_enable_large_page_privilege()) {
      protect |= SEC_LARGE_PAGES;
    } else {
      CloseHandle(file_handle);
      return NULL;
    }
  }

  HANDLE map_handle = CreateFileMappingA(file_handle, NULL, protect, 0, 0, NULL);
  if (map_handle == NULL) {
    CloseHandle(file_handle);
    return NULL;
  }

  DWORD map_access = (mode == XMAP_READ_WRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
  void *data = MapViewOfFile(map_handle, map_access, 0, 0, 0);
  if (data == NULL) {
    CloseHandle(map_handle);
    CloseHandle(file_handle);
    return NULL;
  }

  xmap_t *map = (xmap_t *)malloc(sizeof(xmap_t));
  if (!map) {
    UnmapViewOfFile(data);
    CloseHandle(map_handle);
    CloseHandle(file_handle);
    return NULL;
  }

  map->data = data;
  map->size = (size_t)file_size.QuadPart;
  map->file_handle = file_handle;
  map->map_handle = map_handle;
  return map;
}

bool xmap_unlink_shared(const char *name) {
  (void)name;
  return true;
}

#endif
