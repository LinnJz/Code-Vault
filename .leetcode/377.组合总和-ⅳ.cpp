// 求排列数，先背包后物品
int combinationSum4(vector<int>& nums, int target) {
	std::sort(nums.begin(), nums.end());

	auto *dp = reinterpret_cast<uint32_t *>(::alloca((target + 1) * sizeof(uint32_t)));
	dp[0] = 1, std::fill(dp, dp + target + 1, 0);

	for (int j = 1; j <= target; ++j) {
		// j 是背包容量
		for (int num : nums) {
			if (num <= j) dp[j] += dp[j - num];
			else break;
		}
	}
	return dp[target];
}
/*
 * @lc app=leetcode.cn id=377 lang=cpp
 *
 * [377] 组合总和 Ⅳ
 */

// @lc code=start
class Solution {
public:
    int combinationSum4(std::vector<int>& nums, int target) {
        std::vector<std::size_t> dp(target + 1, 0);
        dp.front() = 1;

        for(int j = 0; j <= target; ++j) {
            #pragma clang loop vectorize(enable) unroll_count(8)
            for(int num : nums) {
                if (j - num >= 0) dp[j] += dp[j - num];
            }
        }
        return dp.back();
    }
};
// @lc code=end

