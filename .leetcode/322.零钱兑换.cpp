int coinChange(vector<int>& coins, int amount) {
	if (amount == 0) [[unlikely]] return 0;
	
	int* dp = reinterpret_cast<int *>(::alloca((amount + 1) * sizeof(int)));
	// amount == 0 就是0， 取最小，我们就设置最大
	dp[0] = 0, std::fill(dp + 1, dp + amount + 1, amount + 1);

	for (int num : coins) {
		#pragma clang loop unroll_count(4)
		for (int j = num; j <= amount; ++j) {
			// + 1 加上本身，及值，coin的值则是weight，取最小
			if(auto val = dp[j - num] + 1; val < dp[j]) dp[j] = val;
		}
	}
	return dp[amount] > amount ? -1 : dp[amount];
}

/*
 * @lc app=leetcode.cn id=322 lang=cpp
 *
 * [322] 零钱兑换
 */

// @lc code=start
class Solution {
public:
/*
DP[i]的定义是：凑到i这么多钱，需要最少多少个硬币。

那么对于[1,3,4,5]，要凑7块钱的计算方法有4种：
选一个硬币1: 需要的数量就是1 + DP[7-1] = 1 + DP[6]， 也就是选一个硬币+DP[6]的量
选一个硬币3: 需要的数量就是1 + DP[7-3] = 1 + DP[4]
选一个硬币4: 需要的数量就是1 + DP[7-4] = 1 + DP[3]
选一个硬币5: 需要的数量就是1 + DP[7-5] = 1 + DP[2]

综合下来选最小：
DP[7] = min(1 + DP[6], 1 + DP[4], 1 + DP[3], 1 + DP[2])
*/
    int coinChange(vector<int>& coins, int amount) {
        // 创建dp数组，初始值设为amount+1（一个不可能的值，因为最多只需要amount个1元硬币）
        // 下标代表金额，元素代表凑成金额的硬币数
        vector<int> dp(amount + 1, amount + 1);
        
        // 初始状态：凑成金额0需要0个硬币
        dp[0] = 0;
        
        // 计算每个金额所需的最少硬币数
        for (int i = 1; i <= amount; ++i) {
            // 尝试使用每种面值的硬币
            for (int coin : coins) {
                // 只有当前金额i大于等于硬币面值时才能使用该硬币
                if (i >= coin) {
                    // 状态转移方程：dp[i] = min(dp[i], dp[i-coin] + 1)
                    dp[i] = min(dp[i], dp[i - coin] + 1);
                }
            }
        }
        
        // 如果dp[amount]仍然是初始值，说明无法凑成目标金额
        return dp[amount] > amount ? -1 : dp[amount];
    }
};
// @lc code=end

