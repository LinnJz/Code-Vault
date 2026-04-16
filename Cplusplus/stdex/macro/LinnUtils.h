#ifndef LINN_UTILS_H_
#define LINN_UTILS_H_

#ifdef __GNUC__
#define UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#define UNREACHABLE() __assume(false)
#else
#define UNREACHABLE() std::abort()
#endif

#if defined(__clang__)
// Clang support clang loop unroll_count or unroll
#define LOOP_UNROLL(n) _Pragma(EXPAND_AND_STRINGIFY(clang loop unroll_count(n)))
#define ALWAYS_INLINE  inline __attribute__((always_inline))
#elif defined(__GNUC__) && !defined(__INTEL_COMPILER) && __GNUC__ >= 8
// GCC 8+ support #pragma GCC unroll
#define LOOP_UNROLL(n) _Pragma(EXPAND_AND_STRINGIFY(GCC unroll n))
#define ALWAYS_INLINE  inline __attribute__((always_inline))
#elif defined(_MSC_VER) && _MSC_VER >= 1920
// MSVC 2019+ support __pragma(unroll)
#define LOOP_UNROLL(n) //__pragma(unroll(n))
#define ALWAYS_INLINE  inline __forceinline
#else
#define LOOP_UNROLL(n)
#define ALWAYS_INLINE inline
#endif

#define LINN_EXPAND(x) x

#define LINN_STRINGIFY_I(x) #x
#define LINN_STRINGIFY(x)   LINN_STRINGIFY_I(x)
#define EXPAND_AND_STRINGIFY(x) LINN_STRINGIFY(x)

#define LINN_CAT_I(a, b)      a##b
#define LINN_CAT(a, b)        LINN_CAT_I(a, b)
#define LINN_CAT3(a, b, c)    LINN_CAT(LINN_CAT(a, b), c)
#define LINN_CAT4(a, b, c, d) LINN_CAT(LINN_CAT3(a, b, c), d)


#define LINN_NARGS_I(_1, _2, _3, N, ...) N
#define LINN_NARGS(...)                  LINN_NARGS_I(__VA_ARGS__ __VA_OPT__(, ) 3, 2, 1, 0)

#define LINN_IF(cond)   LINN_CAT(LINN_IF_, cond)
#define LINN_IF_1(t, f) t
#define LINN_IF_0(t, f) f

#define LINN_CHECK_N(x, n, ...)  n
#define LINN_CHECK(...)          LINN_CHECK_N(__VA_ARGS__, 0)
#define LINN_PROBE(x)            x, 1
#define LINN_IS_PAREN(x)         LINN_CHECK(LINN_IS_PAREN_PROBE x)
#define LINN_IS_PAREN_PROBE(...) LINN_PROBE(~)

#define LINN_PAIR(a, b)                (a, b)
#define LINN_PAIR_FIRST_I(tag, value)  tag
#define LINN_PAIR_FIRST(pair)          LINN_PAIR_FIRST_I pair
#define LINN_PAIR_SECOND_I(tag, value) value
#define LINN_PAIR_SECOND(pair)         LINN_PAIR_SECOND_I pair

#define LINN_OR(a, b) LINN_CAT(LINN_OR_, LINN_CAT(a, b))
#define LINN_OR_00    0
#define LINN_OR_01    1
#define LINN_OR_10    1
#define LINN_OR_11    1

#define LINN_DISABLE_COPY(CLASS)           \
  CLASS(const CLASS&)            = delete; \
  CLASS& operator=(const CLASS&) = delete;

#define LINN_DISABLE_MOVE(CLASS)      \
  CLASS(CLASS&&)            = delete; \
  CLASS& operator=(CLASS&&) = delete;

#define LINN_DISABLE_COPY_MOVE(CLASS) \
  LINN_DISABLE_COPY(CLASS)            \
  LINN_DISABLE_MOVE(CLASS)

#endif // !LINN_UTILS_H_
