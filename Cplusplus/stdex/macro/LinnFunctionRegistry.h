#ifndef LINN_FUNCTION_REGISTRY_H_
#define LINN_FUNCTION_REGISTRY_H_

#include <functional>    // std::function, std::invoke (C++17)
#include <mutex>         // std::mutex, std::scoped_lock (C++17)
#include <string>        // std::string
#include <string_view>   // std::string_view (C++17)
#include <type_traits>   // std::is_void_v (C++17)
#include <unordered_map> // std::unordered_map
#include <utility>       // std::forward, std::move

#include "LinnProperty.h"

namespace linn::detail
{
  template<typename Signature>
  class FunctionRegistry;

  template<typename R, typename... Args>
  class FunctionRegistry<R(Args...)>
  {
  public:
    using Fn = std::function<R(Args...)>;

    bool Register(std::string key, Fn fn)
    {
      std::scoped_lock lock(m_Mutex);
      return m_Map.emplace(std::move(key), std::move(fn)).second;
    }

    bool Unregister(std::string_view key)
    {
      std::scoped_lock lock(m_Mutex);
      return m_Map.erase(std::string(key)) > 0;
    }

    Fn Find(std::string_view key) const
    {
      std::scoped_lock lock(m_Mutex);
      if (auto it = m_Map.find(std::string(key)); it != m_Map.end()) { return it->second; }
      return {};
    }

    template<typename... CallArgs>
    R Invoke(std::string_view key, CallArgs&&...args) const
    {
      auto fn = Find(key);
      LINN_CONTRACT_ASSERT_ALWAYS((bool) fn, "Unregistered function key");
      if constexpr (std::is_void_v<R>)
      {
        std::invoke(fn, std::forward<CallArgs>(args)...);
        return;
      }
      else
      {
        return std::invoke(fn, std::forward<CallArgs>(args)...);
      }
    }

  private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, Fn> m_Map;
  };
} // namespace linn::detail

#define LINN_DECLARE_FUNCTION_REGISTRY(NAME, SIGNATURE)                                                               \
                                                                                                                      \
public:                                                                                                               \
  using LINN_CAT(NAME, Function) = std::function<SIGNATURE>;                                                          \
  static bool LINN_CAT(Register, NAME)(std::string key, LINN_CAT(NAME, Function) fn)                                  \
  {                                                                                                                   \
    return LINN_CAT(NAME, Registry)().Register(std::move(key), std::move(fn));                                        \
  }                                                                                                                   \
  static bool LINN_CAT(Unregister, NAME)(std::string_view key) { return LINN_CAT(NAME, Registry)().Unregister(key); } \
  static LINN_CAT(NAME, Function) LINN_CAT(Find, NAME)(std::string_view key)                                          \
  {                                                                                                                   \
    return LINN_CAT(NAME, Registry)().Find(key);                                                                      \
  }                                                                                                                   \
  template<typename... Args>                                                                                          \
  static decltype(auto) LINN_CAT(Invoke, NAME)(std::string_view key, Args && ...args)                                 \
  {                                                                                                                   \
    return LINN_CAT(NAME, Registry)().Invoke(key, std::forward<Args>(args)...);                                       \
  }                                                                                                                   \
                                                                                                                      \
private:                                                                                                              \
  static ::linn::detail::FunctionRegistry<SIGNATURE>& LINN_CAT(NAME, Registry)()                                      \
  {                                                                                                                   \
    static ::linn::detail::FunctionRegistry<SIGNATURE> registry;                                                      \
    return registry;                                                                                                  \
  }

#define LINN_REGISTER_FUNCTION(CLASS, NAME, KEY, FN_EXPR)                  \
  namespace                                                                \
  {                                                                        \
    [[maybe_unused]] const bool LINN_CAT(_linn_fnreg_, __COUNTER__) = []() \
    {                                                                      \
      const bool ok = (CLASS::LINN_CAT(Register, NAME)(KEY, (FN_EXPR)));   \
      LINN_CONTRACT_ASSERT_ALWAYS(ok, "Duplicate function registration");  \
      return ok;                                                           \
    }();                                                                   \
  }

#endif // !LINN_FUNCTION_REGISTRY_H_
