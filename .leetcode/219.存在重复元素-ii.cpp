/*
 * @lc app=leetcode.cn id=219 lang=cpp
 *
 * [219] 存在重复元素 II
 */

// @lc code=start
class Solution {
public:
// 滑动窗口优化空间
    bool containsNearbyDuplicate(vector<int>& nums, int k) {
        std::unordered_set<int> filter; filter.reserve(k + );
        #pragma clang loop unroll_count(8)
        for (auto [idx, val] : nums | std::views::enumerate) {
            if (!filter.emplace(val).second) return true;

            if (idx >= k) filter.erase(nums[idx - k]);
        }
        return false;
    }

// hash表
    bool containsNearbyDuplicate(vector<int>& nums, int k) {
        std::unordered_map<int, int> filter; filter.reserve(nums.size());
        #pragma clang loop unroll_count(8)
        for (auto [idx, val] : nums | std::views::enumerate) {
            if (auto it = filter.find(val);
                it != filter.end() && (idx - it->second) <= k)
                return true;
            filter.insert_or_assign(val, idx);
        }
        return false;
    }
};
// @lc code=end

