#ifndef LINN_SINGLETON_H_
#define LINN_SINGLETON_H_

#include <atomic>  // std::atomic_bool, std::memory_order (C++11)
#include <memory>  // std::unique_ptr, std::shared_ptr, std::make_unique, std::make_shared (C++11)
#include <mutex>  // std::once_flag, std::call_once, std::mutex, std::scoped_lock (C++17)
#include <utility>  // std::forward (C++11)

#include "LinnUtils.h"

#define LINN_SINGLETON_MODE_UNIQUE 0
#define LINN_SINGLETON_MODE_SHARED 1
#define LINN_SINGLETON_MODE_MEYERS 2

#define LINN_SINGLETON_DESTROY_NO_DESTROY          0
#define LINN_SINGLETON_DESTROY_ENABLE_DESTROY      1
#define LINN_SINGLETON_DESTROY_ENABLE_LOCK_DESTROY 2

#define LINN_SINGLETON_UNIQUE(CLASS) LINN_PAIR(LINN_SINGLETON_MODE_UNIQUE, CLASS)
#define LINN_SINGLETON_SHARED(CLASS) LINN_PAIR(LINN_SINGLETON_MODE_SHARED, CLASS)
#define LINN_SINGLETON_MEYERS(CLASS) LINN_PAIR(LINN_SINGLETON_MODE_MEYERS, CLASS)

#define LINN_SINGLETON_NO_DESTROY     LINN_SINGLETON_DESTROY_NO_DESTROY
#define LINN_SINGLETON_ENABLE_DESTROY LINN_SINGLETON_DESTROY_ENABLE_DESTROY
#define LINN_SINGLETON_ENABLE_LOCK_DESTROY   \
  LINN_SINGLETON_DESTROY_ENABLE_LOCK_DESTROY

#ifdef LINN_SINGLETON_ENABLE_SHORT_MACROS
#  ifndef UNIQUE
#    define UNIQUE(CLASS) LINN_SINGLETON_UNIQUE(CLASS)
#  endif
#  ifndef SHARED
#    define SHARED(CLASS) LINN_SINGLETON_SHARED(CLASS)
#  endif
#  ifndef MEYERS
#    define MEYERS(CLASS) LINN_SINGLETON_MEYERS(CLASS)
#  endif
#  ifndef NO_DESTROY
#    define NO_DESTROY LINN_SINGLETON_NO_DESTROY
#  endif
#  ifndef ENABLE_DESTROY
#    define ENABLE_DESTROY LINN_SINGLETON_ENABLE_DESTROY
#  endif
#  ifndef ENABLE_LOCK_DESTROY
#    define ENABLE_LOCK_DESTROY LINN_SINGLETON_ENABLE_LOCK_DESTROY
#  endif
#  define SINGLETON_CREATE(...)                                       \
    LINN_CAT(SINGLETON_CREATE_, LINN_NARGS(__VA_ARGS__))(__VA_ARGS__)
#  define SINGLETON_CREATE_1(MODE_PAIR)                      \
    SINGLETON_CREATE_2(MODE_PAIR, LINN_SINGLETON_NO_DESTROY)
#  define SINGLETON_CREATE_2(MODE_PAIR, DESTROY_MODE)        \
    LINN_SINGLETON_CREATE_FROM_PAIR(MODE_PAIR, DESTROY_MODE)
#endif
#define LINN_SINGLETON_CREATE(...)                                       \
  LINN_CAT(LINN_SINGLETON_CREATE_, LINN_NARGS(__VA_ARGS__))(__VA_ARGS__)
#define LINN_SINGLETON_CREATE_1(MODE_PAIR)                      \
  LINN_SINGLETON_CREATE_2(MODE_PAIR, LINN_SINGLETON_NO_DESTROY)
#define LINN_SINGLETON_CREATE_2(MODE_PAIR, DESTROY_MODE)   \
  LINN_SINGLETON_CREATE_FROM_PAIR(MODE_PAIR, DESTROY_MODE)
/*
 *
 */
#pragma region LINN_SINGLETON_HELPER_CORE

#define LINN_SINGLETON_PP_EQ(a, b) LINN_CAT(LINN_SINGLETON_PP_EQ_, LINN_CAT(a, b))
#define LINN_SINGLETON_PP_EQ_00    1
#define LINN_SINGLETON_PP_EQ_01    0
#define LINN_SINGLETON_PP_EQ_02    0
#define LINN_SINGLETON_PP_EQ_10    0
#define LINN_SINGLETON_PP_EQ_11    1
#define LINN_SINGLETON_PP_EQ_12    0
#define LINN_SINGLETON_PP_EQ_20    0
#define LINN_SINGLETON_PP_EQ_21    0
#define LINN_SINGLETON_PP_EQ_22    1

#define LINN_SINGLETON_CREATE_BODY_UNIQUE_NO_DESTROY(CLASS)           \
                                                                      \
public:                                                               \
                                                                      \
  LINN_DISABLE_COPY_MOVE(CLASS)                                       \
                                                                      \
  template<typename... Args>                                          \
  static CLASS* getInstance(Args&&... args)                           \
  {                                                                   \
    std::call_once(s_InitFlag,                                        \
                   [args_tuple = std::forward_as_tuple(               \
                        std::forward<Args>(args)...)]() mutable {     \
      std::apply([](auto&&... unpacked_args) {                        \
        s_Instance = _InstanceCreateImpl(                             \
            std::forward<decltype(unpacked_args)>(unpacked_args)...); \
      }, std::move(args_tuple));                                      \
    });                                                               \
    return s_Instance.get();                                          \
  }                                                                   \
                                                                      \
private:                                                              \
                                                                      \
  friend std::default_delete<CLASS>;                                  \
                                                                      \
  template<typename... Args>                                          \
  static std::unique_ptr<CLASS> _InstanceCreateImpl(Args&&... args)   \
  {                                                                   \
    struct LinnSingletonMakeHelper : public CLASS                     \
    {                                                                 \
      explicit LinnSingletonMakeHelper(Args&&... args)                \
          : CLASS(std::forward<Args>(args)...)                        \
      {                                                               \
      }                                                               \
    };                                                                \
                                                                      \
    return std::make_unique<LinnSingletonMakeHelper>(                 \
        std::forward<Args>(args)...);                                 \
  }                                                                   \
                                                                      \
  inline static std::once_flag         s_InitFlag;                    \
  inline static std::unique_ptr<CLASS> s_Instance { nullptr };

#define LINN_SINGLETON_CREATE_BODY_UNIQUE_ENABLE_DESTROY(CLASS)      \
                                                                     \
public:                                                              \
                                                                     \
  LINN_DISABLE_COPY_MOVE(CLASS)                                      \
                                                                     \
  template<typename... Args>                                         \
  static CLASS* getInstance(Args&&... args)                          \
  {                                                                  \
    if (s_Destroyed) { return nullptr; }                             \
    if (!s_Instance) {                                               \
      s_Instance = _InstanceCreateImpl(std::forward<Args>(args)...); \
    }                                                                \
    return s_Instance.get();                                         \
  }                                                                  \
                                                                     \
  static void destroyInstance()                                      \
  {                                                                  \
    s_Instance.reset();                                              \
    s_Destroyed = true;                                              \
  }                                                                  \
                                                                     \
  static bool isDestroyed() { return s_Destroyed; }                  \
                                                                     \
private:                                                             \
                                                                     \
  friend std::default_delete<CLASS>;                                 \
                                                                     \
  template<typename... Args>                                         \
  static std::unique_ptr<CLASS> _InstanceCreateImpl(Args&&... args)  \
  {                                                                  \
    struct LinnSingletonMakeHelper : public CLASS                    \
    {                                                                \
      explicit LinnSingletonMakeHelper(Args&&... args)               \
          : CLASS(std::forward<Args>(args)...)                       \
      {                                                              \
      }                                                              \
    };                                                               \
                                                                     \
    return std::make_unique<LinnSingletonMakeHelper>(                \
        std::forward<Args>(args)...);                                \
  }                                                                  \
                                                                     \
  inline static bool                   s_Destroyed { false };        \
  inline static std::unique_ptr<CLASS> s_Instance { nullptr };

#define LINN_SINGLETON_CREATE_BODY_UNIQUE_ENABLE_LOCK_DESTROY(CLASS)     \
                                                                         \
public:                                                                  \
                                                                         \
  LINN_DISABLE_COPY_MOVE(CLASS)                                          \
                                                                         \
  template<typename... Args>                                             \
  static CLASS* getInstance(Args&&... args)                              \
  {                                                                      \
    if (s_Destroyed.load(std::memory_order_acquire)) { return nullptr; } \
    std::scoped_lock lock(s_Mutex);                                      \
    if (s_Destroyed.load(std::memory_order_relaxed)) { return nullptr; } \
    if (!s_Instance) {                                                   \
      s_Instance = _InstanceCreateImpl(std::forward<Args>(args)...);     \
    }                                                                    \
    return s_Instance.get();                                             \
  }                                                                      \
                                                                         \
  static void destroyInstance()                                          \
  {                                                                      \
    std::scoped_lock lock(s_Mutex);                                      \
    s_Instance.reset();                                                  \
    s_Destroyed.store(true, std::memory_order_release);                  \
  }                                                                      \
                                                                         \
  static bool isDestroyed()                                              \
  {                                                                      \
    return s_Destroyed.load(std::memory_order_acquire);                  \
  }                                                                      \
                                                                         \
private:                                                                 \
                                                                         \
  friend std::default_delete<CLASS>;                                     \
                                                                         \
  template<typename... Args>                                             \
  static std::unique_ptr<CLASS> _InstanceCreateImpl(Args&&... args)      \
  {                                                                      \
    struct LinnSingletonMakeHelper : public CLASS                        \
    {                                                                    \
      explicit LinnSingletonMakeHelper(Args&&... args)                   \
          : CLASS(std::forward<Args>(args)...)                           \
      {                                                                  \
      }                                                                  \
    };                                                                   \
                                                                         \
    return std::make_unique<LinnSingletonMakeHelper>(                    \
        std::forward<Args>(args)...);                                    \
  }                                                                      \
                                                                         \
  inline static std::mutex             s_Mutex;                          \
  inline static std::atomic_bool       s_Destroyed { false };            \
  inline static std::unique_ptr<CLASS> s_Instance { nullptr };

#define LINN_SINGLETON_CREATE_BODY_SHARED_NO_DESTROY(CLASS)           \
                                                                      \
public:                                                               \
                                                                      \
  LINN_DISABLE_COPY_MOVE(CLASS)                                       \
                                                                      \
  template<typename... Args>                                          \
  static std::shared_ptr<CLASS> getInstance(Args&&... args)           \
  {                                                                   \
    std::call_once(s_InitFlag,                                        \
                   [args_tuple = std::forward_as_tuple(               \
                        std::forward<Args>(args)...)]() mutable {     \
      std::apply([](auto&&... unpacked_args) {                        \
        s_Instance = _InstanceCreateImpl(                             \
            std::forward<decltype(unpacked_args)>(unpacked_args)...); \
      }, std::move(args_tuple));                                      \
    });                                                               \
    return s_Instance;                                                \
  }                                                                   \
                                                                      \
private:                                                              \
                                                                      \
  friend std::default_delete<CLASS>;                                  \
                                                                      \
  template<typename... Args>                                          \
  static std::shared_ptr<CLASS> _InstanceCreateImpl(Args&&... args)   \
  {                                                                   \
    struct LinnSingletonMakeHelper : public CLASS                     \
    {                                                                 \
      explicit LinnSingletonMakeHelper(Args&&... args)                \
          : CLASS(std::forward<Args>(args)...)                        \
      {                                                               \
      }                                                               \
    };                                                                \
                                                                      \
    return std::make_shared<LinnSingletonMakeHelper>(                 \
        std::forward<Args>(args)...);                                 \
  }                                                                   \
                                                                      \
  inline static std::once_flag         s_InitFlag;                    \
  inline static std::shared_ptr<CLASS> s_Instance { nullptr };

#define LINN_SINGLETON_CREATE_BODY_SHARED_ENABLE_DESTROY(CLASS)      \
                                                                     \
public:                                                              \
                                                                     \
  LINN_DISABLE_COPY_MOVE(CLASS)                                      \
                                                                     \
  template<typename... Args>                                         \
  static std::shared_ptr<CLASS> getInstance(Args&&... args)          \
  {                                                                  \
    if (s_Destroyed) { return {}; }                                  \
    if (!s_Instance) {                                               \
      s_Instance = _InstanceCreateImpl(std::forward<Args>(args)...); \
    }                                                                \
    return s_Instance;                                               \
  }                                                                  \
                                                                     \
  static void destroyInstance()                                      \
  {                                                                  \
    s_Instance.reset();                                              \
    s_Destroyed = true;                                              \
  }                                                                  \
                                                                     \
  static bool isDestroyed() { return s_Destroyed; }                  \
                                                                     \
private:                                                             \
                                                                     \
  friend std::default_delete<CLASS>;                                 \
                                                                     \
  template<typename... Args>                                         \
  static std::shared_ptr<CLASS> _InstanceCreateImpl(Args&&... args)  \
  {                                                                  \
    struct LinnSingletonMakeHelper : public CLASS                    \
    {                                                                \
      explicit LinnSingletonMakeHelper(Args&&... args)               \
          : CLASS(std::forward<Args>(args)...)                       \
      {                                                              \
      }                                                              \
    };                                                               \
                                                                     \
    return std::make_shared<LinnSingletonMakeHelper>(                \
        std::forward<Args>(args)...);                                \
  }                                                                  \
                                                                     \
  inline static bool                   s_Destroyed { false };        \
  inline static std::shared_ptr<CLASS> s_Instance { nullptr };

#define LINN_SINGLETON_CREATE_BODY_SHARED_ENABLE_LOCK_DESTROY(CLASS) \
                                                                     \
public:                                                              \
                                                                     \
  LINN_DISABLE_COPY_MOVE(CLASS)                                      \
                                                                     \
  template<typename... Args>                                         \
  static std::shared_ptr<CLASS> getInstance(Args&&... args)          \
  {                                                                  \
    if (s_Destroyed.load(std::memory_order_acquire)) { return {}; }  \
    std::scoped_lock lock(s_Mutex);                                  \
    if (s_Destroyed.load(std::memory_order_relaxed)) { return {}; }  \
    if (!s_Instance) {                                               \
      s_Instance = _InstanceCreateImpl(std::forward<Args>(args)...); \
    }                                                                \
    return s_Instance;                                               \
  }                                                                  \
                                                                     \
  static void destroyInstance()                                      \
  {                                                                  \
    std::scoped_lock lock(s_Mutex);                                  \
    s_Instance.reset();                                              \
    s_Destroyed.store(true, std::memory_order_release);              \
  }                                                                  \
                                                                     \
  static bool isDestroyed()                                          \
  {                                                                  \
    return s_Destroyed.load(std::memory_order_acquire);              \
  }                                                                  \
                                                                     \
private:                                                             \
                                                                     \
  friend std::default_delete<CLASS>;                                 \
                                                                     \
  template<typename... Args>                                         \
  static std::shared_ptr<CLASS> _InstanceCreateImpl(Args&&... args)  \
  {                                                                  \
    struct LinnSingletonMakeHelper : public CLASS                    \
    {                                                                \
      explicit LinnSingletonMakeHelper(Args&&... args)               \
          : CLASS(std::forward<Args>(args)...)                       \
      {                                                              \
      }                                                              \
    };                                                               \
                                                                     \
    return std::make_shared<LinnSingletonMakeHelper>(                \
        std::forward<Args>(args)...);                                \
  }                                                                  \
                                                                     \
  inline static std::mutex             s_Mutex;                      \
  inline static std::atomic_bool       s_Destroyed { false };        \
  inline static std::shared_ptr<CLASS> s_Instance { nullptr };

#define LINN_SINGLETON_CREATE_BODY_MEYERS_NO_DESTROY(CLASS) \
                                                            \
public:                                                     \
                                                            \
  LINN_DISABLE_COPY_MOVE(CLASS)                             \
                                                            \
  static CLASS& getInstance()                               \
  {                                                         \
    static CLASS instance;                                  \
    return instance;                                        \
  }

#define LINN_SINGLETON_CREATE_BODY_MEYERS_ENABLE_DESTROY(CLASS) \
  LINN_SINGLETON_CREATE_BODY_UNIQUE_ENABLE_DESTROY(CLASS)

#define LINN_SINGLETON_CREATE_BODY_MEYERS_ENABLE_LOCK_DESTROY(CLASS) \
  LINN_SINGLETON_CREATE_BODY_UNIQUE_ENABLE_LOCK_DESTROY(CLASS)

#define LINN_SINGLETON_SELECT_DESTROY_UNIQUE(DESTROY_MODE, CLASS)             \
  LINN_IF(                                                                    \
      LINN_SINGLETON_PP_EQ(DESTROY_MODE, LINN_SINGLETON_DESTROY_NO_DESTROY))( \
      LINN_SINGLETON_CREATE_BODY_UNIQUE_NO_DESTROY(CLASS),                    \
      LINN_IF(LINN_SINGLETON_PP_EQ(DESTROY_MODE,                              \
                                   LINN_SINGLETON_DESTROY_ENABLE_DESTROY))(   \
          LINN_SINGLETON_CREATE_BODY_UNIQUE_ENABLE_DESTROY(CLASS),            \
          LINN_SINGLETON_CREATE_BODY_UNIQUE_ENABLE_LOCK_DESTROY(CLASS)))

#define LINN_SINGLETON_SELECT_DESTROY_SHARED(DESTROY_MODE, CLASS)             \
  LINN_IF(                                                                    \
      LINN_SINGLETON_PP_EQ(DESTROY_MODE, LINN_SINGLETON_DESTROY_NO_DESTROY))( \
      LINN_SINGLETON_CREATE_BODY_SHARED_NO_DESTROY(CLASS),                    \
      LINN_IF(LINN_SINGLETON_PP_EQ(DESTROY_MODE,                              \
                                   LINN_SINGLETON_DESTROY_ENABLE_DESTROY))(   \
          LINN_SINGLETON_CREATE_BODY_SHARED_ENABLE_DESTROY(CLASS),            \
          LINN_SINGLETON_CREATE_BODY_SHARED_ENABLE_LOCK_DESTROY(CLASS)))

#define LINN_SINGLETON_SELECT_DESTROY_MEYERS(DESTROY_MODE, CLASS)             \
  LINN_IF(                                                                    \
      LINN_SINGLETON_PP_EQ(DESTROY_MODE, LINN_SINGLETON_DESTROY_NO_DESTROY))( \
      LINN_SINGLETON_CREATE_BODY_MEYERS_NO_DESTROY(CLASS),                    \
      LINN_IF(LINN_SINGLETON_PP_EQ(DESTROY_MODE,                              \
                                   LINN_SINGLETON_DESTROY_ENABLE_DESTROY))(   \
          LINN_SINGLETON_CREATE_BODY_MEYERS_ENABLE_DESTROY(CLASS),            \
          LINN_SINGLETON_CREATE_BODY_MEYERS_ENABLE_LOCK_DESTROY(CLASS)))

#define LINN_SINGLETON_CREATE_FROM_TAG(MODE_TAG, DESTROY_MODE, CLASS)      \
  LINN_IF(LINN_SINGLETON_PP_EQ(MODE_TAG, LINN_SINGLETON_MODE_UNIQUE))(     \
      LINN_SINGLETON_SELECT_DESTROY_UNIQUE(DESTROY_MODE, CLASS),           \
      LINN_IF(LINN_SINGLETON_PP_EQ(MODE_TAG, LINN_SINGLETON_MODE_SHARED))( \
          LINN_SINGLETON_SELECT_DESTROY_SHARED(DESTROY_MODE, CLASS),       \
          LINN_SINGLETON_SELECT_DESTROY_MEYERS(DESTROY_MODE, CLASS)))

#define LINN_SINGLETON_CREATE_FROM_PAIR(MODE_PAIR, DESTROY_MODE)           \
  LINN_SINGLETON_CREATE_FROM_TAG(LINN_PAIR_FIRST(MODE_PAIR), DESTROY_MODE, \
                                 LINN_PAIR_SECOND(MODE_PAIR))

#pragma endregion LINN_SINGLETON_HELPER_CORE

#endif  // !LINN_SINGLETON_H_
