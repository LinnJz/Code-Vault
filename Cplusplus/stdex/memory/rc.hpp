#pragma once

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#define RC_CXX20 1
#else
#define RC_CXX20 0
#endif
#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define RC_CXX17 1
#else
#define RC_CXX17 0
#endif

#include <cstddef>
#if RC_CXX20
#include <compare>
#endif
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <new>
#include <ostream>
#include <type_traits>
#include <typeinfo>
#include <utility>

// This library intentionally extends namespace std because its public API is
// std::rc.  User programs must otherwise not add declarations to namespace std.
namespace std {

template <class T>
class rc;

template <class T>
class weak_rc;

template <class T>
class enable_rc_from_this;

namespace rc_detail {

struct adopt_control_t {
    explicit adopt_control_t() = default;
};

struct overwrite_t {
    explicit overwrite_t() = default;
};

template <class Exception>
[[noreturn]] inline void throw_exception() {
#if defined(_MSC_VER)
#if defined(_CPPUNWIND)
    throw Exception();
#else
    terminate();
#endif
#elif defined(__cpp_exceptions) || defined(__EXCEPTIONS)
    throw Exception();
#else
    terminate();
#endif
}

[[noreturn]] inline void throw_bad_weak_ptr() {
    throw_exception<bad_weak_ptr>();
}

[[noreturn]] inline void throw_bad_array_new_length() {
    throw_exception<bad_array_new_length>();
}

class control_block {
public:
    control_block() noexcept = default;
    control_block(const control_block&) = delete;
    control_block& operator=(const control_block&) = delete;

    void add_shared() noexcept {
        ++shared_;
    }

    [[nodiscard]] bool try_add_shared() noexcept {
        if (shared_ == 0) {
            return false;
        }
        ++shared_;
        return true;
    }

    void release_shared() noexcept {
        if (--shared_ == 0) {
            destroy_object();
            release_weak(); // release the implicit weak owner
        }
    }

    void add_weak() noexcept {
        ++weak_;
    }

    void release_weak() noexcept {
        if (--weak_ == 0) {
            delete_self();
        }
    }

    [[nodiscard]] long use_count() const noexcept {
        return static_cast<long>(shared_);
    }

    [[nodiscard]] virtual void* get_deleter(const type_info&) noexcept {
        return nullptr;
    }

protected:
    virtual ~control_block() = default;

private:
    virtual void destroy_object() noexcept = 0;
    virtual void delete_self() noexcept = 0;

    size_t shared_ = 1;
    size_t weak_ = 1;
};

template <class Resource, class Deleter>
class resource_guard {
public:
    resource_guard(Resource& resource, Deleter& deleter) noexcept
        : resource_(addressof(resource)), deleter_(addressof(deleter)) {}

    resource_guard(const resource_guard&) = delete;
    resource_guard& operator=(const resource_guard&) = delete;

    ~resource_guard() noexcept {
        if (deleter_ != nullptr) {
            (*deleter_)(*resource_);
        }
    }

    void release() noexcept { deleter_ = nullptr; }

private:
    Resource* resource_;
    Deleter* deleter_;
};

template <class Alloc>
class allocation_guard {
public:
    using traits = allocator_traits<Alloc>;
    using pointer = typename traits::pointer;

    allocation_guard(Alloc& alloc, pointer memory, size_t count) noexcept
        : alloc_(addressof(alloc)), memory_(memory), count_(count) {}

    allocation_guard(const allocation_guard&) = delete;
    allocation_guard& operator=(const allocation_guard&) = delete;

    ~allocation_guard() noexcept {
        if (alloc_ != nullptr) {
            traits::deallocate(*alloc_, memory_, count_);
        }
    }

    void release() noexcept { alloc_ = nullptr; }

private:
    Alloc* alloc_;
    pointer memory_;
    size_t count_;
};

template <class Resource, class Deleter, class Alloc>
class pointer_control_block final : public control_block {
public:
    pointer_control_block(Resource resource, Deleter deleter, const Alloc& alloc)
        noexcept(is_nothrow_copy_constructible<Resource>::value
                 && is_nothrow_move_constructible<Deleter>::value
                 && is_nothrow_copy_constructible<Alloc>::value)
        : resource_(resource), deleter_(std::move(deleter)), alloc_(alloc) {}

    [[nodiscard]] void* get_deleter(const type_info& info) noexcept override {
        return info == typeid(Deleter) ? addressof(deleter_) : nullptr;
    }

private:
    using block_alloc = typename allocator_traits<Alloc>::template rebind_alloc<pointer_control_block>;
    using block_traits = allocator_traits<block_alloc>;

    void destroy_object() noexcept override {
        deleter_(resource_);
    }

    void delete_self() noexcept override {
        block_alloc alloc(alloc_);
        using pointer = typename block_traits::pointer;
        pointer self = pointer_traits<pointer>::pointer_to(*this);
        this->~pointer_control_block();
        block_traits::deallocate(alloc, self, 1);
    }

    Resource resource_;
#if RC_CXX20
#if defined(_MSC_VER)
    [[msvc::no_unique_address]]
#else
    [[no_unique_address]]
#endif
#endif
    Deleter deleter_;
#if RC_CXX20
#if defined(_MSC_VER)
    [[msvc::no_unique_address]]
#else
    [[no_unique_address]]
#endif
#endif
    Alloc alloc_;
};

template <class Resource, class Deleter, class Alloc>
control_block* make_pointer_control(Resource resource, Deleter deleter, const Alloc& alloc) {
    using block = pointer_control_block<Resource, Deleter, Alloc>;
    using block_alloc = typename allocator_traits<Alloc>::template rebind_alloc<block>;
    using traits = allocator_traits<block_alloc>;
    resource_guard<Resource, Deleter> owner(resource, deleter);
    block_alloc rebound(alloc);
    auto memory = traits::allocate(rebound, 1);
    allocation_guard<block_alloc> allocation(rebound, memory, 1);
    block* raw = addressof(*memory);
    ::new (static_cast<void*>(raw)) block(resource, std::move(deleter), alloc);
    allocation.release();
    owner.release();
    return raw;
}

template <class T, class Alloc>
class inplace_control_block final : public control_block {
public:
    using value_type = typename remove_cv<T>::type;
    using object_alloc = typename allocator_traits<Alloc>::template rebind_alloc<value_type>;

    template <class... Args>
    explicit inplace_control_block(const Alloc& alloc, Args&&... args)
        noexcept(is_nothrow_copy_constructible<Alloc>::value
                 && is_nothrow_constructible<object_alloc, const Alloc&>::value
                 && noexcept(allocator_traits<object_alloc>::construct(
                     declval<object_alloc&>(), declval<value_type*>(), declval<Args>()...)))
        : alloc_(alloc) {
        object_alloc object_allocator(alloc_);
        allocator_traits<object_alloc>::construct(object_allocator, get(), std::forward<Args>(args)...);
        constructed_ = true;
    }

    inplace_control_block(const Alloc& alloc, overwrite_t)
        noexcept(is_nothrow_copy_constructible<Alloc>::value && is_nothrow_default_constructible<value_type>::value)
        : alloc_(alloc) {
        ::new (static_cast<void*>(get())) value_type;
        constructed_ = true;
    }

    [[nodiscard]] value_type* get() noexcept {
        return reinterpret_cast<value_type*>(storage_.bytes);
    }

private:
    using block_alloc = typename allocator_traits<Alloc>::template rebind_alloc<inplace_control_block>;
    using block_traits = allocator_traits<block_alloc>;

    void destroy_object() noexcept override {
        if (constructed_) {
            object_alloc object_allocator(alloc_);
            allocator_traits<object_alloc>::destroy(object_allocator, get());
            constructed_ = false;
        }
    }

    void delete_self() noexcept override {
        block_alloc alloc(alloc_);
        using pointer = typename block_traits::pointer;
        pointer self = pointer_traits<pointer>::pointer_to(*this);
        this->~inplace_control_block();
        block_traits::deallocate(alloc, self, 1);
    }

#if RC_CXX20
#if defined(_MSC_VER)
    [[msvc::no_unique_address]]
#else
    [[no_unique_address]]
#endif
#endif
    Alloc alloc_;
    union storage_union {
        value_type value;
        unsigned char bytes[sizeof(value_type)];

        storage_union() noexcept {}
        ~storage_union() noexcept {}
    } storage_;
    bool constructed_ = false;
};

template <class T, class Alloc, class... Args>
inplace_control_block<T, Alloc>* make_inplace_control(const Alloc& alloc, Args&&... args) {
    using block = inplace_control_block<T, Alloc>;
    using block_alloc = typename allocator_traits<Alloc>::template rebind_alloc<block>;
    using traits = allocator_traits<block_alloc>;
    block_alloc rebound(alloc);
    auto memory = traits::allocate(rebound, 1);
    allocation_guard<block_alloc> allocation(rebound, memory, 1);
    block* raw = addressof(*memory);
    ::new (static_cast<void*>(raw)) block(alloc, std::forward<Args>(args)...);
    allocation.release();
    return raw;
}

enum class array_init { value, overwrite, fill };

template <size_t Alignment>
struct aligned_storage_unit {
    alignas(Alignment) unsigned char bytes[Alignment];
};

template <class T, class Alloc>
struct array_operations {
    using element_type = typename remove_extent<T>::type;
    using storage_element = typename remove_cv<element_type>::type;
    using scalar_type = typename remove_cv<typename remove_all_extents<T>::type>::type;
    using scalar_alloc = typename allocator_traits<Alloc>::template rebind_alloc<scalar_type>;
    using scalar_traits = allocator_traits<scalar_alloc>;

    static constexpr size_t scalars_per_element() noexcept {
        return sizeof(storage_element) / sizeof(scalar_type);
    }

    static size_t checked_scalar_count(size_t top_count) {
        const size_t per_element = scalars_per_element();
        if (top_count > numeric_limits<size_t>::max() / per_element) {
            throw_bad_array_new_length();
        }
        return top_count * per_element;
    }

    static scalar_type* scalar_data(storage_element* data) noexcept {
        return reinterpret_cast<scalar_type*>(data);
    }

    static void destroy_one(scalar_alloc& alloc, scalar_type* value, array_init init) noexcept {
        if (init == array_init::overwrite) {
            value->~scalar_type();
        } else {
            scalar_traits::destroy(alloc, value);
        }
    }

    class construction_guard {
    public:
        construction_guard(scalar_alloc& alloc, scalar_type* data, array_init init) noexcept
            : alloc_(addressof(alloc)), data_(data), init_(init) {}

        construction_guard(const construction_guard&) = delete;
        construction_guard& operator=(const construction_guard&) = delete;

        ~construction_guard() noexcept {
            if (alloc_ != nullptr) {
                while (constructed_ != 0) {
                    destroy_one(*alloc_, data_ + --constructed_, init_);
                }
            }
        }

        void constructed_one() noexcept { ++constructed_; }
        void release() noexcept { alloc_ = nullptr; }

    private:
        scalar_alloc* alloc_;
        scalar_type* data_;
        array_init init_;
        size_t constructed_ = 0;
    };

    static void construct(
        Alloc& alloc, storage_element* data, size_t top_count, array_init init, const element_type* fill) {
        const size_t scalar_count = checked_scalar_count(top_count);
        scalar_alloc values(alloc);
        scalar_type* raw = scalar_data(data);
        const scalar_type* source = fill == nullptr ? nullptr : reinterpret_cast<const scalar_type*>(fill);
        construction_guard guard(values, raw, init);
        for (size_t index = 0; index < scalar_count; ++index) {
            if (init == array_init::overwrite) {
                ::new (static_cast<void*>(raw + index)) scalar_type;
            } else if (init == array_init::fill) {
                scalar_traits::construct(values, raw + index, source[index % scalars_per_element()]);
            } else {
                scalar_traits::construct(values, raw + index);
            }
            guard.constructed_one();
        }
        guard.release();
    }

    static void destroy(Alloc& alloc, storage_element* data, size_t top_count, array_init init) noexcept {
        scalar_alloc values(alloc);
        scalar_type* raw = scalar_data(data);
        size_t count = top_count * scalars_per_element();
        while (count != 0) {
            destroy_one(values, raw + --count, init);
        }
    }
};

template <class T, class Alloc>
class bounded_array_control_block final : public control_block {
public:
    static_assert(is_array<T>::value && extent<T>::value != 0, "bounded array required");
    using operations = array_operations<T, Alloc>;
    using element_type = typename operations::element_type;
    using storage_element = typename operations::storage_element;

    bounded_array_control_block(const Alloc& alloc, array_init init, const element_type* fill)
        : alloc_(alloc), init_(init) {
        operations::construct(alloc_, storage_.values, extent<T>::value, init_, fill);
        alive_ = true;
    }

    [[nodiscard]] element_type* get() noexcept {
        return reinterpret_cast<element_type*>(storage_.values);
    }

private:
    using block_alloc = typename allocator_traits<Alloc>::template rebind_alloc<bounded_array_control_block>;
    using block_traits = allocator_traits<block_alloc>;

    void destroy_object() noexcept override {
        if (alive_) {
            operations::destroy(alloc_, storage_.values, extent<T>::value, init_);
            alive_ = false;
        }
    }

    void delete_self() noexcept override {
        block_alloc alloc(alloc_);
        using pointer = typename block_traits::pointer;
        pointer self = pointer_traits<pointer>::pointer_to(*this);
        this->~bounded_array_control_block();
        block_traits::deallocate(alloc, self, 1);
    }

#if RC_CXX20
#if defined(_MSC_VER)
    [[msvc::no_unique_address]]
#else
    [[no_unique_address]]
#endif
#endif
    Alloc alloc_;
    array_init init_;
    union storage_union {
        storage_element values[extent<T>::value];
        storage_union() noexcept {}
        ~storage_union() noexcept {}
    } storage_;
    bool alive_ = false;
};

template <class T, class Alloc>
class unbounded_array_control_block final : public control_block {
public:
    static_assert(is_array<T>::value && extent<T>::value == 0, "unbounded array required");
    using operations = array_operations<T, Alloc>;
    using element_type = typename operations::element_type;
    using storage_element = typename operations::storage_element;
    static constexpr size_t storage_alignment =
        alignof(control_block) > alignof(Alloc)
        ? (alignof(control_block) > alignof(storage_element) ? alignof(control_block) : alignof(storage_element))
        : (alignof(Alloc) > alignof(storage_element) ? alignof(Alloc) : alignof(storage_element));
    using storage_unit = aligned_storage_unit<storage_alignment>;
    using storage_alloc = typename allocator_traits<Alloc>::template rebind_alloc<storage_unit>;
    using storage_traits = allocator_traits<storage_alloc>;

    unbounded_array_control_block(const Alloc& alloc, size_t count, size_t allocation_units,
        array_init init, const element_type* fill)
        : alloc_(alloc), count_(count), allocation_units_(allocation_units), init_(init) {
        operations::construct(alloc_, storage_.values, count_, init_, fill);
        alive_ = true;
    }

    [[nodiscard]] element_type* get() noexcept {
        return reinterpret_cast<element_type*>(storage_.values);
    }

    static size_t checked_allocation_units(size_t count) {
        const size_t extra_count = count > 1 ? count - 1 : 0;
        constexpr size_t alignment = storage_alignment;
        constexpr size_t maximum = numeric_limits<size_t>::max();
        if (extra_count > (maximum - sizeof(unbounded_array_control_block) - (alignment - 1))
                / sizeof(storage_element)) {
            throw_bad_array_new_length();
        }
        const size_t bytes = sizeof(unbounded_array_control_block) + extra_count * sizeof(storage_element);
        return (bytes + alignment - 1) / sizeof(storage_unit);
    }

private:
    void destroy_object() noexcept override {
        if (alive_) {
            operations::destroy(alloc_, storage_.values, count_, init_);
            alive_ = false;
        }
    }

    void delete_self() noexcept override {
        storage_alloc alloc(alloc_);
        using pointer = typename storage_traits::pointer;
        auto* storage = reinterpret_cast<storage_unit*>(this);
        pointer self = pointer_traits<pointer>::pointer_to(*storage);
        const size_t units = allocation_units_;
        this->~unbounded_array_control_block();
        storage_traits::deallocate(alloc, self, units);
    }

#if RC_CXX20
#if defined(_MSC_VER)
    [[msvc::no_unique_address]]
#else
    [[no_unique_address]]
#endif
#endif
    Alloc alloc_;
    size_t count_ = 0;
    size_t allocation_units_ = 0;
    array_init init_;
    bool alive_ = false;
    union storage_union {
        storage_element values[1];
        storage_union() noexcept {}
        ~storage_union() noexcept {}
    } storage_;
};

template <class T, class Alloc, typename enable_if<is_array<T>::value && extent<T>::value != 0, int>::type = 0>
bounded_array_control_block<T, Alloc>* make_array_control(
    const Alloc& alloc, size_t, array_init init, const typename remove_extent<T>::type* fill = nullptr) {
    using block = bounded_array_control_block<T, Alloc>;
    using block_alloc = typename allocator_traits<Alloc>::template rebind_alloc<block>;
    using traits = allocator_traits<block_alloc>;
    block_alloc rebound(alloc);
    auto memory = traits::allocate(rebound, 1);
    allocation_guard<block_alloc> allocation(rebound, memory, 1);
    block* raw = addressof(*memory);
    ::new (static_cast<void*>(raw)) block(alloc, init, fill);
    allocation.release();
    return raw;
}

template <class T, class Alloc, typename enable_if<is_array<T>::value && extent<T>::value == 0, int>::type = 0>
unbounded_array_control_block<T, Alloc>* make_array_control(
    const Alloc& alloc, size_t count, array_init init, const typename remove_extent<T>::type* fill = nullptr) {
    using block = unbounded_array_control_block<T, Alloc>;
    using storage_alloc = typename block::storage_alloc;
    using traits = allocator_traits<storage_alloc>;
    static_assert(alignof(block) == block::storage_alignment, "array control block alignment mismatch");
    const size_t units = block::checked_allocation_units(count);
    storage_alloc rebound(alloc);
    auto memory = traits::allocate(rebound, units);
    allocation_guard<storage_alloc> allocation(rebound, memory, units);
    block* raw = reinterpret_cast<block*>(addressof(*memory));
    ::new (static_cast<void*>(raw)) block(alloc, count, units, init, fill);
    allocation.release();
    return raw;
}

template <class T>
struct is_unbounded_array : false_type {};

template <class T>
struct is_unbounded_array<T[]> : true_type {};

template <class T>
struct is_bounded_array : integral_constant<bool, is_array<T>::value && extent<T>::value != 0> {};

template <class From, class To>
struct bounded_array_to_unbounded : false_type {};

template <class U, size_t Count>
struct bounded_array_to_unbounded<U[Count], U[]> : true_type {};

template <class U, size_t Count>
struct bounded_array_to_unbounded<U[Count], const U[]> : true_type {};

template <class U, size_t Count>
struct bounded_array_to_unbounded<U[Count], volatile U[]> : true_type {};

template <class U, size_t Count>
struct bounded_array_to_unbounded<U[Count], const volatile U[]> : true_type {};

template <class From, class To>
struct pointer_compatible
    : integral_constant<bool,
          is_convertible<From*, To*>::value || bounded_array_to_unbounded<From, To>::value> {};

template <class Y, class T, class = void>
struct raw_pointer_compatible
    : integral_constant<bool, !is_array<T>::value && is_convertible<Y*, T*>::value> {};

template <class Y, class U, size_t Count>
struct raw_pointer_compatible<Y, U[Count],
    typename enable_if<is_convertible<Y (*)[Count], U (*)[Count]>::value>::type> : true_type {};

template <class Y, class U>
struct raw_pointer_compatible<Y, U[],
    typename enable_if<is_convertible<Y (*)[], U (*)[]>::value>::type> : true_type {};

template <class Y, class = void>
struct is_complete : false_type {};

template <class Y>
struct is_complete<Y, void_t<decltype(sizeof(Y))>> : true_type {};

template <class Y, class = void>
struct is_scalar_deletable : false_type {};

template <class Y>
struct is_scalar_deletable<Y, void_t<decltype(delete declval<Y*>())>>
    : integral_constant<bool, !is_void<Y>::value && is_complete<Y>::value> {};

template <class Y, class = void>
struct is_array_deletable : false_type {};

template <class Y>
struct is_array_deletable<Y, void_t<decltype(delete[] declval<Y*>())>>
    : is_complete<Y> {};

template <class Y, class T>
struct default_pointer_compatible
    : integral_constant<bool,
          raw_pointer_compatible<Y, T>::value
              && conditional<is_array<T>::value, is_array_deletable<Y>, is_scalar_deletable<Y>>::type::value> {};

template <class Deleter, class Resource, class = void>
struct well_formed_deleter : false_type {};

template <class Deleter, class Resource>
struct well_formed_deleter<Deleter, Resource,
    void_t<decltype(declval<Deleter&>()(declval<Resource&>()))>> : true_type {};

template <class Y, class Deleter, class T>
struct pointer_deleter_compatible
    : integral_constant<bool,
          raw_pointer_compatible<Y, T>::value
              && is_move_constructible<Deleter>::value
              && well_formed_deleter<Deleter, Y*>::value> {};

template <class Deleter>
struct nullptr_deleter_compatible
    : integral_constant<bool,
          is_move_constructible<Deleter>::value
              && well_formed_deleter<Deleter, nullptr_t>::value> {};

template <class Managed, class Y>
struct default_rc_deleter {
    using type = typename conditional<is_array<Managed>::value, default_delete<Y[]>, default_delete<Y>>::type;
};

struct access;

} // namespace rc_detail

template <class T>
class rc {
public:
    using element_type = typename remove_extent<T>::type;
    using weak_type = weak_rc<T>;
    using pointer = element_type*;

    constexpr rc() noexcept = default;
    constexpr rc(nullptr_t) noexcept {}

    template <class Y, typename enable_if<rc_detail::default_pointer_compatible<Y, T>::value, int>::type = 0>
    explicit rc(Y* value)
        : rc(value, typename rc_detail::default_rc_deleter<T, Y>::type{}) {}

    template <class Y, class Deleter,
        typename enable_if<rc_detail::pointer_deleter_compatible<Y, Deleter, T>::value, int>::type = 0>
    rc(Y* value, Deleter deleter)
        : rc(value, std::move(deleter), allocator<unsigned char>{}) {}

    template <class Y, class Deleter, class Alloc,
        typename enable_if<rc_detail::pointer_deleter_compatible<Y, Deleter, T>::value, int>::type = 0>
    rc(Y* value, Deleter deleter, Alloc alloc)
        : ptr_(value), control_(rc_detail::make_pointer_control(value, std::move(deleter), alloc)) {
        enable_owner(value);
    }

    template <class Deleter,
        typename enable_if<rc_detail::nullptr_deleter_compatible<Deleter>::value, int>::type = 0>
    rc(nullptr_t value, Deleter deleter)
        : rc(value, std::move(deleter), allocator<unsigned char>{}) {}

    template <class Deleter, class Alloc,
        typename enable_if<rc_detail::nullptr_deleter_compatible<Deleter>::value, int>::type = 0>
    rc(nullptr_t value, Deleter deleter, Alloc alloc)
        : ptr_(nullptr), control_(rc_detail::make_pointer_control(value, std::move(deleter), alloc)) {}

    template <class Y>
    rc(const rc<Y>& owner, pointer value) noexcept : ptr_(value), control_(owner.control_) {
        add_ref();
    }

    template <class Y>
    rc(rc<Y>&& owner, pointer value) noexcept : ptr_(value), control_(owner.control_) {
        owner.ptr_ = nullptr;
        owner.control_ = nullptr;
    }

    rc(const rc& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        add_ref();
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    rc(const rc<Y>& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        add_ref();
    }

    rc(rc&& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        other.ptr_ = nullptr;
        other.control_ = nullptr;
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    rc(rc<Y>&& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        other.ptr_ = nullptr;
        other.control_ = nullptr;
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    explicit rc(const weak_rc<Y>& other);

    template <class Y, class Deleter,
        typename enable_if<rc_detail::pointer_compatible<Y, T>::value
                && is_convertible<typename unique_ptr<Y, Deleter>::pointer, pointer>::value,
            int>::type = 0>
    rc(unique_ptr<Y, Deleter>&& other) {
        using resource = typename unique_ptr<Y, Deleter>::pointer;
        using original_element = typename unique_ptr<Y, Deleter>::element_type;
        using stored_deleter = typename conditional<is_reference<Deleter>::value,
            reference_wrapper<typename remove_reference<Deleter>::type>, Deleter>::type;
        resource value = other.get();
        if (value) {
            original_element* raw = value;
            stored_deleter deleter = make_stored_deleter<stored_deleter>(
                other.get_deleter(), integral_constant<bool, is_reference<Deleter>::value>{});
            control_ = rc_detail::make_pointer_control(value, std::move(deleter), allocator<unsigned char>{});
            ptr_ = value;
            other.release();
            enable_owner(raw);
        }
    }

    ~rc() noexcept {
        release();
    }

    rc& operator=(const rc& other) noexcept {
        rc(other).swap(*this);
        return *this;
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    rc& operator=(const rc<Y>& other) noexcept {
        rc(other).swap(*this);
        return *this;
    }

    rc& operator=(rc&& other) noexcept {
        rc(std::move(other)).swap(*this);
        return *this;
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    rc& operator=(rc<Y>&& other) noexcept {
        rc(std::move(other)).swap(*this);
        return *this;
    }

    template <class Y, class Deleter,
        typename enable_if<rc_detail::pointer_compatible<Y, T>::value
                && is_convertible<typename unique_ptr<Y, Deleter>::pointer, pointer>::value,
            int>::type = 0>
    rc& operator=(unique_ptr<Y, Deleter>&& other) {
        rc(std::move(other)).swap(*this);
        return *this;
    }

    void reset() noexcept {
        rc().swap(*this);
    }

    template <class Y, typename enable_if<rc_detail::default_pointer_compatible<Y, T>::value, int>::type = 0>
    void reset(Y* value) {
        rc(value).swap(*this);
    }

    template <class Y, class Deleter,
        typename enable_if<rc_detail::pointer_deleter_compatible<Y, Deleter, T>::value, int>::type = 0>
    void reset(Y* value, Deleter deleter) {
        rc(value, std::move(deleter)).swap(*this);
    }

    template <class Y, class Deleter, class Alloc,
        typename enable_if<rc_detail::pointer_deleter_compatible<Y, Deleter, T>::value, int>::type = 0>
    void reset(Y* value, Deleter deleter, Alloc alloc) {
        rc(value, std::move(deleter), alloc).swap(*this);
    }

    void swap(rc& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(control_, other.control_);
    }

    [[nodiscard]] constexpr pointer get() const noexcept { return ptr_; }

    template <class U = T>
    [[nodiscard]] typename enable_if<!is_array<U>::value && !is_void<element_type>::value,
        typename add_lvalue_reference<element_type>::type>::type
    operator*() const noexcept {
        return *ptr_;
    }

    template <class U = T>
    [[nodiscard]] constexpr typename enable_if<!is_array<U>::value, pointer>::type
    operator->() const noexcept { return ptr_; }

    template <class U = T>
    [[nodiscard]] typename enable_if<is_array<U>::value, typename add_lvalue_reference<element_type>::type>::type
    operator[](ptrdiff_t index) const noexcept {
        return ptr_[index];
    }

    [[nodiscard]] long use_count() const noexcept { return control_ == nullptr ? 0 : control_->use_count(); }
#if !RC_CXX20
    [[nodiscard]] bool unique() const noexcept { return use_count() == 1; }
#endif
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }

    template <class Y>
    [[nodiscard]] bool owner_before(const rc<Y>& other) const noexcept {
        return less<rc_detail::control_block*>{}(control_, other.control_);
    }

    template <class Y>
    [[nodiscard]] bool owner_before(const weak_rc<Y>& other) const noexcept {
        return less<rc_detail::control_block*>{}(control_, other.control_);
    }

private:
    template <class>
    friend class rc;
    template <class>
    friend class weak_rc;
    template <class>
    friend class enable_rc_from_this;
    friend struct rc_detail::access;
    template <class Deleter, class Y>
    friend Deleter* get_deleter(const rc<Y>&) noexcept;

    rc(pointer value, rc_detail::control_block* control, rc_detail::adopt_control_t) noexcept
        : ptr_(value), control_(control) {}

    template <class Stored, class Deleter>
    static Stored make_stored_deleter(Deleter& deleter, false_type)
        noexcept(is_nothrow_constructible<Stored, Deleter&&>::value) {
        return std::move(deleter);
    }

    template <class Stored, class Deleter>
    static Stored make_stored_deleter(Deleter& deleter, true_type) noexcept {
        return ref(deleter);
    }

    void add_ref() noexcept {
        if (control_ != nullptr) {
            control_->add_shared();
        }
    }

    void release() noexcept {
        if (control_ != nullptr) {
            control_->release_shared();
        }
    }

    template <class Y>
    void enable_owner(Y* value) noexcept;

    template <class Y, class U = typename Y::rc_esft_type>
    typename enable_if<is_convertible<Y*, enable_rc_from_this<U>*>::value && is_convertible<Y*, U*>::value>::type
    enable_owner_impl(Y* value, int) noexcept;

    template <class Y>
    void enable_owner_impl(Y*, ...) noexcept {}

    pointer ptr_ = nullptr;
    rc_detail::control_block* control_ = nullptr;
};

namespace rc_detail {

struct access {
    template <class T>
    static rc<T> adopt(typename rc<T>::pointer value, control_block* control) noexcept {
        return rc<T>(value, control, adopt_control_t{});
    }

    template <class T, class Y>
    static void enable(rc<T>& owner, Y* value) noexcept {
        owner.enable_owner(value);
    }
};

} // namespace rc_detail

template <class T>
class weak_rc {
public:
    using element_type = typename remove_extent<T>::type;

    constexpr weak_rc() noexcept = default;

    weak_rc(const weak_rc& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        add_ref();
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    weak_rc(const weak_rc<Y>& other) noexcept : control_(other.control_) {
        if (control_ != nullptr) {
            control_->add_weak();
            if (control_->try_add_shared()) {
                ptr_ = other.ptr_;
                control_->release_shared();
            }
        }
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    weak_rc(const rc<Y>& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        add_ref();
    }

    weak_rc(weak_rc&& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        other.ptr_ = nullptr;
        other.control_ = nullptr;
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    weak_rc(weak_rc<Y>&& other) noexcept : control_(other.control_) {
        if (control_ != nullptr && control_->try_add_shared()) {
            ptr_ = other.ptr_;
            control_->release_shared();
        }
        other.ptr_ = nullptr;
        other.control_ = nullptr;
    }

    ~weak_rc() noexcept {
        release();
    }

    weak_rc& operator=(const weak_rc& other) noexcept {
        weak_rc(other).swap(*this);
        return *this;
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    weak_rc& operator=(const weak_rc<Y>& other) noexcept {
        weak_rc(other).swap(*this);
        return *this;
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    weak_rc& operator=(const rc<Y>& other) noexcept {
        weak_rc(other).swap(*this);
        return *this;
    }

    weak_rc& operator=(weak_rc&& other) noexcept {
        weak_rc(std::move(other)).swap(*this);
        return *this;
    }

    template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type = 0>
    weak_rc& operator=(weak_rc<Y>&& other) noexcept {
        weak_rc(std::move(other)).swap(*this);
        return *this;
    }

    void reset() noexcept { weak_rc().swap(*this); }

    void swap(weak_rc& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(control_, other.control_);
    }

    [[nodiscard]] long use_count() const noexcept { return control_ == nullptr ? 0 : control_->use_count(); }
    [[nodiscard]] bool expired() const noexcept { return use_count() == 0; }

    [[nodiscard]] rc<T> lock() const noexcept {
        if (control_ != nullptr && control_->try_add_shared()) {
            return rc<T>(ptr_, control_, rc_detail::adopt_control_t{});
        }
        return {};
    }

    template <class Y>
    [[nodiscard]] bool owner_before(const rc<Y>& other) const noexcept {
        return less<rc_detail::control_block*>{}(control_, other.control_);
    }

    template <class Y>
    [[nodiscard]] bool owner_before(const weak_rc<Y>& other) const noexcept {
        return less<rc_detail::control_block*>{}(control_, other.control_);
    }

private:
    template <class>
    friend class rc;
    template <class>
    friend class weak_rc;
    template <class>
    friend class enable_rc_from_this;

    void add_ref() noexcept {
        if (control_ != nullptr) {
            control_->add_weak();
        }
    }

    void release() noexcept {
        if (control_ != nullptr) {
            control_->release_weak();
        }
    }

    element_type* ptr_ = nullptr;
    rc_detail::control_block* control_ = nullptr;
};

template <class T>
class enable_rc_from_this {
public:
    using rc_esft_type = T;

    [[nodiscard]] rc<T> rc_from_this() { return rc<T>(weak_this_); }
    [[nodiscard]] rc<const T> rc_from_this() const { return rc<const T>(weak_this_); }
    [[nodiscard]] weak_rc<T> weak_from_this() noexcept { return weak_this_; }
    [[nodiscard]] weak_rc<const T> weak_from_this() const noexcept { return weak_this_; }

    // Familiar spellings for code mechanically ported from shared_ptr.
    [[nodiscard]] rc<T> shared_from_this() { return rc_from_this(); }
    [[nodiscard]] rc<const T> shared_from_this() const { return rc_from_this(); }

protected:
    constexpr enable_rc_from_this() noexcept = default;
    enable_rc_from_this(const enable_rc_from_this&) noexcept {}
    enable_rc_from_this& operator=(const enable_rc_from_this&) noexcept { return *this; }
    ~enable_rc_from_this() = default;

private:
    template <class>
    friend class rc;
    mutable weak_rc<T> weak_this_;
};

template <class T>
template <class Y, typename enable_if<rc_detail::pointer_compatible<Y, T>::value, int>::type>
rc<T>::rc(const weak_rc<Y>& other) : control_(other.control_) {
    if (control_ == nullptr || !control_->try_add_shared()) {
        control_ = nullptr;
        rc_detail::throw_bad_weak_ptr();
    }
    ptr_ = other.ptr_;
}

template <class T>
template <class Y>
void rc<T>::enable_owner(Y* value) noexcept {
    if constexpr (!is_array<T>::value) {
        if (value != nullptr) {
            enable_owner_impl(value, 0);
        }
    }
}

template <class T>
template <class Y, class U>
typename enable_if<is_convertible<Y*, enable_rc_from_this<U>*>::value && is_convertible<Y*, U*>::value>::type
rc<T>::enable_owner_impl(Y* value, int) noexcept {
    auto* base = static_cast<enable_rc_from_this<U>*>(value);
    if (base->weak_this_.expired()) {
        base->weak_this_ = rc<U>(*this, static_cast<U*>(value));
    }
}

template <class T, class Alloc, class... Args,
    typename enable_if<!is_array<T>::value && !is_void<T>::value, int>::type = 0>
[[nodiscard]] rc<T> allocate_rc(const Alloc& alloc, Args&&... args) {
    auto* control = rc_detail::make_inplace_control<T>(alloc, std::forward<Args>(args)...);
    auto result = rc_detail::access::adopt<T>(control->get(), control);
    rc_detail::access::enable(result, control->get());
    return result;
}

template <class T, class... Args, typename enable_if<!is_array<T>::value && !is_void<T>::value, int>::type = 0>
[[nodiscard]] rc<T> make_rc(Args&&... args) {
    return allocate_rc<T>(allocator<typename remove_cv<T>::type>{}, std::forward<Args>(args)...);
}

template <class T, class Alloc, typename enable_if<rc_detail::is_unbounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> allocate_rc(const Alloc& alloc, size_t count) {
    auto* control = rc_detail::make_array_control<T>(alloc, count, rc_detail::array_init::value);
    return rc_detail::access::adopt<T>(control->get(), control);
}

template <class T, class Alloc, typename enable_if<rc_detail::is_unbounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> allocate_rc(
    const Alloc& alloc, size_t count, const typename remove_extent<T>::type& value) {
    auto* control = rc_detail::make_array_control<T>(alloc, count, rc_detail::array_init::fill, addressof(value));
    return rc_detail::access::adopt<T>(control->get(), control);
}

template <class T, typename enable_if<rc_detail::is_unbounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> make_rc(size_t count) {
    return allocate_rc<T>(allocator<typename remove_all_extents<T>::type>{}, count);
}

template <class T, typename enable_if<rc_detail::is_unbounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> make_rc(size_t count, const typename remove_extent<T>::type& value) {
    return allocate_rc<T>(allocator<typename remove_all_extents<T>::type>{}, count, value);
}

template <class T, class Alloc, typename enable_if<rc_detail::is_bounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> allocate_rc(const Alloc& alloc) {
    auto* control = rc_detail::make_array_control<T>(alloc, extent<T>::value, rc_detail::array_init::value);
    return rc_detail::access::adopt<T>(control->get(), control);
}

template <class T, class Alloc, typename enable_if<rc_detail::is_bounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> allocate_rc(const Alloc& alloc, const typename remove_extent<T>::type& value) {
    auto* control = rc_detail::make_array_control<T>(alloc, extent<T>::value, rc_detail::array_init::fill, addressof(value));
    return rc_detail::access::adopt<T>(control->get(), control);
}

template <class T, typename enable_if<rc_detail::is_bounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> make_rc() {
    return allocate_rc<T>(allocator<typename remove_all_extents<T>::type>{});
}

template <class T, typename enable_if<rc_detail::is_bounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> make_rc(const typename remove_extent<T>::type& value) {
    return allocate_rc<T>(allocator<typename remove_all_extents<T>::type>{}, value);
}

template <class T, class Alloc, typename enable_if<!is_array<T>::value && !is_void<T>::value, int>::type = 0>
[[nodiscard]] rc<T> allocate_rc_for_overwrite(const Alloc& alloc) {
    auto* control = rc_detail::make_inplace_control<T>(alloc, rc_detail::overwrite_t{});
    auto result = rc_detail::access::adopt<T>(control->get(), control);
    rc_detail::access::enable(result, control->get());
    return result;
}

template <class T, typename enable_if<!is_array<T>::value && !is_void<T>::value, int>::type = 0>
[[nodiscard]] rc<T> make_rc_for_overwrite() {
    return allocate_rc_for_overwrite<T>(allocator<typename remove_cv<T>::type>{});
}

template <class T, class Alloc, typename enable_if<rc_detail::is_unbounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> allocate_rc_for_overwrite(const Alloc& alloc, size_t count) {
    auto* control = rc_detail::make_array_control<T>(alloc, count, rc_detail::array_init::overwrite);
    return rc_detail::access::adopt<T>(control->get(), control);
}

template <class T, typename enable_if<rc_detail::is_unbounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> make_rc_for_overwrite(size_t count) {
    return allocate_rc_for_overwrite<T>(allocator<typename remove_all_extents<T>::type>{}, count);
}

template <class T, class Alloc, typename enable_if<rc_detail::is_bounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> allocate_rc_for_overwrite(const Alloc& alloc) {
    auto* control = rc_detail::make_array_control<T>(alloc, extent<T>::value, rc_detail::array_init::overwrite);
    return rc_detail::access::adopt<T>(control->get(), control);
}

template <class T, typename enable_if<rc_detail::is_bounded_array<T>::value, int>::type = 0>
[[nodiscard]] rc<T> make_rc_for_overwrite() {
    return allocate_rc_for_overwrite<T>(allocator<typename remove_all_extents<T>::type>{});
}

template <class T, class U>
bool operator==(const rc<T>& left, const rc<U>& right) noexcept { return left.get() == right.get(); }
template <class T, class U>
bool operator!=(const rc<T>& left, const rc<U>& right) noexcept { return !(left == right); }
template <class T>
bool operator==(const rc<T>& value, nullptr_t) noexcept { return value.get() == nullptr; }
template <class T>
bool operator==(nullptr_t, const rc<T>& value) noexcept { return value.get() == nullptr; }
template <class T>
bool operator!=(const rc<T>& value, nullptr_t) noexcept { return value.get() != nullptr; }
template <class T>
bool operator!=(nullptr_t, const rc<T>& value) noexcept { return value.get() != nullptr; }

#if RC_CXX20
template <class T, class U>
strong_ordering operator<=>(const rc<T>& left, const rc<U>& right) noexcept {
    return left.get() <=> right.get();
}
template <class T>
strong_ordering operator<=>(const rc<T>& value, nullptr_t) noexcept {
    return value.get() <=> static_cast<typename rc<T>::pointer>(nullptr);
}
#else
template <class T, class U>
bool operator<(const rc<T>& left, const rc<U>& right) noexcept {
    return less<const volatile void*>{}(left.get(), right.get());
}
template <class T, class U>
bool operator>(const rc<T>& left, const rc<U>& right) noexcept { return right < left; }
template <class T, class U>
bool operator<=(const rc<T>& left, const rc<U>& right) noexcept { return !(right < left); }
template <class T, class U>
bool operator>=(const rc<T>& left, const rc<U>& right) noexcept { return !(left < right); }
template <class T>
bool operator<(const rc<T>& value, nullptr_t) noexcept {
    return less<typename rc<T>::pointer>{}(value.get(), nullptr);
}
template <class T>
bool operator<(nullptr_t, const rc<T>& value) noexcept {
    return less<typename rc<T>::pointer>{}(nullptr, value.get());
}
template <class T>
bool operator>(const rc<T>& value, nullptr_t) noexcept { return nullptr < value; }
template <class T>
bool operator>(nullptr_t, const rc<T>& value) noexcept { return value < nullptr; }
template <class T>
bool operator<=(const rc<T>& value, nullptr_t) noexcept { return !(nullptr < value); }
template <class T>
bool operator<=(nullptr_t, const rc<T>& value) noexcept { return !(value < nullptr); }
template <class T>
bool operator>=(const rc<T>& value, nullptr_t) noexcept { return !(value < nullptr); }
template <class T>
bool operator>=(nullptr_t, const rc<T>& value) noexcept { return !(nullptr < value); }
#endif

template <class Character, class Traits, class T>
basic_ostream<Character, Traits>& operator<<(basic_ostream<Character, Traits>& output, const rc<T>& value) {
    return output << value.get();
}

template <class T>
void swap(rc<T>& left, rc<T>& right) noexcept { left.swap(right); }
template <class T>
void swap(weak_rc<T>& left, weak_rc<T>& right) noexcept { left.swap(right); }

template <class T, class U>
[[nodiscard]] rc<T> static_pointer_cast(const rc<U>& owner) noexcept {
    return rc<T>(owner, static_cast<typename rc<T>::pointer>(owner.get()));
}
template <class T, class U>
[[nodiscard]] rc<T> static_pointer_cast(rc<U>&& owner) noexcept {
    auto value = static_cast<typename rc<T>::pointer>(owner.get());
    return rc<T>(std::move(owner), value);
}
template <class T, class U>
[[nodiscard]] rc<T> dynamic_pointer_cast(const rc<U>& owner) noexcept {
    auto value = dynamic_cast<typename rc<T>::pointer>(owner.get());
    return value == nullptr ? rc<T>{} : rc<T>(owner, value);
}
template <class T, class U>
[[nodiscard]] rc<T> dynamic_pointer_cast(rc<U>&& owner) noexcept {
    auto value = dynamic_cast<typename rc<T>::pointer>(owner.get());
    return value == nullptr ? rc<T>{} : rc<T>(std::move(owner), value);
}
template <class T, class U>
[[nodiscard]] rc<T> const_pointer_cast(const rc<U>& owner) noexcept {
    return rc<T>(owner, const_cast<typename rc<T>::pointer>(owner.get()));
}
template <class T, class U>
[[nodiscard]] rc<T> const_pointer_cast(rc<U>&& owner) noexcept {
    auto value = const_cast<typename rc<T>::pointer>(owner.get());
    return rc<T>(std::move(owner), value);
}
template <class T, class U>
[[nodiscard]] rc<T> reinterpret_pointer_cast(const rc<U>& owner) noexcept {
    return rc<T>(owner, reinterpret_cast<typename rc<T>::pointer>(owner.get()));
}
template <class T, class U>
[[nodiscard]] rc<T> reinterpret_pointer_cast(rc<U>&& owner) noexcept {
    auto value = reinterpret_cast<typename rc<T>::pointer>(owner.get());
    return rc<T>(std::move(owner), value);
}

template <class Deleter, class T>
[[nodiscard]] Deleter* get_deleter(const rc<T>& value) noexcept {
    return value.control_ == nullptr
        ? nullptr
        : static_cast<Deleter*>(value.control_->get_deleter(typeid(Deleter)));
}

template <class T>
struct owner_less<rc<T>> {
    using result_type = bool;
    using first_argument_type = rc<T>;
    using second_argument_type = rc<T>;
    using is_transparent = void;

    template <class U, class V>
    bool operator()(const rc<U>& left, const rc<V>& right) const noexcept { return left.owner_before(right); }
    template <class U, class V>
    bool operator()(const rc<U>& left, const weak_rc<V>& right) const noexcept { return left.owner_before(right); }
    template <class U, class V>
    bool operator()(const weak_rc<U>& left, const rc<V>& right) const noexcept { return left.owner_before(right); }
};

template <class T>
struct owner_less<weak_rc<T>> {
    using result_type = bool;
    using first_argument_type = weak_rc<T>;
    using second_argument_type = weak_rc<T>;
    using is_transparent = void;

    template <class U, class V>
    bool operator()(const weak_rc<U>& left, const weak_rc<V>& right) const noexcept { return left.owner_before(right); }
    template <class U, class V>
    bool operator()(const weak_rc<U>& left, const rc<V>& right) const noexcept { return left.owner_before(right); }
    template <class U, class V>
    bool operator()(const rc<U>& left, const weak_rc<V>& right) const noexcept { return left.owner_before(right); }
};

template <class T>
struct hash<rc<T>> {
    [[nodiscard]] size_t operator()(const rc<T>& value) const noexcept {
        return hash<typename rc<T>::pointer>{}(value.get());
    }
};

template <class T>
using Rc = rc<T>;

template <class T>
using WeakRc = weak_rc<T>;

template <class T>
using enable_shared_from_rc = enable_rc_from_this<T>;

#if RC_CXX17
template <class T>
rc(weak_rc<T>) -> rc<T>;

template <class T, class Deleter>
rc(unique_ptr<T, Deleter>) -> rc<T>;

template <class T>
weak_rc(rc<T>) -> weak_rc<T>;
#endif

} // namespace std

#undef RC_CXX17
#undef RC_CXX20
