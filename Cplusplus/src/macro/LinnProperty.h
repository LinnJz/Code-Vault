#ifndef LINN_PROPERTY_H_
#define LINN_PROPERTY_H_

#include <cstdlib>          // std::abort
#include <iostream>         // std::cerr
#include <memory>           // std::unique_ptr
#include <source_location>  // C++20
#include <type_traits>      // std::remove_cvref_t
#include <utility>          // std::move

#include "LinnUtils.h"

#define LINN_DECLARE_PRIVATE(CLASS)                                            \
  friend class CLASS##Private;                                                 \
  inline CLASS##Private*       d_func() noexcept { return d_ptr.get(); }       \
  inline const CLASS##Private* d_func() const noexcept { return d_ptr.get(); }

#define LINN_DECLARE_PUBLIC(CLASS)                              \
  friend class CLASS;                                           \
  inline CLASS*       q_func() noexcept { return q_ptr; }       \
  inline const CLASS* q_func() const noexcept { return q_ptr; }

#define LINN_D(CLASS) auto* const d = d_func()
#define LINN_Q(CLASS) auto* const q = q_func()

#define LINN_Q_CREATE(CLASS)  \
                              \
protected:                    \
                              \
  CLASS* q_ptr;               \
                              \
private:                      \
                              \
  LINN_DECLARE_PUBLIC(CLASS)

#define LINN_D_CREATE(CLASS)             \
                                         \
protected:                               \
                                         \
  explicit CLASS(CLASS##Private& dd);    \
  std::unique_ptr<CLASS##Private> d_ptr; \
                                         \
private:                                 \
                                         \
  LINN_DISABLE_COPY(CLASS)               \
  LINN_DECLARE_PRIVATE(CLASS)

#define LINN_PROPERTY_TAG_DEFAULT         0
#define LINN_PROPERTY_TAG_CONTRACT_ALWAYS 1
#define LINN_PROPERTY_TAG_CONTRACT_DEBUG  2
#define LINN_PROPERTY_TAG_ASSIGN_MOVE     3
#define LINN_PROPERTY_TAG_ASSIGN_COPY     4

#define LINN_PROPERTY_DEFAULT(val) LINN_PAIR(LINN_PROPERTY_TAG_DEFAULT, val)
#define LINN_PROPERTY_CONTRACT(expr)                 \
  LINN_PAIR(LINN_PROPERTY_TAG_CONTRACT_ALWAYS, expr)
#define LINN_PROPERTY_CONTRACT_D(expr)              \
  LINN_PAIR(LINN_PROPERTY_TAG_CONTRACT_DEBUG, expr)

#define LINN_PROPERTY_MOVE(name) LINN_PAIR(LINN_PROPERTY_TAG_ASSIGN_MOVE, name)
#define LINN_PROPERTY_COPY(name) LINN_PAIR(LINN_PROPERTY_TAG_ASSIGN_COPY, name)

#ifdef LINN_PROPERTY_ENABLE_SHORT_MACROS
#  ifndef DEFAULT
#    define DEFAULT(val) LINN_PROPERTY_DEFAULT(val)
#  endif
#  ifndef CONTRACT
#    define CONTRACT(expr) LINN_PROPERTY_CONTRACT(expr)
#  endif
#  ifndef CONTRACT_D
#    define CONTRACT_D(expr) LINN_PROPERTY_CONTRACT_D(expr)
#  endif
#  ifndef MOVE
#    define MOVE(name) LINN_PROPERTY_MOVE(name)
#  endif
#  ifndef COPY
#    define COPY(name) LINN_PROPERTY_COPY(name)
#  endif
#  define PROPERTY_CREATE_X(SET_TYPE, GET_TYPE, NAME, ...)           \
    LINN_PROPERTY_CREATE_X_FROM_PAIR(SET_TYPE, GET_TYPE,             \
                                     LINN_PROPERTY_NAME_PAIR(NAME)   \
                                         __VA_OPT__(, ) __VA_ARGS__)

#  define PROPERTY_CREATE(TYPE, NAME, ...)                         \
    PROPERTY_CREATE_X(TYPE, TYPE, NAME __VA_OPT__(, ) __VA_ARGS__)

#  define PROPERTY_CREATE_2(TYPE, NAME, ...)           \
    PROPERTY_CREATE_X(TYPE, std::remove_cvref_t<TYPE>, \
                      NAME __VA_OPT__(, ) __VA_ARGS__)
#endif

#define LINN_PROPERTY_CREATE_X(SET_TYPE, GET_TYPE, NAME, ...)                   \
  LINN_PROPERTY_CREATE_X_FROM_PAIR(SET_TYPE, GET_TYPE,                          \
                                   LINN_PROPERTY_NAME_PAIR(NAME) __VA_OPT__(, ) \
                                       __VA_ARGS__)

#define LINN_PROPERTY_CREATE(TYPE, NAME, ...)                         \
  LINN_PROPERTY_CREATE_X(TYPE, TYPE, NAME __VA_OPT__(, ) __VA_ARGS__)

#define LINN_PROPERTY_CREATE_2(TYPE, NAME, ...)           \
  LINN_PROPERTY_CREATE_X(TYPE, std::remove_cvref_t<TYPE>, \
                         NAME __VA_OPT__(, ) __VA_ARGS__)

#define LINN_PROPERTY_CREATE_X_FROM_PAIR(SET_TYPE, GET_TYPE, PAIR, ...) \
  LINN_PROPERTY_CREATE_X_FROM_PAIR_UNPACK(                              \
      SET_TYPE, GET_TYPE, LINN_PROPERTY_NAME_ID(PAIR),                  \
      LINN_PROPERTY_NAME_ASSIGN_TAG(PAIR) __VA_OPT__(, ) __VA_ARGS__)

#define LINN_PROPERTY_CREATE_X_FROM_PAIR_UNPACK(SET_TYPE, GET_TYPE, NAME, \
                                                ASSIGN_TAG, ...)          \
                                                                          \
public:                                                                   \
                                                                          \
  void LINN_CAT(Set, NAME)(SET_TYPE NAME)                                 \
  {                                                                       \
    LINN_CONTRACT_CHECK_FROM_ARGS(NAME __VA_OPT__(, ) __VA_ARGS__)        \
    LINN_CAT(m_, NAME) =                                                  \
        LINN_IF(LINN_IS_ASSIGN_MOVE(ASSIGN_TAG))(std::move(NAME), NAME);  \
  }                                                                       \
  [[nodiscard]] GET_TYPE LINN_CAT(Get, NAME)() const noexcept             \
  {                                                                       \
    return LINN_CAT(m_, NAME);                                            \
  }                                                                       \
                                                                          \
private:                                                                  \
                                                                          \
  std::remove_cvref_t<SET_TYPE> LINN_CAT(m_, NAME)                        \
      LINN_MEMBER_INIT_FROM_ARGS(__VA_ARGS__);
/*
 *
 */
#pragma region LINN_PROPERTY_HELPER_CORE

#ifndef NDEBUG
#  define LINN_CONTRACT_ASSERT_DEBUG(expr, msg, location)                  \
    do {                                                                   \
      if (!(expr)) [[unlikely]] {                                          \
        std::cerr << "[CONTRACT VIOLATION] " << (msg) << "\n"              \
                  << "  Condition: " << #expr << "\n"                      \
                  << "  File: " << (location).file_name() << ":"           \
                  << (location).line() << "\n"                             \
                  << "  Function: " << (location).function_name() << "\n"; \
        std::abort();                                                      \
      }                                                                    \
    } while (0)
#else
#  define LINN_CONTRACT_ASSERT_DEBUG(expr, msg, location) ((void)0)
#endif

#define LINN_CONTRACT_ASSERT_ALWAYS(expr, msg)                       \
  do {                                                               \
    if (!(expr)) [[unlikely]] {                                      \
      std::cerr << "[CRITICAL CONTRACT VIOLATION] " << (msg) << "\n" \
                << "  Condition: " << #expr << "\n";                 \
      std::abort();                                                  \
    }                                                                \
  } while (0)

#define LINN_PROPERTY_NAME_PAIR(NAME)                                    \
  LINN_IF(LINN_IS_PAREN(LINN_EXPAND(NAME)))(                             \
      LINN_EXPAND(NAME), LINN_PAIR(LINN_PROPERTY_TAG_ASSIGN_COPY, NAME))

#define LINN_PROPERTY_NAME_ASSIGN_TAG(PAIR) LINN_PAIR_FIRST(PAIR)
#define LINN_PROPERTY_NAME_ID(PAIR)         LINN_PAIR_SECOND(PAIR)

#define LINN_PROPERTY_PP_EQ(a, b) LINN_CAT(LINN_PROPERTY_PP_EQ_, LINN_CAT(a, b))
#define LINN_PROPERTY_PP_EQ_00    1
#define LINN_PROPERTY_PP_EQ_01    0
#define LINN_PROPERTY_PP_EQ_02    0
#define LINN_PROPERTY_PP_EQ_03    0
#define LINN_PROPERTY_PP_EQ_04    0
#define LINN_PROPERTY_PP_EQ_10    0
#define LINN_PROPERTY_PP_EQ_11    1
#define LINN_PROPERTY_PP_EQ_12    0
#define LINN_PROPERTY_PP_EQ_13    0
#define LINN_PROPERTY_PP_EQ_14    0
#define LINN_PROPERTY_PP_EQ_20    0
#define LINN_PROPERTY_PP_EQ_21    0
#define LINN_PROPERTY_PP_EQ_22    1
#define LINN_PROPERTY_PP_EQ_23    0
#define LINN_PROPERTY_PP_EQ_24    0
#define LINN_PROPERTY_PP_EQ_30    0
#define LINN_PROPERTY_PP_EQ_31    0
#define LINN_PROPERTY_PP_EQ_32    0
#define LINN_PROPERTY_PP_EQ_33    1
#define LINN_PROPERTY_PP_EQ_34    0
#define LINN_PROPERTY_PP_EQ_40    0
#define LINN_PROPERTY_PP_EQ_41    0
#define LINN_PROPERTY_PP_EQ_42    0
#define LINN_PROPERTY_PP_EQ_43    0
#define LINN_PROPERTY_PP_EQ_44    1

#define LINN_IS_DEFAULT(tag) LINN_PROPERTY_PP_EQ(tag, LINN_PROPERTY_TAG_DEFAULT)
#define LINN_IS_ASSIGN_MOVE(tag)                          \
  LINN_PROPERTY_PP_EQ(tag, LINN_PROPERTY_TAG_ASSIGN_MOVE)

#define LINN_MEMBER_INIT_FROM_ARGS_0()
#define LINN_MEMBER_INIT_FROM_ARGS_1(a1)                                    \
  LINN_IF(LINN_IS_DEFAULT(LINN_PAIR_FIRST(a1)))({ LINN_PAIR_SECOND(a1) }, )
#define LINN_MEMBER_INIT_FROM_ARGS_2(a1, a2)                      \
  LINN_IF(LINN_IS_DEFAULT(LINN_PAIR_FIRST(a1)))(                  \
      { LINN_PAIR_SECOND(a1) }, LINN_MEMBER_INIT_FROM_ARGS_1(a2))
#define LINN_MEMBER_INIT_FROM_ARGS_3(a1, a2, a3)                      \
  LINN_IF(LINN_IS_DEFAULT(LINN_PAIR_FIRST(a1)))(                      \
      { LINN_PAIR_SECOND(a1) }, LINN_MEMBER_INIT_FROM_ARGS_2(a2, a3))

#define LINN_MEMBER_INIT_FROM_ARGS(...)                                       \
  LINN_CAT(LINN_MEMBER_INIT_FROM_ARGS_, LINN_NARGS(__VA_ARGS__))(__VA_ARGS__)

#define LINN_CONTRACT_CHECK_FROM_TAG_0(NAME, EXPR)
#define LINN_CONTRACT_CHECK_FROM_TAG_1(NAME, EXPR)                               \
  LINN_CONTRACT_ASSERT_ALWAYS(                                                   \
      (EXPR), "Critical invariant violated in " LINN_STRINGIFY(NAME) " setter");
#define LINN_CONTRACT_CHECK_FROM_TAG_2(NAME, EXPR)                       \
  LINN_CONTRACT_ASSERT_DEBUG(                                            \
      (EXPR), "Precondition failed for " LINN_STRINGIFY(NAME) " setter", \
      std::source_location::current());
#define LINN_CONTRACT_CHECK_FROM_TAG_3(NAME, EXPR)
#define LINN_CONTRACT_CHECK_FROM_TAG_4(NAME, EXPR)

#define LINN_CONTRACT_CHECK_FROM_ARG(NAME, pair)                \
  LINN_CAT(LINN_CONTRACT_CHECK_FROM_TAG_,                       \
           LINN_PAIR_FIRST(pair))(NAME, LINN_PAIR_SECOND(pair))

#define LINN_CONTRACT_CHECK_FROM_ARGS_0(NAME)
#define LINN_CONTRACT_CHECK_FROM_ARGS_1(NAME, a1) \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a1)
#define LINN_CONTRACT_CHECK_FROM_ARGS_2(NAME, a1, a2) \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a1)              \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a2)
#define LINN_CONTRACT_CHECK_FROM_ARGS_3(NAME, a1, a2, a3) \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a1)                  \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a2)                  \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a3)

#define LINN_CONTRACT_CHECK_FROM_ARGS(NAME, ...)                     \
  LINN_CAT(LINN_CONTRACT_CHECK_FROM_ARGS_,                           \
           LINN_NARGS(__VA_ARGS__))(NAME __VA_OPT__(, ) __VA_ARGS__)

#pragma endregion LINN_PROPERTY_HELPER_CORE

#endif  // !LINN_PROPERTY_H_
