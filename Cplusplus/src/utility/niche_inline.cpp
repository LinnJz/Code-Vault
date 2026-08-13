#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <meta>
#include <ranges>
#include <type_traits>
#include <utility>


namespace std { 
  
consteval bool __is_pod__(std::meta::info type_info) noexcept {
  return std::meta::is_trivially_destructible_type(type_info) ||
         std::meta::is_trivially_default_constructible_type(type_info);
}

// ============ 公共元数据：反射只算一次，两套布局共享 ============
template <class T>
struct __storage_meta 
{
  static constexpr auto __nonstatic_data_members {
    // unchecked()为"无访问限制"上下文，可拿到private/protected成员；
    // access_context::current()按"求值点"过滤，在类模板内求值会漏掉不可访问成员
    std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))
  };
  template<size_t I>
  using __member_type = typename [: std::meta::type_of(__nonstatic_data_members[I]) :];

  // 存储适配：byte数组里无法直接存放引用成员，仿照libc++ optional<T&>的方案
  // （__optional_storage_base<_Tp, is_reference_v<_Tp>> 偏特化：__raw_type* __value_）
  // 将 M& 存成"指向被引用对象的指针"。
  // 值成员剥掉cv再存（与libc++的remove_cv_t<value_type> __val_一致）：
  // construct_at不支持const T*（libstdc++实现里void* __loc = __location;丢const），
  // const语义由API层__member_type保留（get返回const int&，对象按int构造）。
  template <class M>
  struct __storage_of { using type = std::remove_cv_t<M>; };
  template <class M>
  struct __storage_of<M&>  { using type = std::remove_reference_t<M>*; };
  template <class M>
  struct __storage_of<M&&> { using type = std::remove_reference_t<M>*; };
  template <size_t I>
  using __storage_type = typename __storage_of<__member_type<I>>::type;

  static constexpr size_t __member_count = __nonstatic_data_members.size();
  using __tag_type = std::conditional_t<
      (__member_count <= 255), 
      std::uint8_t,
      std::conditional_t<
          (__member_count <= 65536), 
          std::uint16_t, 
          std::conditional_t<
              (__member_count <= 4294967295ULL), 
              std::uint32_t, 
              std::uint64_t
          >
      >
  >;

  static constexpr auto __storage_traits = [] consteval noexcept {
    struct storage_traits {
      size_t __max_size = 0;
      size_t __max_align = 1;
    } traits;
    template for (constexpr auto I : std::views::iota(0zu, __member_count)) {
      if (traits.__max_size < sizeof(__storage_type<I>)) {
        traits.__max_size = sizeof(__storage_type<I>);
      }
      if (traits.__max_align < alignof(__storage_type<I>)) { 
        traits.__max_align = alignof(__storage_type<I>);
      }
    }
    // 空类：零长度数组不合法，退化为1字节
    return (traits.__max_size = traits.__max_size ? traits.__max_size : 1, traits);
  }();

  static constexpr auto __nothrow_traits = [] consteval noexcept {
    struct nothrow_traits {
      bool __is_all_mems_copy_assignment_nothrow = true;
      bool __is_all_mems_copy_constructor_nothrow = true;
      bool __is_all_mems_move_assignment_nothrow = true;
      bool __is_all_mems_move_constructor_nothrow = true;
      bool __is_all_mems_default_constructor_nothrow = true;
      bool __is_all_mems_equality_comparison_nothrow = true;
    } traits;
    template for (constexpr auto I : std::views::iota(0zu, __member_count)) {
      using storage_t = __storage_type<I>;            // 引用成员的操作实为指针拷贝，恒不抛
      using member_plain_t = std::remove_cvref_t<__member_type<I>>; // 比较语义作用于被引用对象
      traits.__is_all_mems_move_constructor_nothrow &= 
          std::is_nothrow_move_constructible_v<storage_t>;
      traits.__is_all_mems_default_constructor_nothrow &= 
          std::is_nothrow_default_constructible_v<storage_t>;
      traits.__is_all_mems_copy_constructor_nothrow &= 
          std::is_nothrow_copy_constructible_v<storage_t>;
      traits.__is_all_mems_copy_assignment_nothrow &= 
          std::is_nothrow_copy_assignable_v<storage_t>;
      traits.__is_all_mems_move_assignment_nothrow &= 
          std::is_nothrow_move_assignable_v<storage_t>;
      traits.__is_all_mems_equality_comparison_nothrow &= 
          std::equality_comparable<member_plain_t> && 
          requires(const member_plain_t& __lhs, const member_plain_t& __rhs) { { __lhs == __rhs } noexcept; };
    }
    return traits;
  }();

  // ---- niche 判定：全部引用成员（适配后指针强制非空 → null 腾出）&& 对齐位容量足够 ----
  // 编码：存储值 = 真实地址 | 变体编号I；disengaged = 0
  // 借用位数 = min_i(log2(alignof(T_i)))，要求 2^bits >= 成员数（单成员时 bits=0 亦可）
  static constexpr size_t __tag_bits = [] consteval noexcept {
    size_t bits = std::numeric_limits<size_t>::digits;
    template for (constexpr auto I : std::views::iota(0zu, __member_count)) {
      // 对象地址必被其alignof整除（alignof为2的幂），低log2(alignof)位恒为0
      bits = std::min<size_t>(bits, static_cast<size_t>(std::countr_zero(
          alignof(std::remove_reference_t<__member_type<I>>))));
    }
    return bits;
  }();
  static constexpr bool __niche_capable = [] consteval noexcept {
    bool all_ref = true;
    template for (constexpr auto I : std::views::iota(0zu, __member_count)) {
      all_ref = all_ref && std::is_reference_v<__member_type<I>>;
    }
    return all_ref && (__member_count <= (1uz << __tag_bits));
  }();
  static constexpr size_t __tag_mask = __tag_bits ? (1uz << __tag_bits) - 1 : 0;
};

// ============ 主模板仅声明，约束特化按 __niche_capable 互斥选择 ============
template <class T>
struct __storage_base;

// ---- 通用版：byte数组 + 独立__tag（成员含值类型/普通指针/对齐位不足时回退）----
template <class T>
  requires (!__storage_meta<T>::__niche_capable)
struct __storage_base<T> : __storage_meta<T>
{
  using __base = __storage_meta<T>;
  // 别名收编：短名直接使用，避免整页 __storage_meta<T>:: 前缀
  using __tag_type = typename __base::__tag_type;
  static constexpr size_t __member_count = __base::__member_count;
  static constexpr auto __storage_traits = __base::__storage_traits;
  template <size_t I>
  using __member_type = typename __base::template __member_type<I>;
  template <size_t I>
  using __storage_type = typename __base::template __storage_type<I>;

  alignas(__storage_traits.__max_align)
      std::byte __storage[__storage_traits.__max_size];
  __tag_type __tag { static_cast<__tag_type>(__member_count) };

#pragma region
  constexpr std::size_t index() const noexcept {
    return static_cast<size_t>(__tag);
  }

  constexpr bool has_value() const noexcept {
    return static_cast<size_t>(__tag) != __member_count;
  }

  template <size_t I>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(self.index() == I && "get: member is not the active one");
    // 仅剥引用保留cv：is_const必须看到对象的const（remove_cvref会丢掉const）
    using self_t = std::remove_reference_t<decltype(self)>;
    using member_t = __member_type<I>;
    using member_plain_t = std::remove_reference_t<member_t>; // 保留成员自身的cv（如 const int）
    if constexpr (std::is_reference_v<member_t>) {
      // 引用成员：storage里存的是"指向被引用对象的指针"，须双重解引用取引用；
      // 与真实成员访问一致，对象的const不会穿透引用成员（同optional<T&>::__get() const返回value_type&）
      // 目标指针类型加上const，避免reinterpret_cast从const std::byte* cast away constness
      using storage_ptr_t = std::add_const_t<__storage_type<I>> *;
      return **reinterpret_cast<storage_ptr_t>(self.__storage);
    } else {
      using member_ptr_t = std::conditional_t<std::is_const_v<self_t>, member_plain_t const *, member_plain_t *>;
      return std::forward_like<decltype(self)>(*reinterpret_cast<member_ptr_t>(self.__storage));
    }
  }

  template <size_t I, class... Args>
  constexpr decltype(auto) construct(Args&&...args) noexcept(
      std::is_reference_v<__member_type<I>> ||
      std::is_nothrow_constructible_v<__storage_type<I>, Args...>
  ) {
    static_assert(I < __member_count, "Index out of range");
    assert(!has_value() && "construct: storage already engaged");
    if constexpr (std::is_reference_v<__member_type<I>>) {
      // 引用成员：仿optional<T&>，绑定实参后存地址；只接受左值，防止悬垂
      static_assert(sizeof...(Args) == 1 && (std::is_lvalue_reference_v<Args> && ...),
                    "construct for a reference member requires exactly one lvalue argument");
      static_assert(!(std::reference_constructs_from_temporary_v<__member_type<I>, Args> && ...),
                    "construct for a reference member would bind to a temporary");
      std::construct_at(reinterpret_cast<__storage_type<I> *>(__storage),
                        std::addressof(std::forward<Args>(args)...));
    } else {
      // 值成员（含const/volatile限定）：placement new直接构造，
      // cv限定类型是合法的new目标（如 ::new (p) const int(42)）
      std::construct_at(reinterpret_cast<__storage_type<I> *>(__storage),
                        std::forward<Args>(args)...);
    }
    __tag = static_cast<__tag_type>(I);
    return (*this).template get<I>();
  }

  template <size_t I>
  constexpr void destroy() noexcept(std::is_nothrow_destructible_v<__storage_type<I>>) {
    static_assert(I < __member_count, "Index out of range");
    assert(index() == I && "destroy: member is not the active one");
    // 引用成员的适配类型是指针（平凡析构），不会销毁被引用对象本身
    if constexpr (!std::is_reference_v<__member_type<I>> &&
                  !std::is_trivially_destructible_v<__storage_type<I>>) {
      std::destroy_at(reinterpret_cast<__storage_type<I> *>(__storage));
    }
    __tag = static_cast<__tag_type>(__member_count);
  }
#pragma endregion
};

// ---- niche版：全引用成员，8字节uintptr_t槽，无__tag字段 ----
// 存储值 = 真实地址 | I（低 __tag_bits 位恒0，对齐保证）；disengaged = 0
template <class T>
  requires (__storage_meta<T>::__niche_capable)
struct __storage_base<T> : __storage_meta<T>
{
  using __base = __storage_meta<T>;
  // 别名收编：短名直接使用，避免整页 __storage_meta<T>:: 前缀
  static constexpr size_t __member_count = __base::__member_count;
  static constexpr size_t __tag_mask = __base::__tag_mask;
  template <size_t I>
  using __member_type = typename __base::template __member_type<I>;

  // uintptr_t对象直接读写：无严格别名问题（避免reinterpret_cast<std::byte*→uintptr_t*>的UB）
  std::uintptr_t __slot = 0;

#pragma region
  constexpr std::size_t index() const noexcept {
    // 单成员时 __tag_mask=0：engaged恒返回0；disengaged返回__member_count
    return __slot == 0 ? __member_count
                       : static_cast<size_t>(__slot & __tag_mask);
  }

  constexpr bool has_value() const noexcept {
    return __slot != 0;
  }

  template <size_t I>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(self.index() == I && "get: member is not the active one");
    auto stored = self.__slot;                                  // const对象也只读槽值
    auto addr = reinterpret_cast<void *>(stored & ~__tag_mask); // 1条AND清位还原
    // 引用成员const不穿透：返回M&（同optional<T&>::__get() const返回value_type&）
    return *static_cast<std::remove_reference_t<__member_type<I>> *>(addr);
  }

  template <size_t I, class... Args>
  constexpr decltype(auto) construct(Args&&...args) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(!has_value() && "construct: storage already engaged");
    // 引用成员：仿optional<T&>，绑定实参后存地址；只接受左值，防止悬垂
    static_assert(sizeof...(Args) == 1 && (std::is_lvalue_reference_v<Args> && ...),
                  "construct for a reference member requires exactly one lvalue argument");
    static_assert(!(std::reference_constructs_from_temporary_v<__member_type<I>, Args> && ...),
                  "construct for a reference member would bind to a temporary");
    auto addr = reinterpret_cast<std::uintptr_t>(std::addressof(std::forward<Args>(args)...));
    assert((addr & __tag_mask) == 0);                           // 对齐保证，防御性检查
    __slot = addr | I;                                          // 1条OR写入变体编号
    return (*this).template get<I>();
  }

  template <size_t I>
  constexpr void destroy() noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(index() == I && "destroy: member is not the active one");
    __slot = 0;                                                 // 引用成员无析构，置0即disengaged
  }
#pragma endregion
};

} // namespace std

#include <cassert>
#include <cstdio>
#include <string>

struct Ref2   { int& a; std::string& b; };            // 全引用：可niche
struct Ref1   { int& a; };                            // 单引用：可niche
struct Ref3   { int& a; std::string& b; double& c; }; // 3引用：2位对齐位足够
struct NoNiche { int& a; char& b; };                  // char对齐1，无位可借 → tag回退
struct Mixed  { int x; std::string& s; };             // 含值成员 → tag回退

// ---- 编译期判定 ----
static_assert(std::__storage_meta<Ref2>::__niche_capable);
static_assert(std::__storage_meta<Ref1>::__niche_capable);
static_assert(std::__storage_meta<Ref3>::__niche_capable);
static_assert(!std::__storage_meta<NoNiche>::__niche_capable);
static_assert(!std::__storage_meta<Mixed>::__niche_capable);

// ---- 尺寸：niche版恒8字节（无tag字段），tag版按现状 ----
static_assert(sizeof(std::__storage_base<Ref2>) == 8);   // 旧方案16 → 省8
static_assert(sizeof(std::__storage_base<Ref1>) == 8);
static_assert(sizeof(std::__storage_base<Ref3>) == 8);
static_assert(sizeof(std::__storage_base<NoNiche>) == 16);
static_assert(sizeof(std::__storage_base<Mixed>) == 16); // string& 适配为指针8 + tag 1 → 对齐8

// ---- 返回类型：引用成员const不穿透 ----
static_assert(std::is_same_v<decltype(std::declval<std::__storage_base<Ref2>&>().get<0>()), int&>);
static_assert(std::is_same_v<decltype(std::declval<std::__storage_base<Ref2>&>().get<1>()), std::string&>);
static_assert(std::is_same_v<decltype(std::declval<const std::__storage_base<Ref2>&>().get<1>()), std::string&>);

int main() {
  int x = 1;
  std::string s = "hello";

  { // 双引用 niche
    std::__storage_base<Ref2> r;
    assert(!r.has_value());
    r.construct<0>(x);
    assert(r.has_value() && r.index() == 0);
    r.get<0>() = 2;
    assert(x == 2);                                // 引用绑定生效
    r.destroy<0>();
    assert(!r.has_value() && r.index() == 2);      // disengaged index == member_count

    r.construct<1>(s);
    assert(r.index() == 1);
    r.get<1>() += "!";
    assert(s == "hello!");                         // 修改透过引用
    const auto& cr = r;
    assert(cr.get<1>() == "hello!");               // const对象get：const不穿透
    r.destroy<1>();
    assert(!r.has_value());
  }

  { // 单引用 niche（mask=0 退化路径）
    std::__storage_base<Ref1> r;
    r.construct<0>(x);
    assert(r.index() == 0 && r.get<0>() == 2);
    r.destroy<0>();
    assert(r.index() == 1);
  }

  { // 三引用 niche
    double d = 3.14;
    std::__storage_base<Ref3> r;
    r.construct<2>(d);
    assert(r.index() == 2 && r.get<2>() == 3.14);
    r.destroy<2>();
    assert(r.index() == 3);
  }

  { // 对齐不足 → tag版回退（引用分支仍正常）
    char c = 'z';
    std::__storage_base<NoNiche> r;
    r.construct<1>(c);
    assert(r.index() == 1 && r.get<1>() == 'z');
    r.destroy<1>();
    assert(r.index() == 2);
  }

  { // 含值成员 → tag版回退
    std::__storage_base<Mixed> r;
    r.construct<0>(42);
    assert(r.index() == 0 && r.get<0>() == 42);
    r.destroy<0>();
    std::string s2 = "world";
    r.construct<1>(s2);
    assert(r.index() == 1 && r.get<1>() == "world");
    r.destroy<1>();
    assert(r.index() == 2);
  }

  std::puts("ALL PASS");
}


