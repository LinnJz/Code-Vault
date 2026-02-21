#ifndef LINN_PROPERTY_H_
#define LINN_PROPERTY_H_

#include <cstdlib>         // std::abort
#include <iostream>        // std::cerr
#include <memory>          // std::unique_ptr
#include <source_location> // C++20
#include <type_traits>     // std::remove_cvref_t
#include <utility>         // std::move

#include "LinnUtils.h"

#define PIMPL_DECLARE_D_PRIVATE(CLASS)                                         \
  friend class CLASS##Private;                                                 \
  inline CLASS##Private *d_func() noexcept { return d_ptr.get(); }             \
  inline const CLASS##Private *d_func() const noexcept { return d_ptr.get(); }

#define PIMPL_DECLARE_Q_PUBLIC(CLASS)                           \
  friend class CLASS;                                           \
  inline CLASS *q_func() noexcept { return q_ptr; }             \
  inline const CLASS *q_func() const noexcept { return q_ptr; }

#define PIMPL_D(CLASS) auto *const d = d_func()
#define PIMPL_Q(CLASS) auto *const q = q_func()

#define PIMPL_D_CREATE(CLASS)    \
                                 \
protected:                       \
  CLASS *q_ptr;                  \
                                 \
private:                         \
  PIMPL_DECLARE_Q_PUBLIC(CLASS);

#define PIMPL_Q_CREATE(CLASS)            \
                                         \
protected:                               \
  explicit CLASS(CLASS##Private& dd);    \
  std::unique_ptr<CLASS##Private> d_ptr; \
                                         \
private:                                 \
  LINN_DISABLE_COPY(CLASS);              \
  PIMPL_DECLARE_D_PRIVATE(CLASS)

#define DEFAULT(val)     (DEFAULT, val)
#define CONTRACT(expr)   (CONTRACT_ALWAYS, expr)
#define CONTRACT_D(expr) (CONTRACT_DEBUG, expr)

#define MOVE(name) LINN_PAIR(ASSIGN_MOVE, name)
#define COPY(name) LINN_PAIR(ASSIGN_COPY, name)

#define PROPERTY_CREATE_X(SET_TYPE, GET_TYPE, NAME, ...)                                                    \
  PROPERTY_CREATE_X_FROM_PAIR(SET_TYPE, GET_TYPE, LINN_PROPERTY_NAME_PAIR(NAME) __VA_OPT__(, ) __VA_ARGS__)

#define PROPERTY_CREATE(TYPE, NAME, ...) PROPERTY_CREATE_X(TYPE, TYPE, NAME __VA_OPT__(, ) __VA_ARGS__)

#define PROPERTY_CREATE_2(TYPE, NAME, ...)                                            \
  PROPERTY_CREATE_X(TYPE, std::remove_cvref_t<TYPE>, NAME __VA_OPT__(, ) __VA_ARGS__)

#define PROPERTY_CREATE_X_FROM_PAIR_UNPACK(SET_TYPE, GET_TYPE, NAME, ASSIGN_TAG, ...)        \
                                                                                             \
public:                                                                                      \
  void LINN_CAT(Set, NAME)(SET_TYPE NAME)                                                    \
  {                                                                                          \
    LINN_CONTRACT_CHECK_FROM_ARGS(NAME __VA_OPT__(, ) __VA_ARGS__)                           \
    LINN_CAT(m_, NAME) = LINN_IF(LINN_IS_ASSIGN_MOVE(ASSIGN_TAG))(std::move(NAME), NAME);    \
  }                                                                                          \
  [[nodiscard]] GET_TYPE LINN_CAT(Get, NAME)() const noexcept { return LINN_CAT(m_, NAME); } \
                                                                                             \
private:                                                                                     \
  std::remove_cvref_t<SET_TYPE> LINN_CAT(m_, NAME) LINN_MEMBER_INIT_FROM_ARGS(__VA_ARGS__)

#define PROPERTY_CREATE_X_FROM_PAIR(SET_TYPE, GET_TYPE, PAIR, ...)                                                     \
  PROPERTY_CREATE_X_FROM_PAIR_UNPACK(                                                                                  \
      SET_TYPE, GET_TYPE, LINN_PROPERTY_NAME_ID(PAIR), LINN_PROPERTY_NAME_ASSIGN_TAG(PAIR) __VA_OPT__(, ) __VA_ARGS__)
/*
 *
 */
#pragma region LINN_PROPERTY_HELPER_CORE

#ifndef NDEBUG
#define LINN_CONTRACT_ASSERT_DEBUG(expr, msg, location)                                     \
  do                                                                                        \
  {                                                                                         \
    if (!(expr)) [[unlikely]]                                                               \
    {                                                                                       \
      std::cerr << "[CONTRACT VIOLATION] " << (msg) << "\n"                                 \
                << "  Condition: " << #expr << "\n"                                         \
                << "  File: " << (location).file_name() << ":" << (location).line() << "\n" \
                << "  Function: " << (location).function_name() << "\n";                    \
      std::abort();                                                                         \
    }                                                                                       \
  }                                                                                         \
  while (0)
#else
#define LINN_CONTRACT_ASSERT_DEBUG(expr, msg, location) ((void) 0)
#endif

#define LINN_CONTRACT_ASSERT_ALWAYS(expr, msg)                       \
  do                                                                 \
  {                                                                  \
    if (!(expr)) [[unlikely]]                                        \
    {                                                                \
      std::cerr << "[CRITICAL CONTRACT VIOLATION] " << (msg) << "\n" \
                << "  Condition: " << #expr << "\n";                 \
      std::abort();                                                  \
    }                                                                \
  }                                                                  \
  while (0)

#define LINN_PROPERTY_NAME_PAIR(NAME)                                                        \
  LINN_IF(LINN_IS_PAREN(LINN_EXPAND(NAME)))(LINN_EXPAND(NAME), LINN_PAIR(ASSIGN_COPY, NAME))

#define LINN_PROPERTY_NAME_ASSIGN_TAG(PAIR) LINN_PAIR_FIRST(PAIR)
#define LINN_PROPERTY_NAME_ID(PAIR)         LINN_PAIR_SECOND(PAIR)

#define LINN_IS_DEFAULT(tag)            LINN_CAT(LINN_IS_DEFAULT_, tag)
#define LINN_IS_DEFAULT_DEFAULT         1
#define LINN_IS_DEFAULT_CONTRACT_ALWAYS 0
#define LINN_IS_DEFAULT_CONTRACT_DEBUG  0
#define LINN_IS_DEFAULT_ASSIGN_MOVE     0
#define LINN_IS_DEFAULT_ASSIGN_COPY     0

#define LINN_IS_ASSIGN_MOVE(tag)            LINN_CAT(LINN_IS_ASSIGN_MOVE_, tag)
#define LINN_IS_ASSIGN_MOVE_DEFAULT         0
#define LINN_IS_ASSIGN_MOVE_CONTRACT_ALWAYS 0
#define LINN_IS_ASSIGN_MOVE_CONTRACT_DEBUG  0
#define LINN_IS_ASSIGN_MOVE_ASSIGN_MOVE     1
#define LINN_IS_ASSIGN_MOVE_ASSIGN_COPY     0

#define LINN_MEMBER_INIT_FROM_ARGS_0()
#define LINN_MEMBER_INIT_FROM_ARGS_1(a1) LINN_IF(LINN_IS_DEFAULT(LINN_PAIR_FIRST(a1)))({ LINN_PAIR_SECOND(a1) }, )
#define LINN_MEMBER_INIT_FROM_ARGS_2(a1, a2)                                                                \
  LINN_IF(LINN_IS_DEFAULT(LINN_PAIR_FIRST(a1)))({ LINN_PAIR_SECOND(a1) }, LINN_MEMBER_INIT_FROM_ARGS_1(a2))
#define LINN_MEMBER_INIT_FROM_ARGS_3(a1, a2, a3)                                                                \
  LINN_IF(LINN_IS_DEFAULT(LINN_PAIR_FIRST(a1)))({ LINN_PAIR_SECOND(a1) }, LINN_MEMBER_INIT_FROM_ARGS_2(a2, a3))

#define LINN_MEMBER_INIT_FROM_ARGS(...) LINN_CAT(LINN_MEMBER_INIT_FROM_ARGS_, LINN_NARGS(__VA_ARGS__))(__VA_ARGS__)

#define LINN_CONTRACT_CHECK_FROM_TAG_DEFAULT(NAME, EXPR)
#define LINN_CONTRACT_CHECK_FROM_TAG_CONTRACT_ALWAYS(NAME, EXPR)                                         \
  LINN_CONTRACT_ASSERT_ALWAYS((EXPR), "Critical invariant violated in " LINN_STRINGIFY(NAME) " setter");
#define LINN_CONTRACT_CHECK_FROM_TAG_CONTRACT_DEBUG(NAME, EXPR)                                            \
  LINN_CONTRACT_ASSERT_DEBUG(                                                                              \
      (EXPR), "Precondition failed for " LINN_STRINGIFY(NAME) " setter", std::source_location::current());
#define LINN_CONTRACT_CHECK_FROM_TAG_ASSIGN_MOVE(NAME, EXPR)
#define LINN_CONTRACT_CHECK_FROM_TAG_ASSIGN_COPY(NAME, EXPR)

#define LINN_CONTRACT_CHECK_FROM_ARG(NAME, pair)                                               \
  LINN_CAT(LINN_CONTRACT_CHECK_FROM_TAG_, LINN_PAIR_FIRST(pair))(NAME, LINN_PAIR_SECOND(pair))

#define LINN_CONTRACT_CHECK_FROM_ARGS_0(NAME)
#define LINN_CONTRACT_CHECK_FROM_ARGS_1(NAME, a1) LINN_CONTRACT_CHECK_FROM_ARG(NAME, a1)
#define LINN_CONTRACT_CHECK_FROM_ARGS_2(NAME, a1, a2) \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a1)              \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a2)
#define LINN_CONTRACT_CHECK_FROM_ARGS_3(NAME, a1, a2, a3) \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a1)                  \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a2)                  \
  LINN_CONTRACT_CHECK_FROM_ARG(NAME, a3)

#define LINN_CONTRACT_CHECK_FROM_ARGS(NAME, ...)                                                     \
  LINN_CAT(LINN_CONTRACT_CHECK_FROM_ARGS_, LINN_NARGS(__VA_ARGS__))(NAME __VA_OPT__(, ) __VA_ARGS__)

#pragma endregion LINN_PROPERTY_HELPER_CORE

#endif // !LINN_PROPERTY_H_
