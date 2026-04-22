#pragma once
#include <omp.h>

#include <array>
#include <bit>
#include <cmath>
#include <concepts>
#include <execution>
#include <iterator>
#include <memory>
#include <type_traits>

#include "simd/simd_detect.hpp"

#define STRINGIFY(x)            #x
#define EXPAND_AND_STRINGIFY(x) STRINGIFY(x)

#ifdef _MSC_VER
#  define WARNING(msg)                                                          \
    __pragma(message(__FILE__ "(" _CRT_STRINGIZE(__LINE__) "): warning: " msg))
#elif defined(__GNUC__) || defined(__clang__)
#  define WARNING(msg) _Pragma(STRINGIFY(GCC warning msg))
#else
#  define WARNING(msg)
#endif

#if defined(__clang__)
// Clang support clang loop unroll_count or unroll
#  define LOOP_UNROLL(n) _Pragma(EXPAND_AND_STRINGIFY(clang loop unroll_count(n)))
#  define ALWAYS_INLINE  inline __attribute__((always_inline))
#elif defined(__GNUC__) && !defined(__INTEL_COMPILER) && __GNUC__ >= 8
// GCC 8+ support #pragma GCC unroll
#  define LOOP_UNROLL(n) _Pragma(EXPAND_AND_STRINGIFY(GCC unroll n))
#  define ALWAYS_INLINE  inline __attribute__((always_inline))
#elif defined(_MSC_VER) && _MSC_VER >= 1920
// MSVC 2019+ support __pragma(unroll)
#  define LOOP_UNROLL(n)  //__pragma(unroll(n))
#  define ALWAYS_INLINE   inline __forceinline
#else
#  define LOOP_UNROLL(n)
#  define ALWAYS_INLINE inline
#endif

namespace stdex {

enum class SortOrder
{
  Ascending,
  Descending
};

// < Even with signed types, if you are sure there will be no negative numbers,
// < you can enable the unsigned option to improve performance to some extent.
enum class HasNegative
{
  Yes,
  No
};

enum class NaNPosition
{
  //< Temp Data: {INFINITY, -INFINITY, NAN, 1.0f / 1.0f, -1.0f / 1.0f,
  //         std::sqrt(-1.0f), 3.14f}

  //< -nan(ind) always ahead, nan always behind
  Unhandled,  // -nan(ind) -inf -1 1 3.14 inf nan

  //< -nan(ind)/nan order is uncertain, but they are always behind/ahead of
  // other values
  AtStart,  // nan -nan(ind) -inf -1 1 3.14 inf
  AtEnd     // -inf -1 1 3.14 inf -nan(ind) nan
};

template<typename T>
struct identity_key_extractor
{
  template<typename U>
  constexpr auto operator() (U&& value) const noexcept
      -> std::enable_if_t<std::is_same_v<std::decay_t<U>, T>, T>
  {
    return std::forward<U>(value);
  }
};

template<typename Class, typename Key>
struct member_key_extractor
{
  Key Class::* member_ptr;

  constexpr member_key_extractor(Key Class::* ptr) noexcept : member_ptr(ptr) { }

  constexpr Key operator() (const Class& obj) const noexcept
  {
    return obj.*member_ptr;
  }
};

template<typename Func>
struct function_key_extractor
{
  Func func;

  constexpr function_key_extractor(Func f) : func(std::move(f)) { }

  template<typename T>
  constexpr auto operator() (T&& value) const
      -> decltype(func(std::forward<T>(value)))
  {
    return func(std::forward<T>(value));
  }
};

using Bucket_size_t    = std::uint32_t;
using Dataset_size32_t = std::uint32_t;
using Dataset_size64_t = std::uint64_t;

namespace details {
template<typename ExPo>
concept SupportedExecutionPolicy =
    std::same_as<std::decay_t<ExPo>, std::execution::sequenced_policy> ||
    std::same_as<std::decay_t<ExPo>, std::execution::parallel_policy> ||
    std::same_as<std::decay_t<ExPo>, std::execution::unsequenced_policy> ||
    std::same_as<std::decay_t<ExPo>, std::execution::parallel_unsequenced_policy>;

template<typename T, typename Class>
concept MemberPointer =
    std::is_member_pointer_v<T> && requires (T ptr, Class obj) {
      { obj.*ptr } -> std::convertible_to<typename std::remove_pointer<T>::type>;
    };

template<typename Func, typename ValueType, typename KeyType>
concept InvocableKeyExtractor = std::invocable<Func, ValueType> &&
    std::convertible_to<std::invoke_result_t<Func, ValueType>, KeyType>;

template<typename Iter>
concept ContiguousIterator = std::contiguous_iterator<Iter>;

template<typename T>
concept ArithmeticKey = std::is_arithmetic_v<T> && !std::same_as<T, bool>;

template<typename Compare>
concept ValidComparator =
    std::same_as<Compare, std::less<>> || std::same_as<Compare, std::greater<>>;

template<Bucket_size_t BucketSize>
inline constexpr bool is_valid_bucket_size =
    BucketSize == 256U || BucketSize == 65536U;

template<typename Key_t, Bucket_size_t Bucket_size>
struct Radix_constexpr_params
{
  using Unsigned_t = std::make_unsigned_t<std::conditional_t<
      std::is_floating_point_v<Key_t>,
      std::conditional_t<sizeof(Key_t) == 4, std::uint32_t, std::uint64_t>,
      Key_t>>;

  static constexpr std::uint8_t Passes =
      Bucket_size == 256U ? sizeof(Key_t) : sizeof(Key_t) >> 1;
  static constexpr std::uint8_t  Shift_of_sign_bit = (sizeof(Key_t) << 3) - 1;
  static constexpr std::uint8_t  Shift_of_byte_idx = Bucket_size == 256U ? 3 : 4;
  static constexpr std::uint16_t Mask              = Bucket_size - 1;
  static constexpr Unsigned_t    Sign_bit_mask     = Unsigned_t { 1 }
      << Shift_of_sign_bit;
  static constexpr Unsigned_t All_bits_mask = ~Unsigned_t { 0 };
};
}  // namespace details

template<details::ContiguousIterator ContigIter>
using Identity_ke = identity_key_extractor<std::iter_value_t<ContigIter>>;

template<details::ContiguousIterator ContigIter>
using Member_ke = member_key_extractor<
    std::iter_value_t<ContigIter>,
    std::decay_t<decltype(std::declval<std::iter_value_t<ContigIter>>())>>;

template<details::ContiguousIterator ContigIter>
using Function_ke = function_key_extractor<std::iter_value_t<ContigIter>>;

template<typename KeyExtractor, std::integral DatasetSize>
struct Radix_template_params
{
  using Key_extractor_t = KeyExtractor;
  using Dataset_size_t  = DatasetSize;
  Bucket_size_t Bucket_size { 256U };
  SortOrder     Order { SortOrder::Ascending };
  HasNegative   Has_negative { HasNegative::Yes };
  NaNPosition   NaN_position { NaNPosition::Unhandled };
};

namespace details {
template<typename Size_t, typename Value_t, typename Key_extractor_t,
         typename Radix_cp, auto Radix_tp, bool Is_last_pass>
ALWAYS_INLINE void Adl_count_buckets(Size_t Start_idx, Size_t End_idx,
                                     Size_t  Pass, const Value_t* __restrict Src,
                                     Size_t* Bucket_counts,
                                     const Key_extractor_t&   Extractor,
                                     [[maybe_unused]] Size_t* NaN_counts,
                                     [[maybe_unused]] int     Thread_id)
{
  using Key_t      = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;
  using Unsigned_t = typename Radix_cp::Unsigned_t;

  LOOP_UNROLL(8) for (Size_t Idx = Start_idx; Idx < End_idx; ++Idx)
  {
    auto Key = Extractor(Src [Idx]);

    if constexpr (Is_last_pass &&
                  Radix_tp.NaN_position != NaNPosition::Unhandled) {
      if (std::isnan(Key)) [[unlikely]] {
        ++NaN_counts [Thread_id];
        continue;
      }
    }

    Unsigned_t Unsigned_value = std::bit_cast<Unsigned_t>(Key);

    if constexpr (std::is_floating_point_v<Key_t> &&
                  Radix_tp.Has_negative == HasNegative::Yes) {
      if constexpr (Is_last_pass) {
        Unsigned_value ^= ((Unsigned_value >> Radix_cp::Shift_of_sign_bit) == 0)
            ? Radix_cp::Sign_bit_mask
            : Radix_cp::All_bits_mask;
      } else {
        if (Unsigned_value >> Radix_cp::Shift_of_sign_bit)
          Unsigned_value ^= Radix_cp::All_bits_mask;
      }
    } else if constexpr (std::is_integral_v<Key_t> &&
                         Radix_tp.Has_negative == HasNegative::Yes &&
                         Is_last_pass) {
      Unsigned_value ^= Radix_cp::Sign_bit_mask;
    }

    std::uint16_t Byte_idx =
        (Unsigned_value >> (Pass << Radix_cp::Shift_of_byte_idx)) &
        Radix_cp::Mask;

    if constexpr (Radix_tp.Order == SortOrder::Descending)
      Byte_idx = Radix_cp::Mask - Byte_idx;

    ++Bucket_counts [Byte_idx];
  }
}

template<typename Size_t, typename Value_t, typename Key_extractor_t,
         typename Radix_cp, auto Radix_tp, bool Is_last_pass>
ALWAYS_INLINE void Adl_distribute_to_buckets(
    Size_t Start_idx, Size_t End_idx, Size_t Pass, Value_t* __restrict Src,
    Value_t* __restrict Dst, Size_t* Scanned_offsets,
    const Key_extractor_t& Extractor, [[maybe_unused]] Size_t Thread_NaN_offset)
{
  using Key_t      = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;
  using Unsigned_t = typename Radix_cp::Unsigned_t;

  LOOP_UNROLL(8) for (Size_t Idx = Start_idx; Idx < End_idx; ++Idx)
  {
    auto& Value = Src [Idx];
    auto  Key   = Extractor(Value);

    if constexpr (std::is_floating_point_v<Key_t> && Is_last_pass &&
                  Radix_tp.NaN_position != NaNPosition::Unhandled) {
      if (std::isnan(Key)) [[unlikely]] {
        if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart) {
          Dst [--Thread_NaN_offset] = std::move(Value);
        } else if constexpr (Radix_tp.NaN_position == NaNPosition::AtEnd) {
          Dst [Thread_NaN_offset++] = std::move(Value);
        }
        continue;
      }
    }

    Unsigned_t Unsigned_value = std::bit_cast<Unsigned_t>(Key);

    if constexpr (std::is_floating_point_v<Key_t> &&
                  Radix_tp.Has_negative == HasNegative::Yes) {
      if constexpr (Is_last_pass) {
        Unsigned_value ^= ((Unsigned_value >> Radix_cp::Shift_of_sign_bit) == 0)
            ? Radix_cp::Sign_bit_mask
            : Radix_cp::All_bits_mask;
      } else {
        if (Unsigned_value >> Radix_cp::Shift_of_sign_bit)
          Unsigned_value ^= Radix_cp::All_bits_mask;
      }
    } else if constexpr (std::is_integral_v<Key_t> &&
                         Radix_tp.Has_negative == HasNegative::Yes &&
                         Is_last_pass) {
      Unsigned_value ^= Radix_cp::Sign_bit_mask;
    }

    std::uint16_t Byte_idx =
        (Unsigned_value >> (Pass << Radix_cp::Shift_of_byte_idx)) &
        Radix_cp::Mask;

    if constexpr (Radix_tp.Order == SortOrder::Descending)
      Byte_idx = Radix_cp::Mask - Byte_idx;
    Dst [Scanned_offsets [Byte_idx]++] = std::move(Value);
  }
}

template<typename T>
struct SIMD_type_traits;

#if defined(__AVX2__) || defined(__AVX__)
template<>
struct SIMD_type_traits<std::uint32_t>
{
  static constexpr bool Is_32bit = true;
  using Simd_vec_t               = __m256i;

  static Simd_vec_t set1(std::uint32_t val) { return _mm256_set1_epi32(val); }

  static Simd_vec_t srli(Simd_vec_t vec, int shift)
  {
    return _mm256_srli_epi32(vec, shift);
  }

  static Simd_vec_t xor_si(Simd_vec_t a, Simd_vec_t b)
  {
    return _mm256_xor_si256(a, b);
  }

  static Simd_vec_t and_si(Simd_vec_t a, Simd_vec_t b)
  {
    return _mm256_and_si256(a, b);
  }

  static Simd_vec_t cmpeq(Simd_vec_t a, Simd_vec_t b)
  {
    return _mm256_cmpeq_epi32(a, b);
  }

  static Simd_vec_t blendv(Simd_vec_t a, Simd_vec_t b, Simd_vec_t mask)
  {
    return _mm256_blendv_epi8(a, b, mask);
  }

  static Simd_vec_t sub_epi(Simd_vec_t a, Simd_vec_t b)
  {
    return _mm256_sub_epi32(a, b);
  }
};

template<>
struct SIMD_type_traits<std::uint64_t>
{
  static constexpr bool Is_32bit = false;
  using Simd_vec_t               = __m256i;

  static Simd_vec_t set1(std::uint64_t val) { return _mm256_set1_epi64x(val); }

  static Simd_vec_t srli(Simd_vec_t vec, int shift)
  {
    return _mm256_srli_epi64(vec, shift);
  }

  static Simd_vec_t xor_si(Simd_vec_t a, Simd_vec_t b)
  {
    return _mm256_xor_si256(a, b);
  }

  static Simd_vec_t and_si(Simd_vec_t a, Simd_vec_t b)
  {
    return _mm256_and_si256(a, b);
  }

  static Simd_vec_t cmpeq(Simd_vec_t a, Simd_vec_t b)
  {
    return _mm256_cmpeq_epi64(a, b);
  }

  static Simd_vec_t blendv(Simd_vec_t a, Simd_vec_t b, Simd_vec_t mask)
  {
    return _mm256_blendv_epi8(a, b, mask);
  }

  static Simd_vec_t sub_epi(Simd_vec_t a, Simd_vec_t b)
  {
    return _mm256_sub_epi64(a, b);
  }
};

template<typename Size_t, typename Value_t, typename Key_extractor_t,
         typename Radix_cp, auto Radix_tp, bool Is_last_pass>
ALWAYS_INLINE void Adl_count_buckets_avx2_impl(
    Size_t Start_idx, Size_t End_idx, Size_t Pass, const Value_t* __restrict Src,
    Size_t* Bucket_counts, const Key_extractor_t& Extractor,
    [[maybe_unused]] Size_t* NaN_counts, [[maybe_unused]] int Thread_id)
{
  using Key_t      = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;
  using Unsigned_t = typename Radix_cp::Unsigned_t;
  using SIMDTraits = SIMD_type_traits<Unsigned_t>;
  using Simd_vec_t = typename SIMDTraits::Simd_vec_t;

  constexpr Size_t simd_width = sizeof(__m256i) / sizeof(Unsigned_t);
  Size_t simd_end = Start_idx + ((End_idx - Start_idx) / simd_width) * simd_width;

  Simd_vec_t mask = SIMDTraits::set1(static_cast<Unsigned_t>(Radix_cp::Mask));
  Simd_vec_t sign_bit_mask = SIMDTraits::set1(Radix_cp::Sign_bit_mask);
  Simd_vec_t all_bits_mask = SIMDTraits::set1(Radix_cp::All_bits_mask);
  Simd_vec_t zero_vec      = SIMDTraits::set1(0);

  for (Size_t Idx = Start_idx; Idx < simd_end; Idx += simd_width) {
    Simd_vec_t keys;
    bool       is_nan [simd_width] = { false };

    if constexpr (std::is_same_v<Value_t, Key_t>) {
      keys = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(Src + Idx));
    } else {
      alignas(32) Unsigned_t keys_arr [simd_width];
      if constexpr (Is_last_pass &&
                    Radix_tp.NaN_position != NaNPosition::Unhandled) {
        for (Size_t i = 0; i < simd_width; ++i) {
          auto Key = Extractor(Src [Idx + i]);
          if (std::isnan(Key)) [[unlikely]] {
            ++NaN_counts [Thread_id];
            is_nan [i]   = true;
            keys_arr [i] = 0;
          } else {
            keys_arr [i] = std::bit_cast<Unsigned_t>(Key);
          }
        }
      } else {
        for (Size_t i = 0; i < simd_width; ++i) {
          keys_arr [i] = std::bit_cast<Unsigned_t>(Extractor(Src [Idx + i]));
        }
      }
      keys = _mm256_load_si256(reinterpret_cast<const __m256i*>(keys_arr));
    }

    if constexpr (std::is_floating_point_v<Key_t> &&
                  Radix_tp.Has_negative == HasNegative::Yes) {
      if constexpr (!Is_last_pass) {
        Simd_vec_t sign_bits   = SIMDTraits::and_si(keys, sign_bit_mask);
        Simd_vec_t is_negative = SIMDTraits::cmpeq(sign_bits, sign_bit_mask);
        Simd_vec_t xor_mask =
            SIMDTraits::blendv(SIMDTraits::set1(0), all_bits_mask, is_negative);
        keys = SIMDTraits::xor_si(keys, xor_mask);
      } else if constexpr (Is_last_pass) {
        alignas(32) Unsigned_t keys_arr [simd_width];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(keys_arr), keys);

        for (Size_t i = 0; i < simd_width; ++i) {
          if (is_nan [i]) continue;
          keys_arr [i] ^= ((keys_arr [i] >> Radix_cp::Shift_of_sign_bit) == 0)
              ? Radix_cp::Sign_bit_mask
              : Radix_cp::All_bits_mask;
        }

        keys = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(keys_arr));
      }
    } else if constexpr (std::is_integral_v<Key_t> &&
                         Radix_tp.Has_negative == HasNegative::Yes &&
                         Is_last_pass) {
      keys = SIMDTraits::xor_si(keys, sign_bit_mask);
    }

    const int shift_amount =
        static_cast<int>(Pass << Radix_cp::Shift_of_byte_idx);
    Simd_vec_t shifted      = SIMDTraits::srli(keys, shift_amount);
    Simd_vec_t byte_indices = SIMDTraits::and_si(shifted, mask);

    if constexpr (Radix_tp.Order == SortOrder::Descending) {
      byte_indices = SIMDTraits::sub_epi(mask, byte_indices);
    }

    alignas(32) Unsigned_t indices [simd_width];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(indices), byte_indices);

    for (Size_t i = 0; i < simd_width; ++i) {
      if (is_nan [i]) continue;
      ++Bucket_counts [indices [i]];
    }
  }

  for (Size_t Idx = simd_end; Idx < End_idx; ++Idx) {
    auto       Key            = Extractor(Src [Idx]);
    Unsigned_t Unsigned_value = std::bit_cast<Unsigned_t>(Key);

    if constexpr (Is_last_pass &&
                  Radix_tp.NaN_position != NaNPosition::Unhandled) {
      if (std::isnan(Key)) [[unlikely]] {
        ++NaN_counts [Thread_id];
        continue;
      }
    }

    if constexpr (std::is_floating_point_v<Key_t> &&
                  Radix_tp.Has_negative == HasNegative::Yes) {
      if constexpr (Is_last_pass) {
        Unsigned_value ^= ((Unsigned_value >> Radix_cp::Shift_of_sign_bit) == 0)
            ? Radix_cp::Sign_bit_mask
            : Radix_cp::All_bits_mask;
      } else {
        if (Unsigned_value >> Radix_cp::Shift_of_sign_bit)
          Unsigned_value ^= Radix_cp::All_bits_mask;
      }
    } else if constexpr (std::is_integral_v<Key_t> &&
                         Radix_tp.Has_negative == HasNegative::Yes &&
                         Is_last_pass) {
      Unsigned_value ^= Radix_cp::Sign_bit_mask;
    }

    std::uint16_t Byte_idx =
        (Unsigned_value >> (Pass << Radix_cp::Shift_of_byte_idx)) &
        Radix_cp::Mask;
    if constexpr (Radix_tp.Order == SortOrder::Descending)
      Byte_idx = Radix_cp::Mask - Byte_idx;
    ++Bucket_counts [Byte_idx];
  }
}

template<typename Size_t, typename Value_t, typename Key_extractor_t,
         typename Radix_cp, auto Radix_tp, bool Is_last_pass>
ALWAYS_INLINE void Adl_distribute_to_buckets_avx2_impl(
    Size_t Start_idx, Size_t End_idx, Size_t Pass, Value_t* __restrict Src,
    Value_t* __restrict Dst, Size_t* Scanned_offsets,
    const Key_extractor_t& Extractor, [[maybe_unused]] Size_t Thread_NaN_offset)
{
  using Key_t      = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;
  using Unsigned_t = typename Radix_cp::Unsigned_t;
  using SIMDTraits = SIMD_type_traits<Unsigned_t>;
  using Simd_vec_t = typename SIMDTraits::Simd_vec_t;

  constexpr Size_t simd_width = sizeof(__m256i) / sizeof(Unsigned_t);
  Size_t simd_end = Start_idx + ((End_idx - Start_idx) / simd_width) * simd_width;

  Simd_vec_t mask = SIMDTraits::set1(static_cast<Unsigned_t>(Radix_cp::Mask));
  Simd_vec_t sign_bit_mask = SIMDTraits::set1(Radix_cp::Sign_bit_mask);
  Simd_vec_t all_bits_mask = SIMDTraits::set1(Radix_cp::All_bits_mask);
  Simd_vec_t zero_vec      = SIMDTraits::set1(0);

  for (Size_t Idx = Start_idx; Idx < simd_end; Idx += simd_width) {
    Simd_vec_t          keys;
    alignas(32) Value_t values [simd_width];
    bool                is_nan [simd_width] = { false };

    if constexpr (std::is_same_v<Value_t, Key_t>) {
      keys = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(Src + Idx));
      for (Size_t i = 0; i < simd_width; ++i) {
        values [i] = Src [Idx + i];
      }
    } else {
      alignas(32) Unsigned_t keys_arr [simd_width];
      for (Size_t i = 0; i < simd_width; ++i) {
        values [i]   = Src [Idx + i];
        keys_arr [i] = std::bit_cast<Unsigned_t>(Extractor(values [i]));
      }
      keys = _mm256_load_si256(reinterpret_cast<const __m256i*>(keys_arr));
    }

    if constexpr (std::is_floating_point_v<Key_t> && Is_last_pass &&
                  Radix_tp.NaN_position != NaNPosition::Unhandled) {
      for (Size_t i = 0; i < simd_width; ++i) {
        auto Key = Extractor(values [i]);
        if (std::isnan(Key)) [[unlikely]] {
          is_nan [i] = true;
          if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart) {
            Dst [--Thread_NaN_offset] = std::move(values [i]);
          } else if constexpr (Radix_tp.NaN_position == NaNPosition::AtEnd) {
            Dst [Thread_NaN_offset++] = std::move(values [i]);
          }
        }
      }
    }

    if constexpr (std::is_floating_point_v<Key_t> &&
                  Radix_tp.Has_negative == HasNegative::Yes) {
      if constexpr (Is_last_pass) {
        alignas(32) Unsigned_t keys_arr [simd_width];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(keys_arr), keys);

        for (Size_t i = 0; i < simd_width; ++i) {
          if (is_nan [i]) continue;
          keys_arr [i] ^= ((keys_arr [i] >> Radix_cp::Shift_of_sign_bit) == 0)
              ? Radix_cp::Sign_bit_mask
              : Radix_cp::All_bits_mask;
        }

        keys = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(keys_arr));
      } else {
        Simd_vec_t sign_bits   = SIMDTraits::and_si(keys, sign_bit_mask);
        Simd_vec_t is_negative = SIMDTraits::cmpeq(sign_bits, sign_bit_mask);
        Simd_vec_t xor_mask =
            SIMDTraits::blendv(SIMDTraits::set1(0), all_bits_mask, is_negative);
        keys = SIMDTraits::xor_si(keys, xor_mask);
      }
    } else if constexpr (std::is_integral_v<Key_t> &&
                         Radix_tp.Has_negative == HasNegative::Yes &&
                         Is_last_pass) {
      keys = SIMDTraits::xor_si(keys, sign_bit_mask);
    }

    const int shift_amount =
        static_cast<int>(Pass << Radix_cp::Shift_of_byte_idx);
    ;
    Simd_vec_t shifted      = SIMDTraits::srli(keys, shift_amount);
    Simd_vec_t byte_indices = SIMDTraits::and_si(shifted, mask);

    if constexpr (Radix_tp.Order == SortOrder::Descending) {
      byte_indices = SIMDTraits::sub_epi(mask, byte_indices);
    }

    alignas(32) Unsigned_t indices [simd_width];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(indices), byte_indices);

    for (Size_t i = 0; i < simd_width; ++i) {
      if (is_nan [i]) continue;
      Dst [Scanned_offsets [indices [i]]++] = std::move(values [i]);
    }
  }

  for (Size_t Idx = simd_end; Idx < End_idx; ++Idx) {
    auto&      Value          = Src [Idx];
    auto       Key            = Extractor(Value);
    Unsigned_t Unsigned_value = std::bit_cast<Unsigned_t>(Key);

    if constexpr (std::is_floating_point_v<Key_t> && Is_last_pass &&
                  Radix_tp.NaN_position != NaNPosition::Unhandled) {
      if (std::isnan(Key)) [[unlikely]] {
        if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart) {
          Dst [--Thread_NaN_offset] = std::move(Value);
        } else if constexpr (Radix_tp.NaN_position == NaNPosition::AtEnd) {
          Dst [Thread_NaN_offset++] = std::move(Value);
        }
        continue;
      }
    }

    if constexpr (std::is_floating_point_v<Key_t> &&
                  Radix_tp.Has_negative == HasNegative::Yes) {
      if constexpr (Is_last_pass) {
        Unsigned_value ^= ((Unsigned_value >> Radix_cp::Shift_of_sign_bit) == 0)
            ? Radix_cp::Sign_bit_mask
            : Radix_cp::All_bits_mask;
      } else {
        if (Unsigned_value >> Radix_cp::Shift_of_sign_bit)
          Unsigned_value ^= Radix_cp::All_bits_mask;
      }
    } else if constexpr (std::is_integral_v<Key_t> &&
                         Radix_tp.Has_negative == HasNegative::Yes &&
                         Is_last_pass) {
      Unsigned_value ^= Radix_cp::Sign_bit_mask;
    }

    std::uint16_t Byte_idx =
        (Unsigned_value >> (Pass << Radix_cp::Shift_of_byte_idx)) &
        Radix_cp::Mask;
    if constexpr (Radix_tp.Order == SortOrder::Descending)
      Byte_idx = Radix_cp::Mask - Byte_idx;
    Dst [Scanned_offsets [Byte_idx]++] = std::move(Value);
  }
}
#elif defined(__ARM_NEON) || defined(__aarch64__)
template<>
struct SIMD_type_traits<std::uint32_t>
{
  static constexpr bool Is_32bit = true;
  using Simd_vec_t               = uint32x4_t;

  static Simd_vec_t set1(std::uint32_t val) { return vdupq_n_u32(val); }

  static Simd_vec_t srli(Simd_vec_t vec, int shift)
  {
    return vshrq_n_u32(vec, shift);
  }

  static Simd_vec_t xor_si(Simd_vec_t a, Simd_vec_t b) { return veorq_u32(a, b); }

  static Simd_vec_t and_si(Simd_vec_t a, Simd_vec_t b) { return vandq_u32(a, b); }

  static Simd_vec_t cmpeq(Simd_vec_t a, Simd_vec_t b) { return vceqq_u32(a, b); }

  static Simd_vec_t blendv(Simd_vec_t a, Simd_vec_t b, Simd_vec_t mask)
  {
    return vbslq_u32(mask, b, a);
  }

  static Simd_vec_t sub_epi(Simd_vec_t a, Simd_vec_t b)
  {
    return vsubq_u32(a, b);
  }
};

template<>
struct SIMD_type_traits<std::uint64_t>
{
  static constexpr bool Is_32bit = false;
  using Simd_vec_t               = uint64x2_t;

  static Simd_vec_t set1(std::uint64_t val) { return vdupq_n_u64(val); }

  static Simd_vec_t srli(Simd_vec_t vec, int shift)
  {
    return vshrq_n_u64(vec, shift);
  }

  static Simd_vec_t xor_si(Simd_vec_t a, Simd_vec_t b) { return veorq_u64(a, b); }

  static Simd_vec_t and_si(Simd_vec_t a, Simd_vec_t b) { return vandq_u64(a, b); }

  static Simd_vec_t cmpeq(Simd_vec_t a, Simd_vec_t b) { return vceqq_u64(a, b); }

  static Simd_vec_t blendv(Simd_vec_t a, Simd_vec_t b, Simd_vec_t mask)
  {
    return vbslq_u64(mask, b, a);
  }

  static Simd_vec_t sub_epi(Simd_vec_t a, Simd_vec_t b)
  {
    return vsubq_u64(a, b);
  }
};

template<typename Size_t, typename Value_t, typename Key_extractor_t,
         typename Radix_cp, auto Radix_tp, bool Is_last_pass>
ALWAYS_INLINE void Adl_distribute_to_buckets_neon_impl(
    Size_t Start_idx, Size_t End_idx, Size_t Pass, Value_t* __restrict Src,
    Value_t* __restrict Dst, Size_t* Scanned_offsets,
    const Key_extractor_t& Extractor, [[maybe_unused]] Size_t Thread_NaN_offset)
{
  using Key_t      = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;
  using Unsigned_t = typename Radix_cp::Unsigned_t;
  using SIMDTraits = SIMD_type_traits<Unsigned_t>;
  using Simd_vec_t = typename SIMDTraits::Simd_vec_t;

  constexpr Size_t simd_width = std::is_same_v<Unsigned_t, std::uint32_t> ? 4 : 2;
  Size_t simd_end = Start_idx + ((End_idx - Start_idx) / simd_width) * simd_width;

  Simd_vec_t mask = SIMDTraits::set1(static_cast<Unsigned_t>(Radix_cp::Mask));
  Simd_vec_t sign_bit_mask = SIMDTraits::set1(Radix_cp::Sign_bit_mask);
  Simd_vec_t all_bits_mask = SIMDTraits::set1(Radix_cp::All_bits_mask);

  for (Size_t Idx = Start_idx; Idx < simd_end; Idx += simd_width) {
    Simd_vec_t          keys;
    alignas(16) Value_t values [simd_width];
    bool                is_nan [simd_width] = { false };

    if constexpr (std::is_same_v<Value_t, Key_t>) {
      if constexpr (simd_width == 4) {
        keys = vld1q_u32(reinterpret_cast<const uint32_t*>(Src + Idx));
      } else {
        keys = vld1q_u64(reinterpret_cast<const uint64_t*>(Src + Idx));
      }
      for (Size_t i = 0; i < simd_width; ++i) {
        values [i] = Src [Idx + i];
      }
    } else {
      alignas(16) Unsigned_t keys_arr [simd_width];
      for (Size_t i = 0; i < simd_width; ++i) {
        values [i]   = Src [Idx + i];
        keys_arr [i] = std::bit_cast<Unsigned_t>(Extractor(values [i]));
      }
      if constexpr (simd_width == 4) {
        keys = vld1q_u32(keys_arr);
      } else {
        keys = vld1q_u64(keys_arr);
      }
    }

    if constexpr (std::is_floating_point_v<Key_t> && Is_last_pass &&
                  Radix_tp.NaN_position != NaNPosition::Unhandled) {
      for (Size_t i = 0; i < simd_width; ++i) {
        auto Key = Extractor(values [i]);
        if (std::isnan(Key)) [[unlikely]] {
          is_nan [i] = true;
          if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart) {
            Dst [--Thread_NaN_offset] = std::move(values [i]);
          } else if constexpr (Radix_tp.NaN_position == NaNPosition::AtEnd) {
            Dst [Thread_NaN_offset++] = std::move(values [i]);
          }
        }
      }
    }

    if constexpr (std::is_floating_point_v<Key_t> &&
                  Radix_tp.Has_negative == HasNegative::Yes) {
      if constexpr (Is_last_pass) {
        alignas(16) Unsigned_t keys_arr [simd_width];
        if constexpr (simd_width == 4) {
          vst1q_u32(keys_arr, keys);
        } else {
          vst1q_u64(keys_arr, keys);
        }

        for (Size_t i = 0; i < simd_width; ++i) {
          if (is_nan [i]) continue;
          keys_arr [i] ^= ((keys_arr [i] >> Radix_cp::Shift_of_sign_bit) == 0)
              ? Radix_cp::Sign_bit_mask
              : Radix_cp::All_bits_mask;
        }

        if constexpr (simd_width == 4) {
          keys = vld1q_u32(keys_arr);
        } else {
          keys = vld1q_u64(keys_arr);
        }
      } else {
        Simd_vec_t sign_bits   = SIMDTraits::and_si(keys, sign_bit_mask);
        Simd_vec_t is_negative = SIMDTraits::cmpeq(sign_bits, sign_bit_mask);
        Simd_vec_t xor_mask =
            SIMDTraits::blendv(SIMDTraits::set1(0), all_bits_mask, is_negative);
        keys = SIMDTraits::xor_si(keys, xor_mask);
      }
    } else if constexpr (std::is_integral_v<Key_t> &&
                         Radix_tp.Has_negative == HasNegative::Yes &&
                         Is_last_pass) {
      keys = SIMDTraits::xor_si(keys, sign_bit_mask);
    }

    const int shift_amount =
        static_cast<int>(Pass << Radix_cp::Shift_of_byte_idx);
    Simd_vec_t shifted      = SIMDTraits::srli(keys, shift_amount);
    Simd_vec_t byte_indices = SIMDTraits::and_si(shifted, mask);

    if constexpr (Radix_tp.Order == SortOrder::Descending) {
      byte_indices = SIMDTraits::sub_epi(mask, byte_indices);
    }

    alignas(16) Unsigned_t indices [simd_width];
    if constexpr (simd_width == 4) {
      vst1q_u32(indices, byte_indices);
    } else {
      vst1q_u64(indices, byte_indices);
    }

    for (Size_t i = 0; i < simd_width; ++i) {
      if (is_nan [i]) continue;
      Dst [Scanned_offsets [indices [i]]++] = std::move(values [i]);
    }
  }

  for (Size_t Idx = simd_end; Idx < End_idx; ++Idx) {
    auto&      Value          = Src [Idx];
    auto       Key            = Extractor(Value);
    Unsigned_t Unsigned_value = std::bit_cast<Unsigned_t>(Key);

    if constexpr (std::is_floating_point_v<Key_t> && Is_last_pass &&
                  Radix_tp.NaN_position != NaNPosition::Unhandled) {
      if (std::isnan(Key)) [[unlikely]] {
        if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart) {
          Dst [--Thread_NaN_offset] = std::move(Value);
        } else if constexpr (Radix_tp.NaN_position == NaNPosition::AtEnd) {
          Dst [Thread_NaN_offset++] = std::move(Value);
        }
        continue;
      }
    }

    if constexpr (std::is_floating_point_v<Key_t> &&
                  Radix_tp.Has_negative == HasNegative::Yes) {
      if constexpr (Is_last_pass) {
        Unsigned_value ^= ((Unsigned_value >> Radix_cp::Shift_of_sign_bit) == 0)
            ? Radix_cp::Sign_bit_mask
            : Radix_cp::All_bits_mask;
      } else {
        if (Unsigned_value >> Radix_cp::Shift_of_sign_bit)
          Unsigned_value ^= Radix_cp::All_bits_mask;
      }
    } else if constexpr (std::is_integral_v<Key_t> &&
                         Radix_tp.Has_negative == HasNegative::Yes &&
                         Is_last_pass) {
      Unsigned_value ^= Radix_cp::Sign_bit_mask;
    }

    std::uint16_t Byte_idx =
        (Unsigned_value >> (Pass << Radix_cp::Shift_of_byte_idx)) &
        Radix_cp::Mask;
    if constexpr (Radix_tp.Order == SortOrder::Descending)
      Byte_idx = Radix_cp::Mask - Byte_idx;
    Dst [Scanned_offsets [Byte_idx]++] = std::move(Value);
  }
}

template<typename Size_t, typename Value_t, typename Key_extractor_t,
         typename Radix_cp, auto Radix_tp, bool Is_last_pass>
ALWAYS_INLINE void Adl_count_buckets_neon_impl(
    Size_t Start_idx, Size_t End_idx, Size_t Pass, const Value_t* __restrict Src,
    Size_t* Bucket_counts, const Key_extractor_t& Extractor,
    [[maybe_unused]] Size_t* NaN_counts, [[maybe_unused]] int Thread_id)
{
  using Key_t      = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;
  using Unsigned_t = typename Radix_cp::Unsigned_t;
  using SIMDTraits = SIMD_type_traits<Unsigned_t>;
  using Simd_vec_t = typename SIMDTraits::Simd_vec_t;

  constexpr Size_t simd_width = std::is_same_v<Unsigned_t, std::uint32_t> ? 4 : 2;
  Size_t simd_end = Start_idx + ((End_idx - Start_idx) / simd_width) * simd_width;

  Simd_vec_t mask = SIMDTraits::set1(static_cast<Unsigned_t>(Radix_cp::Mask));
  Simd_vec_t sign_bit_mask = SIMDTraits::set1(Radix_cp::Sign_bit_mask);
  Simd_vec_t all_bits_mask = SIMDTraits::set1(Radix_cp::All_bits_mask);

  for (Size_t Idx = Start_idx; Idx < simd_end; Idx += simd_width) {
    Simd_vec_t keys;
    bool       is_nan [simd_width] = { false };

    if constexpr (std::is_same_v<Value_t, Key_t>) {
      if constexpr (simd_width == 4) {
        keys = vld1q_u32(reinterpret_cast<const uint32_t*>(Src + Idx));
      } else {
        keys = vld1q_u64(reinterpret_cast<const uint64_t*>(Src + Idx));
      }
    } else {
      alignas(16) Unsigned_t keys_arr [simd_width];
      if constexpr (Is_last_pass &&
                    Radix_tp.NaN_position != NaNPosition::Unhandled) {
        for (Size_t i = 0; i < simd_width; ++i) {
          auto Key = Extractor(Src [Idx + i]);
          if (std::isnan(Key)) [[unlikely]] {
            ++NaN_counts [Thread_id];
            is_nan [i]   = true;
            keys_arr [i] = 0;
          } else {
            keys_arr [i] = std::bit_cast<Unsigned_t>(Key);
          }
        }
      } else {
        for (Size_t i = 0; i < simd_width; ++i) {
          keys_arr [i] = std::bit_cast<Unsigned_t>(Extractor(Src [Idx + i]));
        }
      }
      if constexpr (simd_width == 4) {
        keys = vld1q_u32(keys_arr);
      } else {
        keys = vld1q_u64(keys_arr);
      }
    }

    if constexpr (std::is_floating_point_v<Key_t> &&
                  Radix_tp.Has_negative == HasNegative::Yes) {
      if constexpr (!Is_last_pass) {
        Simd_vec_t sign_bits   = SIMDTraits::and_si(keys, sign_bit_mask);
        Simd_vec_t is_negative = SIMDTraits::cmpeq(sign_bits, sign_bit_mask);
        Simd_vec_t xor_mask =
            SIMDTraits::blendv(SIMDTraits::set1(0), all_bits_mask, is_negative);
        keys = SIMDTraits::xor_si(keys, xor_mask);
      } else if constexpr (Is_last_pass) {
        alignas(16) Unsigned_t keys_arr [simd_width];
        if constexpr (simd_width == 4) {
          vst1q_u32(keys_arr, keys);
        } else {
          vst1q_u64(keys_arr, keys);
        }

        for (Size_t i = 0; i < simd_width; ++i) {
          if (is_nan [i]) continue;
          keys_arr [i] ^= ((keys_arr [i] >> Radix_cp::Shift_of_sign_bit) == 0)
              ? Radix_cp::Sign_bit_mask
              : Radix_cp::All_bits_mask;
        }

        if constexpr (simd_width == 4) {
          keys = vld1q_u32(keys_arr);
        } else {
          keys = vld1q_u64(keys_arr);
        }
      }
    } else if constexpr (std::is_integral_v<Key_t> &&
                         Radix_tp.Has_negative == HasNegative::Yes &&
                         Is_last_pass) {
      keys = SIMDTraits::xor_si(keys, sign_bit_mask);
    }

    const int shift_amount =
        static_cast<int>(Pass << Radix_cp::Shift_of_byte_idx);
    Simd_vec_t shifted      = SIMDTraits::srli(keys, shift_amount);
    Simd_vec_t byte_indices = SIMDTraits::and_si(shifted, mask);

    if constexpr (Radix_tp.Order == SortOrder::Descending) {
      byte_indices = SIMDTraits::sub_epi(mask, byte_indices);
    }

    alignas(16) Unsigned_t indices [simd_width];
    if constexpr (simd_width == 4) {
      vst1q_u32(indices, byte_indices);
    } else {
      vst1q_u64(indices, byte_indices);
    }

    for (Size_t i = 0; i < simd_width; ++i) {
      if (is_nan [i]) continue;
      ++Bucket_counts [indices [i]];
    }
  }

  for (Size_t Idx = simd_end; Idx < End_idx; ++Idx) {
    auto       Key            = Extractor(Src [Idx]);
    Unsigned_t Unsigned_value = std::bit_cast<Unsigned_t>(Key);

    if constexpr (Is_last_pass &&
                  Radix_tp.NaN_position != NaNPosition::Unhandled) {
      if (std::isnan(Key)) [[unlikely]] {
        ++NaN_counts [Thread_id];
        continue;
      }
    }

    if constexpr (std::is_floating_point_v<Key_t> &&
                  Radix_tp.Has_negative == HasNegative::Yes) {
      if constexpr (Is_last_pass) {
        Unsigned_value ^= ((Unsigned_value >> Radix_cp::Shift_of_sign_bit) == 0)
            ? Radix_cp::Sign_bit_mask
            : Radix_cp::All_bits_mask;
      } else {
        if (Unsigned_value >> Radix_cp::Shift_of_sign_bit)
          Unsigned_value ^= Radix_cp::All_bits_mask;
      }
    } else if constexpr (std::is_integral_v<Key_t> &&
                         Radix_tp.Has_negative == HasNegative::Yes &&
                         Is_last_pass) {
      Unsigned_value ^= Radix_cp::Sign_bit_mask;
    }

    std::uint16_t Byte_idx =
        (Unsigned_value >> (Pass << Radix_cp::Shift_of_byte_idx)) &
        Radix_cp::Mask;
    if constexpr (Radix_tp.Order == SortOrder::Descending)
      Byte_idx = Radix_cp::Mask - Byte_idx;
    ++Bucket_counts [Byte_idx];
  }
}
#endif

template<typename Size_t, typename Value_t, typename Key_extractor_t,
         typename Radix_cp, auto Radix_tp, bool Is_last_pass>
ALWAYS_INLINE void Adl_count_buckets_simd(
    Size_t Start_idx, Size_t End_idx, Size_t Pass, const Value_t* __restrict Src,
    Size_t* Bucket_counts, const Key_extractor_t& Extractor,
    [[maybe_unused]] Size_t* NaN_counts, [[maybe_unused]] int Thread_id)
{
  using Unsigned_t = typename Radix_cp::Unsigned_t;
#if defined(__AVX2__) || defined(__AVX__)
  Adl_count_buckets_avx2_impl<Size_t, Value_t, Key_extractor_t, Radix_cp,
                              Radix_tp, Is_last_pass>(
      Start_idx, End_idx, Pass, Src, Bucket_counts, Extractor, NaN_counts,
      Thread_id);
#elif defined(__ARM_NEON) || defined(__aarch64__)
  Adl_count_buckets_neon_impl<Size_t, Value_t, Key_extractor_t, Radix_cp,
                              Radix_tp, Is_last_pass>(
      Start_idx, End_idx, Pass, Src, Bucket_counts, Extractor, NaN_counts,
      Thread_id);
#endif
}

template<typename Size_t, typename Value_t, typename Key_extractor_t,
         typename Radix_cp, auto Radix_tp, bool Is_last_pass>
ALWAYS_INLINE void Adl_distribute_to_buckets_simd(
    Size_t Start_idx, Size_t End_idx, Size_t Pass, Value_t* __restrict Src,
    Value_t* __restrict Dst, Size_t* Scanned_offsets,
    const Key_extractor_t& Extractor, [[maybe_unused]] Size_t Thread_NaN_offset)
{
  using Unsigned_t = typename Radix_cp::Unsigned_t;
#if defined(__AVX2__) || defined(__AVX__)
  Adl_distribute_to_buckets_avx2_impl<Size_t, Value_t, Key_extractor_t, Radix_cp,
                                      Radix_tp, Is_last_pass>(
      Start_idx, End_idx, Pass, Src, Dst, Scanned_offsets, Extractor,
      Thread_NaN_offset);
#elif defined(__ARM_NEON) || defined(__aarch64__)
  Adl_distribute_to_buckets_neon_impl<Size_t, Value_t, Key_extractor_t, Radix_cp,
                                      Radix_tp, Is_last_pass>(
      Start_idx, End_idx, Pass, Src, Dst, Scanned_offsets, Extractor,
      Thread_NaN_offset);
#endif
}

}  // namespace details

namespace details {
template<typename ContigIter>
consteval auto Adl_default_radix_params()
{
  using Key_extractor_t = Identity_ke<ContigIter>;
  using Value_t         = typename std::iterator_traits<ContigIter>::value_type;
  using Key_t           = std::decay_t<decltype(Key_extractor_t {}.operator() (
      std::declval<Value_t>()))>;

  if constexpr (std::unsigned_integral<Key_t>) {
    return Radix_template_params<Key_extractor_t, Dataset_size32_t> {
      .Has_negative = HasNegative::No
    };
  } else {
    return Radix_template_params<Key_extractor_t, Dataset_size32_t> {};
  }
}

template<ContiguousIterator ContigIter,
         auto Radix_tp = details::Adl_default_radix_params<ContigIter>()>
void radix_sort_impl(std::execution::sequenced_policy, ContigIter First,
                     ContigIter Last)
{
  static_assert(is_valid_bucket_size<Radix_tp.Bucket_size>,
                "Bucket size must be 256 or 65536");

  using Value_t         = typename std::iterator_traits<ContigIter>::value_type;
  using Key_extractor_t = typename decltype(Radix_tp)::Key_extractor_t;
  Key_extractor_t Extractor {};
  using Key_t = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;

  static_assert(
      ArithmeticKey<Key_t>,
      "Key type must be arithmetic (excluding bool and character types)");

  static_assert(!(sizeof(Key_t) == 1 && Radix_tp.Bucket_size == 65536U),
                "Cannot use 65536 bucket size with 1-byte key type");

  static_assert(
      !(std::is_unsigned_v<Key_t> && Radix_tp.Has_negative == HasNegative::Yes),
      "Cannot use HasNegative::Yes with unsigned key type");

  using Unsigned_t = std::make_unsigned_t<std::conditional_t<
      std::is_floating_point_v<Key_t>,
      std::conditional_t<sizeof(Key_t) == 4, std::uint32_t, std::uint64_t>,
      Key_t>>;

  using Size_t = typename decltype(Radix_tp)::Dataset_size_t;

  Size_t Size = static_cast<Size_t>(std::distance(First, Last));
  if (Size <= 1) [[unlikely]]
    return;

  Value_t* Start_ptr = &*First;

  using Radix_cp = Radix_constexpr_params<Key_t, Radix_tp.Bucket_size>;

  /*static */ std::array<Size_t, Radix_tp.Bucket_size> Bucket_counts {};
  /*static */ std::array<Size_t, Radix_tp.Bucket_size> Scanned_offsets;

  // Do not use std::make_unique, We need a pod type that is not initialized
  std::unique_ptr<Value_t []> Buffer(new Value_t [Size] /*()*/);
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
  Value_t* __restrict Src = Start_ptr;
  Value_t* __restrict Dst = Buffer.get();
#else
  Value_t* Src = Start_ptr;
  Value_t* Dst = Buffer.get();
#endif

  for (std::uint8_t Pass = 0; Pass < Radix_cp::Passes - 1; ++Pass) {
    Adl_count_buckets<Size_t, Value_t, Key_extractor_t, Radix_cp, Radix_tp,
                      false>(
        /*Start_idx     */ 0,
        /*End_idx       */ Size,
        /*Pass          */ Pass,
        /*Src           */ Src,
        /*Bucket_counts */ Bucket_counts.data(),
        /*Extractor     */ Extractor,
        /*NaN_counts    */ nullptr,
        /*Thread_id     */ 0);

    std::exclusive_scan(Bucket_counts.begin(), Bucket_counts.end(),
                        Scanned_offsets.begin(), 0, std::plus<> {});

    Adl_distribute_to_buckets<Size_t, Value_t, Key_extractor_t, Radix_cp,
                              Radix_tp, false>(
        /*Start_idx       */ 0,
        /*End_idx         */ Size,
        /*Pass            */ Pass,
        /*Src             */ Src,
        /*Dst             */ Dst,
        /*Scanned_offsets */ Scanned_offsets.data(),
        /*Extractor       */ Extractor,
        /*Thread_NaN_offset       */ 0);
    std::swap(Src, Dst);

    std::fill(Bucket_counts.begin(), Bucket_counts.end(), 0);
  }

  // Handle the sign bit of the last round
  //        ^^^^^^^^^^^^

  constexpr bool Handle_NaN = std::is_floating_point_v<Key_t> &&
      Radix_tp.NaN_position != NaNPosition::Unhandled;
  [[maybe_unused]] std::array<Size_t, Handle_NaN ? 1 : 0> NaN_counts {};

  Adl_count_buckets<Size_t, Value_t, Key_extractor_t, Radix_cp, Radix_tp, true>(
      /*Start_idx     */ 0,
      /*End_idx       */ Size,
      /*Pass          */ Radix_cp::Passes - 1,
      /*Src           */ Src,
      /*Bucket_counts */ Bucket_counts.data(),
      /*Extractor     */ Extractor,
      /*NaN_counts    */ Handle_NaN ? NaN_counts.data() : nullptr,
      /*Thread_id     */ 0);

  if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart && Handle_NaN) {
    std::exclusive_scan(Bucket_counts.begin(), Bucket_counts.end(),
                        Scanned_offsets.begin(), NaN_counts [0], std::plus<> {});
  } else {
    std::exclusive_scan(Bucket_counts.begin(), Bucket_counts.end(),
                        Scanned_offsets.begin(), 0, std::plus<> {});
  }

  // For sequential version, we need to handle NaN differently
  // AtStart: NaN should be placed from 0 to NaN_count-1 (using --NaN_count)
  // AtEnd: NaN should be placed from Size-NaN_count to Size-1 (using Size -
  // NaN_count--)
  [[maybe_unused]] Size_t Thread_NaN_offset = 0;
  if constexpr (Handle_NaN) {
    if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart) {
      Thread_NaN_offset = NaN_counts [0];
    } else if constexpr (Radix_tp.NaN_position == NaNPosition::AtEnd) {
      Thread_NaN_offset = Size - NaN_counts [0];
    }
  }

  Adl_distribute_to_buckets<Size_t, Value_t, Key_extractor_t, Radix_cp, Radix_tp,
                            true>(
      /*Start_idx       */ 0,
      /*End_idx         */ Size,
      /*Pass            */ Radix_cp::Passes - 1,
      /*Src             */ Src,
      /*Dst             */ Dst,
      /*Scanned_offsets */ Scanned_offsets.data(),
      /*Extractor       */ Extractor,
      /*Thread_NaN_offset */ Handle_NaN ? Thread_NaN_offset : 0);
  std::swap(Src, Dst);

  if constexpr (Radix_cp::Passes & 1) std::move(Src, Src + Size, Start_ptr);
}

template<ContiguousIterator ContigIter,
         auto Radix_tp = details::Adl_default_radix_params<ContigIter>()>
void radix_sort_impl(std::execution::unsequenced_policy, ContigIter First,
                     ContigIter Last)
{
#if defined(__AVX2__) || defined(__ARM_NEON) || defined(__aarch64__)
  static_assert(is_valid_bucket_size<Radix_tp.Bucket_size>,
                "Bucket size must be 256 or 65536");

  using Value_t         = typename std::iterator_traits<ContigIter>::value_type;
  using Key_extractor_t = typename decltype(Radix_tp)::Key_extractor_t;
  Key_extractor_t Extractor {};
  using Key_t = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;

  static_assert(
      ArithmeticKey<Key_t>,
      "Key type must be arithmetic (excluding bool and character types)");

  static_assert(!(sizeof(Key_t) == 1 && Radix_tp.Bucket_size == 65536U),
                "Cannot use 65536 bucket size with 1-byte key type");

  static_assert(
      !(std::is_unsigned_v<Key_t> && Radix_tp.Has_negative == HasNegative::Yes),
      "Cannot use HasNegative::Yes with unsigned key type");

  using Unsigned_t = std::make_unsigned_t<std::conditional_t<
      std::is_floating_point_v<Key_t>,
      std::conditional_t<sizeof(Key_t) == 4, std::uint32_t, std::uint64_t>,
      Key_t>>;

  using Size_t = typename decltype(Radix_tp)::Dataset_size_t;

  Size_t Size = static_cast<Size_t>(std::distance(First, Last));
  if (Size <= 1) [[unlikely]]
    return;

  Value_t* Start_ptr = &*First;

  using Radix_cp = Radix_constexpr_params<Key_t, Radix_tp.Bucket_size>;

  std::array<Size_t, Radix_tp.Bucket_size> Bucket_counts {};
  std::array<Size_t, Radix_tp.Bucket_size> Scanned_offsets;

  std::unique_ptr<Value_t []> Buffer(new Value_t [Size]);
#  if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
  Value_t* __restrict Src = Start_ptr;
  Value_t* __restrict Dst = Buffer.get();
#  else
  Value_t* Src = Start_ptr;
  Value_t* Dst = Buffer.get();
#  endif

  for (std::uint8_t Pass = 0; Pass < Radix_cp::Passes - 1; ++Pass) {
    Adl_count_buckets_simd<Size_t, Value_t, Key_extractor_t, Radix_cp, Radix_tp,
                           false>(
        /*Start_idx     */ 0,
        /*End_idx       */ Size,
        /*Pass          */ Pass,
        /*Src           */ Src,
        /*Bucket_counts */ Bucket_counts.data(),
        /*Extractor     */ Extractor,
        /*NaN_counts    */ nullptr,
        /*Thread_id     */ 0);

    std::exclusive_scan(Bucket_counts.begin(), Bucket_counts.end(),
                        Scanned_offsets.begin(), 0, std::plus<> {});

    Adl_distribute_to_buckets_simd<Size_t, Value_t, Key_extractor_t, Radix_cp,
                                   Radix_tp, false>(
        /*Start_idx       */ 0,
        /*End_idx         */ Size,
        /*Pass            */ Pass,
        /*Src             */ Src,
        /*Dst             */ Dst,
        /*Scanned_offsets */ Scanned_offsets.data(),
        /*Extractor       */ Extractor,
        /*Thread_NaN_offset */ 0);
    std::swap(Src, Dst);

    std::fill(Bucket_counts.begin(), Bucket_counts.end(), 0);
  }

  constexpr bool Handle_NaN = std::is_floating_point_v<Key_t> &&
      Radix_tp.NaN_position != NaNPosition::Unhandled;
  [[maybe_unused]] std::array<Size_t, Handle_NaN ? 1 : 0> NaN_counts {};

  Adl_count_buckets_simd<Size_t, Value_t, Key_extractor_t, Radix_cp, Radix_tp,
                         true>(
      /*Start_idx     */ 0,
      /*End_idx       */ Size,
      /*Pass          */ Radix_cp::Passes - 1,
      /*Src           */ Src,
      /*Bucket_counts */ Bucket_counts.data(),
      /*Extractor     */ Extractor,
      /*NaN_counts    */ Handle_NaN ? NaN_counts.data() : nullptr,
      /*Thread_id     */ 0);

  if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart && Handle_NaN) {
    std::exclusive_scan(Bucket_counts.begin(), Bucket_counts.end(),
                        Scanned_offsets.begin(), NaN_counts [0], std::plus<> {});
  } else {
    std::exclusive_scan(Bucket_counts.begin(), Bucket_counts.end(),
                        Scanned_offsets.begin(), 0, std::plus<> {});
  }

  [[maybe_unused]] Size_t Thread_NaN_offset = 0;
  if constexpr (Handle_NaN) {
    if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart) {
      Thread_NaN_offset = NaN_counts [0];
    } else if constexpr (Radix_tp.NaN_position == NaNPosition::AtEnd) {
      Thread_NaN_offset = Size - NaN_counts [0];
    }
  }

  Adl_distribute_to_buckets_simd<Size_t, Value_t, Key_extractor_t, Radix_cp,
                                 Radix_tp, true>(
      /*Start_idx       */ 0,
      /*End_idx         */ Size,
      /*Pass            */ Radix_cp::Passes - 1,
      /*Src             */ Src,
      /*Dst             */ Dst,
      /*Scanned_offsets */ Scanned_offsets.data(),
      /*Extractor       */ Extractor,
      /*Thread_NaN_offset */ Handle_NaN ? Thread_NaN_offset : 0);
  std::swap(Src, Dst);

  if constexpr (Radix_cp::Passes & 1) std::move(Src, Src + Size, Start_ptr);
#else
  radix_sort_impl(std::execution::seq, First, Last);
#endif
}

template<ContiguousIterator ContigIter,
         auto Radix_tp = details::Adl_default_radix_params<ContigIter>()>
void radix_sort_impl(std::execution::parallel_policy, ContigIter First,
                     ContigIter Last)
{
  static_assert(is_valid_bucket_size<Radix_tp.Bucket_size>,
                "Bucket size must be 256 or 65536");

  using Value_t         = typename std::iterator_traits<ContigIter>::value_type;
  using Key_extractor_t = typename decltype(Radix_tp)::Key_extractor_t;
  Key_extractor_t Extractor {};
  using Key_t = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;

  static_assert(
      ArithmeticKey<Key_t>,
      "Key type must be arithmetic (excluding bool and character types)");

  static_assert(sizeof(Key_t) >= 4, "Arithmetic type must be at least 4 bytes");

  static_assert(
      !(std::is_unsigned_v<Key_t> && Radix_tp.Has_negative == HasNegative::Yes),
      "Cannot use HasNegative::Yes with unsigned key type");

  using Unsigned_t = std::make_unsigned_t<std::conditional_t<
      std::is_floating_point_v<Key_t>,
      std::conditional_t<sizeof(Key_t) == 4, std::uint32_t, std::uint64_t>,
      Key_t>>;

  using Size_t = typename decltype(Radix_tp)::Dataset_size_t;

  Size_t Size = static_cast<Size_t>(std::distance(First, Last));
  if (Size <= 1) [[unlikely]]
    return;

  Value_t* Start_ptr = &*First;

  using Radix_cp = Radix_constexpr_params<Key_t, Radix_tp.Bucket_size>;

  // Optimization: Dynamically adjust thread count to avoid small chunks
  const std::int32_t Hardware_concurrency = omp_get_num_procs();
  const std::int32_t Min_chunk_size       = 1024;
  const std::int32_t Actual_threads       = std::max(
      std::min(Hardware_concurrency, (std::int32_t)(Size / Min_chunk_size)), 1);
  const Size_t Chunk = (Size + Actual_threads - 1) / Actual_threads;  // Average
  const Size_t Local_data_size = Actual_threads * 2 * Radix_tp.Bucket_size;

  // <Continuous data classification> allocation layout: [thread0_bucket0,
  // thread1_bucket0, ... thread0_offset0, thread1_offset0, ...]
  //
  // <Thread-local data continuity> allocation layout: [thread0_bucket0,
  // thread0_offset0, thread1_bucket0, thread1_offset0, ...]
  //
  // Using <Thread-local data continuity> Take advantage of cache locality, help
  // CPU prediction, and have threads load data
  //
  // Merge local bucket count and offset arrays to reduce memory
  std::unique_ptr<Size_t []> Local_data { std::make_unique<Size_t []>(
      Local_data_size) };
  // Helper function: Get thread's bucket count pointer
  auto Func_get_local_bucket_counts = [&](int Thread_id) -> Size_t* {
    return Local_data.get() + Thread_id * 2 * Radix_tp.Bucket_size;
  };
  // Helper function: Get thread's offset pointer
  auto Func_get_local_scanned_offsets = [&](int Thread_id) -> Size_t* {
    return Local_data.get() + (Thread_id * 2 + 1) * Radix_tp.Bucket_size;
  };

  std::array<Size_t, Radix_tp.Bucket_size> Global_prefix {};

  std::unique_ptr<Value_t []> Buffer(new Value_t [Size] /*()*/);
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
  Value_t* __restrict Src = Start_ptr;
  Value_t* __restrict Dst = Buffer.get();
#else
  Value_t* Src = Start_ptr;
  Value_t* Dst = Buffer.get();
#endif

  LOOP_UNROLL(8) for (std::uint8_t Pass = 0; Pass < Radix_cp::Passes - 1; ++Pass)
  {
    // Phase 1: Parallel counting phase
    {
#pragma omp parallel num_threads(Actual_threads)
      {
        const int Thread_id           = omp_get_thread_num();
        Size_t*   Local_bucket_counts = Func_get_local_bucket_counts(Thread_id);

        // Calculate interval processed by current thread
        const Size_t Start_idx = Thread_id * Chunk;
        const Size_t End_idx   = std::min(Size, (Thread_id + 1) * Chunk);

        // Local counting
        Adl_count_buckets<Size_t, Value_t, Key_extractor_t, Radix_cp, Radix_tp,
                          false>(
            /*Start_idx     */ Start_idx,
            /*End_idx       */ End_idx,
            /*Pass          */ Pass,
            /*Src           */ Src,
            /*Bucket_counts */ Local_bucket_counts,
            /*Extractor     */ Extractor,
            /*NaN_counts    */ nullptr,
            /*Thread_id     */ 0);
      }
    }

    // Phase 2: Optimized prefix sum calculation
    {
      // Global reduction
      for (std::int32_t Thread = 0; Thread < Actual_threads; ++Thread) {
        Size_t* Local_bucket_counts = Func_get_local_bucket_counts(Thread);

        LOOP_UNROLL(8)
        for (std::uint32_t Bucket = 0; Bucket < Radix_tp.Bucket_size; ++Bucket) {
          Global_prefix [Bucket] += Local_bucket_counts [Bucket];
        }
      }

      //  Calculate global prefix sum
      std::exclusive_scan(Global_prefix.begin(), Global_prefix.end(),
                          Global_prefix.begin(), 0, std::plus<> {});

      // Calculate local offsets - Single pass completion
      for (std::int32_t Thread = 0; Thread < Actual_threads; ++Thread) {
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
        Size_t* __restrict Local_bucket_counts =
            Func_get_local_bucket_counts(Thread);
        Size_t* __restrict Local_scanned_offsets =
            Func_get_local_scanned_offsets(Thread);
#else
        Size_t* Local_bucket_counts   = Func_get_local_bucket_counts(Thread);
        Size_t* Local_scanned_offsets = Func_get_local_scanned_offsets(Thread);
#endif
        LOOP_UNROLL(8)
        for (std::uint32_t Bucket = 0; Bucket < Radix_tp.Bucket_size; ++Bucket) {
          Local_scanned_offsets [Bucket] = Global_prefix [Bucket];
          Global_prefix [Bucket] += Local_bucket_counts [Bucket];
        }
      }
    }

    // Phase 3: Parallel scattering phase
    {
#pragma omp parallel num_threads(Actual_threads)
      {
        const int Thread_id           = omp_get_thread_num();
        Size_t* Local_scanned_offsets = Func_get_local_scanned_offsets(Thread_id);

        const Size_t Start_idx = Thread_id * Chunk;
        const Size_t End_idx   = std::min(Size, (Thread_id + 1) * Chunk);

        // Scatter elements to correct positions

        Adl_distribute_to_buckets<Size_t, Value_t, Key_extractor_t, Radix_cp,
                                  Radix_tp, false>(
            /*Start_idx       */ Start_idx,
            /*End_idx         */ End_idx,
            /*Pass            */ Pass,
            /*Src             */ Src,
            /*Dst             */ Dst,
            /*Scanned_offsets */ Local_scanned_offsets,
            /*Extractor       */ Extractor,
            /*NaN_count       */ 0);
      }
    }

    std::swap(Src, Dst);

    std::fill(Local_data.get(), Local_data.get() + Local_data_size, 0);
    std::fill(Global_prefix.begin(), Global_prefix.end(), 0);
  }

  constexpr bool Handle_NaN = std::is_floating_point_v<Key_t> &&
      Radix_tp.NaN_position != NaNPosition::Unhandled;
  [[maybe_unused]] std::unique_ptr<Size_t []> Local_NaN_counts {
    Handle_NaN ? std::make_unique<Size_t []>(Actual_threads) : nullptr
  };
  [[maybe_unused]] std::unique_ptr<Size_t []> Thread_NaN_offsets {
    Handle_NaN ? std::make_unique<Size_t []>(Actual_threads) : nullptr
  };
  {
    {
#pragma omp parallel num_threads(Actual_threads)
      {
        const int Thread_id           = omp_get_thread_num();
        Size_t*   Local_bucket_counts = Func_get_local_bucket_counts(Thread_id);

        const Size_t Start_idx = Thread_id * Chunk;
        const Size_t End_idx   = std::min(Size, (Thread_id + 1) * Chunk);

        Adl_count_buckets<Size_t, Value_t, Key_extractor_t, Radix_cp, Radix_tp,
                          true>(
            /*Start_idx     */ Start_idx,
            /*End_idx       */ End_idx,
            /*Pass          */ Radix_cp::Passes - 1,
            /*Src           */ Src,
            /*Bucket_counts */ Local_bucket_counts,
            /*Extractor     */ Extractor,
            /*NaN_counts    */ Handle_NaN ? Local_NaN_counts.get() : nullptr,
            /*Thread_id     */ Handle_NaN ? Thread_id : 0);
      }
    }

    {
      [[maybe_unused]] Size_t Total_NaN_count = 0;
      if constexpr (Handle_NaN) {
        Total_NaN_count = std::accumulate(
            Local_NaN_counts.get(), Local_NaN_counts.get() + Actual_threads, 0);
      }

      for (std::int32_t Thread = 0; Thread < Actual_threads; ++Thread) {
        Size_t* Local_bucket_counts = Func_get_local_bucket_counts(Thread);

        LOOP_UNROLL(8)
        for (std::uint32_t Bucket = 0; Bucket < Radix_tp.Bucket_size; ++Bucket) {
          Global_prefix [Bucket] += Local_bucket_counts [Bucket];
        }
      }

      if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart && Handle_NaN) {
        std::exclusive_scan(Global_prefix.begin(), Global_prefix.end(),
                            Global_prefix.begin(), Total_NaN_count,
                            std::plus<> {});
      } else {
        std::exclusive_scan(Global_prefix.begin(), Global_prefix.end(),
                            Global_prefix.begin(), 0, std::plus<> {});
      }

      for (std::int32_t Thread = 0; Thread < Actual_threads; ++Thread) {
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
        Size_t* __restrict Local_bucket_counts =
            Func_get_local_bucket_counts(Thread);
        Size_t* __restrict Local_scanned_offsets =
            Func_get_local_scanned_offsets(Thread);
#else
        Size_t* Local_bucket_counts   = Func_get_local_bucket_counts(Thread);
        Size_t* Local_scanned_offsets = Func_get_local_scanned_offsets(Thread);
#endif
        LOOP_UNROLL(8)
        for (std::uint32_t Bucket = 0; Bucket < Radix_tp.Bucket_size; ++Bucket) {
          Local_scanned_offsets [Bucket] = Global_prefix [Bucket];
          Global_prefix [Bucket] += Local_bucket_counts [Bucket];
        }
      }

      if constexpr (Handle_NaN) {
        Size_t NaN_running_sum;
        if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart) {
          NaN_running_sum = 0;
        } else if constexpr (Radix_tp.NaN_position == NaNPosition::AtEnd) {
          NaN_running_sum = Size - Total_NaN_count;
        }

        LOOP_UNROLL(8)
        for (std::int32_t Thread = 0; Thread < Actual_threads; ++Thread) {
          Thread_NaN_offsets [Thread] = NaN_running_sum;
          NaN_running_sum += Local_NaN_counts [Thread];
        }
      }
    }

    {
#pragma omp parallel num_threads(Actual_threads)
      {
        const int Thread_id           = omp_get_thread_num();
        Size_t* Local_scanned_offsets = Func_get_local_scanned_offsets(Thread_id);

        const Size_t Start_idx = Thread_id * Chunk;
        const Size_t End_idx   = std::min(Size, (Thread_id + 1) * Chunk);

        Adl_distribute_to_buckets<Size_t, Value_t, Key_extractor_t, Radix_cp,
                                  Radix_tp, true>(
            /*Start_idx       */ Start_idx,
            /*End_idx         */ End_idx,
            /*Pass            */ Radix_cp::Passes - 1,
            /*Src             */ Src,
            /*Dst             */ Dst,
            /*Scanned_offsets */ Local_scanned_offsets,
            /*Extractor       */ Extractor,
            /*NaN_count       */ Handle_NaN ? Thread_NaN_offsets [Thread_id] : 0);
      }
    }
  }

  std::swap(Src, Dst);
}

template<ContiguousIterator ContigIter,
         auto Radix_tp = details::Adl_default_radix_params<ContigIter>()>
void radix_sort_impl(std::execution::parallel_unsequenced_policy,
                     ContigIter First, ContigIter Last)
{
#if defined(__AVX2__) || defined(__ARM_NEON) || defined(__aarch64__)
  static_assert(is_valid_bucket_size<Radix_tp.Bucket_size>,
                "Bucket size must be 256 or 65536");

  using Value_t         = typename std::iterator_traits<ContigIter>::value_type;
  using Key_extractor_t = typename decltype(Radix_tp)::Key_extractor_t;
  Key_extractor_t Extractor {};
  using Key_t = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;

  static_assert(
      ArithmeticKey<Key_t>,
      "Key type must be arithmetic (excluding bool and character types)");

  static_assert(sizeof(Key_t) >= 4, "Arithmetic type must be at least 4 bytes");

  static_assert(
      !(std::is_unsigned_v<Key_t> && Radix_tp.Has_negative == HasNegative::Yes),
      "Cannot use HasNegative::Yes with unsigned key type");

  using Unsigned_t = std::make_unsigned_t<std::conditional_t<
      std::is_floating_point_v<Key_t>,
      std::conditional_t<sizeof(Key_t) == 4, std::uint32_t, std::uint64_t>,
      Key_t>>;

  using Size_t = typename decltype(Radix_tp)::Dataset_size_t;

  Size_t Size = static_cast<Size_t>(std::distance(First, Last));
  if (Size <= 1) [[unlikely]]
    return;

  Value_t* Start_ptr = &*First;

  using Radix_cp = Radix_constexpr_params<Key_t, Radix_tp.Bucket_size>;

  // Optimization: Dynamically adjust thread count to avoid small chunks
  const std::int32_t Hardware_concurrency = omp_get_num_procs();
  const std::int32_t Min_chunk_size       = 1024;
  const std::int32_t Actual_threads       = std::max(
      std::min(Hardware_concurrency, (std::int32_t)(Size / Min_chunk_size)), 1);
  const Size_t Chunk = (Size + Actual_threads - 1) / Actual_threads;  // Average
  const Size_t Local_data_size = Actual_threads * 2 * Radix_tp.Bucket_size;

  // Merge local bucket count and offset arrays to reduce memory
  std::unique_ptr<Size_t []> Local_data { std::make_unique<Size_t []>(
      Local_data_size) };
  // Helper function: Get thread's bucket count pointer
  auto Func_get_local_bucket_counts = [&](int Thread_id) -> Size_t* {
    return Local_data.get() + Thread_id * 2 * Radix_tp.Bucket_size;
  };
  // Helper function: Get thread's offset pointer
  auto Func_get_local_scanned_offsets = [&](int Thread_id) -> Size_t* {
    return Local_data.get() + (Thread_id * 2 + 1) * Radix_tp.Bucket_size;
  };

  std::array<Size_t, Radix_tp.Bucket_size> Global_prefix {};

  std::unique_ptr<Value_t []> Buffer(new Value_t [Size]);
#  if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
  Value_t* __restrict Src = Start_ptr;
  Value_t* __restrict Dst = Buffer.get();
#  else
  Value_t* Src = Start_ptr;
  Value_t* Dst = Buffer.get();
#  endif

  LOOP_UNROLL(8) for (std::uint8_t Pass = 0; Pass < Radix_cp::Passes - 1; ++Pass)
  {
    // Phase 1: Parallel counting phase with SIMD optimization
    {
#  pragma omp parallel num_threads(Actual_threads)
      {
        const int Thread_id           = omp_get_thread_num();
        Size_t*   Local_bucket_counts = Func_get_local_bucket_counts(Thread_id);

        // Calculate interval processed by current thread
        const Size_t Start_idx = Thread_id * Chunk;
        const Size_t End_idx   = std::min(Size, (Thread_id + 1) * Chunk);

        // Local counting with SIMD
        Adl_count_buckets_simd<Size_t, Value_t, Key_extractor_t, Radix_cp,
                               Radix_tp, false>(
            /*Start_idx     */ Start_idx,
            /*End_idx       */ End_idx,
            /*Pass          */ Pass,
            /*Src           */ Src,
            /*Bucket_counts */ Local_bucket_counts,
            /*Extractor     */ Extractor,
            /*NaN_counts    */ nullptr,
            /*Thread_id     */ 0);
      }
    }

    // Phase 2: Optimized prefix sum calculation
    {
      // Global reduction
      for (std::int32_t Thread = 0; Thread < Actual_threads; ++Thread) {
        Size_t* Local_bucket_counts = Func_get_local_bucket_counts(Thread);

        LOOP_UNROLL(8)
        for (std::uint32_t Bucket = 0; Bucket < Radix_tp.Bucket_size; ++Bucket) {
          Global_prefix [Bucket] += Local_bucket_counts [Bucket];
        }
      }

      //  Calculate global prefix sum
      std::exclusive_scan(Global_prefix.begin(), Global_prefix.end(),
                          Global_prefix.begin(), 0, std::plus<> {});

      // Calculate local offsets - Single pass completion
      for (std::int32_t Thread = 0; Thread < Actual_threads; ++Thread) {
#  if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
        Size_t* __restrict Local_bucket_counts =
            Func_get_local_bucket_counts(Thread);
        Size_t* __restrict Local_scanned_offsets =
            Func_get_local_scanned_offsets(Thread);
#  else
        Size_t* Local_bucket_counts   = Func_get_local_bucket_counts(Thread);
        Size_t* Local_scanned_offsets = Func_get_local_scanned_offsets(Thread);
#  endif
        LOOP_UNROLL(8)
        for (std::uint32_t Bucket = 0; Bucket < Radix_tp.Bucket_size; ++Bucket) {
          Local_scanned_offsets [Bucket] = Global_prefix [Bucket];
          Global_prefix [Bucket] += Local_bucket_counts [Bucket];
        }
      }
    }

    // Phase 3: Parallel scattering phase with SIMD optimization
    {
#  pragma omp parallel num_threads(Actual_threads)
      {
        const int Thread_id           = omp_get_thread_num();
        Size_t* Local_scanned_offsets = Func_get_local_scanned_offsets(Thread_id);

        const Size_t Start_idx = Thread_id * Chunk;
        const Size_t End_idx   = std::min(Size, (Thread_id + 1) * Chunk);

        // Scatter elements to correct positions with SIMD
        Adl_distribute_to_buckets_simd<Size_t, Value_t, Key_extractor_t, Radix_cp,
                                       Radix_tp, false>(
            /*Start_idx       */ Start_idx,
            /*End_idx         */ End_idx,
            /*Pass            */ Pass,
            /*Src             */ Src,
            /*Dst             */ Dst,
            /*Scanned_offsets */ Local_scanned_offsets,
            /*Extractor       */ Extractor,
            /*Thread_NaN_offset */ 0);
      }
    }

    std::swap(Src, Dst);

    std::fill(Local_data.get(), Local_data.get() + Local_data_size, 0);
    std::fill(Global_prefix.begin(), Global_prefix.end(), 0);
  }

  constexpr bool Handle_NaN = std::is_floating_point_v<Key_t> &&
      Radix_tp.NaN_position != NaNPosition::Unhandled;
  [[maybe_unused]] std::unique_ptr<Size_t []> Local_NaN_counts {
    Handle_NaN ? std::make_unique<Size_t []>(Actual_threads) : nullptr
  };
  [[maybe_unused]] std::unique_ptr<Size_t []> Thread_NaN_offsets {
    Handle_NaN ? std::make_unique<Size_t []>(Actual_threads) : nullptr
  };
  {
    {
#  pragma omp parallel num_threads(Actual_threads)
      {
        const int Thread_id           = omp_get_thread_num();
        Size_t*   Local_bucket_counts = Func_get_local_bucket_counts(Thread_id);

        const Size_t Start_idx = Thread_id * Chunk;
        const Size_t End_idx   = std::min(Size, (Thread_id + 1) * Chunk);

        // Local counting with SIMD for last pass
        Adl_count_buckets_simd<Size_t, Value_t, Key_extractor_t, Radix_cp,
                               Radix_tp, true>(
            /*Start_idx     */ Start_idx,
            /*End_idx       */ End_idx,
            /*Pass          */ Radix_cp::Passes - 1,
            /*Src           */ Src,
            /*Bucket_counts */ Local_bucket_counts,
            /*Extractor     */ Extractor,
            /*NaN_counts    */ Handle_NaN ? Local_NaN_counts.get() : nullptr,
            /*Thread_id     */ Handle_NaN ? Thread_id : 0);
      }
    }

    {
      [[maybe_unused]] Size_t Total_NaN_count = 0;
      if constexpr (Handle_NaN) {
        Total_NaN_count = std::accumulate(
            Local_NaN_counts.get(), Local_NaN_counts.get() + Actual_threads, 0);
      }

      for (std::int32_t Thread = 0; Thread < Actual_threads; ++Thread) {
        Size_t* Local_bucket_counts = Func_get_local_bucket_counts(Thread);

        LOOP_UNROLL(8)
        for (std::uint32_t Bucket = 0; Bucket < Radix_tp.Bucket_size; ++Bucket) {
          Global_prefix [Bucket] += Local_bucket_counts [Bucket];
        }
      }

      if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart && Handle_NaN) {
        std::exclusive_scan(Global_prefix.begin(), Global_prefix.end(),
                            Global_prefix.begin(), Total_NaN_count,
                            std::plus<> {});
      } else {
        std::exclusive_scan(Global_prefix.begin(), Global_prefix.end(),
                            Global_prefix.begin(), 0, std::plus<> {});
      }

      for (std::int32_t Thread = 0; Thread < Actual_threads; ++Thread) {
#  if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
        Size_t* __restrict Local_bucket_counts =
            Func_get_local_bucket_counts(Thread);
        Size_t* __restrict Local_scanned_offsets =
            Func_get_local_scanned_offsets(Thread);
#  else
        Size_t* Local_bucket_counts   = Func_get_local_bucket_counts(Thread);
        Size_t* Local_scanned_offsets = Func_get_local_scanned_offsets(Thread);
#  endif
        LOOP_UNROLL(8)
        for (std::uint32_t Bucket = 0; Bucket < Radix_tp.Bucket_size; ++Bucket) {
          Local_scanned_offsets [Bucket] = Global_prefix [Bucket];
          Global_prefix [Bucket] += Local_bucket_counts [Bucket];
        }
      }

      if constexpr (Handle_NaN) {
        Size_t NaN_running_sum;
        if constexpr (Radix_tp.NaN_position == NaNPosition::AtStart) {
          NaN_running_sum = 0;
        } else if constexpr (Radix_tp.NaN_position == NaNPosition::AtEnd) {
          NaN_running_sum = Size - Total_NaN_count;
        }

        LOOP_UNROLL(8)
        for (std::int32_t Thread = 0; Thread < Actual_threads; ++Thread) {
          Thread_NaN_offsets [Thread] = NaN_running_sum;
          NaN_running_sum += Local_NaN_counts [Thread];
        }
      }
    }

    {
#  pragma omp parallel num_threads(Actual_threads)
      {
        const int Thread_id           = omp_get_thread_num();
        Size_t* Local_scanned_offsets = Func_get_local_scanned_offsets(Thread_id);

        const Size_t Start_idx = Thread_id * Chunk;
        const Size_t End_idx   = std::min(Size, (Thread_id + 1) * Chunk);

        // Scatter elements to correct positions with SIMD for last pass
        Adl_distribute_to_buckets_simd<Size_t, Value_t, Key_extractor_t, Radix_cp,
                                       Radix_tp, true>(
            /*Start_idx       */ Start_idx,
            /*End_idx         */ End_idx,
            /*Pass            */ Radix_cp::Passes - 1,
            /*Src             */ Src,
            /*Dst             */ Dst,
            /*Scanned_offsets */ Local_scanned_offsets,
            /*Extractor       */ Extractor,
            /*Thread_NaN_offset */
            Handle_NaN ? Thread_NaN_offsets [Thread_id] : 0);
      }
    }
  }

  std::swap(Src, Dst);

#else
  radix_sort_impl(std::execution::par, First, Last);
#endif
}

}  // namespace details

template<details::ContiguousIterator ContigIter>
void radix_sort(ContigIter first, ContigIter last)
{
  details::radix_sort_impl(std::execution::seq, first, last);
}

template<details::ContiguousIterator       ContigIter,
         details::SupportedExecutionPolicy ExPo>
void radix_sort(ExPo&& policy, ContigIter first, ContigIter last)
{
  details::radix_sort_impl(std::forward<ExPo>(policy), first, last);
}

template<details::ContiguousIterator ContigIter, auto Radix_tp>
void radix_sort(ContigIter first, ContigIter last)
{
  details::radix_sort_impl<ContigIter, Radix_tp>(std::execution::seq, first,
                                                 last);
}

template<details::ContiguousIterator       ContigIter,
         details::SupportedExecutionPolicy ExPo, auto Radix_tp>
void radix_sort(ExPo&& policy, ContigIter first, ContigIter last)
{
  details::radix_sort_impl<ContigIter, Radix_tp>(std::forward<ExPo>(policy),
                                                 first, last);
}

template<details::ContiguousIterator ContigIter, typename KeyType>
void radix_sort_by_member(ContigIter first, ContigIter last,
                          KeyType(std::iter_value_t<ContigIter>::* member_ptr))
{
  using Value_t = std::iter_value_t<ContigIter>;
  details::radix_sort_impl<
      ContigIter,
      Radix_template_params<member_key_extractor<Value_t, KeyType>,
                            std::uint32_t> {}>(std::execution::seq, first, last);
}

template<details::ContiguousIterator       ContigIter,
         details::SupportedExecutionPolicy ExPo, typename KeyType>
void radix_sort_by_member(ExPo&& policy, ContigIter first, ContigIter last,
                          KeyType(std::iter_value_t<ContigIter>::* member_ptr))
{
  using Value_t = std::iter_value_t<ContigIter>;
  details::radix_sort_impl<
      ContigIter,
      Radix_template_params<member_key_extractor<Value_t, KeyType>,
                            std::uint32_t> {}>(std::forward<ExPo>(policy), first,
                                               last);
}

template<details::ContiguousIterator ContigIter, auto Radix_tp, typename KeyType>
void radix_sort_by_member(ContigIter first, ContigIter last,
                          KeyType(std::iter_value_t<ContigIter>::* member_ptr))
{
  using Value_t     = std::iter_value_t<ContigIter>;
  using NewRadix_tp = decltype(Radix_tp);
  using NewParams = Radix_template_params<member_key_extractor<Value_t, KeyType>,
                                          typename NewRadix_tp::Dataset_size_t>;
  NewParams new_params {};
  new_params.Bucket_size  = Radix_tp.Bucket_size;
  new_params.Order        = Radix_tp.Order;
  new_params.Has_negative = Radix_tp.Has_negative;
  new_params.NaN_position = Radix_tp.NaN_position;
  details::radix_sort_impl<ContigIter, new_params>(std::execution::seq, first,
                                                   last);
}

template<details::ContiguousIterator       ContigIter,
         details::SupportedExecutionPolicy ExPo, auto Radix_tp, typename KeyType>
void radix_sort_by_member(ExPo&& policy, ContigIter first, ContigIter last,
                          KeyType(std::iter_value_t<ContigIter>::* member_ptr))
{
  using Value_t     = std::iter_value_t<ContigIter>;
  using NewRadix_tp = decltype(Radix_tp);
  using NewParams = Radix_template_params<member_key_extractor<Value_t, KeyType>,
                                          typename NewRadix_tp::Dataset_size_t>;
  NewParams new_params {};
  new_params.Bucket_size  = Radix_tp.Bucket_size;
  new_params.Order        = Radix_tp.Order;
  new_params.Has_negative = Radix_tp.Has_negative;
  new_params.NaN_position = Radix_tp.NaN_position;
  details::radix_sort_impl<ContigIter, new_params>(std::forward<ExPo>(policy),
                                                   first, last);
}

template<details::ContiguousIterator ContigIter, typename Func>
void radix_sort_by(ContigIter first, ContigIter last, Func&& func)
{
  using Value_t = std::iter_value_t<ContigIter>;
  using Func_t  = std::decay_t<Func>;
  details::radix_sort_impl<
      ContigIter,
      Radix_template_params<function_key_extractor<Func_t>, std::uint32_t> {}>(
      std::execution::seq, first, last);
}

template<details::ContiguousIterator       ContigIter,
         details::SupportedExecutionPolicy ExPo, typename Func>
void radix_sort_by(ExPo&& policy, ContigIter first, ContigIter last, Func&& func)
{
  using Value_t = std::iter_value_t<ContigIter>;
  using Func_t  = std::decay_t<Func>;
  details::radix_sort_impl<
      ContigIter,
      Radix_template_params<function_key_extractor<Func_t>, std::uint32_t> {}>(
      std::forward<ExPo>(policy), first, last);
}

template<details::ContiguousIterator ContigIter, auto Radix_tp, typename Func>
void radix_sort_by(ContigIter first, ContigIter last, Func&& func)
{
  using Value_t     = std::iter_value_t<ContigIter>;
  using Func_t      = std::decay_t<Func>;
  using NewRadix_tp = decltype(Radix_tp);
  using NewParams   = Radix_template_params<function_key_extractor<Func_t>,
                                            typename NewRadix_tp::Dataset_size_t>;
  NewParams new_params {};
  new_params.Bucket_size  = Radix_tp.Bucket_size;
  new_params.Order        = Radix_tp.Order;
  new_params.Has_negative = Radix_tp.Has_negative;
  new_params.NaN_position = Radix_tp.NaN_position;
  details::radix_sort_impl<ContigIter, new_params>(std::execution::seq, first,
                                                   last);
}

template<details::ContiguousIterator       ContigIter,
         details::SupportedExecutionPolicy ExPo, auto Radix_tp, typename Func>
void radix_sort_by(ExPo&& policy, ContigIter first, ContigIter last, Func&& func)
{
  using Value_t     = std::iter_value_t<ContigIter>;
  using Func_t      = std::decay_t<Func>;
  using NewRadix_tp = decltype(Radix_tp);
  using NewParams   = Radix_template_params<function_key_extractor<Func_t>,
                                            typename NewRadix_tp::Dataset_size_t>;
  NewParams new_params {};
  new_params.Bucket_size  = Radix_tp.Bucket_size;
  new_params.Order        = Radix_tp.Order;
  new_params.Has_negative = Radix_tp.Has_negative;
  new_params.NaN_position = Radix_tp.NaN_position;
  details::radix_sort_impl<ContigIter, new_params>(std::forward<ExPo>(policy),
                                                   first, last);
}

// TODO
namespace experimental {
// You might use this version, as it has lower memory overhead and high
// performance when the passed-in type <Value_t> is large.
template<details::ContiguousIterator ContigIter,
         auto Radix_tp = details::Adl_default_radix_params<ContigIter>()>
void radix_sort_indexed_impl(std::execution::sequenced_policy, ContigIter First,
                             ContigIter Last)
{
  using Value_t         = std::iter_value_t<ContigIter>;
  using Key_extractor_t = typename decltype(Radix_tp)::Key_extractor_t;
  Key_extractor_t Extractor {};
  using Key_t = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;

  static_assert(
      details::ArithmeticKey<Key_t>,
      "Key type must be arithmetic (excluding bool and character types)");
  static_assert(!(sizeof(Key_t) == 1 && Radix_tp.Bucket_size == 65536U),
                "Cannot use 65536 bucket size with 1-byte key type");

  using Radix_cp   = details::Radix_constexpr_params<Key_t, Radix_tp.Bucket_size>;
  using Unsigned_t = typename Radix_cp::Unsigned_t;
  using Size_t     = typename decltype(Radix_tp)::Dataset_size_t;

  Size_t Size = std::distance(First, Last);
  if (Size <= 1) [[unlikely]]
    return;
  auto* Ptr = &*First;

  std::array<Size_t, Radix_tp.Bucket_size> Bucket_count;
  std::array<Size_t, Radix_tp.Bucket_size> Scanned;

  // Size_t is 4Byte or 8Byte, can set
  std::unique_ptr<Size_t []> indices_current(new Size_t [Size]);
  std::unique_ptr<Size_t []> indices_next(new Size_t [Size]);
  /*
  // also can use Pointer, it depends on the computer's bit version
  std::unique_ptr<Value_t* []> ptrs_current(new Value_t * [Size]);
  std::unique_ptr<Value_t* []> ptrs_next(new Value_t * [Size]);
  */
  for (Size_t i = 0; i < Size; ++i) {
    indices_current [i] = i;
  }
  Size_t* __restrict idx_src = indices_current.get();
  Size_t* __restrict idx_dst = indices_next.get();
  /*
  Value_t** __restrict ptr_src = ptrs_current.get();
  Value_t** __restrict ptr_dst = ptrs_next.get();
  */
  for (std::uint8_t Pass = 0; Pass < Radix_cp::Passes; ++Pass) {
    std::fill(Bucket_count.begin(), Bucket_count.end(), 0);

    for (Size_t i = 0; i < Size; ++i) {
      Size_t     data_idx = idx_src [i];
      Unsigned_t Unsigned_value =
          std::bit_cast<Unsigned_t>(Extractor(Ptr [data_idx]));

      if constexpr (std::is_floating_point_v<Key_t> &&
                    Radix_tp.Has_negative == HasNegative::Yes) {
        Unsigned_value ^= (Unsigned_value >> Radix_cp::Shift_of_sign_bit)
            ? Radix_cp::All_bits_mask
            : Radix_cp::Sign_bit_mask;
      } else if constexpr (std::is_signed_v<Key_t> &&
                           Radix_tp.Has_negative == HasNegative::Yes) {
        Unsigned_value ^= Radix_cp::Sign_bit_mask;
      }

      std::uint16_t Byte_idx =
          (Unsigned_value >> (Pass << Radix_cp::Shift_of_byte_idx)) &
          Radix_cp::Mask;
      if constexpr (Radix_tp.Order == SortOrder::Descending)
        Byte_idx = Radix_cp::Mask - Byte_idx;
      ++Bucket_count [Byte_idx];
    }

    std::exclusive_scan(Bucket_count.begin(), Bucket_count.end(), Scanned.begin(),
                        0, std::plus<> {});

    for (Size_t i = 0; i < Size; ++i) {
      Size_t     data_idx = idx_src [i];
      Unsigned_t Unsigned_value =
          std::bit_cast<Unsigned_t>(Extractor(Ptr [data_idx]));

      if constexpr (std::is_floating_point_v<Key_t> &&
                    Radix_tp.Has_negative == HasNegative::Yes) {
        Unsigned_value ^= (Unsigned_value >> Radix_cp::Shift_of_sign_bit)
            ? Radix_cp::All_bits_mask
            : Radix_cp::Sign_bit_mask;
      } else if constexpr (std::is_signed_v<Key_t> &&
                           Radix_tp.Has_negative == HasNegative::Yes) {
        Unsigned_value ^= Radix_cp::Sign_bit_mask;
      }

      std::uint16_t Byte_idx =
          (Unsigned_value >> (Pass << Radix_cp::Shift_of_byte_idx)) &
          Radix_cp::Mask;
      if constexpr (Radix_tp.Order == SortOrder::Descending)
        Byte_idx = Radix_cp::Mask - Byte_idx;

      idx_dst [Scanned [Byte_idx]++] = data_idx;
    }

    std::swap(idx_src, idx_dst);
  }
  if constexpr (true) {
    std::unique_ptr<Value_t []> temp_buffer(new Value_t [Size]);
    for (Size_t i = 0; i < Size; ++i) {
      temp_buffer [i] = std::move(Ptr [idx_src [i]] /* *ptr_src[i] */);
    }

    std::move(temp_buffer.get(), temp_buffer.get() + Size, Ptr);
  } else {
    // slow but the space complexity is O(1)
    for (Size_t i = 0; i < Size; ++i) {
      while (idx_src [i] != i) {
        Size_t target_idx = idx_src [i];
        std::swap(Ptr [i], Ptr [target_idx]);
        std::swap(idx_src [i], idx_src [target_idx]);
      }
    }
  }
}
enum class SortVariant
{
  Improved,
  Indexed
};
constexpr std::size_t L1_CACHE_SIZE = 32 * 1024;
constexpr std::size_t L2_CACHE_SIZE = 256 * 1024;
constexpr std::size_t L3_CACHE_SIZE = 8 * 1024 * 1024;

constexpr std::size_t DATA_THRESHOLD   = 10000;
constexpr std::size_t STRUCT_THRESHOLD = 64;

template<std::size_t StructSize, bool IsPOD>
constexpr SortVariant select_sort_variant(std::size_t data_size)
{
  if (IsPOD) {
    if (StructSize <= STRUCT_THRESHOLD || data_size < DATA_THRESHOLD) {
      return SortVariant::Indexed;
    }
    return SortVariant::Improved;
  } else {
    return StructSize <= STRUCT_THRESHOLD ? SortVariant::Improved
                                          : SortVariant::Indexed;
  }
}

template<details::ContiguousIterator ContigIter>
void dispatch_radix_sort(std::execution::sequenced_policy policy,
                         ContigIter first, ContigIter last)
{
  using Value_t = std::iter_value_t<ContigIter>;

  const std::size_t     data_size   = std::distance(first, last);
  constexpr std::size_t struct_size = sizeof(Value_t);
  constexpr bool        is_pod      = std::is_trivially_copyable_v<Value_t>;

  const SortVariant variant = select_sort_variant<struct_size, is_pod>(data_size);

  switch (variant) {
  case SortVariant::Improved : break;
  case SortVariant::Indexed  : break;
  default :
#ifdef __GNUC__  // GCC, Clang, ICC
    __builtin_unreachable();
#elif defined(_MSC_VER)  // msvc
    __assume(false);
#endif
  }
}
}  // namespace experimental
}  // namespace stdex

#undef WARNING
#undef ALWAYS_INLINE
#undef LOOP_UNROLL
#undef EXPAND_AND_STRINGIFY
#undef STRINGIFY