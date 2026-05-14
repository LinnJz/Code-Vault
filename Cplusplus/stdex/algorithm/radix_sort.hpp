/*
 * stdex::radix_sort – A High-Performance Parallel/Sequential Radix Sort for C++20
 * ==============================================================================
 *
 * OVERVIEW
 * --------
 * This header provides a highly optimized radix sort implementation supporting:
 *   - Integral and floating‑point types (float, double)
 *   - Custom key extraction (projection) and comparison order (ascending / descending)
 *   - Fine‑grained control over NaN placement (unhandled, at the beginning, or at the end)
 *   - Sequential, parallel, unsequenced, and parallel_unsequenced execution policies
 *   - Custom allocators for the temporary buffer
 *   - Explicit selection of LSD (Least Significant Digit) or MSD (Most Significant Digit)
 *     via radix_traits::mode_type (default = LSD)
 *   - Loop unrolling, cache‑friendly bucket processing, and OpenMP parallelism
 *   - Automatic handling of reverse iterators (order is swapped accordingly)
 *
 * The user chooses the sorting strategy via radix_traits::sort_mode (lsd or msd).
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
 *    - `radix_default_key_predicate` extracts the sort key (by default the element itself).
 *    - Signed integers are transformed via a bias (midpoint shift) for MSD, or
 *      by flipping the sign bit for LSD, so that negative values appear before positives
 *      in unsigned radix order.
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
 *    - For small thread‑local bucket arrays, a hybrid stack/heap allocation (`hybrid_scoped_malloca`)
 *      is used to avoid heap allocations for small sizes and fall back to malloc for large ones.
 *    - The `AllocatedBufferHolder_` RAII class ensures proper construction/destruction
 *      of non‑trivial types.
 *
 * 5. Floating‑Point & NaN Handling
 *    - NaNs are detected via `std::isnan`. When `nan_pos` is `at_begin` or `at_end`, a
 *      partitioning step (performed while computing the maximum key) moves all NaNs
 *      to the chosen side before sorting the non‑NaN elements. This preserves the
 *      relative order of NaNs (which are all considered equal by the sorting key).
 *    - Infinity values are handled correctly because their bit patterns are ordered
 *      after the sign‑flip transformation.
 *
 * 6. Performance Optimisations
 *    - Loop unrolling (configurable via `unroll_n` in `radix_traits`).
 *    - `RESTRICT_KEYWORD` (__restrict) to enable better alias analysis.
 *    - `ALWAYS_INLINE` for critical small functions.
 *    - Trivial type detection avoids unnecessary construction/destruction in buffers.
 *    - Early exit when only one non‑empty bucket exists.
 *    - MSD recursion uses a threshold (`chunk_size`) to switch to a multi‑pass LSD‑like
 *      fallback for small sub‑problems, reducing recursion overhead.
 *
 * 7. Reverse Iterator Support
 *    - When a `std::reverse_iterator` is detected, the `sort_order` is automatically
 *      flipped (asc ↔ desc) so that the sorting result matches the logical order of
 *      the reversed view.
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
 *   constexpr stdex::radix_traits traits {
 *      .sort_order = stdex::radix_traits::order_type::desc,
 *      .nan_pos    = stdex::radix_traits::nan_position::at_end,
 *   };
 *   stdex::radix_sort<traits>(std::execution::par, v.begin(), v.end());
 *
 * Explicitly use MSD (Most Significant Digit) for 64‑bit integers:
 *   constexpr stdex::radix_traits msd_traits {
 *      .sort_mode = stdex::radix_traits::mode_type::msd
 *   };
 *   stdex::radix_sort<msd_traits>(std::execution::par, v.begin(), v.end());
 *
 * Using a custom key projection (e.g., sort by absolute value):
 *   auto abs_key = [](double x) { return std::fabs(x); };
 *   stdex::radix_sort(std::execution::par_unseq, v.begin(), v.end(), abs_key);
 *
 * Sorting a reverse range (descending order can also be achieved via reverse iterators):
 *   stdex::radix_sort(v.rbegin(), v.rend());
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
 *   // Predicate that returns the sort key (by default the element itself)
 *   template<typename T>
 *   struct radix_default_key_predicate {
 *     constexpr T operator()(T val) const noexcept;
 *   };
 *
 *   struct radix_traits {
 *     enum class mode_type   { lsd, msd };
 *     enum class order_type  { asc, desc };
 *     enum class nan_position { unhandled, at_begin, at_end };
 *     enum class dataset_size_bound { fourbyte, eightbyte };
 *
 *     mode_type   sort_mode   = mode_type::lsd;       // LSD or MSD strategy
 *     order_type  sort_order  = order_type::asc;      // ascending or descending
 *     dataset_size_bound size_bound = dataset_size_bound::fourbyte // dataset size bound, 4byte(2^32 - 1) or 8byte(2^64 - 1)
 *     nan_position nan_pos    = nan_position::unhandled; // NaN handling
 *     uint32_t    bucket_size = 256;                 // 256 or 65536
 *     uint32_t    unroll_n    = 8;                   // loop unroll factor
 *     uint32_t    chunk_size  = 256 * 256;           // MSD → LSD fallback threshold
 *   };
 *
 *   // Overloads with explicit allocator
 *   template<radix_traits traits = radix_traits{},
 *            class ExecPolicy,
 *            RandomAccessIterator Iter,
 *            class Allocator,
 *            class KeyFunc = radix_default_key_predicate<std::iter_value_t<Iter>>>
 *   void radix_sort(ExecPolicy&& policy,
 *                   Iter first, Iter last,
 *                   const Allocator& alloc,
 *                   KeyFunc&& key_func = {});
 *
 *   // Overload without explicit allocator (uses std::allocator)
 *   template<radix_traits traits = radix_traits{},
 *            class ExecPolicy,
 *            RandomAccessIterator Iter,
 *            class KeyFunc = radix_default_key_predicate<std::iter_value_t<Iter>>>
 *   void radix_sort(ExecPolicy&& policy, Iter first, Iter last, KeyFunc&& key_func = {});
 *
 *   // Overloads for containers (random-access range)
 *   template<radix_traits traits = radix_traits{},
 *            class ExecPolicy, class Container, class KeyFunc = ...>
 *   void radix_sort(ExecPolicy&& policy, Container& c, KeyFunc&& key_func = {});
 *
 *   template<radix_traits traits = radix_traits{},
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
#include <cassert>
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
#pragma push_macro("hybrid_scoped_malloca")
#pragma push_macro("hybrid_scoped_freea")

#undef ALWAYS_INLINE
#undef RESTRICT_KEYWORD
#undef hybrid_scoped_malloca
#undef hybrid_scoped_freea

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

#if defined(_WIN32)

#  define hybrid_scoped_malloca(size) _malloca(size)
#  define hybrid_scoped_freea(ptr)    _freea(ptr)

#else /* Linux / macOS / Other */

// Refer to Microsoft's code implementation, see Windows <malloc.h>

#  define HYBRID_SCOPED_ALLOCA_THRESHOLD 1024

#  define HYBRID_SCOPED_ALLOCA_STACK_MARKER 0xCC'CC
#  define HYBRID_SCOPED_ALLOCA_HEAP_MARKER  0xDD'DD

#  if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
#    define HYBRID_SCOPED_ALLOCA_MARKER_SIZE 16
#  else
#    define HYBRID_SCOPED_ALLOCA_MARKER_SIZE 8
#  endif

inline void *
linn_hybrid_scoped_alloca_mark(void *ptr, unsigned int marker) noexcept
{
  if (ptr)
  {
    *((unsigned int *) ptr) = marker;
    ptr                     = (char *) ptr + HYBRID_SCOPED_ALLOCA_MARKER_SIZE;
  }
  return ptr;
}

inline size_t
linn_hybrid_scoped_alloca_compute_size(size_t size)
{
  size_t marked_size = size + HYBRID_SCOPED_ALLOCA_MARKER_SIZE;
  return marked_size > size ? marked_size : 0;
}

inline void
linn_hybrid_scoped_freea(void *ptr)
{
  if (!ptr)
  {
    return;
  }
  void        *base   = (char *) ptr - HYBRID_SCOPED_ALLOCA_MARKER_SIZE;
  unsigned int marker = *((unsigned int *) base);
  if (marker == HYBRID_SCOPED_ALLOCA_HEAP_MARKER)
  {
    free(base);
  }
#  ifdef assert
  else if (marker != HYBRID_SCOPED_ALLOCA_STACK_MARKER)
  {
    assert(("Corrupted pointer passed to _freea" && 0));
  }
#  endif //  assert
}
#  if defined(DEBUG) || defined(_DEBUG)
#    define hybrid_scoped_malloca(size)                                                                                \
      (::linn_hybrid_scoped_alloca_compute_size(size) != 0                                                             \
           ? ::linn_hybrid_scoped_alloca_mark(::malloc(::linn_hybrid_scoped_alloca_compute_size(size)),                \
                                              HYBRID_SCOPED_ALLOCA_HEAP_MARKER)                                        \
           : NULL)
#  else
#    define hybrid_scoped_malloca(size)                                                                                \
      (::linn_hybrid_scoped_alloca_compute_size(size) != 0                                                             \
           ? (((::linn_hybrid_scoped_alloca_compute_size(size) <= HYBRID_SCOPED_ALLOCA_THRESHOLD)                      \
                   ? ::linn_hybrid_scoped_alloca_mark(::alloca(::linn_hybrid_scoped_alloca_compute_size(size)),        \
                                                      HYBRID_SCOPED_ALLOCA_STACK_MARKER)                               \
                   : ::linn_hybrid_scoped_alloca_mark(::malloc(::linn_hybrid_scoped_alloca_compute_size(size)),        \
                                                      HYBRID_SCOPED_ALLOCA_HEAP_MARKER)))                              \
           : NULL)
#  endif // DEBUG) || defined(_DEBUG)

#  define hybrid_scoped_freea(ptr) linn_hybrid_scoped_freea(ptr)

#endif

namespace stdex
{
namespace details
{
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


template<typename Pred_, typename Iter_>
concept Invocable_key_predicate_ =
    std::indirect_unary_predicate<Pred_, Iter_> &&
    std::is_arithmetic_v<std::invoke_result_t<Pred_, typename std::iter_value_t<Iter_> &>>;

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
} // namespace details

template<typename Ty_>
struct radix_default_key_predicate
{
  constexpr Ty_ operator() (Ty_ Val_) const noexcept { return Val_; }
};

struct radix_traits
{
  enum class mode_type
  {
    lsd,
    msd
  } sort_mode { mode_type::lsd };

  enum class order_type
  {
    asc,
    desc
  } sort_order { order_type::asc };

  enum class dataset_size_bound
  {
    fourbyte  = 4,
    eightbyte = sizeof(size_t)
  } size_bound { dataset_size_bound::fourbyte };

  enum class nan_position
  {
    //< Temp Data: {INFINITY, -INFINITY, NAN, -0.0f, 1.0f / 1.0f, 0.0f, -1.0f / 1.0f,
    //         std::sqrt(-1.0f), 3.14f}

    unhandled, //< maybe output: -nan(ind) -inf -1 -0 0 1 3.14 inf nan
    at_begin,  //< maybe output: -nan(ind) nan -inf -1 -0 0 1 3.14 inf
    at_end     //< maybe output: -inf -1 -0 0 1 3.14 inf nan -nan(ind)
  } nan_pos { nan_position::unhandled };

  uint32_t bucket_size = 256U;
  uint32_t unroll_n    = 8U;

  //< msd recursive exit: fallback to lsd threshold
  uint32_t chunk_size = 256 * 256U;
};

namespace details
{
inline consteval radix_traits
Adjust_traits_order_(radix_traits Traits_, bool Is_reverse_) noexcept
{
  if (Is_reverse_)
  {
    Traits_.sort_order = Traits_.sort_order == radix_traits::order_type::asc ? radix_traits::order_type::desc
                                                                             : radix_traits::order_type::asc;
  }
  return Traits_;
}

inline consteval radix_traits
Change_traits_mode_(radix_traits Traits_, radix_traits::mode_type Mode_) noexcept
{
  Traits_.sort_mode = Mode_;
  return Traits_;
}

template<uint32_t Bucket_size_>
inline constexpr size_t Radix_bits_v_ = Bucket_size_ == 256U ? 8U : 16U;

template<uint32_t Bucket_size_, typename Key_ty_>
inline constexpr size_t Radix_count_v_ = sizeof(Key_ty_) * 8U / Radix_bits_v_<Bucket_size_>;

template<uint32_t Bucket_size_>
inline constexpr size_t
Highest_radix_index_(size_t Max_val_) noexcept
{
  return static_cast<size_t>(
      (std::bit_width(Max_val_) + Radix_bits_v_<Bucket_size_> - 1) / Radix_bits_v_<Bucket_size_> - 1);
}

template<bool Is_parallel_, uint32_t Loop_N_, typename Random_point_, typename Predicate_>
inline decltype(auto)
Max_element_(Random_point_ Begin_, size_t Size_, Predicate_ const &Pred_) noexcept
{
  using Key_ty_ = std::remove_cvref_t<std::invoke_result_t<Predicate_, std::remove_pointer_t<Random_point_>>>;

  Key_ty_ Max_val_ = 0;

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
        if (auto const Current_val_ = Pred_(Begin_[Index_]); Current_val_ > Local_val_)
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
      if (auto const Current_val_ = Pred_(Begin_[Index_]); Current_val_ > Max_val_)
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
      if (auto const Val_ = Pred_(Begin_[Index_]); Val_ > Max_val_)
      {
        Max_val_ = Val_;
      }
    });
#else
#  pragma omp simd reduction(max : Max_val_)
    for (size_t Index_ = 0; Index_ < Size_; ++Index_)
    {
      if (auto const Val_ = Pred_(Begin_[Index_]); Val_ > Max_val_)
      {
        Max_val_ = Val_;
      }
    }
#endif
  }

  return Max_val_;
}

template<Random_access_iterator_ Random_iter_, typename Predicate_>
inline decltype(auto)
Max_while_partition_by_nan_(Random_iter_      Begin_,
                            Random_iter_      End_,
                            Predicate_ const &Pred_,
                            bool              NaN_at_begin_) noexcept
{
  using Key_ty_ = std::remove_cvref_t<std::invoke_result_t<Predicate_, std::iter_value_t<Random_iter_>>>;

  static_assert(std::floating_point<Key_ty_>);

  struct Max_partition_result_
  {
    size_t  Part_index_;
    Key_ty_ Max_val_;
  };

  Key_ty_ Max_val_ = -std::numeric_limits<Key_ty_>::infinity();

  auto F_ = NaN_at_begin_ 
    ? [](Key_ty_ Val_) { return std::isnan(Val_); }
    : [](Key_ty_ Val_) { return !std::isnan(Val_); };

  auto First_ = Begin_, Last_ = End_;
  for (;;)
  {
    for (;;)
    {
      if (First_ == Last_)
      {
        return Max_partition_result_ { static_cast<size_t>(std::distance(Begin_, First_)),
                                       !std::isnan(*Begin_) || !std::isnan(*(End_ - 1)) ? Max_val_ : 0 };
      }

      if (!F_(Pred_(*First_)))
      {
        break;
      }

      // "NaN" compared with any value is always false
      if (auto const Curr_val_ = Pred_(*First_); Curr_val_ > Max_val_)
      {
        Max_val_ = Curr_val_;
      }

      ++First_;
    }

    do
    {
      --Last_;

      if (First_ == Last_)
      {
        if (auto const Curr_val_ = Pred_(*First_); Curr_val_ > Max_val_)
        {
          Max_val_ = Curr_val_;
        }
        return Max_partition_result_ { static_cast<size_t>(std::distance(Begin_, First_)),
                                       !std::isnan(*Begin_) || !std::isnan(*(End_ - 1)) ? Max_val_ : 0 };
      }
    }
    while (!F_(Pred_(*Last_)));

    if (auto const Curr_val_ = Pred_(*First_); Curr_val_ > Max_val_)
    {
      Max_val_ = Curr_val_;
    }

    std::iter_swap(First_, Last_);

    ++First_;
  }
}

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
  if constexpr (Unroll_size_ != 1)
  {
    for (; Index_ < End_; ++Index_)
    {
      Func_(Index_);
    }
  }
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

  AllocatedBufferHolder_(const AllocatedBufferHolder_ &)             = delete;
  AllocatedBufferHolder_ &operator= (const AllocatedBufferHolder_ &) = delete;

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

  typename std::allocator_traits<Allocator_>::pointer Get_buffer_() noexcept { return M_buffer_; }

private:
  size_t                                              M_size_;
  Allocator_                                          M_alloc_;
  typename std::allocator_traits<Allocator_>::pointer M_buffer_;
};

template<typename Elem_ty_>
class MallocaArrayHolder_
{
public:
  MallocaArrayHolder_(const MallocaArrayHolder_ &)             = delete;
  MallocaArrayHolder_ &operator= (const MallocaArrayHolder_ &) = delete;

  explicit MallocaArrayHolder_(void *MallocaRet_)
  {
    if (MallocaRet_ == nullptr)
    {
      throw ::std::bad_alloc();
    }
    M_elem_array_ = static_cast<Elem_ty_ *>(MallocaRet_);
  }

  ~MallocaArrayHolder_() { hybrid_scoped_freea(M_elem_array_); }

  Elem_ty_ *Get_raw_point_() noexcept { return M_elem_array_; }

private:
  Elem_ty_ *M_elem_array_;
};

template<typename Key_ty_>
struct Radix_projection_
{
  using Unsigned_ty_ = std::make_unsigned_t<
      std::conditional_t<std::is_floating_point_v<Key_ty_>,
                         std::conditional_t<std::same_as<Key_ty_, float>, std::uint32_t, std::uint64_t>,
                         Key_ty_>>;

  static constexpr std::uint8_t Shift_of_sign_bit_ = sizeof(Key_ty_) * 8 - 1;
  static constexpr Unsigned_ty_ All_bit_mask_      = ~Unsigned_ty_ { 0 };
  static constexpr Unsigned_ty_ Sign_bit_mask_     = All_bit_mask_ << Shift_of_sign_bit_;

  //< Does not support types like __int128, which are larger than size_t
  static_assert((sizeof(Unsigned_ty_) <= sizeof(size_t)), "Type size is bigger than size_t size.");

  static constexpr size_t Unsigned_integral_proj_(Key_ty_ Val_) noexcept { return Val_; }

  static constexpr size_t Signed_integral_proj_of_msd_(Key_ty_ Val_) noexcept
  {
    //< The default function needs to take the signed integer-like representation and map it to an unsigned one. The
    //< following code will take the midpoint of the unsigned representable range (SIZE_MAX/2)+1 and does an unsigned
    //< add of the value. Thus, it maps a [-signed_min,+signed_max] range into a [0, unsigned_max] range.

    return ((std::numeric_limits<size_t>::max() / 2) + 1) + static_cast<size_t>(Val_);
  }

  static constexpr size_t Signed_integral_proj_of_lsd_(Key_ty_ Val_) noexcept
  {
    //< Another strategy "return static_cast<Unsigned_ty_>(Val_) ^ Sign_bit_mask_;"
    //<
    //< if only LSD sorting is used, only the highest significant bit needs to be processed.
    //< Currently, a mixed strategy of MSD and LSD is adopted, using the above scheme applied to the function-Radix_key.

    return static_cast<Unsigned_ty_>(Val_) ^ Sign_bit_mask_;
  }

  static constexpr size_t Floating_point_proj_(Key_ty_ Val_) noexcept
  {
    Unsigned_ty_ const Unsigned_val_ = std::bit_cast<Unsigned_ty_>(Val_);

    return Unsigned_val_ ^ (Unsigned_ty_(0) - (Unsigned_val_ >> Shift_of_sign_bit_));
  }

  static constexpr size_t Floating_point_proj_highest_byte_(Key_ty_ Val_) noexcept
  {
    Unsigned_ty_ const Unsigned_val_ = std::bit_cast<Unsigned_ty_>(Val_);

    Unsigned_ty_ const Mask_ = (Unsigned_ty_(0) - (Unsigned_val_ >> Shift_of_sign_bit_));

    return (Unsigned_val_ ^ Mask_) | (Sign_bit_mask_ & ~Mask_);
  }
};

template<radix_traits Radix_traits_, typename Unsigned_ty_>
ALWAYS_INLINE size_t
Radix_key_(Unsigned_ty_ Val_, size_t Radix_) noexcept
{
  //< Mask (value 255 or 65535) and Shift (value 8 or 16) depends on the size of the bucket
  constexpr size_t Mask_      = Radix_traits_.bucket_size - 1U;
  constexpr size_t Shift_     = Radix_bits_v_<Radix_traits_.bucket_size>;
  constexpr size_t Max_radix_ = sizeof(Unsigned_ty_) * 8U / Shift_;
  assert(Radix_ < Max_radix_);

  if constexpr (Radix_traits_.sort_order == radix_traits::order_type::asc)
  {
    return (Val_ >> Shift_ * Radix_) & Mask_;
  }
  else /* desc */
  {
    return Mask_ - ((Val_ >> Shift_ * Radix_) & Mask_);
  }
}

template<bool Is_handle_highest_byte_, radix_traits Radix_traits_, typename Key_ty_>
ALWAYS_INLINE size_t
Compute_radix_key_idx_(size_t Radix_, Key_ty_ Key_val_) noexcept
{
  using Proj_ = Radix_projection_<Key_ty_>;

  if constexpr (std::floating_point<Key_ty_>)
  {
    if constexpr (Is_handle_highest_byte_)
    {
      return Radix_key_<Radix_traits_>(Proj_::Floating_point_proj_highest_byte_(Key_val_), Radix_);
    }
    else
    {
      return Radix_key_<Radix_traits_>(Proj_::Floating_point_proj_(Key_val_), Radix_);
    }
  }
  else if constexpr (std::signed_integral<Key_ty_>)
  {
    if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::lsd)
    {
      if constexpr (Is_handle_highest_byte_)
      {
        return Radix_key_<Radix_traits_>(Proj_::Signed_integral_proj_of_lsd_(Key_val_), Radix_);
      }
      else
      {
        return Radix_key_<Radix_traits_>(Proj_::Unsigned_integral_proj_(Key_val_), Radix_);
      }
    }
    else // msd
    {
      return Radix_key_<Radix_traits_>(Proj_::Signed_integral_proj_of_msd_(Key_val_), Radix_);
    }
  }
  else if constexpr (std::unsigned_integral<Key_ty_>)
  {
    return Radix_key_<Radix_traits_>(Proj_::Unsigned_integral_proj_(Key_val_), Radix_);
  }
}

template<bool Is_handle_highest_byte_, radix_traits Radix_traits_, typename Key_ty_>
ALWAYS_INLINE void
Radix_histogram_(size_t  Thread_id_,
                 size_t  Radix_,
                 Key_ty_ Key_val_,
                 std::conditional_t<Radix_traits_.size_bound == radix_traits::dataset_size_bound::fourbyte,
                                    uint32_t,
                                    uint64_t> (*const Chunks_)[Radix_traits_.bucket_size],
                 [[maybe_unused]] std::span<size_t> NaN_counts_ = {})
{
  if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::lsd && std::floating_point<Key_ty_> &&
                Is_handle_highest_byte_ && Radix_traits_.nan_pos != radix_traits::nan_position::unhandled)
  {
    if (std::isnan(Key_val_)) [[unlikely]]
    {
      ++NaN_counts_[Thread_id_];
      return;
    }
  }

  size_t Radix_key_idx_ = Compute_radix_key_idx_<Is_handle_highest_byte_, Radix_traits_>(Radix_, Key_val_);

  ++Chunks_[Thread_id_][Radix_key_idx_];
}

template<bool         Is_handle_highest_byte_,
         radix_traits Radix_traits_,
         typename Random_point_,
         typename Random_buffer_point_,
         typename Key_ty_>
ALWAYS_INLINE void
Radix_collection_(size_t  Thread_id_,
                  size_t  Radix_,
                  Key_ty_ Key_val_,
                  std::conditional_t<Radix_traits_.size_bound == radix_traits::dataset_size_bound::fourbyte,
                                     uint32_t,
                                     uint64_t> (*const Chunks_)[Radix_traits_.bucket_size],
                  Random_point_ const &RESTRICT_KEYWORD        Begin_,
                  size_t                                       Index_,
                  Random_buffer_point_ const &RESTRICT_KEYWORD Output_,
                  [[maybe_unused]] size_t                     &NaN_offset_ = 0)
{
  if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::lsd && std::floating_point<Key_ty_> &&
                Is_handle_highest_byte_ && Radix_traits_.nan_pos != radix_traits::nan_position::unhandled)
  {
    if (std::isnan(Key_val_)) [[unlikely]]
    {
      Output_[--NaN_offset_] = std::move(Begin_[Index_]);
      return;
    }
  }

  size_t Radix_key_idx_ = Compute_radix_key_idx_<Is_handle_highest_byte_, Radix_traits_>(Radix_, Key_val_);

  Output_[--Chunks_[Thread_id_][Radix_key_idx_]] = std::move(Begin_[Index_]);
}

struct Radix_parallel_data_
{
  size_t   Step_;
  uint32_t Remain_;
  uint32_t Threads_num_ { 1 };
};

ALWAYS_INLINE Radix_parallel_data_
Radix_make_parallel_data_(size_t Size_, uint32_t Threads_num_) noexcept
{
  return Radix_parallel_data_ { .Step_        = static_cast<uint32_t>(Size_ / Threads_num_),
                                .Remain_      = static_cast<uint32_t>(Size_ % Threads_num_),
                                .Threads_num_ = Threads_num_ };
}

ALWAYS_INLINE decltype(auto)
Compute_radix_task_indices_(size_t Thread_id_, size_t Step_, size_t Remain_) noexcept
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

  return std::pair { Beg_index_, End_index_ };
}

//< Declare
template<bool         Is_parallel_,
         radix_traits Radix_traits_,
         typename Random_point_,
         typename Random_buffer_point_,
         typename Predicate_>
inline void MSD_integer_radix_sort_(Random_point_ const &RESTRICT_KEYWORD        Begin_,
                                    size_t                                       Size_,
                                    Random_buffer_point_ const &RESTRICT_KEYWORD Output_,
                                    size_t                                       Radix_,
                                    Predicate_ const                            &Pred_,
                                    size_t                                       Deep_);

template<bool         Is_msd_fallback_,
         bool         Is_parallel_,
         bool         Is_handle_highest_byte_,
         radix_traits Radix_traits_,
         typename Random_point_,
         typename Random_buffer_point_,
         typename Predicate_>
ALWAYS_INLINE void
Radix_pass_(Radix_parallel_data_ Paral_data_,
            size_t               Radix_,
            std::conditional_t<Radix_traits_.size_bound == radix_traits::dataset_size_bound::fourbyte,
                               uint32_t,
                               uint64_t> (*const Chunks_)[Radix_traits_.bucket_size],
            size_t                                       Chunk_size_,
            Random_point_ const &RESTRICT_KEYWORD        Begin_,
            size_t                                       Size_,
            Random_buffer_point_ const &RESTRICT_KEYWORD Output_,
            Predicate_ const                            &Pred_,
            [[maybe_unused]] std::span<size_t>           NaN_counts_  = {},
            [[maybe_unused]] std::span<size_t>           NaN_offsets_ = {})
{
  ::memset(Chunks_, 0, Chunk_size_);

  using Key_ty_ = std::remove_cvref_t<std::invoke_result_t<Predicate_, std::remove_pointer_t<Random_point_>>>;

  if constexpr (Is_parallel_)
  {
#pragma omp parallel num_threads(Paral_data_.Threads_num_)
    {
      size_t Thread_id_ = omp_get_thread_num();

      auto [Beg_index_, End_index_] = Compute_radix_task_indices_(Thread_id_, Paral_data_.Step_, Paral_data_.Remain_);

      Unroll_loop_<Radix_traits_.unroll_n>(Beg_index_, End_index_, [&](size_t Index_)
      {
        Radix_histogram_<Is_handle_highest_byte_, Radix_traits_>(Thread_id_, Radix_, Pred_(Begin_[Index_]), Chunks_,
                                                                 NaN_counts_);
      });
    }
    {
      if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::lsd && std::floating_point<Key_ty_> &&
                    Is_handle_highest_byte_ && Radix_traits_.nan_pos != radix_traits::nan_position::unhandled)
      {
        size_t Prefix_NaN_count_ = 0;
        for (uint32_t Thread_id_ = 0; Thread_id_ < Paral_data_.Threads_num_; ++Thread_id_)
        {
          Prefix_NaN_count_ += NaN_counts_[Thread_id_];
          NaN_offsets_[Thread_id_] = Prefix_NaN_count_;
        }

        if constexpr (Radix_traits_.nan_pos == radix_traits::nan_position::at_begin)
        {
          Chunks_[0][0] += Prefix_NaN_count_;
        }
        else if constexpr (Radix_traits_.nan_pos == radix_traits::nan_position::at_end)
        {
          size_t NaN_base_ = Size_ - Prefix_NaN_count_;
          for (uint32_t Thread_id_ = 0; Thread_id_ < Paral_data_.Threads_num_; ++Thread_id_)
          {
            NaN_offsets_[Thread_id_] += NaN_base_;
          }
        }
      }

      size_t Non_empty_count_ = 0;
      for (size_t Local_thread_id_ = 1; Local_thread_id_ < Paral_data_.Threads_num_; ++Local_thread_id_)
      {
        Chunks_[Local_thread_id_][0] += Chunks_[Local_thread_id_ - 1][0];
      }
      Non_empty_count_ += (Chunks_[Paral_data_.Threads_num_ - 1][0] != 0);

      Unroll_loop_<Radix_traits_.unroll_n>(1, Radix_traits_.bucket_size, [&](size_t Index_)
      {
        size_t Last_ = Chunks_[Paral_data_.Threads_num_ - 1][Index_ - 1];
        Chunks_[0][Index_] += Last_;

        for (size_t Local_thread_id_ = 1; Local_thread_id_ < Paral_data_.Threads_num_; ++Local_thread_id_)
        {
          Chunks_[Local_thread_id_][Index_] += Chunks_[Local_thread_id_ - 1][Index_];
        }

        Non_empty_count_ += (Chunks_[Paral_data_.Threads_num_ - 1][Index_] - Last_ != 0);
      });


      if (Non_empty_count_ <= 1)
      {
        if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::msd)
        {
          details::MSD_integer_radix_sort_<Is_parallel_, Radix_traits_>(Begin_, Size_, Output_, Radix_ - 1, Pred_,
                                                                        NaN_counts_[0]); // NaN_counts_[0] as Deep_
        }
        return;
      }
    }
#pragma omp parallel num_threads(Paral_data_.Threads_num_)
    {
      size_t Thread_id_ = omp_get_thread_num();

      auto [Beg_index_, End_index_] = Compute_radix_task_indices_(Thread_id_, Paral_data_.Step_, Paral_data_.Remain_);

      size_t Thread_nan_offset_ = 0;
      if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::lsd && std::floating_point<Key_ty_> &&
                    Is_handle_highest_byte_ && Radix_traits_.nan_pos != radix_traits::nan_position::unhandled)
      {
        Thread_nan_offset_ = NaN_offsets_[Thread_id_];
      }
      Unroll_loop_<Radix_traits_.unroll_n>(Beg_index_, End_index_, [&](size_t Index_)
      {
        size_t Reverse_index_ = End_index_ - 1 - (Index_ - Beg_index_);
        Radix_collection_<Is_handle_highest_byte_, Radix_traits_>(Thread_id_, Radix_, Pred_(Begin_[Reverse_index_]),
                                                                  Chunks_, Begin_, Reverse_index_, Output_,
                                                                  Thread_nan_offset_);
      });
    }

    if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::msd)
    {
      for (uint32_t Index_ = 0; Index_ < Radix_traits_.bucket_size; ++Index_)
      {
        size_t Chunk_begin_ = Chunks_[0][Index_];
        size_t Chunk_end_   = Index_ < Radix_traits_.bucket_size - 1 ? Chunks_[0][Index_ + 1] : Size_;
        size_t Chunk_size_  = Chunk_end_ - Chunk_begin_;
        if (Chunk_size_ == 0)
        {
          continue;
        }

        details::MSD_integer_radix_sort_<Is_parallel_, Radix_traits_>(
            Output_ + Chunk_begin_, Chunk_size_, Begin_ + Chunk_begin_, Radix_ - 1, Pred_, NaN_counts_[0] + 1);
      }
    }
  }
  else
  {
    Unroll_loop_<Radix_traits_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      Radix_histogram_<Is_handle_highest_byte_, Radix_traits_>(0, Radix_, Pred_(Begin_[Index_]), Chunks_, NaN_counts_);
    });

    if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::lsd && std::floating_point<Key_ty_> &&
                  Is_handle_highest_byte_ && Radix_traits_.nan_pos != radix_traits::nan_position::unhandled)
    {
      size_t Prefix_NaN_count_ = NaN_counts_[0];
      NaN_offsets_[0]          = Prefix_NaN_count_;

      if constexpr (Radix_traits_.nan_pos == radix_traits::nan_position::at_begin)
      {
        Chunks_[0][0] += NaN_counts_[0];
      }
      else if constexpr (Radix_traits_.nan_pos == radix_traits::nan_position::at_end)
      {
        NaN_offsets_[0] += (Size_ - Prefix_NaN_count_);
      }
    }

    if constexpr (!Is_msd_fallback_)
    {
      size_t Non_empty_count_ = 0, Prev_sum_ = 0;

      Unroll_loop_<Radix_traits_.unroll_n>(0, Radix_traits_.bucket_size, [&](size_t Index_)
      {
        size_t Original_val_ = Chunks_[0][Index_];
        Non_empty_count_ += (Original_val_ != 0);

        Chunks_[0][Index_] += Prev_sum_;
        Prev_sum_ = Chunks_[0][Index_];
      });

      if (Non_empty_count_ <= 1)
      {
        if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::msd)
        {
          details::MSD_integer_radix_sort_<Is_parallel_, Radix_traits_>(Begin_, Size_, Output_, Radix_ - 1, Pred_,
                                                                        NaN_counts_[0]); // NaN_counts_[0] as Deep_
        }
        return;
      }
    }
    else
    {
      size_t Prev_sum_ = 0;
      Unroll_loop_<Radix_traits_.unroll_n>(0, Radix_traits_.bucket_size, [&](size_t Index_)
      {
        Chunks_[0][Index_] += Prev_sum_;
        Prev_sum_ = Chunks_[0][Index_];
      });
    }

    size_t Local_nan_offset_ = 0;
    if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::lsd && std::floating_point<Key_ty_> &&
                  Is_handle_highest_byte_ && Radix_traits_.nan_pos != radix_traits::nan_position::unhandled)
    {
      Local_nan_offset_ = NaN_offsets_[0];
    }

    Unroll_loop_<Radix_traits_.unroll_n>(0, Size_, [&](size_t Index_)
    {
      Radix_collection_<Is_handle_highest_byte_, Radix_traits_>(0, Radix_, Pred_(Begin_[Size_ - 1 - Index_]), Chunks_,
                                                                Begin_, Size_ - 1 - Index_, Output_, Local_nan_offset_);
    });

    if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::msd && !Is_msd_fallback_)
    {
      Unroll_loop_<Radix_traits_.unroll_n>(0, Radix_traits_.bucket_size - 1, [&](size_t Index_)
      {
        details::MSD_integer_radix_sort_<Is_parallel_, Radix_traits_>(
            Output_ + Chunks_[0][Index_], Chunks_[0][Index_ + 1] - Chunks_[0][Index_], Begin_ + Chunks_[0][Index_],
            Radix_ - 1, Pred_, NaN_counts_[0] + 1);
      });
      details::MSD_integer_radix_sort_<Is_parallel_, Radix_traits_>(
          Output_ + Chunks_[0][Radix_traits_.bucket_size - 1], Size_ - Chunks_[0][Radix_traits_.bucket_size - 1],
          Begin_ + Chunks_[0][Radix_traits_.bucket_size - 1], Radix_ - 1, Pred_, NaN_counts_[0] + 1);
    }
  }
}

template<bool         Is_msd_fallback_,
         bool         Is_parallel_,
         radix_traits Radix_traits_,
         typename Random_point_,
         typename Random_buffer_point_,
         typename Predicate_>
inline void
LSD_integer_radix_sort_(Random_point_ const &RESTRICT_KEYWORD        Begin_,
                        size_t                                       Size_,
                        Random_buffer_point_ const &RESTRICT_KEYWORD Output_,
                        size_t                                       Radix_,
                        Predicate_ const                            &Pred_,
                        size_t                                       Deep_ = 0)
{
  if (Size_ == 0) [[unlikely]]
  {
    return;
  }

  using Size_ty_ =
      std::conditional_t<Radix_traits_.size_bound == radix_traits::dataset_size_bound::fourbyte, uint32_t, uint64_t>;

  using Chunk_ptr_ty_ = Size_ty_(*)[Radix_traits_.bucket_size];

  int          Threads_num_ = !Is_parallel_ ? 1 : omp_get_max_threads();
  size_t const Chunk_size_ =
      Radix_traits_.bucket_size * Threads_num_ *
      static_cast<std::underlying_type_t<radix_traits::dataset_size_bound>>(Radix_traits_.size_bound);
  auto const Paral_data_ = Radix_make_parallel_data_(Size_, Threads_num_);

  MallocaArrayHolder_<Size_ty_> Holder_(hybrid_scoped_malloca(Chunk_size_));
  auto *RESTRICT_KEYWORD        Chunks_ = reinterpret_cast<Chunk_ptr_ty_>(Holder_.Get_raw_point_());

  size_t Curr_radix_ = 0;

  while (Curr_radix_ < Radix_)
  {
    Radix_pass_<Is_msd_fallback_, Is_parallel_, Is_msd_fallback_, Radix_traits_>(
        Paral_data_, Curr_radix_++, Chunks_, Chunk_size_, Begin_, Size_, Output_, Pred_);

    if constexpr (Is_msd_fallback_)
    {
      Radix_pass_<Is_msd_fallback_, Is_parallel_, Is_msd_fallback_, Radix_traits_>(
          Paral_data_, Curr_radix_++, Chunks_, Chunk_size_, Output_, Size_, Begin_, Pred_);
    }
    else
    {
      if (Curr_radix_ != Radix_)
      {
        Radix_pass_<Is_msd_fallback_, Is_parallel_, Is_msd_fallback_, Radix_traits_>(
            Paral_data_, Curr_radix_++, Chunks_, Chunk_size_, Output_, Size_, Begin_, Pred_);
      }
    }
  }

  if (Curr_radix_ == Radix_)
  {
    if constexpr (Is_msd_fallback_)
    {
      Radix_pass_<true, Is_parallel_, true, Radix_traits_>(Paral_data_, Curr_radix_++, Chunks_, Chunk_size_, Begin_,
                                                           Size_, Output_, Pred_);
    }
    else
    {
      using Key_ty_ = std::remove_cvref_t<std::invoke_result_t<Predicate_, std::remove_pointer_t<Random_point_>>>;

      if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::lsd && std::floating_point<Key_ty_> &&
                    Radix_traits_.nan_pos != radix_traits::nan_position::unhandled)
      {
        size_t *NaN_counts_  = static_cast<size_t *>(alloca(Threads_num_ * sizeof(size_t)));
        size_t *NaN_offsets_ = static_cast<size_t *>(alloca(Threads_num_ * sizeof(size_t)));
        ::memset(NaN_counts_, 0, Threads_num_ * sizeof(size_t));
        ::memset(NaN_offsets_, 0, Threads_num_ * sizeof(size_t));

        std::span<size_t> NaN_counts_span_(NaN_counts_, Threads_num_);
        std::span<size_t> NaN_offsets_span_(NaN_offsets_, Threads_num_);

        Radix_pass_<false, Is_parallel_, true, Radix_traits_>(Paral_data_, Radix_, Chunks_, Chunk_size_, Output_, Size_,
                                                              Begin_, Pred_, NaN_counts_span_, NaN_offsets_span_);
      }
      else
      {
        if constexpr (sizeof(Key_ty_) == 1)
        {
          Radix_pass_<false, Is_parallel_, true, Radix_traits_>(Paral_data_, Radix_, Chunks_, Chunk_size_, Begin_,
                                                                Size_, Output_, Pred_);
        }
        else
        {
          Radix_pass_<false, Is_parallel_, true, Radix_traits_>(Paral_data_, Radix_, Chunks_, Chunk_size_, Output_,
                                                                Size_, Begin_, Pred_);
        }
      }
    }
  }

  if (Deep_ + Radix_ + 1 & 1)
  {
    if (Radix_ + 1 & 1)
    {
      std::move(Output_, Output_ + Size_, Begin_);
    }
    else
    {
      std::move(Begin_, Begin_ + Size_, Output_);
    }
  }
}

template<bool         Is_parallel_,
         radix_traits Radix_traits_,
         typename Random_point_,
         typename Random_buffer_point_,
         typename Predicate_>
inline void
MSD_integer_radix_sort_(Random_point_ const &RESTRICT_KEYWORD        Begin_,
                        size_t                                       Size_,
                        Random_buffer_point_ const &RESTRICT_KEYWORD Output_,
                        size_t                                       Radix_,
                        Predicate_ const                            &Pred_,
                        size_t                                       Deep_ = 0)
{
  if (Size_ <= Radix_traits_.chunk_size || Radix_ < 1)
  {
    return LSD_integer_radix_sort_<true, false, Radix_traits_>(Begin_, Size_, Output_, Radix_, Pred_, Deep_);
  }

  int Threads_num_ = 1;
  if constexpr (Is_parallel_)
  {
    Threads_num_ = omp_get_max_threads();
  }

  using Size_ty_ =
      std::conditional_t<Radix_traits_.size_bound == radix_traits::dataset_size_bound::fourbyte, uint32_t, uint64_t>;

  using Chunk_ptr_ty_ = Size_ty_(*)[Radix_traits_.bucket_size];

  size_t const Chunk_size_ =
      Radix_traits_.bucket_size * Threads_num_ *
      static_cast<std::underlying_type_t<radix_traits::dataset_size_bound>>(Radix_traits_.size_bound);
  auto const Paral_data_ = Radix_make_parallel_data_(Size_, Threads_num_);

  MallocaArrayHolder_<Size_ty_> Holder_(hybrid_scoped_malloca(Chunk_size_));

  auto *RESTRICT_KEYWORD Chunks_ = reinterpret_cast<Chunk_ptr_ty_>(Holder_.Get_raw_point_());

  std::span<size_t> Deep_span_(&Deep_, 1);

  Radix_pass_<false, Is_parallel_, true, Radix_traits_>(Paral_data_, Radix_, Chunks_, Chunk_size_, Begin_, Size_,
                                                        Output_, Pred_, Deep_span_);
}
} // namespace details

template<radix_traits                         Radix_traits_ = radix_traits {},
         details::Supported_execution_policy_ Exec_policy_,
         details::Random_access_iterator_     Random_iter_,
         details::Standard_allocator_         Allocator_,
         typename Key_predicate_ = radix_default_key_predicate<std::iter_value_t<Random_iter_>>>
inline void radix_sort(Exec_policy_    &&Exec_po_,
                       Random_iter_      Begin_,
                       Random_iter_      End_,
                       Allocator_ const &Alloc_,
                       Key_predicate_  &&Pred_ = {})
{
  using Key_ty_ = std::remove_cvref_t<std::invoke_result_t<Key_predicate_, std::iter_value_t<Random_iter_>>>;

  static_assert(Radix_traits_.bucket_size == 256U || Radix_traits_.bucket_size == 65536U,
                "Bucket size must be 256 or 65536!");

  static_assert(details::Invocable_key_predicate_<Key_predicate_, Random_iter_>,
                "Key_predicate_ must be invocable with iterator's value_type& and return arithmetic type!");

  static_assert(!(Radix_traits_.bucket_size == 65536U && sizeof(Key_ty_) == 1),
                "Do not set a two-byte buffer size for one-byte data!");

  size_t Size_ = End_ - Begin_;

  if (Size_ <= 1) [[unlikely]]
  {
    return;
  }

  auto *RESTRICT_KEYWORD Primary_addr_ = &*Begin_;
  if constexpr (details::Reverse_iterator_<Random_iter_>)
  {
    Primary_addr_ = &*End_.base();
  }
  constexpr auto Traits_ = details::Adjust_traits_order_(Radix_traits_, details::Is_reverse_iterator_v_<Random_iter_>);

  size_t Range_left_ = 0, Range_right_ = Size_;
  size_t Max_val_ = 0;

  using Proj_ = details::Radix_projection_<Key_ty_>;
  if constexpr (Traits_.sort_mode == radix_traits::mode_type::msd)
  {
    if constexpr ((std::floating_point<Key_ty_> && Traits_.nan_pos == radix_traits::nan_position::unhandled) ||
                  std::integral<Key_ty_>)
    {
      if constexpr (std::signed_integral<Key_ty_>)
      {
        Max_val_ = details::Max_element_<details::Is_parallel_policy_v_<Exec_policy_>, Traits_.unroll_n>(
            Primary_addr_, Size_, [&Pred_](auto const &Val_)
        {
          return Proj_::Signed_integral_proj_of_msd_(Pred_(Val_));
        });
      }
      else
      {
        Max_val_ = details::Max_element_<details::Is_parallel_policy_v_<Exec_policy_>, Traits_.unroll_n>(Primary_addr_,
                                                                                                         Size_, Pred_);
      }
    }
    /*floating_point handle NaN*/
    else if (Traits_.nan_pos == radix_traits::nan_position::at_begin)
    {
      auto Max_with_part_ = details::Max_while_partition_by_nan_(Primary_addr_, Primary_addr_ + Size_, Pred_, true);

      Range_left_ = Max_with_part_.Part_index_;
      Max_val_    = Max_with_part_.Max_val_;
    }
    else if (Traits_.nan_pos == radix_traits::nan_position::at_end)
    {
      auto Max_with_part_ = details::Max_while_partition_by_nan_(Primary_addr_, Primary_addr_ + Size_, Pred_, false);

      Range_right_ = Max_with_part_.Part_index_;
      Max_val_     = Max_with_part_.Max_val_;
    }
    // If all elements are NaN or Zero, then there is no need to sort
    if (Max_val_ == 0) [[unlikely]]
    {
      return;
    }

    Primary_addr_ = Primary_addr_ + Range_left_;
  }

  details::AllocatedBufferHolder_<Allocator_> Holder_(Range_right_ - Range_left_, Alloc_);
  auto *RESTRICT_KEYWORD                      Buffer_ = Holder_.Get_buffer_();

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

  if constexpr (Radix_traits_.sort_mode == radix_traits::mode_type::msd)
  {
    // Calculate highest radix digit index
    Max_val_ = details::Highest_radix_index_<Radix_traits_.bucket_size>(Max_val_);

    details::MSD_integer_radix_sort_<details::Is_parallel_policy_v_<Exec_policy_>, Radix_traits_>(
        Primary_addr_, Sort_size_, Buffer_, Max_val_, Pred_);
  }
  else
  {
    Max_val_ = details::Radix_count_v_<Radix_traits_.bucket_size, Key_ty_>;

    details::LSD_integer_radix_sort_<false, details::Is_parallel_policy_v_<Exec_policy_>, Radix_traits_>(
        Primary_addr_, Sort_size_, Buffer_, Max_val_ - 1, Pred_);
  }
}

template<radix_traits Radix_traits_ = radix_traits {},
         typename Exec_policy_,
         typename Random_iter_,
         typename Key_predicate_ = radix_default_key_predicate<std::iter_value_t<Random_iter_>>>
requires details::Supported_execution_policy_<Exec_policy_> && details::Random_access_iterator_<Random_iter_>
inline void radix_sort(Exec_policy_ &&Expo_, Random_iter_ Begin_, Random_iter_ End_, Key_predicate_ &&Pred_ = {})
{
  using Value_type_ = typename std::iter_value_t<Random_iter_>;
  radix_sort<Radix_traits_>(std::forward<Exec_policy_>(Expo_), Begin_, End_, std::allocator<Value_type_> {},
                            std::forward<Key_predicate_>(Pred_));
}

template<radix_traits Radix_traits_ = radix_traits {},
         typename Random_iter_,
         typename Key_predicate_ = radix_default_key_predicate<std::iter_value_t<Random_iter_>>>
requires details::Random_access_iterator_<Random_iter_>
inline void radix_sort(Random_iter_ Begin_, Random_iter_ End_, Key_predicate_ &&Pred_ = {})
{
  radix_sort<Radix_traits_>(std::execution::seq, Begin_, End_, std::forward<Key_predicate_>(Pred_));
}

template<radix_traits Radix_traits_ = radix_traits {},
         typename Exec_policy_,
         typename Container_,
         typename Key_predicate_ = radix_default_key_predicate<typename Container_::value_type>>
requires details::Supported_execution_policy_<Exec_policy_> && details::Random_access_range_<Container_>
inline void radix_sort(Exec_policy_ &&Expo_, Container_ &C_, Key_predicate_ &&Pred_ = {})
{
  radix_sort<Radix_traits_>(std::forward<Exec_policy_>(Expo_), std::begin(C_), std::end(C_),
                            std::forward<Key_predicate_>(Pred_));
}

template<radix_traits Radix_traits_ = radix_traits {},
         typename Container_,
         typename Key_predicate_ = radix_default_key_predicate<typename Container_::value_type>>
requires details::Random_access_range_<Container_>
inline void radix_sort(Container_ &C_, Key_predicate_ &&Pred_ = {})
{
  radix_sort<Radix_traits_>(std::execution::seq, C_, std::forward<Key_predicate_>(Pred_));
}

} // namespace stdex

#pragma pop_macro("hybrid_scoped_freea")
#pragma pop_macro("hybrid_scoped_malloca")
#pragma pop_macro("RESTRICT_KEYWORD")
#pragma pop_macro("ALWAYS_INLINE")
