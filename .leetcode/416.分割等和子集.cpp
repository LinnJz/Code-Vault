bool canPartition(vector<int>& nums) {
	int totalSum = 0, maxNum = 0;
	for (int num : nums) {
		totalSum += num;
		if (maxNum < num) maxNum = num;
	}
	if (totalSum & 1) return false;

	int halfSum = totalSum >> 1;
	if (maxNum > halfSum) return false;

	int *dp = reinterpret_cast<int *>(::alloca((halfSum + 1) * sizeof(int)));
	std::fill(dp, dp + halfSum + 1, 0);

	for (int num : nums) {
		#pragma clang loop unroll_count(4)
		for (int j = halfSum; j >= num; --j) {
			if (auto val = dp[j - num] + num; val > dp[j])
				dp[j] = val;
		}
	}
	return dp[halfSum] == halfSum;
}
/*
 * @lc app=leetcode.cn id=416 lang=cpp
 *
 * [416] 分割等和子集
 */

// @lc code=start
#include <vector>
#include <algorithm>
#include <numeric>
using namespace std;
class Solution {
public:
    bool canPartition(vector<int>& nums) {
         int sum = accumulate(nums.begin(), nums.end(), 0);
        
        // 如果总和为奇数，无法分成两个相等的子集
        if (sum % 2 != 0) return false;
        
        int target = sum / 2;
        int n = nums.size();
        
        // dp[i]表示是否可以选择一些数字使它们的和为i
        vector<bool> dp(target + 1, false);
        dp[0] = true;  // 不选择任何数字，和为0是可能的
        int curr = 0; //动态上界跟踪——跟踪当前可能达到的最大和
        for (int num : nums) {
            // 从大到小遍历，避免重复使用同一个数字
            curr = min(curr + num, target);
            for (int i = curr; i >= num; --i) {
                // 状态转移：如果dp[i-num]为true，则dp[i]也为true
                dp[i] = dp[i] || dp[i - num];
            }
        }
        
        return dp[target];
        /*
        贪心只能处理部分测试，如[1,1,2,2]不能通过
        sort(nums.begin(), nums.end());
        int part1 = 0;
        int part2 = nums.back();
        for(int left = 0, right = nums.size() - 1; left < right; ++left){
            part1 += nums[left];
            if(part1 > part2) {
                part2 += nums[--right];
            }
            if(right - left == 1 && part1 == part2) return true;
        }
        return false;
        */
    }
};
// @lc code=end

