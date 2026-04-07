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
  explicit MemoryMap(std::string_view filepath, Mode mode = Mode::ReadOnly) {
    handle_ = xmap_open(filepath.data(), static_cast<xmap_mode_t>(mode));
    if (!handle_) {
      throw std::runtime_error("Failed to map file to memory: " + std::string(filepath));
    }
  }

  ~MemoryMap() {
    close();
  }

  void close() noexcept {
    if (handle_) {
      xmap_close(handle_);
      handle_ = nullptr;
    }
  }

  /**
   * @brief Returns a typed std::span representing the memory.
   * @tparam T Type to cast the memory to (defaults to std::byte).
   */
  template <typename T = std::byte> std::span<T> data() const {
    if (!handle_)
      return {};
    return std::span<T>(static_cast<T *>(xmap_data(handle_)), xmap_size(handle_) / sizeof(T));
  }

  size_t size() const noexcept {
    return handle_ ? xmap_size(handle_) : 0;
  }

  bool flush(bool async = false) noexcept {
    return handle_ ? xmap_flush(handle_, async) : false;
  }

  bool is_valid() const noexcept {
    return handle_ != nullptr;
  }
};

} // namespace xmap

#endif // XMAP_HPP
