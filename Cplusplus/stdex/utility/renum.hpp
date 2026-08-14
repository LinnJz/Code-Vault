#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <compare>
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
  
template <class T>
struct __storage_meta 
{
  static constexpr auto __nonstatic_data_members {
    std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))
  };
  static constexpr size_t __member_count = __nonstatic_data_members.size();
  using __tag_type = std::conditional_t<
      (__member_count <= 255), 
      std::uint8_t,
      std::conditional_t<
          (__member_count <= 65535), 
          std::uint16_t, 
          std::conditional_t<
              (__member_count <= 4294967295ULL), 
              std::uint32_t, 
              std::uint64_t
          >
      >
  >;

  template<size_t I>
  using __member_type = typename [: std::meta::type_of(__nonstatic_data_members[I]) :];
  template <size_t... Is>
  static auto __member_tuple_impl(std::index_sequence<Is...>) -> std::tuple<__member_type<Is>...>;
  using __member_types = decltype(__member_tuple_impl(std::make_index_sequence<__member_count>{}));
  
  template <class U, class Tuple>
  struct __index_of_impl;
  template <class U, class... Ts>
  struct __index_of_impl<U, std::tuple<Ts...>> {
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
  static constexpr size_t __index_of = __index_of_impl<U, __member_types>::__value;
  template <class U>
  static constexpr bool __has_member_type = __index_of_impl<U, __member_types>::__count >= 1;
  
  // 1. cannot directly store reference members in a byte array; 
  //    store M& as a "pointer to the referenced object"; 
  // 2. construct_at does not support const T*;
  //    value members are stripped of cv before storage,
  //    and const semantics are preserved __member_type API layer
  template <class M>
  struct __storage_of { using type = std::remove_cv_t<M>; };
  template <class M>
  struct __storage_of<M&>  { using type = std::remove_reference_t<M>*; };
  template <class M>
  struct __storage_of<M&&> { using type = std::remove_reference_t<M>*; };
  template <size_t I>
  using __storage_type = typename __storage_of<__member_type<I>>::type;

  static constexpr auto __metadata = [] consteval noexcept {
    struct metadata {
      size_t __max_size = 0;
      size_t __max_align = 1;
      bool __are_all_mems_copy_assignment_nothrow = true;
      bool __are_all_mems_copy_constructor_nothrow = true;
      bool __are_all_mems_move_assignment_nothrow = true;
      bool __are_all_mems_move_constructor_nothrow = true;
      bool __are_all_mems_default_constructor_nothrow = true;
      bool __are_all_mems_ref_qualified = true;
      bool __are_all_empty_type_aggregated = true;
      size_t __ref_tag_bits = std::numeric_limits<size_t>::digits;
      size_t __ref_tag_mask = 0;
      size_t __bool_type_index = 0;
      size_t __bool_type_cnt = 0;
      size_t __empty_type_cnt = 0;
      std::array<size_t, __member_count> __empty_type_indices{};
      std::array<unsigned char, __member_count> __empty_type_niche_tags{};
    } m;
    template for (constexpr auto I : std::views::iota(0zu, __member_count)) {
      using storage_t = __storage_type<I>;
      using storage_plain_t = std::remove_pointer_t<storage_t>;
      if (m.__max_size < sizeof(storage_t)) { m.__max_size = sizeof(storage_t); }
      if (m.__max_align < alignof(storage_t)) { m.__max_align = alignof(storage_t); }
      m.__are_all_mems_move_constructor_nothrow &= std::is_nothrow_move_constructible_v<storage_t>;
      m.__are_all_mems_default_constructor_nothrow &= std::is_nothrow_default_constructible_v<storage_t>;
      m.__are_all_mems_copy_constructor_nothrow &= std::is_nothrow_copy_constructible_v<storage_t>;
      m.__are_all_mems_copy_assignment_nothrow &= std::is_nothrow_copy_assignable_v<storage_t>;
      m.__are_all_mems_move_assignment_nothrow &= std::is_nothrow_move_assignable_v<storage_t>;
      m.__are_all_mems_ref_qualified &= std::is_reference_v<__member_type<I>>;
      if constexpr (std::is_reference_v<__member_type<I>> && requires { sizeof(storage_plain_t); }) { 
        // the address must be divisible by alignof (alignof is a power of 2),
        // with the lower log2(alignof) bits always being 0.
        m.__ref_tag_bits = std::min<size_t>(m.__ref_tag_bits, std::countr_zero(alignof(storage_plain_t)));
      } else if constexpr (std::is_reference_v<__member_type<I>>) {
        m.__are_all_mems_ref_qualified = false;  // points to an incomplete type, giving up the niche
      }
      if constexpr (std::is_same_v<storage_t, bool>) {
        m.__bool_type_index = I;
        ++m.__bool_type_cnt;
      } else if constexpr (std::is_empty_v<storage_t>) {
        m.__are_all_empty_type_aggregated &= std::is_aggregate_v<storage_t> && std::is_trivially_destructible_v<storage_t>;
        m.__empty_type_indices[m.__empty_type_cnt] = I;
        m.__empty_type_niche_tags[I] = static_cast<unsigned char>(2 + m.__empty_type_cnt);
        ++m.__empty_type_cnt;
      }
    }
    // empty class/zero-length arrays aren't allowed, so they end up as 1 byte
    m.__max_size = m.__max_size ? m.__max_size : 1;
    m.__ref_tag_mask = m.__ref_tag_bits >= std::numeric_limits<size_t>::digits
        ? 0 : (1uz << m.__ref_tag_bits) - 1;
    return m;
  }();

  static constexpr bool __niche_opt_in = requires { typename T::is_niche; };
  // ref niche: all referenced members
  static constexpr bool __niche_ref_capable =
      __niche_opt_in && __metadata.__are_all_mems_ref_qualified &&
      (__metadata.__ref_tag_bits >= std::numeric_limits<size_t>::digits ||
      __member_count <= (1uz << __metadata.__ref_tag_bits));
  // bool niche: exactly one bool member, the rest are empty classes.
  //             bool 2 states + each slot variant 1 state  + 1 disengaged state <= 256
  static constexpr bool __niche_bool_capable = 
      __niche_opt_in && __metadata.__are_all_empty_type_aggregated && __metadata.__bool_type_cnt == 1 &&
      (2 + __metadata.__empty_type_cnt + 1 <= 256) &&
      (__metadata.__bool_type_cnt + __metadata.__empty_type_cnt) == __member_count;

#pragma region
  constexpr size_t size() const noexcept {
    return __member_count;
  }
  
  constexpr size_t npos() const noexcept {
    return __member_count;
  }
#pragma endregion
};

template <size_t I, class T>
using __member_type_t = typename __storage_meta<T>::template __member_type<I>;

template <class T>
struct __storage_base;

template <class T>
requires (!__storage_meta<T>::__niche_ref_capable && !__storage_meta<T>::__niche_bool_capable)
struct __storage_base<T> : __storage_meta<T>
{
  using __base = __storage_meta<T>;
  using __tag_type = typename __base::__tag_type;
  template <size_t I>
  using __member_type = typename __base::template __member_type<I>;
  template <size_t I>
  using __storage_type = typename __base::template __storage_type<I>;
  using __base::__member_count;
  using __base::__metadata;

  alignas(__metadata.__max_align) std::byte __storage[__metadata.__max_size];
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
    using self_t = decltype(self);
    using storage_t = __storage_type<I>;
    using member_unref_t = std::remove_reference_t<__member_type<I>>;
    if constexpr (std::is_reference_v<__member_type<I>>) {
      return **reinterpret_cast<storage_t *>(self.__storage);

    } else {
      return std::forward_like<self_t>(*reinterpret_cast<member_unref_t *>(self.__storage));
    }
  }

  template <class U>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(__base::template __index_of<U> != __member_count,
                  "get<U>: U must match exactly one member type");
    return self.template get<__base::template __index_of<U>>();
  }

  template <size_t I, class... Args>
  constexpr decltype(auto) construct(Args&&...args) noexcept(
      std::is_reference_v<__member_type<I>> ||
      std::is_nothrow_constructible_v<__storage_type<I>, Args...>
  ) {
    static_assert(I < __member_count, "Index out of range");
    assert(!has_value() && "construct: storage already engaged");
    using storage_t = __storage_type<I>;
    if constexpr (std::is_reference_v<__member_type<I>>) {
      static_assert(sizeof...(Args) == 1 && (std::is_lvalue_reference_v<Args> && ...),
                    "construct for a reference member requires exactly one lvalue argument");
      static_assert(!(std::reference_constructs_from_temporary_v<__member_type<I>, Args> && ...),
                    "construct for a reference member would bind to a temporary");
      std::construct_at(reinterpret_cast<storage_t *>(__storage), std::addressof(std::forward<Args>(args)...));
    } else {
      std::construct_at(reinterpret_cast<storage_t *>(__storage), std::forward<Args>(args)...);
    }
    __tag = static_cast<__tag_type>(I);
    return (*this).template get<I>();
  }

  template <size_t I>
  constexpr void destroy() noexcept(std::is_nothrow_destructible_v<__storage_type<I>>) {
    static_assert(I < __member_count, "Index out of range");
    assert(index() == I && "destroy: member is not the active one");
    if constexpr (!std::is_reference_v<__member_type<I>> &&
                  !std::is_trivially_destructible_v<__storage_type<I>>) {
      std::destroy_at(reinterpret_cast<__storage_type<I> *>(__storage));
    }
    __tag = static_cast<__tag_type>(__member_count);
  }

#pragma endregion
};

/// niche version: all members are reference, and member_cnt <= min(std::countr_zero(alignof(M))...)
template <class T>
requires (__storage_meta<T>::__niche_ref_capable)
struct __storage_base<T> : __storage_meta<T>
{
  using __base = __storage_meta<T>;
  template <size_t I>
  using __member_type = typename __base::template __member_type<I>;
  using __base::__member_count;
  using __base::__metadata;
  
  // directly reading and writing uintptr_t objects: no strict aliasing issues
  // avoids UB from reinterpret_cast<std::byte* to uintptr_t*>
  std::uintptr_t __slot = 0;

#pragma region
  constexpr std::size_t index() const noexcept {
    // when there's only one member, __ref_tag_mask=0: engaged always returns 0;
    // disengaged returns __member_count
    return __slot == 0 ? __member_count
                       : static_cast<size_t>(__slot & __metadata.__ref_tag_mask);
  }

  constexpr bool has_value() const noexcept {
    return __slot != 0;
  }

  template <size_t I>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(self.index() == I && "get: member is not the active one");
    using storage_t = __storage_type<I>;
    return *reinterpret_cast<storage_t>(self.__slot & ~__metadata.__ref_tag_mask);
  }

  template <class U>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(__base::template __index_of<U> != __member_count,
                  "get<U>: U must match exactly one member type");
    return self.template get<__base::template __index_of<U>>();
  }

  template <size_t I, class... Args>
  constexpr decltype(auto) construct(Args&&...args) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(!has_value() && "construct: storage already engaged");
    static_assert(sizeof...(Args) == 1 && (std::is_lvalue_reference_v<Args> && ...),
                  "construct for a reference member requires exactly one lvalue argument");
    static_assert(!(std::reference_constructs_from_temporary_v<__member_type<I>, Args> && ...),
                  "construct for a reference member would bind to a temporary");
    auto addr = reinterpret_cast<std::uintptr_t>(std::addressof(std::forward<Args>(args)...));
    assert((addr & __metadata.__ref_tag_mask) == 0);  // alignment guarantee, defensive checks
    __slot = addr | I;
    return (*this).template get<I>();
  }

  template <size_t I>
  constexpr void destroy() noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(index() == I && "destroy: member is not the active one");
    __slot = 0; // disengaged
  }

#pragma endregion
};

/// bool niche version: exactly 1 bool member, the rest are empty classes/structs
template <class T>
requires (__storage_meta<T>::__niche_bool_capable)
struct __storage_base<T> : __storage_meta<T>
{
  using __base = __storage_meta<T>;
  template <size_t I>
  using __member_type = typename __base::template __member_type<I>;
  using __base::__member_count; // [1...254]
  using __base::__metadata;
  
  static constexpr unsigned char __disengaged = static_cast<unsigned char>(__member_count + 1);
  unsigned char __slot = __disengaged;

#pragma region
  constexpr std::size_t index() const noexcept {
    if (__slot <= 1) { return __metadata.__bool_type_index; }
    if (__slot <= __member_count) { return __metadata.__empty_type_indices[__slot - 2]; }
    return __member_count; // index, __member_count = __disengaged - 1
  }

  constexpr bool has_value() const noexcept {
    return __slot != __disengaged;
  }

  template <size_t I>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(self.index() == I && "get: member is not the active one");
    using self_t = decltype(self);
    using member_unref_t = std::remove_reference_t<__member_type<I>>;
    if constexpr (I == __metadata.__bool_type_index) {
      return std::forward_like<self_t>(*reinterpret_cast<member_unref_t *>(&self.__slot));
    } else {
      return __member_type<I>{};
    }
  }

  template <class U>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(__base::template __index_of<U> != __member_count,
                  "get<U>: U must match exactly one member type");
    return self.template get<__base::template __index_of<U>>();
  }
  
  template <size_t I, class... Args>
  constexpr decltype(auto) construct(Args&&...args) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(!has_value() && "construct: storage already engaged");
    if constexpr (I == __metadata.__bool_type_index) {
      std::construct_at(reinterpret_cast<bool *>(&__slot), std::forward<Args>(args)...);
    } else {
      static_assert(sizeof...(Args) == 0, // empty variant
                    "construct for an empty member requires no arguments");
      __slot = __metadata.__empty_type_niche_tags[I];
    }
    return (*this).template get<I>();
  }

  template <size_t I>
  constexpr void destroy() noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(index() == I && "destroy: member is not the active one");
    if constexpr (I == __metadata.__bool_type_index) {
      std::destroy_at(reinterpret_cast<bool *>(&__slot));
    }
    __slot = __disengaged;
  }

#pragma endregion

};

} // namespace std
