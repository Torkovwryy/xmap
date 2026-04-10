#include "xmap/xmap.h"
#include "xmap/xmap.hpp"
#include <pybind11/pybind11.h>

namespace py = pybind11;

class PyMemoryMap {
private:
  xmap_t *handle_ = nullptr;
  xmap::Mode mode_;

public:
  PyMemoryMap(const std::string &filepath, xmap::Mode mode, xmap::Flags flags) : mode_(mode) {
    handle_ = xmap_open_ext(filepath.c_str(), static_cast<xmap_mode_t>(mode),
                            static_cast<xmap_flags_t>(flags));
    if (handle_) {
      throw std::runtime_error(std::string("Failed to map file: ") + xmap_last_error());
    }
  }

  ~PyMemoryMap() {
    close();
  }
 void close() {
    if (handle_ != nullptr) {
      xmap_close(handle_);
      handle_ = nullptr;
    }
  }

  size_t size() const {
    return handle_ ? xmap_size(handle_) : 0;
  }
  bool flush(bool async) {
    return handle_ ? xmap_flush(handle_, async) : false;
  }
  bool is_valid() const {
    return handle_ != nullptr;
  }

  void *raw_data() const {
    return handle_ ? xmap_data(handle_) : nullptr;
  }
  xmap::Mode mode() const {
    return mode_;
  }
};

PYBIND11_MODULE(xmap_ext, m) {
  m.doc() = "Zero-dependency, ABI-stable, cross-platform memory-mapped I/O.";

  py::enum_<xmap::Mode>(m, "Mode")
      .value("ReadOnly", xmap::Mode::ReadOnly)
      .value("ReadWrite", xmap::Mode::ReadWrite)
      .export_values();

  py::enum_<xmap::IpcFlags>(m, "IpcFlags", py::arithmetic())
      .value("CreateIfMissing", xmap::IpcFlags::CreateIfMissing)
      .value("OpenExisting", xmap::IpcFlags::OpenExisting)
      .export_values();
}