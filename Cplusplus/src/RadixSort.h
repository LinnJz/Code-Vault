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

#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include <folly/ConstexprMath.h>
#include <folly/FollyMemset.h>
#include <folly/Traits.h>
#include <folly/container/Iterator.h>
#include <folly/container/MallocaArray.h>
#include <folly/lang/Assume.h>
#include <folly/memory/AllocatedBufferHolder.h>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace folly {
/**
 * @brief Radix sort traversal strategy.
 *
 * @details
 * - Lsd: Least Significant Digit first. Processes digits from least to most
 *   significant byte. Stable, iterative, and uses a single flat array.
 *
 * - Msd: Most Significant Digit first. Recursively partitions by the most
 *   significant digit. Forms a tree of sub-problems.
 *
 * # Memory layout & algorithm flow
 *
 * ## LSD (iterative, multi-pass)
 *
 *   Pass 1 (LSB)       Pass 2             Pass 3             Pass 4 (MSB)
 *   +----------------+ +----------------+ +----------------+ +----------------+
 *   | array -> buffer| | buffer -> array| | array -> buffer| | buffer -> array|
 *   +----------------+ +----------------+ +----------------+ +----------------+
 *   -> sorted
 *
 *   - Works on the whole array every pass.
 *   - Each pass scatters elements into 256 (or 65536) buckets.
 *   - No recursion, very good cache locality.
 *
 * ## MSD (recursive, depth-first)
 *
 *   Level 0 (byte 7)      Level 1 (byte 6)          Level 2 (byte 5) ...
 *   +-------------------------------------+
 *   |          whole array                |
 *   +-----------------+-------------------+
 *                     |
 *                     | split by most significant byte
 *              +------+------+
 *              |      |      |
 *        +-----+  +-----+  +-----+
 *        |bucket0|  |bucket1|  ... (up to 256 buckets)
 *        +--+---+  +--+---+
 *           |         |
 *           | recursive sort on next byte
 *        +--+---+  +--+---+
 *        |      |  |      |
 *       ...    ...  ...   ...
 *
 *   - High-order bits decide global order early.
 *   - Once a bucket contains <= threshold, fallback to LSD or insertion sort.
 *
 * # Performance considerations
 *
 * ## 32-bit keys (e.g., `uint32_t`)
 *
 *   - LSD with 8-bit chunks: only 4 passes, each pass scans the whole array.
 *   - Very predictable memory access, minimal recursion overhead.
 *   - Usually faster than MSD on modern CPUs (better cache and SIMD friendly).
 *
 * ## 64-bit keys (e.g., `uint64_t`)
 *
 *   - LSD with 8-bit chunks needs 8 passes -> more memory writes.
 *   - LSD with 16-bit chunks reduces passes to 4 but uses 65536 buckets,
 *     increasing histogram memory traffic.
 *   - MSD can be faster because the highest bytes often distribute data widely.
 *     After the first pass, sub-problems become much smaller, reducing total
 *     work. Recursion overhead is amortized over large inputs.
 *
 * ## Rule of thumb
 *
 *   - For 32-bit integers or small element types, prefer `Lsd`.
 *   - For 64-bit integers, especially with skewed distributions or when
 *     the most significant byte has high entropy, `Msd` may outperform `Lsd`.
 *   - Always benchmark with your actual data.
 */
enum class RadixSortStrategy { Lsd, Msd };

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
 * @brief Sort order for radix sort.
 */
enum class RadixSortOrder { Ascending, Descending };

/**
 * @brief Number of bits processed per radix pass.
 *
 * @details
 * Determines the bucket count as 2^RadixBitsPerPass.
 * - `Bits8`  : 8 bits per pass → 256 buckets, 4 passes for 32‑bit keys.
 * - `Bits16` : 16 bits per pass → 65536 buckets, 2 passes for 32‑bit keys.
 *
 * `Bits16` can be faster for large arrays because it halves the number of
 * passes, but the larger histogram incurs more memory traffic.
 */
enum class RadixBitsPerPass { Bits8 = 8, Bits16 = 16 };

/**
 * @brief Integer type used for bucket counters in the histogram.
 *
 * @details
 * - `UInt32` (32‑bit): Sufficient for arrays with fewer than 2³² elements.
 *   Uses less memory for the counter array.
 * - `UInt64` (64‑bit): Prevents overflow for arrays with 2³² or more elements,
 *   but doubles the memory footprint of the counter arrays compared to
 * `UInt32`.
 *
 * The choice affects both memory usage and the maximum array size that can
 * be sorted without counter overflow.
 */
enum class RadixHistogramStorageType { UInt32 = 32, UInt64 = 64 };

/**
 * @brief Controls where NaN values are placed in the sorted order
 *        when sorting floating-point types.
 *
 * @details
 * This enum is used together with `RadixNaNsAssumption`. When the assumption
 * is `Existence`, the sort will apply the selected positioning strategy.
 * When the assumption is `NonExistence`, this setting is ignored (no NaN
 * handling overhead).
 *
 * @warning Requires IEEE 754 compliance (std::numeric_limits<T>::is_iec559)
 *          to be guaranteed at compile time for predictable behavior.
 */
enum class RadixNaNsPosHandling {
  // No special treatment. NaN values may appear in arbitrary positions
  // because IEEE 754 does not define a total order for NaN.
  Unhandled,

  // All NaN values are placed at the very beginning of the sorted output,
  // before -inf (the smallest finite value).
  AtFirst,

  // All NaN values are placed at the very end, after +inf.
  AtLast
};

/**
 * @brief Informs the sort whether the input floating‑point array may contain
 *        NaN values.
 *
 * @details
 * The default is `NonExistence`, which provides the best performance by
 * skipping all NaN‑related logic. If `Existence` is chosen, the sort will
 * use the strategy specified by `RadixNaNsPosHandling` to position NaN
 * values.
 *
 * @note This enum applies only to floating‑point types (float, double, etc.).
 *       For integer types, it has no effect.
 */
enum class RadixNaNsAssumption {
  // NaN values may be present; the sort will use `RadixNaNsPosHandling`
  // to determine their final position.
  Existence,

  // Assume that no NaN values occur in the input. The sort will skip
  // all NaN‑handling code, and `RadixNaNsPosHandling` is ignored.
  NonExistence
};

/**
 * @brief Complete set of compile-time options for radix sort.
 *
 * @details
 * Customize sorting behavior by constructing an instance with the desired
 * settings and passing it as a template parameter to radixSort().
 *
 * Available tuning knobs:
 *  - `SortStrategy`        : selects LSD or MSD traversal strategy.
 *  - `ExecutionPolicy`     : sequential or parallel execution.
 *  - `SortOrder`           : ascending or descending order.
 *  - `BitsPerPass`         : number of bits processed per pass (8 or 16).
 *  - `HistogramStorageType`: integer type for bucket counters (32‑bit or
 * 64‑bit).
 *  - `NaNsPosHandling`     : placement of NaN values (if any are present).
 *  - `NaNsAssumption`      : whether the input may contain NaN values.
 *  - `MsdFallbackToLsdThreshold`: when an MSD sub‑problem falls below this
 *    size, the implementation falls back to LSD to avoid excessive recursion
 *    overhead.
 *  - `HistogramStackThresholdBytes`: maximum bytes to allocate on the stack
 *    for temporary histogram arrays. If the required size exceeds this value,
 *    heap allocation is used instead.
 */
struct RadixSortOptions {
  RadixSortStrategy SortStrategy = RadixSortStrategy::Lsd;
  RadixExecutionPolicy ExecutionPolicy = RadixExecutionPolicy::Seq;
  RadixSortOrder SortOrder = RadixSortOrder::Ascending;
  RadixBitsPerPass BitsPerPass = RadixBitsPerPass::Bits8;
  RadixHistogramStorageType HistogramStorageType =
      RadixHistogramStorageType::UInt32;
  RadixNaNsPosHandling NaNsPosHandling = RadixNaNsPosHandling::Unhandled;
  RadixNaNsAssumption NaNsAssumption = RadixNaNsAssumption::NonExistence;

  size_t MsdFallbackToLsdThreshold = 256U * 256U;
  size_t HistogramStackThresholdBytes = 16U * 1024U;
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
 * @details
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
 * @brief Specialisation for reverse iterators - inverts the sort order.
 *
 * @details
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
// Utilities for unsigned-type mapping
// ---------------------------------------------------------------------------

/**
 * @brief Identity key map - returns the input value unchanged.
 *
 * @details
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
 * @brief Provides bit masks and indices for the sign bit of an arithmetic type.
 *
 * @details
 * For signed integers and IEEE 754 floating-point numbers, the most significant
 * bit (MSB) indicates the sign. When sorting these types by their bitwise
 * representation (e.g., after reinterpreting as unsigned), negative values
 * would appear after positive values because the MSB is set for negatives. This
 * trait supplies the necessary masks and bit index to adjust the key for
 * correct ordering.
 *
 * @tparam T  An arithmetic type (integral or floating-point).
 *
 * The following static constants are provided:
 * - `uint_t`: the unsigned integer type with the same width as `T`.
 * - `kSignBitIndex`: the zero-based index of the sign bit (sizeof(T)*8 - 1).
 * - `kAllBitMask`: all bits set for `uint_t`.
 * - `kSignBitMask`: a mask with only the sign bit set.
 */
template <typename T>
struct SignBitTraits {
  static_assert(std::is_arithmetic_v<T>, "arithmetic type required");
  using type = T;
  using uint_t = folly::uint_bits_t<sizeof(T) * CHAR_BIT>;
  static constexpr size_t kSignBitIndex = sizeof(T) * CHAR_BIT - 1;
  static constexpr uint_t kAllBitMask = ~uint_t(0);
  static constexpr uint_t kSignBitMask = kAllBitMask << kSignBitIndex;
};

// ---------------------------------------------------------------------------
// Radix sort traits (compile-time parameter bundle)
// ---------------------------------------------------------------------------

/**
 * @brief Compile-time parameter pack combining user options, element type,
 *        and key projection.
 *
 * @details
 * This trait bundles all compile‑time decisions into a single type for use
 * by the radix sort implementation. It inherits `SignBitTraits<key_t>` to
 * obtain sign‑bit masks and indices for correcting the ordering of signed
 * integers and floating‑point numbers.
 *
 * The following core types are defined:
 * - `value_t` : element type (`T`)
 * - `key_t`   : result type of `KeyMap(value)` – must be an arithmetic type
 * - `uint_t`  : unsigned integer of the same width as `key_t`
 * - `hist_t`  : bucket counter type (32‑bit or 64‑bit, selected by options)
 *
 * All options from `RadixOptions` are re‑exported as `static constexpr` members
 * for convenient access elsewhere:
 * - `kSortStrategy`, `kExecutionPolicy`, `kSortOrder`
 * - `kNaNsPosHandling`, `kNaNsAssumption`
 * - `kHistogramStackThresholdBytes`, `kMsdFallbackToLsdThreshold`
 *
 * Key radix parameters are derived from `kBitsPerPass` (8 or 16) and the width
 * of `uint_t`:
 * - `kNumPasses` : number of passes to cover all bits of `uint_t`
 * - `kNumBuckets`: 2^kBitsPerPass
 * - `kRadixMask` : `kNumBuckets - 1` (used to extract a digit)
 *
 * Traversal direction (`kFirstPass`, `kLastPass`) depends on the strategy:
 * - LSD starts at pass 0 and goes up to `kNumPasses-1`
 * - MSD starts at `kNumPasses-1` and goes down to 0
 *
 * Sign‑bit handling is applied on the first processed pass if:
 * - the strategy is MSD (or there is only one pass), **and**
 * - the key type is signed.
 * This is controlled by `kIsSignHandlingOnFirstPass`.
 *
 * Lastly, `kPassIndices` provides an `std::index_sequence<0, ...,
 * kNumPasses-1>` for compile‑time loop unrolling.
 *
 * @tparam RadixOptions  A `RadixSortOptions` instance (compile‑time constants)
 * @tparam T             Type of the elements to be sorted
 * @tparam KeyMap        A callable `T -> arithmetic type` that extracts
 *                       the sort key from each element.
 */
template <
    RadixSortOptions RadixOptions,
    typename T,
    detail::arithmetic_key_map<T> KeyMap>
struct RadixSortTraits : SignBitTraits<std::invoke_result_t<KeyMap, T>> {
  // Re‑export user options
  static constexpr auto kSortStrategy = RadixOptions.SortStrategy;
  static constexpr auto kExecutionPolicy = RadixOptions.ExecutionPolicy;
  static constexpr auto kSortOrder = RadixOptions.SortOrder;
  static constexpr auto kNaNsPosHandling = RadixOptions.NaNsPosHandling;
  static constexpr auto kNaNsAssumption = RadixOptions.NaNsAssumption;
  static constexpr auto kHistogramStackThresholdBytes =
      RadixOptions.HistogramStackThresholdBytes;
  static constexpr auto kMsdFallbackToLsdThreshold =
      RadixOptions.MsdFallbackToLsdThreshold;

  // Core type aliases
  using value_t = T;
  using key_t = std::invoke_result_t<KeyMap, T>;
  using uint_t = typename SignBitTraits<key_t>::uint_t;
  using hist_t = std::conditional_t<
      RadixOptions.HistogramStorageType == RadixHistogramStorageType::UInt32,
      std::uint32_t,
      std::uint64_t>;

  // Radix parameters
  static constexpr size_t kBitsPerPass =
      static_cast<std::underlying_type_t<RadixBitsPerPass>>(
          RadixOptions.BitsPerPass);
  static constexpr size_t kNumPasses = sizeof(uint_t) * CHAR_BIT / kBitsPerPass;
  static constexpr size_t kNumBuckets = constexpr_pow(2, kBitsPerPass);
  static constexpr size_t kRadixMask = kNumBuckets - 1U;

  // Pass order (for LSD vs MSD)
  static constexpr size_t kFirstPass =
      kSortStrategy == RadixSortStrategy::Lsd ? 0 : kNumPasses - 1;
  static constexpr size_t kLastPass =
      kSortStrategy == RadixSortStrategy::Lsd ? kNumPasses - 1 : 0;

  // Whether sign correction is applied on the first pass
  static constexpr bool kIsSignHandlingOnFirstPass =
      (kSortStrategy == RadixSortStrategy::Msd || kNumPasses == 1) &&
      std::is_signed_v<key_t>;

  // Compile‑time sequence of all pass indices
  static constexpr auto kPassIndices = std::make_index_sequence<kNumPasses>{};
};

// ---------------------------------------------------------------------------
// Unsigned integer projection
// ---------------------------------------------------------------------------

/**
 * @brief A callable that projects elements to unsigned integers suitable for
 *        radix sorting.
 *
 * @details
 * This function object composes the user‑supplied `KeyMap` with a bit‑level
 * transformation that maps signed integers and IEEE 754 floating‑point values
 * into a total order that is compatible with radix sort. The transformation
 * accounts for sign bits, NaN placement, and the ability to defer sign‑bit
 * correction to a specific pass.
 *
 * ## Workflow
 * 1. Extract an arithmetic key from the element using the stored `KeyMap`.
 * 2. If the key is signed, flip the sign bit (so negatives sort before
 *    positives) – optionally controlled by `IsSignBitHandling`.
 * 3. If the key is a floating‑point type, convert its bit representation to a
 *    total order (including optional NaN repositioning).
 *
 *    value_t(any type)        ->  KeyMap(user-defined function)        ->
 *    key_t  (arithmetic type) ->  Projection(radix_uint_projection_fn) ->
 *    uint_t (unsigned type)
 *
 * ## Deferred sign‑bit handling
 * For LSD radix sort, the sign bit (MSB) is only relevant in the final pass.
 * By setting `IsSignBitHandling = true` only in that pass, the transformation
 * saves several bitwise operations per element for all earlier passes,
 * improving performance. For MSD, sign handling is applied on the first pass.
 *
 * The `Projector` nested template provides two key members:
 * - `operator()<IsSignBitHandling>(const auto&)` – computes the unsigned key.
 * - `getKeyMap()` – retrieves the stored key extraction functor.
 *
 * @note This struct is intended to be instantiated via its `operator()` which
 *       takes an `std::in_place_type_t<T>` tag and a `KeyMap` rvalue.
 */
struct radix_uint_projection_fn {
  /**
   * @brief Projector object that holds the key extraction functor and
   *        provides the unsigned projection logic.
   *
   * @tparam NaNsPosHandling  Strategy for NaN placement (used only for floats).
   * @tparam Arithmetic       The arithmetic type returned by `KeyMap`.
   * @tparam KeyMap           The stored key extraction functor type.
   */
  template <
      RadixNaNsPosHandling NaNsPosHandling,
      typename Arithmetic,
      typename KeyMap>
  struct Projector {
    KeyMap km_;

    using Traits = SignBitTraits<Arithmetic>;
    using uint_t = typename Traits::uint_t;

    /**
     * @brief Transforms an arithmetic key into an unsigned integer.
     *
     * @tparam IsSignBitHandling  Whether to apply sign‑bit correction.
     *         Defaults to `true` for signed types, `false` for unsigned.
     *
     * @param key  The arithmetic value to transform.
     * @return     An unsigned integer key with the correct order.
     *
     * @details
     * - For unsigned types: identity.
     * - For signed integers: if `IsSignBitHandling` is true, flip the sign bit
     *   so that the most negative maps to 0 and the most positive to all‑ones.
     *   Otherwise, return the raw two's‑complement representation.
     * - For floating‑point: IEEE 754 is assumed (checked at compile time).
     *   The following transformation ensures lexicographic ordering:
     *
     *   IEEE 754 floats are almost lexicographically ordered by their
     *   bit representation, except:
     *    - The sign bit inverts ordering for negative floats (similar to
     *      signed integers).
     *    - Negative floats sort in reverse order (e.g., -1.0 > -2.0 in
     *      bit representation).
     *
     *   The fix: for negative values (signMask == all-ones), flip every
     *   bit except the sign bit itself.  For positive values (signMask
     *   == 0), flip only the sign bit.  This produces the mapping:
     *
     *     -inf  -> 0x0000...
     *     -0.0  -> 0x7FFF...  (just below +0.0)
     *     +0.0  -> 0x8000...
     *     +inf  -> 0xFFFF...
     *
     *   - NaN: if `NaNsPosHandling` is `AtFirst` or `AtLast`, NaN is
     *     assigned to the smallest or largest possible unsigned value.
     *     If `Unhandled`, NaN remains in its original bit pattern.
     *   - When `AtFirst` is active, NaNs occupy slot 0, so all finite values
     *     are shifted by +1 to reserve that slot. This shift is safe because
     *     the all‑ones bit pattern (exponent all‑1 and mantissa non‑zero) is
     *     guaranteed to be a NaN per IEEE 754. Since NaNs have already been
     *     handled and mapped to 0 (or max) before this step, no finite
     *     floating‑point number (including ±inf, ±0, normals, subnormals) can
     *     have the all‑ones pattern. Therefore, `base + 1U` never overflows.
     *
     * @note
     * About @tparam 'IsSignBitHandling', this separation exists because for
     * signed integers and floats the sign bit is the most significant bit. In
     * LSD order the MSB is only considered during the very last pass, so we can
     * skip the correction logic for all earlier passes. Compiler Explorer
     * confirms that this saves several instructions per element:
     * https://compiler-explorer.com/z/j56PxGK1s
     */
    template <bool IsSignBitHandling = !std::unsigned_integral<Arithmetic>>
    static constexpr auto transform(Arithmetic key) noexcept -> uint_t {
      if constexpr (
          std::same_as<Arithmetic, bool> ||
          std::unsigned_integral<Arithmetic>) {
        // Unsigned types need no transformation.
        return key;
      } else if constexpr (std::signed_integral<Arithmetic>) {
        // Two's complement signed integers: flip sign bit for correct order.
        return IsSignBitHandling
            ? static_cast<uint_t>(key) ^ Traits::kSignBitMask
            : static_cast<uint_t>(key);
      } else if constexpr (std::floating_point<Arithmetic>) {
        static_assert(
            !(NaNsPosHandling != RadixNaNsPosHandling::Unhandled &&
              !std::numeric_limits<Arithmetic>::is_iec559),
            "NaN pos handling requires IEEE 754 floating-point");

        // Handle NaNs if requested.
        if constexpr (NaNsPosHandling != RadixNaNsPosHandling::Unhandled) {
          if (std::isnan(key)) {
            // Map NaN to either the smallest or largest possible key so
            // that it sorts to the beginning or end of the sequence.
            return NaNsPosHandling == RadixNaNsPosHandling::AtFirst
                ? uint_t(0)
                : std::numeric_limits<uint_t>::max();
          }
        }
        const auto v = std::bit_cast<uint_t>(key);

        // signMask is 0 for positive floats, ~0 for negative.
        // Computed as 0 - (v >> kSignBitIndex) with unsigned wrap.
        const auto signMask = (uint_t(0) - (v >> Traits::kSignBitIndex));

        // Apply sign‑bit flip and, for negatives, invert all bits except sign.
        uint_t base = IsSignBitHandling
            ? (v ^ signMask) | (Traits::kSignBitMask & ~signMask)
            : (v ^ signMask);

        // Shift finite values up by one if NaN occupies slot 0.
        if constexpr (NaNsPosHandling == RadixNaNsPosHandling::AtFirst) {
          base += 1U;
        }
        return base;
      } else {
        assume_unreachable();
      }
    }

    /**
     * @brief Projects an element to its unsigned sort key.
     *
     * @tparam IsSignBitHandling  Whether to apply sign‑bit correction.
     * @param x  The element to project.
     * @return   The unsigned integer key.
     */
    template <bool IsSignBitHandling = !std::unsigned_integral<Arithmetic>>
    constexpr auto operator()(const auto& x) const noexcept -> uint_t {
      return transform<IsSignBitHandling>(km_(x));
    }

    // Returns a const reference to the stored key extraction functor.
    constexpr auto getKeyMap() const noexcept -> const KeyMap& { return km_; }
  };

  /**
   * @brief Creates a Projector instance for a given element type and KeyMap.
   *
   * @tparam NaNsPosHandling  NaN placement strategy (must match the call site).
   * @tparam T                The element type (used only for type deduction).
   * @param keyMap            The key extraction functor (will be decay‑copied).
   * @return A Projector object that stores `keyMap` and provides projection.
   */
  template <RadixNaNsPosHandling NaNsPosHandling, typename T, typename KeyMap>
  constexpr auto operator()(
      std::in_place_type_t<T>, KeyMap&& keyMap) const noexcept {
    using arithmetic_t = std::invoke_result_t<KeyMap, T>;
    return Projector<NaNsPosHandling, arithmetic_t, std::decay_t<KeyMap>>{
        std::forward<KeyMap>(keyMap)};
  }
};
inline constexpr radix_uint_projection_fn radix_uint_projection{};

// ---------------------------------------------------------------------------
// Radix digit extraction
// ---------------------------------------------------------------------------

/**
 * @brief Extracts the bucket index for a given key during a specific radix
 * pass.
 *
 * @details
 * Shifts the key right by `pass * kBitsPerPass` and masks with `kRadixMask` to
 * isolate the current digit. For descending order, the key is bitwise‑inverted
 * before extraction so that larger original keys map to lower bucket indices.
 *
 * The function is force‑inlined in non‑debug builds to reduce call overhead
 * in the inner sorting loops.
 *
 * @tparam RadixTraits  Compile‑time radix parameters (must provide
 *                      `kBitsPerPass`, `kRadixMask`, and `kSortOrder`).
 * @param x             The projected unsigned key value (of type
 *                      `RadixTraits::uint_t`).
 * @param pass          The current pass index (0 = least significant digit).
 * @return              The bucket index in the range `[0, kNumBuckets)`.
 */
template <typename RadixTraits>
#if !defined(DEBUG) && !defined(_DEBUG)
FOLLY_ALWAYS_INLINE
#endif
    constexpr auto
    nthRadix(typename RadixTraits::uint_t x, size_t pass) noexcept -> size_t {
  if constexpr (RadixTraits::kSortOrder == RadixSortOrder::Ascending) {
    return (x >> RadixTraits::kBitsPerPass * pass) & RadixTraits::kRadixMask;
  } else if constexpr (RadixTraits::kSortOrder == RadixSortOrder::Descending) {
    return ((~x) >> RadixTraits::kBitsPerPass * pass) & RadixTraits::kRadixMask;
  } else {
    assume_unreachable();
  }
}

/**
 * @brief Computes the index of the most significant set bit (MSB) of an
 *        arithmetic value.
 *
 * @details
 * This function is primarily used to determine the actual number of radix
 * passes required for sorting: if all higher‑order bits above a certain
 * position are zero, those passes can be skipped entirely, reducing
 * unnecessary work.
 *
 * For integral types, the value is treated as its two's‑complement bit pattern
 * (i.e., reinterpreted as unsigned). For floating‑point types, the IEEE 754
 * bit representation is used. If the input is zero, the function returns 0.
 *
 * @tparam T  An arithmetic type (integral or floating‑point).
 * @param value  The input value.
 * @return     The zero‑based index of the most significant 1‑bit; 0 if `value`
 *             is zero.
 *
 * @note This function is intended for internal use by the radix sort
 *       implementation to optimize pass count. It is force‑inlined in
 *       non‑debug builds.
 */
template <typename T>
#if !defined(DEBUG) && !defined(_DEBUG)
FOLLY_ALWAYS_INLINE
#endif
    constexpr auto
    highestOneBitIndex(T value) noexcept -> size_t {
  static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
  using uint_t = uint_bits_t<sizeof(T) * CHAR_BIT>;

  uint_t bits;
  if constexpr (std::is_integral_v<T>) {
    // Preserve two's complement bit pattern
    bits = static_cast<uint_t>(value);
  } else if constexpr (std::is_floating_point_v<T>) {
    bits = std::bit_cast<uint_t>(value);
  } else {
    assume_unreachable();
  }

  return std::bit_width(static_cast<uint_t>(bits | uint_t(1))) - 1;
}

// ---------------------------------------------------------------------------
// Radix parallel computing utils
// ---------------------------------------------------------------------------

/**
 * @brief Partition information for distributing work among OpenMP threads.
 *
 * @details
 * Given a total of `n` elements and `threads` workers, the distribution is:
 *   - `step = n / threads` (base chunk size per thread)
 *   - `remain = n % threads` (first `remain` threads get one extra element)
 *
 * This ensures each thread processes either `step` or `step + 1` elements,
 * all chunks are contiguous in memory, and the load is balanced.
 *
 * @note This struct is typically computed once at the start of a parallel
 *       radix sort pass and used to initialize per‑thread ranges.
 */
struct ThreadChunkInfo {
  uint32_t threads; ///< Total number of OpenMP threads.
  uint32_t remain; ///< Number of threads that process one extra element.
  size_t step; ///< Base chunk size (floor(n / threads)).
};

/**
 * @brief Statistics for a chunk of data processed by a single thread during
 *        parallel radix sort.
 *
 * @tparam T The type of elements being sorted. Must support comparison
 *           operators (<, <=, etc.) and be copyable.
 *
 * @details
 * After a thread finishes sorting its assigned contiguous chunk, it returns
 * these statistics to indicate:
 * - Whether the chunk is internally sorted (in non‑decreasing order).
 * - The minimum and maximum values within the chunk.
 *
 * These per‑chunk statistics are later combined across all threads to
 * determine if the entire array is sorted. If all chunks are sorted and the
 * maximum of each chunk is ≤ the minimum of the next chunk, then the whole
 * array is sorted.
 *
 * @note This struct is used only when parallel execution is enabled.
 */
template <typename T>
struct ThreadChunkStats {
  bool isSorted; ///< True if the chunk is sorted in ascending order.
  T minVal; ///< Minimum value in the chunk.
  T maxVal; ///< Maximum value in the chunk.
};

/**
 * @brief Computes the inclusive start and exclusive end indices of the chunk
 *        assigned to a given thread.
 *
 * @details
 * The allocation strategy distributes elements as follows:
 *   - The first `remain` threads (`tid` in [0, `remain`)) receive `step + 1`
 *     elements each, starting at `tid * (step + 1)`.
 *   - The remaining threads (`tid` in [`remain`, `numThreads`)) receive
 *     `step` elements each, starting at `remain * (step + 1) +
 *     (tid - remain) * step`.
 *
 * The resulting intervals are contiguous, non‑overlapping, and cover exactly
 * the range `[0, n)` (where `n = step * numThreads + remain`). This ensures
 * balanced load with at most a one‑element difference between any two threads.
 *
 * @param tid    The thread ID (0‑based, within `[0, numThreads)`).
 * @param step   The base chunk size (`n / numThreads`).
 * @param remain The number of threads that get an extra element
 *               (`n % numThreads`).
 * @return       A `std::pair<size_t, size_t>` where `first` is the inclusive
 *               start index and `second` is the exclusive end index.
 *
 * @note This function is force‑inlined in non‑debug builds for performance.
 */
#if !defined(DEBUG) && !defined(_DEBUG)
FOLLY_ALWAYS_INLINE
#endif
constexpr auto computeThreadChunkRange(
    size_t tid, size_t step, size_t remain) noexcept
    -> std::pair<size_t, size_t> {
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

// ---------------------------------------------------------------------------
// Radix sort fallback sorting mode
// ---------------------------------------------------------------------------

/// Threshold below which insertion sort is used instead of radix sort.
inline constexpr size_t kRadixSortThreshold = 64U;

/**
 * @brief Partitions boolean values into false/true groups in a single O(n)
 * pass.
 *
 * @details
 * Iterates once over the range, moving `true` values to one end and `false`
 * values to the other. This is significantly faster than using a general radix
 * sort for boolean data.
 *
 * @tparam RandIter    A random-access iterator type.
 * @tparam Descending  If `true`, `true` values are placed before `false`;
 *                     otherwise, `false` comes first.
 * @param first        Inclusive start of the range.
 * @param last         Exclusive end of the range.
 *
 * @note The algorithm is stable in the sense that the relative order of equal
 *       values is preserved, but for booleans stability is irrelevant.
 */
template <std::random_access_iterator RandIter, bool Descending>
constexpr void sortBool(RandIter first, RandIter last) {
  size_t cnt = 0;
  for (auto curr = first; curr < last; ++curr) {
    const auto v = *curr;
    *curr = Descending ? false : true;
    *(first + cnt) = v;
    cnt += static_cast<size_t>(Descending ? v : !v);
  }
}

/**
 * @brief Insertion sort over a bidirectional iterator range.
 *
 * @details
 * Used as a fallback when the input size is below `kRadixSortThreshold`.
 * Insertion sort has O(n²) worst‑case complexity but very low constant
 * factors and excellent performance on nearly‑sorted data, making it faster
 * than radix sort for very small arrays. It also avoids the memory allocation
 * and histogram overhead of radix sort.
 *
 * @tparam BiIter   A bidirectional iterator type.
 * @tparam Compare  A strict weak ordering predicate (default:
 * `std::ranges::less`).
 * @param first     Inclusive start of the range.
 * @param last      Exclusive end of the range.
 * @param comp      Comparison functor (default: `{}`).
 */
template <
    std::bidirectional_iterator BiIter,
    std::indirect_strict_weak_order<BiIter> Compare = std::ranges::less>
constexpr void insertionSort(BiIter first, BiIter last, Compare comp = {}) {
  if (first == last) [[unlikely]] {
    return;
  }
  for (auto current = std::next(first); current != last; ++current) {
    const auto value = std::move(*current);
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
 * @brief Fallback LSD radix sort used by MSD for small sub‑problems.
 *
 * @details
 * This function implements a standard LSD (least significant digit) radix sort
 * that is invoked by MSD's `sortImpl` when the sub‑problem size drops below
 * `MsdFallbackToLsdThreshold`, or when the remaining byte depth is ≤ 1.
 *
 * It performs a fixed number of passes (`passes`) over the data. Each pass
 * consists of three phases:
 *   1. **Histogram** – count occurrences of each digit (bucket).
 *   2. **Prefix sum** – convert counts to starting offsets.
 *   3. **Scatter** – move elements into the buffer using backward iteration
 *      to maintain stability.
 *
 * The same `hist1d` array is reused across all passes. The `data` and `buffer`
 * pointers are swapped after each pass. After all passes, if `(depth + passes)`
 * is odd, the final result is moved from `data` to `buffer` to ensure the
 * sorted data resides in the original `data` pointer (since the caller expects
 * the result in `data`). This odd‑even logic handles the alternating buffer
 * arrangement used by the MSD recursion.
 *
 * @tparam RadixTraits        Compile‑time radix traits (provides `kNumBuckets`,
 *                            `uint_t`, `hist_t`, etc.).
 * @tparam Projection         A functor that maps `value_t` to an unsigned key
 *                            (of type `RadixTraits::uint_t`).
 * @tparam IsSignBitHandling  Whether to apply sign‑bit correction during key
 *                            extraction (default: `false`).
 *
 * @param data    [in,out] Pointer to the sub‑array to sort. After the function
 *                         returns, the sorted data may reside in `data` or
 *                         `buffer` depending on parity; the caller must handle
 *                         the final move if necessary.
 * @param n       Number of elements in the sub‑array.
 * @param buffer  Temporary buffer of size at least `n`.
 * @param passes  Number of LSD radix passes to perform.
 * @param hist1d  [in,out] 1‑D counter array of size `RadixTraits::kNumBuckets`,
 *                         reused across passes (must be pre‑allocated).
 * @param proj    Projection functor.
 * @param depth   Current recursion depth (used for parity calculation; default
 * 0).
 *
 * @note This function is intended for internal use by MSD radix sort and is
 *       not part of the public API.
 */
template <
    typename RadixTraits,
    typename Projection,
    bool IsSignBitHandling = false>
void integerRadixSort(
    typename RadixTraits::value_t* FOLLY_RESTRICT data,
    const size_t n,
    typename RadixTraits::value_t* FOLLY_RESTRICT buffer,
    const size_t passes,
    typename RadixTraits::hist_t* FOLLY_RESTRICT hist1d,
    Projection proj,
    size_t depth = 0) {
  if (n == 0) {
    return;
  }

  for (size_t pass = 0; pass < passes; ++pass) {
    using hist_t = typename RadixTraits::hist_t;
    __folly_memset(hist1d, 0, RadixTraits::kNumBuckets * sizeof(hist_t));

    for (size_t i = 0; i < n; i++) {
      auto x = proj.template operator()<IsSignBitHandling>(data[i]);
      ++hist1d[nthRadix<RadixTraits>(x, pass)];
    }

    for (size_t i = 1; i < RadixTraits::kNumBuckets; i++) {
      hist1d[i] += hist1d[i - 1];
    }

    for (size_t i = n; i-- > 0;) {
      auto x = proj.template operator()<IsSignBitHandling>(data[i]);
      buffer[--hist1d[nthRadix<RadixTraits>(x, pass)]] = std::move(data[i]);
    }
    std::swap(data, buffer);
  }

  if (depth + passes & 1) {
    std::move(data, data + n, buffer);
  }
}

// ---------------------------------------------------------------------------
// Implementations of Radix Sort in Both LSD and MSD Modes, with Sequential and
// Parallel Versions
// ---------------------------------------------------------------------------

// 分发器前向声明，传入RadixSortTraits结构体，随机迭代器，内存分配器，投影函数
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    folly::standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
struct RadixSortImplDispatcher;

// ---------------------------------------------------------------------------
// LSD + Seq
// ---------------------------------------------------------------------------

/**
 * @brief Sequential LSD radix sort implementation.
 *
 * @details
 * This specialization is selected when `RadixTraits::kSortStrategy == Lsd` and
 * `kExecutionPolicy == Seq`. It performs a stable LSD (least significant digit)
 * radix sort using a single temporary buffer and a 2‑D histogram that stores
 * bucket counts for all passes simultaneously.
 *
 * ## Algorithm overview
 *
 * 1. **Histogram construction** – A single pass over the input builds per‑pass
 *    bucket counts for every radix pass in one go. This reduces memory traffic
 *    compared to building histograms pass‑by‑pass.
 * 2. **Sortedness detection** – During the same scan, the input is checked for
 *    being already sorted; if so, all subsequent work is skipped.
 * 3. **Prefix sums** – For each pass, bucket counts are converted to starting
 *    offsets, and the number of non‑empty buckets is recorded to skip passes
 *    where all keys share the same digit.
 * 4. **Scatter** – For each pass, elements are scattered into the buffer using
 *    the offsets (with backward iteration for stability). The source and
 *    destination pointers are swapped after each pass.
 * 5. **Final move‑back** – If the sorted data ends up in the temporary buffer,
 *    it is moved back to the original array to satisfy the post‑condition.
 *
 * ## Performance optimisations
 * - All histograms are built in a single scan, improving cache locality.
 * - Sign‑bit correction (`IsSignBitHandling = true`) is applied only on the
 *   last pass (for LSD), saving several instructions per element on earlier
 *   passes.
 * - Passes with one or zero non‑empty buckets are skipped entirely.
 * - Histogram memory can be allocated on the stack (via `MallocaArray`) if
 *   its size is below `kHistogramStackThresholdBytes`; otherwise it falls
 *   back to heap allocation.
 *
 * @tparam RadixTraits   Compile‑time radix traits (options + derived
 * constants).
 * @tparam RandIter      Random‑access iterator type.
 * @tparam Allocator     Allocator for the temporary element buffer.
 * @tparam Projection    A functor that maps `value_t` to an unsigned integral
 * key.
 */
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    folly::standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
  requires(
      RadixTraits::kSortStrategy == RadixSortStrategy::Lsd &&
      RadixTraits::kExecutionPolicy == RadixExecutionPolicy::Seq)
struct RadixSortImplDispatcher<RadixTraits, RandIter, Allocator, Projection>
    : RadixTraits {
  // Re‑export frequently used types and constants from the base trait.
  using Traits = RadixTraits;
  using Base = RadixTraits;
  using Base::kHistogramStackThresholdBytes;
  using Base::kLastPass;
  using Base::kNumBuckets;
  using Base::kNumPasses;
  using Base::kPassIndices;
  using Base::kSortOrder;
  using typename Base::hist_t;
  using typename Base::key_t;
  using typename Base::value_t;
  using hist2d_t = hist_t (*)[kNumBuckets];

  /**
   * @brief Builds all per‑pass histograms in one scan and checks sortedness.
   *
   * @details
   * This function iterates once over the input. For each element, it computes
   * the projected key with full sign‑bit handling and increments the counter
   * for every pass in parallel (using a compile‑time unrolled loop over
   * `kPassIndices`). At the same time, it verifies monotonicity of the keys:
   * if the entire sequence is already in the desired order, the caller can
   * skip the scatter phase entirely.
   *
   * Because this scan touches every cache line anyway, the sortedness check
   * comes at negligible extra cost.
   *
   * @param data     [in] Pointer to the input array.
   * @param n        [in] Number of elements.
   * @param hist2d   [out] 2‑D counter array of size
   * `[kNumPasses][kNumBuckets]`. It must be zero‑initialised on entry; on exit
   * it contains the raw bucket counts for each pass.
   * @param proj     [in] Projection functor.
   * @return `true` if the input is already sorted (according to `kSortOrder`),
   *         `false` otherwise.
   */
  static constexpr auto buildHistogram(
      const value_t* data, const size_t n, hist2d_t hist2d, Projection proj)
      -> bool {
    // Choose the comparison that indicates "not out of order":
    // for ascending, we expect prev ≤ key; for descending, prev ≥ key.
    auto comp = std::conditional_t<
        kSortOrder == RadixSortOrder::Ascending,
        std::greater_equal<>,
        std::less_equal<>>{};
    auto prev = kSortOrder == RadixSortOrder::Ascending
        ? std::numeric_limits<key_t>::lowest()
        : std::numeric_limits<key_t>::max();

    const auto& keyMap = proj.getKeyMap();
    bool isSorted = true;

    for (size_t i = 0; i < n; ++i) {
      const auto key = keyMap(data[i]);
      isSorted = isSorted && comp(key, prev);
      prev = key;

      // Unroll over all passes: increment the bucket counter for each pass.
      [&]<size_t... Is>(std::index_sequence<Is...>) {
        ((++hist2d[Is][nthRadix<Traits>(Projection::transform(key), Is)]), ...);
      }(kPassIndices);
    }
    return isSorted;
  }

  /**
   * @brief Converts per‑pass bucket counts to prefix‑sum offsets.
   *
   * @details
   * For each pass, this function performs an in‑place prefix sum over the
   * bucket counts, so that `hist2d[pass][i]` becomes the starting index of
   * bucket `i` in the output. It also counts the number of non‑empty buckets
   * per pass; passes with fewer than 2 occupied buckets can be skipped during
   * the scatter phase because moving elements would not change their order.
   *
   * @tparam Is   Compile‑time index sequence `[0, kNumPasses)`.
   * @param hist2d  [in,out] 2‑D array `[kNumPasses][kNumBuckets]`; on entry
   *                it holds raw counts, on exit it holds prefix sums.
   * @return An array of size `kNumPasses` where element `p` is the number of
   *         non‑empty buckets in pass `p`.
   */
  template <size_t... Is>
  static auto prefixSum(hist2d_t hist2d, std::index_sequence<Is...>)
      -> std::array<size_t, sizeof...(Is)> {
    std::array<size_t, sizeof...(Is)> nonEmptyCounts;
    auto process = [&](size_t index) -> void {
      nonEmptyCounts[index] = (hist2d[index][0] != 0);
      for (size_t i = 1; i < kNumBuckets; ++i) {
        nonEmptyCounts[index] += (hist2d[index][i] != 0);
        hist2d[index][i] += hist2d[index][i - 1];
      }
    };
    (process(Is), ...);
    return nonEmptyCounts;
  }

  /**
   * @brief Performs the stable scatter phase for all passes.
   *
   * @details
   * This function iterates over each pass in LSD order (from 0 to `kLastPass`).
   * For each pass, if it has more than one non‑empty bucket, it scatters the
   * elements into the buffer using the prefix‑sum offsets. The scatter walks
   * backward through the input (`i = n-1 … 0`) to preserve stability.
   *
   * **Sign‑bit handling**: For all passes except the last one, the projection
   * is called with `IsSignBitHandling = false` – this skips the sign‑bit
   * correction because the sign bit (MSB) is only relevant in the most
   * significant digit. On the last pass, the regular `operator()` (which
   * applies sign‑bit handling depending on the key type) is used.
   *
   * After each scatter, the roles of `data` and `buffer` are swapped so that
   * the next pass reads from the newly arranged order.
   *
   * @param data            [in,out] Pointer to the current source array
   *                        (will be swapped with `buffer` after each pass).
   * @param n               [in] Number of elements.
   * @param buffer          [out] Temporary buffer of size at least `n`.
   * @param nonEmptyCounts  [in] Per‑pass non‑empty bucket counts (from
   *                        `prefixSum`); passes with ≤1 bucket are skipped.
   * @param hist2d          [in,out] 2‑D prefix‑sum offsets; the offsets are
   *                        decremented during scatter as elements are placed.
   * @param proj            [in] Projection functor.
   * @return Pointer to the array that contains the sorted data after all
   *         passes. This may be either the original `data` or the `buffer`,
   *         depending on the parity of the number of passes actually executed.
   */
  static constexpr auto doScatter(
      value_t* FOLLY_RESTRICT data,
      const size_t n,
      value_t* FOLLY_RESTRICT buffer,
      const std::array<size_t, kNumPasses>& nonEmptyCounts,
      hist2d_t hist2d,
      Projection proj) -> value_t* {
    // All passes except the last one: sign‑bit correction is not needed.
    for (size_t pass = 0; pass < kLastPass; ++pass) {
      if (nonEmptyCounts[pass] <= 1) {
        continue; // All elements have the same digit; no reordering needed.
      }
      for (size_t i = n; i-- > 0;) {
        const auto x = proj.template operator()<false>(data[i]);
        buffer[--hist2d[pass][nthRadix<Traits>(x, pass)]] = std::move(data[i]);
      }
      std::swap(data, buffer);
    }

    // Last pass: apply full sign‑bit handling if the key type is signed.
    if (nonEmptyCounts[kLastPass] > 1) {
      for (size_t i = n; i-- > 0;) {
        const auto x = proj(data[i]); // uses the default `IsSignBitHandling`
        buffer[--hist2d[kLastPass][nthRadix<Traits>(x, kLastPass)]] =
            std::move(data[i]);
      }
      std::swap(data, buffer);
    }
    return data;
  }

  /**
   * @brief Main entry point for sequential LSD radix sort.
   *
   * @details
   * This function orchestrates the entire sorting process:
   *
   * 1. **Allocate temporary buffers**:
   *    - An element buffer of size `n` is obtained from the provided allocator.
   *    - A 2‑D histogram of size `kNumPasses * kNumBuckets` is allocated,
   *      either on the stack (via `MallocaArray`) if its size is below
   *      `kHistogramStackThresholdBytes`, or on the heap otherwise.
   *
   * 2. **Build histograms**:
   *    - `buildHistogram` scans the input once, filling the 2‑D histogram and
   *      detecting whether the array is already sorted. If sorted, the function
   *      returns immediately.
   *
   * 3. **Prefix sums**:
   *    - `prefixSum` converts each pass's counts to offsets and returns the
   *      number of non‑empty buckets per pass.
   *
   * 4. **Scatter**:
   *    - `doScatter` performs the stable scatter for all passes, swapping
   *      `data` and `buffer` after each pass.
   *
   * 5. **Final move‑back**:
   *    - If the sorted result resides in the temporary buffer (i.e., the last
   *      scatter swapped `data` to point to the buffer), the data is moved
   *      back to the original array to ensure the post‑condition that the
   *      result is always in the caller's original memory.
   *
   * @param data      [in,out] Pointer to the input array. After the call, the
   *                   sorted result is always placed in the original array
   *                   (the one pointed to by `data` on entry).
   * @param n         [in] Number of elements to sort.
   * @param allocator [in] Allocator for the temporary element buffer.
   * @param proj      [in] Projection functor producing unsigned keys.
   */
  static constexpr void sort(
      value_t* FOLLY_RESTRICT data,
      const size_t n,
      const Allocator& allocator,
      Projection proj) {
    auto* const original = data;

    // Allocate temporary element buffer.
    AllocatedBufferHolder<Allocator> holder(n, allocator);
    auto* FOLLY_RESTRICT buffer = holder.data();

    const size_t histSize = kNumPasses * kNumBuckets;
    const size_t histBytes = histSize * sizeof(hist_t);

    // Allocate histogram storage, preferring stack if size is small.
    MallocaArray<hist_t> histStorage(
        FOLLY_MALLOCA_THRESHOLD(histBytes, kHistogramStackThresholdBytes),
        histSize);
    __folly_memset(histStorage.data(), 0, histBytes);
    auto hist2d = reinterpret_cast<hist2d_t>(histStorage.data());

    // Step 1: build histograms and check sortedness.
    if (bool sorted = buildHistogram(data, n, hist2d, proj)) {
      return; // Already sorted; nothing to do.
    }

    // Step 2: convert counts to offsets and get non‑empty bucket counts.
    std::array nonEmptyCounts = prefixSum(hist2d, kPassIndices);

    // Step 3: scatter all passes.
    // The result may end up in the temporary buffer if the number of passes
    // executed is odd.
    auto* result = doScatter(data, n, buffer, nonEmptyCounts, hist2d, proj);

    // Step 4: if the sorted data is not in the original array, move it back.
    if (result != original) {
      std::move(result, result + n, original);
    }
  }
};

// ---------------------------------------------------------------------------
// MSD + Seq
// ---------------------------------------------------------------------------

/**
 * @brief Sequential MSD (most significant digit) radix sort implementation.
 *
 * @details
 * This specialization is selected when `RadixTraits::kSortStrategy == Msd`,
 * `kExecutionPolicy == Seq`, and `kNumPasses > 1`. It performs a recursive
 * MSD radix sort that processes bytes from the most significant down to the
 * least significant digit.
 *
 * ## Algorithm overview
 *
 * 1. **First pass scan** – A single scan over the input builds the histogram
 *    for the most significant digit (MSB pass) while also:
 *    - Detecting whether the array is already sorted (early exit).
 *    - Computing the bitwise OR of all projected keys to determine the
 *      highest set bit (`msb`), which is used to reduce the number of
 *      passes actually needed (skipping leading zero digits).
 *
 * 2. **Histogram construction** – For each subsequent pass (lower digits),
 *    a histogram is built on‑the‑fly for the bucket being processed,
 *    reusing a pre‑allocated 2‑D histogram array.
 *
 * 3. **Prefix sum** – Converts bucket counts to starting offsets and counts
 *    non‑empty buckets for pass skipping.
 *
 * 4. **Scatter** – Stably scatters elements into the buffer using the offsets.
 *
 * 5. **Recursive bucketing** – After scattering, each non‑empty bucket is
 *    recursively sorted on the next lower digit. Buckets of size 1 are moved
 *    back to the original array if the current recursion depth is even
 *    (handling alternating buffer roles).
 *
 * 6. **Fallback** – When a sub‑problem size falls below
 *    `kMsdFallbackToLsdThreshold` or the remaining pass count is 0, the
 *    implementation switches to `integerRadixSort` (LSD) for efficiency.
 *
 * ## Performance considerations
 * - The first pass always applies sign‑bit handling (`IsSignBitHandling`)
 *   when the key type is signed, because the sign bit is the most significant.
 * - Subsequent passes use the faster `operator()<false>` projection.
 * - Histogram memory is allocated once for all passes (`kNumPasses *
 * kNumBuckets`) to avoid per‑recursion allocation overhead.
 * - Recursion depth is limited by the number of passes; each recursive call
 *   moves to the next lower digit.
 *
 * @tparam RadixTraits   Compile‑time radix traits (options + derived
 * constants).
 * @tparam RandIter      Random‑access iterator type.
 * @tparam Allocator     Allocator for the temporary element buffer.
 * @tparam Projection    A functor that maps `value_t` to an unsigned integral
 * key.
 */
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    folly::standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
  requires(
      RadixTraits::kSortStrategy == RadixSortStrategy::Msd &&
      RadixTraits::kExecutionPolicy == RadixExecutionPolicy::Seq &&
      RadixTraits::kNumPasses > 1)
struct RadixSortImplDispatcher<RadixTraits, RandIter, Allocator, Projection>
    : RadixTraits {
  // Re‑export frequently used types and constants from the base trait.
  using Traits = RadixTraits;
  using Base = RadixTraits;
  using Base::kBitsPerPass;
  using Base::kFirstPass;
  using Base::kHistogramStackThresholdBytes;
  using Base::kIsSignHandlingOnFirstPass;
  using Base::kMsdFallbackToLsdThreshold;
  using Base::kNumBuckets;
  using Base::kNumPasses;
  using Base::kSortOrder;
  using typename Base::hist_t;
  using typename Base::key_t;
  using typename Base::uint_t;
  using typename Base::value_t;
  using hist2d_t = hist_t (*)[kNumBuckets];

  /**
   * @brief Performs the first‑pass scan: builds the MSB histogram, checks
   *        sortedness, and computes the highest set bit of all keys.
   *
   * @details
   * This function scans the input exactly once. For each element:
   *   - It projects the key using full sign‑bit handling (since the MSB
   *     pass is affected by the sign bit for signed types).
   *   - It increments the bucket counter for the most significant digit
   *     (`kFirstPass`).
   *   - It ORs all projected keys together to compute the bitwise union,
   *     from which the highest set bit (`msb`) is later extracted to
   *     determine the actual number of passes needed (skipping leading zeros).
   *   - It verifies monotonicity of the keys to detect if the input is
   *     already sorted.
   *
   * @param data     [in] Pointer to the input array.
   * @param n        [in] Number of elements.
   * @param hist2d   [out] 2‑D histogram array; only the `kFirstPass` row is
   *                 filled with the MSB bucket counts.
   * @param proj     [in] Projection functor.
   * @return A pair `{msb, sorted}` where:
   *         - `msb` is the index of the most significant set bit among all
   *           projected keys (0 if all keys are zero).
   *         - `sorted` is `true` if the input is already in the desired order.
   */
  static constexpr auto firstPassScan(
      const value_t* data, const size_t n, hist2d_t hist2d, Projection proj)
      -> std::pair<size_t, bool> {
    auto prev = kSortOrder == RadixSortOrder::Ascending
        ? std::numeric_limits<key_t>::lowest()
        : std::numeric_limits<key_t>::max();
    auto comp = std::conditional_t<
        kSortOrder == RadixSortOrder::Ascending,
        std::greater_equal<>,
        std::less_equal<>>{};

    const auto& keyMap = proj.getKeyMap();
    uint_t combined = 0;
    bool sorted = true;
    for (size_t i = 0; i < n; ++i) {
      const auto key = keyMap(data[i]);
      sorted = sorted && comp(key, prev);
      prev = key;
      combined |= std::bit_cast<uint_t>(key);
      const auto x = Projection::transform(key);
      ++hist2d[kFirstPass][nthRadix<Traits>(x, kFirstPass)];
    }

    return {highestOneBitIndex(combined), sorted};
  }

  /**
   * @brief Builds the histogram for a given non‑first pass.
   *
   * @details
   * This function is called during recursion for passes other than the first.
   * It scans the current sub‑array and increments bucket counters for the
   * specified `pass`. Sign‑bit handling is disabled (`operator()<false>`)
   * because the sign bit is only relevant in the MSB pass.
   *
   * @param data     [in] Pointer to the current sub‑array.
   * @param n        [in] Number of elements in the sub‑array.
   * @param hist2d   [out] 2‑D histogram array; the row `pass` is filled.
   * @param pass     [in] The current pass index (0 = LSB, `kNumPasses-1` =
   * MSB).
   * @param proj     [in] Projection functor.
   */
  static constexpr void buildHistogram(
      value_t* data,
      const size_t n,
      hist2d_t hist2d,
      const size_t pass,
      Projection proj) {
    for (size_t i = 0; i < n; ++i) {
      auto x = proj.template operator()<false>(data[i]);
      ++hist2d[pass][nthRadix<Traits>(x, pass)];
    }
  }

  /**
   * @brief Converts bucket counts to prefix‑sum offsets for a given pass and
   *        returns the number of non‑empty buckets.
   *
   * @param hist2d   [in,out] 2‑D histogram array; the row `pass` is modified
   *                 in‑place to hold prefix sums.
   * @param pass     [in] The pass index.
   * @return The number of buckets in this pass that contain at least one
   * element.
   */
  static constexpr auto prefixSum(hist2d_t hist2d, const size_t pass)
      -> size_t {
    size_t nonEmptyCount = (hist2d[pass][0] != 0);
    for (size_t i = 1; i < kNumBuckets; ++i) {
      nonEmptyCount += (hist2d[pass][i] != 0);
      hist2d[pass][i] += hist2d[pass][i - 1];
    }
    return nonEmptyCount;
  }

  /**
   * @brief Scatters elements into the buffer for a given pass.
   *
   * @tparam IsSignBitHandling  Whether to apply sign‑bit correction in the
   *                            projection (determined by
   * `kIsSignHandlingOnFirstPass` for the first pass, and `false` for others).
   *
   * @param data     [in,out] Pointer to the source array.
   * @param n        [in] Number of elements.
   * @param buffer   [out] Temporary buffer where scattered elements are placed.
   * @param pass     [in] The current pass index.
   * @param hist2d   [in,out] 2‑D offset array; offsets are decremented as
   *                 elements are placed.
   * @param proj     [in] Projection functor.
   */
  template <bool IsSignBitHandling = kIsSignHandlingOnFirstPass>
  static constexpr void doScatter(
      value_t* FOLLY_RESTRICT data,
      const size_t n,
      value_t* FOLLY_RESTRICT buffer,
      const size_t pass,
      hist2d_t hist2d,
      Projection proj) {
    for (size_t i = n; i-- > 0;) {
      auto x = proj.template operator()<IsSignBitHandling>(data[i]);
      buffer[--hist2d[pass][nthRadix<Traits>(x, pass)]] = std::move(data[i]);
    }
  }

  /**
   * @brief Recursively processes each bucket after a scatter.
   *
   * @details
   * Iterates over all buckets (0..`kNumBuckets-1`) using the prefix‑sum
   * offsets to determine the range of elements belonging to each bucket.
   * For each bucket with size > 1, it calls `doMsdRecursion` on the next
   * lower digit (`pass - 1`). For singleton buckets, if the current
   * recursion depth is even, it moves the element back to the original
   * array to maintain the correct buffer location.
   *
   * @param data     [in,out] Pointer to the array that currently holds the
   *                 scattered data (often the buffer after `doScatter`).
   * @param n        [in] Total number of elements in the current sub‑problem.
   * @param buffer   [in,out] The alternate buffer (used as source for
   *                 singleton moves).
   * @param pass     [in] The current pass index (the one just scattered).
   * @param hist2d   [in] 2‑D histogram with prefix sums for the current pass.
   * @param proj     [in] Projection functor.
   * @param depth    [in] Current recursion depth (0 at top level).
   */
  static constexpr void recurseBuckets(
      value_t* FOLLY_RESTRICT data,
      const size_t n,
      value_t* FOLLY_RESTRICT buffer,
      const size_t pass,
      hist2d_t hist2d,
      Projection proj,
      size_t depth = 0) {
    for (size_t i = 0; i < kNumBuckets; ++i) {
      size_t chunkBeg = hist2d[pass][i];
      size_t chunkEnd = (i + 1 < kNumBuckets) ? hist2d[pass][i + 1] : n;

      if (size_t chunkSize = chunkEnd - chunkBeg; chunkSize > 1) {
        // Recurse to the next lower digit, increasing depth.
        doMsdRecursion(
            buffer + chunkBeg,
            chunkSize,
            data + chunkBeg,
            pass - 1,
            hist2d,
            proj,
            depth + 1);
      } else if (chunkSize == 1 && (depth & 1) == 0) {
        // For singleton buckets at even depth, move the element back to the
        // original array because the buffer roles alternate each recursion.
        data[chunkBeg] = std::move(buffer[chunkBeg]);
      }
    }
  }

  /**
   * @brief Recursive helper for processing passes other than the first.
   *
   * @details
   * This function handles a sub‑problem starting at a given `pass` (which is
   * lower than `kFirstPass`). It decides whether to:
   *   - Fall back to LSD `integerRadixSort` if the sub‑problem is small or
   *     `pass` reaches 0.
   *   - Otherwise, build the histogram for the current pass, compute prefix
   *     sums, and (if more than one non‑empty bucket) scatter and recurse.
   *   - If only one bucket is non‑empty, it skips the scatter and recurses
   *     directly on the next lower pass with the same data.
   *
   * @param data     [in,out] Pointer to the current sub‑array.
   * @param n        [in] Size of the sub‑array.
   * @param buffer   [in,out] Temporary buffer.
   * @param pass     [in] Current pass index (decremented each recursion).
   * @param hist2d   [in,out] 2‑D histogram array (reused).
   * @param proj     [in] Projection functor.
   * @param depth    [in] Current recursion depth.
   */
  static constexpr void doMsdRecursion(
      value_t* FOLLY_RESTRICT data,
      const size_t n,
      value_t* FOLLY_RESTRICT buffer,
      const size_t pass,
      hist2d_t hist2d,
      Projection proj,
      size_t depth = 0) {
    auto* hist1d = hist2d[pass];
    // Fallback to LSD when the sub‑problem is small or no lower digits remain.
    if (n <= kMsdFallbackToLsdThreshold || pass < 1) {
      // `pass + 1` because LSD expects the number of passes, not the index.
      integerRadixSort<Traits>(data, n, buffer, pass + 1, hist1d, proj, depth);
      return;
    }

    // Step 1 : Clear the histogram for the current pass.
    __folly_memset(hist1d, 0, kNumBuckets * sizeof(hist_t));

    // Step 2 : build histogram for the current pass.
    buildHistogram(data, n, hist2d, pass, proj);

    // Step 3 :
    if (size_t nonEmptyCount = prefixSum(hist2d, pass); nonEmptyCount <= 1) {
      // If all elements share the same digit, skip scatter and go to next pass.
      return doMsdRecursion(data, n, buffer, pass - 1, hist2d, proj, depth);
    }

    // Step 4 : scatter using the current pass (no sign‑bit handling for
    // non‑first passes).
    doScatter<false>(data, n, buffer, pass, hist2d, proj);
    // Step 5 : recurse into each bucket.
    recurseBuckets(data, n, buffer, pass, hist2d, proj, depth);
  }

  /**
   * @brief Main entry point for sequential MSD radix sort.
   *
   * @details
   * This function orchestrates the sorting process:
   *
   * 1. Allocate temporary element buffer and a 2‑D histogram array of size
   *    `kNumPasses * kNumBuckets` (pre‑allocated to avoid per‑recursion
   *    allocation). The histogram memory may be stack‑allocated if small
   *    enough (via `MallocaArray`).
   *
   * 2. Perform the first‑pass scan (`firstPassScan`) which:
   *    - Builds the histogram for the MSB (`kFirstPass`).
   *    - Detects sortedness; returns early if already sorted.
   *    - Computes the highest set bit (`msb`) of all keys.
   *
   * 3. Calculate the actual number of passes required:
   *    `passes = ceil((msb + 1) / kBitsPerPass)`. This skips leading zero
   *    digits, reducing work.
   *
   * 4. If `passes` equals the full `kNumPasses`, the first pass histogram is
   *    already built. If the first pass has more than one non‑empty bucket,
   *    perform scatter and recurse; otherwise, start recursion from
   *    `passes - 1` (skipping the empty leading pass).
   *
   * 5. The recursion handles all subsequent passes and final fallback.
   *
   * @param data      [in,out] Pointer to the input array. After the call,
   *                   the sorted data resides in the original array.
   * @param n         [in] Number of elements.
   * @param allocator [in] Allocator for the temporary element buffer.
   * @param proj      [in] Projection functor.
   */
  static constexpr void sort(
      value_t* FOLLY_RESTRICT data,
      const size_t n,
      const Allocator& allocator,
      Projection proj) {
    AllocatedBufferHolder<Allocator> holder(n, allocator);
    auto* FOLLY_RESTRICT buffer = holder.data();

    const size_t histSize = kNumPasses * kNumBuckets;
    const size_t histBytes = histSize * sizeof(hist_t);

    // Pre‑allocate histogram memory for all passes at once,
    // prevent memory allocation overhead in every recursion.
    MallocaArray<hist_t> histStorage(
        FOLLY_MALLOCA_THRESHOLD(histBytes, kHistogramStackThresholdBytes),
        histSize);
    auto hist2d = reinterpret_cast<hist2d_t>(histStorage.data());

    // Only the first pass histogram needs to be cleared; other passes will be
    // cleared individually in `doMsdRecursion`.
    __folly_memset(hist2d[kFirstPass], 0, kNumBuckets * sizeof(hist_t));

    // First pass scan: builds MSB histogram, checks sortedness, computes msb.
    const auto [msb, sorted] = firstPassScan(data, n, hist2d, proj);
    if (sorted) {
      return;
    }

    // Compute the actual number of passes needed based on the highest set bit.
    // Example: for 32‑bit with 8‑bit chunks, msb=24 -> passes=(24+8)/8=4.
    const auto passes = (msb + kBitsPerPass) / kBitsPerPass;

    // If the full number of passes is needed, the first pass histogram is
    // ready.
    if (passes == kNumPasses && prefixSum(hist2d, kFirstPass) > 1) {
      // Scatter using the first pass (with sign‑bit handling if needed).
      doScatter(data, n, buffer, kFirstPass, hist2d, proj);
      recurseBuckets(data, n, buffer, kFirstPass, hist2d, proj);
    } else {
      // Start recursion from the highest pass that actually contains data.
      doMsdRecursion(data, n, buffer, passes - 1, hist2d, proj);
    }
  }
};

#ifdef _OPENMP
/**
 * @brief Helper utilities shared by parallel LSD and MSD radix sort
 *        implementations.
 *
 * @details
 * This class template factors out the common parallel operations used by both
 * parallel sorting strategies (LSD and MSD). It provides functions for:
 * - First‑pass scanning with per‑thread histograms and global sortedness check.
 * - Building histograms for subsequent passes.
 * - Computing prefix sums using a multi‑threaded 'И'‑shaped (N‑shaped) scan.
 * - Scattering elements in parallel.
 *
 * The histograms are stored as a 2‑D array `hist2d[thread][bucket]`, where the
 * first dimension is the thread index and the second is the bucket number.
 * This layout avoids false sharing and allows independent accumulation.
 *
 * @tparam RadixTraits  Compile‑time radix traits (options + derived constants).
 * @tparam Allocator    Allocator for temporary buffers (not directly used
 * here).
 * @tparam Projection   Functor mapping `value_t` to an unsigned integral key.
 */
template <typename RadixTraits, typename Allocator, typename Projection>
struct RadixSortParHelpers : RadixTraits {
  // Re‑export frequently used types and constants from the base trait.
  using Traits = RadixTraits;
  using Base = RadixTraits;
  using Base::kFirstPass;
  using Base::kIsSignHandlingOnFirstPass;
  using Base::kNaNsAssumption;
  using Base::kNumBuckets;
  using Base::kSortOrder;
  using typename Base::hist_t;
  using typename Base::key_t;
  using typename Base::uint_t;
  using typename Base::value_t;
  using hist2d_t = hist_t (*)[kNumBuckets];

  /**
   * @brief Performs the first‑pass scan in parallel.
   *
   * @details
   * This function is the parallel equivalent of the MSD sequential
   * `firstPassScan`. It divides the input into contiguous chunks according to
   * the `ThreadChunkInfo` distribution. Each thread:
   *   - Builds its own histogram for the most significant digit (`kFirstPass`)
   *     using the full sign‑bit handling (`IsSignHandlingOnFirstPass`).
   *   - Checks sortedness within its chunk.
   *   - Tracks the minimum and maximum keys in the chunk (skipping NaNs if
   *     they are assumed present).
   *
   * After all threads finish, the main thread combines the results:
   *   - The global sortedness is `true` only if every chunk is sorted and the
   *     max of each chunk ≤ min of the next chunk (for ascending order).
   *   - The combined MSB is the bitwise OR of all per‑chunk min and max values
   *     (reinterpreted as unsigned), which determines the actual number of
   *     passes needed.
   *
   * @param data   [in] Pointer to the input array.
   * @param info   [in] Thread distribution information (`threads`, `remain`,
   * `step`).
   * @param hist2d [out] 2‑D histogram array of size `[threads][kNumBuckets]`;
   *                 each thread writes to its own row.
   * @param proj   [in] Projection functor.
   * @return A pair `{msb, sorted}` where `msb` is the highest set bit among all
   *         keys, and `sorted` indicates whether the entire input is already
   * sorted.
   */
  static auto firstPassScan(
      const value_t* data,
      const ThreadChunkInfo info,
      hist2d_t hist2d,
      Projection proj) -> std::pair<size_t, bool> {
    auto comp = std::conditional_t<
        kSortOrder == RadixSortOrder::Ascending,
        std::greater_equal<>,
        std::less_equal<>>{};

    const auto& keyMap = proj.getKeyMap();
    const uint32_t threads = info.threads;
    const uint32_t remain = info.remain;
    const size_t step = info.step;
    using ThreadChunkStatsWrapped = ThreadChunkStats<key_t>;
    auto statsArray = FOLLY_MALLOCA_ARRAY(ThreadChunkStatsWrapped, threads);

#pragma omp parallel num_threads((int)threads)
    {
      const int tid = omp_get_thread_num();
      const auto [beg, end] = computeThreadChunkRange(tid, step, remain);

      bool sorted = true;
      auto minVal = std::numeric_limits<key_t>::max();
      auto maxVal = std::numeric_limits<key_t>::lowest();
      auto prev = kSortOrder == RadixSortOrder::Ascending
          ? std::numeric_limits<key_t>::lowest()
          : std::numeric_limits<key_t>::max();

      for (size_t i = beg; i < end; ++i) {
        const auto key = keyMap(data[i]);
        sorted = sorted && comp(key, prev);
        prev = key;

        auto x{Projection::template transform<kIsSignHandlingOnFirstPass>(key)};
        ++hist2d[tid][nthRadix<Traits>(x, kFirstPass)];

        // Skip NaN when computing min/max if NaNs are allowed (they break
        // ordering).
        if constexpr (
            std::is_floating_point_v<key_t> &&
            kNaNsAssumption == RadixNaNsAssumption::Existence) {
          if (std::isnan(key))
            continue;
        }
        if (key < minVal) {
          minVal = key;
        }
        if (key > maxVal) {
          maxVal = key;
        }
      }
      statsArray[tid] = ThreadChunkStats{sorted, minVal, maxVal};
    }

    // Merge per‑thread statistics.
    auto& [sorted1, minVal1, maxVal1] = statsArray[0];
    uint_t combined = 0;
    combined |= std::bit_cast<uint_t>(minVal1) | std::bit_cast<uint_t>(maxVal1);
    for (size_t i = 1; i < threads; ++i) {
      const auto [sorted, minVal, maxVal] = statsArray[i];
      combined |= std::bit_cast<uint_t>(minVal) | std::bit_cast<uint_t>(maxVal);
      sorted1 = sorted1 && sorted;
      if constexpr (kSortOrder == RadixSortOrder::Ascending) {
        sorted1 = sorted1 && (minVal >= statsArray[i - 1].maxVal);
      } else {
        sorted1 = sorted1 && (maxVal <= statsArray[i - 1].minVal);
      }
    }
    return {highestOneBitIndex(combined), sorted1};
  }

  /**
   * @brief Builds a histogram for a given pass in parallel.
   *
   * @details
   * Each thread processes its assigned chunk and increments the bucket counters
   * for its own row in `hist2d`. The first dimension is the thread ID.
   *
   * @tparam IsSignBitHandling  Whether to apply sign‑bit correction in the
   *                            projection (default: `false`).
   * @param data   [in] Pointer to the current array.
   * @param info   [in] Thread distribution information.
   * @param hist2d [out] 2‑D histogram array.
   * @param pass   [in] The pass index for which the histogram is built.
   * @param proj   [in] Projection functor.
   */
  template <bool IsSignBitHandling = false>
  static void buildHistogram(
      value_t* data,
      const ThreadChunkInfo info,
      hist2d_t hist2d,
      const size_t pass,
      Projection proj) {
    const uint32_t threads = info.threads;
    const uint32_t remain = info.remain;
    const size_t step = info.step;
#pragma omp parallel num_threads((int)threads)
    {
      const int tid = omp_get_thread_num();
      const auto [beg, end] = computeThreadChunkRange(tid, step, remain);

      for (size_t i = beg; i < end; ++i) {
        const auto x = proj.template operator()<IsSignBitHandling>(data[i]);
        ++hist2d[tid][nthRadix<Traits>(x, pass)];
      }
    }
  }

  /**
   * @brief Converts per‑thread bucket counts to global prefix‑sum offsets.
   *
   * @details
   * The input `hist2d` initially contains per‑thread counts for each bucket.
   * This function performs a two‑phase 'И'‑shaped (N‑shaped) scan:
   *   1. Vertically accumulate each bucket column: for each bucket `b`,
   *      `hist2d[t][b] += hist2d[t-1][b]` so that each row contains the
   *      cumulative count of that bucket from all threads up to that thread.
   *   2. Then horizontally shift the offsets: for each bucket `b` (starting
   *      from 1), add the total count of the previous bucket (which is now
   *      available in `hist2d[threads-1][b-1]`) to the first row of the
   *      current bucket, and propagate vertically again to make each row
   *      hold the global starting index for that bucket.
   *
   * The result is that `hist2d[tid][bucket]` becomes the global starting
   * offset for elements belonging to `bucket` that are handled by thread `tid`.
   * After this, the scatter phase can use these offsets to place elements
   * correctly.
   *
   * The function also counts the total number of non‑empty buckets across all
   * threads, which is used to skip passes with only one occupied bucket.
   *
   * @param hist2d [in,out] 2‑D histogram array; on entry raw counts,
   *                 on exit global offsets.
   * @param threads [in] Number of threads.
   * @return The total number of non‑empty buckets (across all threads).
   */
  static auto prefixSum(hist2d_t hist2d, size_t threads) -> size_t {
    // Vertically accumulate the first bucket column.
    for (size_t tid = 1; tid < threads; ++tid) {
      hist2d[tid][0] += hist2d[tid - 1][0];
    }

    size_t nonEmpty = (hist2d[threads - 1][0] != 0);
    for (size_t i = 1; i < kNumBuckets; ++i) {
      const auto last = hist2d[threads - 1][i - 1];
      hist2d[0][i] += last;

      for (size_t tid = 1; tid < threads; ++tid) {
        hist2d[tid][i] += hist2d[tid - 1][i];
      }

      nonEmpty += (hist2d[threads - 1][i] - last != 0);
    }
    return nonEmpty;
  }

  /**
   * @brief Scatters elements into the buffer in parallel.
   *
   * @details
   * Each thread processes its assigned chunk of the source array, uses the
   * global offsets (computed by `prefixSum`) to place each element into the
   * correct bucket in the buffer, and then decrements the offset. The scatter
   * walks backward (stable placement) within each thread's sub‑range.
   *
   * @tparam IsSignBitHandling  Whether to apply sign‑bit correction in the
   *                            projection (default: `false`).
   * @param data   [in,out] Pointer to the source array.
   * @param info   [in] Thread distribution information.
   * @param buffer [out] Temporary buffer where scattered elements are placed.
   * @param pass   [in] Current pass index.
   * @param hist2d [in,out] 2‑D offset array (modified as offsets are consumed).
   * @param proj   [in] Projection functor.
   */
  template <bool IsSignBitHandling = false>
  static void doScatter(
      value_t* FOLLY_RESTRICT data,
      const ThreadChunkInfo info,
      value_t* FOLLY_RESTRICT buffer,
      const size_t pass,
      hist2d_t hist2d,
      Projection proj) {
    const uint32_t threads = info.threads;
    const uint32_t remain = info.remain;
    const size_t step = info.step;
#pragma omp parallel num_threads((int)threads)
    {
      int tid = omp_get_thread_num();
      auto [beg, end] = computeThreadChunkRange(tid, step, remain);
      for (size_t i = end; i-- > beg;) {
        const auto x = proj.template operator()<IsSignBitHandling>(data[i]);
        buffer[--hist2d[tid][nthRadix<Traits>(x, pass)]] = std::move(data[i]);
      }
    }
  }
};

// ---------------------------------------------------------------------------
// LSD + Par (requires _OPENMP)
// ---------------------------------------------------------------------------

/**
 * @brief Parallel LSD radix sort implementation.
 *
 * @details
 * This specialization is selected when `RadixTraits::kSortStrategy == Lsd` and
 * `kExecutionPolicy == Par`. It uses OpenMP to parallelize the histogram,
 * prefix‑sum, and scatter phases. The parallel helper utilities are inherited
 * from `RadixSortParHelpers`.
 *
 * ## Algorithm flow
 *
 * The algorithm follows the standard LSD radix sort but with parallel phases:
 *
 * 1. **First pass scan** (`firstPassScan`):
 *    - Each thread processes a chunk of the input.
 *    - Builds per‑thread histograms for the most significant digit (MSB)
 *      while checking sortedness and tracking min/max keys.
 *    - The global `msb` (highest set bit) and sortedness are merged from
 *      per‑thread statistics.
 *    - If the entire array is already sorted, the function returns early.
 *
 * 2. **First pass scatter**:
 *    - The per‑thread histograms are converted to global offsets via
 *      the parallel 'И'‑shaped prefix sum.
 *    - If more than one non‑empty bucket exists, scatter the elements
 *      using the first pass (with sign‑bit handling if required).
 *
 * 3. **Subsequent passes**:
 *    - For each remaining pass (from 1 to `passes-1`), the histogram is
 *      rebuilt and cleared before each pass.
 *    - The last pass (if it equals `kNumPasses`) uses sign‑bit handling
 *      (`IsSignBitHandling = true`); earlier passes do not.
 *    - After each scatter, the `data` and `buffer` pointers are swapped.
 *
 * 4. **Final move‑back**:
 *    - If the sorted data ends up in the temporary buffer (determined by
 *      the parity of the number of executed passes), it is moved back to
 *      the original array.
 *
 * ## Differences from sequential LSD
 * - Histograms are built pass‑by‑pass (not all at once) because each pass
 *   requires per‑thread local counts that are later merged via prefix sum.
 * - The prefix sum uses an 'И'‑shaped scan to combine thread‑local counts
 *   into global offsets.
 *
 * @tparam RadixTraits   Compile‑time radix traits (options + derived
 * constants).
 * @tparam RandIter      Random‑access iterator type.
 * @tparam Allocator     Allocator for the temporary element buffer.
 * @tparam Projection    A functor that maps `value_t` to an unsigned integral
 * key.
 */
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    folly::standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
  requires(
      RadixTraits::kSortStrategy == RadixSortStrategy::Lsd &&
      RadixTraits::kExecutionPolicy == RadixExecutionPolicy::Par)
struct RadixSortImplDispatcher<RadixTraits, RandIter, Allocator, Projection>
    : public RadixSortParHelpers<RadixTraits, Allocator, Projection> {
  // Re‑export frequently used types and constants from the base trait.
  using Traits = RadixTraits;
  using Base = RadixSortParHelpers<RadixTraits, Allocator, Projection>;
  using Base::buildHistogram;
  using Base::doScatter;
  using Base::firstPassScan;
  using Base::kBitsPerPass;
  using Base::kFirstPass;
  using Base::kHistogramStackThresholdBytes;
  using Base::kIsSignHandlingOnFirstPass;
  using Base::kNumBuckets;
  using Base::kNumPasses;
  using Base::prefixSum;
  using typename Base::hist_t;
  using typename Base::value_t;
  using hist2d_t = hist_t (*)[kNumBuckets];

  /**
   * @brief Main entry point for parallel LSD radix sort.
   *
   * @param data      [in,out] Pointer to the input array. After the call,
   *                   the sorted result is placed in the original array.
   * @param n         [in] Number of elements.
   * @param allocator [in] Allocator for the temporary element buffer.
   * @param proj      [in] Projection functor.
   */
  static void sort(
      value_t* FOLLY_RESTRICT data,
      const size_t n,
      const Allocator& allocator,
      Projection proj) {
    auto* const original = data;
    AllocatedBufferHolder<Allocator> holder(n, allocator);
    auto* FOLLY_RESTRICT buffer = holder.data();

    const size_t threads = ::omp_get_max_threads();
    const size_t histSize = threads * kNumBuckets;
    const size_t histBytes = histSize * sizeof(hist_t);
    const ThreadChunkInfo chunkInfo(threads, n % threads, n / threads);

    MallocaArray<hist_t> histStorage(
        FOLLY_MALLOCA_THRESHOLD(histBytes, kHistogramStackThresholdBytes),
        histSize);
    __folly_memset(histStorage.data(), 0, histBytes);
    auto hist2d = reinterpret_cast<hist2d_t>(histStorage.data());

    auto [msb, sorted] = firstPassScan(data, chunkInfo, hist2d, proj);
    if (sorted) {
      return;
    }

    if (size_t numEmptyCount = prefixSum(hist2d, threads); numEmptyCount > 1) {
      Base::template doScatter<kIsSignHandlingOnFirstPass>(
          data, chunkInfo, buffer, kFirstPass, hist2d, proj);
      std::swap(data, buffer);
    }

    const auto passes = (msb + kBitsPerPass) / kBitsPerPass;

    // Process remaining passes (from 1 to passes-1).
    for (size_t pass = 1; pass < passes; ++pass) {
      __folly_memset(histStorage.data(), 0, histBytes);

      if (pass != passes - 1 || passes != kNumPasses) {
        buildHistogram(data, chunkInfo, hist2d, pass, proj);
        if (prefixSum(hist2d, threads) > 1) {
          doScatter(data, chunkInfo, buffer, pass, hist2d, proj);
          std::swap(data, buffer);
        }
      } else {
        Base::template buildHistogram<true>(
            data, chunkInfo, hist2d, pass, proj);
        if (prefixSum(hist2d, threads) > 1) {
          Base::template doScatter<true>(
              data, chunkInfo, buffer, pass, hist2d, proj);
          std::swap(data, buffer);
        }
      }
    }

    // If the sorted data is in the temporary buffer, move it back to the
    // original array (this happens when the number of passes executed is odd).
    if (data != original) {
      std::move(data, data + n, original);
    }
  }
};

// ---------------------------------------------------------------------------
// MSD + Par (requires _OPENMP)
// ---------------------------------------------------------------------------
/**
 * @brief Parallel MSD (most significant digit) radix sort implementation.
 *
 * @details
 * This specialization is selected when `RadixTraits::kSortStrategy == Msd`,
 * `kExecutionPolicy == Par`, and `kNumPasses > 1`. It uses OpenMP to
 * parallelise the histogram and scatter phases of the MSD algorithm. The
 * parallel helper utilities are inherited from `RadixSortParHelpers`.
 *
 * ## Key differences from the sequential MSD implementation
 * - Histogram memory is allocated **per recursion level** rather than once
 *   upfront. This is necessary because multiple threads may execute recursive
 *   calls concurrently; each recursive invocation needs its own independent
 *   scratch space for the 2‑D histogram (`hist2d[tid][bucket]`) to avoid data
 *   races when building histograms for different buckets.
 * - The `recurseBuckets` method parallelises over buckets using OpenMP, but
 *   only if the current thread is not already inside a parallel region.
 * - The `doMsdRecursion` function determines the number of threads to use
 *   based on whether it is called from within an existing parallel region.
 *
 * For a detailed description of the MSD algorithm flow, please refer to the
 * documentation of the sequential MSD implementation (`RadixSortImplDispatcher`
 * for `Msd` + `Seq`).
 *
 * @tparam RadixTraits   Compile‑time radix traits (options + derived
 * constants).
 * @tparam RandIter      Random‑access iterator type.
 * @tparam Allocator     Allocator for the temporary element buffer.
 * @tparam Projection    A functor that maps `value_t` to an unsigned integral
 * key.
 */
template <
    typename RadixTraits,
    std::random_access_iterator RandIter,
    folly::standard_allocator Allocator,
    unsigned_integral_projection<std::iter_value_t<RandIter>> Projection>
  requires(
      RadixTraits::kSortStrategy == RadixSortStrategy::Msd &&
      RadixTraits::kExecutionPolicy == RadixExecutionPolicy::Par &&
      RadixTraits::kNumPasses > 1)
struct RadixSortImplDispatcher<RadixTraits, RandIter, Allocator, Projection>
    : public RadixSortParHelpers<RadixTraits, Allocator, Projection> {
  // Re‑export frequently used types and constants from the base trait.
  using Traits = RadixTraits;
  using Base = RadixSortParHelpers<RadixTraits, Allocator, Projection>;
  using Base::buildHistogram;
  using Base::doScatter;
  using Base::firstPassScan;
  using Base::kBitsPerPass;
  using Base::kFirstPass;
  using Base::kHistogramStackThresholdBytes;
  using Base::kMsdFallbackToLsdThreshold;
  using Base::kNumBuckets;
  using Base::kNumPasses;
  using Base::prefixSum;
  using typename Base::hist_t;
  using typename Base::value_t;
  using hist2d_t = hist_t (*)[kNumBuckets];

  /**
   * @brief Recursively processes buckets after a scatter, in parallel.
   *
   * @details
   * This function iterates over all buckets (0..`kNumBuckets-1`) using the
   * prefix‑sum offsets stored in `hist2d[0]` (the first thread row, which holds
   * the global offsets after prefix sum). For each bucket with size > 1, it
   * calls `doMsdRecursion` on the next lower digit. Singleton buckets are
   * moved back to the original array if the current depth is even.
   *
   * Buckets are processed in parallel using OpenMP. Parallelism is enabled only
   * if `info.threads > 1` and the current thread is not already inside a
   * parallel region (to avoid nested parallelism).
   *
   * @param data     [in,out] Pointer to the array holding the scattered data.
   * @param n        [in] Total number of elements in the current sub‑problem.
   * @param buffer   [in,out] The alternate buffer (used as source for singleton
   * moves).
   * @param pass     [in] The current pass index (the one just scattered).
   * @param hist2d   [in] 2‑D histogram with prefix sums for the current pass
   *                 (global offsets are in row 0).
   * @param info     [in] Thread distribution information for the current
   * recursion.
   * @param proj     [in] Projection functor.
   * @param depth    [in] Current recursion depth (0 at top level).
   */
  static void recurseBuckets(
      value_t* FOLLY_RESTRICT data,
      const size_t n,
      value_t* FOLLY_RESTRICT buffer,
      const size_t pass,
      hist2d_t hist2d,
      const ThreadChunkInfo info,
      Projection proj,
      size_t depth = 0) {
#pragma omp parallel for schedule(static) \
    num_threads((int)info.threads) if (info.threads > 1 && !omp_in_parallel())
    for (size_t i = 0; i < kNumBuckets; ++i) {
      size_t chunkBeg = hist2d[0][i];
      size_t chunkEnd = (i + 1 < kNumBuckets) ? hist2d[0][i + 1] : n;

      if (size_t chunkSize = chunkEnd - chunkBeg; chunkSize > 1) {
        doMsdRecursion(
            buffer + chunkBeg,
            chunkSize,
            data + chunkBeg,
            pass - 1,
            info,
            proj,
            depth + 1);
      } else if (chunkSize == 1 && (depth & 1) == 0) {
        data[chunkBeg] = std::move(buffer[chunkBeg]);
      }
    }
  }

  /**
   * @brief Recursive helper for processing passes other than the first, with
   *        per‑recursion histogram allocation.
   *
   * @details
   * This function is the parallel counterpart of the sequential
   * `doMsdRecursion`. It handles a sub‑problem starting at a given `pass`
   * (lower than `kFirstPass`). The algorithm is identical to the sequential
   * version, except that it allocates a fresh 2‑D histogram for each recursion
   * level to avoid data races between parallel bucket tasks.
   *
   * The number of threads used for this recursion is set to `1` if the function
   * is called from within an OpenMP parallel region (to avoid nested
   * parallelism); otherwise, it uses the global thread count.
   *
   * Fallback to LSD (`integerRadixSort`) occurs when the sub‑problem is small
   * or `pass` reaches 0.
   *
   * @param data     [in,out] Pointer to the current sub‑array.
   * @param n        [in] Size of the sub‑array.
   * @param buffer   [in,out] Temporary buffer.
   * @param pass     [in] Current pass index (decremented each recursion).
   * @param info     [in] Thread distribution information (may be adjusted).
   * @param proj     [in] Projection functor.
   * @param depth    [in] Current recursion depth.
   */
  static void doMsdRecursion(
      value_t* FOLLY_RESTRICT data,
      const size_t n,
      value_t* FOLLY_RESTRICT buffer,
      const size_t pass,
      const ThreadChunkInfo info,
      Projection proj,
      size_t depth = 0) {
    const uint32_t threads = omp_in_parallel() ? 1U : info.threads;
    const size_t histSize = size_t{threads} * kNumBuckets;
    const size_t histBytes = histSize * sizeof(hist_t);

    MallocaArray<hist_t> histStorage(
        FOLLY_MALLOCA_THRESHOLD(histBytes, kHistogramStackThresholdBytes),
        histSize);
    auto hist2d = reinterpret_cast<hist2d_t>(histStorage.data());
    auto* hist1d = hist2d[0];

    if (n <= kMsdFallbackToLsdThreshold || pass < 1) {
      integerRadixSort<Traits>(data, n, buffer, pass + 1, hist1d, proj, depth);
      return;
    }

    __folly_memset(histStorage.data(), 0, histBytes);

    auto [step, remain] = ::lldiv(n, threads);
    const ThreadChunkInfo subInfo(threads, remain, step);
    buildHistogram(data, subInfo, hist2d, pass, proj);

    if (prefixSum(hist2d, threads) <= 1) {
      doMsdRecursion(data, n, buffer, pass - 1, info, proj, depth);
      return;
    }
    doScatter(data, subInfo, buffer, pass, hist2d, proj);
    recurseBuckets(data, n, buffer, pass, hist2d, subInfo, proj, depth);
  }

  /**
   * @brief Main entry point for parallel MSD radix sort.
   *
   * @details
   * This function orchestrates the sorting process using the parallel helpers
   * from `RadixSortParHelpers`. The workflow is similar to the sequential MSD
   * `sort`:
   *
   * 1. Allocate temporary element buffer and a 2‑D histogram of size
   *    `threads * kNumBuckets` for the first pass.
   * 2. Perform the first‑pass scan (`firstPassScan`) – this fills the
   * per‑thread histograms, detects sortedness, and computes the highest set bit
   * (`msb`).
   * 3. If sorted, return early.
   * 4. Compute the actual number of passes needed (`passes`) from `msb`.
   * 5. If `passes == kNumPasses` and the first pass has more than one non‑empty
   *    bucket, scatter using the first pass and then recurse on buckets.
   * 6. Otherwise, start recursion from `passes - 1` (skipping leading zero
   * digits).
   *
   * Unlike the sequential version, no upfront allocation for all passes is
   * made; each recursion level allocates its own histogram via
   * `doMsdRecursion`.
   *
   * @param data      [in,out] Pointer to the input array. After the call,
   *                   the sorted data resides in the original array.
   * @param n         [in] Number of elements.
   * @param allocator [in] Allocator for the temporary element buffer.
   * @param proj      [in] Projection functor.
   */
  static void sort(
      value_t* FOLLY_RESTRICT data,
      const size_t n,
      const Allocator& allocator,
      Projection proj) {
    AllocatedBufferHolder<Allocator> holder(n, allocator);
    auto* FOLLY_RESTRICT buffer = holder.data();

    const size_t threads = ::omp_get_max_threads();
    const size_t histSize = threads * kNumBuckets;
    const size_t histBytes = histSize * sizeof(hist_t);
    const ThreadChunkInfo chunkInfo(threads, n % threads, n / threads);

    MallocaArray<hist_t> histStorage(
        FOLLY_MALLOCA_THRESHOLD(histBytes, kHistogramStackThresholdBytes),
        histSize);
    __folly_memset(histStorage.data(), 0, histBytes);
    auto hist2d = reinterpret_cast<hist2d_t>(histStorage.data());

    auto [msb, sorted] = firstPassScan(data, chunkInfo, hist2d, proj);
    if (sorted) {
      return;
    }

    const auto passes = (msb + kBitsPerPass) / kBitsPerPass;
    if (passes == kNumPasses && prefixSum(hist2d, threads) > 1) {
      Base::template doScatter<true>(
          data, chunkInfo, buffer, kFirstPass, hist2d, proj);
      recurseBuckets(data, n, buffer, kFirstPass, hist2d, chunkInfo, proj);
    } else {
      doMsdRecursion(data, n, buffer, passes - 1, chunkInfo, proj);
    }
  }
};
#endif

/**
 * @brief Selects and invokes the correct RadixSortImplDispatcher.
 *
 * @details
 * This is the internal entry point for radix sort. It handles three concerns
 * that the public API should not expose:
 *  - **Boolean special case**: uses `sortBool` (O(n) single‑pass partition)
 *    instead of the general radix sort.
 *  - **Small‑array fallback**: switches to insertion sort when the size is
 *    below `kRadixSortThreshold` to avoid radix‑sort overhead.
 *  - **Policy validation**: enforces compile‑time constraints on strategy,
 *    execution policy, chunk size, and NaN assumptions.
 *
 * Reverse‑iterator adjustment is handled by the caller (`radixSort`).
 *
 * @tparam RadixTraits    Compile‑time radix traits (options + derived
 * constants).
 * @tparam RandIter       Random‑access iterator type.
 * @tparam Allocator      Allocator for temporary buffer.
 * @tparam Projection     A functor that maps `value_t` to an unsigned integral
 * key.
 * @param first           Inclusive start of the range.
 * @param last            Exclusive end of the range.
 * @param allocator       Allocator instance for obtaining temporary storage.
 * @param proj            Projection functor.
 *
 * @note This function is intended for internal use only and is not part of the
 *       public API. It is `constexpr` to allow compile‑time evaluation in
 * tests.
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
  // If NaNs are assumed absent, NaN‑position handling must be Unhandled.
  static_assert(
      !(RadixTraits::kNaNsAssumption == RadixNaNsAssumption::NonExistence &&
        RadixTraits::kNaNsPosHandling != RadixNaNsPosHandling::Unhandled),
      "If you assumed there are no NaNs, then you shouldn't change the default settings for NaN positions.");

  // Prohibit 16‑bit chunks for 1‑byte keys (would create 65536 buckets for
  // nothing).
  static_assert(
      !(RadixTraits::kBitsPerPass == 16U &&
        sizeof(typename RadixTraits::uint_t) == 1U),
      "Do not set RadixSortOptions.BitsPerPass = RadixBitsPerPass::Bits16 (two‑byte) for one‑byte data!");

  // MSD with only one pass is just counting sort; LSD is simpler and safer.
  // Also, MSD recursion would underflow when computing (pass - 1) for pass ==
  // 0.
  static_assert(
      !(RadixTraits::kSortStrategy == RadixSortStrategy::Msd &&
        RadixTraits::kNumPasses <= 1),
      "MSD sort with only one pass is equivalent to counting sort; "
      "use LSD strategy (RadixSortStrategy::Lsd) for single‑pass sorting, "
      "or ensure at least 2 passes for MSD to avoid unsigned underflow in recurseBuckets (pass(0) - 1).");

#ifndef _OPENMP
  // Parallel policy requires OpenMP support.
  static_assert(
      RadixTraits::kExecutionPolicy != RadixExecutionPolicy::Par,
      "Parallel policy requires _OPENMP defined");
#endif

  // Validate input range.
  assert(first <= last && "Invalid iterator range: first must be <= last");

  // 1. Special case: boolean elements use a dedicated O(n) partition routine.
  if constexpr (std::same_as<std::iter_value_t<RandIter>, bool>) {
    // Invert order if descending is requested.
    sortBool<RandIter, RadixTraits::kSortOrder != RadixSortOrder::Ascending>(
        first, last);
  } else {
    const size_t size = last - first;

    // 2. Small‑array fallback: insertion sort avoids radix‑sort overhead.
    if (size <= kRadixSortThreshold) [[unlikely]] {
      // Use projection as key extractor; compare according to SortOrder.
      insertionSort(first, last, [proj](const auto& a, const auto& b) {
        if constexpr (RadixTraits::kSortOrder == RadixSortOrder::Ascending) {
          return proj(a) < proj(b);
        } else {
          return proj(b) < proj(a);
        }
      });
      return;
    }

    // 3. Regular path: Dispatch to the selected strategy
    //    (LSD/Seq or LSD/Par or MSD/Seq or MSD/Par).
    RadixSortImplDispatcher<RadixTraits, RandIter, Allocator, Projection>::sort(
        &*first, size, allocator, proj);
  }
}

} // namespace detail
namespace detail {

inline constexpr RadixSortOptions kDeprecatedStableRadixSortOptions{};

inline constexpr RadixSortOptions kDeprecatedStableRadixSortDescendingOptions =
    [] {
      RadixSortOptions opts{};
      opts.SortOrder = RadixSortOrder::Descending;
      return opts;
    }();

} // namespace detail

// ----------------------------------------------------------------------
// Public API overloads
// ----------------------------------------------------------------------

template <typename ContiguousIt>
[[deprecated("use folly::radixSort instead")]]
void stable_radix_sort(ContiguousIt first, ContiguousIt last) {
  radixSort(first, last);
}

template <typename ContiguousIt, typename KeyMap>
[[deprecated("use folly::radixSort with a key map instead")]]
void stable_radix_sort(ContiguousIt first, ContiguousIt last, KeyMap keyMap) {
  radixSort(first, last, std::move(keyMap));
}

template <typename ContiguousIt>
[[deprecated("use folly::radixSort with RadixSortOrder::Descending instead")]]
void stable_radix_sort_descending(ContiguousIt first, ContiguousIt last) {
  radixSort<detail::kDeprecatedStableRadixSortDescendingOptions>(first, last);
}

template <typename ContiguousIt, typename KeyMap>
[[deprecated("use folly::radixSort with RadixSortOrder::Descending instead")]]
void stable_radix_sort_descending(
    ContiguousIt first, ContiguousIt last, KeyMap keyMap) {
  radixSort<detail::kDeprecatedStableRadixSortDescendingOptions>(
      first, last, std::move(keyMap));
}

/**
 * @overloadbrief Radix sort a range `[first, last)` with full control over
 *                options, allocator, and key projection.
 *
 * @details
 * This is the most general overload. It accepts a `RadixSortOptions` template
 * argument, an allocator for temporary storage, and an arbitrary `KeyMap`
 * that projects each element to an arithmetic key (integral or floating‑point).
 *
 * The function automatically handles reverse iterators by swapping the
 * sort order and passing the underlying base iterators to the internal
 * implementation.
 *
 * @tparam RadixOptions  Compile‑time sorting options (see `RadixSortOptions`).
 * @tparam RandIter      Random‑access iterator type.
 * @tparam Allocator     Allocator type (must satisfy `standard_allocator`).
 * @tparam KeyMap        A callable `value_t -> arithmetic` (default: identity).
 * @param first          Inclusive start of the range.
 * @param last           Exclusive end of the range.
 * @param allocator      Allocator instance for temporary buffer.
 * @param keyMap         Key extraction functor.
 *
 * @example
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

  // Construct the projection functor that maps value -> unsigned key.
  auto projection =
      detail::radix_uint_projection.template
      operator()<RadixOptions.NaNsPosHandling>(
          std::in_place_type<value_t>, std::move(keyMap));

  /*
   * Reverse iterators invert the semantic order:
   *   sorting a reversed range in ascending order ≡ sorting the original
   *   range in descending order.
   * We adjust `SortOrder` accordingly and pass the underlying base iterators.
   */
  constexpr auto adjustedOptions = [] {
    RadixSortOptions opts = RadixOptions;
    opts.SortOrder =
        detail::RadixRealSortOrder<RandIter, RadixOptions.SortOrder>::value;
    return opts;
  }();

  using RadixTraits = detail::RadixSortTraits<adjustedOptions, value_t, KeyMap>;

  // If the iterator is a reverse_iterator, unwrap to base and swap first/last.
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
 * Convenience overload that uses `std::allocator<value_t>` internally.
 * All other parameters are the same as the four‑argument overload.
 *
 * @tparam RadixOptions  Compile‑time sorting options.
 * @tparam RandIter      Random‑access iterator type.
 * @tparam KeyMap        A callable `value_t -> arithmetic` (default: identity).
 * @param first          Inclusive start of the range.
 * @param last           Exclusive end of the range.
 * @param keyMap         Key extraction functor.
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
 * using LSD radix sort with 8‑bit chunks and all other options at their
 * defaults. This overload is ideal for quick, generic use.
 *
 * @tparam RandIter  Random‑access iterator type.
 * @tparam KeyMap    A callable `value_t -> arithmetic` (default: identity).
 * @param first      Inclusive start of the range.
 * @param last       Exclusive end of the range.
 * @param keyMap     Key extraction functor.
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