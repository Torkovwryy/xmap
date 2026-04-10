#include "xmap/xmap.h"
#include "xmap/xmap.hpp"
#include <pybind11/pybind11.h>

namespace py = pybind11;

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