#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <execution>
#include <type_traits>
#include <limits>
#include <set>
#include <iostream>
#include <vector>
#include <functional>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_sort.h>

#include "../stdex/profiling/ticktock.hpp"
#include "../stdex/profiling/generator.hpp"

// i5-10200H CPU @ 2.40GHz
// VS2022 MSVC C++23
// 
// 非并行: 数量级是 1 << 13 时开始比标准库的std::sort快
// 并行: 数量级是 1 << 23 时开始比标准库的std::sort(std::execution::par)快
// 
// 什么时候并行比非并行快: std::sort和quick_sort都是 数量级是在 1 << 16 与 1 << 17 之间
// std::sort 接近于 1 << 16， quick_sort接近于1 << 17
// 
// 1 << 16数量级以内， quick_sort非并行略快于std::sort(std::execution::par)，差距在3%~20%，1 << 17就明显慢于并行
// 
// tbb::parallel_sort 数量级在1 << 17时和std::sort(std::execution::par)差不多，低于就比sort慢，高于就比sort快
//
// 1 << 10 希尔排序、quick_sort看数据的随机分布情况最优，低于1 << 10 quick_sort最优
// 1 << 11 std::sort最优
struct parallel_tag {};
struct sequential_tag {};
template <typename _Pa, typename _Ty, typename _Compare>
auto quick_sort([[maybe_unused]] _Pa&& para, _Ty* data, std::size_t size, _Compare comp) -> void
{
    if (size <= 32) {
        std::sort(data, data + size, std::less<_Ty>{}); // Insert_Sort
        return;
    }

    // 更好的随机化基准选择
    std::size_t mid = std::hash<std::size_t>{}(size);
    mid ^= std::hash<void*>{}(static_cast<void*>(data));
    mid %= size;
    std::swap(data[0], data[mid]);

    _Ty pivot = data[0];
    std::size_t left = 1, right = size - 1;

    // 正确的分区算法
    while (left <= right) {
        while (left <= right && comp(data[left], pivot)) ++left;
        while (left <= right && comp(pivot, data[right])) --right;
        if (left <= right) {
            std::swap(data[left], data[right]);
            ++left;
            --right;
        }
    }

    // 将基准放到正确位置
    std::swap(data[0], data[right]);

    // 递归排序
    if constexpr (std::is_same_v<std::decay_t<_Pa>, parallel_tag>) {
        tbb::parallel_invoke(
            [&] {
                quick_sort(para, data, right, comp);
            },
            [&] {
                quick_sort(para, data + right + 1, size - right - 1, comp);
            });
    }
    else {
        quick_sort(para, data, right, comp);
        quick_sort(para, data + right + 1, size - right - 1, comp);
    }
}

template <typename _Pa, typename _Ty>
auto quick_sort([[maybe_unused]] _Pa&& para, _Ty* data, std::size_t size) -> void
{
    quick_sort(std::forward<_Pa>(para), data, size, std::less<_Ty>{});
}



namespace detail {
    template<typename RandomIt, typename Compare>
    void sift_down(RandomIt first, typename std::iterator_traits<RandomIt>::difference_type start,
        typename std::iterator_traits<RandomIt>::difference_type end, Compare comp) {
        using difference_type = typename std::iterator_traits<RandomIt>::difference_type;
        difference_type root = start;

        while (root * 2 + 1 <= end) {  // 当根节点有至少一个子节点时
            difference_type child = root * 2 + 1;  // 左子节点
            difference_type swap_idx = root;

            // 比较左子节点
            if (comp(*(first + swap_idx), *(first + child))) {
                swap_idx = child;
            }

            // 检查右子节点
            if (child + 1 <= end && comp(*(first + swap_idx), *(first + (child + 1)))) {
                swap_idx = child + 1;
            }

            // 如果最大元素已经在根位置，结束
            if (swap_idx == root) {
                return;
            }

            // 交换并继续向下调整
            std::iter_swap(first + root, first + swap_idx);
            root = swap_idx;
        }
    }
}

// 堆排序主函数
template<typename RandomIt, typename Compare = std::less<>>
void heap_sort(RandomIt first, RandomIt last, Compare comp = Compare{}) {
    static_assert(std::is_base_of_v<std::random_access_iterator_tag,
        typename std::iterator_traits<RandomIt>::iterator_category>,
        "heap_sort requires random access iterators");

    if (first == last || std::next(first) == last) {
        return;  // 空或单元素范围
    }

    using difference_type = typename std::iterator_traits<RandomIt>::difference_type;
    const difference_type n = std::distance(first, last);

    // 构建最大堆（最大堆用于升序排序）
    // 从最后一个非叶子节点开始
    for (difference_type start = n / 2 - 1; start >= 0; --start) {
        detail::sift_down(first, start, n - 1, comp);
    }

    // 逐个提取元素
    for (difference_type end = n - 1; end > 0; --end) {
        // 将当前最大值（堆顶）移到末尾
        std::iter_swap(first, first + end);
        // 调整剩余堆
        detail::sift_down(first, 0, end - 1, comp);
    }
}

template <typename _Ty>
auto shell_sort(_Ty* data, std::size_t size) -> void
{
    std::size_t h = 1;
    while (h < size / 3) h = 3 * h + 1;
    while (h >= 1) {
        for (std::size_t i = h; i < size; ++i) {
            _Ty temp = data[i];
            std::size_t j;
            for (j = i; j >= h && data[j - h] > temp; j -= h) {
                data[j] = data[j - h];
            }
            data[j] = temp;
        }
        h /= 3;
    }
}

template <typename _Ty>
auto insertion_sort(_Ty* data, std::size_t size) -> void
{
    for (std::size_t i = 1; i < size; ++i) {
        _Ty temp = data[i];
        std::size_t j = i - 1;
        for (j = i; j > 0 && data[j - 1] > temp; --j) {
            data[j] = data[j - 1];
        }
        data[j] = temp;
    }
}

// 简化的插入排序
template<typename Iterator, typename Compare>
void insertion_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    /*
    1 5 4 2 3
        1 -
        1 5
        1 - 5
        1 4 5 -
        1 4 - 5
        1 - 4 5
        1 2 4 5 -
        1 2 4 - 5
        1 2 - 4 5
        1 2 3 4 5 
    */
    for (auto current = std::next(first); current != last; ++current) {
        auto key = std::move(*current);  // 保存当前元素
        auto hole = current;             // 空位位置

        // 向前比较并移动元素
        auto prev = hole;
        while (prev != first && comp(key, *std::prev(prev))) {
            --prev;
            *hole = std::move(*prev);  // 大元素后移
            hole = prev;
        }

        *hole = std::move(key);  // 插入到正确位置
    }
}

static auto benchmark_sort() -> void
{
    auto arr = stdex::generate_random<int32_t>(1 << 10, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
    auto arr2 = arr;
    auto arr3 = arr;
    auto arr4 = arr;
    auto arr5 = arr;
    TICK(quick_sort)
    quick_sort(sequential_tag{}, arr.data(), arr.size(), std::less<int32_t>{});
    TOCK(quick_sort)

    TICK(quick_sort_std_parallel)
    std::sort(std::execution::par, arr2.begin(), arr2.end());
    TOCK(quick_sort_std_parallel)

    TICK(shell_sort)
    shell_sort(arr3.data(), arr3.size());
    TOCK(shell_sort)

    //TICK(heap_sort)
    //heap_sort(arr3.begin(), arr3.end());
    ///*
    //* std::make_heap(arr3.begin(), arr3.end());
    //* std::sort_heap(arr3.begin(), arr3.end());
    //*/
    //TOCK(heap_sort)

    // tbb有缓存，如果上述开启parallel_invoke, 并行sort那么这里速度将会非常快
    TICK(tbb_parallel_sort)
    tbb::parallel_sort(arr4.begin(), arr4.end());
    TOCK(tbb_parallel_sort)

    TICK(multiset_sort)
    std::multiset<int32_t> set(arr5.begin(), arr5.end());
    std::move(set.begin(), set.end(), arr5.data());
    TOCK(multiset_sort)
}

// 版本1: 不展开
template<typename BiIter, typename Compare>
void insert_sort_no_unroll(BiIter start, BiIter end, Compare comp)
{
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本2: 外部展开4，内部展开16
template<typename BiIter, typename Compare>
void insert_sort_4_16(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 4
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

#pragma GCC unroll 16
        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本3: 外部展开4，内部展开8
template<typename BiIter, typename Compare>
void insert_sort_4_8(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 4
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

#pragma GCC unroll 8
        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本4: 外部展开4，内部展开4
template<typename BiIter, typename Compare>
void insert_sort_4_4(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 4
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

#pragma GCC unroll 4
        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本5: 外部展开8，内部展开16
template<typename BiIter, typename Compare>
void insert_sort_8_16(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 8
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

#pragma GCC unroll 16
        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本6: 外部展开8，内部展开8
template<typename BiIter, typename Compare>
void insert_sort_8_8(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 8
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

#pragma GCC unroll 8
        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本7: 外部展开8，内部展开4
template<typename BiIter, typename Compare>
void insert_sort_8_4(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 8
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

#pragma GCC unroll 4
        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本8: 外部展开16，内部展开16
template<typename BiIter, typename Compare>
void insert_sort_16_16(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 16
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

#pragma GCC unroll 16
        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本9: 外部展开16，内部展开8
template<typename BiIter, typename Compare>
void insert_sort_16_8(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 16
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

#pragma GCC unroll 8
        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本10: 外部展开16，内部展开4
template<typename BiIter, typename Compare>
void insert_sort_16_4(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 16
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

#pragma GCC unroll 4
        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本11: 外部展开16，内部展开0
template<typename BiIter, typename Compare>
void insert_sort_16_0(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 16
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本10: 外部展开8，内部展开0
template<typename BiIter, typename Compare>
void insert_sort_8_0(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 8
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;
        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 版本10: 外部展开4，内部展开0
template<typename BiIter, typename Compare>
void insert_sort_4_0(BiIter start, BiIter end, Compare comp)
{
#pragma GCC unroll 4
    for (auto current = std::next(start); current != end; ++current)
    {
        auto value = std::move(*current);
        auto hole = current;
        auto prev = hole;

        while (prev != start && comp(value, *std::prev(prev)))
        {
            --prev;
            *hole = std::move(*prev);
            hole = prev;
        }
        *hole = std::move(value);
    }
}

// 快速排序模板，接受插入排序函数作为参数
template<typename InsertSortFunc, typename RanIter, typename Compare>
void hoare_quick_sort_with_insert(RanIter start, RanIter end, Compare comp, InsertSortFunc insert_sort_func)
{
    const auto size = std::distance(start, end);
    if (size <= 32) {
        insert_sort_func(start, end, comp);
        return;
    }

    // 三数取中法选择枢轴
    auto left = start, right = std::prev(end);
    auto mid = start + ((right - left) >> 1);

    if (comp(*mid, *left)) std::iter_swap(mid, left);
    if (comp(*right, *left)) std::iter_swap(right, left);
    if (comp(*right, *mid)) std::iter_swap(right, mid);

    auto pivot_pos = std::prev(right);
	std::iter_swap(mid, pivot_pos);
	auto& pivot_val = *pivot_pos;

    auto i = std::prev(left), j = pivot_pos;
    while (true) {
        do { ++i; } while (comp(*i, pivot_val));
        do { --j; } while (j >= left && comp(pivot_val, *j));
        if (i >= j) break;
        std::iter_swap(i, j);
    }

    std::iter_swap(i, pivot_pos);

    if (std::distance(left, i) <= std::distance(i, right)) {
        hoare_quick_sort_with_insert(left, i, comp, insert_sort_func);
        hoare_quick_sort_with_insert(std::next(i), end, comp, insert_sort_func);
    }
    else {
        hoare_quick_sort_with_insert(std::next(i), end, comp, insert_sort_func);
        hoare_quick_sort_with_insert(left, i, comp, insert_sort_func);
    }
}

// 测试函数
template<typename InsertSortFunc>
void test_performance(const std::string& version_name, InsertSortFunc insert_sort_func,
    const std::vector<int>& data, const std::string& data_type)
{
    auto data_copy = data;
    auto start = std::chrono::high_resolution_clock::now();

    hoare_quick_sort_with_insert(data_copy.begin(), data_copy.end(), std::less<int>{}, insert_sort_func);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 验证排序正确性
    bool is_sorted = std::is_sorted(data_copy.begin(), data_copy.end());
    if (!is_sorted) {
        std::cout << "ERROR: " << version_name << " failed to sort " << data_type << " data!" << std::endl;
        return;
    }

    std::cout << version_name << " - " << data_type << ": " << duration.count() << " μs" << std::endl;
}

int main()
{
    const int N = 16384; // 2^14

    // 生成测试数据
    std::vector<int> ascending(N);
    std::iota(ascending.begin(), ascending.end(), 1);

    std::vector<int> descending(N);
    std::iota(descending.rbegin(), descending.rend(), 1);

    std::vector<int> mostly_ascending(N);
    std::iota(mostly_ascending.begin(), mostly_ascending.end(), 1);
    // 添加一些随机扰动
    std::random_device rd;
    std::mt19937 gen(rd());
    for (int i = 0; i < N / 10; ++i) {
        int idx1 = gen() % N;
        int idx2 = gen() % N;
        std::swap(mostly_ascending[idx1], mostly_ascending[idx2]);
    }

    std::vector<int> mostly_descending(N);
    std::iota(mostly_descending.rbegin(), mostly_descending.rend(), 1);
    for (int i = 0; i < N / 10; ++i) {
        int idx1 = gen() % N;
        int idx2 = gen() % N;
        std::swap(mostly_descending[idx1], mostly_descending[idx2]);
    }

    std::vector<int> random_data(N);
    std::iota(random_data.begin(), random_data.end(), 1);
    std::shuffle(random_data.begin(), random_data.end(), gen);

    // 测试所有版本
    std::vector<std::pair<std::string, std::function<void(std::vector<int>::iterator, std::vector<int>::iterator, std::less<int>)>>> versions = {
        {"no_unroll", insert_sort_no_unroll<std::vector<int>::iterator, std::less<int>>},
        {"4_16", insert_sort_4_16<std::vector<int>::iterator, std::less<int>>},
        {"4_8", insert_sort_4_8<std::vector<int>::iterator, std::less<int>>},
        {"4_4", insert_sort_4_4<std::vector<int>::iterator, std::less<int>>},
        {"8_16", insert_sort_8_16<std::vector<int>::iterator, std::less<int>>},
        {"8_8", insert_sort_8_8<std::vector<int>::iterator, std::less<int>>},
        {"8_4", insert_sort_8_4<std::vector<int>::iterator, std::less<int>>},
        {"16_16", insert_sort_16_16<std::vector<int>::iterator, std::less<int>>},
        {"16_8", insert_sort_16_8<std::vector<int>::iterator, std::less<int>>},
        {"16_4", insert_sort_16_4<std::vector<int>::iterator, std::less<int>>},
        {"16_0", insert_sort_16_0<std::vector<int>::iterator, std::less<int>>},
        {"8_0", insert_sort_8_0<std::vector<int>::iterator, std::less<int>>},
        {"4_0", insert_sort_4_0<std::vector<int>::iterator, std::less<int>>},
    };

    std::vector<std::pair<std::string, std::vector<int>>> test_cases = {
        {"ascending", ascending},
        {"descending", descending},
        {"mostly_ascending", mostly_ascending},
        {"mostly_descending", mostly_descending},
        {"random", random_data}
    };

    // 运行测试
    for (const auto& test_case : test_cases) {
        std::cout << "\n=== Testing " << test_case.first << " data ===" << std::endl;
        for (const auto& version : versions) {
            test_performance(version.first, version.second, test_case.second, test_case.first);
        }
    }

    return 0;
}