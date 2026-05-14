#ifndef VLARRAY_HPP
#define VLARRAY_HPP

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

namespace stdex
{
// ===== Style macros to mimic MSVC internals =====
#ifndef _CONSTEXPR20
#  if __cplusplus >= 202002L
#    define _CONSTEXPR20 constexpr
#  else
#    define _CONSTEXPR20
#  endif
#endif

#ifndef _NODISCARD
#  if __cplusplus >= 201703L
#    define _NODISCARD [[nodiscard]]
#  else
#    define _NODISCARD
#  endif
#endif

// lightweight verify (like _STL_VERIFY)
#if defined(_DEBUG) || defined(DEBUG)
#  define VLARRAY_VERIFY(cond, msg)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
      if (!(cond))                                                                                                     \
        throw std::out_of_range(msg);                                                                                  \
    }                                                                                                                  \
    while (0)
#endif

template<class T>
class vlarray
{
public:
  // --- types ---
  using value_type      = T;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference       = T &;
  using const_reference = const T &;
  using pointer         = T *;
  using const_pointer   = const T *;

  using iterator               = pointer;
  using const_iterator         = const_pointer;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // --- construct/destroy ---
  // Default construction results in an unusable object (buffer == nullptr).
  _CONSTEXPR20 vlarray() noexcept
      : data_(nullptr)
      , size_(0)
      , capacity_(0)
  {
  }

  // Construction from a caller‑provided buffer.
  _CONSTEXPR20 vlarray(pointer buffer, size_type capacity) noexcept
      : data_(buffer)
      , size_(0)
      , capacity_(capacity)
  {
  }

  // Copy and move are deleted – memory is always owned by the caller.
  vlarray(const vlarray &)             = delete;
  vlarray &operator= (const vlarray &) = delete;
  vlarray(vlarray &&)                  = delete;
  vlarray &operator= (vlarray &&)      = delete;

  _CONSTEXPR20 ~vlarray() { clear(); }

  // --- element access ---
  _NODISCARD _CONSTEXPR20 reference operator[] (size_type pos) noexcept
  {
    VLARRAY_VERIFY(pos < size_, "vlarray subscript out of range");
    return data_[pos];
  }

  _NODISCARD _CONSTEXPR20 const_reference operator[] (size_type pos) const noexcept
  {
    VLARRAY_VERIFY(pos < size_, "vlarray subscript out of range");
    return data_[pos];
  }

  _NODISCARD _CONSTEXPR20 reference at(size_type pos)
  {
    if (pos >= size_)
    {
      throw std::out_of_range("vlarray::at out of range");
    }
    return data_[pos];
  }

  _NODISCARD _CONSTEXPR20 const_reference at(size_type pos) const
  {
    if (pos >= size_)
    {
      throw std::out_of_range("vlarray::at out of range");
    }
    return data_[pos];
  }

  _NODISCARD _CONSTEXPR20 reference front() noexcept
  {
    VLARRAY_VERIFY(size_ > 0, "front() on empty vlarray");
    return data_[0];
  }

  _NODISCARD _CONSTEXPR20 const_reference front() const noexcept
  {
    VLARRAY_VERIFY(size_ > 0, "front() on empty vlarray");
    return data_[0];
  }

  _NODISCARD _CONSTEXPR20 reference back() noexcept
  {
    VLARRAY_VERIFY(size_ > 0, "back() on empty vlarray");
    return data_[size_ - 1];
  }

  _NODISCARD _CONSTEXPR20 const_reference back() const noexcept
  {
    VLARRAY_VERIFY(size_ > 0, "back() on empty vlarray");
    return data_[size_ - 1];
  }

  _NODISCARD _CONSTEXPR20 pointer data() noexcept { return data_; }

  _NODISCARD _CONSTEXPR20 const_pointer data() const noexcept { return data_; }

  // --- iterators ---
  _NODISCARD _CONSTEXPR20 iterator begin() noexcept { return data_; }

  _NODISCARD _CONSTEXPR20 const_iterator begin() const noexcept { return data_; }

  _NODISCARD _CONSTEXPR20 const_iterator cbegin() const noexcept { return begin(); }

  _NODISCARD _CONSTEXPR20 iterator end() noexcept { return data_ + size_; }

  _NODISCARD _CONSTEXPR20 const_iterator end() const noexcept { return data_ + size_; }

  _NODISCARD _CONSTEXPR20 const_iterator cend() const noexcept { return end(); }

  _NODISCARD _CONSTEXPR20 reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

  _NODISCARD _CONSTEXPR20 const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

  _NODISCARD _CONSTEXPR20 const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  _NODISCARD _CONSTEXPR20 reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  _NODISCARD _CONSTEXPR20 const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

  _NODISCARD _CONSTEXPR20 const_reverse_iterator crend() const noexcept { return rend(); }

  // --- capacity ---
  _NODISCARD _CONSTEXPR20 size_type size() const noexcept { return size_; }

  _NODISCARD _CONSTEXPR20 size_type capacity() const noexcept { return capacity_; }

  _NODISCARD _CONSTEXPR20 bool empty() const noexcept { return size_ == 0; }

  static _CONSTEXPR20 size_type max_size() noexcept
  {
    // maximum number of objects that can theoretically fit into size_t
    return static_cast<size_type>(-1) / sizeof(T);
  }

  // --- modifiers ---
  _CONSTEXPR20 void push_back(const T &value) { emplace_back(value); }

  _CONSTEXPR20 void push_back(T &&value) { emplace_back(std::move(value)); }

  template<class... Args>
  _CONSTEXPR20 reference emplace_back(Args &&...args)
  {
    if (size_ >= capacity_)
    {
      throw std::length_error("vlarray capacity exceeded");
    }
    ::new (static_cast<void *>(data_ + size_)) T(std::forward<Args>(args)...);
    ++size_;
    return back();
  }

  _CONSTEXPR20 void pop_back() noexcept
  {
    VLARRAY_VERIFY(size_ > 0, "pop_back() on empty vlarray");
    data_[size_ - 1].~T();
    --size_;
  }

  _CONSTEXPR20 void clear() noexcept
  {
    std::destroy_n(data_, size_);
    size_ = 0;
  }

  _CONSTEXPR20 iterator insert(const_iterator pos, const T &value) { return emplace(pos, value); }

  _CONSTEXPR20 iterator insert(const_iterator pos, T &&value) { return emplace(pos, std::move(value)); }

  template<class... Args>
  _CONSTEXPR20 iterator emplace(const_iterator pos, Args &&...args)
  {
    if (size_ >= capacity_)
    {
      throw std::length_error("vlarray capacity exceeded");
    }
    const auto off = pos - data_;
    VLARRAY_VERIFY(off >= 0 && off <= static_cast<difference_type>(size_),
                        "vlarray emplace iterator out of range");

    if (off == static_cast<difference_type>(size_))
    {
      // at end
      emplace_back(std::forward<Args>(args)...);
    }
    else
    {
      // shift last element out if we are full? actually capacity check guarantees space
      // construct a temporary copy at the back
      ::new (static_cast<void *>(data_ + size_)) T(std::move(data_[size_ - 1]));
      ++size_;

      // move all elements from pos to end-1 one position to the right
      for (auto i = size_ - 1; i > static_cast<size_type>(off); --i)
      {
        data_[i] = std::move(data_[i - 1]);
      }
      // destroy the old element at pos and construct new
      data_[off].~T();
      ::new (static_cast<void *>(data_ + off)) T(std::forward<Args>(args)...);
    }
    return data_ + off;
  }

  _CONSTEXPR20 iterator erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>)
  {
    const auto off = pos - data_;
    VLARRAY_VERIFY(size_ > 0 && off >= 0 && off < static_cast<difference_type>(size_),
                        "vlarray erase iterator out of range");

    // move elements left
    for (auto i = off; i < static_cast<difference_type>(size_) - 1; ++i)
    {
      data_[i] = std::move(data_[i + 1]);
    }
    data_[size_ - 1].~T();
    --size_;
    return data_ + off;
  }

  _CONSTEXPR20 void resize(size_type count) { resize_impl(count); }

  _CONSTEXPR20 void resize(size_type count, const T &value) { resize_impl(count, value); }

private:
  // Helper resize
  template<class... Args>
  _CONSTEXPR20 void resize_impl(size_type count, const Args &...args)
  {
    if (count > capacity_)
    {
      throw std::length_error("vlarray resize exceeds capacity");
    }
    if (count > size_)
    {
      // append
      for (; size_ < count; ++size_)
      {
        ::new (static_cast<void *>(data_ + size_)) T(args...);
      }
    }
    else if (count < size_)
    {
      // shrink
      std::destroy_n(data_ + count, size_ - count);
      size_ = count;
    }
  }

  pointer   data_;
  size_type size_;
  size_type capacity_;
};

// --- comparison operators---
template<class T>
_NODISCARD _CONSTEXPR20 bool
operator== (const vlarray<T> &lhs, const vlarray<T> &rhs)
{
  if (lhs.size() != rhs.size())
    return false;
  return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

#if __cplusplus < 202002L
template<class T>
_NODISCARD _CONSTEXPR20 bool
operator!= (const vlarray<T> &lhs, const vlarray<T> &rhs)
{
  return !(lhs == rhs);
}

template<class T>
_NODISCARD _CONSTEXPR20 bool
operator< (const vlarray<T> &lhs, const vlarray<T> &rhs)
{
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<class T>
_NODISCARD _CONSTEXPR20 bool
operator> (const vlarray<T> &lhs, const vlarray<T> &rhs)
{
  return rhs < lhs;
}

template<class T>
_NODISCARD _CONSTEXPR20 bool
operator<= (const vlarray<T> &lhs, const vlarray<T> &rhs)
{
  return !(rhs < lhs);
}

template<class T>
_NODISCARD _CONSTEXPR20 bool
operator>= (const vlarray<T> &lhs, const vlarray<T> &rhs)
{
  return !(lhs < rhs);
}
#else
template<class T>
_NODISCARD constexpr auto
operator<=> (const vlarray<T> &lhs, const vlarray<T> &rhs)
{
  return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                                                std::compare_three_way {});
}
#endif

// macro to create a vlarray<T> with a given element count
// usage: auto sa = MAKE_VLARRAY(int, 100);
// The memory is valid until the enclosing function returns.
#define MAKE_VLARRAY(T, count) vlarray<T>(static_cast<T *>(alloca((count) * sizeof(T))), (count))

} // namespace stdex
#endif // VLARRAY_HPP
