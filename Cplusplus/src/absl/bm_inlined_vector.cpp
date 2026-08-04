#include <algorithm>
#include <benchmark/benchmark.h>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#if defined(_WIN32)

#  define hybrid_scoped_malloca(size) _malloca(size)
#  define hybrid_scoped_freea(ptr)    _freea(ptr)
#endif
// 引入 Abseil InlinedVector
#include "absl/container/inlined_vector.h"

// ========== hybrid_scoped_malloca 宏定义 (Linux/macOS 版本) ==========
#ifndef _WIN32

#  define HYBRID_SCOPED_ALLOCA_THRESHOLD    1024
#  define HYBRID_SCOPED_ALLOCA_STACK_MARKER 0xCC'CC
#  define HYBRID_SCOPED_ALLOCA_HEAP_MARKER  0xDD'DD

#  if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
#    define HYBRID_SCOPED_ALLOCA_MARKER_SIZE 16
#  else
#    define HYBRID_SCOPED_ALLOCA_MARKER_SIZE 8
#  endif

inline void*
linn_hybrid_scoped_alloca_mark(void* ptr, unsigned int marker) noexcept
{
  if (ptr)
  {
    *static_cast<unsigned int*>(ptr) = marker;
    ptr                              = static_cast<char*>(ptr) + HYBRID_SCOPED_ALLOCA_MARKER_SIZE;
  }
  return ptr;
}

inline size_t
linn_hybrid_scoped_alloca_compute_size(size_t size)
{
  size_t marked_size = size + HYBRID_SCOPED_ALLOCA_MARKER_SIZE;
  return marked_size > size ? marked_size : 0;
}

inline void
linn_hybrid_scoped_freea(void* ptr)
{
  if (!ptr)
    return;
  void*        base   = static_cast<char*>(ptr) - HYBRID_SCOPED_ALLOCA_MARKER_SIZE;
  unsigned int marker = *static_cast<unsigned int*>(base);
  if (marker == HYBRID_SCOPED_ALLOCA_HEAP_MARKER)
  {
    free(base);
  }
#  ifdef assert
  else if (marker != HYBRID_SCOPED_ALLOCA_STACK_MARKER)
  {
    assert(("Corrupted pointer passed to _freea" && 0));
  }
#  endif
}

#  if defined(DEBUG) || defined(_DEBUG)
#    define hybrid_scoped_malloca(size)                                                                                \
      (::linn_hybrid_scoped_alloca_compute_size(size) != 0                                                             \
           ? ::linn_hybrid_scoped_alloca_mark(::malloc(::linn_hybrid_scoped_alloca_compute_size(size)),                \
                                              HYBRID_SCOPED_ALLOCA_HEAP_MARKER)                                        \
           : nullptr)
#  else
#    define hybrid_scoped_malloca(size)                                                                                \
      (::linn_hybrid_scoped_alloca_compute_size(size) != 0                                                             \
           ? ((::linn_hybrid_scoped_alloca_compute_size(size) <= HYBRID_SCOPED_ALLOCA_THRESHOLD)                       \
                  ? ::linn_hybrid_scoped_alloca_mark(::alloca(::linn_hybrid_scoped_alloca_compute_size(size)),         \
                                                     HYBRID_SCOPED_ALLOCA_STACK_MARKER)                                \
                  : ::linn_hybrid_scoped_alloca_mark(::malloc(::linn_hybrid_scoped_alloca_compute_size(size)),         \
                                                     HYBRID_SCOPED_ALLOCA_HEAP_MARKER))                                \
           : nullptr)
#  endif

#  define hybrid_scoped_freea(ptr) linn_hybrid_scoped_freea(ptr)

#endif // !_WIN32

// ========== 核心测试逻辑（独立于计时） ==========
// 为避免模板膨胀，将算法部分提取为函数模板，用于两种分配方式
template<typename Container>
int
RunAlgorithm(Container& container, int n, unsigned seed)
{
  // 确保容器已预分配大小
  container.reserve(n);

  std::mt19937 rng(seed);
  for (int i = 0; i < n; ++i)
  {
    container.push_back(static_cast<int>(rng()));
  }

  std::sort(container.begin(), container.end());
  std::reverse(container.begin(), container.end());

  std::int64_t sum = 0;
  for (int v : container)
    sum += v;

  for (int i = 0; i < n / 2; ++i)
  {
    container[i] ^= container[n - 1 - i];
  }
  std::sort(container.begin(), container.end());

  int result = 0;
  for (int i = 0; i < n; ++i)
  {
    result ^= (container[i] * (i + 1));
  }
  result ^= static_cast<int>(sum & 0xff'ff'ff'ff);
  return result;
}

// hybrid 版本包装
int
test_hybrid(int n, unsigned seed)
{
  void* ptr = hybrid_scoped_malloca(n * sizeof(int));
  if (!ptr)
    return -1;
  int* arr = static_cast<int*>(ptr);

  // 包装成类似容器的接口
  struct ArrayWrapper
  {
    int* data;
    int  size;

    void reserve(int) { }

    void push_back(int v) { data[size++] = v; }

    int* begin() { return data; }

    int* end() { return data + size; }

    int& operator[] (int i) { return data[i]; }
  } wrapper { arr, 0 };

  int result = RunAlgorithm(wrapper, n, seed);
  hybrid_scoped_freea(arr);
  return result;
}

// InlinedVector 版本包装
int
test_inlined(int n, unsigned seed)
{
  absl::InlinedVector<int, 8> vec;
  return RunAlgorithm(vec, n, seed);
}

// ========== Google Benchmark 测试函数 ==========
static void
BM_Hybrid(benchmark::State& state)
{
  int      n    = state.range(0);
  unsigned seed = 12345; // 固定种子，保证确定性
  for (auto _ : state)
  {
    int result = test_hybrid(n, seed);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void
BM_Inlined(benchmark::State& state)
{
  int      n    = state.range(0);
  unsigned seed = 12345;
  for (auto _ : state)
  {
    int result = test_inlined(n, seed);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

// 注册不同大小的测试（阈值内、阈值附近、较大值）
BENCHMARK(BM_Hybrid)->Arg(100)->Arg(256)->Arg(512)->Arg(1000)->Arg(10000);
BENCHMARK(BM_Inlined)->Arg(100)->Arg(256)->Arg(512)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();
