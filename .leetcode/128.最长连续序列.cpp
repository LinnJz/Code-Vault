// 标准写法
// 2O(n)，建表，循环，空间O(n)
int longestConsecutive(vector<int>& nums) {
    std::unordered_set<int> num_set(nums.begin(), nums.end());
    int max_length = 0;
    for (int num : num_set) {
        if (num_set.find(num - 1) == num_set.end()) {
            int current = num;
            int length = 1;
            while (num_set.find(++current) != num_set.end()) {
                ++length;
            }
            if(max_length < length) max_length = length;
			// 剪枝条件的本质是鸽巢原理与连续整数区间的不相交性的组合。
			// 每个连续序列对应一段连续的整数区间（例如 [a, a+L-1]），且不同序列的区间在数轴上互不重叠（否则会合并成一个更大的连续序列）。

			// 代码只从每个区间的最小值（即 num-1 不存在的元素）开始探测，因此每次循环处理的是一个完整的、互不重叠的连续区间。

			// 设当前找到的连续区间长度为 L，原始数组长度为 n（含重复值）。由于每个不同的整数至少出现一次，所以所有互不重叠的连续区间的长度之和 ≤ 不同整数的个数 ≤ n。

			// 如果 2L ≥ n，则其余所有区间长度之和 ≤ n - L ≤ L，因此任何其他区间长度不可能超过 L。此时 L 已经是全局最长连续序列长度，可以提前终止循环。
            if(max_length * 2 >= nums.size()) break;
        }
    }

    return max_length;
}
int longestConsecutive(vector<int>& nums) {
	if (nums.empty()) [[unlikely]] return 0;

	radix_sort(nums.data(), nums.size());

	int dummy = nums.front() - 1, currLen = 0, maxLen = 0;
	#pragma clang loop unroll_count(2)
	for (int num : nums) {
		if (dummy == num) continue; // 重复
		else if (dummy + 1 != num) currLen = 1; // 断裂
		else if (++currLen > maxLen) maxLen = currLen; // 连续
		dummy = num;
	}
	return maxLen;
}
// 排序剪枝优化
int longestConsecutive(vector<int>& nums) {
        if (nums.empty()) [[unlikely]] return 0;

        radix_sort(nums.data(), nums.size());

        int dummy = nums.front() - 1, currLen = 0, maxLen = 0;
        #pragma clang loop unroll_count(4)
        for (int num : nums) {
            if (dummy == num) continue;
            else if (dummy + 1 != num) {
				// 断裂的时候应用剪枝
                if (currLen > maxLen && (maxLen = currLen) * 2 >= nums.size()) break;
                currLen = 1;
            } 
            else ++currLen;

            dummy = num;
        }
		// 处理最后一段连续到尾
        if (currLen > maxLen) maxLen = currLen;
        return maxLen;
    }
/*
 * @lc app=leetcode.cn id=128 lang=cpp
 *
 * [128] 最长连续序列
 */

// @lc code=start
#ifdef _DEBUG
#include <iostream>
#include <format>
#endif // _DEBUG

#include <vector>
#include <ranges>
#include <algorithm>
#include <unordered_set>
using namespace std;
class Solution {
public:
    //方案一：计数排序，然后再遍历找最长length，
    //  2O（n）复杂度，但是num[i] <= 10^9 不太现实
    //方案二：普通排序，然后再遍历找最长length，
    //  时间复杂度是排序时间+O(n)
    //方案三：过滤出无重复的没有前驱的数字集合，然后遍历这个集合统计长度，
    //  时间复杂度低于O(n^2)，但是大于2*O(n)，实际是求和n次找到前驱的时间

    //实际情况考虑到量级[0, 10^5]次方，最优的情况应该采用方案二
     /*  步骤
      * 1、使用标准库的排序，排序数组
      * 2、循环查找严格连续递增的序列，使用临时长度变量，max统计其最大长度，
           如果相差不等于1，则重置临时长度为1，重复元素跳过
      */
    int longestConsecutive(vector<int>& nums) {
#define SORT
#ifdef SORT
        if (nums.empty()) return 0;
        std::sort(nums.begin(), nums.end());
        int currLen = 1, maxLen = currLen;
        #pragma clang loop unroll_count(8)
        for (auto it = std::next(nums.begin()); it != nums.end(); ++it) {
            int prev_val = *std::prev(it);
            if (prev_val == *it) continue;
            if (prev_val + 1 == *it) {
                if (++currLen > maxLen) maxLen = currLen;
                if (maxLen * 2 >= nums.size()) break;
            }
            else {
                currLen = 1;
            }
        }
        return maxLen;
#else
        int max_length = 1;
        std::unordered_set<int> num_set(nums.begin(), nums.end()); // 替换为boost::unordered::unordered_flat_set（开放地址） 性能提升4~7倍
        int max_length = 0;
        // C++23范围视图：筛选没有前驱的数字作为序列起点
        auto start_points = num_set | std::views::filter([&](int num) {
            return !num_set.contains(num - 1);
            });
        // 遍历所有起点，扩展连续序列
        for (int start : start_points) {
            int current = start;
            int length = 1;
            // 向后扩展序列直到不连续
            while (num_set.contains(++current)) {
                ++length;
            }

            if(max_length < length) max_length = length;
            if(max_length * 2 >= nums.size()) break;
        }
        return max_length;
#endif // SORT
    }
};
// @lc code=end

// 最快基数排序，符合O(n), 排序8n + 8(256), 查找O(n), 空间 O(n) + 2 * 256
class Solution {
public:
    int longestConsecutive(vector<int>& nums) {
        if (nums.empty()) [[unlikely]] return 0;
        using Iter_t = decltype(nums.begin());
        using Value_t = std::iter_value_t<Iter_t>;
        radixsort<Iter_t, std::less<>, identity_key_extractor<int>, 256U>(nums.begin(), nums.end(), {});
        int dummy = nums.front() - 1, currLen = 0, maxLen = 0;
        for (int num : nums) {
            if (dummy == num) continue;
            if (dummy + 1 != num) currLen = 1;
            else if (++currLen > maxLen) maxLen = currLen;
            dummy = num;
        }
        return maxLen;
    }
    template<class T>
    struct identity_key_extractor {
        template<class U>
        constexpr auto operator()(U&& value)
            -> std::enable_if_t<std::same_as<T, std::decay_t<U>>, T>
        {
            return std::forward<U>(value);
        }
    };

    template<class ContigIter, class Compare, class KeyExtractor, std::uint32_t Bucket_size>
    requires((std::same_as<Compare, std::less<>> || std::same_as<Compare, std::greater<>>) || 
             (Bucket_size == 256U || Bucket_size == 65536U))
    void radixsort(ContigIter First, ContigIter Last, KeyExtractor Extractor) 
    {
        static_assert(std::contiguous_iterator<ContigIter>, "");
        using Value_t = std::iter_value_t<ContigIter>;
        using Key_t = std::decay_t<decltype(Extractor(std::declval<Value_t>()))>;
        static_assert(!(sizeof(Key_t) == 1 && Bucket_size == 65536U), "");
        static_assert(std::is_arithmetic_v<Key_t> && !std::same_as<Key_t, bool>, "");

        using Unsigned_t = std::make_unsigned_t<
            std::conditional_t<
                std::is_floating_point_v<Key_t>,
                std::conditional_t<sizeof(Key_t) == 4, std::uint32_t, std::uint64_t>,
                Key_t
            >
        >;

        
        std::size_t Size = std::distance(First, Last);
        if (Size <= 1) [[unlikely]] return;
        auto* Ptr = &*First;

        constexpr std::uint8_t Passes = Bucket_size == 256U ? sizeof(Key_t) : sizeof(Key_t) >> 1;
        constexpr std::uint8_t Shift = Bucket_size == 256U ? 3 : 4;
        constexpr std::uint16_t Mask = Bucket_size - 1;

        constexpr std::uint8_t Key_bits = sizeof(Key_t) << 3;
        constexpr Unsigned_t Sign_bit_mask = Unsigned_t{ 1 } << (Key_bits - 1);
        constexpr Unsigned_t All_Bits_mask = ~Unsigned_t{ 0 };

        constexpr bool Is_descending = std::same_as<Compare, std::greater<>>;

        std::array<std::size_t, Bucket_size> Bucket_count;
        std::array<std::size_t, Bucket_size> Scanned;

        std::vector<Value_t> Buffer(Size);
        auto* __restrict Start = Ptr;
        auto* __restrict End = Buffer.data();

        for (std::uint8_t Pass = 0; Pass < Passes; ++Pass) {
            std::fill(Bucket_count.begin(), Bucket_count.end(), 0);
            
            #pragma clang loop unroll_count(8)
            for (std::size_t Idx = 0; Idx < Size; ++Idx) {
                Unsigned_t Unsigned_value = std::bit_cast<Unsigned_t>(Extractor(Start[Idx]));
                if constexpr (std::is_floating_point_v<Key_t>) {
                    Unsigned_value ^= (Unsigned_value >> (Key_bits - 1) == 0) ? Sign_bit_mask : All_Bits_mask;
                }
                else if constexpr (std::is_signed_v<Key_t>) {
                    Unsigned_value ^= Sign_bit_mask;
                }

                std::uint16_t Byte_idx = (Unsigned_value >> (Pass << Shift)) & Mask;
                if constexpr (Is_descending) Byte_idx = Mask - Byte_idx; 
                ++Bucket_count[Byte_idx];
            }

            std::exclusive_scan(Bucket_count.begin(), Bucket_count.end(), Scanned.begin(), 0, std::plus<>{});

            #pragma clang loop unroll_count(8)
            for (std::size_t Idx = 0; Idx < Size; ++Idx) {
                auto Value = std::move(Start[Idx]);
                Unsigned_t Unsigned_value = std::bit_cast<Unsigned_t>(Extractor(Value));
                if constexpr (std::is_floating_point_v<Key_t>) {
                    Unsigned_value ^= (Unsigned_value >> (Key_bits - 1) == 0) ? Sign_bit_mask : All_Bits_mask;
                }
                else if constexpr (std::is_signed_v<Key_t>) {
                    Unsigned_value ^= Sign_bit_mask;
                }

                std::uint16_t Byte_idx = (Unsigned_value >> (Pass << Shift)) & Mask;
                if constexpr (Is_descending) Byte_idx = Mask - Byte_idx; 
                
                End[Scanned[Byte_idx]++] = std::move(Value);
            }

            std::swap(Start, End);
        }

        if (sizeof(Key_t) == 1) std::move(Start, Start + Size, Ptr);
    }
};