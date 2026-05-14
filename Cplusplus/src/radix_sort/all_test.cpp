#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <span>
#include <vector>

#include "algorithm/radix_sort.hpp"

using namespace std;

template<typename T>
bool
verify_sorted(span<const T> data, stdex::radix_traits::order_type order, stdex::radix_traits::nan_position nanpos)
{
  if constexpr (is_floating_point_v<T>)
  {
    // 分离 NaN 和非 NaN
    vector<size_t> nan_idx, num_idx;
    for (size_t i = 0; i < data.size(); ++i)
    {
      if (isnan(data[i]))
        nan_idx.push_back(i);
      else
        num_idx.push_back(i);
    }
    // 检查 NaN 位置
    if (nanpos == stdex::radix_traits::nan_position::at_begin)
    {
      if (!nan_idx.empty() && (nan_idx.front() != 0 || nan_idx.back() != nan_idx.size() - 1))
        return false;
    }
    else if (nanpos == stdex::radix_traits::nan_position::at_end)
    {
      if (!nan_idx.empty() && (nan_idx.front() != data.size() - nan_idx.size() || nan_idx.back() != data.size() - 1))
        return false;
    }
    // 检查非 NaN 部分排序
    if (num_idx.size() >= 2)
    {
      if (order == stdex::radix_traits::order_type::asc)
      {
        for (size_t i = 1; i < num_idx.size(); ++i)
          if (data[num_idx[i]] < data[num_idx[i - 1]])
            return false;
      }
      else
      {
        for (size_t i = 1; i < num_idx.size(); ++i)
          if (data[num_idx[i - 1]] < data[num_idx[i]])
            return false;
      }
    }
    return true;
  }
  else
  {
    // 整数：直接使用 is_sorted
    if (order == stdex::radix_traits::order_type::asc)
      return is_sorted(data.begin(), data.end());
    else
      return is_sorted(data.begin(), data.end(), greater<> {});
  }
}

// ========== 数据生成 ==========
random_device rd;
mt19937_64    gen(rd());

template<typename T>
vector<T>
generate_integer_data(size_t n)
{
  vector<T> v(n);
  if constexpr (is_same_v<T, uint32_t> || is_same_v<T, uint64_t>)
  {
    uniform_int_distribution<T> dist(numeric_limits<T>::min(), numeric_limits<T>::max());
    for (auto &x : v)
      x = dist(gen);
  }
  else
  {
    uniform_int_distribution<long long> dist(numeric_limits<T>::min(), numeric_limits<T>::max());
    for (auto &x : v)
      x = static_cast<T>(dist(gen));
  }
  return v;
}

template<typename T>
vector<T>
generate_float_data(size_t n)
{
  vector<T>                    v(n);
  uniform_real_distribution<T> dist(numeric_limits<T>::lowest() / 2, numeric_limits<T>::max() / 2);
  for (auto &x : v)
  {
    int r = uniform_int_distribution<int>(0, 12)(gen);
    if (r == 0)
      x = numeric_limits<T>::quiet_NaN();
    else if (r == 1)
      x = -numeric_limits<T>::quiet_NaN();
    else if (r == 2)
      x = numeric_limits<T>::infinity();
    else if (r == 3)
      x = -numeric_limits<T>::infinity();
    else if (r == 4)
      x = T(0.0);
    else if (r == 5)
      x = T(-0.0);
    else
      x = dist(gen);
  }
  // 确保至少有一个 NaN
  if (none_of(v.begin(), v.end(), [](T x)
  {
    return isnan(x);
  }))
    v.front() = numeric_limits<T>::quiet_NaN();
  return v;
}

// ========== 编译期 traits 常量 ==========
namespace TraitsList
{
using namespace stdex;
constexpr radix_traits lsd_asc { .sort_mode  = radix_traits::mode_type::lsd,
                                 .sort_order = radix_traits::order_type::asc };
constexpr radix_traits lsd_desc { .sort_mode  = radix_traits::mode_type::lsd,
                                  .sort_order = radix_traits::order_type::desc };
constexpr radix_traits msd_asc { .sort_mode  = radix_traits::mode_type::msd,
                                 .sort_order = radix_traits::order_type::asc };
constexpr radix_traits msd_desc { .sort_mode  = radix_traits::mode_type::msd,
                                  .sort_order = radix_traits::order_type::desc };

// 浮点专用（含nan位置）
constexpr radix_traits lsd_asc_nan_begin { .sort_mode  = radix_traits::mode_type::lsd,
                                           .sort_order = radix_traits::order_type::asc,
                                           .nan_pos    = radix_traits::nan_position::at_begin };
constexpr radix_traits lsd_asc_nan_end { .sort_mode  = radix_traits::mode_type::lsd,
                                         .sort_order = radix_traits::order_type::asc,
                                         .nan_pos    = radix_traits::nan_position::at_end };
constexpr radix_traits lsd_asc_nan_none { .sort_mode  = radix_traits::mode_type::lsd,
                                          .sort_order = radix_traits::order_type::asc,
                                          .nan_pos    = radix_traits::nan_position::unhandled };
constexpr radix_traits lsd_desc_nan_begin { .sort_mode  = radix_traits::mode_type::lsd,
                                            .sort_order = radix_traits::order_type::desc,
                                            .nan_pos    = radix_traits::nan_position::at_begin };
constexpr radix_traits lsd_desc_nan_end { .sort_mode  = radix_traits::mode_type::lsd,
                                          .sort_order = radix_traits::order_type::desc,
                                          .nan_pos    = radix_traits::nan_position::at_end };
constexpr radix_traits lsd_desc_nan_none { .sort_mode  = radix_traits::mode_type::lsd,
                                           .sort_order = radix_traits::order_type::desc,
                                           .nan_pos    = radix_traits::nan_position::unhandled };
constexpr radix_traits msd_asc_nan_begin { .sort_mode  = radix_traits::mode_type::msd,
                                           .sort_order = radix_traits::order_type::asc,
                                           .nan_pos    = radix_traits::nan_position::at_begin };
constexpr radix_traits msd_asc_nan_end { .sort_mode  = radix_traits::mode_type::msd,
                                         .sort_order = radix_traits::order_type::asc,
                                         .nan_pos    = radix_traits::nan_position::at_end };
constexpr radix_traits msd_asc_nan_none { .sort_mode  = radix_traits::mode_type::msd,
                                          .sort_order = radix_traits::order_type::asc,
                                          .nan_pos    = radix_traits::nan_position::unhandled };
constexpr radix_traits msd_desc_nan_begin { .sort_mode  = radix_traits::mode_type::msd,
                                            .sort_order = radix_traits::order_type::desc,
                                            .nan_pos    = radix_traits::nan_position::at_begin };
constexpr radix_traits msd_desc_nan_end { .sort_mode  = radix_traits::mode_type::msd,
                                          .sort_order = radix_traits::order_type::desc,
                                          .nan_pos    = radix_traits::nan_position::at_end };
constexpr radix_traits msd_desc_nan_none { .sort_mode  = radix_traits::mode_type::msd,
                                           .sort_order = radix_traits::order_type::desc,
                                           .nan_pos    = radix_traits::nan_position::unhandled };
} // namespace TraitsList

// ========== 测试执行模板 ==========
template<stdex::radix_traits Traits, typename T>
void
run_test(const string &type_name, const string &trait_desc)
{
  constexpr size_t N = 1'000'000;
  vector<T>        data;
  if constexpr (is_integral_v<T>)
    data = generate_integer_data<T>(N);
  else
    data = generate_float_data<T>(N);

  stdex::radix_sort<Traits>(execution::par, data.begin(), data.end());

  bool ok = verify_sorted<T>(data, Traits.sort_order, Traits.nan_pos);
  cout << left << setw(12) << type_name << " | " << setw(50) << trait_desc << " | " << (ok ? "PASS" : "FAIL") << endl;
}

#define TEST_INT(Traits, Type) run_test<Traits, Type>(#Type, #Traits)
#define TEST_FLT(Traits, Type) run_test<Traits, Type>(#Type, #Traits)

int
main()
{
  ios_base::sync_with_stdio(false);
  cout << boolalpha;
  cout << "Type         | Traits                                              | Result\n";
  cout << string(90, '-') << endl;

  using namespace TraitsList;

  // int32
  TEST_INT(lsd_asc, int32_t);
  TEST_INT(lsd_desc, int32_t);
  // uint32
  TEST_INT(lsd_asc, uint32_t);
  TEST_INT(lsd_desc, uint32_t);
  // int64
  TEST_INT(lsd_asc, int64_t);
  TEST_INT(lsd_desc, int64_t);
  // uint64
  TEST_INT(lsd_asc, uint64_t);
  TEST_INT(lsd_desc, uint64_t);

  // float
  TEST_FLT(lsd_asc_nan_begin, float);
  TEST_FLT(lsd_asc_nan_end, float);
  TEST_FLT(lsd_asc_nan_none, float);
  TEST_FLT(lsd_desc_nan_begin, float);
  TEST_FLT(lsd_desc_nan_end, float);
  TEST_FLT(lsd_desc_nan_none, float);
  // double
  TEST_FLT(lsd_asc_nan_begin, double);
  TEST_FLT(lsd_asc_nan_end, double);
  TEST_FLT(lsd_asc_nan_none, double);
  TEST_FLT(lsd_desc_nan_begin, double);
  TEST_FLT(lsd_desc_nan_end, double);
  TEST_FLT(lsd_desc_nan_none, double);

  TEST_INT(msd_asc, int32_t);
  TEST_INT(msd_desc, int32_t);
  TEST_INT(msd_asc, uint32_t);
  TEST_INT(msd_desc, uint32_t);
  TEST_INT(msd_asc, int64_t);
  TEST_INT(msd_desc, int64_t);
  TEST_INT(msd_asc, uint64_t);
  TEST_INT(msd_desc, uint64_t);
  TEST_FLT(msd_asc_nan_begin, float);
  TEST_FLT(msd_asc_nan_end, float);
  TEST_FLT(msd_asc_nan_none, float);
  TEST_FLT(msd_desc_nan_begin, float);
  TEST_FLT(msd_desc_nan_end, float);
  TEST_FLT(msd_desc_nan_none, float);
  TEST_FLT(msd_asc_nan_begin, double);
  TEST_FLT(msd_asc_nan_end, double);
  TEST_FLT(msd_asc_nan_none, double);
  TEST_FLT(msd_desc_nan_begin, double);
  TEST_FLT(msd_desc_nan_end, double);
  TEST_FLT(msd_desc_nan_none, double);

  cout << "\nAll tests completed.\n";
  return 0;
}
