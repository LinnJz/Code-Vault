// ver 6.5.1
#include <glaze/glaze.hpp>

#include <any>
#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <complex>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <format>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <ranges>
#include <regex>
#include <set>
#include <shared_mutex>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <valarray>
#include <variant>
#include <vector>

// =================================================================
// 1. 定义所有自定义类型
// =================================================================
enum class Color
{
  RED,
  GREEN,
  BLUE
};

enum Direction
{
  UP,
  DOWN,
  LEFT,
  RIGHT
};

union UnionExample
{
  int   i;
  float f;
  char  c;
};

struct Point
{
  double x, y;
};

struct Container
{
  Point *ptr { nullptr };
};

class Base
{
public:
  Base(const std::string &str)
      : str(str)
  {
  }

  virtual ~Base() = default;
  std::string str;
};

class DerivedA : public Base
{
public:
  DerivedA(int num, const std::string &str)
      : Base(str)
      , num(num)
  {
  }

  int num;
};

struct DerivedB : Base
{
  DerivedB(double num, const std::string &str)
      : Base(str)
      , num(num)
  {
  }

  double num { 0.0 };
};

struct Red
{
  uint8_t r;
};

struct Green
{
  uint8_t g;
};

struct Blue
{
  uint8_t b;
};

struct RGB
    : public Red
    , Green
    , Blue
{
};

struct Person
{
  int         age;
  double      weight;
  std::string name;
};

class User
{
private:
  std::string name;
  int         age { 0 };

  // 关键：声明友元，允许 glz::meta 访问私有成员
  friend struct glz::meta<User>;

public:
  // 需要保留默认构造函数以支持反序列化
  User() = default;

  User(std::string n, int a)
      : name(std::move(n))
      , age(a)
  {
  }
};

class MyData
{
public:
  auto &get_value() { return value_; } // 非 const 引用 getter

private:
  double value_ { 0.0 };
};

struct Animal
{
  std::string species;
};

struct Flyable
{
  int wingspan { 0 };
};

// 多重继承的类
struct FlyingAnimal
    : public Animal
    , public Flyable
{
  int age { 0 };

  // 禁用 Glaze 对此结构体的自动反射
  static constexpr bool glaze_reflect = false;
};

// =================================================================
// 2. 为Glaze注册自定义类型的元信息
// =================================================================
// 注册枚举
template<>
struct glz::meta<Color>
{
  using enum Color;
  static constexpr auto value = enumerate("RED", RED, "GREEN", GREEN, "BLUE", BLUE);
};

template<>
struct glz::meta<Direction>
{
  using enum Direction;
  static constexpr auto value = enumerate("UP", UP, "DOWN", DOWN, "LEFT", LEFT, "RIGHT", RIGHT);
};

// 注册需要序列化的结构体和类
template<>
struct glz::meta<Point>
{
  static constexpr auto value = object(&Point::x, &Point::y);
};

template<>
struct glz::meta<Container>
{
  using T                     = Container;
  static constexpr auto value = glz::object("pointer_data", &T::ptr // 直接注册指针成员
  );
};

template<>
struct glz::meta<Base>
{
  static constexpr auto value = object(&Base::str);
};

template<>
struct glz::meta<DerivedA>
{
  static constexpr auto value = object(&DerivedA::num, &DerivedA::str);
};

template<>
struct glz::meta<DerivedB>
{
  static constexpr auto value = object(&DerivedB::num, &DerivedB::str);
};

// 不提供默认构造需要加上monostate
using MyPolymorphicType = std::variant<std::monostate, DerivedA, DerivedB>;

template<>
struct glz::meta<MyPolymorphicType>
{
  // 定义JSON中的标签字段名为 "type"
  static constexpr std::string_view tag = "type";
  // 定义与每个variant索引对应的标签字符串
  static constexpr auto ids = std::array { "null", "DerivedA", "DerivedB" };
};

template<>
struct glz::meta<Red>
{
  static constexpr auto value = object(&Red::r);
};

template<>
struct glz::meta<Green>
{
  static constexpr auto value = object(&Green::g);
};

template<>
struct glz::meta<Blue>
{
  static constexpr auto value = object(&Blue::b);
};

template<>
struct glz::meta<RGB>
{
  static constexpr auto value = object(&RGB::r, &RGB::g, &RGB::b);
};

template<>
struct glz::meta<Person>
{
  static constexpr auto value = object(&Person::age, &Person::weight, &Person::name);
};

template<>
struct glz::meta<User>
{
  using T                     = User;
  static constexpr auto value = glz::object("name",
                                            &T::name, // 直接访问私有成员
                                            "age",
                                            &T::age);
};

template<>
struct glz::meta<MyData>
{
  using T                     = MyData;
  static constexpr auto value = glz::object("value",
                                            [](T &obj) -> auto &
  {
    return obj.get_value();
  } // 通过 lambda 访问
  );
};

// 手动特化 glz::meta，集中注册所有需要序列化的成员
template<>
struct glz::meta<FlyingAnimal>
{
  using T                     = FlyingAnimal;
  static constexpr auto value = glz::object(
      // 从基类 Animal 继承的成员
      "species",
      &T::species,
      // 从基类 Flyable 继承的成员
      "wingspan",
      &T::wingspan,
      // 自身的成员
      "age",
      &T::age);
};

// =================================================================
// 3. 主函数
// =================================================================
int
main()
{
  // --- 3.1 基本类型 ---
  bool                 bool_type           = true;
  char                 char_type           = 'a';
  signed char          char_signed_type    = -128;
  unsigned char        char_unsigned_type  = 255;
  wchar_t              wchar_type          = L'a';  // wchar_t 序列化无标准映射，输出可能异常
  char8_t              char8_type          = u8'a'; // char8_t 输出支持取决于编译器和 Glaze 版本
  char16_t             char16_type         = u'a';  // char16_t 序列化未标准化
  char32_t             char32_type         = U'a';  // char32_t 序列化未标准化
  short                short_type          = -32768;
  unsigned short       ushort_type         = 65535;
  int                  int_type            = -2147483648;
  unsigned int         uint_type           = 4294967295;
  long                 long_type           = -9223372036854775807L;
  unsigned long        ulong_type          = 18446744073709551615UL;
  long long            long_long_type      = -9223372036854775807LL;
  unsigned long long   ulong_long_type     = 18446744073709551615ULL;
  float                float_type          = 3.14f;
  double               double_type         = 3.14;
  long double          long_double_type    = 3.14l; // long double 序列化精度可能丢失，部分编译器不支持
  std::complex<double> complex_double_type = { 1.0, 2.0 };

  // --- 3.2 字面量类型 ---
  std::nullptr_t nullptr_type   = nullptr;
  std::nullopt_t nullopt_type   = std::nullopt;
  std::nothrow_t nothrow_type   = std::nothrow; // std::nothrow_t 是空标签类型，无数据可序列化
  std::monostate monostate_type = std::monostate();

  // --- 3.3 引用和指针类型 (不支持序列化) ---
  // volatile std::size_t volatile_size_t_type = std::size_t();   // volatile 限定符在序列化中无意义
  std::size_t size_t_type = std::size_t();
  // std::size_t& size_t_ref_type = size_t_type;                  // 引用类型不能跨序列化边界保持，且无法表达
  // const std::size_t const_size_t_type = std::size_t();         // const 限定符无法被序列化体现
  // const std::size_t& const_size_t_ref_type = const_size_t_type;// 同引用类型
  // std::size_t&& size_t_rvalue_ref_type = std::size_t();        // 右值引用无法序列化
  // std::reference_wrapper<int> ref_wrapper_type = std::ref(int_type);// 内部存储指针，无法安全序列化

  // 原始指针 / void* 无法安全序列化（类型擦除、生命周期不确定）
  // void* void_type_ptr = nullptr;
  // const void* const_void_type_ptr = nullptr;
  // const void* const const_void_type_ptr_const = nullptr;

  // --- 3.4 数组 ---
  // 注意：C风格多维数组不支持自动反射，建议使用std::array
  std::array<char, 3> char_array_type = { 'a', 'b', 'c' };
  // int multi_array_type[2][3] = {{1, 2, 3}, {4, 5, 6}}; // Glaze 不支持原生多维数组，需转为 std::array<std::array<>>

  // --- 3.5 自定义类型 ---
  Color     color_type     = Color::RED;
  Direction direction_type = UP;
  // UnionExample union_type = { .i = 10 };                // 联合体无法安全序列化，仅能序列化活跃成员且需手动指定
  Point    point_type = { 1.0, 2.0 };
  Base     base_type("hello");
  DerivedA Derived_type(10, "world");
  // Base* Derived_ptr_type = &Derived_type;               // 裸指针指向基类，序列化会导致对象切片或悬空

  RGB rgb_type = { 255, 0, 0 };
  // RGB* rgb_type_ptr = &rgb_type;                        // 裸指针无法安全序列化
  // Red* red_type_ptr = static_cast<Red*>(rgb_type_ptr);   // 裸指针，同上
  // Green* green_type_ptr = static_cast<Green*>(rgb_type_ptr);
  // Blue* blue_type_ptr = static_cast<Blue*>(rgb_type_ptr);

  // --- 3.6 智能指针 ---
  auto rgb_unique_ptr_type = std::make_unique<RGB>();
  auto rgb_shared_ptr_type = std::make_shared<RGB>();
  // std::weak_ptr<RGB> rgb_weak_ptr_type = rgb_shared_ptr_type; // weak_ptr 无法直接序列化，需先 lock()

  // --- 3.7 同步原语 (不可序列化，纯运行时同步机制) ---
  // std::mutex mutex_type;
  // std::timed_mutex timed_mutex_type;
  // std::recursive_mutex recursive_mutex_type;
  // std::recursive_timed_mutex recursive_timed_mutex_type;
  // std::shared_mutex shared_mutex_type;
  // std::shared_timed_mutex shared_timed_mutex_type;

  // --- 3.8 锁类型 (基于同步原语，不可序列化) ---
  // std::lock_guard<std::mutex> lock_guard_type(mutex_type);
  // std::unique_lock<std::timed_mutex> unique_lock_type(timed_mutex_type);
  // std::shared_lock<std::shared_mutex> shared_lock_type(shared_mutex_type);

  // --- 3.9 条件变量 (不可序列化) ---
  // std::condition_variable condition_variable_type;
  // std::condition_variable_any condition_variable_any_type;

  // --- 3.10 原子类型 (不可序列化，代表并发状态) ---
  // std::atomic_bool atomic_bool_type = true;
  // std::atomic_char atomic_char_type = 'a';
  // std::atomic_flag atomic_flag_type = ATOMIC_FLAG_INIT;

  // --- 3.11 其他标准库类型 ---
  std::byte                  byte_type          = std::byte(0);
  std::array<std::byte, 3>   byte_array_type    = { std::byte(0), std::byte(1), std::byte(2) };
  std::initializer_list<int> init_list_int_type = { 1, 2, 3 };
  // std::initializer_list<std::string> init_list_string_type = {"hello", "world"}; // initializer_list<string> 底层元素为临时对象，序列化不安全
  std::array<int, 3>       array_int_type        = { 1, 2, 3 };
  std::vector<std::string> vector_string_type    = { "hello", "world" };
  std::deque<int>          deque_int_type        = { 1, 2, 3 };
  std::list<int>           list_int_type         = { 1, 2, 3 };
  std::forward_list<int>   forward_list_int_type = { 1, 2, 3 };
  // 容器适配器无公开迭代器接口，不支持直接序列化
  // std::stack<int> stack_int_type(array_int_type.begin(), array_int_type.end());
  // std::queue<int> queue_int_type(array_int_type.begin(), array_int_type.end());
  // std::priority_queue<int> priority_queue_int_type(array_int_type.begin(), array_int_type.end());
  std::span<int> span_int_type = array_int_type;
  std::bitset<8> bitset_type   = 0b10'101'010;
  // std::valarray<int> valarray_int_type = { 1, 2, 3 }; // Glaze 支持有限，序列化为数值数组可能丢失行为特性

  std::string      string_type      = "hello world";
  std::string_view string_view_type = "hello world";
  // 宽字符字符串序列化无标准化映射，可能失败
  // std::wstring     wstring_type     = L"hello world";
  // std::u8string    u8string_type    = u8"hello world";
  // std::u16string   u16string_type   = u"hello world";
  // std::u32string   u32string_type   = U"hello world";

  std::map<std::string, int> map_string_int_type = {
    { "hello", 1 },
    { "world", 2 }
  };
  std::unordered_map<std::string, int> unordered_map_string_int_type = {
    { "hello", 1 },
    { "world", 2 }
  };
  std::set<int>                   set_int_type             = { 1, 2, 3 };
  std::unordered_set<int>         unordered_set_int_type   = { 1, 2, 3 };
  std::multimap<std::string, int> multimap_string_int_type = {
    { "hello", 1 },
    { "world", 2 }
  };
  std::unordered_multimap<std::string, int> unordered_multimap_string_int_type = {
    { "hello", 1 },
    { "world", 2 }
  };
  std::multiset<int>           multiset_int_type           = { 1, 2, 3 };
  std::unordered_multiset<int> unordered_multiset_int_type = { 1, 2, 3 };

  // 复杂元组，包含不可复制/移动的类型（unique_ptr<char[]>）可能导致序列化失败
  // std::tuple<int, double, std::string, char, bool, long, Person, Base*,
  //            std::unique_ptr<char[]>, std::shared_ptr<RGB>>
  //     tuple_type = {10, 3.14, "hello", 'a', true, 1000L, Person{25, 60.5, "John"},
  //                   &base_type, std::make_unique<char[]>(10), std::make_shared<RGB>()};
  // 这里使用简化版来演示可序列化元组
  std::tuple<int, double, std::string, Person> tuple_type = {
    10,
    3.14,
    "hello",
    { 25, 60.5, "John" }
  };

  // pair 包含裸指针，无法安全序列化，故注释
  // std::pair<std::string, RGB *> pair_type = { "hello", &rgb_type };
  // 改为可序列化的 pair
  std::pair<std::string, RGB> pair_type = { "hello", rgb_type };

  // 复杂 variant 包含不可复制/移动类型，无法序列化
  // std::variant<int, double, std::string, char, bool, long, Person, Base*,
  //              std::unique_ptr<char[]>, std::shared_ptr<RGB>>
  //     variant_type = std::make_shared<RGB>();
  // 简化版
  std::variant<int, double, std::string> variant_type = 3.14;

  // any 包含非反射类型（shared_ptr<RGB>）无法安全序列化
  // std::any any_type = std::make_shared<RGB>();
  std::any any_type = 42;

  std::optional<int> optional_int_type = 10;
  // std::strong_ordering strong_ordering_type = std::strong_ordering::equal; // 比较结果类型，无数据可序列化

  // --- 3.12 流对象（不可序列化，代表I/O状态）---
  // std::stringstream stringstream_type;
  // std::ifstream input_file_stream_type("input.txt");
  // std::ofstream output_file_stream_type("output.txt");
  // std::fstream file_stream_type;
  // std::iostream* iostream_type = &stringstream_type; // 流指针更不可序列化

  // --- 3.13 内存管理（不可序列化）---
  // std::pmr::memory_resource* memory_resource_type = std::pmr::get_default_resource(); // 资源句柄
  // std::allocator<int> allocator_type; // 分配器无数据

  // --- 3.14 时间相关 ---
  std::chrono::system_clock::time_point time_point_type = std::chrono::system_clock::now();
  // std::chrono::steady_clock::time_point steady_clock_time_point_type; // steady_clock 无标准 epoch，不可序列化
  std::chrono::duration<double> duration_type = std::chrono::seconds(1);

  // --- 3.15 正则相关（不可序列化，状态机复杂）---
  // std::regex regex_type = std::regex("hello.*world");
  // std::smatch smatch_type;
  // std::regex_iterator<std::string::iterator> regex_iterator_type;
  // std::regex_token_iterator<std::string::iterator> regex_token_iterator_type;

  // --- 3.16 文件系统（部分不支持）---
  std::filesystem::path path_type = "C:/Users/John/Documents";
  // std::filesystem::directory_entry directory_entry_type; // 包含状态和缓存，不可序列化
  // std::filesystem::file_status file_status_type;         // 平台相关权限标志，不可序列化

  // --- 3.17 格式化（不可序列化）---
  // std::formatter<std::string> formatter_type; // 格式化器无数据状态

  // --- 3.18 错误处理（不可序列化）---
  // std::error_code error_code_type;
  // std::error_condition error_condition_type;
  // std::exception_ptr exception_ptr_type; // 异常指针包含运行时类型
  // std::runtime_error runtime_error_type("error"); // 异常对象无法序列化

  // --- 3.19 元编程与随机数（不可序列化）---
  // std::integral_constant<int, 5> integral_constant_type; // 编译时常量，无运行时数据
  // std::mt19937 mt19937_type; // 随机数引擎状态复杂且不可移植
  // std::random_device random_device_type; // 硬件熵源
  // std::uniform_int_distribution<int> uniform_int_dist_type(0, 100); // 分布参数序列化可考虑，但通常无意义
  // std::normal_distribution<double> normal_dist_type;
  // std::seed_seq seed_seq_type; // 种子序列

  // --- 3.20 线程与异步（不可序列化）---
  // std::thread thread_type([](int a) { std::cout << "Thread: " << a << std::endl; }, 10);
  // std::jthread jthread_type;
  // std::future<int> future_int_type = std::async(std::launch::async, []() { return 10; });
  // std::promise<int> promise_int_type = std::promise<int>();
  // std::shared_future<int> shared_future_type = future_int_type.share();
  // std::packaged_task<int(int)> packaged_task_int_type([](int a) { return a * 2; });

  // --- 3.21 可调用对象（不可序列化）---
  // std::function<std::string(int, double, std::string)> func_type =
  //     [](int a, double b, std::string c) {
  //         return std::to_string(a) + " " + std::to_string(b) + " " + c;
  //     };
  // int (*func_ptr_type)(int, double) = nullptr;                  // 函数指针
  // double Point::* member_ptr_type = &Point::x;                  // 成员指针
  // void (Base::*member_func_ptr_type)(const std::string&) = nullptr; // 成员函数指针

  // --- 3.22 迭代器与范围适配器（不可序列化）---
  // std::istream_iterator<int> istream_iterator_type(std::cin);
  // std::ostream_iterator<int> ostream_iterator_type(std::cout);
  // std::reverse_iterator<std::vector<int>::iterator> reverse_iterator_type;
  // std::back_insert_iterator<std::vector<std::string>> back_insert_iterator_type(vector_string_type);
  // std::move_iterator<std::vector<int>::iterator> move_iterator_type;
  // std::ranges::subrange<std::vector<int>::iterator> subrange_type;
  // std::ranges::ref_view<std::vector<std::string>> ref_view_type(vector_string_type);
  // std::ranges::owning_view<std::vector<int>> owning_view_type;
  // std::ranges::empty_view<int> empty_view_type;

// --- 序列化并输出所有支持的类型 ---
#define GLAZE_PRINT(var)                                                                                               \
  do                                                                                                                   \
  {                                                                                                                    \
    std::string buffer {};                                                                                             \
    auto        ec = glz::write_json(var, buffer);                                                                     \
    if (ec)                                                                                                            \
    {                                                                                                                  \
      std::cerr << "Error serializing " #var ": " << glz::format_error(ec, buffer) << std::endl;                       \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      std::cout << #var << ": " << buffer << std::endl;                                                                \
    }                                                                                                                  \
  }                                                                                                                    \
  while (0)

  std::cout << "=== 基本类型 ===" << std::endl;
  GLAZE_PRINT(bool_type);
  GLAZE_PRINT(char_type);
  GLAZE_PRINT(char_signed_type);
  GLAZE_PRINT(char_unsigned_type);
  // GLAZE_PRINT(wchar_type);
  // GLAZE_PRINT(char8_type);
  // GLAZE_PRINT(char16_type);
  // GLAZE_PRINT(char32_type);
  GLAZE_PRINT(short_type);
  GLAZE_PRINT(ushort_type);
  GLAZE_PRINT(int_type);
  GLAZE_PRINT(uint_type);
  GLAZE_PRINT(long_type);
  GLAZE_PRINT(ulong_type);
  GLAZE_PRINT(long_long_type);
  GLAZE_PRINT(ulong_long_type);
  GLAZE_PRINT(float_type);
  GLAZE_PRINT(double_type);
  // GLAZE_PRINT(long_double_type);
  GLAZE_PRINT(complex_double_type);

  std::cout << "\n=== 字面量 ===" << std::endl;
  GLAZE_PRINT(nullptr_type);
  GLAZE_PRINT(nullopt_type);
  // GLAZE_PRINT(nothrow_type);
  GLAZE_PRINT(monostate_type);

  std::cout << "\n=== 数组 ===" << std::endl;
  GLAZE_PRINT(char_array_type);

  std::cout << "\n=== 自定义类型 ===" << std::endl;
  GLAZE_PRINT(color_type);
  GLAZE_PRINT(direction_type);
  // GLAZE_PRINT(union_type);
  GLAZE_PRINT(point_type);
  GLAZE_PRINT(base_type);
  GLAZE_PRINT(Derived_type);
  GLAZE_PRINT(rgb_type);

  std::cout << "\n=== 智能指针 ===" << std::endl;
  GLAZE_PRINT(rgb_unique_ptr_type);
  GLAZE_PRINT(rgb_shared_ptr_type);
  // GLAZE_PRINT(rgb_weak_ptr_type);

  std::cout << "\n=== 容器和字符串 ===" << std::endl;
  GLAZE_PRINT(byte_type);
  GLAZE_PRINT(byte_array_type);
  GLAZE_PRINT(init_list_int_type);
  GLAZE_PRINT(array_int_type);
  GLAZE_PRINT(vector_string_type);
  GLAZE_PRINT(deque_int_type);
  GLAZE_PRINT(list_int_type);
  GLAZE_PRINT(forward_list_int_type);
  GLAZE_PRINT(span_int_type);
  GLAZE_PRINT(bitset_type);
  // GLAZE_PRINT(valarray_int_type);

  GLAZE_PRINT(string_type);
  GLAZE_PRINT(string_view_type);
  // GLAZE_PRINT(wstring_type);
  // GLAZE_PRINT(u8string_type);
  // GLAZE_PRINT(u16string_type);
  // GLAZE_PRINT(u32string_type);

  std::cout << "\n=== 关联容器 ===" << std::endl;
  GLAZE_PRINT(map_string_int_type);
  GLAZE_PRINT(unordered_map_string_int_type);
  GLAZE_PRINT(set_int_type);
  GLAZE_PRINT(unordered_set_int_type);
  GLAZE_PRINT(multimap_string_int_type);
  GLAZE_PRINT(unordered_multimap_string_int_type);
  GLAZE_PRINT(multiset_int_type);
  GLAZE_PRINT(unordered_multiset_int_type);

  std::cout << "\n=== 其他标准库类型 ===" << std::endl;
  GLAZE_PRINT(tuple_type);
  GLAZE_PRINT(pair_type);
  GLAZE_PRINT(variant_type);
  // GLAZE_PRINT(any_type); // 若 any 包含非反射类型则失败，这里注释以策安全
  GLAZE_PRINT(optional_int_type);
  GLAZE_PRINT(time_point_type);
  GLAZE_PRINT(duration_type);
  // GLAZE_PRINT(regex_type);
  GLAZE_PRINT(path_type);

  std::cout << "\n=== 私有成员 ===" << std::endl;
  // --- 私有成员 序列化测试 ---
  // 声明友元
  User user { "Alice", 30 };
  GLAZE_PRINT(user);
  // lambda 访问
  MyData data;
  data.get_value() = 3.14;
  GLAZE_PRINT(data);

  std::cout << "\n=== 多态 ===" << std::endl;
  // --- 多态 序列化测试 ---
  // 多重继承
  FlyingAnimal eagle { { "Eagle" }, { 210 }, 5 };
  GLAZE_PRINT(eagle);
  // 多态
  Base objA = DerivedA(42, "DerivedA");
  Base objB = DerivedB(42.0, "DerivedB");
  GLAZE_PRINT(objA);
  GLAZE_PRINT(objB);

  std::cout << "\n=== 指针序列化/反序列化 ===" << std::endl;
  // --- 指针(不安全) 序列化测试 ---
  // 需要确保指针在序列化/反序列化期间始终有效
  Point       p { 10, 20 };
  Container   container { &p };
  std::string json = *glz::write_json(container);
  std::cout << "序列化: " << json << std::endl;

  Point     p2;
  Container container2 { &p2 };
  glz::read_json(container2, json);
  std::cout << "反序列化: " << "x:" << p2.x << ", y:" << p2.y << std::endl;

  // =================================================================
  // 4. 指定类型的反序列化测试（仅测试多态、initializer_list、span、string_view）
  // =================================================================
  std::cout << "\n=== 反序列化测试（指定类型） ===" << std::endl;

  // --- 4.1 多态：FlyingAnimal（多重继承，手动反射） ---
  {
    // 获取之前序列化 eagle 的 JSON
    std::string  eagle_json = glz::write_json(eagle).value_or("{}");
    FlyingAnimal eagle_restored; // 默认构造
    auto         ec = glz::read_json(eagle_restored, eagle_json);
    if (ec)
    {
      std::cerr << "  FlyingAnimal 反序列化失败: " << glz::format_error(ec, eagle_json) << std::endl;
    }
    else
    {
      std::cout << "  FlyingAnimal 反序列化成功: species=" << eagle_restored.species
                << ", wingspan=" << eagle_restored.wingspan << ", age=" << eagle_restored.age << std::endl;
    }
  }

  // --- 4.2 多态：值语义切片（Base objA，切片自 DerivedA） ---
  {
    // 获取之前序列化 objA 的 JSON（只包含 Base::str）
    std::string obja_json = glz::write_json(objA).value_or("{}");
    Base        objA_restored(""); // 默认构造空字符串，之后会被覆盖
    auto        ec = glz::read_json(objA_restored, obja_json);
    if (ec)
    {
      std::cerr << "  值切片 objA 反序列化失败: " << glz::format_error(ec, obja_json) << std::endl;
    }
    else
    {
      std::cout << "  值切片 objA 反序列化成功 (仅 Base 部分): str=" << objA_restored.str << std::endl;
    }
  }
  // --- 4.3 多态 variant处理切片问题---
  {
    // 序列化
    MyPolymorphicType objA  = DerivedA { 10, "world" };
    std::string       jsonA = glz::write_json(objA).value_or("");
    std::cout << "序列化 DerivedA: " << jsonA << std::endl;
    // 输出: {"type":"DerivedA","num":10,"str":"world"}

    // 反序列化，必须提供默认构造函数，否则无法反序列化!!!!!!!!!!!除非进行optional包装或者自定义
    MyPolymorphicType restored;
    std::string       jsonToRead = R"({"type":"DerivedB","num":3.14,"str":"hello"})";
    /*auto              ec         = glz::read_json(restored, jsonToRead);
    if (!ec)
    {
      if (std::holds_alternative<DerivedB>(restored))
      {
        auto &b = std::get<DerivedB>(restored);
        std::cout << "反序列化成功: DerivedB { num=" << b.num << ", str=" << b.str << " }" << std::endl;
      }
    }*/
    // 忽略 glz::json_t 弃用警告（若不想忽略可改用 glz::generic_json<glz::num_mode::f64>）
#pragma warning(push)
#pragma warning(disable : 4996)
    glz::json_t json_tree;
#pragma warning(pop)

    auto ec = glz::read_json(json_tree, jsonToRead);
    if (!ec)
    {
      // 注意：必须用 std::string，不能用 std::string_view
      std::string type = json_tree["type"].get<std::string>();
      if (type == "DerivedA")
      {
        // glz数值/*std::variant<null_t, double, std::string, bool, array_t, object_t>*/存储为 double，需转换
        int         num = static_cast<int>(json_tree["num"].get<double>());
        std::string str = json_tree["str"].get<std::string>();
        restored        = DerivedA(num, str); // 带参构造
      }
      else if (type == "DerivedB")
      {
        double      num = json_tree["num"].get<double>();
        std::string str = json_tree["str"].get<std::string>();
        restored        = DerivedB(num, str); // 带参构造
      }
      else
      {
        restored = std::monostate {};
      }

      if (std::holds_alternative<DerivedB>(restored))
      {
        auto &b = std::get<DerivedB>(restored);
        std::cout << "反序列化成功: DerivedB { num=" << b.num << ", str=" << b.str << " }" << std::endl;
      }
    }
    else
    {
      std::cerr << "JSON 解析失败: " << glz::format_error(ec, jsonToRead) << std::endl;
    }
  }
  // --- 4.4 initializer_list<int> 反序列化 ---
  {
    // 注意：initializer_list 不拥有内存，无法直接作为反序列化目标。
    // 这里演示从 JSON 读取到 vector<int> 来说明数据本身是可恢复的。
    std::string il_json = glz::write_json(init_list_int_type).value_or("[]");

    // 尝试直接反序列化到 initializer_list（若 Glaze 不支持，此处会编译错误；已注释以防编译中断）
    // std::initializer_list<int> il_direct;
    // auto ec = glz::read_json(il_direct, il_json);   // 无法通过编译或运行时错误

    // 正确的做法：读取到拥有所有权的容器
    std::vector<int> vec_from_il;
    auto             ec = glz::read_json(vec_from_il, il_json);
    if (ec)
    {
      std::cerr << "  initializer_list -> vector 反序列化失败: " << glz::format_error(ec, il_json) << std::endl;
    }
    else
    {
      std::cout << "  initializer_list 数据反序列化到 vector 成功:";
      for (int x : vec_from_il)
        std::cout << " " << x;
      std::cout << std::endl;
    }
  }

  // --- 4.5 span<int> 反序列化 ---
  {
    // span 只是视图，反序列化时需要底层数组已经存在，glaze 会将数据写入该数组。
    std::string    span_json   = glz::write_json(span_int_type).value_or("[]");
    int            arr_span[3] = { 0, 0, 0 }; // 底层存储
    std::span<int> span_restored(arr_span);   // 绑定视图
    auto           ec = glz::read_json(span_restored, span_json);
    if (ec)
    {
      std::cerr << "  span 反序列化失败: " << glz::format_error(ec, span_json) << std::endl;
    }
    else
    {
      std::cout << "  span 反序列化成功 (底层数组): " << arr_span[0] << " " << arr_span[1] << " " << arr_span[2]
                << std::endl;
    }
  }

  // --- 4.6 string_view 反序列化 ---
  {
    // string_view 同样不拥有内存，不能直接作为反序列化目标。
    // 正确流程：先反序列化到 std::string，再绑定 string_view。
    std::string sv_json = glz::write_json(string_view_type).value_or("\"\"");

    // 尝试直接反序列化到 string_view（预期失败或编译错误，已注释）
    // std::string_view sv_direct;
    // auto ec = glz::read_json(sv_direct, sv_json);   // 编译错误或运行时未定义行为

    std::string str_restored;
    auto        ec = glz::read_json(str_restored, sv_json);
    if (ec)
    {
      std::cerr << "  string_view 反序列化 (到 string) 失败: " << glz::format_error(ec, sv_json) << std::endl;
    }
    else
    {
      std::string_view sv_restored(str_restored); // 绑定安全
      std::cout << "  string_view 反序列化成功 (通过 string): " << sv_restored << std::endl;
    }
  }
  return 0;
}
