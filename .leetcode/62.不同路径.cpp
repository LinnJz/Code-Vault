/*
 * @lc app=leetcode.cn id=62 lang=cpp
 *
 * [62] 不同路径
 */

// @lc code=start
// 定义使用哪种解法
#define ONE_DIM_DP  // 默认使用一维动态规划
//#define TWO_DIM_DP
//#define DFS_MEMO
//#define MATH_SOLUTION
// https://quick-bench.com/q/SDR0NnIy_hNEubCN4lIA0Ea8L18
class Solution {
public:
    int uniquePaths(int m, int n) {
#ifdef DFS_MEMO
        // DFS + 记忆化搜索解法
        vector<vector<int>> memo(m, vector<int>(n, -1));
        return dfs(0, 0, m - 1, n - 1, memo);

#elif defined(TWO_DIM_DP)
        // 二维动态规划解法
        vector<vector<int>> dp(m, vector<int>(n, 0));
        
        // 初始化第一行和第一列
        for (int i = 0; i < m; i++) {
            dp[i][0] = 1;
        }
        for (int j = 0; j < n; j++) {
            dp[0][j] = 1;
        }
        
        // 填充dp数组
        for (int i = 1; i < m; i++) {
            for (int j = 1; j < n; j++) {
                dp[i][j] = dp[i-1][j] + dp[i][j-1];
            }
        }
        
        return dp[m-1][n-1];

#elif defined(ONE_DIM_DP)
        // 一维动态规划解法（空间优化）
        int *dp = reinterpret_cast<int *>(::alloca(n * sizeof(int)));
        std::fill(dp, dp + n, 1);  // 初始化为1，因为第一行都是1
        
        // 动态规划过程
        for (int i = 1; i < m; i++) {
            #pragma GCC unroll 8
            for (int j = 1; j < n; j++) {
                dp[j] += dp[j-1];  // dp[j] = 上方的值(旧的dp[j]) + 左方的值(dp[j-1])
            }
        }
        
        return dp[n - 1];

#elif defined(MATH_SOLUTION)
        // 数学解法（组合数学）
        // 从起点到终点需要移动 m-1+n-1 次，其中向下移动 m-1 次，向右移动 n-1 次
        // 所以不同路径数等于从 m+n-2 次移动中选择 m-1 次向下移动的方式数，即 C(m+n-2, m-1) 等价于 C(m+n-2, m-1)（组合数的对称性 C(N, k) = C(N, N-k)）
        
        // 为避免溢出，选择较小的值作为组合数计算的基础
        if (m < n) std::swap(m, n);
        std::size_t smaller( n - 1 ), larger( m - 1 ), result{ 1ULL };

        // 计算组合数 C(m+n-2, smaller)
        #pragma GCC unroll 8
        for (int i = 1; i <= smaller; ++i) {
            result = result * (larger + i) / i;
        }
        return static_cast<int>(result);
#endif
    }

private:
#ifdef DFS_MEMO
    // DFS + 记忆化搜索辅助函数
    int dfs(int i, int j, int m, int n, vector<vector<int>>& memo) {
        // 到达终点
        if (i == m && j == n) return 1;
        
        // 越界
        if (i > m || j > n) return 0;
        
        // 已经计算过
        if (memo[i][j] != -1) return memo[i][j];
        
        // 向右和向下移动
        memo[i][j] = dfs(i + 1, j, m, n, memo) + dfs(i, j + 1, m, n, memo);
        return memo[i][j];
    }
#endif
};
// @lc code=end