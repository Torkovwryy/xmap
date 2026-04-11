cmake_minimum_required(VERSION 3.21)
project(xmap VERSION 1.0.0 LANGUAGES C CXX)

# --- Options ---
option(XMAP_BUILD_SHARED_LIBS "Build xmap as a shared library" OFF)
option(XMAP_BUILD_TESTS "Build xmap test suite" ON)
option(XMAP_BUILD_BENCHMARKS "Build xmap benchmark suite" OFF)
option(XMAP_BUILD_PYTHON "Build Python bindings via pybind11" OFF)

# --- Compiler Standards ---
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- RPATH configuration ---
if (WIN32)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
endif ()

# --- Target definition ---
if (XMAP_BUILD_SHARED_LIBS)
    add_library(xmap SHARED)
    target_compile_features(xmap PUBLIC c_std_11)
    target_compile_definitions(xmap PRIVATE XMAP_BUILD_SHARED)
else ()
    add_library(xmap STATIC)
    target_compile_features(xmap PUBLIC c_std_11)
endif ()

set_target_properties(xmap PROPERTIES POSITION_INDEPENDENT_CODE ON)

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

# --- Doxygen Custom Target ---
find_package(Doxygen)
if (DOXYGEN_FOUND)
    add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
    )
endif ()

# --- Installation Rules ---
include(GNUInstallDirs)

install(TARGETS xmap
        EXPORT xmapTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/xmap DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT xmapTargets
        FILE xmapTargets.cmake
        NAMESPACE xmap::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xmap
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "xmapConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xmapConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xmap
)

# --- Python Bindings ---
if (XMAP_BUILD_PYTHON)
    add_subdirectory(python)
endif ()