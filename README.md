# libxmap 🗺️

[![CI Multi-platform](https://github.com/Torkovwryy/xmap/actions/workflows/ci.yml/badge.svg)](https://github.com/Torkovwryy/xmap/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

**Zero-dependency, ABI-stable, cross-platform memory-mapped I/O with modern C++20 bindings.**

Memory-mapped I/O is the fastest way to read/write large files, perform fast IPC, and build storage engines. However,
the C++ standard lacks native `mmap` abstractions, and existing solutions (like Boost) are heavy and inflate compile
times.

`libxmap` solves this by providing bare-metal OS performance via a pure C11 core (guaranteeing ABI stability) wrapped in
a zero-cost, type-safe C++20 interface using `std::span`.

## Features

* 🚀 **Zero Dependencies:** No Boost, no external libraries.
* 🛡️ **ABI Stable:** Core API is pure C. Safe to use across dynamic library boundaries.
* 💻 **Cross-Platform:** Native implementations for Linux, macOS (POSIX), and Windows (Win32).
* ✨ **Modern C++20 Wrapper:** RAII semantics, move-only types, and `std::span` for bounds-safe memory access.

## Performance

`libxmap` is rigorously benchmarked against the standard C++ library (`std::fstream`) using [Google Benchmark](https://gihub/com/google/benchmark). By utilizing OS-level page cache and zero-copy reads, it achieves orders of magnitude faster access times.

*Metrics for 100MB payload (Linux, btrfs):*
* **Sequential Write:** `libxmap` is ~3% faster by avoiding user-space buffer copies (leveraging `MAP_POPULATE` pre-faulting).
* **Sequential Read:** `libxmap` is **~2600x faster** (5.88 TiB/s vs 2.26 GiB/s) because it returns a direct kernel pointer (Zero-Copy) rather than copying data into a user-space buffer.

## Quick Start

### Using the C++20 API (Recommended)

```cpp
#include <xmap/xmap.hpp>
#include <iostream>

int main() {
    try {
        // Map file in ReadWrite mode
        xmap::MemoryMap map("database.bin", xmap::Mode::ReadWrite);
        
        // Access data securely via std::span<std::byte>
        auto data = map.data<char>();
        
        // Mutate memory directly
        data[0] = 'X';
        
        // Flush changes to disk asynchronously
        map.flush(true); 

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
    return 0; // RAII automatically unmaps and closes the file
}
```

### Using the pure C11 API

```c
#include <xmap/xmap.h>
#include <stdio.h>

int main() {
    xmap_t* map = xmap_open("database.bin", XMAP_READ_WRITE);
    if (!map) {
        fprintf(stderr, "Failed to mapfile.\n");
        return 1;
    }
    
    char* data = (char*)xmap_data(map);
    data[0] = 'X';
    
    xmap_flash(map, false); // Synchronous flush
    xmap_close(map);
    
    return 0;
}
```

## Integration (CMake)

You can easily integrate lbxmap unsing CMake's FetchContent. It will build from source alongside your project.

```cmake
include(FetchContent)

FetchContent_Declare(
        xmap
        GIT_REPOSITORY https://github.com/Torkovwryy/xmap.git
        GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(xmap)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE xmap::xmap)
```

## Roadmap

* [x] POSIX/Windows core implementation.
* [x] C++20 RAII Wrapper.
* [X] Add support for HugePages (Linux) / Large Pages (Windows).
* [X] Add IPC Primitives (Named Shared Memory).

## License

MIT License. See [license](LICENSE) for details.
