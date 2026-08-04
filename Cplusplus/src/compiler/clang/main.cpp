// https://clang.llvm.org/docs/LanguageExtensions.html
// https://clang.llvm.org/docs/AttributeReference.html#aarch64-sve-pcs

#include <stdio.h>
import std;

using std::literals::string_view_literals::operator""sv;

#pragma region BUILDIN_MACROS

auto s_base_file = __BASE_FILE__;

auto s_file_name = __FILE_NAME__;

auto s_timestamp = __TIMESTAMP__;

auto s_clang = __clang__;

auto s_clang_major      = __clang_major__;
auto s_clang_minor      = __clang_minor__;
auto s_clang_patchlevel = __clang_patchlevel__;
auto s_clang_version    = __clang_version__;

auto s_clang_literal_encoding      = __clang_literal_encoding__;
auto s_clang_wide_literal_encoding = __clang_wide_literal_encoding__;

#pragma endregion BUILDIN_MACROS

#pragma region DEFINED_KEYWORDS
/*
static_assert(std::same_as<
              std::remove_cv_t<std::string const volatile>,
              __typeof_unqual__(std::string const volatile)>);
*/
#pragma endregion DEFINED_KEYWORDS

__attribute__((visibility("default"))) void func();

#pragma export(foo)
extern "C" void foo(int a);

#if __has_builtin(__builtin_get_vtable_pointer)

struct PolymorphicClass
{
  virtual ~PolymorphicClass();
};

static PolymorphicClass anInstance;
void const *vtablePointer = __builtin_get_vtable_pointer(&anInstance);

#endif

#if __has_builtin(__builtin_cpu_supports)

struct S
{
  int x, y;
  float f;

  struct T
  {
    int i;
  } t;
};

void dump_struct(struct S *s)
{
  __builtin_dump_struct(s, ::printf);
}

constexpr void constexpr_sprintf(std::string &out, char const *format, auto... args)
{
  // ...
}

constexpr std::string dump_struct(auto &x)
{
  std::string s;
  __builtin_dump_struct(&x, constexpr_sprintf, s);
  return s;
}

#endif

#if __has_builtin(__builtin_shufflevector)
// __builtin_shufflevector(vec1, vec2, index1, index2, ...)

/*
// identity operation - return 4-element vector v1.
__builtin_shufflevector(v1, v1, 0, 1, 2, 3)

// "Splat" element 0 of V1 into a 4-element result.
__builtin_shufflevector(V1, V1, 0, 0, 0, 0)

// Reverse 4-element vector V1.
__builtin_shufflevector(V1, V1, 3, 2, 1, 0)

// Concatenate every other element of 4-element vectors V1 and V2.
__builtin_shufflevector(V1, V2, 0, 2, 4, 6)

// Concatenate every other element of 8-element vectors V1 and V2.
__builtin_shufflevector(V1, V2, 0, 2, 4, 6, 8, 10, 12, 14)

// Shuffle v1 with some elements being undefined. Not allowed in constexpr.
__builtin_shufflevector(v1, v1, 3, -1, 1, -1)
*/
#endif

#if __has_builtin(__builtin_bitreverse)
/*
uint8_t rev_x = __builtin_bitreverse8(x);
uint16_t rev_x = __builtin_bitreverse16(x);
uint32_t rev_y = __builtin_bitreverse32(y);
uint64_t rev_z = __builtin_bitreverse64(z);
*/
#endif

#if __has_builtin(__builtin_rotateleft)
/*
uint8_t rot_x = __builtin_rotateleft8(x, y);
uint16_t rot_x = __builtin_rotateleft16(x, y);
uint32_t rot_x = __builtin_rotateleft32(x, y);
uint64_t rot_x = __builtin_rotateleft64(x, y);

uint8_t rot_x = __builtin_rotateright8(x, y);
uint16_t rot_x = __builtin_rotateright16(x, y);
uint32_t rot_x = __builtin_rotateright32(x, y);
uint64_t rot_x = __builtin_rotateright64(x, y);

T __builtin_stdc_rotate_left(T value, count)
T __builtin_stdc_rotate_right(T value, count)

<bit>中没有的功能
__builtin_stdc_first_leading_zero	无直接对应	查找从最高位开始第一个 0 的位置。
__builtin_stdc_first_leading_one	无直接对应	查找从最高位开始第一个 1 的位置。
__builtin_stdc_first_trailing_zero	无直接对应	查找从最低位开始第一个 0 的位置。
__builtin_stdc_first_trailing_one	无直接对应	查找从最低位开始第一个 1 的位置。
__builtin_stdc_count_zeros 无直接对应	统计整个数值中 0 的总个数。
*/
#endif


#if __has_builtin(__builtin_expect_with_probability)
// long __builtin_expect(long expr, long val)
// long __builtin_expect_with_probability(long expr, long val, double p);
// x是0的概率是大概是百分之30
// if (__builtin_expect_with_probability(x, 0, .3))
// {
//   bar();
// }
#endif

#if __has_builtin(__builtin_prefetch)
// 用于与缓存处理程序通信，以便在数据被使用前将其加载到缓存中。常常和循环展开搭配使用优化
/**
 * @brief 
 * @param addr addr 是需要被载入缓存的地址
 * @param rw 0 表示读取，1 表示写入，在读写访问的情况下，应使用 1
 * @param locality locality 表示数据在缓存中的预期持久性，0 表示数据在下次使用后可以被丢弃，3 表示数据一旦载入缓存就会被大量重用。1 和 2 提供了这两种极端情况之间的中间行为
 */
// void __builtin_prefetch(const void *addr, int rw=0, int locality=3)
#endif

#if __has_builtin(__builtin_prefetch)
// 用于在内存中原子地交换整数或指针。
// type __sync_swap(type *ptr, type value, ...)
// int old_value = __sync_swap(&value, new_value);

//__sync_bool_compare_and_swap()
// __sync_lock_test_and_set()
#endif

/*
弃用宏
#define MIN(x, y) x < y ? x : y
#pragma clang deprecated(MIN, "use std::min instead")

最终宏定义
#define FINAL_MACRO 1
#pragma clang final(FINAL_MACRO)

#define FINAL_MACRO // warning: FINAL_MACRO is marked final and should not be redefined
#undef FINAL_MACRO  // warning: FINAL_MACRO is marked final and should not be undefined
*/

/*
#pragma clang attribute push (__attribute__((annotate("custom"))), apply_to = function)

void function(); // The function now has the annotate("custom") attribute

#pragma clang attribute pop

#pragma clang attribute push ([[noreturn]], apply_to = function)

void function(); // The function now has the [[noreturn]] attribute

#pragma clang attribute push([[nodiscard]], apply_to = any(record, enum))

enum Enum2 { A2, B2 }; // The enum will receive [[nodiscard]]

struct Record2 { }; // The struct *will* receive [[nodiscard]]

#pragma clang attribute pop

#pragma clang attribute pop
Clang supports the following match rules:
Clang 支持以下匹配规则：

    function: Can be used to apply attributes to functions. This includes C++ member functions, static functions, operators, and constructors/destructors.
    可用于对函数应用属性。这包括C++成员函数、静态函数、运算符以及构造函数/析构函数。

    function(is_member): Can be used to apply attributes to C++ member functions. This includes members like static functions, operators, and constructors/destructors.
    可用于对C++成员函数应用属性。这包括静态函数、运算符以及构造函数/析构函数等成员。

    hasType(functionType): Can be used to apply attributes to functions, C++ member functions, and variables/fields whose type is a function pointer. It does not apply attributes to Objective-C methods or blocks.
    可用于对函数、C++成员函数以及类型为函数指针的变量/字段应用属性。它不适用于Objective-C方法或块。

    type_alias: Can be used to apply attributes to typedef declarations and C++11 type aliases.
    可用于对typedef声明和C++11类型别名应用属性。

    record: Can be used to apply attributes to struct, class, and union declarations.
    可用于对结构体、类和联合体声明应用属性。

    record(unless(is_union)): Can be used to apply attributes only to struct and class declarations.
    只能用于对结构体和类声明应用属性。

    enum: Can be used to apply attributes to enumeration declarations.
    可用于对枚举声明应用属性。

    enum_constant: Can be used to apply attributes to enumerators.
    可用于对枚举值应用属性。

    variable: Can be used to apply attributes to variables, including local variables, parameters, global variables, and static member variables. It does not apply attributes to instance member variables or Objective-C ivars.
    可用于对变量应用属性，包括局部变量、参数、全局变量和静态成员变量。它不用于对实例成员变量或Objective-C的ivars应用属性。

    variable(is_thread_local): Can be used to apply attributes to thread-local variables only.
    只能用于为线程局部变量应用属性。

    variable(is_global): Can be used to apply attributes to global variables only.
    只能用于对全局变量应用属性。

    variable(is_local): Can be used to apply attributes to local variables only.
    只能用于给局部变量添加属性。

    variable(is_parameter): Can be used to apply attributes to parameters only.
    只能用于对参数应用属性。

    variable(unless(is_parameter)): Can be used to apply attributes to all the variables that are not parameters.
    可用于为所有非参数的变量应用属性。

    field: Can be used to apply attributes to non-static member variables in a record. This includes Objective-C ivars.
    可用于为记录中的非静态成员变量应用属性。这包括Objective-C的实例变量。

    namespace: Can be used to apply attributes to namespace declarations.
    可用于对命名空间声明应用属性。

    objc_interface: Can be used to apply attributes to @interface declarations.
    可用于应用属性到@interface声明。

    objc_protocol: Can be used to apply attributes to @protocol declarations.
    可用于对@protocol声明应用属性。

    objc_category: Can be used to apply attributes to category declarations, including class extensions.
    可用于对类别声明应用属性，包括类扩展。

    objc_method: Can be used to apply attributes to Objective-C methods, including instance and class methods. Implicit methods like implicit property getters and setters do not receive the attribute.
    可用于对Objective-C方法应用属性，包括实例方法和类方法。隐式方法如隐式属性的getter和setter不会接收该属性。

    objc_method(is_instance): Can be used to apply attributes to Objective-C instance methods.
    可用于对Objective-C实例方法应用属性。

    objc_property: Can be used to apply attributes to @property declarations.
    可用于应用属性到@property声明。

    block: Can be used to apply attributes to block declarations. This does not include variables/fields of block pointer type.
    可用于对块声明应用属性。这不包括块指针类型的变量/字段。

*/

/*
// 本示例说明，柔性数组成员数组的计数为其分配的元素数：
struct bar;

struct foo {
  size_t count;
  char other;
  struct bar *array[] __attribute__((counted_by(count)));
};
*/

// [[no_unique_address]] msvc请使用[[msvc::no_unique_address]]

// 循环展开参数可以通过选项 -mllvm -unroll-count=n 和 -mllvm -pragma-unroll-threshold=n 来控制。

auto main() -> int
{
  __fp16 fp16 = 11.f16;
  std::println("{}", 42);
  return 0;
}
