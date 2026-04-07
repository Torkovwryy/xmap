include(FetchContent)

# Baixa a versão estável mais recente do GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Para Windows/MSVC: Força o uso da C-Runtime correta para evitar conflitos de linkagem
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Define o executável de testes
add_executable(xmap_test test_xmap.cpp)

# Linka nossa biblioteca e o GTest
target_link_libraries(xmap_test PRIVATE xmap::xmap GTest::gtest_main)

# Permite que o CTest descubra os testes automaticamente
include(GoogleTest)
gtest_discover_tests(xmap_test)