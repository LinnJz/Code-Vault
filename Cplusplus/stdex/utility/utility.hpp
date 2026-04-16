#pragma once

#include <cmath>
#include <array>
#include <type_traits>

namespace stdex {

  
template<size_t Begin, size_t End, size_t Factor, typename Func>
void unroll_for(Func&& func)
{
  static_assert(Factor > 0 && (Factor & (Factor - 1)) == 0,
                "N must be a power of two");
  static_assert(End >= Begin, "End must >= Begin");

  constexpr size_t total      = End - Begin;
  constexpr size_t main_iters = total / Factor;
  constexpr size_t remainder  = total % Factor;

  auto step = [&](size_t base) {
    [&]<size_t... Is>(std::index_sequence<Is...>) {
      (func(base + Is), ...);
    }(std::make_index_sequence<Factor>());
  };

  for (size_t i = 0; i < main_iters; ++i) {
    step(Begin + i * Factor);
  }
  for (size_t i = 0; i < remainder; ++i) {
    func(Begin + main_iters * Factor + i);
  }
}

template<size_t Factor, typename Func>
void unroll_for(size_t begin, size_t end, Func&& func)
{
  static_assert(Factor > 0 && (Factor & (Factor - 1)) == 0,
                "N must be a power of two");
  const auto [main_iters, remainder] = ::lldiv(end - begin, Factor);

  auto step = [&](size_t base) {
    [&]<size_t... Is>(std::index_sequence<Is...>) {
      (func(base + Is), ...);
    }(std::make_index_sequence<Factor>());
  };

  for (size_t i = 0; i < (size_t)main_iters; ++i) {
    step(begin + i * Factor);
  }
  for (size_t i = 0; i < (size_t)remainder; ++i) {
    func(begin + (size_t)main_iters * Factor + i);
  }
}

template<typename T, std::size_t N, std::size_t... Is>
constexpr auto make_array_impl(T&& value, std::index_sequence<Is...>)
{
  // remove cvref from T
  // 逗号运算符：返回最后一个表达式的结果（std::forward<T>(value)）
  return std::array<std::decay_t<T>, N> { (static_cast<void>(Is),
                                           std::forward<T>(value))... };
  /*
   return std::array<std::decay_t<T>, N>{
      (void)Is...,  // 展开并丢弃所有Is
      std::forward<T>(value)  // 最后一个值用作初始化
  };
  // like this:
  return std::array<int, 3>{
      (static_cast<void>(0), std::forward<T>(value)), // 类似宏的"挤兑"
      (static_cast<void>(1), std::forward<T>(value)),
      (static_cast<void>(2), std::forward<T>(value))
  };
  */
}

// 第二个版本：重复参数包直到填充N个元素
template<typename... Args, std::size_t... Is>
constexpr auto make_array_repeat_impl(std::index_sequence<Is...>,
                                      Args&&... args)
{
  constexpr std::size_t M = sizeof...(Args);
  using T                 = std::common_type_t<Args...>;

  // 对于每个Is，选择args中的第(Is % M)个参数
  return std::array<T, sizeof...(Is)> { [&] {
    // 展开参数包到数组中，便于索引访问
    T temp [] = { static_cast<T>(std::forward<Args>(args))... };
    return temp [Is % M];
  }()... };
}

template<typename T, std::size_t N>
[[nodiscard]] constexpr auto make_array(T&& value)
{
  return make_array_impl<T, N>(std::forward<T>(value),
                               std::make_index_sequence<N>());
}

template<std::size_t N, class... Args>
[[nodiscard]] constexpr auto make_array(Args&&... args)
{
  static_assert(N > 0, "N must be greater than 0");
  static_assert(sizeof...(Args) > 0, "At least one argument is required");

  return make_array_repeat_impl(std::make_index_sequence<N>(),
                                std::forward<Args>(args)...);
}

template<typename T>
[[nodiscard]] constexpr T& as_mutable(T const& t) noexcept
{
  return const_cast<T&>(t);
}

template<typename T>
requires (!std::is_lvalue_reference_v<T>)
[[nodiscard]] constexpr T& as_mutable(T&& t) noexcept
{
  return const_cast<T&>(t);
}

template<typename Class, typename Ty>
struct strong_type
{
protected:

  Ty value_;

public:

  explicit strong_type(Ty value) noexcept : value_(value) { }

  explicit operator Ty() const noexcept { return value_; }
};

// 先进的boost::hash_combine 的方法，可适用于tuple、array
template<class... Ts>
constexpr std::size_t hash_combine(Ts const&... ts)
{
  std::size_t h = 0;
  ((h ^= std::hash<Ts>()(ts) + 0x9e'37'79'b9 + (h << 6) + (h >> 2)), ...);
  return h;
}

template<class... Ts>
struct std::hash<std::tuple<Ts...>>
{
  size_t operator() (std::tuple<Ts...> const& x) const
  {
    return std::apply(hash_combine<Ts...>, x);
  }
};

template<class T, size_t N>
struct std::hash<std::array<T, N>>
{
  size_t operator() (const std::array<T, N>& arr) const
  {
    std::hash<T> hasher;
    size_t       seed = 0;

    for (const auto& elem : arr) {
      seed ^= hasher(elem) + 0x9e'37'79'b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

template<std::floating_point T>
inline bool compare_with_nan_at_end(T a, T b)
{
  if (bool a_is_nan = std::isnan(a), b_is_nan = std::isnan(b);
      a_is_nan && b_is_nan) [[unlikely]]
  {
    return false;  // 所有 NaN 相等，排序时相对位置不变
  } else if (a_is_nan) [[unlikely]] {
    return false;  // a 是 NaN，b 不是 → a 应该在后面，返回 false
  } else if (b_is_nan) [[unlikely]] {
    return true;  // b 是 NaN，a 不是 → a 应该在前面，返回 true
  }

  return a < b;
}

template<std::floating_point T>
inline bool compare_with_nan_at_start(T a, T b)
{
  if (bool a_is_nan = std::isnan(a), b_is_nan = std::isnan(b);
      a_is_nan && b_is_nan) [[unlikely]]
  {
    return false;
  } else if (a_is_nan) [[unlikely]] {
    return true;
  } else if (b_is_nan) [[unlikely]] {
    return false;
  }
  return a < b;
}
}  // namespace stdex

/*
// Usage Examples:

#include <iostream>
#include <string>
#include <unordered_map>

int main() {
    using namespace stdex;

    // Example 1: make_array - Create an array filled with the same value
    auto arr1 = make_array<int, 5>(42);
    std::cout << "make_array: ";
    for (auto val : arr1) std::cout << val << " ";
    std::cout << "\n";

    // Example 2: as_mutable - Remove const qualifier
    const std::string text = "hello";
    as_mutable(text)[0] = 'H';
    std::cout << "as_mutable: " << text << "\n";

    // Example 3: strong_type - Type-safe wrapper
    using UserId = strong_type<class UserIdTag, int>;
    using ProductId = strong_type<class ProductIdTag, int>;

    UserId user(123);
    ProductId product(456);

    // These won't compile - type safety enforced:
    // user = product;  // Error: different types
    // UserId user2 = product;  // Error: different types

    int userId = static_cast<int>(user);
    std::cout << "strong_type: " << userId << "\n";

    // Example 4: hash_combine - Combine multiple hash values
    size_t h1 = hash_combine(1, 2, 3);
    std::cout << "hash_combine(1,2,3): " << h1 << "\n";

    // Example 5: Use hash_combine with std::unordered_map for tuple keys
    std::unordered_map<std::tuple<int, int, int>, std::string> tuple_map;

    tuple_map[{1, 2, 3}] = "first";
    tuple_map[{4, 5, 6}] = "second";

    std::cout << "tuple map[{1,2,3}]: " << tuple_map[{1, 2, 3}] << "\n";

    // Example 6: Use hash_combine with std::unordered_map for array keys
    std::unordered_map<std::array<int, 3>, int, std::hash<std::array<int, 3>>>
array_map;

    array_map[{1, 2, 3}] = 100;
    array_map[{4, 5, 6}] = 200;

    std::cout << "array map[{1,2,3}]: " << array_map[{1, 2, 3}] << "\n";

    return 0;
}
*/