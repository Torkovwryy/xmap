#include "xmap/xmap.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <string>

class XMapIPCTest : public ::testing::Test {
protected:
  const std::string shm_name = "/xmap_hft_tick_buffer_test";
  const size_t shm_size = 4096;

  void SetUp() override {
    xmap::SharedMemory::unlink(shm_name);
  }

  void TearDown() override {
    xmap::SharedMemory::unlink(shm_name);
  }
};

TEST_F(XMapIPCTest, ZeroCopyCommunicationBetweenInstances) {
  xmap::SharedMemory process_a(shm_name, shm_size, xmap::Mode::ReadWrite);
  ASSERT_TRUE(process_a.is_valid());

  auto data_a = process_a.data<char>();

  std::string tick_data = "AAPL:150.25|MSFT:305.10";
  std::ranges::copy(tick_data, data_a.begin());
  data_a[tick_data.size()] = '\0';

  xmap::SharedMemory process_b(shm_name, shm_size, xmap::Mode::ReadWrite);
  ASSERT_TRUE(process_b.is_valid());

  auto data_b = process_b.data<char>();

  std::string received_data(data_b.data(), tick_data.size());
  EXPECT_EQ(received_data, tick_data);

  data_b[0] = 'X';

  EXPECT_EQ(data_a[0], 'X');
}

TEST_F(XMapIPCTest, UnlinkDestroysMemoryAccessibility) {
  {
    xmap::SharedMemory mem(shm_name, shm_size, xmap::Mode::ReadWrite);
    ASSERT_TRUE(mem.is_valid());
  }
  EXPECT_TRUE(xmap::SharedMemory::unlink(shm_name));
}