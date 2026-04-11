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

class PySharedMemory {
private:
  xmap_t *handle_ = nullptr;
  xmap::Mode mode_;

public:
  PySharedMemory(const std::string &name, size_t size, xmap::Mode mode, xmap::IpcFlags flags)
      : mode_(mode) {
    handle_ = xmap_open_shared(name.c_str(), size, static_cast<xmap_mode_t>(mode),
                               static_cast<xmap_ipc_flags_t>(flags));
    if (!handle_) {
      throw std::runtime_error(std::string("Failed to open shared memory: ") + xmap_last_error());
    }
  }

  ~PySharedMemory() {
    close();
  }

  void close() {
    if (handle_) {
      xmap_close(handle_);
      handle_ = nullptr;
    }
  }

  size_t size() const {
    return handle_ ? xmap_size(handle_) : 0;
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
  static py::exception<std::runtime_error> xmap_error(m, "XMapError");

  py::register_exception_translator([](std::exception_ptr p) {
    try {
      if (p)
        std::rethrow_exception(p);
    } catch (const std::runtime_error &e) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
    }
  });

  m.doc() = "Zero-dependency, ABI-stable, cross-platform memory-mapped I/O.";

  py::enum_<xmap::Mode>(m, "Mode")
      .value("ReadOnly", xmap::Mode::ReadOnly)
      .value("ReadWrite", xmap::Mode::ReadWrite)
      .export_values();

  py::enum_<xmap::IpcFlags>(m, "IpcFlags", py::arithmetic())
      .value("CreateIfMissing", xmap::IpcFlags::CreateIfMissing)
      .value("OpenExisting", xmap::IpcFlags::OpenExisting)
      .export_values();

  py::class_<PyMemoryMap>(m, "MemoryMap", py::buffer_protocol())
      .def(py::init<const std::string &, xmap::Mode, xmap::Flags>(), py::arg("filepath"),
           py::arg("mode") = xmap::Mode::ReadOnly, py::arg("flags") = xmap::Flags::None)
      .def_buffer([](PyMemoryMap &m) -> py::buffer_info {
        if (!m.is_valid()) {
          throw std::runtime_error("Cannot access data: Memory map is closed or invalid.");
        }

        return py::buffer_info(m.raw_data(), sizeof(uint8_t),
                               py::format_descriptor<uint8_t>::format(), 1, {m.size()},
                               {sizeof(uint8_t)}, m.mode() == xmap::Mode::ReadOnly);
      })
      .def("close", &PyMemoryMap::close, "Unmaps the memory and releases OS resources.")
      .def("flush", &PyMemoryMap::flush, py::arg("async_flush") = false,
           "Flushes memory changes to the physical disk")
      .def_property_readonly("size", &PyMemoryMap::size,
                             "Gets the total size of the mapping in bytes.")
      .def_property_readonly("is_valid", &PyMemoryMap::is_valid, "Checks if the mapping is active.")
      .def("__enter__",
           [](PyMemoryMap &self) -> PyMemoryMap & {
             if (!self.is_valid()) {
               throw std::runtime_error("MemoryMap is invalid.");
             }
             return self;
           })
      .def("__exit__",
           [](PyMemoryMap &self, py::object exc_type, py::object exc_value, py::object traceback) {
             self.close();
             return false;
           })
      .def("__repr__", [](const PyMemoryMap &m) {
        return "<xmap.MemoryMap valid=" + std::string(m.is_valid() ? "True" : "False") +
               " size=" + std::to_string(m.size()) + " bytes";
      });

  py::class_<PySharedMemory>(m, "SharedMemory", py::buffer_protocol())
      .def(py::init<const std::string &, size_t, xmap::Mode, xmap::IpcFlags>(), py::arg("name"),
           py::arg("size"), py::arg("mode") = xmap::Mode::ReadWrite,
           py::arg("flags") = xmap::IpcFlags::CreateIfMissing)

      .def_buffer([](PySharedMemory &m) -> py::buffer_info {
        if (!m.is_valid())
          throw std::runtime_error("SharedMemory is invalid.");
        return py::buffer_info(m.raw_data(), sizeof(uint8_t),
                               py::format_descriptor<uint8_t>::format(), 1, {m.size()},
                               {sizeof(uint8_t)}, m.mode() == xmap::Mode::ReadOnly);
      })
      .def("close", &PySharedMemory::close)
      .def_property_readonly("size", &PySharedMemory::size)
      .def_property_readonly("is_valid", &PySharedMemory::is_valid)
      .def("__enter__", [](PySharedMemory &self) -> PySharedMemory & { return self; })
      .def("__exit__",
           [](PySharedMemory &self, py::object, py::object, py::object) {
             self.close();
             return false;
           })
      .def_static(
          "unlink", [](const std::string &name) { return xmap::SharedMemory::unlink(name); },
          py::arg("name"), "Removes the shared memory segment from the OS.")
      .def("__repr__", [](const PySharedMemory &m) {
        return "<xmap.SharedMemory valid=" + std::string(m.is_valid() ? "True" : "False") +
               " size=" + std::to_string(m.size()) + " bytes>";
      });
}