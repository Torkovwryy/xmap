/**
 * @file xmap.h
 * @brief C-API for cross-platform memory mapped files.
 */
#ifndef XMAP_H
#define XMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef XMAP_BUILD_SHARED
#define XMAP_API __declspec(dllexport)
#elif defined(XMAP_USE_SHARED)
#define XMAP_API __declspec(dllimport)
#else
#define XMAP_API
#endif
#else
#if __GNUC__ >= 4
#define XMAP_API __attribute__((visibility("default")))
#else
#define XMAP_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Access mode for the memory map.
 */
typedef enum { XMAP_READ_ONLY = 0, XMAP_READ_WRITE = 1 } xmap_mode_t;

/**
 * @brief Opaque handle representing a memory mapped file.
 */
typedef struct xmap_t xmap_t;

/**
 * @brief Opens and maps a file into memory.
 * * @param filepath The path to the file.
 * @param mode Access mode (XMAP_READ_ONLY or XMAP_READ_WRITE).
 * @return xmap_t* A valid handle on success, NULL on failure.
 */
XMAP_API xmap_t *xmap_open(const char *filepath, xmap_mode_t mode);

/**
 * @brief Unmaps the memory and closes the file handle.
 * * @param map The memory map handle to close.
 */
XMAP_API void xmap_close(xmap_t *map);

/**
 * @brief Gets the pointer to the mapped memory.
 * * @param map The memory map handle.
 * @return void* Pointer to the mapped memory, or NULL if invalid.
 */
XMAP_API void *xmap_data(const xmap_t *map);

/**
 * @brief Gets the size of the mapped memory in bytes.
 * * @param map The memory map handle.
 * @return size_t Size of the mapping in bytes.
 */
XMAP_API size_t xmap_size(const xmap_t *map);

/**
 * @brief Flushes modifications back to the disk.
 * * @param map The memory map handle.
 * @param async If true, schedules the flush and returns immediately.
 * @return true on success, false on failure.
 */
XMAP_API bool xmap_flush(xmap_t *map, bool async);

/**
 * @brief Creates or opens a named shared memory segment for Inter-Process Communication (IPC).
 * * @param name The unique for the shared memory (e.g., "/my_shared_mem").
 * @param size The size of the memory segment in bytes.
 * @param mode Access mode (XMAP_READ_ONLY or XMAP_READ_WRITE).
 * @return xmap_t* A valid handle on success, NULL on failure.
 */
XMAP_API xmap_t *xmap_open_shared(const char *name, size_t size, xmap_mode_t mode);

/**
 * @brief Removes a named shared memory segment from the system.
 * @param name The unique name of the shared memory.
 * @return true on success, false on failure.
 */
XMAP_API bool xmap_unlink_shared(const char *name);
#ifdef __cplusplus
}
#endif

#endif
