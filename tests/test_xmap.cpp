#include "xmap/xmap.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

namespace fs = std::filesystem;

class XMapTest : public ::testing::Test {
protected:
  const std::string test_file = "test_data.bin";

  void SetUp() override {
    std::ofstream ofs(test_file, std::ios::binary);
    ofs << "HELLO XMAP";
    ofs.close();
  }

  void TearDown() override {
    if (fs::exists(test_file)) {
      fs::remove(test_file);
    }
  }
};

TEST_F(XMapTest, MapAndReadSuccess) {
  xmap::MemoryMap<xmap::Mode::ReadOnly> map(test_file);

  ASSERT_TRUE(map.is_valid());
  EXPECT_EQ(map.size(), 10);

  auto data = map.data<const char>();
  std::string_view content(data.data(), data.size());

  EXPECT_EQ(content, "HELLO XMAP");
}

TEST_F(XMapTest, MapAndWriteSuccess) {
  {
    xmap::MemoryMap<xmap::Mode::ReadWrite> map(test_file);
    ASSERT_TRUE(map.is_valid());

    auto data = map.data<char>();
    data[0] = 'Y';

    EXPECT_TRUE(map.flush());
  }

  std::ifstream ifs(test_file, std::ios::binary);
  std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

  EXPECT_EQ(content, "YELLO XMAP");
}

TEST_F(XMapTest, MapInvalidFileFails) {
  EXPECT_THROW(
      { xmap::MemoryMap<xmap::Mode::ReadOnly> map("arquivo_inexistente_123.bin"); },
      std::runtime_error);
}

TEST_F(XMapTest, HugePagesFallbackTest) {
  try {
    xmap::MemoryMap<xmap::Mode::ReadOnly> map(test_file, xmap::Flags::HugePages);
    ASSERT_TRUE(map.is_valid());
    std::cout << "[  INFO  ] HugePages are natively supported and active on this system!\n";
  } catch (const std::runtime_error &) {
    std::cout << "[  SKIP  ] HugePages OS configuration not preset. Skipping test gracefully.\n";
    SUCCEED();
  }
}