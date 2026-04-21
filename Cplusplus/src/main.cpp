#include <omp.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <concepts>
#include <execution>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <type_traits>

#include <print>

#pragma push_macro("ALWAYS_INLINE")
#pragma push_macro("RESTRICT_KEYWORD")
#undef ALWAYS_INLINE
#undef RESTRICT_KEYWORD

#if defined(__GNUC__)
#  define ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#  define ALWAYS_INLINE inline __forceinline
#else
#  define LOOP_UNROLL(n)
#  define ALWAYS_INLINE inline
#endif

#if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
#  define RESTRICT_KEYWORD __restrict
#else // assume unsupported compiler
#  define RESTRICT_KEYWORD
#endif

namespace stdexx
{
enum class radix_order
{
  asc,
  desc
};

enum class radix_nan_position
{
  //< Temp Data: {INFINITY, -INFINITY, NAN, -0.0f, 1.0f / 1.0f, 0.0f, -1.0f / 1.0f,
  //         std::sqrt(-1.0f), 3.14f}

  unhandled, // maybe output: -nan(ind) -inf -1 -0 0 1 3.14 inf nan
  begin,     // maybe output: -nan(ind) nan -inf -1 -0 0 1 3.14 inf
  end        // maybe output: -inf -1 -0 0 1 3.14 inf -nan(ind) nan
};

template<typename Ty_>
struct radix_default_key_function
{
  constexpr Ty_ operator() (Ty_ Val_) const noexcept { return Val_; }
};

struct radix_options
{
  uint8_t            unroll_n    = 8U;
  uint16_t           bucket_size = 256U;
  uint32_t           chunk_size  = 256U * 256U;
  radix_order        sort_order  = radix_order::asc;
  radix_nan_position nan_pos     = radix_nan_position::begin;
};

template<typename Exec_policy_>
concept Supported_execution_policy_ =
    std::same_as<std::decay_t<Exec_policy_>, std::execution::sequenced_policy> ||
    std::same_as<std::decay_t<Exec_policy_>, std::execution::parallel_policy> ||
    std::same_as<std::decay_t<Exec_policy_>, std::execution::unsequenced_policy> ||
    std::same_as<std::decay_t<Exec_policy_>, std::execution::parallel_unsequenced_policy>;

template<typename Alloc_>
concept Standard_allocator_ = requires (Alloc_ A_, typename Alloc_::value_type *P_, std::size_t N_) {
  typename Alloc_::value_type;

  std::is_default_constructible_v<Alloc_>;
  std::is_copy_constructible_v<Alloc_>;
  std::is_move_constructible_v<Alloc_>;

  { A_.allocate(N_) } -> std::same_as<typename Alloc_::value_type *>;

  { A_.deallocate(P_, N_) } noexcept -> std::same_as<void>;

  { A_ == A_ } -> std::convertible_to<bool>;
  { A_ != A_ } -> std::convertible_to<bool>;
};

template<typename Func_, typename Ty_>
concept Invocable_key_function_ = std::invocable<Func_, Ty_ const &> && !std::invocable<Func_>;

template<typename Iter_>
concept Random_access_iterator_ = std::random_access_iterator<Iter_>;

struct Max_partition_result_
{
  size_t Part_index_;
  size_t Max_val_;
};

//< Only applies to floating-point numbers,
//< while dividing according to NaN, find the maximum based on the projection.
//
//< Proj_func_.F_() return floating-point
//< Proj_func_()    return projection, type is size_t
template<Random_access_iterator_ Random_iter_, typename Predicate_, typename Proj_functor_>
inline Max_partition_result_
Max_while_partition_(Random_iter_         Begin_,
                     Random_iter_         End_,
                     Predicate_ const    &Pred_,
                     Proj_functor_ const &Proj_func_) noexcept
{
  size_t Max_val_ = 0;

  auto First_ = Begin_, Last_ = End_;

  for (;;)
  {
    for (;;)
    {
      if (First_ == Last_)
      {
        return { static_cast<size_t>(std::distance(Begin_, First_)), Max_val_ };
      }

      if (!Pred_(*First_))
      {
        break;
      }

      if (!std::isnan(Proj_func_.F_(*First_)))
      {
        if (auto const Curr_val_ = Proj_func_(*First_); Curr_val_ > Max_val_)
        {
          Max_val_ = Curr_val_;
        }
      }

      ++First_;
    }

    do
    {
      --Last_;

      if (First_ == Last_)
      {
        if (!std::isnan(Proj_func_.F_(*First_)))
        {
          if (auto const Curr_val_ = Proj_func_(*First_); Curr_val_ > Max_val_)
          {
            Max_val_ = Curr_val_;
          }
        }
        return { static_cast<size_t>(std::distance(Begin_, First_)), Max_val_ };
      }
    }
    while (!Pred_(*Last_));

    if (!std::isnan(Proj_func_.F_(*First_)))
    {
      if (auto const Curr_val_ = Proj_func_(*First_); Curr_val_ > Max_val_)
      {
        Max_val_ = Curr_val_;
      }
    }

    std::iter_swap(First_, Last_);

    ++First_;
  }
}

template<bool Is_parallel_, uint32_t Loop_N_, typename Random_point_, typename Proj_functor_>
inline size_t
Max_element_(Random_point_ Begin_, size_t Size_, Proj_functor_ const &Proj_func_) noexcept
{
  size_t Max_val_ { 0 };

  if constexpr (Is_parallel_)
  {
    int Actual_threads_ = omp_get_num_procs();
#if defined(_MSC_VER) && !defined(_OPENMP_LLVM_RUNTIME)
#  pragma omp parallel num_threads(Actual_threads_)
    {
      auto Local_val_ = Max_val_;

      // openmp 2.0 does not support "reduction(max : ?)" and loop index must be signed
#  pragma omp for nowait
      for (int64_t Index_ { 0 }; Index_ < Size_; ++Index_)
      {
        if (auto const Current_val_ = Proj_func_(Begin_[Index_]); Current_val_ > Local_val_)
        {
          Local_val_ = Current_val_;
        }
      }

#  pragma omp critical
      {
        if (Local_val_ > Max_val_)
        {
          Max_val_ = Local_val_;
        }
      }
    }
#else /* GNU, Clang, MSVC(/openmp:llvm) */
#  pragma omp parallel for reduction(max : Max_val_) num_threads(Actual_threads_)
    for (size_t Index_ { 0 }; Index_ < Size_; ++Index_)
    {
      if (auto const Current_val_ = Proj_func_(Begin_[Index_]); Current_val_ > Max_val_)
      {
        Max_val_ = Current_val_;
      }
    }
#endif
  }
  else
  {
#if defined(_MSC_VER) && !defined(_OPENMP_LLVM_RUNTIME)

    Unroll_loop_<Loop_N_>(0, Size_, [&](size_t Index_)
    {
      if (auto const Val_ = Proj_func_(Begin_[Index_]); Val_ > Max_val_)
      {
        Max_val_ = Val_;
      }
    });
#else
#  pragma omp simd reduction(max : Max_val_)
    for (size_t Index_ { 0 }; Index_ < Size_; ++Index_)
    {
      if (auto const Val_ = Proj_func_(Begin_[Index_]); Val_ > Max_val_)
      {
        Max_val_ = Val_;
      }
    }
#endif
  }

  return Max_val_;
}

enum class Unroll_loop_direction_
{
  Increasing_,
  Decreasing_
};

template<Unroll_loop_direction_ Loop_direct_, size_t Unroll_size_, typename Function_>
ALWAYS_INLINE void
Unrolled_call_(size_t Base_, Function_ const &Func_) noexcept
{
  [&]<size_t... Is_>(std::index_sequence<Is_...>)
  {
    if constexpr (Loop_direct_ == Unroll_loop_direction_::Increasing_)
    {
      (Func_(Base_ + Is_), ...);
    }
    else
    {
      (Func_(Base_ - Is_), ...);
    }
  }.template operator() (std::make_index_sequence<Unroll_size_> {});
}

template<Unroll_loop_direction_ Loop_direct_, size_t Unroll_size_, typename Function_>
ALWAYS_INLINE void
Unroll_loop_(size_t Start_, size_t End_, Function_ const &Func_) noexcept
{
  static_assert((Unroll_size_ & (Unroll_size_ - 1)) == 0, "Unroll size must be a power of two");

  auto const [Main_iters_, Remainder_] = ::lldiv(End_ - Start_, Unroll_size_);

  for (size_t Index_ { 0 }; Index_ < (size_t) Main_iters_; ++Index_)
  {
    Unrolled_call_<Unroll_size_>(Start_ + Index_ * Unroll_size_, Func_);
  }

  for (size_t Index_ { 0 }; Index_ < (size_t) Remainder_; ++Index_)
  {
    Func_(Start_ + Index_ + ((size_t) Main_iters_ * Unroll_size_));
  }
}

template<size_t Start_, size_t End_, size_t Unroll_size_, typename Function_>
ALWAYS_INLINE void
Unroll_loop_(Function_ const &Func_) noexcept
{
  static_assert((Unroll_size_ & (Unroll_size_ - 1)) == 0, "Unroll size must be a power of two");

  constexpr size_t Size_       = End_ - Start_;
  constexpr size_t Main_iters_ = Size_ / Unroll_size_;
  constexpr size_t Remainder_  = Size_ % Unroll_size_;

  for (size_t Index_ { 0 }; Index_ < Main_iters_; ++Index_)
  {
    Unrolled_call_<Unroll_size_>(Start_ + Index_ * Unroll_size_, Func_);
  }

  for (size_t Index_ { 0 }; Index_ < Remainder_; ++Index_)
  {
    Func_(Start_ + Index_ + (Main_iters_ * Unroll_size_));
  }
}

// Allocate and construct a buffer
template<typename Allocator_>
ALWAYS_INLINE typename ::std::allocator_traits<Allocator_>::pointer
Construct_buffer_(size_t N_, Allocator_ &Alloc_)
{
  using Traits_     = ::std::allocator_traits<Allocator_>;
  using Value_type_ = typename Allocator_::value_type;
  using Pointer_    = typename Traits_::pointer;

  Pointer_ const P_ = Alloc_.allocate(N_);

  // If the objects being sorted have trivial default initialization, they do not need to be
  // initialized here. This can benefit performance.
  if (!::std::is_trivially_default_constructible_v<Value_type_>)
  {
    for (size_t I_ = 0; I_ < N_; ++I_)
    {
      // Objects being sorted must be default-initializable
      Traits_::construct(Alloc_, P_ + I_);
    }
  }

  return P_;
}

// Destroy and deallocate a buffer
template<typename Allocator_>
ALWAYS_INLINE void
Destroy_buffer_(typename ::std::allocator_traits<Allocator_>::pointer P_, size_t N_, Allocator_ &Alloc_)
{
  using Traits_ = ::std::allocator_traits<Allocator_>;

  // If the objects being sorted have trivial destruction, they do not need to be
  // destroyed here. This can benefit performance.
  if (!::std::is_trivially_destructible_v<typename Allocator_::value_type>)
  {
    for (size_t I_ = 0; I_ < N_; ++I_)
    {
      Traits_::destroy(Alloc_, P_ + I_);
    }
  }

  Alloc_.deallocate(P_, N_);
}

template<typename Allocator_>
class AllocatedBufferHolder_
{
public:
  AllocatedBufferHolder_(size_t Size_, Allocator_ const &Alloc_)
      : M_size_(Size_)
      , M_alloc_(Alloc_)
      , M_buffer_(Construct_buffer_(Size_, M_alloc_))
  {
  }

  ~AllocatedBufferHolder_() { Destroy_buffer_(M_buffer_, M_size_, M_alloc_); }

  typename std::allocator_traits<Allocator_>::pointer Get_buffer_() { return M_buffer_; }

private:
  size_t                                              M_size_;
  Allocator_                                          M_alloc_;
  typename std::allocator_traits<Allocator_>::pointer M_buffer_;
};

template<typename Key_function_, typename Ty_>
struct Radix_projection_functor_
{
  static_assert(
      Invocable_key_function_<Key_function_, Ty_>,
      "Key_function_ must be callable with exactly one required argument of type Ty_, const Ty_, or const Ty_&. "
      "Additional arguments must have default values.");

  using Key_ty_ = std::remove_cvref_t<std::invoke_result_t<Key_function_, Ty_>>;

  static_assert(std::is_arithmetic_v<Key_ty_>, "Type must be arithmetic (integral or floating-point)!");
  // does not support types like __int128, which are larger than size_t
  static_assert((sizeof(Key_ty_) <= sizeof(size_t)), "Type size is bigger than size_t size.");

  Key_function_ F_;

  constexpr size_t operator() (Ty_ const &Val_) const noexcept
  {
    if constexpr (std::unsigned_integral<Ty_>)
    {
      return F_(Val_);
    }
    else if constexpr (std::signed_integral<Ty_>)
    {
      // The default function needs to take the signed integer-like representation and map it to an unsigned one. The
      // following code will take the midpoint of the unsigned representable range (SIZE_MAX/2)+1 and does an unsigned
      // add of the value. Thus, it maps a [-signed_min,+signed_max] range into a [0, unsigned_max] range.
      return ((std::numeric_limits<size_t>::max() / 2) + 1) + static_cast<size_t>(F_(Val_));

      // Another strategy "return std::bit_cast<std::make_unsigned_t<Ty_>>(F_(Val_)) ^ Sign_bit_mask_;",
      // this requires that the type Ty_ and the return value must be exactly the same,
      // so use "return static_cast<size_t>(F_(Val_)) ^ Sign_bit_mask_;",
      //
      // if only LSD sorting is used, only the highest significant bit needs to be processed.
      // Currently, a mixed strategy of MSD and LSD is adopted, using the above scheme applied to the function-Radix_key.
    }
    else /* std::floating_point<Ty_> */
    {
      using Unsigned_ty_ = std::conditional_t<std::same_as<Ty_, float>, std::uint32_t, std::uint64_t>;
      constexpr std::uint8_t Shift_of_sign_bit_ = sizeof(Ty_) * 8 - 1;
      constexpr Unsigned_ty_ Sign_bit_mask_     = static_cast<Unsigned_ty_>(1) << Shift_of_sign_bit_;

      Unsigned_ty_ const Unsigned_val_ = std::bit_cast<Unsigned_ty_>(F_(Val_));

      // NaN cannot be compared, so we do not use >=, but instead use shifting
      return ((Unsigned_val_ >> Shift_of_sign_bit_) == 0) ? Unsigned_val_ | Sign_bit_mask_ : ~Unsigned_val_;
    }
  }
};

template<radix_options Radix_opts_, typename Ty_, typename Proj_functor_>
ALWAYS_INLINE size_t
Radix_key_(Ty_ const &Val_, size_t Radix_, Proj_functor_ const &Proj_func_)
{
  // Mask (value 255 or 65535) and Shift (value 8 or 16) depends on the size of the bucket
  constexpr uint16_t Mask_  = Radix_opts_.bucket_size - 1;
  constexpr uint16_t Shift_ = Radix_opts_.bucket_size == 256U ? 8 : 16;

  if constexpr (Radix_opts_.sort_order == radix_order::asc)
  {
    return (Proj_func_(Val_) >> static_cast<uint16_t>(Shift_ * Radix_)) & Mask_;
  }
  else /* Radix_opts_.sort_order == radix_order::desc */
  {
    return Mask_ - ((Proj_func_(Val_) >> static_cast<uint16_t>(Shift_ * Radix_)) & Mask_);
  }
}

template<radix_options Radix_opts_, typename Random_point_, typename Random_buffer_point_, typename Proj_functor_>
ALWAYS_INLINE void
LSD_integer_radix_pass_(Random_point_ &RESTRICT_KEYWORD        Begin_,
                        size_t                                 Size_,
                        Random_buffer_point_ &RESTRICT_KEYWORD Output_,
                        size_t                                 Curr_radix_,
                        Proj_functor_ const                   &Proj_func_)
{
  size_t Pos_[Radix_opts_.bucket_size] {};
  // Histogram
  Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
  {
    ++Pos_[Radix_key_<Radix_opts_>(Begin_[Index_], Curr_radix_, Proj_func_)];
  });

  // Prefix Sum
  Unroll_loop_<1, Radix_opts_.bucket_size, Radix_opts_.unroll_n>([&](size_t Index_)
  {
    Pos_[Index_] += Pos_[Index_ - 1];
  });

  // Collect
  Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
  {
    size_t Curr_index_ = Size_ - 1 - Index_;
    Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[Curr_index_], Curr_radix_, Proj_func_)]] =
        std::move(Begin_[Curr_index_]);
  });
  std::swap(Begin_, Output_);
}

template<radix_options Radix_opts_, typename Random_point_, typename Random_buffer_point_, typename Proj_functor_>
ALWAYS_INLINE void
LSD_integer_radix_sort_(Random_point_ &RESTRICT_KEYWORD        Begin_,
                        size_t                                 Size_,
                        Random_buffer_point_ &RESTRICT_KEYWORD Output_,
                        size_t                                 Radix_,
                        Proj_functor_ const                   &Proj_func_,
                        size_t                                 Deep_ = 0)
{
  if (Size_ == 0)
  {
    return;
  }

  if constexpr (Radix_opts_.bucket_size == 256U)
  {
    Radix_ = Radix_ + 1;
  }
  else /*if constexpr (Radix_opts_.bucket_size == 65536U)*/
  {
    Radix_ = Radix_ / 2 + 1; // floor
  }

  for (size_t Curr_radix_ { 0 }; Curr_radix_ < Radix_; ++Curr_radix_)
  {
    size_t Pos_[Radix_opts_.bucket_size] {};
    // Histogram
    //for (size_t Index_ { 0 }; Index_ < Size_; ++Index_)
    //{
    //  ++Pos_[Radix_key_<Radix_opts_>(Begin_[Index_], Curr_radix_, Proj_func_)];
    //}
    Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      ++Pos_[Radix_key_<Radix_opts_>(Begin_[Index_], Curr_radix_, Proj_func_)];
    });
    // Prefix Sum
    Unroll_loop_<1, Radix_opts_.bucket_size, Radix_opts_.unroll_n>([&](size_t Index_)
    {
      Pos_[Index_] += Pos_[Index_ - 1];
    });

    size_t Index_ { 0 };
    for (; Index_ + 7 < Size_; Index_ += 8)
    {
      size_t i0 = Size_ - 1 - Index_;
      size_t i1 = i0 - 1;
      size_t i2 = i0 - 2;
      size_t i3 = i0 - 3;
      size_t i4 = i0 - 4;
      size_t i5 = i0 - 5;
      size_t i6 = i0 - 6;
      size_t i7 = i0 - 7;

      Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[i0], Curr_radix_, Proj_func_)]] = std::move(Begin_[i0]);
      Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[i1], Curr_radix_, Proj_func_)]] = std::move(Begin_[i1]);
      Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[i2], Curr_radix_, Proj_func_)]] = std::move(Begin_[i2]);
      Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[i3], Curr_radix_, Proj_func_)]] = std::move(Begin_[i3]);
      Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[i4], Curr_radix_, Proj_func_)]] = std::move(Begin_[i4]);
      Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[i5], Curr_radix_, Proj_func_)]] = std::move(Begin_[i5]);
      Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[i6], Curr_radix_, Proj_func_)]] = std::move(Begin_[i6]);
      Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[i7], Curr_radix_, Proj_func_)]] = std::move(Begin_[i7]);
    }

    for (; Index_ < Size_; ++Index_)
    {
      size_t Curr_index_ = Size_ - 1 - Index_;
      Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[Curr_index_], Curr_radix_, Proj_func_)]] =
          std::move(Begin_[Curr_index_]);
    }

    // Collect
    Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      size_t Curr_index_ = Size_ - 1 - Index_;
      Output_[--Pos_[Radix_key_<Radix_opts_>(Begin_[Curr_index_], Curr_radix_, Proj_func_)]] =
          std::move(Begin_[Curr_index_]);
    });
    std::swap(Begin_, Output_);
  }

  if (Radix_ & 1)
  {
    std::move(Begin_, Begin_ + Size_, Output_);
  }
}

template<radix_options Radix_opts_, typename Random_point_, typename Random_buffer_point_, typename Proj_functor_>
inline void
MSD_integer_radix_sort_(Random_point_ &RESTRICT_KEYWORD        Begin_,
                        size_t                                 Size_,
                        Random_buffer_point_ &RESTRICT_KEYWORD Output_,
                        size_t                                 Radix_,
                        Proj_functor_ const                   &Proj_func_,
                        size_t                                 Deep_ = 0)
{
  // If the chunk _Size is too small, then turn to serial least-significant-byte radix sort
  if (Size_ <= Radix_opts_.chunk_size || Radix_ < 1)
  {
    return LSD_integer_radix_sort_<Radix_opts_>(Begin_, Size_, Output_, Radix_, Proj_func_, Deep_);
  }

  size_t Chunk_[Radix_opts_.bucket_size] {};

  Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
  {
    ++Chunk_[Radix_key_<Radix_opts_>(Begin_[Index_], Radix_, Proj_func_)];
  });

  size_t Non_empty_count_ = 0, Prev_sum_ = 0;

  Unroll_loop_<0, Radix_opts_.bucket_size, Radix_opts_.unroll_n>([&](size_t Index_)
  {
    size_t Original_val_ = Chunk_[Index_];
    Non_empty_count_ += (Original_val_ != 0);

    Chunk_[Index_] += Prev_sum_;
    Prev_sum_ = Chunk_[Index_];
  });

  if (Non_empty_count_ > 1)
  {
    Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      size_t Curr_index_ = Size_ - 1 - Index_;
      Output_[--Chunk_[Radix_key_<Radix_opts_>(Begin_[Curr_index_], Radix_, Proj_func_)]] =
          std::move(Begin_[Curr_index_]);
    });

    Unroll_loop_<0, Radix_opts_.bucket_size - 1, Radix_opts_.unroll_n>([&](size_t Index_)
    {
      auto *New_beign_  = Begin_ + Chunk_[Index_];
      auto *New_output_ = Output_ + Chunk_[Index_];

      MSD_integer_radix_sort_<Radix_opts_>(New_output_, Chunk_[Index_ + 1] - Chunk_[Index_], New_beign_, Radix_ - 1,
                                           Proj_func_, Deep_ + 1);
    });
    auto *New_beign_  = Begin_ + Chunk_[Radix_opts_.bucket_size - 1];
    auto *New_output_ = Output_ + Chunk_[Radix_opts_.bucket_size - 1];
    MSD_integer_radix_sort_<Radix_opts_>(New_output_, Size_ - Chunk_[Radix_opts_.bucket_size - 1], New_beign_,
                                         Radix_ - 1, Proj_func_, Deep_ + 1);
  }
  else
  {
    // Only one non-empty bucket:
    // all elements of the current byte have the same value, no need to reorder, directly process the next byte
    MSD_integer_radix_sort_<Radix_opts_>(Begin_, Size_, Output_, Radix_ - 1, Proj_func_, Deep_);
  }
}

template<radix_options               Radix_opts_,
         Supported_execution_policy_ Exec_policy_,
         typename Random_point_,
         typename Random_buffer_point_,
         typename Proj_functor_>
inline void
Radix_sort_impl_(Exec_policy_ && /*Expo_*/,
                 Random_point_ &RESTRICT_KEYWORD        Begin_,
                 size_t                                 Size_,
                 Random_buffer_point_ &RESTRICT_KEYWORD Output_,
                 size_t                                 Radix_,
                 Proj_functor_ const                   &Proj_func_)
{
  using Expo_ty_ = std::remove_cvref_t<Exec_policy_>;
  if constexpr (std::same_as<Expo_ty_, std::execution::sequenced_policy> or
                std::same_as<Expo_ty_, std::execution::unsequenced_policy>)
  {
    MSD_integer_radix_sort_<Radix_opts_>(Begin_, Size_, Output_, Radix_, Proj_func_);
  }
  else /*if constexpr (std::same_as<Expo_ty_, std::execution::parallel_policy> or
                     std::same_as<Expo_ty_, std::execution::parallel_unsequenced_policy>)*/
  {
  }
}

template<radix_options               Radix_opts_ = radix_options {},
         Supported_execution_policy_ Exec_policy_,
         Random_access_iterator_     Random_iter_,
         Standard_allocator_         Allocator_,
         typename Key_function_ = radix_default_key_function<std::iter_value_t<Random_iter_>>>
inline void radix_sort(
    Exec_policy_ &&Expo_, Random_iter_ Begin_, Random_iter_ End_, Allocator_ const &Alloc_, Key_function_ &&F_ = {})
{
  static_assert(Radix_opts_.bucket_size == 256U || Radix_opts_.bucket_size == 65536U,
                "Bucket size must be 256 or 65536!");

  size_t Size_ = End_ - Begin_;

  if (Size_ <= 1) [[unlikely]]
  {
    return;
  }

  // Radix sort supports sorting of floating-point type data.
  //
  // Because floating-point numbers may have NaN (Not a Number),
  // and we need to support choosing the position of NaN (beginning or end),
  // it is necessary to perform <Max_while_partition_> to partition and obtain the "index" of NaN,
  // in order to construct the range of non-NaN data for sorting.

  using Proj_functor_ = Radix_projection_functor_<Key_function_, decltype(F_(*Begin_))>;
  Proj_functor_          Proj_func_ { std::forward<Key_function_>(F_) };
  auto *RESTRICT_KEYWORD Primary_addr_ = &*Begin_;

  size_t Range_left_ = 0, Range_right_ = Size_;
  size_t Max_val_;

  if constexpr (std::integral<typename Proj_functor_::Key_ty_> ||
                (std::floating_point<typename Proj_functor_::Key_ty_> &&
                 Radix_opts_.nan_pos == radix_nan_position::unhandled))
  {
    Max_val_ = Max_element_<false, Radix_opts_.unroll_n>(Primary_addr_, Size_, Proj_func_);
  }
  else if constexpr (Radix_opts_.nan_pos == radix_nan_position::begin)
  {
    auto Max_with_part_ = Max_while_partition_(Primary_addr_, Primary_addr_ + Size_, [&](auto const &Val_)
    {
      return std::isnan(Proj_func_.F_(Val_));
    }, Proj_func_);

    Range_left_ = Max_with_part_.Part_index_;
    Max_val_    = Max_with_part_.Max_val_;
  }
  else /*if constexpr (Radix_opts_.nan_pos == radix_nan_position::end)*/
  {
    auto Max_with_part_ = Max_while_partition_(Primary_addr_, Primary_addr_ + Size_, [&](auto const &Val_)
    {
      return !std::isnan(Proj_func_.F_(Val_));
    }, Proj_func_);

    Range_right_ = Max_with_part_.Part_index_;
    Max_val_     = Max_with_part_.Max_val_;
  }

  // If all elements are NaN or Zero, then there is no need to sort
  if (Max_val_ == 0) [[unlikely]]
  {
    return;
  }

  size_t Highest_byte_idx_ = static_cast<size_t>((std::bit_width(Max_val_) + 7) / 8 - 1);

  Primary_addr_ = Primary_addr_ + Range_left_;

  AllocatedBufferHolder_<Allocator_> Holder_(Range_right_ - Range_left_, Alloc_);
  auto *RESTRICT_KEYWORD             Buffer_ = Holder_.Get_buffer_();

  // If it is a floating-point number, and the "begin" or "end" enum value is selected,
  // > for the "begin" enum value :
  // > [0, Range_left_) is NaN range,
  // > so let the sorting range start from "Range_left_" as the "0 starting" index,
  // > and use the "Size_" (Range_right_) value as the "ending" index.
  //
  // > for the "end" enum value   :
  // > [Range_right_, Size_) is NaN range, sort range is [0, Range_right_).
  //
  // Else Range_left_ = 0, Range_right_ = Size_, sort range is [0, Range_right_).
  //
  // So we always sort the range of non-NaN elements [0, Range_right_)

  Radix_sort_impl_<Radix_opts_>(std::forward<Exec_policy_>(Expo_), Primary_addr_, Range_right_, Buffer_,
                                Highest_byte_idx_, Proj_func_);
}

template<radix_options               Radix_opts_ = radix_options {},
         Supported_execution_policy_ Exec_policy_,
         Random_access_iterator_     Random_iter_,
         typename Key_function_ = radix_default_key_function<std::iter_value_t<Random_iter_>>>
inline void radix_sort(Exec_policy_ &&Expo_, Random_iter_ Begin_, Random_iter_ End_, Key_function_ &&F_ = {})
{
  using Value_type_ = typename std::iterator_traits<Random_iter_>::value_type;

  radix_sort(std::forward<Exec_policy_>(Expo_), Begin_, End_, std::allocator<Value_type_> {},
             std::forward<Key_function_>(F_));
}

} // namespace stdexx

#pragma pop_macro("ALWAYS_INLINE")
#pragma pop_macro("RESTRICT_KEYWORD")

#include <ppl.h>
#include <random>

#include "algorithm/radix_sort.hpp"
#include "profiling/ticktock.hpp"

int
main()
{
#if 1
  //std::vector<int32_t> v { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5 };

  std::random_device rd;
  std::mt19937       gen(rd());
  constexpr auto     N = 65536 * 4 * 4;
  // 2. 定义整数分布范围，这里使用 0 ~ 10000 作为示例
  std::uniform_real_distribution<float> dist(INT_MIN, INT_MAX);

  // 3. 创建 vector 并预留空间（提高性能）
  std::vector<float> v;
  v.reserve(N);

  // 4. 生成 10000 个随机数并填入 vector
  std::generate_n(std::back_inserter(v), N, [&]()
  {
    return dist(gen);
  });

  //std::vector<int32_t> v { 12, 1, 123, 1234, 123456, 12345, 12345678, 123456789, 1234567891, 1234567, 0 };

#else
  std::vector<float> v { INFINITY, -INFINITY, NAN, -0.0f, 1.0f / 1.0f, 0.0f, -1.0f / 1.0f, std::sqrt(-1.0f), 3.14f };
#endif // 1
  auto vv = v;

  TICK(stdexx);
  stdexx::radix_sort(std::execution::seq, v.begin(), v.end());
  std::println("{}", std::is_sorted(v.begin(), v.end()));
  TOCK(stdexx);

  /*TICK(stdex);
  stdex::radix_sort(std::execution::seq, vv.begin(), vv.end());
  std::println("{}", std::is_sorted(vv.begin(), vv.end()));
  TOCK(stdex);*/

  /*concurrency::parallel_radixsort(vv.begin(), vv.end(), concurrency::_Radix_sort_default_function<int> {}, 10);
  std::sort(v.begin(), v.end());*/
  return 0;
}
