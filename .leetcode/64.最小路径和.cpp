int minPathSum(vector<vector<int>>& grid) {
	size_t const ROWS = grid.size(), COLS = grid[0].size();
	
	int *dp = reinterpret_cast<int *>(::alloca((COLS + 1) * sizeof(int)));
	dp[0] = dp[1] = 0;
	std::fill(dp + 2, dp + COLS + 1, INT_MAX);
	
	for (size_t row = 1; row <= ROWS; ++row) {
		dp[0] = dp[1]; // std::min取最小，因为第0列都是0，所以相等避免不是真实值
		#pragma clang loop unroll_count(4)
		for (int col = 1; col <= COLS; ++col) {
			dp[col] = std::min(dp[col], dp[col - 1]) + grid[row - 1][col - 1];
		}
	}

	return dp[COLS];
}
/*
 * @lc app=leetcode.cn id=64 lang=cpp
 *
 * [64] 最小路径和
 */

// @lc code=start
#define OPTIMIZATION
class Solution {
public:
    int minPathSum(vector<vector<int>>& grid) {
#ifdef OPTIMIZATION
        if (grid.empty() || grid[0].empty()) return 0;
        
        int m = grid.size();
        int n = grid[0].size();
        
        // 使用一维dp数组
        vector<int> dp(n, 0);
        
        // 初始化
        dp[0] = grid[0][0];
        for (int j = 1; j < n; j++) {
            dp[j] = dp[j-1] + grid[0][j];
        }
        
        // 动态规划过程
        for (int i = 1; i < m; i++) {
            // 更新第一列
            dp[0] += grid[i][0];
            
            for (int j = 1; j < n; j++) {
                // dp[j]表示上方的值，dp[j-1]表示左方的值
                dp[j] = min(dp[j], dp[j-1]) + grid[i][j];
            }
        }
        
        return dp[n-1];
#else
        if (grid.empty() || grid[0].empty()) return 0;
        
        int m = grid.size();
        int n = grid[0].size();
        
        // dp[i][j] 表示从左上角到达 (i,j) 位置的最小路径和
        vector<vector<int>> dp(m, vector<int>(n, 0));
        
        // 初始化起点
        dp[0][0] = grid[0][0];
        
        // 初始化第一行（只能从左边来）
        for (int j = 1; j < n; j++) {
            dp[0][j] = dp[0][j-1] + grid[0][j];
        }
        
        // 初始化第一列（只能从上边来）
        for (int i = 1; i < m; i++) {
            dp[i][0] = dp[i-1][0] + grid[i][0];
        }
        
        // 填充dp数组
        for (int i = 1; i < m; i++) {
            for (int j = 1; j < n; j++) {
                // 可以从上边或左边到达当前位置，选择路径和较小的那个
                dp[i][j] = min(dp[i-1][j], dp[i][j-1]) + grid[i][j];
            }
        }
        
        return dp[m-1][n-1];
#endif
    }
};
// @lc code=end

