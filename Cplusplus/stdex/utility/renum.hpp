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

/// Reflection-driven metadata for T's non-static data members (P2996).
template <class T>
struct __storage_meta 
{
  static constexpr auto __nonstatic_data_members {
    std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))
  };
  static constexpr size_t __member_count = __nonstatic_data_members.size();
  /// Tag width sized to the member count.
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
  /// Index of the unique member of type U; __member_count serves as the
  /// sentinel when U is absent or matches more than one member.
  template <class U>
  static constexpr size_t __index_of = __index_of_impl<U, __member_types>::__value;
  template <class U>
  static constexpr bool __has_member_type = __index_of_impl<U, __member_types>::__count >= 1;
  
  /// Storage type mapping for each member type:
  /// - Reference members are stored as a pointer to the referenced object,
  ///   since a reference member cannot be stored in a byte array.
  /// - Value members are stored without cv-qualifiers, because
  ///   std::construct_at does not accept const T*; const semantics are
  ///   preserved at the __member_type API layer.
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
      bool __are_all_mems_three_way_comparable = true;
      bool __are_all_mems_compare_nothrow = true;
      bool __are_all_empty_type_aggregated = true;
      bool __are_all_mems_destructible = true;
      bool __are_all_mems_trivially_destructible = true;
      bool __are_all_mems_copy_constructible = true;
      bool __are_all_mems_trivially_copy_constructible = true;
      bool __are_all_mems_copy_assignable = true;
      bool __are_all_mems_trivially_copy_assignable = true;
      bool __are_all_mems_move_constructible = true;
      bool __are_all_mems_trivially_move_constructible = true;
      bool __are_all_mems_move_assignable = true;
      bool __are_all_mems_trivially_move_assignable = true;
      size_t __ref_tag_bits = std::numeric_limits<size_t>::digits;
      size_t __ref_tag_mask = 0;
      size_t __bool_type_index = 0;
      size_t __bool_type_cnt = 0;
      size_t __empty_type_cnt = 0;
      /// Code table: 0/1 denote bool (false/true), 2..N denote empty members
      /// in declaration order, and N+1 denotes the disengaged state;
      /// the valid codes are exactly 0..N+1, N+2 in total.
      std::array<size_t, __member_count + 2> __index_of_code;
    } m;
    template for (constexpr auto I : std::views::iota(0zu, __member_count)) {
      using storage_t = __storage_type<I>;
      using storage_plain_t = std::remove_pointer_t<storage_t>;
      using member_plain_t = std::remove_cvref_t<__member_type<I>>;
      if (m.__max_size < sizeof(storage_t)) { m.__max_size = sizeof(storage_t); }
      if (m.__max_align < alignof(storage_t)) { m.__max_align = alignof(storage_t); }
      m.__are_all_mems_copy_assignment_nothrow &= std::is_nothrow_copy_assignable_v<storage_t>;
      m.__are_all_mems_copy_constructor_nothrow &= std::is_nothrow_copy_constructible_v<storage_t>;
      m.__are_all_mems_move_assignment_nothrow &= std::is_nothrow_move_assignable_v<storage_t>;
      m.__are_all_mems_move_constructor_nothrow &= std::is_nothrow_move_constructible_v<storage_t>;
      m.__are_all_mems_default_constructor_nothrow &= std::is_nothrow_default_constructible_v<storage_t>;
      m.__are_all_mems_destructible &= std::is_destructible_v<__member_type<I>>;
      m.__are_all_mems_trivially_destructible &= std::is_trivially_destructible_v<__member_type<I>>;
      m.__are_all_mems_copy_constructible &= std::is_copy_constructible_v<__member_type<I>>;
      m.__are_all_mems_trivially_copy_constructible &= std::is_trivially_copy_constructible_v<__member_type<I>>;
      m.__are_all_mems_copy_assignable &= std::is_copy_constructible_v<__member_type<I>> && std::is_copy_assignable_v<__member_type<I>>;
      m.__are_all_mems_trivially_copy_assignable &= std::is_trivially_copy_constructible_v<__member_type<I>> && std::is_trivially_copy_assignable_v<__member_type<I>>;
      m.__are_all_mems_move_constructible &= std::is_move_constructible_v<__member_type<I>>;
      m.__are_all_mems_trivially_move_constructible &= std::is_trivially_move_constructible_v<__member_type<I>>;
      m.__are_all_mems_move_assignable &= std::is_move_constructible_v<__member_type<I>> && std::is_move_assignable_v<__member_type<I>>;
      m.__are_all_mems_trivially_move_assignable &= std::is_trivially_move_constructible_v<__member_type<I>> && std::is_trivially_move_assignable_v<__member_type<I>>;
      m.__are_all_mems_ref_qualified &= std::is_reference_v<__member_type<I>>;
      
      if constexpr (std::is_reference_v<__member_type<I>> && requires { sizeof(storage_plain_t); }) { 
        /// An aligned address is a multiple of alignof (a power of two),
        /// so its low log2(alignof) bits are always zero.
        m.__ref_tag_bits = std::min<size_t>(m.__ref_tag_bits, std::countr_zero(alignof(storage_plain_t)));
      } else if constexpr (std::is_reference_v<__member_type<I>>) {
        m.__are_all_mems_ref_qualified = false;  ///< Points to an incomplete type; the niche is abandoned.
      }
      
      if constexpr (std::is_reference_v<__member_type<I>> && !requires { sizeof(member_plain_t); }) {
          m.__are_all_mems_three_way_comparable = false;
          m.__are_all_mems_compare_nothrow = false;
      } else if constexpr (!std::is_empty_v<member_plain_t>) {
          m.__are_all_mems_three_way_comparable &= std::three_way_comparable<member_plain_t>;
          m.__are_all_mems_compare_nothrow &= std::three_way_comparable<member_plain_t> &&
              requires (member_plain_t const &a, member_plain_t const &b) { { a <=> b } noexcept; };
      }
      
      if constexpr (std::is_same_v<storage_t, bool>) {
        m.__bool_type_index = I;
        ++m.__bool_type_cnt;
      } else if constexpr (std::is_empty_v<storage_t>) {
        m.__are_all_empty_type_aggregated &= std::is_aggregate_v<storage_t> && std::is_trivially_destructible_v<storage_t>;
        ++m.__empty_type_cnt;
        /// Empty members are coded 2..N in declaration order; each bool
        /// member scanned before I shifts the code by one.
        m.__index_of_code[2 + I - m.__bool_type_cnt] = I;
      }
      
    }
    /// Code 0/1 = bool (false/true); N+1 = disengaged.
    m.__index_of_code[0] = m.__index_of_code[1] = m.__bool_type_index;
    m.__index_of_code[__member_count + 1] = __member_count;
    
    /// Empty members contribute zero size; clamp to 1 byte, since a
    /// zero-length storage array is not permitted.
    m.__max_size = m.__max_size ? m.__max_size : 1;
    m.__ref_tag_mask = m.__ref_tag_bits >= std::numeric_limits<size_t>::digits
        ? 0 : (1uz << m.__ref_tag_bits) - 1;
    return m;
  }();

  static constexpr bool __niche_opt_in = requires { typename T::is_niche; };
  /// Reference niche: every member is a reference.
  static constexpr bool __niche_ref_capable =
      __niche_opt_in && __metadata.__are_all_mems_ref_qualified &&
      (__metadata.__ref_tag_bits >= std::numeric_limits<size_t>::digits ||
      __member_count <= (1uz << __metadata.__ref_tag_bits));
  /// Bool niche: exactly one bool member; all others are empty classes.
  /// The bool's two states, one state per empty member, and the disengaged
  /// state must fit into a single byte (N+2 <= 256).
  static constexpr bool __niche_bool_capable = 
      __niche_opt_in && __metadata.__are_all_empty_type_aggregated && __metadata.__bool_type_cnt == 1 &&
      (2 + __metadata.__empty_type_cnt + 1 <= 256) &&
      (__metadata.__bool_type_cnt + __metadata.__empty_type_cnt) == __member_count;
};

template <size_t I, class T>
using __member_type_t = typename __storage_meta<T>::template __member_type<I>;

template <class T>
struct __storage_base;

/// Default storage: a byte array holding the active member, tagged by __tag.
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
  using __base::__nonstatic_data_members;
  using __base::__member_count;
  using __base::__metadata;


/// GCC has not yet implemented the P2996R13 feature of injecting empty
/// destructors, so a std::byte array with reinterpret_cast is used for now.
/// Non-POD types are supported, though constant evaluation is limited.
#ifndef P2996R13_meta_reflection_define_aggregate
  alignas(__metadata.__max_align) std::byte __storage[__metadata.__max_size]{};
#else
  /// The reflection-defined union path supports POD types only.
  union __union;
  consteval {
    std::meta::define_aggregate(^^__union, [] consteval noexcept /* std::span */ {
      std::array<std::meta::info, __member_count> members;
      template for (constexpr auto I : std::views::iota(0zu, __member_count)) {
        members[I] = std::meta::data_member_spec(^^__storage_type<I>, { 
          .name = std::meta::identifier_of(__nonstatic_data_members[I]) 
        });
      }
      return members;
    }());
  }
  __union __u{};

  template <size_t I>
  static consteval std::meta::info __member_refl() noexcept {
    return std::meta::nonstatic_data_members_of(^^__union, std::meta::access_context::unchecked())[I];
  }
#endif
  __tag_type __tag { static_cast<__tag_type>(__member_count) };  ///< Default: disengaged (tag = __member_count).

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
    using member_unref_t = std::remove_reference_t<__member_type<I>>;
    if constexpr (std::is_reference_v<__member_type<I>>) {
#ifdef P2996R13_meta_reflection_define_aggregate
      return *(self.__u.[: __member_refl<I>() :]);
#else
      return **reinterpret_cast<std::add_const_t<__storage_type<I>> *>(self.__storage);
#endif
    } else {
      using cv_member_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>,
                                             std::add_const_t<member_unref_t>, member_unref_t>;
#ifdef P2996R13_meta_reflection_define_aggregate
      return std::forward_like<self_t>(*static_cast<cv_member_t *>(std::addressof(self.__u.[: __member_refl<I>() :])));
#else
      return std::forward_like<self_t>(*reinterpret_cast<cv_member_t *>(self.__storage));
#endif
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
      (std::is_array_v<__storage_type<I>>
         ? (sizeof...(Args) == 0 ||
            (std::is_nothrow_constructible_v<std::remove_extent_t<__storage_type<I>>, Args> && ...))
         : std::is_nothrow_constructible_v<__storage_type<I>, Args...>)
  ) {
    static_assert(I < __member_count, "Index out of range");
    assert(!has_value() && "construct: storage already engaged");
    using storage_t = __storage_type<I>;
    if constexpr (std::is_reference_v<__member_type<I>>) {
      static_assert(sizeof...(Args) == 1 && (std::is_lvalue_reference_v<Args> && ...),
                    "construct for a reference member requires exactly one lvalue argument");
      static_assert(!(std::reference_constructs_from_temporary_v<__member_type<I>, Args> && ...),
                    "construct for a reference member would bind to a temporary");
#ifdef P2996R13_meta_reflection_define_aggregate
      std::construct_at(std::addressof(__u.[: __member_refl<I>() :]),
                        std::addressof(std::forward<Args>(args)...));
#else
      std::construct_at(reinterpret_cast<storage_t *>(__storage), std::addressof(std::forward<Args>(args)...));
#endif
    } else if constexpr (std::is_array_v<storage_t>) {
    /// [specialized.construct]/2 (N5032) forbids construct_at with arguments
    /// for array types, so arrays are initialized via braced placement new:
    /// each argument value-initializes one element in place.
    static_assert(sizeof...(Args) == 0 || sizeof...(Args) == std::extent_v<storage_t>,
                  "array member: pass exactly one initializer per element, or none for value-initialization");
#ifdef P2996R13_meta_reflection_define_aggregate
    ::new (static_cast<void *>(std::addressof(__u.[: __member_refl<I>() :]))) storage_t{std::forward<Args>(args)...};
#else
    ::new (static_cast<void *>(__storage)) storage_t{std::forward<Args>(args)...};
#endif
    } else {
#ifdef P2996R13_meta_reflection_define_aggregate
      std::construct_at(std::addressof(__u.[: __member_refl<I>() :]), std::forward<Args>(args)...);
#else
      std::construct_at(reinterpret_cast<storage_t *>(__storage), std::forward<Args>(args)...);
#endif
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
#ifdef P2996R13_meta_reflection_define_aggregate
      std::destroy_at(std::addressof(__u.[: __member_refl<I>() :]));
#else
      std::destroy_at(reinterpret_cast<__storage_type<I> *>(__storage));
#endif
    }
    __tag = static_cast<__tag_type>(__member_count);
  }

#pragma endregion
};

/// Reference-niche specialization: every member is a reference, and the
/// member count fits within the alignment-guaranteed zero bits of an
/// address (member_cnt <= 2^__ref_tag_bits).
template <class T>
requires (__storage_meta<T>::__niche_ref_capable)
struct __storage_base<T> : __storage_meta<T>
{
  using __base = __storage_meta<T>;
  template <size_t I>
  using __member_type = typename __base::template __member_type<I>;
  template <size_t I>
  using __storage_type = typename __base::template __storage_type<I>;
  using __base::__member_count;
  using __base::__metadata;
  
  /// The pointer and tag are stored in a single uintptr_t slot, read and
  /// written as a real object; this avoids the strict-aliasing UB that a
  /// reinterpret_cast from std::byte* to uintptr_t* would entail.
  std::uintptr_t __slot = 0;

#pragma region
  constexpr std::size_t index() const noexcept {
    /// With a single member, __ref_tag_mask == 0: an engaged slot always
    /// yields index 0, while a disengaged slot yields __member_count.
    return __slot == 0 ? __member_count
                       : static_cast<size_t>(__slot & __metadata.__ref_tag_mask);
  }

  constexpr bool has_value() const noexcept {
    return __slot != 0;
  }

  template <size_t I>
  decltype(auto) get(this auto &&self) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(self.index() == I && "get: member is not the active one");
    /// Clears the tag bits and dereferences the stored pointer.
    return *reinterpret_cast<__storage_type<I>>(self.__slot & ~__metadata.__ref_tag_mask);
  }

  template <class U>
  decltype(auto) get(this auto &&self) noexcept {
    static_assert(__base::template __index_of<U> != __member_count,
                  "get<U>: U must match exactly one member type");
    return self.template get<__base::template __index_of<U>>();
  }

  template <size_t I, class... Args>
  decltype(auto) construct(Args&&...args) noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(!has_value() && "construct: storage already engaged");
    static_assert((std::is_same_v<std::remove_cvref_t<Args>,
                              std::remove_cvref_t<__member_type<I>>> && ...),
                  "argument type must match the reference member type");
    static_assert(sizeof...(Args) == 1 && (std::is_lvalue_reference_v<Args> && ...),
                  "construct for a reference member requires exactly one lvalue argument");
    static_assert(!(std::reference_constructs_from_temporary_v<__member_type<I>, Args> && ...),
                  "construct for a reference member would bind to a temporary");
    auto addr = reinterpret_cast<std::uintptr_t>(std::addressof(std::forward<Args>(args)...));
    assert((addr & __metadata.__ref_tag_mask) == 0);  ///< Alignment guarantee (defensive check).
    __slot = addr | I;
    return (*this).template get<I>();
  }

  template <size_t I>
  constexpr void destroy() noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(index() == I && "destroy: member is not the active one");
    __slot = 0;  ///< Disengaged.
  }

#pragma endregion
};

/// Bool-niche specialization: exactly one bool member; all others are empty classes.
template <class T>
requires (__storage_meta<T>::__niche_bool_capable)
struct __storage_base<T> : __storage_meta<T>
{
  using __base = __storage_meta<T>;
  template <size_t I>
  using __member_type = typename __base::template __member_type<I>;
  using __base::__member_count;  ///< Fits one byte (codes 0..255): at most 254
                                 ///< members, leaving codes for the bool states
                                 ///< and the disengaged state.
  using __base::__metadata;
  
  static constexpr unsigned char __disengaged = static_cast<unsigned char>(__member_count + 1);

  /// Code 0/1 are reserved for bool; empty members start at 2, skipping
  /// the bool slot.
  static constexpr unsigned char __code_of_member(size_t I) noexcept {
    return static_cast<unsigned char>(2 + I - (I > __metadata.__bool_type_index));
  }
  
  union __union {
    bool __bool_value;
    unsigned char __state_code = __disengaged;
  } __u;

#pragma region
  /// O(1) lookup of the member index via the state-code table.
  std::size_t index() const noexcept {
    return __metadata.__index_of_code[std::bit_cast<unsigned char>(__u)];
  }

  bool has_value() const noexcept {
    return std::bit_cast<unsigned char>(__u) != __disengaged;
  }

  template <size_t I>
  constexpr decltype(auto) get(this auto &&self) noexcept {
      static_assert(I < __member_count, "Index out of range");
      assert(self.index() == I && "get: member is not the active one");
      if constexpr (I == __metadata.__bool_type_index) {
          return std::forward_like<decltype(self)>(self.__u.__bool_value);
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
      std::construct_at(&__u.__bool_value, std::forward<Args>(args)...);
    } else {
      static_assert(sizeof...(Args) == 0 ||
                    (sizeof...(Args) == 1 &&
                     (std::is_same_v<std::remove_cvref_t<Args>,
                                     std::remove_cv_t<__member_type<I>>> && ...)),
                    "construct for an empty member requires no arguments");
      std::construct_at(&__u.__state_code, __code_of_member(I));
    }
    return (*this).template get<I>();
  }

  template <size_t I>
  constexpr void destroy() noexcept {
    static_assert(I < __member_count, "Index out of range");
    assert(index() == I && "destroy: member is not the active one");
    std::construct_at(&__u.__state_code, __disengaged);
  }

#pragma endregion

};

/// Dispatches f over the active member index; used by member_variant's
/// special members.
template <class Self, class F>
constexpr decltype(auto) __at_index(Self&& self, F&& f) {
  constexpr size_t N = std::remove_cvref_t<Self>::__member_count;
  return [&]<size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
    using R = std::common_reference_t<decltype(f(std::integral_constant<size_t, Is>{}))...>;
    auto dispatch = [&]<size_t I>(this auto&& d) -> R {
      if constexpr (I == N) {
        std::unreachable();  ///< Disengaged.
      } else if (self.index() == I) {
        return f(std::integral_constant<size_t, I>{});
      } else {
        return d.template operator()<I + 1>();
      }
    };
    return dispatch.template operator()<0>();
  }(std::make_index_sequence<N>{});
}

/// Variant-like wrapper holding exactly one non-static data member of T;
/// special members follow std::variant's P0848 conditional-trivial rules.
template <class T>
class member_variant {
  using __meta = __storage_meta<T>;
  static_assert(__meta::__member_count > 0 && 
      []<std::size_t... Is>(std::index_sequence<Is...>) constexpr noexcept -> bool {
        return (std::is_destructible_v<std::tuple_element_t<Is, typename __meta::__member_types>> && ...);
      }(std::make_index_sequence<__meta::__member_count>{}),
      "variant_builder<T>: all non-static data members of T must satisfy Cpp17Destructible "
      "requirements (N4950 [variant.variant.general]/2).");
  
public:
  constexpr member_variant() = default;

  /// ---- destructor ----
  constexpr ~member_variant() requires (__meta::__metadata.__are_all_mems_trivially_destructible) = default;
  constexpr ~member_variant() requires (!__meta::__metadata.__are_all_mems_trivially_destructible &&
                               __meta::__metadata.__are_all_mems_destructible) {
    if (__s.has_value()) {
      __at_index(__s, [&]<size_t I>(std::integral_constant<size_t, I>) 
          noexcept(noexcept(__s.template destroy<I>())) 
          -> void {
        __s.template destroy<I>();
      });
    }
  }
  constexpr ~member_variant() requires (!__meta::__metadata.__are_all_mems_destructible) = delete;

  /// ---- copy constructor ----
  constexpr member_variant(const member_variant&) 
      requires (__meta::__metadata.__are_all_mems_trivially_copy_constructible)
      = default;
  constexpr member_variant(const member_variant& other)
      noexcept(__meta::__metadata.__are_all_mems_copy_constructor_nothrow)
      requires (!__meta::__metadata.__are_all_mems_trivially_copy_constructible &&
                __meta::__metadata.__are_all_mems_copy_constructible)
      : __s() {
    if (other.__s.has_value()) {
      __at_index(other.__s, [&]<size_t I>(std::integral_constant<size_t, I>) 
          noexcept(noexcept(__s.template construct<I>(other.__s.template get<I>()))) 
          -> void {
        __s.template construct<I>(other.__s.template get<I>());
      });
    }
  }
  constexpr member_variant(const member_variant&) 
      requires (!__meta::__metadata.__are_all_mems_copy_constructible)
      = delete;

  /// ---- move constructor ----
  constexpr member_variant(member_variant&&) 
      requires (__meta::__metadata.__are_all_mems_trivially_move_constructible) 
      = default;
  constexpr member_variant(member_variant&& other)
      noexcept(__meta::__metadata.__are_all_mems_move_constructor_nothrow)
      requires (!__meta::__metadata.__are_all_mems_trivially_move_constructible &&
                __meta::__metadata.__are_all_mems_move_constructible)
      : __s() {
    if (other.__s.has_value()) {
      __at_index(other.__s, [&]<size_t I>(std::integral_constant<size_t, I>) noexcept(
          std::is_reference_v<typename __meta::template __member_type<I>> ||
          std::is_nothrow_move_constructible_v<typename __meta::template __storage_type<I>>) 
          -> void {
        if constexpr (std::is_reference_v<typename __meta::template __member_type<I>>) {
          __s.template construct<I>(other.__s.template get<I>());  ///< Reference members are always copied.
        } else {
          __s.template construct<I>(std::move(other.__s.template get<I>()));
        }
      });
    }
  }
  constexpr member_variant(member_variant&&) 
      requires (!__meta::__metadata.__are_all_mems_move_constructible) 
      = delete;

  /// ---- copy assignment ----
  constexpr member_variant& operator=(const member_variant&) 
      requires (__meta::__metadata.__are_all_mems_trivially_copy_assignable) 
      = default;
  constexpr member_variant& operator=(const member_variant& other)
      noexcept(__meta::__metadata.__are_all_mems_copy_assignment_nothrow &&
               __meta::__metadata.__are_all_mems_copy_constructor_nothrow)
      requires (!__meta::__metadata.__are_all_mems_trivially_copy_assignable &&
                __meta::__metadata.__are_all_mems_copy_assignable) {
    if (__s.index() == other.__s.index()) {
      if (__s.has_value()) {
        __at_index(__s, [&]<size_t I>(std::integral_constant<size_t, I>) 
          noexcept(noexcept(__s.template get<I>() = other.__s.template get<I>())) 
          -> void {
          __s.template get<I>() = other.__s.template get<I>();
        });
      }
    } else {
      if (__s.has_value()) {
        __at_index(__s, [&]<size_t I>(std::integral_constant<size_t, I>) 
          noexcept(noexcept(__s.template destroy<I>())) 
          -> void {
          __s.template destroy<I>();
        });
      }
      if (other.__s.has_value()) {
        __at_index(other.__s, [&]<size_t I>(std::integral_constant<size_t, I>) 
          noexcept(noexcept(__s.template construct<I>(other.__s.template get<I>()))) 
          -> void {
          __s.template construct<I>(other.__s.template get<I>());
        });
      }
    }
    return *this;
  }
  constexpr member_variant& operator=(const member_variant&) 
      requires (!__meta::__metadata.__are_all_mems_copy_assignable) 
      = delete;

  /// ---- move assignment ----
  constexpr member_variant& operator=(member_variant&&) 
      requires (__meta::__metadata.__are_all_mems_trivially_move_assignable) 
      = default;
  constexpr member_variant& operator=(member_variant&& other)
      noexcept(__meta::__metadata.__are_all_mems_move_assignment_nothrow &&
               __meta::__metadata.__are_all_mems_move_constructor_nothrow)
      requires (!__meta::__metadata.__are_all_mems_trivially_move_assignable &&
                __meta::__metadata.__are_all_mems_move_assignable) {
    if (__s.index() == other.__s.index()) {
      if (__s.has_value()) {
        __at_index(__s, [&]<size_t I>(std::integral_constant<size_t, I>) 
          noexcept(noexcept(__s.template get<I>() = std::move(other.__s.template get<I>()))) 
          -> void {
          __s.template get<I>() = std::move(other.__s.template get<I>());
        });
      }
    } else {
      if (__s.has_value()) {
        __at_index(__s, [&]<size_t I>(std::integral_constant<size_t, I>) 
          noexcept(noexcept(__s.template destroy<I>())) 
          -> void {
          __s.template destroy<I>();
        });
      }
      if (other.__s.has_value()) {
        __at_index(other.__s, [&]<size_t I>(std::integral_constant<size_t, I>) noexcept(
            std::is_reference_v<typename __meta::template __member_type<I>> ||
            std::is_nothrow_move_constructible_v<typename __meta::template __storage_type<I>>) 
            -> void {
          if constexpr (std::is_reference_v<typename __meta::template __member_type<I>>) {
            __s.template construct<I>(other.__s.template get<I>());
          } else {
            __s.template construct<I>(std::move(other.__s.template get<I>()));
          }
        });
      }
    }
    return *this;
  }
  constexpr member_variant& operator=(member_variant&&) 
      requires (!__meta::__metadata.__are_all_mems_move_assignable) 
      = delete;

  constexpr std::size_t size() const noexcept { return __meta::__member_count; }
  constexpr bool has_value() const noexcept { return __s.has_value(); }
  constexpr std::size_t index() const noexcept { return __s.index(); }

  template <size_t I>
  constexpr decltype(auto) get(this auto&& self) noexcept
      requires (I < __meta::__member_count) {
    return std::forward_like<decltype(self)>(self).__s.template get<I>();
  }
  template <class U>
  constexpr decltype(auto) get(this auto&& self) noexcept
      requires (__meta::template __index_of<U> != __meta::__member_count) {
    return std::forward_like<decltype(self)>(self).__s.template get<U>();
  }

  /// Destroys the active member, then constructs index I in place.
  template <size_t I, class... Args>
  constexpr decltype(auto) emplace(Args&&... args) noexcept(
      std::is_reference_v<typename __meta::template __member_type<I>> ||
      std::is_nothrow_constructible_v<typename __meta::template __storage_type<I>, Args...>)
      requires (I < __meta::__member_count) {
    if (__s.has_value()) {
      __at_index(__s, [&]<size_t J>(std::integral_constant<size_t, J>) 
        noexcept(noexcept(__s.template destroy<J>())) 
        -> void {
        __s.template destroy<J>();
      });
    }
    return __s.template construct<I>(std::forward<Args>(args)...);
  }

  /// ---- The following three members are declared only; their bodies
  /// ---- are implemented elsewhere. ----
  constexpr auto operator<=>(const member_variant& other) const
      noexcept(__meta::__metadata.__are_all_mems_compare_nothrow)
      requires (__meta::__metadata.__are_all_mems_three_way_comparable);

  template <class F>
  constexpr decltype(auto) visit(this auto&& self, F&& f);

  constexpr void swap(member_variant& other) noexcept(
      __meta::__metadata.__are_all_mems_move_constructor_nothrow &&
      __meta::__metadata.__are_all_mems_move_assignment_nothrow);
      
private:
  __storage_base<T> __s;
};

} // namespace std
