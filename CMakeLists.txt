cmake_minimum_required(VERSION 3.21)
project(xmap VERSION 1.0.0 LANGUAGES C CXX)

# --- Options ---
option(XMAP_BUILD_SHARED_LIBS "Build xmap as a shared library" OFF)
option(XMAP_BUILD_TESTS "Build xmap test suite" ON)
option(XMAP_BUILD_BENCHMARKS "Build xmap benchmark suite" OFF)

# --- Compiler Standards ---
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Target definition ---
if (XMAP_BUILD_SHARED_LIBS)
    add_library(xmap SHARED)
    target_compile_definitions(xmap PRIVATE XMAP_BUILD_SHARED)
else ()
    add_library(xmap STATIC)
endif ()

add_library(xmap::xmap ALIAS xmap)

# --- Platform specific sources ---
if (WIN32)
    target_sources(xmap PRIVATE src/xmap_win32.c)
elseif (UNIX)
    target_sources(xmap PRIVATE src/xmap_posix.c)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(xmap PRIVATE rt)
    endif ()
endif ()


# --- Include directories ---
target_include_directories(xmap
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# --- Sanitizers (Debug Only)
option(XMAP_ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)

if (XMAP_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "libxmap: Enabling Address and Undefined Behavior Sanitizers")

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    elseif (MSVC)
        add_compile_options(/fsanitize=address)
    endif ()
endif ()

# --- Strict compiler warnings ---
if (MSVC)
    target_compile_options(xmap PRIVATE /W4 /WX)
else ()
    target_compile_options(xmap PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif ()

# --- Tests ---
if (XMAP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

# --- Benchmarks ---
if (XMAP_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif ()