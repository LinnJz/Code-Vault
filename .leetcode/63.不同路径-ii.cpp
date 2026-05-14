#include <vector>
#include <algorithm>
#include <iterator>
/*
 * @lc app=leetcode.cn id=63 lang=cpp
 *
 * [63] 不同路径 II
 */

// @lc code=start
class Solution {
public:
    int uniquePathsWithObstacles(std::vector<std::vector<int>>& obstacleGrid) {
        auto const &firstRowVec = obstacleGrid[0];
        size_t const ROWS = obstacleGrid.size(), COLS = firstRowVec.size();
		// 如果起点或终点有障碍物，直接返回0
        if (firstRowVec[0] == 1 || obstacleGrid.back().back() == 1) [[unlikely]] return 0;
        
		int *dp = reinterpret_cast<int *>(::alloca(COLS * sizeof(int)));
        for (size_t i = 0; i < COLS; ++i) {
            if (firstRowVec[i]) { // 有障碍说明无法到达 设置为0，否则是左边值能够到达当前
                std::fill(dp + i, dp + COLS, 0);
                break; 
            }
            dp[i] = 1;
        }
		
        for (size_t row = 1; row < ROWS; ++row) {
            if (obstacleGrid[row][0] == 1) dp[0] = 0;
            #pragma clang loop unroll_count(4)
            for (size_t col = 1; col < COLS; ++col) {
                if (obstacleGrid[row][col] == 0) dp[col] += dp[col - 1]; // curr = top + left 
                else dp[col] = 0; // 有障碍说明无法到达 设置为0
            }
        }

        return dp[COLS - 1];
    }
};
// @lc code=end

