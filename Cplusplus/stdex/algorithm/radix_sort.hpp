/*
 * stdex::radix_sort – A High-Performance Parallel/Sequential Radix Sort for C++20
 * ==============================================================================
 *
 * OVERVIEW
 * --------
 * This header provides a highly optimized radix sort implementation supporting:
 *   - Integral and floating‑point types (float, double)
 *   - Custom key extraction (projection) and comparison order (ascending / descending)
 *   - Fine‑grained control over NaN placement (beginning, end, or unhandled)
 *   - Sequential, parallel, unsequenced, and parallel_unsequenced execution policies
 *   - Custom allocators for the temporary buffer
 *   - Explicit selection of LSD (Least Significant Digit) or MSD (Most Significant Digit)
 *     via radix_options::sort_mode (default = LSD)
 *   - Loop unrolling, cache‑friendly bucket processing, and OpenMP parallelism
 *
 * The user can choose the sorting strategy via radix_options::sort_mode (lsd or msd).
 * LSD is generally faster for uniformly distributed data and small key types,
 * while MSD can be more cache‑efficient for 64‑bit keys and allows early exit
 * when many elements share the same high‑order digits. The implementation does
 * not automatically switch between strategies; the choice is explicit.
 *
 *
 * DESIGN & IMPLEMENTATION NOTES
 * -----------------------------
 *
 * 1. Key Projection & Transformation
 *    - `radix_default_key_function` extracts the sort key (by default the element itself).
 *    - Signed integers are transformed via a bias (midpoint shift) so that negative
 *      values appear before positives in unsigned radix order.
 *    - Floating‑point numbers are bit‑cast to unsigned integers and the sign bit is
 *      flipped, placing negative zeros and negatives correctly. NaNs are handled
 *      separately when requested.
 *
 * 2. Radix Digit Selection
 *    - Two bucket sizes are supported: 256 (8‑bit digit) and 65536 (16‑bit digit).
 *    - Smaller bucket size reduces memory traffic but increases the number of passes.
 *    - LSD: number of passes = sizeof(key_type) (for 8‑bit) or sizeof(key_type)/2 (for 16‑bit).
 *    - MSD: one pass per digit, with recursion depth = number of digits.
 *
 * 3. Parallelism Strategy
 *    - OpenMP is used for both histogramming and scattering.
 *    - The input range is partitioned into nearly equal chunks per thread.
 *    - Histograms are private per thread and then merged with a parallel prefix sum.
 *    - MSD recursion parallelizes over independent buckets after the first digit pass.
 *    - For MSVC with native OpenMP 2.0, workarounds (e.g., critical sections for reductions)
 *      are used where needed.
 *
 * 4. Memory Management
 *    - A temporary buffer of the same size as the input range is allocated using the
 *      provided allocator (or `std::allocator` by default).
 *    - For small thread‑local bucket arrays, stack memory (`alloca`) is used to avoid
 *      heap allocations.
 *    - The `AllocatedBufferHolder_` RAII class ensures proper construction/destruction
 *      of non‑trivial types.
 *
 * 5. Floating‑Point & NaN Handling
 *    - NaNs are detected via `std::isnan`. When `nan_pos` is `begin` or `end`, a
 *      partitioning step (performed while computing the maximum key) moves all NaNs
 *      to the chosen side before sorting the non‑NaN elements. This preserves the
 *      relative order of NaNs (which are all considered equal by the sorting key).
 *    - Infinity values are handled correctly because their bit patterns are ordered
 *      after the sign‑flip transformation.
 *
 * 6. Performance Optimisations
 *    - Loop unrolling (configurable via `unroll_n` in `radix_options`).
 *    - `RESTRICT_KEYWORD` (__restrict) to enable better alias analysis.
 *    - `ALWAYS_INLINE` for critical small functions.
 *    - Trivial type detection avoids unnecessary construction/destruction in buffers.
 *    - Early exit when only one non‑empty bucket exists.
 *    - MSD recursion uses a threshold (`chunk_size`) to switch to a multi‑pass LSD‑like
 *      fallback for small sub‑problems, reducing recursion overhead.
 *
 *
 * USAGE EXAMPLES
 * --------------
 *
 * Basic ascending sort with default execution policy (sequential, LSD):
 *   std::vector<int> v = {5, 2, 8, 1};
 *   stdex::radix_sort(std::execution::seq, v.begin(), v.end());
 *
 * Parallel descending sort for floats, placing NaNs at the end, using LSD:
 *   constexpr stdex::radix_options opts {
 *      .sort_order = stdex::radix_sort_order::desc,
 *      .nan_pos    = stdex::radix_nan_position::end,
 *   };
 *   stdex::radix_sort<opts>(std::execution::par, v.begin(), v.end());
 *
 * Explicitly use MSD (Most Significant Digit) for 64‑bit integers:
 *   constexpr stdex::radix_options msd_opts {
 *      .sort_mode = stdex::radix_options::msd
 *   };
 *   stdex::radix_sort<msd_opts>(std::execution::par, v.begin(), v.end());
 *
 * Using a custom key projection (e.g., sort by absolute value):
 *   auto abs_key = [](double x) { return std::fabs(x); };
 *   stdex::radix_sort(std::execution::par_unseq, v.begin(), v.end(), abs_key);
 *
 * Sorting a reverse range (descending order can also be achieved via reverse iterators):
 *   stdex::radix_sort(std::execution::par, v.rbegin(), v.rend());
 *
 * Using a custom allocator:
 *   std::vector<int, MyAllocator<int>> v(10000);
 *   // ... fill v ...
 *   stdex::radix_sort(std::execution::par, v.begin(), v.end(), MyAllocator<int>());
 *
 *
 * API REFERENCE (brief)
 * ---------------------
 *
 * namespace stdex {
 *
 *   enum class radix_sort_order { asc, desc };
 *   enum class radix_nan_position { unhandled, begin, end };
 *
 *   struct radix_options {
 *     enum mode { lsd, msd };
 *     mode         sort_mode   = lsd;      // LSD or MSD strategy
 *     uint8_t      unroll_n    = 8;        // loop unroll factor
 *     uint16_t     bucket_size = 256;      // 256 or 65536
 *     uint32_t     chunk_size  = 256*256;  // threshold for MSD→LSD fallback
 *     radix_sort_order   sort_order = asc;
 *     radix_nan_position nan_pos    = unhandled;
 *   };
 *
 *   // Overloads with explicit allocator
 *   template<radix_options opts = radix_options{},
 *            class ExecPolicy,
 *            RandomAccessIterator Iter,
 *            class Allocator,
 *            class KeyFunc = radix_default_key_function<std::iter_value_t<Iter>>>
 *   void radix_sort(ExecPolicy&& policy,
 *                   Iter first, Iter last,
 *                   const Allocator& alloc,
 *                   KeyFunc&& key_func = {});
 *
 *   // Overload without explicit allocator (uses std::allocator)
 *   template<radix_options opts = radix_options{},
 *            class ExecPolicy,
 *            RandomAccessIterator Iter,
 *            class KeyFunc = radix_default_key_function<std::iter_value_t<Iter>>>
 *   void radix_sort(ExecPolicy&& policy, Iter first, Iter last, KeyFunc&& key_func = {});
 *
 *   // Container overloads
 *   template<radix_options opts = radix_options{},
 *            class ExecPolicy, class Container, class KeyFunc = ...>
 *   void radix_sort(ExecPolicy&& policy, Container& c, KeyFunc&& key_func = {});
 *
 *   template<radix_options opts = radix_options{},
 *            class Container, class KeyFunc = ...>
 *   void radix_sort(Container& c, KeyFunc&& key_func = {});
 * }
 *
 * Supported execution policies:
 *   - std::execution::seq                (sequential)
 *   - std::execution::par                (parallel, uses OpenMP)
 *   - std::execution::unseq              (vectorised sequential)
 *   - std::execution::par_unseq          (parallel + vectorisation)
 *
 * REQUIREMENTS
 * ------------
 * - C++20 or later
 * - OpenMP 2.0+ (for parallel policies)
 * - Compilers: GCC >= 11.1, Clang >= 13.0, MSVC >= 19.28 (VS 2019 16.8)
 *
 * ==============================================================================
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
#include <span>
#include <type_traits>

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
  enum mode
  {
    lsd,
    msd
  } sort_mode { lsd };

  uint32_t           unroll_n    = 8U;
  uint32_t           bucket_size = 256U;
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

template<typename T>
concept Random_access_range_ = requires (T &t) {
  { t.begin() } -> Random_access_iterator_;
  { t.end() } -> Random_access_iterator_;
};

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
#if defined(_MSC_VER) && !defined(_OPENMP_LLVM_RUNTIME)
    int Actual_threads_ = omp_get_max_threads();
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
#  pragma omp parallel for reduction(max : Max_val_)
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
Destroy_buffer_(typename ::std::allocator_traits<Allocator_>::pointer P_, size_t N_, Allocator_ &Alloc_) noexcept
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
Radix_key_(Unsigned_ty_ Val_, size_t Radix_) noexcept
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
Radix_key_(Ty_ const &Val_, size_t Radix_, Proj_functor_ const &Proj_func_) noexcept
{
  return Radix_key_<Radix_opts_>(Proj_func_(Tag_ {}, Proj_func_.Get_key_value_(Val_)), Radix_);
}

template<radix_options Radix_opts_, typename Ty_, typename Proj_functor_>
ALWAYS_INLINE size_t
Radix_key_(Ty_ const &Val_, size_t Radix_, Proj_functor_ const &Proj_func_) noexcept
{
  return Radix_key_<Radix_projection_functor_default_tag_, Radix_opts_, Ty_, Proj_functor_>(Val_, Radix_, Proj_func_);
}

struct Radix_parallel_data_
{
  size_t Step_;
  int    Remain_;
  int    Threads_num_ { 1 };
};

ALWAYS_INLINE Radix_parallel_data_
Radix_make_parallel_data_(size_t Size_, int Threads_num_) noexcept
{
  return Radix_parallel_data_ { .Step_        = Size_ / static_cast<size_t>(Threads_num_),
                                .Remain_      = static_cast<int>(Size_ % static_cast<size_t>(Threads_num_)),
                                .Threads_num_ = Threads_num_ };
}

template<radix_options Radix_opts_>
struct Radix_chunk_context_
{
  using Chunk_ptr_ty_ = size_t (*)[Radix_opts_.bucket_size];

  static constexpr size_t Stack_bucket_capacity_ = 1024;

  //size_t                                         Stack_bucket_buffer_[Stack_bucket_capacity_];
  AllocatedBufferHolder_<std::allocator<size_t>> Heap_bucket_holder_;
  Chunk_ptr_ty_                                  Chunks_ { nullptr };
  size_t                                         Buffer_size_ { 0 };
  Radix_parallel_data_                           Paral_data_ {};

  ALWAYS_INLINE void Init_(size_t Size_, int Threads_num_)
  {
    Buffer_size_ = static_cast<size_t>(Radix_opts_.bucket_size) * static_cast<size_t>(Threads_num_);
    Paral_data_  = Radix_make_parallel_data_(Size_, Threads_num_);

    if (Buffer_size_ <= Stack_bucket_capacity_)
    {
      size_t *Stack_bucket_buffer_ = static_cast<size_t *>(alloca(Buffer_size_ * sizeof(size_t)));
      Chunks_                      = reinterpret_cast<Chunk_ptr_ty_>(Stack_bucket_buffer_);
    }
    else
    {
      Heap_bucket_holder_.Allocate_(Buffer_size_, std::allocator<size_t> {});
      Chunks_ = reinterpret_cast<Chunk_ptr_ty_>(Heap_bucket_holder_.Get_buffer_());
    }
  }
};

ALWAYS_INLINE std::pair<size_t, size_t>
              Radix_compute_task_indices_(size_t Thread_id_, size_t Step_, size_t Remain_) noexcept
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
        Output_[--NaN_offset_] = std::move(Begin_[Index_]);
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
                        [[maybe_unused]] std::span<size_t> NaN_counts_  = {},
                        [[maybe_unused]] std::span<size_t> NaN_offsets_ = {})
{
  bool Need_collect_ = true;

  if constexpr (Is_parallel_)
  {
#pragma omp parallel if (Paral_data_.Threads_num_ > 1)
    {
      int const Thread_id_ = omp_get_thread_num();

      auto [Beg_index_, End_index_] = Radix_compute_task_indices_(Thread_id_, Paral_data_.Step_, Paral_data_.Remain_);

      Unroll_loop_<Radix_opts_.unroll_n>(Beg_index_, End_index_, [&](size_t Index_)
      {
        Radix_lsd_histogram_<Is_lass_pass_, Radix_opts_>(Begin_[Index_], Chunks_, Thread_id_, Radix_, Proj_func_,
                                                         NaN_counts_);
      });

#pragma omp barrier
#pragma omp single
      {
        if constexpr (std::floating_point<typename Proj_functor_::Key_ty_> and
                      Radix_opts_.nan_pos != radix_nan_position::unhandled and Is_lass_pass_)
        {
          size_t Prefix_NaN_count_ = 0;
          for (size_t Thread_id_ = 0; Thread_id_ < static_cast<size_t>(Paral_data_.Threads_num_); ++Thread_id_)
          {
            Prefix_NaN_count_ += NaN_counts_[Thread_id_];
            NaN_offsets_[Thread_id_] = Prefix_NaN_count_;
          }

          if constexpr (Radix_opts_.nan_pos == radix_nan_position::begin)
          {
            for (size_t Thread_id_ = 0; Thread_id_ < static_cast<size_t>(Paral_data_.Threads_num_); ++Thread_id_)
            {
              Chunks_[Thread_id_][0] += NaN_counts_[Thread_id_];
            }
          }
          else if constexpr (Radix_opts_.nan_pos == radix_nan_position::end)
          {
            size_t const NaN_base_ = Size_ - Prefix_NaN_count_;
            for (size_t Thread_id_ = 0; Thread_id_ < static_cast<size_t>(Paral_data_.Threads_num_); ++Thread_id_)
            {
              NaN_offsets_[Thread_id_] += NaN_base_;
            }
          }
        }

        size_t Non_empty_count_ = 0;
        Unroll_loop_<Radix_opts_.unroll_n>(0, Radix_opts_.bucket_size, [&](size_t Index_)
        {
          size_t Last_ = Index_ ? Chunks_[Paral_data_.Threads_num_ - 1][Index_ - 1] : 0;
          Chunks_[0][Index_] += Last_;

          for (size_t Local_thread_id_ = 1; Local_thread_id_ < static_cast<size_t>(Paral_data_.Threads_num_);
               ++Local_thread_id_)
          {
            Chunks_[Local_thread_id_][Index_] += Chunks_[Local_thread_id_ - 1][Index_];
          }

          Non_empty_count_ += (Chunks_[Paral_data_.Threads_num_ - 1][Index_] - Last_ != 0);
        });

        Need_collect_ = Non_empty_count_ > 1;
      }
#pragma omp barrier

      if (Need_collect_)
      {
        size_t Thread_nan_offset_ = 0;
        if constexpr (std::floating_point<typename Proj_functor_::Key_ty_> and
                      Radix_opts_.nan_pos != radix_nan_position::unhandled and Is_lass_pass_)
        {
          Thread_nan_offset_ = NaN_offsets_[Thread_id_];
        }

        Unroll_loop_<Radix_opts_.unroll_n>(Beg_index_, End_index_, [&](size_t Index_)
        {
          Radix_lsd_collect_<Is_lass_pass_, Radix_opts_>(Begin_, Size_, Beg_index_ + (End_index_ - 1 - Index_), Output_,
                                                         Chunks_, Thread_id_, Radix_, Proj_func_, Thread_nan_offset_);
        });
      }
    }

    if (!Need_collect_)
    {
      return;
    }
  }
  else
  {
    Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      Radix_lsd_histogram_<Is_lass_pass_, Radix_opts_>(Begin_[Index_], Chunks_, 0, Radix_, Proj_func_, NaN_counts_);
    });

    if constexpr (std::floating_point<typename Proj_functor_::Key_ty_> and
                  Radix_opts_.nan_pos != radix_nan_position::unhandled and Is_lass_pass_)
    {
      size_t Prefix_NaN_count_ = NaN_counts_[0];
      NaN_offsets_[0]          = Prefix_NaN_count_;

      if constexpr (Radix_opts_.nan_pos == radix_nan_position::begin)
      {
        Chunks_[0][0] += NaN_counts_[0];
      }
      else if constexpr (Radix_opts_.nan_pos == radix_nan_position::end)
      {
        NaN_offsets_[0] += (Size_ - Prefix_NaN_count_);
      }
    }

    size_t Non_empty_count_ = 0;
    Unroll_loop_<Radix_opts_.unroll_n>(0, Radix_opts_.bucket_size, [&](size_t Index_)
    {
      size_t Last_ = Index_ ? Chunks_[0][Index_ - 1] : 0;
      Chunks_[0][Index_] += Last_;
      Non_empty_count_ += (Chunks_[0][Index_] - Last_ != 0);
    });

    if (Non_empty_count_ <= 1)
    {
      return;
    }

    size_t Local_nan_offset_ = 0;
    if constexpr (std::floating_point<typename Proj_functor_::Key_ty_> and
                  Radix_opts_.nan_pos != radix_nan_position::unhandled and Is_lass_pass_)
    {
      Local_nan_offset_ = NaN_offsets_[0];
    }

    Unroll_loop_<Radix_opts_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      Radix_lsd_collect_<Is_lass_pass_, Radix_opts_>(Begin_, Size_, Size_ - 1 - Index_, Output_, Chunks_, 0, Radix_,
                                                     Proj_func_, Local_nan_offset_);
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

  int Threads_num_ = 1;
  if constexpr (Is_parallel_)
  {
    Threads_num_ = omp_get_max_threads();
  }

  Radix_chunk_context_<Radix_opts_> Chunk_ctx_;
  Chunk_ctx_.Init_(Size_, Threads_num_);

  auto *RESTRICT_KEYWORD Chunks_      = Chunk_ctx_.Chunks_;
  size_t                 Buffer_size_ = Chunk_ctx_.Buffer_size_ * sizeof(size_t);
  auto const             Paral_data_  = Chunk_ctx_.Paral_data_;
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
    ::memset(Chunks_, 0, Buffer_size_);

    LSD_integer_radix_pass_<Is_parallel_, false, Radix_opts_>(Paral_data_, Begin_, Size_, Output_, Chunks_, Curr_radix_,
                                                              Proj_func_);
  }

  {
    ::memset(Chunks_, 0, Buffer_size_);

    std::span<size_t> NaN_counts_span_;
    std::span<size_t> NaN_offsets_span_;

    if constexpr (std::floating_point<typename Proj_functor_::Key_ty_> and
                  Radix_opts_.nan_pos != radix_nan_position::unhandled)
    {
      size_t *NaN_counts_  = static_cast<size_t *>(alloca(Threads_num_ * sizeof(size_t)));
      size_t *NaN_offsets_ = static_cast<size_t *>(alloca(Threads_num_ * sizeof(size_t)));
      ::memset(NaN_counts_, 0, Threads_num_ * sizeof(size_t));
      ::memset(NaN_offsets_, 0, Threads_num_ * sizeof(size_t));
      NaN_counts_span_  = std::span<size_t>(NaN_counts_, Threads_num_);
      NaN_offsets_span_ = std::span<size_t>(NaN_offsets_, Threads_num_);
    }

    LSD_integer_radix_pass_<Is_parallel_, true, Radix_opts_>(Paral_data_, Begin_, Size_, Output_, Chunks_, Radix_ - 1,
                                                             Proj_func_, NaN_counts_span_, NaN_offsets_span_);
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

template<bool          Is_parallel_,
         radix_options Radix_opts_,
         typename Random_point_,
         typename Random_buffer_point_,
         typename Proj_functor_>
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

  if constexpr (Is_parallel_)
  {
    // Avoid nested team creation overhead in recursive calls.
    if (omp_in_parallel())
    {
      return MSD_integer_radix_sort_<false, Radix_opts_>(Begin_, Size_, Output_, Radix_, Proj_func_, Deep_);
    }
  }

  // bool const Can_enable_parallel_ = Threads_num_ > 1 && !omp_in_parallel() && Size_ > Radix_opts_.chunk_size * 2;

  if constexpr (!Is_parallel_)
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

        MSD_integer_radix_sort_<Is_parallel_, Radix_opts_>(New_output_, Chunk_[Index_ + 1] - Chunk_[Index_], New_beign_,
                                                           Radix_ - 1, Proj_func_, Deep_ + 1);
      });
      auto *New_beign_  = Begin_ + Chunk_[Radix_opts_.bucket_size - 1];
      auto *New_output_ = Output_ + Chunk_[Radix_opts_.bucket_size - 1];
      MSD_integer_radix_sort_<Is_parallel_, Radix_opts_>(New_output_, Size_ - Chunk_[Radix_opts_.bucket_size - 1],
                                                         New_beign_, Radix_ - 1, Proj_func_, Deep_ + 1);
    }
    else
    {
      // Only one non-empty bucket:
      // all elements of the current byte have the same value, no need to reorder, directly process the next byte
      MSD_integer_radix_sort_<Is_parallel_, Radix_opts_>(Begin_, Size_, Output_, Radix_ - 1, Proj_func_, Deep_);
    }
    return;
  }
  else
  {
    int const                         Threads_num_ = omp_get_max_threads();
    Radix_chunk_context_<Radix_opts_> Chunk_ctx_;
    Chunk_ctx_.Init_(Size_, Threads_num_);

    auto *RESTRICT_KEYWORD Chunks_     = Chunk_ctx_.Chunks_;
    auto const             Paral_data_ = Chunk_ctx_.Paral_data_;

    ::memset(Chunks_, 0, Chunk_ctx_.Buffer_size_ * sizeof(size_t));

#pragma omp parallel if (Threads_num_ > 1)
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
      MSD_integer_radix_sort_<Is_parallel_, Radix_opts_>(Begin_, Size_, Output_, Radix_ - 1, Proj_func_, Deep_);
      return;
    }

#pragma omp parallel if (Threads_num_ > 1)
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

#pragma omp parallel for schedule(guided, 1) if (Threads_num_ > 1)
    for (int Bucket_idx_ = 0; Bucket_idx_ < static_cast<int>(Radix_opts_.bucket_size); ++Bucket_idx_)
    {
      size_t Chunk_begin_ = Chunks_[0][Bucket_idx_];
      size_t Chunk_end_ =
          (Bucket_idx_ + 1 < static_cast<int>(Radix_opts_.bucket_size)) ? Chunks_[0][Bucket_idx_ + 1] : Size_;
      size_t Chunk_size_ = Chunk_end_ - Chunk_begin_;

      if (Chunk_size_ == 0)
      {
        continue;
      }

      auto *New_beign_  = Begin_ + Chunk_begin_;
      auto *New_output_ = Output_ + Chunk_begin_;

      MSD_integer_radix_sort_<Is_parallel_, Radix_opts_>(New_output_, Chunk_size_, New_beign_, Radix_ - 1, Proj_func_,
                                                         Deep_ + 1);
    }
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
  if constexpr (Radix_opts_.sort_mode == radix_options::msd)
  {
    MSD_integer_radix_sort_<Is_parallel_policy_v_<Expo_ty_>, Radix_opts_>(Begin_, Size_, Output_, Radix_, Proj_func_);
  }
  else
  {
    if constexpr (Radix_opts_.bucket_size == 256U)
    {
      Radix_ = sizeof(typename Proj_functor_::Unsigned_ty_);
    }
    else /*if constexpr (Radix_opts_.bucket_size == 65536U)*/
    {
      Radix_ = sizeof(typename Proj_functor_::Unsigned_ty_) / 2;
    }

    LSD_integer_radix_sort_<Is_parallel_policy_v_<Expo_ty_>, Radix_opts_>(Begin_, Size_, Output_, Radix_, Proj_func_);
  }
}

template<radix_options               Radix_opts_ = radix_options {},
         Supported_execution_policy_ Exec_policy_,
         Random_access_iterator_     Random_iter_,
         Standard_allocator_         Allocator_,
         typename Key_function_ = radix_default_key_function<std::iter_value_t<Random_iter_>>>
inline void radix_sort(Exec_policy_    &&Expo_,
                       Random_iter_      Begin_,
                       Random_iter_      End_,
                       Allocator_ const &Alloc_,
                       Key_function_   &&Key_func_ = {})
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

  using Proj_functor_ = Radix_projection_functor_<Key_function_, decltype(Key_func_(*Begin_))>;
  Proj_functor_ Proj_func_ { std::forward<Key_function_>(Key_func_) };
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

template<radix_options Radix_opts_ = radix_options {},
         typename Exec_policy_,
         typename Random_iter_,
         typename Key_function_ = radix_default_key_function<std::iter_value_t<Random_iter_>>>
requires Supported_execution_policy_<Exec_policy_> && Random_access_iterator_<Random_iter_>
inline void radix_sort(Exec_policy_ &&Expo_, Random_iter_ Begin_, Random_iter_ End_, Key_function_ &&Key_func_ = {})
{
  using Value_type_ = typename std::iterator_traits<Random_iter_>::value_type;
  radix_sort<Radix_opts_>(std::forward<Exec_policy_>(Expo_), Begin_, End_, std::allocator<Value_type_> {},
                          std::forward<Key_function_>(Key_func_));
}

template<radix_options Radix_opts_ = radix_options {},
         typename Random_iter_,
         typename Key_function_ = radix_default_key_function<std::iter_value_t<Random_iter_>>>
requires Random_access_iterator_<Random_iter_>
inline void radix_sort(Random_iter_ Begin_, Random_iter_ End_, Key_function_ &&Key_func_ = {})
{
  radix_sort<Radix_opts_>(std::execution::seq, Begin_, End_, std::forward<Key_function_>(Key_func_));
}

template<radix_options Radix_opts_ = radix_options {},
         typename Exec_policy_,
         typename Container_,
         typename Key_function_ = radix_default_key_function<typename Container_::value_type>>
requires Supported_execution_policy_<Exec_policy_> && Random_access_range_<Container_>
inline void radix_sort(Exec_policy_ &&Expo_, Container_ &C_, Key_function_ &&Key_func_ = {})
{
  radix_sort<Radix_opts_>(std::forward<Exec_policy_>(Expo_), std::begin(C_), std::end(C_),
                          std::forward<Key_function_>(Key_func_));
}

template<radix_options Radix_opts_ = radix_options {},
         typename Container_,
         typename Key_function_ = radix_default_key_function<typename Container_::value_type>>
requires Random_access_range_<Container_>
inline void radix_sort(Container_ &C_, Key_function_ &&Key_func_ = {})
{
  radix_sort<Radix_opts_>(std::execution::seq, C_, std::forward<Key_function_>(Key_func_));
}

} // namespace stdex

#pragma pop_macro("ALWAYS_INLINE")
#pragma pop_macro("RESTRICT_KEYWORD")
