/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <bit>
#include <memory>
#include <type_traits>

#include <folly/ConstexprMath.h>
#include <folly/FollyMemset.h>
#include <folly/Traits.h>
#include <folly/Utility.h>
#include <folly/algorithm/simd/MaxElement.h>
#include <folly/container/Iterator.h>
#include <folly/container/MallocaArray.h>
#include <folly/lang/Assume.h>
#include <folly/memory/AllocatedBufferHolder.h>
#ifdef _OPENMP
#include <omp.h>
#endif // _OPENMP
/*
static size_t prefixSum_simd(hist_t* hist) {
    constexpr size_t kLane = 8;
    constexpr size_t kNumVec = kNumBuckets / kLane;
    __m256i carry = _mm256_setzero_si256();
    uint32_t nonEmptyCount = 0;
    for (size_t vi = 0; vi < kNumVec; ++vi) {
        size_t offset = vi * kLane;
        __m256i v = _mm256_loadu_si256((__m256i*)(hist + offset));
        __m256i zero = _mm256_setzero_si256();
        int mask = _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v, zero)));
        nonEmptyCount += __builtin_popcount((~mask) & 0xFF);
        v = _mm256_add_epi32(v, _mm256_slli_si256(v, 4));
        v = _mm256_add_epi32(v, _mm256_slli_si256(v, 8));
        v = _mm256_add_epi32(v, _mm256_slli_si256(v, 16));
        v = _mm256_add_epi32(v, carry);
        uint32_t last = _mm256_extract_epi32(v, 7);
        carry = _mm256_set1_epi32(last);
        _mm256_storeu_si256((__m256i*)(hist + offset), v);
    }
    return nonEmptyCount;
}
*/
namespace folly {

/**
 * @brief Execution policy for radix sort.
 *
 * @details
 * Controls whether the sort runs sequentially or uses parallel execution
 *
 * @note Parallel execution is only available when _OPENMP is defined.
 */
enum class RadixExecutionPolicy {
  Seq, // Sequential execution
  Par, // Parallel execution (requires OpenMP)
  /* Unseq,    */
  /* ParUnseq, */
};

/**
 * @brief Radix sort traversal strategy.
 *
 * @details
 * - Lsd: Least Significant Digit first. Processes digits from least to most
 *   significant byte. Stable, iterative, and uses a single flat array.
 *
 * - Msd: Most Significant Digit first. Recursively partitions by the most
 *   significant digit. Forms a tree of sub‑problems.
 *
 * # Memory layout & algorithm flow
 *
 * ## LSD (iterative, multi‑pass)
 *
 *   Pass 1 (LSB)     Pass 2          Pass 3         Pass 4 (MSB)
 *   ┌─────────┐     ┌─────────┐     ┌─────────┐     ┌─────────┐
 *   │  array  │ ──► │  buffer │ ──► │  array  │ ──► │  buffer │ ──► sorted
 *   └─────────┘     └─────────┘     └─────────┘     └─────────┘
 *
 *   • Works on the whole array every pass.
 *   • Each pass scatters elements into 256 (or 65536) buckets.
 *   • No recursion, very good cache locality.
 *
 * ## MSD (recursive, depth‑first)
 *
 *   Level 0 (byte 7)      Level 1 (byte 6)          Level 2 (byte 5) ...
 *   ┌─────────────────┐
 *   │   whole array   │
 *   └────────┬────────┘
 *            │ split by most significant byte
 *        ┌───┴───┐
 *        ▼       ▼
 *   ┌───────┐ ┌───────┐
 *   │bucket0│ │bucket1│ ... (up to 256 buckets)
 *   └───┬───┘ └───┬───┘
 *       │         │ recursive sort on next byte
 *   ┌───┴───┐ ┌───┴───┐
 *   ▼       ▼ ▼       ▼
 *  ...     ...       ...
 *
 *   • High‑order bits decide global order early.
 *   • Once a bucket contains ≤ threshold, fallback to LSD or insertion sort.
 *
 * # Performance considerations
 *
 * ## 32‑bit keys (e.g., `uint32_t`)
 *
 *   - LSD with 8‑bit chunks: only 4 passes, each pass scans the whole array.
 *   - Very predictable memory access, minimal recursion overhead.
 *   - Usually faster than MSD on modern CPUs (better cache and SIMD friendly).
 *
 * ## 64‑bit keys (e.g., `uint64_t`)
 *
 *   - LSD with 8‑bit chunks needs 8 passes → more memory writes.
 *   - LSD with 16‑bit chunks reduces passes to 4 but uses 65536 buckets,
 *     increasing histogram memory traffic.
 *   - MSD can be faster because the highest bytes often distribute data widely.
 *     After the first pass, sub‑problems become much smaller, reducing total
 *     work. Recursion overhead is amortized over large inputs.
 *
 * ## Rule of thumb
 *
 *   - For 32‑bit integers or small element types, prefer `Lsd`.
 *   - For 64‑bit integers, especially with skewed distributions or when
 *     the most significant byte has high entropy, `Msd` may outperform `Lsd`.
 *   - Always benchmark with your actual data.
 */
enum class RadixSortStrategy { Lsd, Msd };

/**
 * @brief Sort order for radix sort.
 */
enum class RadixSortOrder { Ascending, Descending };

/**
 * @brief Number of bits processed per radix pass.
 *
 * @details
 * Determines the bucket count as 2^ChunkBits. Using Bits8 yields 256 buckets
 * and 4 passes for 32-bit keys. Using Bits16 yields 65536 buckets and 2
 * passes. Bits16 can be faster for large arrays because it halves the number
 * of passes, but the larger histogram incurs more memory traffic.
 */
enum class RadixChunkBits { Bits8 = 8, Bits16 = 16 };

/**
 * @brief Integer type used for bucket counters in the histogram.
 *
 * @details
 * UInt32 is sufficient for arrays with fewer than 2^32 elements and uses
 * less memory. UInt64 avoids overflow for larger arrays but doubles the
 * memory footprint of the counter arrays.
 */
enum class RadixBucketCounterType { UInt32 = 32, UInt64 = 64 };

/**
 * @brief Controls how NaN values are sorted when sorting floating-point types.
 *
 * @warning NaN handling requires IEEE 754 compliance
 * (std::numeric_limits::is_iec559) to be guaranteed at compile time.
 *
 * - Unhandled: No special treatment. NaN values may produce unexpected
 *   ordering because IEEE 754 does not define a total order for NaN.
 * - AtFirst: All NaN values are placed at the very beginning of the
 *   sorted output, before the smallest finite value (-inf).
 * - AtLast: All NaN values are placed at the end, after +inf.
 */
enum class RadixNaNsPosHandling { Unhandled, AtFirst, AtLast };

/**
 * @brief Assumption about the presence of NaN values in floating-point arrays.
 *
 * @details
 * Allows the developer to inform the sort whether the input array may contain
 * NaN. The default is `NonExistence`, which gives the best performance by
 * skipping NaN handling. When `xistence` is selected, the
 * `RadixNaNsPosHandling` setting is ignored and the sort behaves as if
 * `Unhandled` were specified.
 *
 * @note This enum only applies to floating-point types (float, double, etc.).
 */
enum class RadixNaNsAssumption {
  Existence, ///< NaN may be present; use NaNsPosHandling to control ordering.
  NonExistence ///< Assume no NaN values; NaNsPosHandling is ignored.
};

/**
 * @brief Optimizing the highest bit projection for signed types when there are
 *        no negative numbers.
 *
 * @details
 * For signed integers (int) and floating-point types (float, double), radix
 * sort normally flips the most significant bit (MSB) to map negative values
 * before positive ones. For unsigned types (uint32, uint64), this step is
 * naturally skipped, yielding the highest performance.
 *
 * This enum allows users who are using signed types but can **guarantee** that
 * all input values are non-negative (>= 0) to explicitly skip this MSB flip.
 * The sort then treats the signed data as if it were unsigned, achieving
 * the same optimal performance as sorting uint32/uint64.
 *
 * @warning Using `NonExistence` when the input contains any negative
 *          number will produce **incorrect sorting order**.
 *          Only enable this if the guarantee is 100% certain.
 */
enum class RadixNegativeAssumption {
  /**
   * @brief Standard behavior. Has negative, always applies the MSB flip.
   *        Safe for all signed inputs, including negatives.
   */
  Existence = 0,

  /**
   * @brief Optimized path. Non negative, can skip the MSB flip entirely.
   *        Valid ONLY when the user guarantees NO negative values exist.
   *        Equivalent to treating signed types as unsigned for sorting.
   */
  NonExistence = 1
};

/**
 * @brief Complete set of compile-time options for radix sort.
 *
 * @details
 * Customize sorting behavior by constructing an instance with the desired
 * settings and passing it as a template parameter to radixSort().
 *
 * Available tuning knobs:
 *  - MsdFallbackToLsdThreshold: when MSD sub-problems fall below this
 *    size at some recursion depth, the implementation falls back to LSD
 *    for the remaining sub-array to avoid excessive recursion overhead.
 *  - MaxStackBytes: maximum bytes to allocate on the stack for temporary
 *    counter arrays. Exceeding this threshold forces heap allocation.
 */
struct RadixSortOptions {
  RadixExecutionPolicy ExecutionPolicy = RadixExecutionPolicy::Seq;
  RadixSortStrategy SortStrategy = RadixSortStrategy::Lsd;
  RadixSortOrder SortOrder = RadixSortOrder::Ascending;
  RadixChunkBits ChunkBits = RadixChunkBits::Bits8;
  RadixBucketCounterType BucketCounterType = RadixBucketCounterType::UInt32;
  RadixNaNsPosHandling NaNsPosHandling = RadixNaNsPosHandling::Unhandled;
  RadixNaNsAssumption NaNsAssumption = RadixNaNsAssumption::NonExistence;
  RadixNegativeAssumption NegativeAssumption =
      RadixNegativeAssumption::Existence;

  size_t MsdFallbackToLsdThreshold = 256U * 256U;
  size_t MaxStackBytes = 16384U;
};

namespace detail {

// ---------------------------------------------------------------------------
// C++20 concept definitions
// ---------------------------------------------------------------------------

/**
 * @brief A KeyMap must be a callable that returns an arithmetic type.
 *
 * This is the user-facing mapping function that lets you sort by a
 * computed or extracted key.
 *
 * @tparam KeyMap  Callable type
 * @tparam T       Input value type
 */
template <typename KeyMap, typename T>
concept arithmetic_key_map = std::regular_invocable<KeyMap, T> &&
    std::is_arithmetic_v<std::invoke_result_t<KeyMap, T>>;

/**
 * @brief A Projection must map a value to an unsigned integral key.
 *
 * The projection must produce an unsigned integer because the radix sort
 * operates on raw bit patterns.
 *
 * @tparam Projection  Callable type
 * @tparam T           Input value type
 */
template <typename Projection, typename T>
concept unsigned_integral_projection = requires(Projection p, T t) {
  requires std::regular_invocable<Projection, T>;
  { p(t) } -> std::unsigned_integral;
};

// ---------------------------------------------------------------------------
// Reverse-iterator order correction
// ---------------------------------------------------------------------------

/**
 * @brief Adjusts sort order for reverse iterators at compile time.
 *
 * When the caller passes a reverse_iterator, ascending and descending swap
 * meanings: sorting a reversed range in ascending order is equivalent to
 * sorting the original range in descending order. This trait performs that
 * adjustment at compile time.
 *
 * @tparam Iter  Iterator type (or std::reverse_iterator<WrappedIter>)
 * @tparam Order User-requested sort order (Ascending or Descending)
 */
template <typename Iter, RadixSortOrder Order>
struct RadixRealSortOrder {
  static constexpr RadixSortOrder value = Order;
};

/**
 * @brief Specialisation for reverse iterators — inverts the sort order.
 *
 * Sorting a reversed range in ascending order produces the same output
 * as sorting the original range in descending order, and vice versa.
 *
 * @tparam WrappedIter  Underlying iterator wrapped by reverse_iterator
 * @tparam Order        User-requested sort order
 */
template <typename WrappedIter, RadixSortOrder Order>
struct RadixRealSortOrder<std::reverse_iterator<WrappedIter>, Order> {
  static constexpr RadixSortOrder value = (Order == RadixSortOrder::Ascending)
      ? RadixSortOrder::Descending
      : RadixSortOrder::Ascending;
};

// ---------------------------------------------------------------------------
// Counter type selection
// ---------------------------------------------------------------------------

/**
 * @brief Selects the unsigned integral counter type from a
 * RadixBucketCounterType enum value.
 *
 * @tparam CounterType  One of RadixBucketCounterType::UInt32 or UInt64
 */
template <RadixBucketCounterType CounterType>
struct CounterTypeSelector;

/** @brief 32-bit bucket counters (sufficient for n < 2^32). */
template <>
struct CounterTypeSelector<RadixBucketCounterType::UInt32> {
  using type = std::uint32_t;
};

/** @brief 64-bit bucket counters (avoids overflow for n >= 2^32). */
template <>
struct CounterTypeSelector<RadixBucketCounterType::UInt64> {
  using type = std::uint64_t;
};

// ---------------------------------------------------------------------------
// Utilities for unsigned-type mapping
// ---------------------------------------------------------------------------

/**
 * @brief Identity key map — returns the input value unchanged.
 *
 * Used as the default KeyMap in the public API so that sorting
 * containers of arithmetic types requires no projection argument.
 *
 * @struct folly::detail::radix_key_map_fn
 */
struct radix_key_map_fn {
  template <typename T>
  constexpr T operator()(T val) const noexcept {
    return val;
  }
};
inline constexpr radix_key_map_fn radix_key_map{};

// ---------------------------------------------------------------------------
// Sign-bit traits
// ---------------------------------------------------------------------------

/**
 * @brief Extracts the sign-bit properties of an arithmetic type.
 *
 * @details
 * Signed integers and IEEE 754 floats use the most significant bit as a
 * sign indicator. Naive reinterpretation to an unsigned type inverts the
 * ordering for negative values. This trait provides the masks and bit
 * indices needed to correct the ordering.
 *
 * @tparam T  An arithmetic type (signed/unsigned integral or floating-point)
 */
template <typename T>
struct SignBitTraits {
  static_assert(std::is_arithmetic_v<T>, "arithmetic type required");
  using type = T;
  using uint_t = uint_bits_t<sizeof(T) * CHAR_BIT>;
  static constexpr size_t kSignBitIndex = sizeof(T) * CHAR_BIT - 1;
  static constexpr uint_t kAllBitMask = ~uint_t(0);
  static constexpr uint_t kSignBitMask = kAllBitMask << kSignBitIndex;
};

// ---------------------------------------------------------------------------
// Radix sort traits (compile-time parameter bundle)
// ---------------------------------------------------------------------------

/**
 * @brief Compile-time parameters from RadixSortOptions and element type and
 *        KeyMap function.
 *
 * @details
 * Inherits SignBitTraits<T> for sign-bit masks and indices, used to correct
 * ordering of signed integers and floats.
 *
 * Exports selected options for other templates:
 *   - kExecutionPolicy            : execution policy (Seq / Par)
 *   - kSortStrategy               : sorting strategy (Lsd / Msd)
 *   - kSortOrder                  : sort order (Ascending / Descending)
 *   - kBucketCounterType          : counter type choice (UInt32 / UInt64)
 *   - kNaNsPosHandling            : nan postion (Unhandled / AtFrist AtEnd)
 *   - kNaNsAssumption             : floating-point NaNs (Non/Existence)
 *   - kMaxStackBytes              : stack budget for counters
 *   - kMsdFallbackToLsdThreshold  : Threshold for switching to LSD radix sort
 * at the recursion base case in MSD radix sort.
 *
 * Defines value_t = T, the element type;
 * Defines key_t = std::invoke_result_t<KeyMap, T>, the arithmetic type;
 * Defines uint_t = SignBitTraits<T>::uint_t, the unsigned integral type;
 * Defines counter_t = typename CounterTypeSelector<kBucketCounterType>::type;
 *
 * Key constants derived from RadixOptions.ChunkBits and uint_t:
 *   - kChunkBits    : bits per pass (8 or 16)
 *   - kChunkSize    : 2^kChunkBits, buckets per pass
 *   - kRadixMask    : kChunkSize - 1, mask for current chunk
 *   - kNumPasses    : sizeof(uint_t) * CHAR_BIT / kChunkBits
 *
 * Counter memory:
 *   - kChunkBytes: total bytes = kChunkSize * sizeof(counter_t)
 *
 * Stack vs. heap allocation for counters:
 *   - kInlineN      :  If kChunkBytes ≤ kMaxStackBytes,
 *                      kInlineN = kChunkSize (for FOLLY_MALLOCA_THRESHOLD).
 *                      Otherwise kInlineN = 0, forcing heap allocation.
 *
 * Sign bit optimization:
 *   - kIsHandlingSignBit
 *
 * @tparam RadixOptions  Compile-time sorting options
 * @tparam T             Element type
 * @tparam KeyMap        Predicate function
 *                       whose custom return type is an arithmetic type
 */
template <
    RadixSortOptions RadixOptions,
    typename T,
    detail::arithmetic_key_map<T> KeyMap>
struct RadixSortTraits : SignBitTraits<std::invoke_result_t<KeyMap, T>> {
  // Re‑export selected options for convenient access by other templates.
  static constexpr auto kExecutionPolicy = RadixOptions.ExecutionPolicy;
  static constexpr auto kSortStrategy = RadixOptions.SortStrategy;
  static constexpr auto kSortOrder = RadixOptions.SortOrder;
  static constexpr auto kBucketCounterType = RadixOptions.BucketCounterType;
  static constexpr auto kNaNsPosHandling = RadixOptions.NaNsPosHandling;
  static constexpr auto kNaNsAssumption = RadixOptions.NaNsAssumption;
  static constexpr auto kMaxStackBytes = RadixOptions.MaxStackBytes;
  static constexpr auto kMsdFallbackToLsdThreshold =
      RadixOptions.MsdFallbackToLsdThreshold;

  // value_t(any type)        ->  KeyMap(user-defined function)        ->
  // key_t  (arithmetic type) ->  Projection(radix_uint_projection_fn) ->
  // uint_t (unsigned type)
  using value_t = T;
  using key_t = std::invoke_result_t<KeyMap, T>;
  using uint_t = typename SignBitTraits<key_t>::uint_t;
  using counter_t = typename CounterTypeSelector<kBucketCounterType>::type;

  static constexpr size_t kChunkBits =
      static_cast<std::underlying_type_t<RadixChunkBits>>(
          RadixOptions.ChunkBits);
  static constexpr size_t kChunkSize = constexpr_pow(2, kChunkBits);
  static constexpr size_t kRadixMask = kChunkSize - 1U;
  static constexpr size_t kNumPasses = sizeof(uint_t) * CHAR_BIT / kChunkBits;

  // Total memory (in bytes) required for all counters.
  static constexpr size_t kChunkBytes = kChunkSize * sizeof(counter_t);

  // Inline capacity for FOLLY_MALLOCA_THRESHOLD.
  // Inline if counters fit within kMaxStackBytes, heap otherwise.
  static constexpr size_t kInlineN =
      (kChunkBytes <= kMaxStackBytes) ? kChunkSize : 0;

  static constexpr bool kIsHandlingSignBit = std::is_signed_v<key_t> &&
      !static_cast<bool>(RadixOptions.NegativeAssumption);
};

// ---------------------------------------------------------------------------
// Unsigned integer projection
// ---------------------------------------------------------------------------

/**
 * @brief Converts an arbitrary arithmetic key into an unsigned integer suitable
 * for radix sorting.
 *
 * @details
 * The returned callable performs three transformations:
 *  1. Applies the user's KeyMap to extract an arithmetic value.
 *  2. Transforms signed integers by flipping the sign bit so that negative
 *     values sort before positive ones.
 *  3. Transforms IEEE 754 floats by mapping the bit representation into a
 *     total order, handling NaN placement if requested.
 *
 * The transformation is split into two stages controlled by the boolean
 * template parameter IsHandlingSignBit. During early algorithm passes only the
 * lower‑order bytes are examined, so sign‑bit correction can be deferred until
 * the final pass. This strategy reduces the number of bitwise instructions used
 * to project elements to unsigned integers in the hot loop during scatter by
 * 3 * (kNumPasses - 1) / kNumPasses * TotalElements.
 *
 * The returned Projector struct provides:
 *   - template&lt;bool IsHandlingSignBit = false&gt;
 *     auto operator()(const auto&amp; x) const noexcept -&gt; uint_t
 *     for computing the unsigned projection.
 *   - constexpr const KeyMap&amp; getKeyMap() const noexcept
 *     for retrieving the original key extraction functor.
 */
struct radix_uint_projection_fn {
  // Projector: projects elements to unsigned integers for radix sorting.
  // Template parameter NaNsPosHandling controls NaN placement.
  // Template parameter arithmetic_t is the intermediate key type after KeyMap.
  // Template parameter KeyMap is the stored key extraction functor type.
  template <
      RadixNaNsPosHandling NaNsPosHandling,
      typename arithmetic_t,
      typename KeyMap>
  struct Projector {
    KeyMap km;

    using Traits = SignBitTraits<arithmetic_t>;
    using uint_t = Traits::uint_t;

    /*
     * Defaulted template parameter IsHandlingSignBit = false.
     * During the final LSD (least significant digit) pass, the caller
     * explicitly passes <true> to request the sign‑bit correction. For the MSD
     * (most significant digit) case, the corresponding handling is performed
     * during the initial pass instead.
     *
     * This separation exists because for signed integers and floats the
     * sign bit is the most significant bit.  In LSD order the MSB is only
     * considered during the very last pass, so we can skip the correction
     * logic for all earlier passes.  Compiler Explorer confirms that this
     * saves several instructions per element:
     *   https://compiler-explorer.com/z/j56PxGK1s
     */
    template <bool IsHandlingSignBit = false>
    constexpr auto operator()(const auto& x) const noexcept -> uint_t {
      if constexpr (
          std::same_as<arithmetic_t, bool> ||
          std::unsigned_integral<arithmetic_t>) {
        // Unsigned types need no transformation at all.
        return km(x);
      } else if constexpr (std::signed_integral<arithmetic_t>) {
        /*
         * Two's complement signed integers sort backward when reinterpreted
         * as unsigned because negative numbers have the MSB set.  Flipping
         * the sign bit maps the most negative value to 0 and the most
         * positive to UINT_MAX, restoring the natural ordering.
         */
        return IsHandlingSignBit
            ? static_cast<uint_t>(km(x)) ^ Traits::kSignBitMask
            : static_cast<uint_t>(km(x));
      } else if constexpr (std::floating_point<arithmetic_t>) {
        static_assert(
            !(NaNsPosHandling != RadixNaNsPosHandling::Unhandled &&
              !std::numeric_limits<arithmetic_t>::is_iec559),
            "NaN pos handling requires IEEE 754 floating-point");

        /*
         * IEEE 754 floats are almost lexicographically ordered by their
         * bit representation, except:
         *
         *  - The sign bit inverts ordering for negative floats (similar to
         *    signed integers).
         *  - Negative floats sort in reverse order (e.g., -1.0 > -2.0 in
         *    bit representation).
         *
         * The fix: for negative values (signMask == all-ones), flip every
         * bit except the sign bit itself.  For positive values (signMask
         * == 0), flip only the sign bit.  This produces the mapping:
         *
         *   -inf  -> 0x0000...
         *   -0.0  -> 0x7FFF...  (just below +0.0)
         *   +0.0  -> 0x8000...
         *   +inf  -> 0xFFFF...
         */
        const typename Traits::type raw = km(x);
        if constexpr (NaNsPosHandling != RadixNaNsPosHandling::Unhandled) {
          if (std::isnan(raw)) {
            // Map NaN to either the smallest or largest possible key so
            // that it sorts to the beginning or end of the sequence.
            return NaNsPosHandling == RadixNaNsPosHandling::AtFirst
                ? uint_t(0)
                : std::numeric_limits<uint_t>::max();
          }
        }
        const auto v = std::bit_cast<uint_t>(raw);

        /*
         * signMask is 0 for positive floats and ~0 for negative floats.
         * This trick works because v >> kSignBitIndex is 0 for positive
         * values and 1 for negative values; subtracting from 0 gives 0 or
         * all-ones respectively via unsigned wraparound.
         */
        const auto signMask = (uint_t(0) - (v >> Traits::kSignBitIndex));

        uint_t base = IsHandlingSignBit
            ? (v ^ signMask) | (Traits::kSignBitMask & ~signMask)
            : (v ^ signMask);
        /*
         * If AtFirst is active, NaNs occupy slot 0, so we bump every
         * valid float up by one to make room.
         *
         * One might worry: what if a finite float's bit pattern is all-ones
         * (0xFFFFFFFF for 32-bit, 0xFFFFFFFFFFFFFFFF for 64-bit)? Then
         * `base + 1U` would overflow to 0, breaking the sort order.
         *
         * Fortunately, that cannot happen. According to the IEEE 754
         * standard, any bit pattern where all exponent bits are 1 and the
         * mantissa is non‑zero is a NaN. The all‑ones pattern has
         * exponent=all‑1 and mantissa=all‑1 (non‑zero), therefore it is
         * guaranteed to be a NaN. NaNs have already been handled above and
         * are mapped to 0 (first slot) or to max (last slot) depending on
         * NaNsPosHandling. Hence, for every finite floating-point number
         * (including ±inf, ±0, normals, subnormals) the bit pattern is
         * *never* all‑ones, and `base + 1U` never overflows.
         */
        if constexpr (NaNsPosHandling == RadixNaNsPosHandling::AtFirst) {
          base += 1U;
        }
        return base;
      } else {
        assume_unreachable();
      }
    }

    constexpr const KeyMap& getKeyMap() const noexcept { return km; }
  };

  /**
   * @tparam NaNsPosHandling          NaN placement policy
   * @tparam T                        Element type
   * @tparam KeyMap                   User-defined key extraction functor
   * @param  std::in_place_type_t<T>  Assist the compiler in deducing type T
   * @param  keyMap                   User-defined key extraction functor param
   *
   * @return A Projector struct that projects elements to unsigned integers
   *         and provides access to the original key extraction functor.
   *         The operator() has a template parameter:
   *           - bool IsHandlingSignBit (default false): if true, applies the
   *             sign‑bit correction for signed integers and floats.
   *         Use getKeyMap() to retrieve the original KeyMap functor.
   */
  template <RadixNaNsPosHandling NaNsPosHandling, typename T, typename KeyMap>
  constexpr auto operator()(
      std::in_place_type_t<T> /* type_placement */,
      KeyMap&& keyMap) const noexcept {
    using arithmetic_t = std::invoke_result_t<KeyMap, T>;
    return Projector<NaNsPosHandling, arithmetic_t, std::decay_t<KeyMap>>{
        std::forward<KeyMap>(keyMap)};
  }
};

inline constexpr radix_uint_projection_fn radix_uint_projection{};

// ---------------------------------------------------------------------------
// Insertion sort fallback
// ---------------------------------------------------------------------------

/**
 * @brief Insertion sort over a bidirectional iterator range.
 *
 * @details
 * Used as a fallback when the input size is below a small threshold.
 * Insertion sort is O(n^2) but has very low constant factors and exploits
 * near-sorted inputs efficiently, making it faster than radix sort for very
 * small arrays.
 *
 * @tparam BiIter   Bidirectional iterator type
 * @tparam Compare  Strict weak ordering predicate (default: std::ranges::less)
 * @param first     Start of range (inclusive)
 * @param last      End of range (exclusive)
 * @param comp      Comparison functor
 */
template <
    std::bidirectional_iterator BiIter,
    std::indirect_strict_weak_order<BiIter> Compare = std::ranges::less>
void insertionSort(BiIter first, BiIter last, Compare comp = {}) {
  if (first == last) [[unlikely]] {
    return;
  }
  for (auto current = std::next(first); current != last; ++current) {
    auto value = std::move(*current);
    auto hole = current;
    auto prev = hole;
    while (prev != first && comp(value, *std::prev(prev))) {
      --prev;
      *hole = std::move(*prev);
      hole = prev;
    }
    *hole = std::move(value);
  }
}

/**
 * @brief Threshold below which insertion sort replaces radix sort.
 *
 * @details
 * For arrays smaller than this value, the overhead of histogram
 * construction, prefix sums, and two full copies per pass pair
 * exceeds the O(n^2) cost of a simple insertion sort.
 */
inline constexpr size_t kRadixSortThreshold = 64U;

// ---------------------------------------------------------------------------
// Sort bool
// ---------------------------------------------------------------------------

/**
 * @brief Partition booleans into false/true groups in a single O(n) pass.
 *
 * @details
 * Iterates once over the range, moving true values to one end and false
 * values to the other.  This is significantly faster than using a general
 * radix sort for boolean data.
 *
 * @tparam RandIter    Random-access iterator type
 * @tparam Descending  If true, true values are placed before false values
 * @param first        [in,out] Start of range (inclusive)
 * @param last         [in,out] End of range (exclusive)
 */
template <std::random_access_iterator RandIter, bool Descending>
constexpr void sortBool(RandIter first, RandIter last) {
  size_t cnt = 0;
  for (auto curr = first; curr < last; ++curr) {
    auto v = *curr;
    *curr = Descending ? false : true;
    *(first + cnt) = v;
    cnt += static_cast<size_t>(Descending ? v : !v);
  }
}

// ---------------------------------------------------------------------------
// NaN max_element and is_sorted predicate
// ---------------------------------------------------------------------------

/**
 * @brief Generate a NaN‑aware predicate for maximum comparison.
 *
 * @details
 * Rule: NaN is considered less than any non‑NaN, so that the maximum
 *       points to the largest non‑NaN key.
 *
 * @tparam Proj       Projection function type, accepts an element and returns a
 * key
 * @tparam NaNsExist  Compile‑time constant indicating whether NaN handling is
 * needed
 * @param  proj       Projection function object (captured by value)
 * @return A callable object with signature bool(const T& a, const T& b),
 *         where T is the element type.
 */
template <typename Proj, bool NaNsExist>
auto makeMaxElementPred(Proj proj) {
  return
      [proj](const auto& a, const auto& b) noexcept(noexcept(proj(a))) -> bool {
        auto km = proj.getKeyMap();
        using arithmetic_t = std::invoke_result_t<decltype(km), decltype(a)>;
        auto x = km(a);
        auto y = km(b);
        if constexpr (NaNsExist && std::is_floating_point_v<arithmetic_t>) {
          if (std::isnan(x) && !std::isnan(y))
            return true; // NaN < non‑NaN
          if (!std::isnan(x) && std::isnan(y))
            return false; // non‑NaN < NaN
        }
        return proj(x) < proj(y); // normal order: both non‑NaN or both NaN
      };
}

/**
 * @brief Generate a NaN‑aware predicate for ascending order checking.
 *
 * @details
 * Rule: Any comparison involving NaN directly returns true (meaning "current <
 * previous"), so that when negated in the caller it yields false, setting
 * sorted to false.
 *
 * @tparam Proj       Projection function type, accepts an element and returns a
 * key
 * @tparam NaNsExist  Compile‑time constant indicating whether NaN handling is
 * needed
 * @param  proj       Projection function object (captured by value)
 * @return A callable object with signature bool(const T& a, const T& b),
 *         where T is the element type.
 */
template <typename Proj, bool NaNsExist, typename Pred = std::less<>>
auto makeIsSortedPred(Proj proj, Pred pred = {}) {
  return [proj, pred](
             const auto& a, const auto& b) noexcept(noexcept(proj(a))) -> bool {
    auto x = proj(a);
    auto y = proj(b);
    if constexpr (NaNsExist) {
      if (std::isnan(x) || std::isnan(y))
        return true; // NaN involved ⇒ treat as out‑of‑order
    }
    return pred(x, y); // normal ascending comparison
  };
}

// ---------------------------------------------------------------------------
// Radix digit extraction
// ---------------------------------------------------------------------------

/**
 * @brief Extract the bucket index for a given key(uint_t) and radix pass.
 *
 * @details
 * Shifts the key(uint_t) right by `pass * kChunkBits` and masks with
 * `kRadixMask` to isolate the current chunk.  For descending order
 * the key(uint_t) is bitwise-inverted before extraction so that larger
 * original keys land in lower bucket indices.
 *
 * @tparam RadixTraits  Compile-time radix parameters (chunk size,
 *                      sort order, etc.)
 * @param  x            Projected unsigned key value
 * @param  pass         Current pass index (0 = LSB)
 * @return              Bucket index in [0, kChunkSize)
 */
template <typename RadixTraits>
#if !defined(DEBUG) && !defined(_DEBUG)
FOLLY_ALWAYS_INLINE
#endif
    constexpr auto
    nthRadix(typename RadixTraits::uint_t x, size_t pass) noexcept {
  if constexpr (RadixTraits::kSortOrder == RadixSortOrder::Ascending) {
    return (x >> RadixTraits::kChunkBits * pass) & RadixTraits::kRadixMask;
  } else if constexpr (RadixTraits::kSortOrder == RadixSortOrder::Descending) {
    /*
     * Bitwise inversion of the key before extracting the chunk achieves
     * descending order naturally: the largest keys become the smallest
     * unsigned integers, so they land in the first few buckets.
     */
    return ((~x) >> RadixTraits::kChunkBits * pass) & RadixTraits::kRadixMask;
  } else {
    assume_unreachable();
  }
}

template <typename T>
constexpr size_t highestSetBitByteIndex(T x) {
  static_assert(std::is_arithmetic_v<T>);
  if (x == T(0)) [[unlikely]] {
    return 0;
  }
  int bitPos = std::bit_width(x) - 1;
  return static_cast<size_t>(bitPos / CHAR_BIT);
}

// ---------------------------------------------------------------------------
// Radix parallel computing utils
// ---------------------------------------------------------------------------

/**
 * @brief Partition information for distributing work among OpenMP threads.
 *
 * @note
 * Given n total elements and `threads` workers:
 *   step   = n / threads        (base chunk per thread)
 *   remain = n % threads        (first `remain` threads get one extra)
 *
 * This ensures each thread processes either `step` or `step+1`
 * elements and all chunks are contiguous in memory.
 */
struct ThreadChunkInfo {
  uint32_t threads; ///< Total number of OpenMP threads
  uint32_t remain; ///< Number of threads that process one extra element
  size_t step; ///< Base chunk size (floor(n / numThreads))
};

/**
 * @brief Statistics for a chunk of data processed by a single thread during
 * parallel radix sort.
 *
 * @details
 * After a thread finishes sorting its assigned contiguous chunk, it returns
 * this struct to indicate:
 * - Whether the chunk is internally sorted (in non-decreasing order).
 * - The minimum and maximum values within the chunk.
 *
 * These per‑chunk statistics are later combined across all threads to
 * determine if the entire array is sorted. If all chunks are sorted and the
 * maximum of each chunk is ≤ the minimum of the next chunk, then the whole
 * array is sorted.
 */
struct ThreadChunkStats {
  bool isSorted; ///< True if the chunk is sorted in ascending order.
  size_t minVal; ///< Minimum value in the chunk.
  size_t maxVal; ///< Maximum value in the chunk.
};

#if !defined(DEBUG) && !defined(_DEBUG)
FOLLY_ALWAYS_INLINE
#endif // !defined(DEBUG) && !defined(_DEBUG)
/**
 * @brief Computes the inclusive start and exclusive end indices of the chunk
 * assigned to a given thread.
 *
 * @details
 * Allocation strategy:
 *   - First 'remain' threads (tid ∈ [0, remain)) get step+1 elements,
 *     starting at tid * (step + 1).
 *   - Remaining threads (tid ≥ remain) get step elements,
 *     starting at remain * (step + 1) + (tid - remain) * step.
 *
 * The resulting intervals are contiguous, non‑overlapping, and cover exactly
 * [0, n), with at most a one‑element difference between threads for good load
 * balancing.
 *
 * @param tid    Thread ID (0‑based)
 * @param step   Base chunk size (n / numThreads)
 * @param remain Number of threads that get an extra element
 * @return std::pair<size_t, size_t>  beginning (inclusive) and end (exclusive)
 * indices
 */
constexpr auto computeThreadChunkRange(
    size_t tid, size_t step, size_t remain) noexcept {
  size_t begIndex, endIndex;
  if (tid < remain) {
    begIndex = tid * (step + 1);
    endIndex = begIndex + (step + 1);
  } else {
    begIndex = remain * (step + 1) + (tid - remain) * step;
    endIndex = begIndex + step;
  }
  return std::pair{begIndex, endIndex};
}

/**
 * @brief Checks whether the combined data across all threads is globally
 * sorted.
 *
 * @details
 * This function verifies two conditions for every chunk:
 * 1. Each chunk is internally sorted (isSorted == true).
 * 2. Adjacent chunks are well-ordered according to Order:
 *    - Ascending:  prev.maxVal <= curr.minVal
 *    - Descending: prev.minVal >= curr.maxVal
 *
 * If both conditions hold for all adjacent chunks, the entire array is sorted.
 * The function uses a loop-unrolled switch for power-of-two thread counts (≤
 * 32) and falls back to a generic loop for larger or unsupported counts.
 *
 * @tparam Order   Sorting direction (Ascending or Descending).
 * @param a        Pointer to an array of ThreadChunkStats, one per thread.
 * @param threads  Number of threads (must be a power of two,
 *                 e.g., 1, 2, 4, ...).
 * @return true    If the whole dataset is fully sorted in the given order.
 * @return false   Otherwise.
 */
template <RadixSortOrder Order = RadixSortOrder::Ascending>
#if !defined(DEBUG) && !defined(_DEBUG)
FOLLY_ALWAYS_INLINE
#endif // !defined(DEBUG) && !defined(_DEBUG)
    bool
    isChunksSorted(const ThreadChunkStats* a, size_t threads) noexcept {
  assert(
      std::has_single_bit(threads) &&
      "Error: n must be a power of two (1, 2, 4, 8...)");
  assume(std::has_single_bit(threads));

  constexpr auto checkOrder = [](const auto& prev, const auto& curr) -> bool {
    if constexpr (Order == RadixSortOrder::Ascending) {
      return prev.maxVal <= curr.minVal;
    } else {
      return prev.minVal >= curr.maxVal;
    }
  };

#pragma push_macro("CHECK_NEXT")
#pragma push_macro("ALL_IS")
#undef CHECK_NEXT
#undef ALL_IS
#define CHECK_NEXT(i) a[i].isSorted&& checkOrder(a[i - 1], a[i])
#define ALL_IS(i) a[i].isSorted

  switch (threads) {
    case 1: {
      return ALL_IS(0);
    }
    case 2: {
      return ALL_IS(0) && CHECK_NEXT(1);
    }
    case 4: {
      return ALL_IS(0) && CHECK_NEXT(1) && CHECK_NEXT(2) && CHECK_NEXT(3);
    }
    case 8: {
      return ALL_IS(0) && CHECK_NEXT(1) && CHECK_NEXT(2) && CHECK_NEXT(3) &&
          CHECK_NEXT(4) && CHECK_NEXT(5) && CHECK_NEXT(6) && CHECK_NEXT(7);
    }
    case 16: {
      return ALL_IS(0) && CHECK_NEXT(1) && CHECK_NEXT(2) && CHECK_NEXT(3) &&
          CHECK_NEXT(4) && CHECK_NEXT(5) && CHECK_NEXT(6) && CHECK_NEXT(7) &&
          CHECK_NEXT(8) && CHECK_NEXT(9) && CHECK_NEXT(10) && CHECK_NEXT(11) &&
          CHECK_NEXT(12) && CHECK_NEXT(13) && CHECK_NEXT(14) && CHECK_NEXT(15);
    }
    case 32: {
      return ALL_IS(0) && CHECK_NEXT(1) && CHECK_NEXT(2) && CHECK_NEXT(3) &&
          CHECK_NEXT(4) && CHECK_NEXT(5) && CHECK_NEXT(6) && CHECK_NEXT(7) &&
          CHECK_NEXT(8) && CHECK_NEXT(9) && CHECK_NEXT(10) && CHECK_NEXT(11) &&
          CHECK_NEXT(12) && CHECK_NEXT(13) && CHECK_NEXT(14) &&
          CHECK_NEXT(15) && CHECK_NEXT(16) && CHECK_NEXT(17) &&
          CHECK_NEXT(18) && CHECK_NEXT(19) && CHECK_NEXT(20) &&
          CHECK_NEXT(21) && CHECK_NEXT(22) && CHECK_NEXT(23) &&
          CHECK_NEXT(24) && CHECK_NEXT(25) && CHECK_NEXT(26) &&
          CHECK_NEXT(27) && CHECK_NEXT(28) && CHECK_NEXT(29) &&
          CHECK_NEXT(30) && CHECK_NEXT(31);
    }
    default: {
      for (size_t i = 0; i < threads; ++i) {
        if (!a[i].isSorted) {
          return false;
        }
        if (i > 0 && !checkOrder(a[i - 1], a[i])) {
          return false;
        }
      }
      return true;
    }
  }
  assume_unreachable();
#pragma pop_macro("CHECK_NEXT")
#pragma pop_macro("ALL_IS")
}

// ---------------------------------------------------------------------------
// Implementations of Radix Sort in Both LSD and MSD Modes, with Sequential and
// Parallel Versions
// ---------------------------------------------------------------------------

/**
 * @brief Dispatch table for radix sort implementation selection.
 *
 * @details
 * The primary template is left undefined.  Specialisations are
 * provided for each valid (SortStrategy, ExecutionPolicy) pair.
 * Each specialisation exposes a static `sort()` entry point.
 *
 * @tparam RadixTraits   Compile-time sorting options traits
 * @tparam RandIter      Random-access iterator type
 * @tparam Allocator     Allocator for temporary buffer
 * @tparam Projection    Maps value_t to an unsigned integral key
 */
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
struct RadixSortImplDispatcher;

// ---------------------------------------------------------------------------
// LSD + Seq
// ---------------------------------------------------------------------------

/**
 * @brief Sequential LSD radix sort implementation.
 *
 * @details
 * Processes digits from least to most significant byte using
 * counting sort per pass.  Alternating source/destination buffers
 * avoid an extra copy between passes.
 *
 * Algorithm (3 phases per pass):
 *   1. Build per-pass histograms (bucket counts).
 *   2. Convert counts to prefix sums (starting offsets).
 *   3. Scatter elements into buffer using the offsets.
 *
 * Sign-bit correction for signed/floating-point types is deferred
 * to the final pass, saving several instructions per element on
 * every earlier pass.
 *
 * @see RadixSortImplDispatcher (Msd, Seq) for the recursive counterpart.
 *
 * @tparam RadixTraits   Compile-time sorting options traits
 * @tparam RandIter      Random-access iterator type
 * @tparam Allocator     Allocator for temporary buffer
 * @tparam Projection    Maps value_t to an unsigned integral key
 */
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
  requires(
      RadixTraits::kSortStrategy == RadixSortStrategy::Lsd &&
      RadixTraits::kExecutionPolicy == RadixExecutionPolicy::Seq)
struct RadixSortImplDispatcher<RadixTraits, RandIter, Allocator, Projection> {
  using Traits = RadixTraits;
  using value_t = Traits::value_t;
  using key_t = Traits::key_t;
  using uint_t = Traits::uint_t;
  using counter_t = Traits::counter_t;
  using counters2d_t = counter_t (*)[Traits::kChunkSize];

  static_assert(
      !(Traits::kChunkBits == 16U && sizeof(uint_t) == 1U),
      "Do not set a RadixSortOptions.ChunkBits = RadixChunkBits::Bits16(two-byte) for one-byte data!");

  /**
   * @brief Increment all per-pass bucket counters for a single key(uint_t).
   *
   * @details
   * Unrolls to kNumPasses independent increments via fold expression
   * over the compile-time index sequence. The compiler can vectorise
   * or schedule these freely.
   *
   * @tparam Is        Index sequence [0, kNumPasses) at compile time
   * @param counters2D [in,out] 2-D counter array [kNumPasses][kChunkSize]
   * @param key        Projected unsigned key value
   */
  template <size_t... Is>
  static constexpr void countPasses(
      counters2d_t counters2D, const uint_t key, std::index_sequence<Is...>) {
    ((++counters2D[Is][nthRadix<Traits>(key, Is)]), ...);
  }

  /**
   * @brief Scan input once to build per-pass histograms and detect
   * sortedness.
   *
   * @details
   * For each element, computes the projected key with full sign-bit
   * handling and increments every pass's bucket counter via
   * countPasses().  Simultaneously verifies monotonicity: if the
   * entire input is already in the desired order the caller can
   * skip the scatter phase entirely.
   *
   * Because the scan already touches every cache line, the
   * sortedness check has negligible marginal cost.
   *
   * @param data       [in] Pointer to the input array
   * @param n          [in] Number of elements
   * @param counters2D [out] 2-D counter array [kNumPasses][kChunkSize]
   *                   (zeroed on entry, filled on exit)
   * @param proj       [in] Projection functor (unsigned key producer)
   * @return true if the input is already sorted, false otherwise
   */
  static constexpr bool buildHistograms(
      value_t* data, size_t n, counters2d_t counters2D, Projection proj) {
    auto prev = std::numeric_limits<uint_t>::min();
    bool isSorted = true;
    for (size_t i = 0U; i < n; ++i) {
      // NaN cannot be compared, and floating-point types need to be projected
      // to unsigned integers for counting, so the sign bit must be handled.
      auto curr = proj.template operator()<Traits::kIsHandlingSignBit>(data[i]);
      if constexpr (Traits::kSortOrder == RadixSortOrder::Ascending) {
        isSorted = isSorted && (curr >= prev);
      } else {
        isSorted = isSorted && (curr <= prev);
      }
      prev = curr;
      countPasses(
          counters2D, curr, std::make_index_sequence<Traits::kNumPasses>{});
    }
    return isSorted;
  }

  /**
   * @brief Convert per-pass bucket counts to prefix-sum offsets.
   *
   * @details
   * For each pass, accumulates bucket counts in-place so that
   * counters2D[pass][i] becomes the starting offset for bucket i.
   * Also records the number of non-empty buckets per pass so the
   * scatter can skip passes where all elements share the same byte.
   *
   * @post counters2D[pass][i] = Σ(counters2D[pass][0..i]) for all i
   *
   * @tparam Is             Index sequence [0, kNumPasses) at compile time
   * @param nonEmptyCounts  [out] Per-pass count of non-empty buckets
   * @param counters2D      [in,out] 2-D array [kNumPasses][kChunkSize];
   *                        on entry = raw counts, on exit = offsets
   */
  template <size_t... Is>
  static constexpr void prefixSum(
      counter_t nonEmptyCounts[],
      counters2d_t counters2D,
      std::index_sequence<Is...>) {
    auto process = [&](size_t index) -> void {
      nonEmptyCounts[index] = counters2D[index][0];
      for (size_t i = 1; i < Traits::kChunkSize; ++i) {
        nonEmptyCounts[index] += (counters2D[index][i] != 0);
        counters2D[index][i] += counters2D[index][i - 1];
      }
    };

    (process(Is), ...);
  }

  /**
   * @brief Scatter elements into the buffer using per-pass offsets.
   *
   * @details
   * Walks the array backward (stable placement) and copies each
   * element to the position given by its bucket offset, then
   * decrements the offset.  After every pass the roles of data
   * and buffer swap so the next pass reads from the newly sorted
   * arrangement.
   *
   * Sign-bit correction (IsHandlingSignBit = true) is applied only
   * during the last pass (kLastPass).  Earlier passes use the
   * cheaper projection without sign-bit handling, saving several
   * arithmetic instructions per element.
   *
   * @post After each pass the element order is stable with respect
   *       to the current chunk; pointers data/buffer are swapped.
   *
   * @param data            [in,out] Pointer to source array
   *                        (swapped with buffer after each pass)
   * @param n               [in] Number of elements
   * @param buffer          [out] Temporary buffer (alternates with data)
   * @param nonEmptyCounts  [in] Per-pass count of non-empty buckets;
   *                        passes with ≤1 occupied bucket are skipped
   * @param counters2D      [in,out] 2-D offset array
   *                        [kNumPasses][kChunkSize];
   *                        consumed (decremented) during scatter
   * @param proj            [in] Projection functor
   * @return Pointer to the sorted result (may be the original array
   *         or the temporary buffer depending on swap parity).
   */
  static constexpr value_t* scatterBackward(
      value_t* FOLLY_RESTRICT data,
      size_t n,
      value_t* FOLLY_RESTRICT buffer,
      const counter_t nonEmptyCounts[],
      counters2d_t counters2D,
      Projection proj) {
    constexpr size_t kLastPass = Traits::kNumPasses - 1;
    for (size_t pass = 0; pass < kLastPass; ++pass) {
      /*
       * For example, with 256 buckets processing 1 byte:
       * Suppose n elements are mapped to the same bucket. This means that the
       * current byte of those n elements has the same binary value, e.g., all
       * are 0xFF. In this case, no scatter operation is needed at all, which
       * reduces the call overhead. Additionally, when the bucket size is <= 1,
       * it also implies that when the size is 0, the current byte being
       * processed for those elements is 0x00, so scatter is also unnecessary.
       */
      if (nonEmptyCounts[pass] <= 1) {
        continue;
      }
      for (size_t i = n; i-- > 0;) {
        auto x = proj(data[i]);
        buffer[--counters2D[pass][nthRadix<Traits>(x, pass)]] =
            std::move(data[i]);
      }
      std::swap(data, buffer);
    }
    if (nonEmptyCounts[kLastPass] > 1) {
      for (size_t i = n; i-- > 0;) {
        auto x = proj.template operator()<Traits::kIsHandlingSignBit>(data[i]);
        buffer[--counters2D[kLastPass][nthRadix<Traits>(x, kLastPass)]] =
            std::move(data[i]);
      }
      std::swap(data, buffer);
    }
    return data;
  }

  /**
   * @brief Sort the input array using three-phase sequential LSD radix sort.
   *
   * @details
   * 1. Histogram — scan once to build per-pass bucket counts and detect
   *    an already-sorted input (early exit).
   * 2. Prefix sum — convert counts to starting offsets.
   * 3. Scatter — for each pass, scatter elements into the buffer using
   *    the offsets, then swap buffer roles.
   * 4. Final move-back — if the sorted result resides in the temporary
   *    buffer (e.g. when the last pass is skipped because all keys share
   *    the same MSB chunk), copy it back to the original array so that
   *    the post-condition always holds.
   *
   * Counter arrays use small_vector with inline capacity kInlineN.
   * If the total counter memory exceeds RadixSortOptions::MaxStackBytes,
   * allocation falls back to the heap automatically.
   *
   * @post The range [data, data+n) is sorted according to kSortOrder.
   *       The result is always placed in the original array, regardless
   *       of which passes were skipped.
   *
   * @param data      [in,out] Pointer to the input/output array
   * @param n         [in] Number of elements to sort
   * @param allocator [in] Allocator for the temporary element buffer
   * @param proj      [in] Projection functor producing unsigned keys
   */
  static void sort(
      value_t* FOLLY_RESTRICT data,
      size_t n,
      const Allocator& allocator,
      Projection proj) {
    /*
     * Use MallocaArray for the counter arrays.  If the counters fit inside
     * the configured stack budget (kInlineN > 0), the allocation stays on
     * the stack, avoiding a heap trip.  Otherwise MallocaArray falls back
     * to the heap automatically.
     *
     * We reinterpret the flat storage as a 2D array [kNumPasses][kChunkSize]
     * so that each pass gets its own contiguous counter region.
     */
    MallocaArray<counter_t> countersStorage(
        FOLLY_MALLOCA_THRESHOLD(Traits::kChunkBytes, Traits::kInlineN),
        Traits::kChunkSize * Traits::kNumPasses);
    auto counters2D = reinterpret_cast<counters2d_t>(countersStorage.data());

    // Phase 1: build the histogram.
    if (buildHistograms(data, n, counters2D, proj)) {
      return;
    }

    // Phase 2: convert counts to prefix sums (starting offsets).
    counter_t nonEmptyCounts[Traits::kNumPasses];
    prefixSum(
        nonEmptyCounts,
        counters2D,
        std::make_index_sequence<Traits::kNumPasses>{});

    // Phase 3: scatter into the temporary buffer, then swap back.
    auto* const original = data;
    AllocatedBufferHolder<Allocator> holder(n, allocator);
    auto* FOLLY_RESTRICT buffer = holder.data();
    auto* result =
        scatterBackward(data, n, buffer, nonEmptyCounts, counters2D, proj);

    // Ensure the final sorted data resides at the original pointer.
    // scatterBackward swaps data and buffer internally; after it
    // returns, whichever pointer result points to holds the sorted
    // result.  If it isn't the original array, do one final move.
    if (result != original) {
      std::move(result, result + n, original);
    }

    /*
     * We cannot rely on kNumPasses parity to decide the move-back because
     * the last pass may be skipped (nonEmptyCounts[kLastPass] <= 1), in
     * which case the parity-based check would point to the wrong array.
     * Using result != original is always correct regardless of skips.
     */
  }
};

// ---------------------------------------------------------------------------
// LSD + Par (requires _OPENMP)
// ---------------------------------------------------------------------------

#ifdef _OPENMP
/**
 * @brief Parallel LSD radix sort using OpenMP.
 *
 * @details
 * The algorithm proceeds as follows:
 *
 * 1. **Block distribution**: The input array is split into blocks and
 *    distributed across OpenMP threads.
 *
 * 2. **Private histogram construction**: Each thread maintains a private
 *    per-pass histogram of size `[kNumPasses][kChunkSize]`, resulting in
 *    a global 3D counter array of shape
 *    `[numThreads][kNumPasses][kChunkSize]`.
 *
 * 3. **Parallel prefix sum (merge)**: After the parallel histogram
 *    building, a parallel prefix sum merges the private histograms into
 *    global bucket offsets.
 *
 * 4. **Parallel scatter**: The scatter phase also runs in parallel. Each
 *    thread scatters its assigned block into the correct global positions
 *    using its own prefix-summed counters.
 *
 * The overall structure follows the same logic as the sequential LSD
 * variant (LSD + Seq): build histograms, compute prefix sums, scatter
 * backward. Refer to the LSD + Seq documentation for the base algorithm
 * description. The key difference is the 3D counter layout and the
 * additional merge step in the prefix sum.
 *
 * @par Stability
 * within each thread's block the backward-scatter loop is
 * stable. Combined with the deterministic per-thread block assignment,
 * the overall sort remains stable.
 *
 * @note Requires _OPENMP to be defined at compile time.
 *
 * @see RadixSortImplDispatcher (Msd, Seq) for the sequential counterpart.
 *
 * @tparam RadixTraits   Compile-time sorting options traits
 * @tparam RandIter      Random-access iterator type
 * @tparam Allocator     Allocator for temporary buffer
 * @tparam Projection    Maps value_t to an unsigned integral key
 */
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
  requires(
      RadixTraits::kSortStrategy == RadixSortStrategy::Lsd &&
      RadixTraits::kExecutionPolicy == RadixExecutionPolicy::Par)
struct RadixSortImplDispatcher<RadixTraits, RandIter, Allocator, Projection> {
  using Traits = RadixTraits;
  using value_t = Traits::value_t;
  using key_t = Traits::key_t;
  using uint_t = Traits::uint_t;
  using counter_t = Traits::counter_t;
  using counters2d_t = counter_t (*)[Traits::kChunkSize];

  static_assert(
      !(Traits::kChunkBits == 16U && sizeof(uint_t) == 1U),
      "Do not set a RadixSortOptions.ChunkBits = RadixChunkBits::Bits16(two-byte) for one-byte data!");

  /**
   * @brief Convert per-thread histograms to global bucket offsets via
   * column-wise prefix sum.
   *
   * @details
   * Treats counters2D as a 2D matrix:
   *   rows = numThreads, columns = kChunkSize.
   *
   * The prefix sum processes columns in "И"-shaped traversal:
   * vertical prefix-sum within each column, offset by previous column total.
   *
   * Column 0 has no predecessor, so it is handled separately.
   * Columns 1..kChunkSize-1 each start with the total sum of all previous
   * columns (the last-row value of the previous column), then accumulate
   * vertically.
   *
   * After this function returns, counters2D[tid][bucket] holds the
   * global starting offset (exclusive end) for that thread's portion
   * of that bucket.  The nonEmptyCount output tracks how many distinct
   * buckets have at least one element total (used to skip scatter when
   * all elements map to one bucket).
   *
   * @param numThreads  Number of OpenMP threads.
   * @param nonEmptyCount [out] Number of non-empty buckets for this pass.
   * @param counters2D  [in,out] 2-D array [numThreads][kChunkSize];
   *                    on entry = per-thread raw counts,
   *                    on exit = global starting offsets.
   */
  static constexpr void prefixSum(
      size_t numThreads, counter_t& nonEmptyCount, counters2d_t counters2D) {
    /*
     * Column 0: row-wise prefix sum across threads.
     * No predecessor column, so handled separately.
     * Because if processed together, it would introduce separate if checks,
     * which are useless for the subsequent Traits::kChunkSize - 1 passes, and
     * thus not worth the cost.
     */
    for (size_t tid = 1; tid < numThreads; ++tid) {
      counters2D[tid][0] += counters2D[tid - 1][0];
    }
    nonEmptyCount = (counters2D[numThreads - 1][0] != 0);

    // Columns 1..kChunkSize-1: offset by previous column total,
    // then row-wise prefix sum across threads.
    for (size_t i = 1; i < Traits::kChunkSize; ++i) {
      counter_t last = counters2D[numThreads - 1][i - 1];
      counters2D[0][i] += last;
      for (size_t tid = 1; tid < numThreads; ++tid) {
        counters2D[tid][i] += counters2D[tid - 1][i];
      }
      // Original column sum = current cumulative - previous
      // column total.
      nonEmptyCount += (counters2D[numThreads - 1][i] - last != 0);
    }
  }

  /**
   * @brief First pass with sortedness detection.
   *
   * In a single parallel region:
   *   1. Forward loop — builds pass-0 histogram + checks each thread's
   *      chunk for internal sortedness + tracks min/max.
   *   2. Barrier → single thread verifies global sortedness via
   *      `isChunksSorted` (cross-thread boundaries).  If sorted, skips
   *      scatter and returns true.
   *   3. If not sorted: one thread computes prefix sum for pass 0 (reusing
   *      the histogram).  Then all threads do a backward scatter + swap.
   *
   * @tparam IsFirstAlsoLast  True when kNumPasses == 1 and sign handling
   *                          is required (i.e., the only pass IS the last
   *                          pass).
   * @param tChunkInfo  Thread chunk partition info.
   * @param data        [in,out] Source array (swapped with buffer).
   * @param buffer      [out] Temporary buffer.
   * @param counters2D  [in,out] 2-D array [numThreads][kChunkSize];
   *                    zeros on entry, per-thread counts after step 1,
   *                    global offsets after step 3a.
   * @param proj        Projection functor.
   * @return true if the data is already fully sorted (no further work).
   */
  static bool firstPassAndCheckSorted(
      const ThreadChunkInfo& tChunkInfo,
      value_t* FOLLY_RESTRICT& data,
      value_t* FOLLY_RESTRICT& buffer,
      counters2d_t counters2D,
      Projection proj) {
    constexpr bool kHandleSign =
        (Traits::kNumPasses == 1) && Traits::kIsHandlingSignBit;
    const size_t nThreads = tChunkInfo.threads;

    std::fill_n(
        reinterpret_cast<counter_t*>(counters2D),
        nThreads * Traits::kChunkSize,
        counter_t{0});

    auto chunkStats = FOLLY_MALLOCA_ARRAY(ThreadChunkStats, nThreads);

    bool globallySorted = false;
    counter_t nonEmpty = 0;

#pragma omp parallel num_threads((int)nThreads)
    {
      int tid = omp_get_thread_num();
      auto [begIdx, endIdx] =
          computeThreadChunkRange(tid, tChunkInfo.step, tChunkInfo.remain);

      // --- Forward: histogram for pass 0 + sortedness check ---
      uint_t val = proj.template operator()<kHandleSign>(data[begIdx]);
      uint_t minVal = val, maxVal = val, prev = val;
      bool localSorted = true;
      ++counters2D[tid][nthRadix<Traits>(val, 0)];

      for (size_t i = begIdx + 1; i < endIdx; ++i) {
        val = proj.template operator()<kHandleSign>(data[i]);
        if (val < minVal) {
          minVal = val;
        }
        if (val > maxVal) {
          maxVal = val;
        }
        if constexpr (Traits::kSortOrder == RadixSortOrder::Ascending) {
          localSorted = localSorted && val >= prev;
        } else {
          localSorted = localSorted && val <= prev;
        }
        prev = val;
        ++counters2D[tid][nthRadix<Traits>(val, 0)];
      }
      chunkStats[tid] = {
          localSorted,
          static_cast<size_t>(minVal),
          static_cast<size_t>(maxVal)};

#pragma omp barrier
#pragma omp single
      {
        globallySorted =
            isChunksSorted<Traits::kSortOrder>(chunkStats.data(), nThreads);
        if (!globallySorted) {
          prefixSum(nThreads, nonEmpty, counters2D);
        }
      }

      if (!globallySorted && nonEmpty > 1) {
        for (size_t i = endIdx; i-- > begIdx;) {
          val = proj.template operator()<kHandleSign>(data[i]);
          buffer[--counters2D[tid][nthRadix<Traits>(val, 0)]] =
              std::move(data[i]);
        }
#pragma omp barrier
#pragma omp single
        {
          std::swap(data, buffer);
        }
      }
    }
    return globallySorted;
  }

  /**
   * @brief Generic radix pass (middle or last).
   *
   * Single parallel region: histogram → prefixSum → backward scatter → swap.
   *
   * @tparam IsLastPass  When true, applies sign-bit handling
   *                     (`kIsHandlingSignBit`).  When false, uses `false`
   *                     (no sign handling — correct for all non-MSB passes).
   * @param tChunkInfo  Thread chunk partition info.
   * @param data        [in,out] Source array (swapped with buffer).
   * @param buffer      [out] Temporary buffer.
   * @param counters2D  [in,out] 2-D array [numThreads][kChunkSize].
   * @param passIdx     Byte position being processed.
   * @param proj        Projection functor.
   */
  template <bool IsLastPass>
  static void doPass(
      const ThreadChunkInfo& tChunkInfo,
      value_t* FOLLY_RESTRICT& data,
      value_t* FOLLY_RESTRICT& buffer,
      counters2d_t counters2D,
      size_t passIdx,
      Projection proj) {
    constexpr bool kHandleSign = IsLastPass && Traits::kIsHandlingSignBit;
    const size_t nThreads = tChunkInfo.threads;
    counter_t nonEmptyCount = 0;

    std::fill_n(
        reinterpret_cast<counter_t*>(counters2D),
        nThreads * Traits::kChunkSize,
        counter_t{0});

#pragma omp parallel num_threads((int)nThreads)
    {
      int tid = omp_get_thread_num();
      auto [begIdx, endIdx] =
          computeThreadChunkRange(tid, tChunkInfo.step, tChunkInfo.remain);

      for (size_t i = begIdx; i < endIdx; ++i) {
        auto x = proj.template operator()<kHandleSign>(data[i]);
        ++counters2D[tid][nthRadix<Traits>(x, passIdx)];
      }

#pragma omp barrier
#pragma omp single
      {
        prefixSum(nThreads, nonEmptyCount, counters2D);
      }

      if (nonEmptyCount > 1) {
        for (size_t i = endIdx; i-- > begIdx;) {
          auto x = proj.template operator()<kHandleSign>(data[i]);
          buffer[--counters2D[tid][nthRadix<Traits>(x, passIdx)]] =
              std::move(data[i]);
        }
#pragma omp barrier
#pragma omp single
        {
          std::swap(data, buffer);
        }
      }
    }
  }

  /**
   * @brief Parallel LSD radix sort entry point.
   *
   * Phases:
   *   A. First pass with sortedness detection (reuses histogram for scatter).
   *   B. Middle passes 1..kNumPasses-2 (no sign handling).
   *   C. Last pass with sign handling.
   *
   * After all passes, data is moved back to the original array if needed
   * (skipped passes may break the swap-parity assumption).
   *
   * @param data      [in,out] Pointer to the input/output array
   * @param n         [in] Number of elements
   * @param allocator [in] Allocator for temporary buffer
   * @param proj      [in] Projection functor
   */
  static void sort(
      std::iter_value_t<RandIter>* FOLLY_RESTRICT data,
      size_t n,
      const Allocator& allocator,
      Projection proj) {
    AllocatedBufferHolder<Allocator> holder(n, allocator);
    auto* FOLLY_RESTRICT buffer = holder.data();

    const size_t numThreads = omp_get_max_threads();
    const ThreadChunkInfo tChunkInfo{
        .threads = static_cast<uint32_t>(numThreads),
        .remain = static_cast<uint32_t>(n % numThreads),
        .step = n / numThreads};

    const size_t countersTotal = Traits::kChunkSize * numThreads;
    const size_t countersBytes = countersTotal * sizeof(counter_t);

    MallocaArray<counter_t> countersStorage(
        FOLLY_MALLOCA_THRESHOLD(countersBytes, Traits::kInlineN),
        countersTotal);
    auto counters2D = reinterpret_cast<counters2d_t>(countersStorage.data());

    auto* const original = data;

    // Phase A: first pass + sortedness detection
    if (firstPassAndCheckSorted(tChunkInfo, data, buffer, counters2D, proj)) {
      return;
    }

    // Phase B: middle passes (no sign handling)
    if constexpr (Traits::kNumPasses > 1) {
      for (size_t pass = 1; pass < Traits::kNumPasses - 1; ++pass) {
        doPass<false>(tChunkInfo, data, buffer, counters2D, pass, proj);
      }
      // Phase C: last pass (with sign handling)
      doPass<true>(
          tChunkInfo, data, buffer, counters2D, Traits::kNumPasses - 1, proj);
    }

    if (data != original) {
      std::move(data, data + n, original);
    }
  }
};
#endif // _OPENMP

// ---------------------------------------------------------------------------
// MSD + Seq
// ---------------------------------------------------------------------------

/**
 * @brief Fallback LSD radix sort used by MSD for small sub-problems.
 *
 * @details
 * A simple single-pass LSD radix sort used as the fallback in MSD's
 * `sortImpl` when the sub-problem size drops below
 * MsdFallbackToLsdThreshold, or when the remaining byte depth is ≤ 1.
 * Iterates for the given number of passes, building a 1-D histogram,
 * converting to prefix sums, and scattering with backward iteration.
 * Reuses the same 1-D counter array across all passes.
 *
 * @tparam RadixTraits       Compile-time sorting options traits
 * @tparam Projection        Maps value_t to an unsigned integral key
 * @tparam IsHandlingSignBit Whether to apply MSB-flip for signed/floating
 * @param data    [in,out] Pointer to the sub-array to sort
 * @param n       [in] Number of elements
 * @param buffer  [out] Temporary buffer (alternates with data per pass)
 * @param passes  [in] Number of radix passes to perform
 * @param counters1D [in,out] 1-D counter array [kChunkSize], reused per pass
 * @param proj    [in] Projection functor
 * @param depth   [in] Counter array depth index (reserved for future use)
 */
template <
    typename RadixTraits,
    typename Projection,
    bool IsHandlingSignBit = false>
void integerRadixSort(
    typename RadixTraits::value_t* FOLLY_RESTRICT data,
    const size_t n,
    typename RadixTraits::value_t* FOLLY_RESTRICT buffer,
    size_t passes,
    typename RadixTraits::counter_t* FOLLY_RESTRICT counters1D,
    Projection proj,
    size_t depth = 0) {
  if (n == 0) {
    return;
  }

  for (size_t pass = 0; pass < passes; ++pass) {
    using counter_t = typename RadixTraits::counter_t;
    std::fill_n(counters1D, RadixTraits::kChunkSize, counter_t{});

    for (size_t i = 0; i < n; i++) {
      auto x = proj.template operator()<IsHandlingSignBit>(data[i]);
      ++counters1D[nthRadix<RadixTraits>(x, pass)];
    }

    for (size_t i = 1; i < RadixTraits::kChunkSize; i++) {
      counters1D[i] += counters1D[i - 1];
    }

    for (size_t i = n; i-- > 0;) {
      auto x = proj.template operator()<IsHandlingSignBit>(data[i]);
      buffer[--counters1D[nthRadix<RadixTraits>(x, pass)]] = std::move(data[i]);
    }
    std::swap(data, buffer);
  }

  if (passes & 1) {
    // After an odd number of swaps, the sorted data sits at the `buffer`
    // location (which the local variable `data` now points to).  Move it
    // back to the original `data` location (= `buffer` local).
    std::move(data, data + n, buffer);
  }
}

/**
 * @brief Sequential MSD radix sort implementation.
 *
 * @details
 * Recursively partitions elements by the most significant digit using
 * a depth-first approach.  Once sub-problem sizes fall below
 * MsdFallbackToLsdThreshold, the implementation switches to LSD radix
 * sort to avoid excessive recursion overhead.
 *
 * Compared to the LSD variant:
 *  - MSD examines high-order bytes first, reducing work when the most
 *    significant bytes distribute data widely (common for 64-bit keys).
 *  - Recursion depth is bounded by the number of distinct leading-byte
 *    values in the data, not by the total key width.
 *
 * @see RadixSortImplDispatcher (Lsd, Seq) for the iterative counterpart.
 *
 * @tparam RadixTraits   Compile-time sorting options traits
 * @tparam RandIter      Random-access iterator type
 * @tparam Allocator     Allocator for temporary buffer
 * @tparam Projection    Maps value_t to an unsigned integral key
 */
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
  requires(
      RadixTraits::kSortStrategy == RadixSortStrategy::Msd &&
      RadixTraits::kExecutionPolicy == RadixExecutionPolicy::Seq)
struct RadixSortImplDispatcher<RadixTraits, RandIter, Allocator, Projection> {
  using Traits = RadixTraits;
  using value_t = Traits::value_t;
  using key_t = Traits::key_t;
  using uint_t = Traits::uint_t;
  using counter_t = Traits::counter_t;
  using counters2d_t = counter_t (*)[Traits::kChunkSize];

  static_assert(
      !(Traits::kChunkBits == 16U && sizeof(uint_t) == 1U),
      "Do not set a RadixSortOptions.ChunkBits = RadixChunkBits::Bits16(two-byte) for one-byte data!");

  template <typename FwdIter, typename MaxPred, typename OrderPred>
  static constexpr auto findMaxAndCheckSorted(
      FwdIter first,
      FwdIter last,
      MaxPred maxPred, // compare predicate for finding maximum element
      OrderPred orderPred // ordering predicate for sortedness check
                          // (must satisfy strict weak ordering)
      ) noexcept -> std::pair<FwdIter, bool> {
    if (first == last)
      return {last, true};

    bool sorted = true;
    FwdIter maxIt = first;
    FwdIter prev = first;

    for (++first; first != last; ++first) {
      // Ascending: require !orderPred(*first, *prev).
      // If orderPred(*first, *prev) is ever true, the sequence is not sorted.
      sorted = sorted && !orderPred(*first, *prev);

      // Update max using the maxPred comparator.
      if (maxPred(*maxIt, *first)) {
        maxIt = first;
      }
      prev = first;
    }
    return {maxIt, sorted};
  }

  /**
   * @brief Recursive MSD partitioning implementation.
   *
   * @details
   * Builds a histogram for the current byte position, computes prefix
   * sums, then scatters elements into the buffer.  Recursively processes
   * each non-empty bucket.  Falls back to sorting via insertion sort or
   * LSD radix sort when the sub-problem size drops below
   * MsdFallbackToLsdThreshold, or when the remaining byte passes ≤ 1.
   *
   * The 2-D counter array (counters2D) is pre-allocated once in sort()
   * to avoid repeated heap allocations during recursion.  Each recursion
   * level uses a distinct row via the `depth` index.
   *
   * @param data  [in,out] Pointer to the sub-array to partition
   * @param n     [in] Number of elements in the sub-array
   * @param buffer [out] Temporary buffer for scattering
   * @param pass  [in] Current byte index (descending from MSB-1 to 0)
   * @param counters2D [in,out] 2-D counter array [passes][kChunkSize];
   *                   row `depth` is used for this recursion level
   * @param proj  [in] Projection functor
   * @param depth [in] Row index into counters2D for this recursion level
   */
  template <bool IsHandlingSignBit>
  static void sortImpl(
      value_t* FOLLY_RESTRICT data,
      size_t n,
      value_t* FOLLY_RESTRICT buffer,
      size_t pass,
      counters2d_t counters2D,
      Projection proj,
      size_t depth = 0) {
    if (n < Traits::kMsdFallbackToLsdThreshold || pass < 1) {
      // The LSD fallback counts passes via `pass + 1` because its
      // loop condition is `pass < passes` (strictly less than).
      return integerRadixSort<Traits, Projection, IsHandlingSignBit>(
          data, n, buffer, pass + 1, counters2D[depth], proj, depth);
    }

    auto c1 = counters2D[depth];
    std::fill_n(c1, Traits::kChunkSize, counter_t{});

    for (size_t i = 0U; i < n; ++i) {
      auto x = proj.template operator()<IsHandlingSignBit>(data[i]);
      ++c1[nthRadix<Traits>(x, pass)];
    }

    counter_t nonEmptyCount = c1[0];
    for (size_t i = 1; i < Traits::kChunkSize; ++i) {
      nonEmptyCount += (c1[i] != 0);
      c1[i] += c1[i - 1];
    }

    // All elements map to the same bucket (or all are zero) for this byte;
    // no scatter needed — recurse to the next byte directly.
    if (nonEmptyCount <= 1) {
      return sortImpl<IsHandlingSignBit>(
          data, n, buffer, pass - 1, counters2D, proj, depth + 1);
    }

    for (size_t i = n; i-- > 0;) {
      auto x = proj.template operator()<IsHandlingSignBit>(data[i]);
      buffer[--c1[nthRadix<Traits>(x, pass)]] = std::move(data[i]);
    }

    // Recurse into each non-empty bucket using the next lower byte.
    constexpr size_t kLastOffset = Traits::kChunkSize - 1;
    for (size_t i = 0; i < kLastOffset; ++i) {
      sortImpl<IsHandlingSignBit>(
          buffer + c1[i],
          c1[i + 1] - c1[i],
          data + c1[i],
          pass - 1,
          counters2D,
          proj,
          depth + 1);
    }
    auto lastVal = c1[kLastOffset];
    sortImpl<IsHandlingSignBit>(
        buffer + lastVal,
        n - lastVal,
        data + lastVal,
        pass - 1,
        counters2D,
        proj,
        depth + 1);
  }

  /**
   * @brief Sequential MSD radix sort entry point.
   *
   * @details
   * Allocates a temporary buffer, then delegates to sortImpl for the
   * recursive partitioning.
   *
   * @param data      [in,out] Pointer to the input/output array
   * @param n         [in] Number of elements
   * @param allocator [in] Allocator for temporary buffer
   * @param proj      [in] Projection functor
   */
  static void sort(
      value_t* FOLLY_RESTRICT data,
      size_t n,
      const Allocator& allocator,
      Projection proj) {
    AllocatedBufferHolder<Allocator> holder(n, allocator);
    auto* FOLLY_RESTRICT buffer = holder.data();
    /*
     * 1) Find the maximum element,
     * 2) check whether the input is already sorted,
     * 3) compute the MSB byte index to determine the actual number of passes.
     *
     * Steps 1&2 need the raw KeyMap (value_t -> key_t), NOT the projected key,
     * because the projection may alter the key (e.g. sign-bit flip for floats).
     * Floating-point types with NaN existence need special handling via
     * makeMaxElementPred / makeIsSortedPred to account for NaN ordering.
     *
     * MSD is primarily beneficial for 64-bit keys where the high-order bytes
     * are zero for most practical values, allowing early bytes to be skipped.
     */
    constexpr bool kIsNaNsExistence = std::floating_point<key_t> &&
        Traits::kNaNsAssumption == RadixNaNsAssumption::Existence;

    auto maxPred = makeMaxElementPred<decltype(proj), kIsNaNsExistence>(proj);

    // Build the ordering predicate from the raw key map.
    auto keyMap = proj.getKeyMap();
    auto orderPred = makeIsSortedPred<decltype(keyMap), kIsNaNsExistence>(
        keyMap,
        std::conditional_t < Traits::kSortOrder == RadixSortOrder::Ascending,
        std::less<>,
        std::greater < >> {});

    auto [it, sorted] =
        findMaxAndCheckSorted(data, data + n, maxPred, orderPred);

    if (sorted) {
      return;
    }
    // Use projected unsigned bit pattern so highestSetBitByteIndex sees
    // the correct byte positions for signed/floating-point types.
    const auto projectedMax =
        proj.template operator()<Traits::kIsHandlingSignBit>(*it);
    const size_t byteIndex = highestSetBitByteIndex(projectedMax);

    const size_t passes = (byteIndex + 1) * CHAR_BIT / Traits::kChunkBits;

    const size_t countersTotal = passes * Traits::kChunkSize;
    const size_t countersBytes = countersTotal * sizeof(counter_t);

    // Allocate the 2-D counter array once, avoiding repeated heap
    // allocations during MSD recursion.  sortImpl uses `depth` to
    // index into the appropriate row; each row is zero-filled on
    // first use by std::fill_n inside sortImpl.
    MallocaArray<counter_t> countersStorage(
        FOLLY_MALLOCA_THRESHOLD(countersBytes, Traits::kInlineN),
        countersTotal);
    auto counters2D = reinterpret_cast<counters2d_t>(countersStorage.data());

    if (passes != Traits::kNumPasses) {
      // Start from passes - 1 because sortImpl expects zero-based byte index.
      sortImpl<false>(data, n, buffer, passes - 1, counters2D, proj);
    } else {
      sortImpl<true>(data, n, buffer, passes - 1, counters2D, proj);
    }
  }
};

// ---------------------------------------------------------------------------
// MSD + Par (requires _OPENMP)
// ---------------------------------------------------------------------------

#ifdef _OPENMP
/**
 * @brief Parallel MSD radix sort using OpenMP tasks.
 *
 * @details
 * A single parallel region combines two phases:
 *   1. Chunked parallel scan — each thread builds min/max/sortedness
 *      for its chunk; merged via isChunksSorted for global sortedness
 *      and max-reduce for the maximum projected value.
 *   2. Task-based recursion — sortImpl allocates a local counter array
 *      on the stack (or heap for large kChunkSize via MallocaArray),
 *      builds the histogram, scatters, then spawns one task per
 *      non-empty bucket.
 *
 * Each sortImpl call represents one independent task; tasks are
 * scheduled across the thread pool by the OpenMP runtime.  The LSD
 * fallback (integerRadixSort) runs sequentially within its task
 * (sub-problems are already small).
 *
 * @note Requires _OPENMP to be defined at compile time.
 *
 * @see RadixSortImplDispatcher (Msd, Seq) for the sequential counterpart.
 *
 * @tparam RadixTraits   Compile-time sorting options traits
 * @tparam RandIter      Random-access iterator type
 * @tparam Allocator     Allocator for temporary buffer
 * @tparam Projection    Maps value_t to an unsigned integral key
 */
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
  requires(
      RadixTraits::kSortStrategy == RadixSortStrategy::Msd &&
      RadixTraits::kExecutionPolicy == RadixExecutionPolicy::Par)
struct RadixSortImplDispatcher<RadixTraits, RandIter, Allocator, Projection> {
  using Traits = RadixTraits;
  using value_t = Traits::value_t;
  using key_t = Traits::key_t;
  using uint_t = Traits::uint_t;
  using counter_t = Traits::counter_t;
  using counters2d_t = counter_t (*)[Traits::kChunkSize];

  static_assert(
      !(Traits::kChunkBits == 16U && sizeof(uint_t) == 1U),
      "Do not set a RadixSortOptions.ChunkBits = RadixChunkBits::Bits16(two-byte) for one-byte data!");

  template <typename FwdIter, typename MaxPred, typename OrderPred>
  static constexpr auto findMaxAndCheckSorted(
      FwdIter first,
      FwdIter last,
      MaxPred maxPred, // compare predicate for finding maximum element
      OrderPred orderPred // ordering predicate for sortedness check
                          // (must satisfy strict weak ordering)
      ) noexcept -> std::pair<FwdIter, bool> {
    if (first == last)
      return {last, true};

    bool sorted = true;
    FwdIter maxIt = first;
    FwdIter prev = first;

    for (++first; first != last; ++first) {
      // Ascending: require !orderPred(*first, *prev).
      // If orderPred(*first, *prev) is ever true, the sequence is not sorted.
      sorted = sorted && !orderPred(*first, *prev);

      // Update max using the maxPred comparator.
      if (maxPred(*maxIt, *first)) {
        maxIt = first;
      }
      prev = first;
    }
    return {maxIt, sorted};
  }

  /**
   * @brief Recursive MSD partitioning implementation.
   *
   * @details
   * Builds a histogram for the current byte position, computes prefix
   * sums, then scatters elements into the buffer.  Recursively processes
   * each non-empty bucket.  Falls back to sorting via insertion sort or
   * LSD radix sort when the sub-problem size drops below
   * MsdFallbackToLsdThreshold, or when the remaining byte passes ≤ 1.
   *
   * The 2-D counter array (counters2D) is pre-allocated once in sort()
   * to avoid repeated heap allocations during recursion.  Each recursion
   * level uses a distinct row via the `depth` index.
   *
   * @param data  [in,out] Pointer to the sub-array to partition
   * @param n     [in] Number of elements in the sub-array
   * @param buffer [out] Temporary buffer for scattering
   * @param pass  [in] Current byte index (descending from MSB-1 to 0)
   * @param counters2D [in,out] 2-D counter array [passes][kChunkSize];
   *                   row `depth` is used for this recursion level
   * @param proj  [in] Projection functor
   * @param depth [in] Row index into counters2D for this recursion level
   */
  template <bool IsHandlingSignBit>
  static void sortImpl(
      value_t* FOLLY_RESTRICT data,
      size_t n,
      value_t* FOLLY_RESTRICT buffer,
      size_t pass,
      counters2d_t counters2D,
      Projection proj,
      size_t depth = 0) {
    if (n < Traits::kMsdFallbackToLsdThreshold || pass < 1) {
      // The LSD fallback counts passes via `pass + 1` because its
      // loop condition is `pass < passes` (strictly less than).
      return integerRadixSort<Traits, Projection, IsHandlingSignBit>(
          data, n, buffer, pass + 1, counters2D[depth], proj, depth);
    }

    auto c1 = counters2D[depth];
    std::fill_n(c1, Traits::kChunkSize, counter_t{});
#pragma omp parallel num_threads((int)nThreads)
    {
      for (size_t i = 0U; i < n; ++i) {
        auto x = proj.template operator()<IsHandlingSignBit>(data[i]);
        ++c1[nthRadix<Traits>(x, pass)];
      }
    }

    counter_t nonEmptyCount = c1[0];
    for (size_t i = 1; i < Traits::kChunkSize; ++i) {
      nonEmptyCount += (c1[i] != 0);
      c1[i] += c1[i - 1];
    }

    // All elements map to the same bucket (or all are zero) for this byte;
    // no scatter needed — recurse to the next byte directly.
    if (nonEmptyCount <= 1) {
      return sortImpl<IsHandlingSignBit>(
          data, n, buffer, pass - 1, counters2D, proj, depth + 1);
    }
#pragma omp parallel num_threads((int)nThreads)
    {
      for (size_t i = n; i-- > 0;) {
        auto x = proj.template operator()<IsHandlingSignBit>(data[i]);
        buffer[--c1[nthRadix<Traits>(x, pass)]] = std::move(data[i]);
      }
    }

    // Recurse into each non-empty bucket using the next lower byte.
    constexpr size_t kLastOffset = Traits::kChunkSize - 1;
    for (size_t i = 0; i < kLastOffset; ++i) {
      sortImpl<IsHandlingSignBit>(
          buffer + c1[i],
          c1[i + 1] - c1[i],
          data + c1[i],
          pass - 1,
          counters2D,
          proj,
          depth + 1);
    }
    auto lastVal = c1[kLastOffset];
    sortImpl<IsHandlingSignBit>(
        buffer + lastVal,
        n - lastVal,
        data + lastVal,
        pass - 1,
        counters2D,
        proj,
        depth + 1);
  }

  /**
   * @brief Sequential MSD radix sort entry point.
   *
   * @details
   * Allocates a temporary buffer, then delegates to sortImpl for the
   * recursive partitioning.
   *
   * @param data      [in,out] Pointer to the input/output array
   * @param n         [in] Number of elements
   * @param allocator [in] Allocator for temporary buffer
   * @param proj      [in] Projection functor
   */
  static void sort(
      value_t* FOLLY_RESTRICT data,
      size_t n,
      const Allocator& allocator,
      Projection proj) {
    AllocatedBufferHolder<Allocator> holder(n, allocator);
    auto* FOLLY_RESTRICT buffer = holder.data();
    /*
     * 1) Find the maximum element,
     * 2) check whether the input is already sorted,
     * 3) compute the MSB byte index to determine the actual number of passes.
     *
     * Steps 1&2 need the raw KeyMap (value_t -> key_t), NOT the projected key,
     * because the projection may alter the key (e.g. sign-bit flip for floats).
     * Floating-point types with NaN existence need special handling via
     * makeMaxElementPred / makeIsSortedPred to account for NaN ordering.
     *
     * MSD is primarily beneficial for 64-bit keys where the high-order bytes
     * are zero for most practical values, allowing early bytes to be skipped.
     */
    constexpr bool kIsNaNsExistence = std::floating_point<key_t> &&
        Traits::kNaNsAssumption == RadixNaNsAssumption::Existence;

    auto maxPred = makeMaxElementPred<decltype(proj), kIsNaNsExistence>(proj);

    // Build the ordering predicate from the raw key map.
    auto keyMap = proj.getKeyMap();
    auto orderPred = makeIsSortedPred<decltype(keyMap), kIsNaNsExistence>(
        keyMap,
        std::conditional_t < Traits::kSortOrder == RadixSortOrder::Ascending,
        std::less<>,
        std::greater < >> {});

    auto [it, sorted] =
        findMaxAndCheckSorted(data, data + n, maxPred, orderPred);

    if (sorted) {
      return;
    }
    // Use projected unsigned bit pattern so highestSetBitByteIndex sees
    // the correct byte positions for signed/floating-point types.
    const auto projectedMax =
        proj.template operator()<Traits::kIsHandlingSignBit>(*it);
    const size_t byteIndex = highestSetBitByteIndex(projectedMax);

    const size_t passes = (byteIndex + 1) * CHAR_BIT / Traits::kChunkBits;

    const size_t countersTotal = passes * Traits::kChunkSize;
    const size_t countersBytes = countersTotal * sizeof(counter_t);

    // Allocate the 2-D counter array once, avoiding repeated heap
    // allocations during MSD recursion.  sortImpl uses `depth` to
    // index into the appropriate row; each row is zero-filled on
    // first use by std::fill_n inside sortImpl.
    MallocaArray<counter_t> countersStorage(
        FOLLY_MALLOCA_THRESHOLD(countersBytes, Traits::kInlineN),
        countersTotal);
    auto counters2D = reinterpret_cast<counters2d_t>(countersStorage.data());

    if (passes != Traits::kNumPasses) {
      // Start from passes - 1 because sortImpl expects zero-based byte index.
      sortImpl<false>(data, n, buffer, passes - 1, counters2D, proj);
    } else {
      sortImpl<true>(data, n, buffer, passes - 1, counters2D, proj);
    }
  }
};
#endif // _OPENMP

// ---------------------------------------------------------------------------
// Internal dispatch function
// ---------------------------------------------------------------------------

/**
 * @brief Select and invoke the correct RadixSortImplDispatcher.
 *
 * @details
 * Handles three concerns that the public API should not expose:
 *  - Special case for bool elements: uses a dedicated `sortBool` routine
 *    for efficient bitwise sorting (O(n)) instead of the general radix sort.
 *  - Small-array fallback to insertion sort to avoid radix-sort overhead.
 *  - Compile-time policy validation (strategy and execution policy).
 *
 * Reverse-iterator adjustment is handled by the caller (public radixSort).
 *
 * @tparam RadixTraits    Compile-time sorting options traits
 * @tparam RandIter       Random-access iterator type
 * @tparam Allocator      Allocator type
 * @tparam Projection     Maps T to an unsigned integral key
 * @param first           [in] Start of range (inclusive)
 * @param last            [in] End of range (exclusive)
 * @param allocator       [in] Allocator for temporary buffer
 * @param proj            [in] Projection functor (unsigned key producer)
 */
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
constexpr void radixSortImpl(
    RandIter first,
    RandIter last,
    const Allocator& allocator,
    Projection proj) {
  static_assert(
      !(RadixTraits::kNaNsAssumption == RadixNaNsAssumption::NonExistence &&
        RadixTraits::kNaNsPosHandling != RadixNaNsPosHandling::Unhandled),
      "If you assumed there are no NaNs, then you shouldn't change the default settings for NaN positions.");

  static_assert(
      !(std::is_unsigned_v<typename RadixTraits::key_t> &&
        RadixTraits::kIsHandlingSignBit),
      "If the provided key extraction function returns an unsigned type, negative numbers are impossible.");

#ifndef _OPENMP
  static_assert(
      RadixTraits::kExecutionPolicy != RadixExecutionPolicy::Par,
      "Parallel policy requires _OPENMP defined");
#endif

  assert(first <= last && "Invalid iterator range: first must be <= last");

  if constexpr (std::same_as<std::iter_value_t<RandIter>, bool>) {
    sortBool<RandIter, RadixTraits::kSortOrder != RadixSortOrder::Ascending>(
        first, last);
    return;
  }

  const size_t size = last - first;
  /*
   * Fall back to insertion sort for very small arrays.  Radix sort's
   * overhead — histogram construction, prefix sums, and two full copies
   * per pair of passes — dominates for tiny inputs, making a simple
   * O(n^2) sort faster.
   */
  if (size <= kRadixSortThreshold) [[unlikely]] {
    if (RadixTraits::kSortOrder == RadixSortOrder::Ascending) {
      insertionSort(first, last, [proj](const auto& a, const auto& b) {
        return proj.template operator()<RadixTraits::kIsHandlingSignBit>(a) <
            proj.template operator()<RadixTraits::kIsHandlingSignBit>(b);
      });
    } else {
      insertionSort(first, last, [proj](const auto& a, const auto& b) {
        return proj.template operator()<RadixTraits::kIsHandlingSignBit>(b) <
            proj.template operator()<RadixTraits::kIsHandlingSignBit>(a);
      });
    }
    return;
  }
  RadixSortImplDispatcher<RadixTraits, RandIter, Allocator, Projection>::sort(
      &*first, size, allocator, proj);
}

} // namespace detail

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
 * @overloadbrief Radix sort a range [first, last).
 *
 * @details
 * Most general overload: accepts an arbitrary KeyMap that extracts an
 * arithmetic (integral or floating-point) key from each element, and
 * uses the provided allocator for temporary buffer storage.
 *
 * Example:
 * @code
 *   struct Item { double score; int id; };
 *   std::vector<Item> items = ...;
 *   constexpr RadixSortOptions opts{
 *       .SortStrategy = RadixSortStrategy::Msd,
 *       .SortOrder = RadixSortOrder::Descending};
 *   folly::radixSort<opts>(items.begin(), items.end(),
 *                          std::allocator<Item>{},
 *                          [](const Item& x) { return x.score; });
 * @endcode
 *
 * @tparam RadixOptions  Compile-time sorting options
 * @tparam RandIter      Random-access iterator type
 * @tparam Allocator     Allocator for temporary storage
 * @tparam KeyMap        Maps an element to an arithmetic key (default:
 * identity)
 * @param first          [in] Start of range (inclusive)
 * @param last           [in] End of range (exclusive)
 * @param allocator      [in] Allocator for temporary buffer
 * @param keyMap         [in] Key extraction functor
 */
template <
    RadixSortOptions RadixOptions,
    std::random_access_iterator RandIter,
    standard_allocator Allocator,
    detail::arithmetic_key_map<std::iter_value_t<RandIter>> KeyMap =
        detail::radix_key_map_fn>
constexpr void radixSort(
    RandIter first,
    RandIter last,
    const Allocator& allocator,
    KeyMap keyMap = {}) {
  using value_t = std::iter_value_t<RandIter>;

  auto projection =
      detail::radix_uint_projection.template
      operator()<RadixOptions.NaNsPosHandling>(
          std::in_place_type<value_t>, std::move(keyMap));

  /*
   * Reverse iterators invert the semantic order: sorting a reversed range
   * in ascending order is equivalent to sorting the original range in
   * descending order.  We adjust RadixOptions.SortOrder accordingly and
   * pass the underlying base iterators.
   */
  constexpr auto adjustedOptions = [] {
    RadixSortOptions opts = RadixOptions;
    opts.SortOrder =
        detail::RadixRealSortOrder<RandIter, RadixOptions.SortOrder>::value;
    return opts;
  }();

  using RadixTraits = detail::RadixSortTraits<adjustedOptions, value_t, KeyMap>;

  if constexpr (is_reverse_iterator_v<RandIter>) {
    detail::radixSortImpl<RadixTraits>(
        last.base(), first.base(), allocator, std::move(projection));
  } else {
    detail::radixSortImpl<RadixTraits>(
        first, last, allocator, std::move(projection));
  }
}

/**
 * @overloadbrief Radix sort with explicit options but defaulted allocator.
 *
 * @details
 * Convenience overload that uses std::allocator internally.
 *
 * @tparam RadixOptions  Compile-time sorting options
 * @tparam RandIter      Random-access iterator type
 * @tparam KeyMap        Maps an element to an arithmetic key (default:
 * identity)
 * @param first          [in] Start of range (inclusive)
 * @param last           [in] End of range (exclusive)
 * @param keyMap         [in] Key extraction functor
 */
template <
    RadixSortOptions RadixOptions,
    std::random_access_iterator RandIter,
    detail::arithmetic_key_map<std::iter_value_t<RandIter>> KeyMap =
        detail::radix_key_map_fn>
constexpr void radixSort(RandIter first, RandIter last, KeyMap keyMap = {}) {
  using value_t = typename std::iter_value_t<RandIter>;
  std::allocator<value_t> allocator{};
  radixSort<RadixOptions>(first, last, allocator, std::move(keyMap));
}

/**
 * @overloadbrief Radix sort with default options and defaulted allocator.
 *
 * @details
 * The simplest entry point: sorts `[first, last)` in ascending order
 * using LSD radix sort with 8-bit chunks and all other options at
 * their defaults.
 *
 * @tparam RandIter  Random-access iterator type
 * @tparam KeyMap    Maps an element to an arithmetic key (default: identity)
 * @param first      [in] Start of range (inclusive)
 * @param last       [in] End of range (exclusive)
 * @param keyMap     [in] Key extraction functor
 */
template <
    std::random_access_iterator RandIter,
    detail::arithmetic_key_map<std::iter_value_t<RandIter>> KeyMap =
        detail::radix_key_map_fn>
constexpr void radixSort(RandIter first, RandIter last, KeyMap keyMap = {}) {
  using value_t = typename std::iter_value_t<RandIter>;
  std::allocator<value_t> allocator{};
  constexpr RadixSortOptions radixOptions{};
  radixSort<radixOptions>(first, last, allocator, std::move(keyMap));
}

} // namespace folly
