class Solution {
public:
    int numSquares(int n) {
		int *dp = reinterpret_cast<int *>(::alloca((n + 1) * sizeof(int)));
		dp[0] = 0, std::fill(dp + 1, dp + n + 1, INT_MAX);
		
        for(int i = 1; i * i <= n; ++i) { // 物品 是 完全平方数
            #pragma clang loop unroll_count(8)
            for(int j = i * i; j <= n; ++j) { // 背包
                if(auto val = dp[j - i * i] + 1; val < dp[j]) dp[j] = val;
            }
        }
        return dp[n];
    }
};

int numSquares(int n) {
	int *dp = reinterpret_cast<int *>(::alloca((n + 1) * sizeof(int)));
	dp[0] = 0, std::fill(dp + 1, dp + n + 1, INT_MAX);

	for (int j = 1; j <= n; ++j) { // 背包
		#pragma clang loop unroll_count(8)
		for (int i = 1; i * i <= j; ++i) { // 物品
			if(auto val = dp[j - i * i] + 1; val < dp[j]) dp[j] = val;
		}
	}

	return dp[n];
}
/*
 * @lc app=leetcode.cn id=279 lang=cpp
 *
 * [279] 完全平方数
 */

// @lc code=start
//#define DP
//#define GREEDY
#include <cmath>
class Solution {
public:
/*
13
dp[0]
dp[1] INT_MAX dp[0] + 1  1
dp[2] INT_MAX dp[1] + 1  2
dp[3] INT_MAX dp[2] + 1  3
dp[4] INT_MAX dp[3] + 1  4
dp[4] 4       dp[0] + 1  1
dp[5] INT_MAX dp[4] + 1  2
dp[5] 2       dp[1] + 1  2
dp[6] INT_MAX dp[5] + 1  3
dp[6] 3       dp[2] + 1  3
dp[7] INT_MAX dp[6] + 1  4
dp[7] 4       dp[3] + 1  4
dp[8] INT_MAX dp[7] + 1  5
dp[8] 5       dp[4] + 1  2
dp[9] INT_MAX dp[8] + 1  3
dp[9] 3       dp[5] + 1  3
dp[9] 3       dp[0] + 1  1
- dp数组含义：
——  存储数据：完全平方数的个数
——  下标：参数n
- for循环含义
——  外层
————    顺序，终止条件
————    意义：“归”步骤，最小子问题，自底向上，1到参数n
——  内层
————    顺序，终止条件
————    意义：“归”步骤，最小子问题，自底向上，参数n与完全平方数的关系
             如果是完全平方数那么始终会找到dp[0]，结果1
             如果不是完全平方数，始终是完全平方数和1的和，
             那么会找参数n与小于的完全平方数，进行关联，
- 状态转移公式

*/
    int numSquares(int n) {
#ifdef DP
        int sqrtN = (int)sqrt(n);
        if (sqrtN * sqrtN == n)  return 1;
        // 动态规划解法
        // 创建dp数组，初始值设为最大可能值（即n，因为最差情况下可以用n个1）
        vector<int> dp(n + 1, INT_MAX);
        
        // 初始状态
        dp[0] = 0;
        
        // 计算每个数的最少完全平方数
        for (int i = 1; i <= n; ++i) {
            // 尝试用每个小于等于i的平方数来更新dp[i]
            for (int j = 1; j * j <= i; ++j) {
                dp[i] = min(dp[i], dp[i - j * j] + 1);
            }
        }
        
        return dp[n];
#elif defined(GREEDY)
        // 贪心算法解法
        // 完全平方数列表
        vector<int> squares;
        for (int i = 1; i * i <= n; ++i) {
            squares.push_back(i * i);
        }
        
        // 检查是否为完全平方数
        if (squares.back() == n) {
            return 1;
        }
        
        // BFS搜索最短路径
        queue<pair<int, int>> q; // <剩余数字, 已使用平方数个数>
        q.push({n, 0});
        unordered_set<int> visited; // 避免重复访问
        visited.insert(n);
        
        while (!q.empty()) {
            int remainder = q.front().first;
            int step = q.front().second;
            q.pop();
            
            // 尝试减去每个平方数
            for (int i = squares.size() - 1; i >= 0; --i) { // 从大到小尝试
                int next = remainder - squares[i];
                
                if (next == 0) {
                    return step + 1; // 找到解
                }
                
                if (next > 0 && visited.find(next) == visited.end()) {
                    q.push({next, step + 1});
                    visited.insert(next);
                }
            }
        }
        
        return -1; // 不会到达这里
#else
        // 数学解法（基于四平方和定理）
        // 四平方和定理：任何正整数都可以表示为至多4个平方数的和
        
        // 检查是否为完全平方数
        int sqrtN = (int)sqrt(n);
        if (sqrtN * sqrtN == n) {
            return 1;
        }
        
        // 检查是否可以表示为两个平方数之和
        for (int i = 1; i <= sqrtN; ++i) {
            int j = (int)sqrt(n - i * i);
            if (i * i + j * j == n) {
                return 2;
            }
        }
        
        // 检查是否为4^k*(8*m+7)形式
        // 如果是，则答案为4
        int temp = n;
        while (temp % 4 == 0) {
            temp /= 4;
        }
        if (temp % 8 == 7) {
            return 4;
        }
        
        // 其他情况，答案为3
        return 3;
#endif
    }
};
// @lc code=end