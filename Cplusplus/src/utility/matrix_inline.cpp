#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
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

template <class T>
struct __storage_base 
{
  static constexpr auto __nonstatic_data_members {
    std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))
  };
  template<size_t I>
  using __member_type = typename [: std::meta::type_of(__nonstatic_data_members[I]) :];
  // using __member_type = decltype(std::declval<T &>().[:__nonstatic_data_members[I]:]) // Affected by access control
  
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

  alignas(__storage_traits.__max_align) std::byte __storage[__storage_traits.__max_size];
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
      std::construct_at(reinterpret_cast<__storage_type<I> *>(__storage), std::addressof(std::forward<Args>(args)...));
    } else {
      // 值成员（含const/volatile限定）：placement new直接构造，
      // cv限定类型是合法的new目标（如 ::new (p) const int(42)）
      std::construct_at(reinterpret_cast<__storage_type<I> *>(__storage), std::forward<Args>(args)...);
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

} // namespace std


#include <string>
#include <type_traits>

struct Widget {          // 非平凡析构
  int tag = 0;
  ~Widget() { tag = -1; }
};

struct All {             // POD=int，非POD=std::string
  int m0;
  volatile int m1;
  const int m2;
  volatile const int m3;
  int& m4;
  int&& m5;
  const int& m6;
  int* m7;
  std::string m8;
  volatile std::string m9;
  const std::string m10;
  volatile const std::string m11;
  std::string& m12;
  std::string&& m13;
  const std::string& m14;
  std::string* m15;
};

struct AllW {            // 自定义非平凡类型
  Widget w0;
  Widget& w1;
  Widget* w2;
};

template <class T>
struct R : std::__storage_base<T> {};

int main() {
  R<All> rs;
  int iv = 1;
  int iv2 = 2;
  std::string sv = "sv";
  std::string sv2 = "sv2";

  rs.construct<0>(42);            // int
  static_assert(std::is_same_v<decltype(rs.get<0>()), int&>);
  rs.get<0>() = 43;
  rs.destroy<0>();

  rs.construct<1>(7);             // volatile int
  static_assert(std::is_same_v<decltype(rs.get<1>()), volatile int&>);
  rs.destroy<1>();

  rs.construct<2>(8);             // const int
  static_assert(std::is_same_v<decltype(rs.get<2>()), const int&>);
  rs.destroy<2>();

  rs.construct<3>(9);             // volatile const int
  static_assert(std::is_same_v<decltype(rs.get<3>()), const volatile int&>);
  rs.destroy<3>();

  rs.construct<4>(iv);            // int&
  static_assert(std::is_same_v<decltype(rs.get<4>()), int&>);
  rs.get<4>() = 99;
  rs.destroy<4>();
  assert(iv == 99);

  rs.construct<5>(iv2);           // int&& 按左值绑定存指针
  static_assert(std::is_same_v<decltype(rs.get<5>()), int&>);
  rs.destroy<5>();

  rs.construct<6>(iv);            // const int&
  static_assert(std::is_same_v<decltype(rs.get<6>()), const int&>);
  rs.destroy<6>();

  rs.construct<7>(&iv);           // int*
  static_assert(std::is_same_v<decltype(rs.get<7>()), int*&>);
  *rs.get<7>() = 100;
  rs.destroy<7>();
  assert(iv == 100);

  rs.construct<8>(std::string("a"));   // string
  static_assert(std::is_same_v<decltype(rs.get<8>()), std::string&>);
  rs.get<8>() += "b";
  rs.destroy<8>();

  rs.construct<9>(std::string("v"));   // volatile string
  static_assert(std::is_same_v<decltype(rs.get<9>()), volatile std::string&>);
  rs.destroy<9>();

  rs.construct<10>(std::string("c"));  // const string
  static_assert(std::is_same_v<decltype(rs.get<10>()), const std::string&>);
  rs.destroy<10>();

  rs.construct<11>(std::string("vc")); // volatile const string
  static_assert(std::is_same_v<decltype(rs.get<11>()), const volatile std::string&>);
  rs.destroy<11>();

  rs.construct<12>(sv);            // string&
  static_assert(std::is_same_v<decltype(rs.get<12>()), std::string&>);
  rs.get<12>() += "x";
  rs.destroy<12>();
  assert(sv == "svx");

  rs.construct<13>(sv2);           // string&& 按左值绑定
  static_assert(std::is_same_v<decltype(rs.get<13>()), std::string&>);
  rs.destroy<13>();

  rs.construct<14>(sv);            // const string&
  static_assert(std::is_same_v<decltype(rs.get<14>()), const std::string&>);
  rs.destroy<14>();

  rs.construct<15>(&sv);           // string*
  static_assert(std::is_same_v<decltype(rs.get<15>()), std::string*&>);
  rs.get<15>()->append("y");
  rs.destroy<15>();
  assert(sv == "svxy");

  rs.construct<0>(1);              // const 对象访问
  R<All> const& crs = rs;
  static_assert(std::is_same_v<decltype(crs.get<0>()), const int&>);
  static_assert(std::is_same_v<decltype(crs.get<1>()), const volatile int&>);
  static_assert(std::is_same_v<decltype(crs.get<4>()), int&>);        // 引用成员不穿透const
  static_assert(std::is_same_v<decltype(crs.get<5>()), int&>);
  static_assert(std::is_same_v<decltype(crs.get<6>()), const int&>);
  static_assert(std::is_same_v<decltype(crs.get<7>()), int* const&>);
  static_assert(std::is_same_v<decltype(crs.get<8>()), const std::string&>);
  static_assert(std::is_same_v<decltype(crs.get<12>()), std::string&>);
  static_assert(std::is_same_v<decltype(crs.get<14>()), const std::string&>);
  rs.destroy<0>();

  static_assert(std::is_same_v<decltype(std::move(rs).get<0>()), int&&>);
  static_assert(std::is_same_v<decltype(std::move(rs).get<4>()), int&>);  // 引用成员不forward_like
  static_assert(std::is_same_v<decltype(std::move(rs).get<8>()), std::string&&>);

  // 自定义非平凡析构类型
  R<AllW> rw;
  Widget wt;
  wt.tag = 7;
  rw.construct<0>(Widget{});
  rw.destroy<0>();                 // 显式析构被调用
  rw.construct<1>(wt);             // Widget&
  static_assert(std::is_same_v<decltype(rw.get<1>()), Widget&>);
  rw.get<1>().tag = 8;             // 通过引用修改原对象
  rw.destroy<1>();
  assert(wt.tag == 8);
  rw.construct<2>(&wt);            // Widget*
  static_assert(std::is_same_v<decltype(rw.get<2>()), Widget*&>);
  rw.destroy<2>();
  assert(wt.tag == 8);             // 引用成员析构不影响被引用对象

  return 0;
}
