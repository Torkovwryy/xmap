include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark
        GIT_TAG v1.9.5
)

FetchContent_MakeAvailable(googlebenchmark)
add_executable(xmap_bench bench_xmap.cpp)

target_link_libraries(xmap_bench PRIVATE xmap::xmap benchmark::benchmark benchmark::benchmark_main)

if (MSVC)
    target_compile_options(xmap_bench PRIVATE /O2)
else ()
    target_compile_options(xmap_bench PRIVATE -O3)
endif ()