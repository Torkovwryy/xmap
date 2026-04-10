/**
 * @file xmap.hpp
 * @brief Modern C++20 wrapper for libxmap.
 */
#ifndef XMAP_HPP
#define XMAP_HPP

#include "xmap.h"
#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace xmap {

enum class Mode { ReadOnly = XMAP_READ_ONLY, ReadWrite = XMAP_READ_WRITE };

enum class Flags : uint32_t {
  None = XMAP_FLAG_NONE,
  HugePages = XMAP_FLAG_HUGE_PAGES,
  Populate = XMAP_FLAG_POPULATE
};

inline Flags operator|(Flags a, Flags b) {
  return static_cast<Flags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class IpcFlags : uint32_t {
  CreateIfMissing = XMAP_IPC_CREATE_IF_MISSING,
  OpenExisting = XMAP_IPC_OPEN_EXISTING
};

inline IpcFlags operator|(IpcFlags a, IpcFlags b) {
  return static_cast<IpcFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

template <Mode M = Mode::ReadWrite> class MemoryMap {
private:
  xmap_t *handle_ = nullptr;

  explicit MemoryMap(xmap_t *h) noexcept : handle_(h) {
  }

public:
  // Delete copy semantics to ensure exclusive ownership
  MemoryMap(const MemoryMap &) = delete;
  MemoryMap &operator=(const MemoryMap &) = delete;

  // Move semantics
  MemoryMap(MemoryMap &&other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {
  }

  /**
   * @brief Constructs a MemoryMap object. Throws on failure.
   */
  explicit MemoryMap(const std::filesystem::path &filepath, Flags flags = Flags::None) {
    handle_ = xmap_open_ext(filepath.string().c_str(), static_cast<xmap_mode_t>(M),
                            static_cast<xmap_flags_t>(flags));
    if (handle_ == nullptr) {
      throw std::runtime_error(std::string("Failed to map file: ") + xmap_last_error());
    }
  }

  ~MemoryMap() {
    close();
  }

  void close() noexcept {
    if (handle_ != nullptr) {
      xmap_close(handle_);
      handle_ = nullptr;
    }
  }

  // Factory Function No-Throw
  [[nodiscard]] static std::optional<MemoryMap<M>> create(const std::filesystem::path &filepath,
                                                          Flags flags = Flags::None) noexcept {
    xmap_t *h = xmap_open_ext(filepath.string().c_str(), static_cast<xmap_mode_t>(M),
                              static_cast<xmap_flags_t>(flags));
    if (h == nullptr) {
      return std::nullopt;
    }
    return MemoryMap<M>(h);
  }

  /**
   * @brief Returns a typed std::span representing the memory.
   * @param T Type to cast the memory to (defaults to std::byte).
   */
  template <typename T = std::byte>
  [[nodiscard]] std::span<T> data()
    requires(M == Mode::ReadWrite)
  {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Type mapped to memory must be trivially copyable.");
    if (!handle_) {
      return {};
    }
    size_t bytes = xmap_size(handle_);
    if (bytes % sizeof(T) != 0) {
      throw std::runtime_error("Mapping size is not perfectly aligned with type size.");
    }
    return std::span<T>(static_cast<T *>(xmap_data(handle_)), bytes / sizeof(T));
  }

  template <typename T = std::byte> [[nodiscard]] std::span<const T> data() const {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Type mapped to memory must be trivially copyable.");
    if (!handle_) {
      return {};
    }
    return std::span<const T>(static_cast<const T *>(xmap_data(handle_)),
                              xmap_size(handle_) / sizeof(T));
  }

  [[nodiscard]] size_t size() const noexcept {
    return (handle_ != nullptr) ? xmap_size(handle_) : 0;
  }
  [[nodiscard]] bool is_valid() const noexcept {
    return handle_ != nullptr;
  }

  bool flush(bool async = false) noexcept {
    return (handle_ != nullptr) ? xmap_flush(handle_, async) : false;
  }
};

class SharedMemory {
private:
  xmap_t *handle_ = nullptr;
  std::string name_;

public:
  SharedMemory(const SharedMemory &) = delete;
  SharedMemory &operator=(const SharedMemory &) = delete;

  SharedMemory(SharedMemory &&other) noexcept
      : handle_(std::exchange(other.handle_, nullptr)), name_(std::move(other.name_)) {
  }

  /**
   * @brief Creates or connects to a named shared memory segment.
   */
  SharedMemory(std::string_view name, size_t size, Mode mode = Mode::ReadWrite,
               IpcFlags flags = IpcFlags::CreateIfMissing)
      : name_(name) {
    handle_ = xmap_open_shared(name_.c_str(), size, static_cast<xmap_mode_t>(mode),
                               static_cast<xmap_ipc_flags_t>(flags));

    if (handle_ == nullptr) {
      throw std::runtime_error(std::string("Failed to create/open shared memory: ") +
                               xmap_last_error());
    }
  }

  ~SharedMemory() {
    if (handle_ != nullptr) {
      xmap_close(handle_);
    }
  }

  template <typename T = std::byte> std::span<T> data() {
    if (!handle_)
      return {};
    return std::span<T>(static_cast<T *>(xmap_data(handle_)), xmap_size(handle_) / sizeof(T));
  }

  template <typename T = std::byte> std::span<const T> data() const {
    if (!handle_)
      return {};
    return std::span<const T>(static_cast<const T *>(xmap_data(handle_)),
                              xmap_size(handle_) / sizeof(T));
  }

  size_t size() const noexcept {
    return (handle_ != nullptr) ? xmap_size(handle_) : 0;
  }

  bool is_valid() const noexcept {
    return handle_ != nullptr;
  }

  /**
   * @brief Unlinks the shared memory segment from the OS.
   */
  static bool unlink(std::string name) noexcept {
    return xmap_unlink_shared(std::string(std::move(name)).c_str());
  }
};

} // namespace xmap

#endif // XMAP_HPP
