include(FetchContent)

FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.13.6
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(xmap_ext src/xmap_py.cpp)

set_target_properties(xmap_ext PROPERTIES OUTPUT_NAME "xmap_ext")

target_link_libraries(xmap_ext PRIVATE xmap::xmap)

target_compile_features(xmap_ext PRIVATE cxx_std_20)

install(TARGETS xmap_ext DESTINATION .)

install(FILES xmap.py xmap_ext.pyi DESTINATION .)