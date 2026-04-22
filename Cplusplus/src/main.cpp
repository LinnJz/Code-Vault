/*
c++ ver >= 20
clang ver >= 13.0.0
gcc ver >= 11.1.0
msvc ver >= 19.28 (VS 16.8)
*/
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
#include <span>
#include <type_traits>

#include <print>

#pragma push_macro("ALWAYS_INLINE")
#pragma push_macro("RESTRICT_KEYWORD")
#undef ALWAYS_INLINE
#undef RESTRICT_KEYWORD

#if 1
#  if defined(__GNUC__)
#    define ALWAYS_INLINE inline __attribute__((always_inline))
#  elif defined(_MSC_VER)
#    define ALWAYS_INLINE inline __forceinline
#  else
#    define ALWAYS_INLINE inline
#  endif
#else
#  define ALWAYS_INLINE
#endif

#if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
#  define RESTRICT_KEYWORD __restrict
#else // assume unsupported compiler
#  define RESTRICT_KEYWORD
#endif

namespace stdex
{
enum class radix_sort_order
{
  asc,
  desc
};

enum class radix_nan_position
{
  //< Temp Data: {INFINITY, -INFINITY, NAN, -0.0f, 1.0f / 1.0f, 0.0f, -1.0f / 1.0f,
  //         std::sqrt(-1.0f), 3.14f}

  unhandled, //< maybe output: -nan(ind) -inf -1 -0 0 1 3.14 inf nan
  begin,     //< maybe output: -nan(ind) nan -inf -1 -0 0 1 3.14 inf
  end        //< maybe output: -inf -1 -0 0 1 3.14 inf nan -nan(ind)
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
  radix_sort_order   sort_order  = radix_sort_order::asc;
  radix_nan_position nan_pos     = radix_nan_position::unhandled;
};

inline consteval radix_options
Adjust_radix_options_(radix_options Opts_, bool Is_reverse_) noexcept
{
  if (Is_reverse_)
  {
    Opts_.sort_order = Opts_.sort_order == radix_sort_order::asc ? radix_sort_order::desc : radix_sort_order::asc;
  }
  return Opts_;
}

template<typename Exec_policy_>
concept Supported_execution_policy_ =
    std::same_as<std::decay_t<Exec_policy_>, std::execution::sequenced_policy> ||
    std::same_as<std::decay_t<Exec_policy_>, std::execution::parallel_policy> ||
    std::same_as<std::decay_t<Exec_policy_>, std::execution::unsequenced_policy> ||
    std::same_as<std::decay_t<Exec_policy_>, std::execution::parallel_unsequenced_policy>;

template<typename Exec_policy_>
inline constexpr bool Is_parallel_policy_v_ =
    std::same_as<std::decay_t<Exec_policy_>, std::execution::parallel_policy> ||
    std::same_as<std::decay_t<Exec_policy_>, std::execution::parallel_unsequenced_policy>;

template<typename Alloc_>
concept Standard_allocator_ = requires (Alloc_ A_, typename Alloc_::value_type *P_, std::size_t N_) {
  typename Alloc_::value_type;

  std::is_default_constructible_v<Alloc_>;
  std::is_copy_constructible_v<Alloc_>;
  std::is_move_constructible_v<Alloc_>;

  { A_.allocate(N_) } -> std::same_as<typename Alloc_::value_type *>;

  { A_.deallocate(P_, N_) } -> std::same_as<void>;

  { A_ == A_ } -> std::convertible_to<bool>;
  { A_ != A_ } -> std::convertible_to<bool>;
};

template<typename Func_, typename Ty_>
concept Invocable_key_function_ = std::invocable<Func_, Ty_ const &> && !std::invocable<Func_>;

template<typename Iter_>
concept Random_access_iterator_ = std::random_access_iterator<Iter_>;

template<typename>
struct Is_reverse_iterator_ : std::false_type
{
};

template<typename Iter_>
struct Is_reverse_iterator_<std::reverse_iterator<Iter_>> : std::true_type
{
};

template<typename Iter_>
inline constexpr bool Is_reverse_iterator_v_ = Is_reverse_iterator_<Iter_>::value;

template<typename Iter_>
concept Reverse_iterator_ = Is_reverse_iterator_v_<Iter_>;

struct Max_partition_result_
{
  size_t Part_index_;
  size_t Max_val_;
};

//< Only applies to floating-point numbers,
//< while dividing according to NaN, find the maximum based on the projection.
//
//< Proj_func_.Get_key_value_() return floating-point
//< Proj_func_()                return projection, type is size_t
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

      if (!std::isnan(Proj_func_.Get_key_value_(*First_)))
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
        if (!std::isnan(Proj_func_.Get_key_value_(*First_)))
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

    if (!std::isnan(Proj_func_.Get_key_value_(*First_)))
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
  size_t Max_val_ = 0;

  if constexpr (Is_parallel_)
  {
    int Actual_threads_ = omp_get_num_procs();
#if defined(_MSC_VER) && !defined(_OPENMP_LLVM_RUNTIME)
#  pragma omp parallel num_threads(Actual_threads_)
    {
      auto Local_val_ = Max_val_;

      // openmp 2.0 does not support "reduction(max : ?)" and loop index must be signed
#  pragma omp for nowait
      for (int64_t Index_ = 0; Index_ < Size_; ++Index_)
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
    for (size_t Index_ = 0; Index_ < Size_; ++Index_)
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
    for (size_t Index_ = 0; Index_ < Size_; ++Index_)
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

//< Allocate and construct a buffer
template<typename Allocator_>
ALWAYS_INLINE typename ::std::allocator_traits<Allocator_>::pointer
Construct_buffer_(size_t N_, Allocator_ &Alloc_)
{
  using Traits_     = ::std::allocator_traits<Allocator_>;
  using Value_type_ = typename Allocator_::value_type;
  using Pointer_    = typename Traits_::pointer;

  Pointer_ const P_ = Alloc_.allocate(N_);

  //< If the objects being sorted have trivial default initialization, they do not need to be
  //< initialized here. This can benefit performance.
  if (!::std::is_trivially_default_constructible_v<Value_type_>)
  {
    for (size_t I_ = 0; I_ < N_; ++I_)
    {
      //< Objects being sorted must be default-initializable
      Traits_::construct(Alloc_, P_ + I_);
    }
  }

  return P_;
}

//< Destroy and deallocate a buffer
template<typename Allocator_>
ALWAYS_INLINE void
Destroy_buffer_(typename ::std::allocator_traits<Allocator_>::pointer P_, size_t N_, Allocator_ &Alloc_)
{
  using Traits_ = ::std::allocator_traits<Allocator_>;

  //< If the objects being sorted have trivial destruction, they do not need to be
  //< destroyed here. This can benefit performance.
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
  AllocatedBufferHolder_()
      : M_size_(0)
      , M_alloc_()
      , M_buffer_(nullptr)
  {
  }

  AllocatedBufferHolder_(size_t Size_, Allocator_ const &Alloc_)
      : M_size_(Size_)
      , M_alloc_(Alloc_)
      , M_buffer_(Construct_buffer_(Size_, M_alloc_))
  {
  }

  ~AllocatedBufferHolder_() { Destroy_buffer_(M_buffer_, M_size_, M_alloc_); }

  void Allocate_(size_t Size_, Allocator_ const &Alloc_)
  {
    if (M_buffer_ != nullptr && M_size_ != 0)
    {
      Destroy_buffer_(M_buffer_, M_size_, M_alloc_);
      M_buffer_ = nullptr;
      M_size_   = 0;
    }

    if (Size_ == 0)
    {
      M_alloc_ = Alloc_;
      return;
    }

    M_size_   = Size_;
    M_alloc_  = Alloc_;
    M_buffer_ = Construct_buffer_(M_size_, M_alloc_);
  }

  typename std::allocator_traits<Allocator_>::pointer Get_buffer_() { return M_buffer_; }

private:
  size_t                                              M_size_;
  Allocator_                                          M_alloc_;
  typename std::allocator_traits<Allocator_>::pointer M_buffer_;
};

enum class Unroll_loop_direction_
{
  Increasing_,
  Decreasing_
};

template<size_t Unroll_size_, typename Function_>
ALWAYS_INLINE void
Unrolled_call_(size_t Base_, Function_ const &Func_) noexcept
{
  [&]<size_t... Is_>(std::index_sequence<Is_...>)
  {
    (Func_(Base_ + Is_), ...);
  }.template operator() (std::make_index_sequence<Unroll_size_> {});
}

template<size_t Unroll_size_, typename Function_>
ALWAYS_INLINE void
Unroll_loop_(size_t Start_, size_t End_, Function_ const &Func_) noexcept
{
  static_assert((Unroll_size_ & (Unroll_size_ - 1)) == 0, "Unroll size must be a power of two");

  size_t Index_ { Start_ };

  for (; Unroll_size_ <= End_ - Index_; Index_ += Unroll_size_)
  {
    // Unrolled_call_<Unroll_size_>(Index_, Func_);
    for (size_t Roll_ = 0; Roll_ < Unroll_size_; ++Roll_)
    {
      Func_(Index_ + Roll_);
    }
  }

  for (; Index_ < End_; ++Index_)
  {
    Func_(Index_);
  }
}

struct Radix_projection_functor_tag_
{
};

template<typename Ty_>
concept Radix_projection_tag_ = std::is_void_v<Ty_> || std::derived_from<Ty_, Radix_projection_functor_tag_>;

struct Radix_projection_functor_default_tag_ : Radix_projection_functor_tag_
{
};

struct Radix_projection_functor_signed_integral_xor_tag_ : Radix_projection_functor_tag_
{
};

struct Radix_projection_functor_float_point_bitwise_not_tag_ : Radix_projection_functor_tag_
{
};

inline constexpr Radix_projection_functor_default_tag_                 Radix_default_tag_ {};
inline constexpr Radix_projection_functor_signed_integral_xor_tag_     Radix_int_xor_tag_ {};
inline constexpr Radix_projection_functor_float_point_bitwise_not_tag_ Radix_float_bitwise_not_tag_ {};

template<typename Key_function_, typename Ty_>
struct Radix_projection_functor_
{
  static_assert(
      Invocable_key_function_<Key_function_, Ty_>,
      "Key_function_ must be callable with exactly one required argument of type Ty_, const Ty_, or const Ty_&. "
      "Additional arguments must have default values.");

  using Key_ty_      = std::remove_cvref_t<std::invoke_result_t<Key_function_, Ty_>>;
  using Unsigned_ty_ = std::make_unsigned_t<
      std::conditional_t<std::is_floating_point_v<Key_ty_>,
                         std::conditional_t<std::same_as<Key_ty_, float>, std::uint32_t, std::uint64_t>,
                         Key_ty_>>;
  static constexpr std::uint8_t Shift_of_sign_bit_ = sizeof(Key_ty_) * 8 - 1;
  static constexpr Unsigned_ty_ All_bit_mask_      = ~Unsigned_ty_ { 0 };
  static constexpr Unsigned_ty_ Sign_bit_mask_     = All_bit_mask_ << Shift_of_sign_bit_;

  static_assert(std::is_arithmetic_v<Key_ty_>, "Type must be arithmetic (integral or floating-point)!");
  //< Does not support types like __int128, which are larger than size_t
  static_assert((sizeof(Key_ty_) <= sizeof(size_t)), "Type size is bigger than size_t size.");

  Key_function_ F_;

  constexpr Key_ty_ Get_key_value_(Ty_ const &Val_) const noexcept { return F_(Val_); }

  constexpr size_t operator() (Ty_ const &Val_) const noexcept
  {
    return this->operator() (Radix_default_tag_, Get_key_value_(Val_));
  }

  constexpr size_t operator() (Radix_projection_functor_default_tag_, Key_ty_ Val_) const noexcept
  {
    if constexpr (std::unsigned_integral<Key_ty_>)
    {
      return Val_;
    }
    else if constexpr (std::signed_integral<Key_ty_>)
    {
      //< The default function needs to take the signed integer-like representation and map it to an unsigned one. The
      //< following code will take the midpoint of the unsigned representable range (SIZE_MAX/2)+1 and does an unsigned
      //< add of the value. Thus, it maps a [-signed_min,+signed_max] range into a [0, unsigned_max] range.
      return ((std::numeric_limits<size_t>::max() / 2) + 1) + static_cast<size_t>(Val_);

      //< Another strategy "return static_cast<Unsigned_ty_>(Val_) ^ Sign_bit_mask_;"
      //<
      //< if only LSD sorting is used, only the highest significant bit needs to be processed.
      //< Currently, a mixed strategy of MSD and LSD is adopted, using the above scheme applied to the function-Radix_key.
    }
    else /* std::floating_point<Key_ty_> */
    {
      Unsigned_ty_ const Unsigned_val_ = std::bit_cast<Unsigned_ty_>(Val_);

      //< NaN cannot be compared, so we do not use >=, but instead use shifting
      //< return ((Unsigned_val_ >> Shift_of_sign_bit_) == 0) ? Unsigned_val_ | Sign_bit_mask_ : ~Unsigned_val_;

      Unsigned_ty_ const Mask_ = (Unsigned_ty_(0) - (Unsigned_val_ >> Shift_of_sign_bit_));

      return (Unsigned_val_ ^ Mask_) | (Sign_bit_mask_ & ~Mask_);
    }
  }

  constexpr size_t operator() (Radix_projection_functor_signed_integral_xor_tag_, Key_ty_ Val_) const noexcept
  {
    if constexpr (std::signed_integral<Key_ty_>)
    {
      return static_cast<Unsigned_ty_>(Val_) ^ Sign_bit_mask_;
    }
    else
    {
      return this->operator() (Radix_default_tag_, Val_);
    }
  }

  constexpr size_t operator() (Radix_projection_functor_float_point_bitwise_not_tag_, Key_ty_ Val_) const noexcept
  {
    if constexpr (std::floating_point<Key_ty_>)
    {
      Unsigned_ty_ const Unsigned_val_ = std::bit_cast<Unsigned_ty_>(Val_);

      return Unsigned_val_ ^ (Unsigned_ty_(0) - (Unsigned_val_ >> Shift_of_sign_bit_));
    }
    else
    {
      return this->operator() (Radix_default_tag_, Val_);
    }
  }
};

template<radix_options Radix_opts_, typename Unsigned_ty_>
ALWAYS_INLINE size_t
Radix_key_(Unsigned_ty_ Val_, size_t Radix_)
{
  //< Mask (value 255 or 65535) and Shift (value 8 or 16) depends on the size of the bucket
  constexpr uint16_t Mask_  = Radix_opts_.bucket_size - 1;
  constexpr uint16_t Shift_ = Radix_opts_.bucket_size == 256U ? 8 : 16;

  if constexpr (Radix_opts_.sort_order == radix_sort_order::asc)
  {
    return (Val_ >> Shift_ * Radix_) & Mask_;
  }
  else /* Radix_opts_.sort_order == radix_sort_order::desc */
  {
    return Mask_ - ((Val_ >> Shift_ * Radix_) & Mask_);
  }
}

template<Radix_projection_tag_ Tag_, radix_options Radix_opts_, typename Ty_, typename Proj_functor_>
ALWAYS_INLINE size_t
Radix_key_(Ty_ const &Val_, size_t Radix_, Proj_functor_ const &Proj_func_)
{
  return Radix_key_<Radix_opts_>(Proj_func_(Tag_ {}, Proj_func_.Get_key_value_(Val_)), Radix_);
}

template<radix_options Radix_opts_, typename Ty_, typename Proj_functor_>
ALWAYS_INLINE size_t
Radix_key_(Ty_ const &Val_, size_t Radix_, Proj_functor_ const &Proj_func_)
{
  return Radix_key_<Radix_projection_functor_default_tag_, Radix_opts_, Ty_, Proj_functor_>(Val_, Radix_, Proj_func_);
}

struct Radix_parallel_data_
{
  size_t Step_;
  int    Remain_;
  int    Threads_num_ { 1 };
};

ALWAYS_INLINE std::pair<size_t, size_t>
              Radix_compute_task_indices_(size_t Thread_id_, size_t Step_, size_t Remain_)
{
  size_t Beg_index_, End_index_;

  if (Thread_id_ < Remain_)
  {
    Beg_index_ = Thread_id_ * (Step_ + 1);
    End_index_ = Beg_index_ + (Step_ + 1);
  }
  else
  {
    Beg_index_ = Remain_ * (Step_ + 1) + (Thread_id_ - Remain_) * Step_;
    End_index_ = Beg_index_ + Step_;
  }

  return { Beg_index_, End_index_ };
}

template<bool Is_last_pass_, radix_options Radix_opts_, typename Ty_, typename Proj_functor_>
ALWAYS_INLINE void
Radix_lsd_histogram_(Ty_ const &Val_,
                     size_t (*const Chunks_)[Radix_opts_.bucket_size],
                     size_t                             Thread_id_,
                     size_t                             Radix_,
                     Proj_functor_ const               &Proj_func_,
                     [[maybe_unused]] std::span<size_t> NaN_counts_ = {})
{
  if constexpr (std::floating_point<typename Proj_functor_::Key_ty_>)
  {
    if constexpr (Is_last_pass_ and Radix_opts_.nan_pos != radix_nan_position::unhandled)
    {
      if (std::isnan(Proj_func_.Get_key_value_(Val_))) [[unlikely]]
      {
        ++NaN_counts_[Thread_id_];
        return;
      }
    }
    if constexpr (Is_last_pass_)
    {
      ++Chunks_[Thread_id_][Radix_key_<Radix_opts_>(Val_, Radix_, Proj_func_)];
    }
    else
    {
      ++Chunks_[Thread_id_][Radix_key_<Radix_opts_>(
          Proj_func_(Radix_float_bitwise_not_tag_, Proj_func_.Get_key_value_(Val_)), Radix_)];
    }
  }

  if constexpr (std::integral<typename Proj_functor_::Key_ty_>)
  {
    if constexpr (std::signed_integral<typename Proj_functor_::Key_ty_> and Is_last_pass_)
    {
      ++Chunks_[Thread_id_]
               [Radix_key_<Radix_opts_>(Proj_func_(Radix_int_xor_tag_, Proj_func_.Get_key_value_(Val_)), Radix_)];
    }
    else
    {
      ++Chunks_[Thread_id_][Radix_key_<Radix_opts_>(
          static_cast<typename Proj_functor_::Unsigned_ty_>(Proj_func_.Get_key_value_(Val_)), Radix_)];
    }
  }
}

template<bool          Is_last_pass_,
         radix_options Radix_opts_,
         typename Random_point_,
         typename Random_buffer_point_,
         typename Proj_functor_>
ALWAYS_INLINE void
Radix_lsd_collect_(Random_point_ &RESTRICT_KEYWORD        Begin_,
                   size_t                                 Size_,
                   size_t                                 Index_,
                   Random_buffer_point_ &RESTRICT_KEYWORD Output_,
                   size_t (*const Chunks_)[Radix_opts_.bucket_size],
                   size_t                   Thread_id_,
                   size_t                   Radix_,
                   Proj_functor_ const     &Proj_func_,
                   [[maybe_unused]] size_t &NaN_offset_ = 0)
{
  if constexpr (std::floating_point<typename Proj_functor_::Key_ty_>)
  {
    if constexpr (Is_last_pass_ and Radix_opts_.nan_pos != radix_nan_position::unhandled)
    {
      if (std::isnan(Proj_func_.Get_key_value_(Begin_[Index_]))) [[unlikely]]
      {
        if constexpr (Radix_opts_.nan_pos == radix_nan_position::begin)
        {
          Output_[--NaN_offset_] = std::move(Begin_[Index_]);
        }
        else if constexpr (Radix_opts_.nan_pos == radix_nan_position::end)
        {
          Output_[Size_ - NaN_offset_--] = std::move(Begin_[Index_]);
        }
        return;
      }
    }
    if constexpr (Is_last_pass_)
    {
      Output_[--Chunks_[Thread_id_][Radix_key_<Radix_opts_>(Begin_[Index_], Radix_, Proj_func_)]] =
          std::move(Begin_[Index_]);
    }
    else
    {
      Output_[--Chunks_[Thread_id_][Radix_key_<Radix_opts_>(
          Proj_func_(Radix_float_bitwise_not_tag_, Proj_func_.Get_key_value_(Begin_[Index_])), Radix_)]] =
          std::move(Begin_[Index_]);
    }
  }
  if constexpr (std::integral<typename Proj_functor_::Key_ty_>)
  {
    if constexpr (std::signed_integral<typename Proj_functor_::Key_ty_> and Is_last_pass_)
    {
      Output_[--Chunks_[Thread_id_][Radix_key_<Radix_opts_>(
          Proj_func_(Radix_int_xor_tag_, Proj_func_.Get_key_value_(Begin_[Index_])), Radix_)]] =
          std::move(Begin_[Index_]);
    }
    else
    {
      Output_[--Chunks_[Thread_id_][Radix_key_<Radix_opts_>(
          static_cast<typename Proj_functor_::Unsigned_ty_>(Proj_func_.Get_key_value_(Begin_[Index_])), Radix_)]] =
          std::move(Begin_[Index_]);
    }
  }
}

template<bool          Is_parallel_,
         bool          Is_lass_pass_,
         radix_options Radix_opts_,
         typename Random_point_,
         typename Random_buffer_point_,
         typename Proj_functor_>
ALWAYS_INLINE void
LSD_integer_radix_pass_(Radix_parallel_data_                   Paral_data_,
                        Random_point_ &RESTRICT_KEYWORD        Begin_,
                        size_t                                 Size_,
                        Random_buffer_point_ &RESTRICT_KEYWORD Output_,
                        size_t (*const Chunks_)[Radix_opts_.bucket_size],
                        size_t                             Radix_,
                        Proj_functor_ const               &Proj_func_,
                        [[maybe_unused]] std::span<size_t> NaN_counts_ = {})
{
  if constexpr (Is_parallel_)
  {
#pragma omp parallel num_threads(Paral_data_.Threads_num_)
    {
      int const Thread_id_ = omp_get_thread_num();

      auto [Beg_index_, End_index_] = Radix_compute_task_indices_(Thread_id_, Paral_data_.Step_, Paral_data_.Remain_);

      Unroll_loop_<Radix_opts_.unroll_n>(Beg_index_, End_index_, [&](size_t Index_)
      {
        Radix_lsd_histogram_<Is_lass_pass_, Radix_opts_>(Begin_[Index_], Chunks_, Thread_id_, Radix_, Proj_func_,
                                                         NaN_counts_);
      });
    }
  }
  else
  {
    Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      Radix_lsd_histogram_<Is_lass_pass_, Radix_opts_>(Begin_[Index_], Chunks_, 0, Radix_, Proj_func_, NaN_counts_);
    });
  }

#pragma region PREFIX_SUM

  size_t NaN_offset_ = 0;
  if constexpr (std::floating_point<typename Proj_functor_::Key_ty_> and
                Radix_opts_.nan_pos != radix_nan_position::unhandled and Is_lass_pass_)
  {
    NaN_offset_ = std::reduce(NaN_counts_.begin(), NaN_counts_.end());

    if constexpr (Radix_opts_.nan_pos == radix_nan_position::begin)
    {
      for (size_t Thread_id_ = 0; Thread_id_ < Paral_data_.Threads_num_; ++Thread_id_)
      {
        Chunks_[Thread_id_][0] += NaN_counts_[Thread_id_];
      }
    }
  }
  size_t Non_empty_count_ = 0;

  Unroll_loop_<Radix_opts_.unroll_n>(0, Radix_opts_.bucket_size, [&](size_t Index_)
  {
    size_t Last_ = Index_ ? Chunks_[Paral_data_.Threads_num_ - 1][Index_ - 1] : 0;
    Chunks_[0][Index_] += Last_;

    for (size_t Thread_id_ = 1; Thread_id_ < Paral_data_.Threads_num_; ++Thread_id_)
    {
      Chunks_[Thread_id_][Index_] += Chunks_[Thread_id_ - 1][Index_];
    }

    Non_empty_count_ += (Chunks_[Paral_data_.Threads_num_ - 1][Index_] - Last_);
  });

  if (Non_empty_count_ <= 1)
  {
    return;
  }

#pragma endregion PREFIX_SUM

  if constexpr (Is_parallel_)
  {
#pragma omp parallel num_threads(Paral_data_.Threads_num_)
    {
      int const Thread_id_ = omp_get_thread_num();

      auto [Beg_index_, End_index_] = Radix_compute_task_indices_(Thread_id_, Paral_data_.Step_, Paral_data_.Remain_);

      Unroll_loop_<Radix_opts_.unroll_n>(Beg_index_, End_index_, [&](size_t Index_)
      {
        Radix_lsd_collect_<Is_lass_pass_, Radix_opts_>(Begin_, Size_, Beg_index_ + (End_index_ - 1 - Index_), Output_,
                                                       Chunks_, Thread_id_, Radix_, Proj_func_, NaN_offset_);
      });
    }
  }
  else
  {
    Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      Radix_lsd_collect_<Is_lass_pass_, Radix_opts_>(Begin_, Size_, Size_ - 1 - Index_, Output_, Chunks_, 0, Radix_,
                                                     Proj_func_, NaN_offset_);
    });
  }

  std::swap(Begin_, Output_);
}

template<bool          Is_parallel_,
         radix_options Radix_opts_,
         typename Random_point_,
         typename Random_buffer_point_,
         typename Proj_functor_>
inline void
LSD_integer_radix_sort_(Random_point_ &RESTRICT_KEYWORD        Begin_,
                        size_t                                 Size_,
                        Random_buffer_point_ &RESTRICT_KEYWORD Output_,
                        size_t                                 Radix_,
                        Proj_functor_ const                   &Proj_func_)
{
  if (Size_ == 0) [[unlikely]]
  {
    return;
  }

  using Chunk_ptr_ty_ = size_t (*)[Radix_opts_.bucket_size];

  Chunk_ptr_ty_ Chunks_ = nullptr;

  int Threads_num_ = 1, Buffer_size_ = Radix_opts_.bucket_size;
  if constexpr (Is_parallel_)
  {
    Threads_num_ = omp_get_num_procs();
    Buffer_size_ *= Threads_num_;
  }
  Radix_parallel_data_ Paral_data_ { .Step_        = Size_ / Threads_num_,
                                     .Remain_      = static_cast<int>(Size_ % Threads_num_),
                                     .Threads_num_ = Threads_num_ };

  AllocatedBufferHolder_<std::allocator<size_t>> Heap_bucket_holder_;
  if (Buffer_size_ <= 1024)
  {
    size_t *Stack_bucket_buffer_ = static_cast<size_t *>(alloca(Buffer_size_ * sizeof(size_t)));
    Chunks_                      = reinterpret_cast<Chunk_ptr_ty_>(Stack_bucket_buffer_);
  }
  else
  {
    Heap_bucket_holder_.Allocate_(Buffer_size_, std::allocator<size_t> {});
    Chunks_ = reinterpret_cast<Chunk_ptr_ty_>(Heap_bucket_holder_.Get_buffer_());
  }
  /*
  //< Undefined behavior caused by conflicts between inline optimization and stack variable lifetime/alias analysis
  //
  //< Under (__forceinline), if the following stack array is used, 
  //< it is undefined behavior in /O2 mode. Even though Chunks are initialized to 0 inside the for loop,
  //< the Chunks actually used for histogram statistics are still garbage values, 
  //< ultimately leading to out-of-bounds access during collection.
  //< 
  //< If the code is moved outside of the non-branch, there is no undefined behavior because the compiler can see its lifetime.
  //< But this will waste stack memory of (Radix_opts_.bucket_size)
  //< So we use (alloca)

  else // if constexpr (!Is_parallel_)
  {
    size_t Stack_bucket_buffer_[Radix_opts_.bucket_size];
    Chunks_ = &Stack_bucket_buffer_;
  }
  */

  for (size_t Curr_radix_ = 0; Curr_radix_ < Radix_ - 1; ++Curr_radix_)
  {
    ::memset(Chunks_, 0, Buffer_size_ * sizeof(size_t));

    LSD_integer_radix_pass_<Is_parallel_, false, Radix_opts_>(Paral_data_, Begin_, Size_, Output_, Chunks_, Curr_radix_,
                                                              Proj_func_);
  }

  {
    ::memset(Chunks_, 0, Buffer_size_ * sizeof(size_t));

    std::span<size_t> NaN_counts_span_;

    if constexpr (std::floating_point<typename Proj_functor_::Key_ty_> and
                  Radix_opts_.nan_pos != radix_nan_position::unhandled)
    {
      size_t *NaN_counts_ = static_cast<size_t *>(alloca(Threads_num_ * sizeof(size_t)));
      ::memset(NaN_counts_, 0, Threads_num_ * sizeof(size_t));
      NaN_counts_span_ = std::span<size_t>(NaN_counts_, Threads_num_);
    }

    LSD_integer_radix_pass_<Is_parallel_, true, Radix_opts_>(Paral_data_, Begin_, Size_, Output_, Chunks_, Radix_ - 1,
                                                             Proj_func_, NaN_counts_span_);
  }

  if (Radix_ & 1)
  {
    std::move(Begin_, Begin_ + Size_, Output_);
  }
}

template<radix_options Radix_opts_, typename Random_point_, typename Random_buffer_point_, typename Proj_functor_>
inline void
MSD_recursion_exit_integer_radix_sort_(Random_point_ &RESTRICT_KEYWORD        Begin_,
                                       size_t                                 Size_,
                                       Random_buffer_point_ &RESTRICT_KEYWORD Output_,
                                       size_t                                 Radix_,
                                       Proj_functor_ const                   &Proj_func_,
                                       [[maybe_unused]] size_t                Deep_ = 0)
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

  for (size_t Curr_radix_ = 0; Curr_radix_ < Radix_; ++Curr_radix_)
  {
    size_t Chunks_[Radix_opts_.bucket_size] {};

    // Histogram
    Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      ++Chunks_[Radix_key_<Radix_opts_>(Begin_[Index_], Curr_radix_, Proj_func_)];
    });

    // Prefix Sum
    Unroll_loop_<Radix_opts_.unroll_n>(1, Radix_opts_.bucket_size, [&](size_t Index_)
    {
      Chunks_[Index_] += Chunks_[Index_ - 1];
    });

    //< msvc ver [19.28, 19.44] integer data type, unroll for is wrong, float-point data type is right
    //< msvc ver >= 19.50 (VS 18.0) all arithmetic right

    // Using template unrolling with strong inlining(inline __forceinline) can cause sorting of 'integer data'
    // to generate assembly errors, leading to sorting errors, which was fixed in MSVC ver >= 19.50 (VS 18.0)
    // 

    // Collect
#if defined(_MSC_VER) && _MSC_VER <= 1950
    size_t Index_ = 0;

    for (; Radix_opts_.unroll_n <= Size_ - Index_; Index_ += Radix_opts_.unroll_n)
    {
      for (size_t Roll_ = 0; Roll_ < Radix_opts_.unroll_n; ++Roll_)
      {
        Output_[--Chunks_[Radix_key_<Radix_opts_>(Begin_[Size_ - 1 - (Index_ + Roll_)], Curr_radix_, Proj_func_)]] =
            std::move(Begin_[Size_ - 1 - (Index_ + Roll_)]);
      }
    }

    for (; Index_ < Size_; ++Index_)
    {
      Output_[--Chunks_[Radix_key_<Radix_opts_>(Begin_[Size_ - 1 - Index_], Curr_radix_, Proj_func_)]] =
          std::move(Begin_[Size_ - 1 - Index_]);
    }
#else
    Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      Output_[--Chunks_[Radix_key_<Radix_opts_>(Begin_[Size_ - 1 - Index_], Curr_radix_, Proj_func_)]] =
          std::move(Begin_[Size_ - 1 - Index_]);
    });
#endif // defined(_MSC_VER) && _MSC_VER <= 1950

    std::swap(Begin_, Output_);
  }

  if (Radix_ & 1)
  {
    std::move(Begin_, Begin_ + Size_, Output_);
  }
}

/*
* msvc 参考
template<typename _Random_iterator, typename _Random_buffer_iterator, typename _Function>
void _Parallel_integer_radix_sort(const _Random_iterator &_Begin, size_t _Size, const _Random_buffer_iterator &_Output,
    size_t _Radix, _Function _Proj_func, const size_t _Chunk_size, size_t _Deep = 0)
{
    // If the chunk _Size is too small, then turn to serial least-significant-byte radix sort
    if (_Size <= _Chunk_size || _Radix < 1)
    {
        return _Integer_radix_sort(_Begin, _Size, _Output, _Radix, _Proj_func, _Deep);
    }

    size_t _Threads_num = ::Concurrency::details::_CurrentScheduler::_GetNumberOfVirtualProcessors();
    size_t _Buffer_size = sizeof(size_t) * 256 * _Threads_num;
    size_t _Step = _Size / _Threads_num;
    size_t _Remain = _Size % _Threads_num;

    ::Concurrency::details::_MallocaArrayHolder<size_t> _Holder;
    using _Chunk_ptr_t = size_t (*)[256];
    _Chunk_ptr_t _Chunks = reinterpret_cast<_Chunk_ptr_t>(_Holder._InitOnRawMalloca(_malloca(_Buffer_size)));

    memset(_Chunks, 0, _Buffer_size);

    // Our purpose is to map unsorted data in buffer "_Begin" to buffer "_Output" so that all elements who have the same
    // byte value in the "_Radix" position will be grouped together in the buffer "_Output"
    //
    // Serial version:
    // To understand this algorithm, first consider a serial version. In following example, we treat 1 digit as 1 byte, so we have a
    // total of 10 elements for each digit instead of 256 elements in each byte. Let's suppose "_Radix" == 1 (right most is 0), and:
    //
    //      begin:  [ 32 | 62 | 21 | 43 | 55 | 43 | 23 | 44 ]
    //
    // We want to divide the output buffer "_Output" into 10 chunks, and each the element in the "_Begin" buffer should be mapped into
    // the proper destination chunk based on its current digit (byte) indicated by "_Radix"
    //
    // Because "_Radix" == 1, after a pass of this function, the chunks in the "_Output" should look like:
    //
    //      buffer: [   |   | 21 23 | 32 | 43 43 44 | 55 | 62 |   |   |   ]
    //                0   1     2      3      4        5    6   7   8   9
    //
    // The difficulty is determining where to insert values into the "_Output" to get the above result. The way to get the
    // start position of each chunk of the buffer is:
    //      1. Count the number of elements for each chunk (in above example, chunk0 is 0, chunk1 is 0, chunk2 is 2, chunk3 is 1 ...
    //      2. Make a partial sum for these chunks( in above example,  we will get chunk0=chunk0=0, chunk1=chunk0+chunk1=0,
    //         chunk2=chunk0+chunk1+chunk2=2, chunk3=chunk0+chunk1+chunk2+chunk3=3
    //
    // After these steps, we will get the end position of each chunk in the "_Output". The begin position of each chunk will be the end
    // point of last chunk (begin point is close but the end point is open). After that,  we can scan the original array again and directly
    // put elements from original buffer "_Begin" into specified chunk on buffer "_Output".
    // Finally, we invoke _parallel_integer_radix_sort in parallel for each chunk and sort them in parallel based on the next digit (byte).
    // Because this is a STABLE sort algorithm, if two numbers has same key value on this byte (digit), their original order should be kept.
    //
    // Parallel version:
    // Almost the same as the serial version, the differences are:
    //      1. The count for each chunk is executed in parallel, and each thread will count one segment of the input buffer "_Begin".
    //         The count result will be separately stored in their own chunk size counting arrays so we have a total of threads-number
    //         of chunk count arrays.
    //         For example, we may have chunk00, chunk01, ..., chunk09 for first thread, chunk10, chunk11, ..., chunk19 for second thread, ...
    //      2. The partial sum should be executed across these chunk counting arrays that belong to different threads, instead of just
    //         making a partial sum in one counting array.
    //         This is because we need to put values from different segments into one final buffer, and the absolute buffer position for
    //         each chunkXX is needed.
    //      3. Make a parallel scan for original buffer again, and move numbers in parallel into the corresponding chunk on each buffer based
    //         on these threads' chunk size counters.

    // Count in parallel and separately save their local results without reducing
    ::Concurrency::parallel_for(static_cast<size_t>(0), _Threads_num, [=](size_t _Index)
    {
        size_t _Beg_index, _End_index;

        // Calculate the segment position
        if (_Index < _Remain)
        {
            _Beg_index = _Index * (_Step + 1);
            _End_index = _Beg_index + (_Step + 1);
        }
        else
        {
            _Beg_index = _Remain * (_Step + 1) + (_Index - _Remain) * _Step;
            _End_index = _Beg_index + _Step;
        }

        // Do a counting
        while (_Beg_index != _End_index)
        {
            ++_Chunks[_Index][_Radix_key(_Begin[_Beg_index++], _Radix, _Proj_func)];
        }
    });

    int _Index = -1, _Count = 0;

    // Partial sum cross different threads' chunk counters
    for (int _I = 0; _I < 256; _I++)
    {
        size_t _Last = _I ? _Chunks[_Threads_num - 1][_I - 1] : 0;
        _Chunks[0][_I] += _Last;

        for (size_t _J = 1; _J < _Threads_num; _J++)
        {
            _Chunks[_J][_I] += _Chunks[_J - 1][_I];
        }

        // "_Chunks[_Threads_num - 1][_I] - _Last" will get the global _Size for chunk _I(including all threads local _Size for chunk _I)
        // this will chunk whether the chunk _I is empty or not. If it's not empty, it will be recorded.
        if (_Chunks[_Threads_num - 1][_I] - _Last)
        {
            ++_Count;
            _Index = _I;
        }
    }

    // If there is more than 1 chunk that has content, then continue the original algorithm
    if (_Count > 1)
    {
        // Move the elements in parallel into each chunk
        ::Concurrency::parallel_for(static_cast<size_t>(0), _Threads_num, [=](size_t _Index)
        {
            size_t _Beg_index, _End_index;

            // Calculate the segment position
            if (_Index < _Remain)
            {
                _Beg_index = _Index * (_Step + 1);
                _End_index = _Beg_index + (_Step + 1);
            }
            else
            {
                _Beg_index = _Remain * (_Step + 1) + (_Index - _Remain) * _Step;
                _End_index = _Beg_index + _Step;
            }

            // Do a move operation to directly put each value into its destination chunk
            // Chunk pointer is moved after each put operation.
            if (_Beg_index != _End_index--)
            {
                while (_Beg_index != _End_index)
                {
                    _Output[--_Chunks[_Index][_Radix_key(_Begin[_End_index], _Radix, _Proj_func)]] = ::std::move(_Begin[_End_index]);
                    --_End_index;
                }
                _Output[--_Chunks[_Index][_Radix_key(_Begin[_End_index], _Radix, _Proj_func)]] = ::std::move(_Begin[_End_index]);
            }
        });

        // Invoke _parallel_integer_radix_sort in parallel for each chunk
        ::Concurrency::parallel_for(static_cast<size_t>(0), static_cast<size_t>(256), [=](size_t _Index)
        {
            if (_Index < 256 - 1)
            {
                _Parallel_integer_radix_sort(_Output + _Chunks[0][_Index], _Chunks[0][_Index + 1] - _Chunks[0][_Index],
                    _Begin + _Chunks[0][_Index], _Radix - 1, _Proj_func, _Chunk_size, _Deep + 1);
            }
            else
            {
                _Parallel_integer_radix_sort(_Output + _Chunks[0][_Index], _Size - _Chunks[0][_Index],
                    _Begin + _Chunks[0][_Index], _Radix - 1, _Proj_func, _Chunk_size, _Deep + 1);
            }
        });
    }
    else
    {
        // Only one chunk has content
        // A special optimization is applied because one chunk means all numbers have a same value on this particular byte (digit).
        // Because we cannot sort them at all (they are all equal at this point), directly call _parallel_integer_radix_sort to
        // sort next byte (digit)
        _Parallel_integer_radix_sort(_Begin, _Size, _Output, _Radix - 1, _Proj_func, _Chunk_size, _Deep);
    }
}
*/
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
    return MSD_recursion_exit_integer_radix_sort_<Radix_opts_>(Begin_, Size_, Output_, Radix_, Proj_func_, Deep_);
  }

  int const  Threads_num_       = omp_get_num_procs();
  bool const Can_enable_parallel_ = Threads_num_ > 1 && !omp_in_parallel() && Size_ > Radix_opts_.chunk_size * 2;

  if (!Can_enable_parallel_)
  {
    size_t Chunk_[Radix_opts_.bucket_size] {};

    Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      ++Chunk_[Radix_key_<Radix_opts_>(Begin_[Index_], Radix_, Proj_func_)];
    });

    size_t Non_empty_count_ = 0, Prev_sum_ = 0;

    Unroll_loop_<Radix_opts_.unroll_n>(0, Radix_opts_.bucket_size, [&](size_t Index_)
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

      Unroll_loop_<Radix_opts_.unroll_n>(0, Radix_opts_.bucket_size - 1, [&](size_t Index_)
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
    return;
  }

  using Chunk_ptr_ty_ = size_t (*)[Radix_opts_.bucket_size];

  Chunk_ptr_ty_ Chunks_ = nullptr;
  int           Buffer_size_ = static_cast<int>(Radix_opts_.bucket_size) * Threads_num_;
  Radix_parallel_data_ Paral_data_ { .Step_        = Size_ / Threads_num_,
                                     .Remain_      = static_cast<int>(Size_ % Threads_num_),
                                     .Threads_num_ = Threads_num_ };

  AllocatedBufferHolder_<std::allocator<size_t>> Heap_bucket_holder_;
  if (Buffer_size_ <= 1024)
  {
    size_t *Stack_bucket_buffer_ = static_cast<size_t *>(alloca(Buffer_size_ * sizeof(size_t)));
    Chunks_                      = reinterpret_cast<Chunk_ptr_ty_>(Stack_bucket_buffer_);
  }
  else
  {
    Heap_bucket_holder_.Allocate_(Buffer_size_, std::allocator<size_t> {});
    Chunks_ = reinterpret_cast<Chunk_ptr_ty_>(Heap_bucket_holder_.Get_buffer_());
  }

  ::memset(Chunks_, 0, static_cast<size_t>(Buffer_size_) * sizeof(size_t));

#pragma omp parallel num_threads(Threads_num_)
  {
    int const Thread_id_ = omp_get_thread_num();

    auto [Beg_index_, End_index_] = Radix_compute_task_indices_(Thread_id_, Paral_data_.Step_, Paral_data_.Remain_);

    Unroll_loop_<Radix_opts_.unroll_n>(Beg_index_, End_index_, [&](size_t Index_)
    {
      ++Chunks_[Thread_id_][Radix_key_<Radix_opts_>(Begin_[Index_], Radix_, Proj_func_)];
    });
  }

  size_t Non_empty_count_ = 0;

  Unroll_loop_<Radix_opts_.unroll_n>(0, Radix_opts_.bucket_size, [&](size_t Index_)
  {
    size_t Last_ = Index_ ? Chunks_[Threads_num_ - 1][Index_ - 1] : 0;
    Chunks_[0][Index_] += Last_;

    for (int Thread_id_ = 1; Thread_id_ < Threads_num_; ++Thread_id_)
    {
      Chunks_[Thread_id_][Index_] += Chunks_[Thread_id_ - 1][Index_];
    }

    Non_empty_count_ += (Chunks_[Threads_num_ - 1][Index_] - Last_ != 0);
  });

  if (Non_empty_count_ <= 1)
  {
    MSD_integer_radix_sort_<Radix_opts_>(Begin_, Size_, Output_, Radix_ - 1, Proj_func_, Deep_);
    return;
  }

#pragma omp parallel num_threads(Threads_num_)
  {
    int const Thread_id_ = omp_get_thread_num();

    auto [Beg_index_, End_index_] = Radix_compute_task_indices_(Thread_id_, Paral_data_.Step_, Paral_data_.Remain_);

    Unroll_loop_<Radix_opts_.unroll_n>(Beg_index_, End_index_, [&](size_t Index_)
    {
      size_t Curr_index_ = Beg_index_ + (End_index_ - 1 - Index_);
      Output_[--Chunks_[Thread_id_][Radix_key_<Radix_opts_>(Begin_[Curr_index_], Radix_, Proj_func_)]] =
          std::move(Begin_[Curr_index_]);
    });
  }

#pragma omp parallel for schedule(dynamic) num_threads(Threads_num_)
  for (int Bucket_idx_ = 0; Bucket_idx_ < static_cast<int>(Radix_opts_.bucket_size); ++Bucket_idx_)
  {
    size_t Chunk_begin_ = Chunks_[0][Bucket_idx_];
    size_t Chunk_end_   = (Bucket_idx_ + 1 < static_cast<int>(Radix_opts_.bucket_size))
                              ? Chunks_[0][Bucket_idx_ + 1]
                              : Size_;
    size_t Chunk_size_  = Chunk_end_ - Chunk_begin_;

    if (Chunk_size_ == 0)
    {
      continue;
    }

    auto *New_beign_  = Begin_ + Chunk_begin_;
    auto *New_output_ = Output_ + Chunk_begin_;

    MSD_integer_radix_sort_<Radix_opts_>(New_output_, Chunk_size_, New_beign_, Radix_ - 1, Proj_func_, Deep_ + 1);
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
    if constexpr (sizeof(typename Proj_functor_::Key_ty_) > 4)
    {
      MSD_integer_radix_sort_<Radix_opts_>(Begin_, Size_, Output_, Radix_, Proj_func_);
    }
    else
    {
      if constexpr (Radix_opts_.bucket_size == 256U)
      {
        Radix_ = sizeof(typename Proj_functor_::Unsigned_ty_);
      }
      else /*if constexpr (Radix_opts_.bucket_size == 65536U)*/
      {
        Radix_ = sizeof(typename Proj_functor_::Unsigned_ty_);
      }
      LSD_integer_radix_sort_<Is_parallel_policy_v_<Expo_ty_>, Radix_opts_>(Begin_, Size_, Output_, Radix_, Proj_func_);
    }
  }
  else /*if constexpr (std::same_as<Expo_ty_, std::execution::parallel_policy> or
                     std::same_as<Expo_ty_, std::execution::parallel_unsequenced_policy>)*/
  {
    if constexpr (Radix_opts_.bucket_size == 256U)
    {
      Radix_ = sizeof(typename Proj_functor_::Unsigned_ty_);
    }
    else /*if constexpr (Radix_opts_.bucket_size == 65536U)*/
    {
      Radix_ = sizeof(typename Proj_functor_::Unsigned_ty_);
    }
    LSD_integer_radix_sort_<Is_parallel_policy_v_<Expo_ty_>, Radix_opts_>(Begin_, Size_, Output_, Radix_, Proj_func_);
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
  Proj_functor_ Proj_func_ { std::forward<Key_function_>(F_) };
  static_assert(!(Radix_opts_.bucket_size == 65536U && sizeof(typename Proj_functor_::Key_ty_) == 1),
                "Do not set a two-byte buffer size for one-byte data!");

  auto *RESTRICT_KEYWORD Primary_addr_ = &*Begin_;
  if constexpr (Reverse_iterator_<Random_iter_>)
  {
    Primary_addr_ = &*End_.base();
  }
  constexpr auto Adjust_opts_ = Adjust_radix_options_(Radix_opts_, Is_reverse_iterator_v_<Random_iter_>);

  size_t Range_left_ = 0, Range_right_ = Size_;
  size_t Max_val_ = 0;

  if constexpr (sizeof(typename Proj_functor_::Key_ty_) > 4)
  {
    if constexpr (std::integral<typename Proj_functor_::Key_ty_> or
                  (std::floating_point<typename Proj_functor_::Key_ty_> and
                   Adjust_opts_.nan_pos == radix_nan_position::unhandled))
    {
      Max_val_ = Max_element_<false, Adjust_opts_.unroll_n>(Primary_addr_, Size_, Proj_func_);
    }
    else if constexpr (Adjust_opts_.nan_pos == radix_nan_position::begin)
    {
      auto Max_with_part_ = Max_while_partition_(Primary_addr_, Primary_addr_ + Size_, [&](auto const &Val_)
      {
        return std::isnan(Proj_func_.Get_key_value_(Val_));
      }, Proj_func_);

      Range_left_ = Max_with_part_.Part_index_;
      Max_val_    = Max_with_part_.Max_val_;
    }
    else /*if constexpr (Adjust_opts_.nan_pos == radix_nan_position::end)*/
    {
      auto Max_with_part_ = Max_while_partition_(Primary_addr_, Primary_addr_ + Size_, [&](auto const &Val_)
      {
        return !std::isnan(Proj_func_.Get_key_value_(Val_));
      }, Proj_func_);

      Range_right_ = Max_with_part_.Part_index_;
      Max_val_     = Max_with_part_.Max_val_;
    }

    // If all elements are NaN or Zero, then there is no need to sort
    if (Max_val_ == 0) [[unlikely]]
    {
      return;
    }

    // Calculate highest byte index
    Max_val_ = static_cast<size_t>((std::bit_width(Max_val_) + 7) / 8 - 1);

    Primary_addr_ = Primary_addr_ + Range_left_;
  }

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

  size_t const Sort_size_ = Range_right_ - Range_left_;
  if (Sort_size_ == 0) [[unlikely]]
  {
    return;
  }

  Radix_sort_impl_<Adjust_opts_>(std::forward<Exec_policy_>(Expo_), Primary_addr_, Sort_size_, Buffer_, Max_val_,
                                 Proj_func_);
}

template<radix_options               Radix_opts_ = radix_options {},
         Supported_execution_policy_ Exec_policy_,
         Random_access_iterator_     Random_iter_,
         typename Key_function_ = radix_default_key_function<std::iter_value_t<Random_iter_>>>
inline void radix_sort(Exec_policy_ &&Expo_, Random_iter_ Begin_, Random_iter_ End_, Key_function_ &&F_ = {})
{
  using Value_type_ = typename std::iterator_traits<Random_iter_>::value_type;

  radix_sort<Radix_opts_>(std::forward<Exec_policy_>(Expo_), Begin_, End_, std::allocator<Value_type_> {},
                          std::forward<Key_function_>(F_));
}

} // namespace stdex

#pragma pop_macro("ALWAYS_INLINE")
#pragma pop_macro("RESTRICT_KEYWORD")

#include <iostream>
#include <random>

template<typename T>
void
print_summary(const std::vector<T> &data, bool is_sorted, bool ascending)
{
  std::cout << (is_sorted ? "PASS" : "FAIL") << " (" << (ascending ? "asc" : "desc") << ")\n";
  if (!is_sorted && data.size() > 0)
  {
    std::cout << "  first few elements: ";
    for (size_t i = 0; i < std::min<size_t>(5, data.size()); ++i)
      std::cout << data[i] << ' ';
    std::cout << '\n';
  }
}

template<typename T>
void
test_type(bool ascending)
{
  constexpr size_t   N = 100000;
  std::random_device rd;
  std::mt19937       gen(rd());

  std::vector<T> data(N);

  // 生成随机数据
  if constexpr (std::is_integral_v<T>)
  {
    if constexpr (std::is_signed_v<T>)
    {
      std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
      for (auto &v : data)
        v = dist(gen);
    }
    else
    { // unsigned
      std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
      for (auto &v : data)
        v = dist(gen);
    }
  }
  else
  { // floating point
    std::uniform_real_distribution<T> dist(-100000.0, 100000.0);
    // 以 2% 的概率生成 INFINITY 或 -INFINITY
    std::bernoulli_distribution inf_prob(0.02);
    std::bernoulli_distribution sign_prob(0.5);
    for (auto &v : data)
    {
      if (inf_prob(gen))
      {
        v = sign_prob(gen) ? std::numeric_limits<T>::infinity() : -std::numeric_limits<T>::infinity();
      }
      else
      {
        v = dist(gen);
      }
    }
  }

  // 复制一份用于排序
  std::vector<T> sorted = data;

  // 调用 radix_sort
  if (ascending)
  {
    stdex::radix_sort(std::execution::par, sorted.begin(), sorted.end());
  }
  else
  {
    stdex::radix_sort<stdex::radix_options { .sort_order = stdex::radix_sort_order::desc }>(
        std::execution::seq, sorted.begin(), sorted.end());
  }

  // 验证排序结果
  bool ok;
  if (ascending)
  {
    ok = std::is_sorted(sorted.begin(), sorted.end());
  }
  else
  {
    ok = std::is_sorted(sorted.begin(), sorted.end(), std::greater<>());
  }
  std::cout << typeid(T).name() << " : ";
  std::cout << (ok ? "PASS" : "FAIL") << " (" << (ascending ? "asc" : "desc") << ")\n";
}

#include <ppl.h>

int
main()
{
  // std::vector v { INFINITY, -INFINITY, NAN, -0.0f, 1.0f / 1.0f, 0.0f, -1.0f / 1.0f, std::sqrt(-1.0f), 3.14f };
  // stdex::radix_sort<stdex::radix_options { .nan_pos = stdex::radix_nan_position::end }>(std::execution::par, v.rbegin(),
  //                                                                                       v.rend());
  //
  // std::println("{}", v);

  std::cout << "=== Testing ascending order (default) ===\n";
  test_type<int64_t>(true);
  //test_type<uint64_t>(true);
  //test_type<double>(true);

  test_type<int16_t>(true);
  test_type<uint16_t>(true);
  test_type<int32_t>(true);
  test_type<uint32_t>(true);
  test_type<float>(true);

  //std::cout << "\n=== Testing descending order (radix_sort_order::desc) ===\n";
  //test_type<int16_t>(false);
  //test_type<uint16_t>(false);
  //test_type<int32_t>(false);
  //test_type<uint32_t>(false);
  //test_type<float>(false);

  //test_type<double>(false);
  //test_type<int64_t>(false);
  //test_type<uint64_t>(false);
  return 0;
}
