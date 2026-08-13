#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <meta>
#include <ranges>
#include <tuple>
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

  // ---- 一次template for求全部元数据：避免多次展开循环，加快编译 ----
  static constexpr auto __metadata = [] consteval noexcept {
    struct metadata {
      size_t __max_size = 0;
      size_t __max_align = 1;
      bool __is_all_mems_copy_assignment_nothrow = true;
      bool __is_all_mems_copy_constructor_nothrow = true;
      bool __is_all_mems_move_assignment_nothrow = true;
      bool __is_all_mems_move_constructor_nothrow = true;
      bool __is_all_mems_default_constructor_nothrow = true;
      bool __is_all_mems_equality_comparison_nothrow = true;
      bool __is_all_ref = true;                                // 引用niche：全引用
      size_t __tag_bits = std::numeric_limits<size_t>::digits; // 引用niche借用位数
      size_t __bool_cnt = 0;                                   // bool niche：bool成员数
      size_t __empty_cnt = 0;                                  // bool niche：空类成员数
      size_t __bool_index = 0;                                 // bool niche：bool成员索引
      std::array<size_t, __member_count> __empty_members{};    // bool niche：空变体成员序表（值→成员序反查）
      std::array<unsigned char, __member_count> __empty_code{}; // bool niche：成员→编码值正表（construct O(1)）
    } m;
    template for (constexpr auto I : std::views::iota(0zu, __member_count)) {
      using storage_t = __storage_type<I>;        // 引用成员适配后为指针
      using member_t = __member_type<I>;
      using member_plain_t = std::remove_cvref_t<member_t>;
      // 存储尺寸/对齐
      if (m.__max_size < sizeof(storage_t)) { m.__max_size = sizeof(storage_t); }
      if (m.__max_align < alignof(storage_t)) { m.__max_align = alignof(storage_t); }
      // nothrow traits：引用成员的操作实为指针拷贝，恒不抛；比较语义作用于被引用对象
      m.__is_all_mems_move_constructor_nothrow &= std::is_nothrow_move_constructible_v<storage_t>;
      m.__is_all_mems_default_constructor_nothrow &= std::is_nothrow_default_constructible_v<storage_t>;
      m.__is_all_mems_copy_constructor_nothrow &= std::is_nothrow_copy_constructible_v<storage_t>;
      m.__is_all_mems_copy_assignment_nothrow &= std::is_nothrow_copy_assignable_v<storage_t>;
      m.__is_all_mems_move_assignment_nothrow &= std::is_nothrow_move_assignable_v<storage_t>;
      m.__is_all_mems_equality_comparison_nothrow &=
          std::equality_comparable<member_plain_t> &&
          requires(const member_plain_t& __lhs, const member_plain_t& __rhs) { { __lhs == __rhs } noexcept; };
      m.__is_all_ref = m.__is_all_ref && std::is_reference_v<member_t>;
      // 引用niche：地址必被alignof整除（alignof为2的幂），低log2(alignof)位恒为0
      m.__tag_bits = std::min<size_t>(m.__tag_bits, static_cast<size_t>(std::countr_zero(
          alignof(std::remove_reference_t<member_t>))));
      // bool niche（Rust语义）：bool只借出{0,1}本征值，值域重叠时无法区分
      if constexpr (std::is_same_v<member_plain_t, bool>) {
        ++m.__bool_cnt;
        m.__bool_index = I;
      } else if constexpr (std::is_empty_v<member_plain_t>) {
        m.__empty_members[m.__empty_cnt] = I;
        m.__empty_code[I] = static_cast<unsigned char>(2 + m.__empty_cnt); // 当前计数即ordinal
        ++m.__empty_cnt;
      }
    }
    // 空类：零长度数组不合法，退化为1字节
    m.__max_size = m.__max_size ? m.__max_size : 1;
    return m;
  }();

  // ---- 派生元数据（无template for，直接取自__metadata）----
  static constexpr auto __storage_traits = [] consteval noexcept {
    struct storage_traits { size_t __max_size; size_t __max_align; };
    return storage_traits{__metadata.__max_size, __metadata.__max_align};
  }();
  static constexpr auto __nothrow_traits = [] consteval noexcept {
    struct nothrow_traits {
      bool __is_all_mems_copy_assignment_nothrow;
      bool __is_all_mems_copy_constructor_nothrow;
      bool __is_all_mems_move_assignment_nothrow;
      bool __is_all_mems_move_constructor_nothrow;
      bool __is_all_mems_default_constructor_nothrow;
      bool __is_all_mems_equality_comparison_nothrow;
    };
    return nothrow_traits{__metadata.__is_all_mems_copy_assignment_nothrow,
                          __metadata.__is_all_mems_copy_constructor_nothrow,
                          __metadata.__is_all_mems_move_assignment_nothrow,
                          __metadata.__is_all_mems_move_constructor_nothrow,
                          __metadata.__is_all_mems_default_constructor_nothrow,
                          __metadata.__is_all_mems_equality_comparison_nothrow};
  }();
  static constexpr size_t __tag_bits = __metadata.__tag_bits;
  static constexpr size_t __tag_mask = __tag_bits ? (1uz << __tag_bits) - 1 : 0;
  // niche开关（仿map::is_transparent）：用户类型内 using is_niche = void; 显式开启niche优化，
  // 默认不开启走tag版。类型别名非数据成员，反射与布局均不受影响，requires即可检测。
  static constexpr bool __niche_opt_in = requires { typename T::is_niche; };
  // 引用niche：全部引用成员（适配后指针强制非空 → null腾出）&& 对齐位容量足够（单成员bits=0亦可）
  static constexpr bool __niche_capable =
      __metadata.__is_all_ref && (__member_count <= (1uz << __tag_bits)) && __niche_opt_in;
  // bool niche：恰好1个bool成员 + 其余空类；编码空间 bool 2状态 + 每空变体1状态 + disengaged 1状态 ≤ 256
  static constexpr bool __bool_niche_capable =
      __metadata.__bool_cnt == 1 && (2 + __metadata.__empty_cnt + 1 <= 256) && __niche_opt_in;
  
  // 类型映射
  template <size_t... Is>
  static auto __member_tuple_impl(std::index_sequence<Is...>) -> std::tuple<__member_type<Is>...>;
  using __member_tuple_t = decltype(__member_tuple_impl(std::make_index_sequence<__member_count>{}));

  template <class U, class Tuple>
  struct __index_of_impl;

  template <class U, class... Ts>
  struct __index_of_impl<U, std::tuple<Ts...>> {
    // 精确匹配：U须与某个成员类型完全相等（含cvref），如 int&→int&成员、int→int值成员、const int&→const int&成员
    static constexpr size_t __count = (size_t{std::is_same_v<U, Ts>} + ...);
    template <size_t... Is>
    static constexpr size_t __find(std::index_sequence<Is...>) noexcept {
      size_t idx = __member_count;
      ((std::is_same_v<U, Ts> ? (idx = Is) : 0), ...);
      return idx;
    }
    static constexpr size_t __value = __count == 1
        ? __find(std::make_index_sequence<sizeof...(Ts)>{})
        : __member_count;
  };

  template <class U>
  static constexpr size_t __index_of = __index_of_impl<U, __member_tuple_t>::__value;

  // 类型表是否存在精确匹配U（含cvref）；匹配多个（如Amb{int a; int b;}）同样返回true
  template <class U>
  static constexpr bool __has_member_type = __index_of_impl<U, __member_tuple_t>::__count >= 1;

#pragma region
  constexpr size_t size() const noexcept {
    return __member_count;
  }
  
  constexpr size_t npos() const noexcept {
    return __member_count;
  }
#pragma endregion
};

// 辅助using：取T的第I个成员变量类型（对齐tuple_element_t风格：索引在前、类型在后）
template <size_t I, class T>
using __member_type_t = typename __storage_meta<T>::template __member_type<I>;

// ============ 主模板仅声明，约束特化按 __niche_capable 互斥选择 ============
template <class T>
struct __storage_base;

// ---- 通用版：byte数组 + 独立__tag（成员含值类型/普通指针/对齐位不足/多bool时回退）----
template <class T>
  requires (!__storage_meta<T>::__niche_capable && !__storage_meta<T>::__bool_niche_capable)
struct __storage_base<T> : __storage_meta<T>
{
  using __base = __storage_meta<T>;
  using __tag_type = typename __base::__tag_type;
  static constexpr size_t __member_count = __base::__member_count;
  static constexpr auto __storage_traits = __base::__storage_traits;
  template <size_t I>
  using __member_type = typename __base::template __member_type<I>;
  template <size_t I>
  using __storage_type = typename __base::template __storage_type<I>;

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

  template <class U>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(__base::template __index_of<U> != __member_count,
                  "get<U>: U must match exactly one member type");
    return self.template get<__base::template __index_of<U>>();
  }

  // get_if<I>：engaged且index==I时返回指向get<I>()结果的指针，否则nullptr；
  // 返回类型随self的const传播（const对象→const M*）；空类成员无存储可指，编译期拒绝
  template <size_t I>
  constexpr auto get_if(this auto &&self) noexcept {
    static_assert(I < __member_count, "Index out of range");
    static_assert(!std::is_empty_v<std::remove_reference_t<__member_type<I>>>,
                  "get_if: empty members have no addressable storage; use get<I>()");
    using pointer = std::add_pointer_t<decltype(self.template get<I>())>;
    if (self.index() != I) return pointer{nullptr};
    return std::addressof(self.template get<I>());
  }

  template <class U>
  constexpr auto get_if(this auto &&self) noexcept {
    static_assert(__base::template __index_of<U> != __member_count,
                  "get_if<U>: U must match exactly one member type");
    return self.template get_if<__base::template __index_of<U>>();
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

// ---- niche版：全引用成员，8字节uintptr_t槽，无__tag字段 ----
// 存储值 = 真实地址 | I（低 __tag_bits 位恒0，对齐保证）；disengaged = 0
template <class T>
  requires (__storage_meta<T>::__niche_capable)
struct __storage_base<T> : __storage_meta<T>
{
  using __base = __storage_meta<T>;
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

  template <class U>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(__base::template __index_of<U> != __member_count,
                  "get<U>: U must match exactly one member type");
    return self.template get<__base::template __index_of<U>>();
  }

  // get_if<I>：engaged且index==I时返回指向get<I>()结果的指针，否则nullptr
  template <size_t I>
  constexpr auto get_if(this auto &&self) noexcept {
    static_assert(I < __member_count, "Index out of range");
    using pointer = std::add_pointer_t<decltype(self.template get<I>())>;
    if (self.index() != I) return pointer{nullptr};
    return std::addressof(self.template get<I>());
  }

  template <class U>
  constexpr auto get_if(this auto &&self) noexcept {
    static_assert(__base::template __index_of<U> != __member_count,
                  "get_if<U>: U must match exactly one member type");
    return self.template get_if<__base::template __index_of<U>>();
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

// ---- bool niche版：恰好1个bool成员 + 其余空类，1字节槽，无__tag字段 ----
// 编码：bool成员 = 0/1（合法bool对象表示，get返回真bool&可写）；第j个空变体 = 2+j；disengaged = 成员数+1
// bool值域不随变体序号移位（Rust语义）：两个bool变体值域重叠 → 无法niche，回退tag版
template <class T>
  requires (__storage_meta<T>::__bool_niche_capable)
struct __storage_base<T> : __storage_meta<T>
{
  using __base = __storage_meta<T>;
  static constexpr size_t __member_count = __base::__member_count;
  template <size_t I>
  using __member_type = typename __base::template __member_type<I>;

  // bool成员索引（判定保证恰好1个）与空变体成员序表/编码正表（index()反查、construct O(1)用）
  // 均直接取自基类一次template for求得的__metadata，不再重复展开循环
  static constexpr size_t __bool_index = __base::__metadata.__bool_index;
  static constexpr auto __empty_members = __base::__metadata.__empty_members;
  static constexpr auto __empty_code = __base::__metadata.__empty_code;
  static constexpr unsigned char __disengaged = static_cast<unsigned char>(__member_count + 1);

  // unsigned char直接读写：bool成员engaged时经construct_at创建bool对象（字节恒0/1，永不触碰bool值域规则）
  unsigned char __slot = __disengaged;

#pragma region
  constexpr std::size_t index() const noexcept {
    // 0/1 → bool变体；2..k → 空变体（第s-2个）；k+1 → disengaged（== member_count）
    if (__slot <= 1) { return __bool_index; }
    if (__slot <= __member_count) { return __empty_members[__slot - 2]; }
    return __member_count;
  }

  constexpr bool has_value() const noexcept {
    return __slot != __disengaged;
  }

  template <size_t I>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(self.index() == I && "get: member is not the active one");
    if constexpr (I == __bool_index) {
      // 字节0/1是合法bool对象（construct_at创建），返回真引用可写；const不穿透存储对象
      using self_t = std::remove_reference_t<decltype(self)>;
      using bool_ptr_t = std::conditional_t<std::is_const_v<self_t>, const bool *, bool *>;
      return std::forward_like<decltype(self)>(*reinterpret_cast<bool_ptr_t>(&self.__slot));
    } else {
      // 空类成员无对象可引用：按值返回（无状态，读写无意义；同Rust unit变体）
      return std::remove_cvref_t<__member_type<I>>{};
    }
  }

  template <class U>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(__base::template __index_of<U> != __member_count,
                  "get<U>: U must match exactly one member type");
    return self.template get<__base::template __index_of<U>>();
  }

  // get_if<I>：engaged且index==I时返回指向get<I>()结果的指针，否则nullptr；
  // 空类成员按值返回无存储可指，编译期拒绝
  template <size_t I>
  constexpr auto get_if(this auto &&self) noexcept {
    static_assert(I < __member_count, "Index out of range");
    static_assert(I == __bool_index,
                  "get_if: empty members have no addressable storage; use get<I>()");
    using pointer = std::add_pointer_t<decltype(self.template get<I>())>;
    if (self.index() != I) return pointer{nullptr};
    return std::addressof(self.template get<I>());
  }

  template <class U>
  constexpr auto get_if(this auto &&self) noexcept {
    static_assert(__base::template __index_of<U> != __member_count,
                  "get_if<U>: U must match exactly one member type");
    return self.template get_if<__base::template __index_of<U>>();
  }

  template <size_t I, class... Args>
  constexpr decltype(auto) construct(Args&&...args) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(!has_value() && "construct: storage already engaged");
    if constexpr (I == __bool_index) {
      std::construct_at(reinterpret_cast<bool *>(&__slot), std::forward<Args>(args)...);
    } else {
      // 空变体（Rust unit语义）：无对象，直接写编码字节；I为编译期常量，正表索引即常量折叠
      static_assert(sizeof...(Args) == 0,
                    "construct for an empty member requires no arguments");
      __slot = __empty_code[I];
    }
    return (*this).template get<I>();
  }

  template <size_t I>
  constexpr void destroy() noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(index() == I && "destroy: member is not the active one");
    if constexpr (I == __bool_index) {
      std::destroy_at(reinterpret_cast<bool *>(&__slot));       // bool平凡析构，仅结束生命周期
    }
    __slot = __disengaged;
  }
#pragma endregion

};

} // namespace std
