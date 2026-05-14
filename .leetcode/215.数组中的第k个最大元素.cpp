/*
为什么是 O (n) 复杂度？
1. 数学推导
快速选择的时间复杂度期望为 O (n)，推导过程：

    第一次分区：处理 n 个元素 → O (n)
    第二次分区：处理 n/2 个元素 → O (n/2)
    第三次分区：处理 n/4 个元素 → O (n/4)
    ...
    最后一次分区：处理 1 个元素 → O (1)

总时间：
T(n)=n+2n​+4n​+...+1=2n−1=O(n)
2. 核心原理

    标准快排：每次分区后递归处理左右两个子数组，时间复杂度为 T(n)=2T(n/2)+O(n)=O(nlogn)
    快速选择：每次分区后只递归处理一个子数组（包含目标元素的那个），时间复杂度为 T(n)=T(n/2)+O(n)=O(n)
*/
class Solution {
public:
    template<std::bidirectional_iterator BiIter, std::strict_weak_order<std::iter_value_t<BiIter>, std::iter_value_t<BiIter>> Compare>
    void insert_sort(BiIter start, BiIter end, Compare comp)
    {
        if (start == end) [[unlikely]] return;
        #pragma clang loop unroll_count(8)
        for (auto curr = std::next(start); curr != end; ++curr) {
            auto value = std::move(*curr);
            auto hole = curr;

            auto prev = hole;
            while (prev > start && comp(value, *std::prev(prev))) {
                --prev;
                *hole = std::move(*prev);
                hole = prev;
            }
            *hole = std::move(value);
        }
    }
	// 严格弱序 不能是 <=  >=
    template<std::random_access_iterator RanIter, std::strict_weak_order<std::iter_value_t<RanIter>, std::iter_value_t<RanIter>> Compare>
    RanIter hoare_quick_select(RanIter start, RanIter end, size_t k, Compare comp)
    {
        const size_t size = std::distance(start, end);
        if (size <= 24) {
            insert_sort(start, end, comp);
            return std::next(start, k);
        }

        auto left = start, right = std::prev(end), mid = left + (right - left) / 2;
        if (comp(*mid, *left)) std::iter_swap(mid, left);
        if (comp(*right, *left)) std::iter_swap(right, left);
        if (comp(*right, *mid)) std::iter_swap(mid, right);

        auto pivot_pos = std::prev(right);
        std::iter_swap(mid, pivot_pos);
        const auto& pivot_val = *pivot_pos;

        auto i = std::prev(left), j = pivot_pos;
        while (true) {
            do { ++i; } while (comp(*i, pivot_val)); // 严格弱序 < >， 最终i 会到达pivot_pos，*i 和 pivot_val相等，不会越界解引用
            do { --j; } while (j > left && comp(pivot_val, *j)); // j是反向递减，因此需要 多加上 j > left 防止越界
            if (i >= j) break;
            std::iter_swap(i, j);
        }
        std::iter_swap(i, pivot_pos);

        if (auto target_pos = start + k; target_pos == i) return i;
        else if (target_pos < i) return hoare_quick_select(std::next(i), end, target_pos - (i + 1), comp); // 在后半部分，前半部分是 i，i + 1是后半部分起始，两个区间相减就是后半区间的k位置
        else return hoare_quick_select(start, i, k, comp);
    }
    int findKthLargest(vector<int>& nums, int k) {
        return *hoare_quick_select(nums.begin(), nums.end(), k - 1, std::greater<>{}); // 第k大，数组下标从0开始，减个1
    }
};

    int findKthLargest(vector<int>& nums, int k) {
        using Iter_t = decltype(nums.begin());
        using Value_t = typename std::iterator_traits<Iter_t>::value_type;
        radixsort<Iter_t, std::less<>, identity_key_extractor<Value_t>, 256U>(nums.begin(), nums.end(), {});
        return *(nums.rbegin() + k - 1);
    }

    template<typename T>
    struct identity_key_extractor {
        template<typename U>
        constexpr auto operator()(U&& value)
            -> std::enable_if_t<std::is_same_v<std::decay_t<U>, T>, T>
        {
            return std::forward<U>(value);
        }
    };

    template<typename ContigIter, typename Compare, typename KeyExtractor, std::uint32_t _Bucket_size>
    requires((std::is_same_v<Compare, std::less<>> || std::is_same_v<Compare, std::greater<>>) &&
             (_Bucket_size == 256U || _Bucket_size == 65536U))
    void radixsort(ContigIter _First, ContigIter _Last, KeyExtractor&& _Extractor) {
        static_assert(std::contiguous_iterator<ContigIter>, 
            "Radix sort requires that the iterator must be contiguous! Like vector, array, raw point");

        using Value_t = typename std::iter_value_t<ContigIter>;
        using Key_t = std::decay_t<decltype(_Extractor(std::declval<Value_t>()))>;
        static_assert(std::is_arithmetic_v<Key_t> && !std::is_same_v<Key_t, bool>, 
            "Key type must be arithmetic( not bool )! Like Integer or Floating");

        static_assert(!(sizeof(Key_t) == 1 && _Bucket_size == 65536U), "1-Bytes bucket size should not be set 65536U!");

        using Unsigend_t = std::make_unsigned_t<
            std::conditional_t<
                std::is_floating_point_v<Key_t>,
                std::conditional_t<sizeof(Key_t) == 4, std::uint32_t, std::uint64_t>,
                Key_t
            >
        >;
        
        Value_t* _Ptr = &*_First;
        std::size_t _Size = std::distance(_First, _Last);
        if (_Size <= 1) return;

        constexpr std::uint8_t _Passes = _Bucket_size == 256U ? sizeof(Key_t) : sizeof(Key_t) >> 1;
        constexpr std::uint8_t _Shift = _Bucket_size == 256U ? 3 : 4;
        constexpr std::uint16_t _Mask = _Bucket_size - 1;

        std::array<std::size_t, _Bucket_size> _Bucket_count;
        std::array<std::size_t, _Bucket_size> _Scanned;

        std::vector<Value_t> _Buffer(_Size);
        Value_t* __restrict _Start = _Ptr;
        Value_t* __restrict _End = _Buffer.data(); 

        for (std::uint8_t _Pass = 0; _Pass < _Passes; ++_Pass) {
            std::fill(_Bucket_count.begin(), _Bucket_count.end(), 0);

            #pragma clang loop vectorize(enable) unroll_count(8)
            for (std::size_t _Idx = 0; _Idx < _Size; ++_Idx) {
                Unsigend_t _Unsigned_value = std::bit_cast<Unsigend_t>(_Extractor(_Start[_Idx]));

                if constexpr (std::is_signed_v<Key_t>) {
                    if constexpr (std::is_floating_point_v<Key_t>) {
                        if constexpr (sizeof(Key_t) == 4)
                            _Unsigned_value ^= (_Unsigned_value & 0x8000'0000U) ? 0xFFFF'FFFFU : 0x8000'0000U;
                        else if constexpr (sizeof(Key_t) == 8)
                            _Unsigned_value ^= (_Unsigned_value & 0x8000'0000'0000'0000ULL) ? 0xFFFF'FFFF'FFFF'FFFFULL : 0x8000'0000'0000'0000ULL;
                    }
                    else {
                        if constexpr (sizeof(Key_t) == 1) _Unsigned_value ^= 0x80U;
                        else if constexpr (sizeof(Key_t) == 2) _Unsigned_value ^= 0x8000U;
                        else if constexpr (sizeof(Key_t) == 4) _Unsigned_value ^= 0x8000'0000U;
                        else if constexpr (sizeof(Key_t) == 8) _Unsigned_value ^=  0x8000'0000'0000'0000ULL;
                    }
                }

                std::uint16_t _Byte_idx = (_Unsigned_value >> (_Pass << _Shift)) & _Mask;

                if constexpr (std::is_same_v<Compare, std::greater<>>) _Byte_idx = _Mask - _Byte_idx;

                ++_Bucket_count[_Byte_idx];
            }

            std::exclusive_scan(_Bucket_count.begin(), _Bucket_count.end(), _Scanned.begin(), 0, std::plus<>{});

            #pragma clang loop vectorize(enable) unroll_count(8)
            for (std::size_t _Idx = 0; _Idx < _Size; ++_Idx) {
                auto _Value = std::move(_Start[_Idx]);
                Unsigend_t _Unsigned_value = std::bit_cast<Unsigend_t>(_Extractor(_Value));

                if constexpr (std::is_signed_v<Key_t>) {
                    if constexpr (std::is_floating_point_v<Key_t>) {
                        if constexpr (sizeof(Key_t) == 4)
                            _Unsigned_value ^= (_Unsigned_value & 0x8000'0000U) ? 0xFFFF'FFFFU : 0x8000'0000U;
                        else if constexpr (sizeof(Key_t) == 8)
                            _Unsigned_value ^= (_Unsigned_value & 0x8000'0000'0000'0000ULL) ? 0xFFFF'FFFF'FFFF'FFFFULL : 0x8000'0000'0000'0000ULL;
                    }
                    else {
                        if constexpr (sizeof(Key_t) == 1) _Unsigned_value ^= 0x80U;
                        else if constexpr (sizeof(Key_t) == 2) _Unsigned_value ^= 0x8000U;
                        else if constexpr (sizeof(Key_t) == 4) _Unsigned_value ^= 0x8000'0000U;
                        else if constexpr (sizeof(Key_t) == 8) _Unsigned_value ^=  0x8000'0000'0000'0000ULL;
                    }
                }

                std::uint16_t _Byte_idx = (_Unsigned_value >> (_Pass << _Shift)) & _Mask;

                if constexpr (std::is_same_v<Compare, std::greater<>>) _Byte_idx = _Mask - _Byte_idx;

                _End[_Scanned[_Byte_idx]++] = std::move(_Value);
            }

            std::swap(_Start, _End);
        }

        if (sizeof(Key_t) == 1) std::move(_Start, _Start + _Size, _Ptr);
    }
    
#include <queue>
#include <algorithm>
#include <array>
using namespace std;
// @lc code=end 
/*
 * @lc app=leetcode.cn id=215 lang=cpp
 *
 * [215] 数组中的第K个最大元素
 */

// @lc code=start
class Solution {
public:
    int findKthLargest(vector<int>& nums, int k) {
//#define MIN_HEAP
//#define QUICK_SELCT
//#define SORT
#define COUNT_SORT
#ifdef MIN_HEAP
        priority_queue<int, vector<int>, greater<int>> min_heap; // 最小堆
        for (int num : nums) {
            if (min_heap.size() < k) {
                min_heap.push(num);
            } else if (num > min_heap.top()) {
                min_heap.pop();
                min_heap.push(num);
            }
        }
        return min_heap.top();
#elif defined(QUICK_SELCT)
        // 将第k大转换为第(n - k + 1)小
        int n = nums.size();
        int target_pos = n - k;
        int left = 0, right = n - 1;
        
        while (true) {
            int pivot_index = partition(nums, left, right);
            if (pivot_index == target_pos) {
                return nums[pivot_index];
            } else if (pivot_index < target_pos) {
                left = pivot_index + 1;
            } else {
                right = pivot_index - 1;
            }
        }
#elif defined(SORT)
        sort(nums.begin(), nums.end(), greater<int>());
        return nums[k - 1];
#elif defined(COUNT_SORT)
        array<int, 20001> count{0};
        #pragma GCC unroll 8
        for(int n : nums) ++count[n + 10000]; // 正确统计次数
        #pragma GCC unroll 8
        for(int i = 20000; i >=0; --i){
            if(count[i] == 0) continue;
            // 当剩余k小于当前元素个数时，即为目标值
            if(k > count[i]) {
                k -= count[i];
            } else {
                return i - 10000; // 回撤偏移量
            }
        }
        return -1;
#endif
    }

private:
    int partition(vector<int>& nums, int left, int right) {
        // 随机选择pivot避免最坏情况
        int pivot_index = left + rand() % (right - left + 1);
        int pivot = nums[pivot_index];
        swap(nums[pivot_index], nums[right]); // 将pivot移到末尾
        
        int store_index = left;
        for (int i = left; i < right; ++i) {
            if (nums[i] < pivot) {
                swap(nums[i], nums[store_index]);
                store_index++;
            }
        }
        swap(nums[store_index], nums[right]); // 将pivot放回正确位置
        return store_index;
    }
};
// @lc code=end

