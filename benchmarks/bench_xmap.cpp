#include "xmap/xmap.hpp"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

constexpr size_t DATA_SIZE = 100 * 1024 * 1024; // 100 MB

class IOBenchmark : public benchmark::Fixture {
public:
  const std::string bench_file = "bench_data.bin";
  std::vector<char> buffer;

  void SetUp(const ::benchmark::State &state) {
    if (buffer.empty()) {
      buffer.resize(DATA_SIZE, 'A');
    }
    std::ofstream ofs(bench_file, std::ios::binary);
    ofs.write(buffer.data(), DATA_SIZE);
    ofs.close();
  }

  void TearDown(const ::benchmark::State &state) {
    if (fs::exists(bench_file)) {
      fs::remove(bench_file);
    }
  }
};

BENCHMARK_F(IOBenchmark, FStream_Write_100MB)(benchmark::State &state) {
  for (auto _ : state) {
    xmap::MemoryMap map(bench_file, xmap::Mode::ReadWrite, xmap::Flags::Populate);

    auto data = map.data<char>();

    std::ranges::copy(buffer, data.begin());

    (void)map.flush(false);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * DATA_SIZE);
}

BENCHMARK_F(IOBenchmark, LibXMap_Write_100MB)(benchmark::State &state) {
  for (auto _ : state) {
    xmap::MemoryMap map(bench_file, xmap::Mode::ReadWrite);
    auto data = map.data<char>();

    std::copy(buffer.begin(), buffer.end(), data.begin());

    map.flush(false);

    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * DATA_SIZE);
}

BENCHMARK_F(IOBenchmark, FStream_Read_100MB)(benchmark::State &state) {
  std::vector<char> out_buffer(DATA_SIZE);
  for (auto _ : state) {
    std::ifstream ifs(bench_file, std::ios::binary);
    ifs.read(out_buffer.data(), DATA_SIZE);
    ifs.close();

    benchmark::DoNotOptimize(out_buffer.data());
  }
  state.SetBytesProcessed(state.iterations() * DATA_SIZE);
}

BENCHMARK_F(IOBenchmark, LibXMap_Read_100MB)(benchmark::State &state) {
  for (auto _ : state) {
    xmap::MemoryMap map(bench_file, xmap::Mode::ReadOnly);
    auto data = map.data<const char>();

    char first = data.front();
    char last = data.back();

    benchmark::DoNotOptimize(first);
    benchmark::DoNotOptimize(last);
    benchmark::DoNotOptimize(data.data());
  }
  state.SetBytesProcessed(state.iterations() * DATA_SIZE);
}