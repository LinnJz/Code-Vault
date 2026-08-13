#pragma once

#include <meta>


namespace std { 
  
consteval bool __is_pod__(std::meta::info type_info) noexcept {
  return std::meta::is_trivially_destructible_type(type_info) ||
         std::meta::is_trivially_default_constructible_type(type_info);
}
// 反射遍历成员，获取最大类型进行对齐，统一使用byte存储，不用union
// 不管是pod还是非pod统一construct_at placement new
template <class T>
struct __box 
{
  [[nodiscard]] constexpr decltype(auto) value(this auto &&self) noexcept {
    using Self = std::remove_reference_t<decltype(self)>;
    using Ptr = std::conditional_t<std::is_const_v<Self>, T const *, T *>;
    return std::forward_like<decltype(self)>(*reinterpret_cast<Ptr>(self.__buffer));
  }
  
  template <class... Args>
  constexpr T &construct(Args&&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    return *std::construct_at(reinterpret_cast<T *>(__buffer), std::forward<Args>(args)...);
  }
  
  constexpr void destroy() noexcept {
    std::destroy_at(reinterpret_cast<T *>(__buffer));
  }
  
  alignas(T) std::byte __buffer[sizeof(T)];
};

template <class T>
struct __storage_base_impl
{
  static constexpr auto __nonstatic_data_members {
    std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()))
  };
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
  
  static constexpr auto __nothrow_traits = [] consteval noexcept {
    struct nothrow_traits {
      bool __is_all_mems_copy_assignment_nothrow = true;
      bool __is_all_mems_copy_constructor_nothrow = true;
      bool __is_all_mems_move_assignment_nothrow = true;
      bool __is_all_mems_move_constructor_nothrow = true;
      bool __is_all_mems_default_constructor_nothrow = true;
      bool __is_all_mems_equality_comparison_nothrow = true;
    } traits;
    for (auto i = 0uz; i < __member_count; ++i) {
        using member_type = decltype(std::declval<T &>().[:__nonstatic_data_members[i]:]);
        using member_remove_cvref_t = std::remove_cvref_t<member_type>;
        traits.__is_all_mems_move_constructor_nothrow &= 
            std::is_nothrow_move_constructible_v<member_remove_cvref_t>;
        traits.__is_all_mems_default_constructor_nothrow &= 
            std::is_nothrow_default_constructible_v<member_remove_cvref_t>;
        traits.__is_all_mems_copy_constructor_nothrow &= 
            std::is_nothrow_copy_constructible_v<member_remove_cvref_t>;
        traits.__is_all_mems_copy_assignment_nothrow &= 
            std::is_nothrow_copy_assignable_v<member_remove_cvref_t>;
        traits.__is_all_mems_move_assignment_nothrow &= 
            std::is_nothrow_move_assignable_v<member_remove_cvref_t>;
        traits.__is_all_mems_equality_comparison_nothrow &= 
            std::equality_comparable<member_remove_cvref_t> && 
                noexcept(std::declval<const member_remove_cvref_t&>() == 
                         std::declval<const member_remove_cvref_t&>());
    }
    return traits;
  };
  
  constexpr std::size_t index() const noexcept {
    return static_cast<size_t>(__tag);
  }
  
  constexpr bool has_value() const noexcept {
    return static_cast<size_t>(__tag) != __member_count;
  }
  
  template <size_t I>
  constexpr decltype(auto) get(this auto &&self) noexcept {
    static_assert(I < __member_count, "Index out of range");
    if constexpr (__is_pod__(std::meta::type_of(__nonstatic_data_members[i]))) {
      return std::forward_like<decltype(self)>(self.__storage.[:__nonstatic_data_members[I]:]);
    } else {
      return std::forward_like<decltype(self)>(self.__storage.[:__nonstatic_data_members[I]:].value());
    }
  }
  
  template <size_t I, class... Args>
  constexpr decltype(auto) construct(Args&&...args) noexcept(
      std::is_nothrow_constructible_v<
          std::remove_cvref_t<
              decltype(std::declval<std::meta::type_of([:__nonstatic_data_members[I]:]) &>().[:__nonstatic_data_members[I]:])
          >,
          Args...
      >
  ) {
    static_assert(I < __member_count, "Index out of range");
    __tag = static_cast<__tag_type>(I);
    if constexpr (__is_pod__(std::meta::type_of(__nonstatic_data_members[i]))) {
      return std::construct_at(&(__storage.[:__nonstatic_data_members[I]:]), std::forward<Args>(args)...);
    } else {
      return __storage.[:__nonstatic_data_members[I]:].construct(std::forward<Args>(args)...);
    }
  }
  
  template <size_t I>
  constexpr void destroy() noexcept {
    static_assert(I < __member_count, "Index out of range");
    if constexpr (!__is_pod__(std::meta::type_of(__nonstatic_data_members[i]))) {
      __storage.[:__nonstatic_data_members[I]:].destroy();
    }
    __tag = static_cast<__tag_type>(__member_count);
  }  
   
  union __storage_type;
  consteval { 
    std::meta::define_aggregate(^^__storage_type, [] consteval noexcept {
      std::array<std::meta::info, __member_count> members;
      for (auto i = 0uz, i < __member_count; ++i) {
        auto type_info = std::meta::type_of(__nonstatic_data_members[i]);
        auto member_wrap_info = __is_pod__(type_info)
                                ? type_info
                                : std::meta::substitute(^^__box, { type_info });
        // 1. struct Rect {...} rect;
        // 2. struct {...} rect;
        members[i] = std::meta::data_member_spec(member_wrap_info, {
          .name = std::meta::identifier_of(std::meta::has_identifier(type_info)
                    ? type_info
                    : __nonstatic_data_members[i]);
        });
      }
      return members;
    });
  }
  __storage_type __storage;
  __tag_type __tag { static_cast<__tag_type>(__member_count) };
};



template <class T>
struct __storage_base 
     : __storage_base_impl<T, __storage_traits<T>::__count, __storage_traits<T>::__is_niche>
{
};
  
template <class T>
class variant_builder : public __storage_base<T>
{
  constexpr variant_builder() noexcept {
    
  }
  
  constexpr variant_builder(variant_builder const &other) noexcept(?) {
    
  }
  
  constexpr variant_builder(variant_builder &&other) noexcept {
    
  }
  
  template <class T>
  constexpr variant_builder(T&& t) noexcept(?) {
    
  };
  
  template <class T, class... Args>
  constexpr explicit variant_builder(std::in_place_type_t<T>, Args&&... args) noexcept(?) {
    
  };
  
  template <class T>
  constexpr variant_builder(T&& t) noexcept(?) {
    
  };
  
  template <class T>
  constexpr variant_builder(T&& t) noexcept(?) {
    
  }; 
      
  constexpr variant_builder& operator=(variant_builder const &rhs) noexcept(?) {
    
  }
  
  constexpr variant_builder& operator=(variant_builder &&rhs) noexcept {
    
  }
  
  template <class T>
  variant_builder& operator=(T&& t) noexcept(?) {
    
  };  
}
  
} // namespace std