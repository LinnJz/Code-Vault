#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <iomanip>
#include <limits>
#include <ppl.h>
#include <tbb/parallel_sort.h>

#include "algorithm/radix_sort.hpp"

template<typename T>
std::vector<T> generate_random(std::size_t n)
{
  std::vector<T>     data(n);
  std::random_device rd;
  std::mt19937_64    gen(rd());

  if constexpr (std::is_signed_v<T>) {
    using wide_type = int64_t;
    std::uniform_int_distribution<wide_type> dist(
        static_cast<wide_type>(std::numeric_limits<T>::min()),
        static_cast<wide_type>(std::numeric_limits<T>::max()));
    for (auto& x : data)
      x = static_cast<T>(dist(gen));
  } else {
    using wide_type = uint64_t;
    std::uniform_int_distribution<wide_type> dist(
        0, static_cast<wide_type>(std::numeric_limits<T>::max()));
    for (auto& x : data)
      x = static_cast<T>(dist(gen));
  }
  return data;
}

// 计时模板：对每种算法拷贝数据并排序，返回耗时（秒）
template<typename T, typename SortFunc>
double time_sort(const std::vector<T>& source, SortFunc sort_func)
{
  std::vector<T> work  = source;  // 拷贝独立数据
  auto           start = std::chrono::high_resolution_clock::now();
  sort_func(work);
  auto                          end  = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;
  return diff.count();
}

// 测试一种数据类型，多种数据规模
template<typename T>
void run_tests(const std::string&              type_name,
               const std::vector<std::size_t>& sizes, int repeat = 3)
{
  std::cout << "\n========== " << type_name << " ==========\n";
  std::cout << std::left << std::setw(12) << "Size" << std::setw(15)
            << "MS PPL (s)" << std::setw(15) << "TBB (s)" << std::setw(15)
            << "stdex (s)\n";

  for (std::size_t n : sizes) {
    // 生成原始数据
    auto original = generate_random<T>(n);

    double ms_time = 0.0, tbb_time = 0.0, stdex_time = 0.0;

    for (int r = 0; r < repeat; ++r) {
      // 微软 PPL parallel_radixsort
      ms_time += time_sort(original, [](std::vector<T>& vec) {
        concurrency::parallel_radixsort(vec.begin(), vec.end());
      });

      // TBB parallel_sort
      tbb_time += time_sort(original, [](std::vector<T>& vec) {
        tbb::parallel_sort(vec.begin(), vec.end());
      });

      // 自定义 stdex radix_sort (使用并行策略)
      stdex_time += time_sort(original, [](std::vector<T>& vec) {
        stdex::radix_sort(std::execution::par_unseq, vec.begin(), vec.end());
      });
    }

    // 输出平均值
    std::cout << std::left << std::setw(12) << n << std::fixed
              << std::setprecision(6) << std::setw(15) << (ms_time / repeat)
              << std::setw(15) << (tbb_time / repeat) << std::setw(15)
              << (stdex_time / repeat) << "\n";
  }
}

//int main()
//{
//  using namespace std::literals;
//  std::ios_base::sync_with_stdio(false);
//  std::cout.setf(std::ios_base::boolalpha);
//
//  // 定义测试规模（可根据需要调整）
//  std::vector<std::size_t> sizes = { 1'000'000, 10'000'000, 50'000'000,
//                                     100'000'000 };
//
//  // 测试四种整数类型
//  run_tests<uint32_t>("uint32", sizes);
//  run_tests<int32_t>("int32", sizes);
//  run_tests<uint64_t>("uint64", sizes);
//  run_tests<int64_t>("int64", sizes);
//
//  return 0;
//}
