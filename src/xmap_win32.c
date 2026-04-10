#ifdef _WIN32

#include "xmap/xmap.h"
#include <stdio.h>
#include <windows.h>

#ifdef _MSC_VER
__declspec(thread) static int tls_sys_error = 0;
__declspec(thread) static char tls_error_buf[512];
#else
static __thread int tls_sys_error = 0;
static __thread char tls_error_buf[512];
#endif

static void set_last_error(const char *msg) {
  tls_sys_error = GetLastError();
  snprintf(tls_error_buf, sizeof(tls_error_buf), "%s (System Code: %d)", msg, tls_sys_error);
}

XMAP_API int xmap_last_system_error(void) {
  return tls_sys_error;
}

XMAP_API bool xmap_init_large_page_privileges(void) {
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
    if (!FlushFileBuffers(map->file_handle))
      return false;
  }
  return true;
}

xmap_t *xmap_open_shared(const char *name, size_t size, xmap_mode_t mode, xmap_ipc_flags_t flags) {
  if (!name || size == 0)
    return NULL;

  HANDLE map_handle = NULL;
  DWORD map_access = (mode == XMAP_READ_WRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;

  if (flags & XMAP_IPC_CREATE_IF_MISSING) {
    DWORD protect = (mode == XMAP_READ_WRITE) ? PAGE_READWRITE : PAGE_READONLY;
    map_handle =
        CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, protect, (DWORD)((size >> 32) & 0xFFFFFFFF),
                           (DWORD)(size & 0xFFFFFFFF), name);
    if (map_handle == NULL) {
      set_last_error("CreateFileMappingA failed for shared memory");
      return NULL;
    }
  } else {
    map_handle = OpenFileMappingA(map_access, FALSE, name);
    if (map_handle == NULL) {
      set_last_error("Shared memory segment does not exist (OpenFileMappingA failed)");
      return NULL;
    }
  }

  void *data = MapViewOfFile(map_handle, map_access, 0, 0, size);
  if (data == NULL) {
    set_last_error("MapViewOfFile failed for shared memory");
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

static wchar_t *utf8_to_utf16(const char *utf8) {

  int size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
  if (size <= 0)
    return NULL;

  wchar_t *wstr = (wchar_t *)malloc(size * sizeof(wchar_t));
  if (!wstr)
    return NULL;

  if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, size) == 0) {
    free(wstr);
    return NULL;
  }

  return wstr;
}

xmap_t *xmap_open_ext(const char *filepath, xmap_mode_t mode, xmap_flags_t flags) {
  if (!filepath)
    return NULL;

  DWORD access = (mode == XMAP_READ_WRITE) ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;
  DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;

  wchar_t *wpath = utf8_to_utf16(filepath);
  if (wpath == NULL) {
    set_last_error("UTF-8 conversion failed");
    return NULL;
  }

  HANDLE file_handle =
      CreateFileW(wpath, access, share, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  free(wpath);
  if (file_handle == INVALID_HANDLE_VALUE) {
    set_last_error("CreateFileW failed");
    return NULL;
  }

  LARGE_INTEGER file_size;
  if (!GetFileSizeEx(file_handle, &file_size)) {
    set_last_error("GetFileSizeEx failed");
    CloseHandle(file_handle);
    return NULL;
  }

  if (file_size.QuadPart == 0) {
    set_last_error("File is empty");
    CloseHandle(file_handle);
    return NULL;
  }

  DWORD protect = (mode == XMAP_READ_WRITE) ? PAGE_READWRITE : PAGE_READONLY;

  if (flags & XMAP_FLAG_HUGE_PAGES) {
    const size_t HUGE_PAGE_SIZE = 2 * 1024 * 1024;

    if ((size_t)file_size.QuadPart % HUGE_PAGE_SIZE != 0) {
      set_last_error("File size is not aligned to huge page size");
      CloseHandle(file_handle);
      return NULL;
    }

    if (xmap_init_large_page_privileges()) {
      protect |= SEC_LARGE_PAGES;
    }
  }

  HANDLE map_handle = CreateFileMappingW(file_handle, NULL, protect, 0, 0, NULL);
  if (map_handle == NULL) {
    set_last_error("CreateFileMappingW failed");
    CloseHandle(file_handle);
    return NULL;
  }

  DWORD map_access = (mode == XMAP_READ_WRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
  void *data = MapViewOfFile(map_handle, map_access, 0, 0, 0);
  if (data == NULL) {
    set_last_error("MapViewOfFile failed");
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

XMAP_API const char *xmap_last_error(void) {
  return tls_error_buf;
}

#endif
