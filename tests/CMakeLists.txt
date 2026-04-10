include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest
        GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(xmap_test test_xmap.cpp test_ipc.cpp)

target_compile_features(xmap_test PRIVATE cxx_std_20)
target_link_libraries(xmap_test PRIVATE xmap::xmap GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(xmap_test)