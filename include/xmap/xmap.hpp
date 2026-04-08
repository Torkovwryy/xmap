/**
 * @file xmap.hpp
 * @brief Modern C++20 wrapper for libxmap.
 */
#ifndef XMAP_HPP
#define XMAP_HPP

#include "xmap.h"
#include <cstddef>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace xmap {

enum class Mode { ReadOnly = XMAP_READ_ONLY, ReadWrite = XMAP_READ_WRITE };
enum class Flags : uint32_t { None = XMAP_FLAG_NONE, HugePages = XMAP_FLAG_HUGE_PAGES };

inline Flags operator|(Flags a, Flags b) {
  return static_cast<Flags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

class MemoryMap {
private:
  xmap_t *handle_ = nullptr;

public:
  // Delete copy semantics to ensure exclusive ownership
  MemoryMap(const MemoryMap &) = delete;
  MemoryMap &operator=(const MemoryMap &) = delete;

  // Move semantics
  MemoryMap(MemoryMap &&other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {
  }
  MemoryMap &operator=(MemoryMap &&other) noexcept {
    if (this != &other) {
      close();
      handle_ = std::exchange(other.handle_, nullptr);
    }
    return *this;
  }

  /**
   * @brief Constructs a MemoryMap object. Throws on failure.
   */
  explicit MemoryMap(std::string_view filepath, Mode mode = Mode::ReadOnly,
                     Flags flags = Flags::None) {
    handle_ = xmap_open_ext(filepath.data(), static_cast<xmap_mode_t>(mode),
                            static_cast<xmap_flags_t>(flags));
    if (handle_ == nullptr) {
      throw std::runtime_error(
          "Failed to map file to memory. Check path, permissions or HugePage OS availability : " +
          std::string(filepath));
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

  /**
   * @brief Returns a typed std::span representing the memory.
   * @param T Type to cast the memory to (defaults to std::byte).
   */
  template <typename T = std::byte> std::span<T> data() const {
    if (!handle_) {
      return {};
    }
    return std::span<T>(static_cast<T *>(xmap_data(handle_)), xmap_size(handle_) / sizeof(T));
  }

  [[nodiscard]] size_t size() const noexcept {
    return (handle_ != nullptr) ? xmap_size(handle_) : 0;
  }

  [[nodiscard]] bool flush(bool async = false) noexcept {
    return (handle_ != nullptr) ? xmap_flush(handle_, async) : false;
  }

  [[nodiscard]] bool is_valid() const noexcept {
    return handle_ != nullptr;
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
   * @brief Creates or connects to a name shared memory segment.
   */
  SharedMemory(std::string_view name, size_t size, Mode mode = Mode::ReadWrite) : name_(name) {
    handle_ = xmap_open_shared(name_.c_str(), size, static_cast<xmap_mode_t>(mode));
    if (handle_ == nullptr) {
      throw new std::runtime_error("Failed to create/open shared memory: " + name_);
    }
  }

  ~SharedMemory() {
    if (handle_ != nullptr) {
      xmap_close(handle_);
    }
  }

  template <typename T = std::byte> std::span<T> data() const {
    if (!handle_) {
      return {};
    }
    return std::span<T>(static_cast<T *>(xmap_data(handle_)), xmap_size(handle_) / sizeof(T));
  }
  [[nodiscard]] size_t size() const noexcept {
    return (handle_ != nullptr) ? xmap_size(handle_) : 0;
  }

  [[nodiscard]] bool is_valid() const noexcept {
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
