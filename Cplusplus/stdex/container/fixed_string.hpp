#pragma once

#include <array>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <locale>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <utility>

namespace stdex {
template<typename CharT, ::std::size_t N>
class basic_fixed_string
{
  static_assert(::std::is_standard_layout_v<CharT> && ::std::is_trivial_v<CharT>,
                "CharT must be a POD type (standard layout and trivial)");
  static_assert(N > 0, "Size must include null terminator, so N >= 1");

public:

  using value_type          = CharT;
  using size_type           = ::std::size_t;
  using difference_type     = ::std::ptrdiff_t;
  using const_pointer       = const CharT*;
  using pointer             = CharT*;
  using const_reference     = const CharT&;
  using reference           = CharT&;
  using const_iterator      = const CharT*;
  using iterator            = CharT*;
  using original_type       = CharT [N];
  using const_original_type = const CharT [N];

  constexpr basic_fixed_string() noexcept = default;

  constexpr basic_fixed_string(const CharT (&str) [N]) noexcept
  {
    _assignFromArray(str);
  }

  template<::std::size_t M>
  constexpr basic_fixed_string(const CharT (&str) [M]) noexcept
  {
    _assignFromArray(str);
  }

  template<::std::size_t M>
  constexpr basic_fixed_string(const ::std::array<CharT, M>& arr) noexcept
  {
    _assignFromStdArray(arr);
  }

  constexpr basic_fixed_string(::std::basic_string_view<CharT> sv) noexcept
  {
    _assignFromIterators(sv.begin(), sv.end());
  }

  constexpr basic_fixed_string(std::initializer_list<CharT> init) noexcept
  {
    _assignFromIterators(init.begin(), init.end());
  }

  template<std::input_iterator It, std::sentinel_for<It> Sent>
  requires std::convertible_to<std::iter_value_t<It>, CharT>
  constexpr basic_fixed_string(It first, Sent last) noexcept
  {
    _assignFromIterators(first, last);
  }

  constexpr basic_fixed_string(const basic_fixed_string&) noexcept = default;
  constexpr basic_fixed_string(basic_fixed_string&&) noexcept      = default;

  constexpr basic_fixed_string&
  operator= (const basic_fixed_string&) noexcept = default;
  constexpr basic_fixed_string&
  operator= (basic_fixed_string&&) noexcept = default;

  constexpr basic_fixed_string&
  operator= (::std::basic_string_view<CharT> sv) noexcept
  {
    return assign(sv);
  }

  constexpr const_iterator begin() const noexcept { return m_data; }

  constexpr const_iterator end() const noexcept { return m_data + size(); }

  constexpr const_iterator cbegin() const noexcept { return m_data; }

  constexpr const_iterator cend() const noexcept { return m_data + size(); }

  constexpr iterator begin() noexcept { return m_data; }

  constexpr iterator end() noexcept { return m_data + size(); }

  constexpr size_type size() const noexcept { return N - 1; }

  constexpr size_type max_size() const noexcept { return N - 1; }

  constexpr bool empty() const noexcept { return size() == 0; }

  constexpr const_reference operator[] (size_type pos) const noexcept
  {
    return m_data [pos];
  }

  constexpr reference operator[] (size_type pos) noexcept { return m_data [pos]; }

  constexpr const CharT* data() const noexcept { return m_data; }

  constexpr CharT* data() noexcept { return m_data; }

  constexpr original_type& original() noexcept { return m_data; }

  constexpr const_original_type& original() const noexcept { return m_data; }

  constexpr const CharT* c_str() const noexcept { return m_data; }

  constexpr const_reference front() const noexcept { return m_data [0]; }

  constexpr reference front() noexcept { return m_data [0]; }

  constexpr const_reference back() const noexcept { return m_data [size() - 1]; }

  constexpr reference back() noexcept { return m_data [size() - 1]; }

  constexpr ::std::basic_string_view<CharT> view() const noexcept
  {
    return { m_data, size() };
  }

  constexpr operator ::std::basic_string_view<CharT> () const noexcept
  {
    return view();
  }

  constexpr bool operator== (const basic_fixed_string& rhs) const noexcept
  {
    return view() == rhs.view();
  }

  constexpr auto operator<=> (const basic_fixed_string& rhs) const noexcept
  {
    return view() <=> rhs.view();
  }

  constexpr void swap(basic_fixed_string& other) noexcept
  {
    for (::std::size_t i = 0; i < N; ++i) {
      CharT tmp        = m_data [i];
      m_data [i]       = other.m_data [i];
      other.m_data [i] = tmp;
    }
  }

  constexpr basic_fixed_string&
  assign(::std::basic_string_view<CharT> sv) noexcept
  {
    _assignFromIterators(sv.begin(), sv.end());
    return *this;
  }

  constexpr basic_fixed_string& assign(std::initializer_list<CharT> init) noexcept
  {
    _assignFromIterators(init.begin(), init.end());
    return *this;
  }

  template<std::input_iterator It, std::sentinel_for<It> Sent>
  requires std::convertible_to<std::iter_value_t<It>, CharT>
  constexpr basic_fixed_string& assign(It first, Sent last) noexcept
  {
    _assignFromIterators(first, last);
    return *this;
  }

  constexpr size_type find(CharT ch, size_type pos = 0) const noexcept
  {
    return view().find(ch, pos);
  }

  constexpr size_type find(::std::basic_string_view<CharT> sv,
                           size_type                       pos = 0) const noexcept
  {
    return view().find(sv, pos);
  }

  constexpr size_type rfind(CharT ch, size_type pos = npos) const noexcept
  {
    return view().rfind(ch, pos);
  }

  static constexpr size_type npos = size_type(-1);

private:

  CharT m_data [N] {};

  template<std::input_iterator It, std::sentinel_for<It> Sent>
  requires std::convertible_to<std::iter_value_t<It>, CharT>
  constexpr void _assignFromIterators(It first, Sent last) noexcept
  {
    constexpr size_type max = N - 1;
    size_type           i   = 0;
    for (; first != last && i < max; ++first, ++i) {
      m_data [i] = static_cast<CharT>(*first);
    }
    for (; i < max; ++i) {
      m_data [i] = CharT(0);
    }
    m_data [N - 1] = CharT(0);
  }

  template<::std::size_t M>
  constexpr void _assignFromArray(const CharT (&str) [M]) noexcept
  {
    constexpr size_type max = N - 1;
    size_type           len = 0;
    if constexpr (M > 0) {
      size_type logical = M;
      if (str [M - 1] == CharT(0)) { logical = M - 1; }
      len = logical < max ? logical : max;
      for (size_type i = 0; i < len; ++i) {
        m_data [i] = str [i];
      }
    }
    for (size_type i = len; i < max; ++i) {
      m_data [i] = CharT(0);
    }
    m_data [N - 1] = CharT(0);
  }

  template<::std::size_t M>
  constexpr void _assignFromStdArray(const ::std::array<CharT, M>& arr) noexcept
  {
    constexpr size_type max = N - 1;
    size_type           len = 0;
    if constexpr (M > 0) {
      size_type logical = M;
      if (arr [M - 1] == CharT(0)) { logical = M - 1; }
      len = logical < max ? logical : max;
      for (size_type i = 0; i < len; ++i) {
        m_data [i] = arr [i];
      }
    }
    for (size_type i = len; i < max; ++i) {
      m_data [i] = CharT(0);
    }
    m_data [N - 1] = CharT(0);
  }
};

template<typename CharT, ::std::size_t N>
constexpr bool operator== (const basic_fixed_string<CharT, N>& lhs,
                           const CharT*                        rhs) noexcept
{
  return lhs.view() == rhs;
}

template<typename CharT, ::std::size_t N>
constexpr bool operator== (const CharT*                        lhs,
                           const basic_fixed_string<CharT, N>& rhs) noexcept
{
  return lhs == rhs.view();
}

template<typename CharT, ::std::size_t N>
::std::basic_ostream<CharT>& operator<< (::std::basic_ostream<CharT>&        os,
                                         const basic_fixed_string<CharT, N>& s)
{
  return os.write(s.data(), static_cast<::std::streamsize>(s.size()));
}

// Deduction guides for C-style arrays (valid)
template<::std::size_t N>
basic_fixed_string(const char (&) [N]) -> basic_fixed_string<char, N>;

template<::std::size_t N>
basic_fixed_string(const wchar_t (&) [N]) -> basic_fixed_string<wchar_t, N>;

template<::std::size_t N>
basic_fixed_string(const char8_t (&) [N]) -> basic_fixed_string<char8_t, N>;

template<::std::size_t N>
basic_fixed_string(const char16_t (&) [N]) -> basic_fixed_string<char16_t, N>;

template<::std::size_t N>
basic_fixed_string(const char32_t (&) [N]) -> basic_fixed_string<char32_t, N>;

template<::std::size_t N>
using fixed_string = basic_fixed_string<char, N>;

template<::std::size_t N>
using fixed_wstring = basic_fixed_string<wchar_t, N>;

template<::std::size_t N>
using fixed_u8string = basic_fixed_string<char8_t, N>;

template<::std::size_t N>
using fixed_u16string = basic_fixed_string<char16_t, N>;

template<::std::size_t N>
using fixed_u32string = basic_fixed_string<char32_t, N>;

namespace fixed_string_literals {
template<stdex::fixed_string fs>
constexpr auto operator""_fs ()
{
  return fs;
}

template<stdex::fixed_wstring fws>
constexpr auto operator""_fws ()
{
  return fws;
}

template<stdex::fixed_u8string fu8s>
constexpr auto operator""_fu8s ()
{
  return fu8s;
}

template<stdex::fixed_u16string fu16s>
constexpr auto operator""_fu16s ()
{
  return fu16s;
}

template<stdex::fixed_u32string fu32s>
constexpr auto operator""_fu32s ()
{
  return fu32s;
}
}  // namespace fixed_string_literals

namespace detail {
template<typename ToChar, typename FromChar>
constexpr ToChar convert_char(FromChar ch) noexcept
{
  if constexpr (std::is_same_v<ToChar, FromChar>) {
    return ch;
  } else if constexpr (std::is_same_v<ToChar, wchar_t> &&
                       std::is_same_v<FromChar, char>) {
    return static_cast<wchar_t>(ch);
  } else if constexpr (std::is_same_v<ToChar, char> &&
                       std::is_same_v<FromChar, wchar_t>) {
    return static_cast<char>(ch <= 0xFF ? ch : '?');
  } else if constexpr (std::is_same_v<ToChar, char8_t> &&
                       std::is_same_v<FromChar, char>) {
    return static_cast<char8_t>(ch);
  } else if constexpr (std::is_same_v<ToChar, char> &&
                       std::is_same_v<FromChar, char8_t>) {
    return static_cast<char>(ch);
  } else if constexpr (std::is_same_v<ToChar, char16_t> &&
                       std::is_same_v<FromChar, char>) {
    return static_cast<char16_t>(ch);
  } else if constexpr (std::is_same_v<ToChar, char> &&
                       std::is_same_v<FromChar, char16_t>) {
    return static_cast<char>(ch <= 0xFF ? ch : '?');
  } else if constexpr (std::is_same_v<ToChar, char32_t> &&
                       std::is_same_v<FromChar, char>) {
    return static_cast<char32_t>(ch);
  } else if constexpr (std::is_same_v<ToChar, char> &&
                       std::is_same_v<FromChar, char32_t>) {
    return static_cast<char>(ch <= 0xFF ? ch : '?');
  } else {
    return static_cast<ToChar>(ch);
  }
}
}  // namespace detail

template<typename ToChar, std::size_t ToN, typename FromChar, std::size_t FromN>
constexpr basic_fixed_string<ToChar, ToN>
fixed_string_cast(const basic_fixed_string<FromChar, FromN>& from) noexcept
{
  basic_fixed_string<ToChar, ToN> to;
  constexpr std::size_t           max_copy =
      (ToN - 1) < (FromN - 1) ? (ToN - 1) : (FromN - 1);
  auto* to_data = to.data();
  for (std::size_t i = 0; i < max_copy; ++i) {
    to_data [i] = detail::convert_char<ToChar>(from [i]);
  }

  for (std::size_t i = max_copy; i < ToN - 1; ++i) {
    to_data [i] = ToChar(0);
  }
  to_data [ToN - 1] = ToChar(0);

  return to;
}
}  // namespace stdex

namespace std {
template<typename CharT, size_t N>
struct hash<stdex::basic_fixed_string<CharT, N>>
{
  size_t operator() (const stdex::basic_fixed_string<CharT, N>& s) const noexcept
  {
    return hash<basic_string_view<CharT>> {}(s.view());
  }
};
}  // namespace std
