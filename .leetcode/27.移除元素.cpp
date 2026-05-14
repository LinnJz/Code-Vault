/*
 * @lc app=leetcode.cn id=27 lang=cpp
 *
 * [27] 移除元素
 */

// @lc code=start
class Solution {
public:
    int removeElement(vector<int>& nums, int val) {
        std::erase_if(nums, [&] (int num) { return val == num; });
        return nums.size();
    }
    int removeElement(vector<int>& nums, int val) {
        nums.erase(std::remove(nums.begin(), nums.end(), val), nums.end());
        return nums.size();
    }
    int removeElement(vector<int>& nums, int val) {
        auto it = std::partition(nums.begin(), nums.end(), [val](int n) { return n != val; });
        return std::distance(nums.begin(), it);
    }
    int removeElement(vector<int>& nums, int val) {
        auto p1 = nums.begin();
        for (auto p2{p1}; p2 != nums.end(); ++p2) {
            if (*p2 == val) continue;
            *p1 = *p2;
            ++p1;
        }
        for (int64_t size(std::distance(p1, nums.end())) ; size > 0; --size) {
            nums.pop_back();
        }
        return nums.size();
    }
};
// @lc code=end

