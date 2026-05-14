int minDistance(string const &word1, string const &word2) {
	if (word1.size() > word2.size()) return minDistance(word2, word1);
	size_t const w1Len = word1.size(), w2Len = word2.size();

	int *dp = reinterpret_cast<int *>(::alloca((w2Len + 1) * sizeof(int)));
	std::iota(dp, dp + w2Len + 1, 0);

	for (size_t i = 1; i <= w1Len; ++i) {
		int topLeft = dp[0];
		dp[0] = i;
		#pragma clang loop unroll_count(4)
		for (size_t j = 1; j <= w2Len; ++j) {
			int top = dp[j];
			dp[j] = word1[i - 1] != word2[j - 1]
					? std::min({dp[j - 1], top, topLeft}) + 1
					: topLeft;
			topLeft = top;
		}
	}
	return dp[w2Len];
}
/*
 * @lc app=leetcode.cn id=72 lang=cpp
 *
 * [72] 编辑距离
 */

// @lc code=start

// 定义使用哪种实现方式，默认使用一维DP
// #define TWO_DIM_DP  // 二维DP实现
#define ONE_DIM_DP  // 一维DP实现（空间优化）
class Solution {
public:
    int minDistance(std::string word1, std::string word2) {
        int w1Len = word1.size(), w2Len = word2.size();
        std::string s1, s2;
        s1.reserve(w1Len + 1), s2.reserve(w2Len + 1);
        s1 = std::move(word1), s2 = std::move(word2);
        s1.push_back('#'), s2.push_back('#');

        // 可以使用small_vector优化，int arr[len]仅在gcc支持，并且不是C++标准不推荐
        int cols = w2Len + 1;  // 列数
        std::vector<int> dp((w1Len + 1) * cols, 0);
        for (int i = w1Len; i >= 0; --i) {
            for (int j = w2Len; j >= 0; --j) {
                int index = i * cols + j;  // 正确的索引计算
                if (i == w1Len && j == w2Len) continue; //dp[index] = 0
                else if (i == w1Len) dp[index] = w2Len - j; // word1用完，需要插入剩余的word2字符
                else if (j == w2Len) dp[index] = w1Len - i; // word2用完，需要删除剩余的word1字符
                else if (s1[i] == s2[j]) dp[index] = dp[(i + 1) * cols + (j + 1)];  // 字符匹配
    #pragma push_macro("min")
    #undef min
                else {
                    // 三种操作：替换、插入、删除
                    int replace = dp[(i + 1) * cols + (j + 1)];
                    int insert = dp[i * cols + (j + 1)];
                    int remove = dp[(i + 1) * cols + j];
                    dp[index] = 1 + std::min({ replace, insert, remove });
                }
    #pragma pop_macro("min")
            }
        }
        return dp[0];
    }

    int minDistance(std::string word1, std::string word2) {
        int w1Len = word1.size(), w2Len = word2.size();
        word1 = '#' + word1, word2 = '#' + word2;

        // 可以使用small_vector优化，int arr[len]仅在gcc支持，并且不是C++标准不推荐
        std::vector<int> dp((w1Len + 1) * (w2Len + 1), 0);
        for(int i = 0; i <= w1Len; ++i) {
            for(int j = 0; j <= w2Len; ++j) {
                if (i == 0 && j == 0) continue; //dp[0] = 0;
                else if (i == 0) dp[i * (w2Len + 1) + j] = j;
                else if (j == 0) dp[i * (w2Len + 1) + j] = i;
                else if (word1[i] == word2[j]) dp[i * (w2Len + 1) + j] = dp[(i-1) * (w2Len + 1) + (j-1)];
                else {
                    int replace = dp[(i-1) * (w2Len + 1) + (j-1)];
                    int deleteOp = dp[(i-1) * (w2Len + 1) + j];
                    int insert = dp[i * (w2Len + 1) + (j-1)];
                    dp[i * (w2Len + 1) + j] = 1 + std::min(replace, std::min(deleteOp, insert));
                }
            }
        }

        return dp[w1Len * (w2Len + 1) + w2Len];
    }
};
class Solution {
public:
    int minDistance(string word1, string word2) {
#ifdef TWO_DIM_DP
        // 二维动态规划实现
        int m = word1.size();
        int n = word2.size();
        
        // dp[i][j] 表示 word1 的前 i 个字符转换到 word2 的前 j 个字符需要的最少操作数
        vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));
        
        // 初始化边界情况
        // 当 word2 为空串时，将 word1 的所有字符删除
        for (int i = 1; i <= m; i++) {
            dp[i][0] = i;
        }
        
        // 当 word1 为空串时，插入 word2 的所有字符
        for (int j = 1; j <= n; j++) {
            dp[0][j] = j;
        }
        
        // 填充 dp 数组
        for (int i = 1; i <= m; i++) {
            for (int j = 1; j <= n; j++) {
                if (word1[i - 1] == word2[j - 1]) {
                    // 如果当前字符相同，不需要额外操作
                    dp[i][j] = dp[i - 1][j - 1];
                } else {
                    // 取三种操作的最小值：
                    // 1. 替换：dp[i-1][j-1] + 1
                    // 2. 删除：dp[i-1][j] + 1
                    // 3. 插入：dp[i][j-1] + 1
                    dp[i][j] = min(dp[i - 1][j - 1], min(dp[i - 1][j], dp[i][j - 1])) + 1;
                }
            }
        }
        
        return dp[m][n];
#elif defined(ONE_DIM_DP)
        // 一维动态规划实现（空间优化）
        
        // 如果有一个字符串为空，直接返回另一个字符串的长度
        int word1Len = word1.length(), word2Len = word2.length();
        
        if(!word1Len) return word2Len;
        if(!word2Len) return word1Len;
        
        // 只保留一行 DP 数组，每次迭代更新这一行，+1是为了留出一方是空串的初始化
        std::vector<int> dp(word2Len + 1, 0);
        // 初始化第一行，相当于 word1 为空串的情况
        std::iota(dp.begin(), dp.end(), 0);

        // 逐行填充 dp 数组
        for(int rowVal = 1; rowVal <= word1Len; ++rowVal) {
            // 保存左上角的值（即上一行的 dp[j-1]）
            int topLeft = dp[0];
            // 更新 dp[0]，相当于 word2 为空串的情况
            dp[0] = rowVal;  // 左值
            
            for(int colVal = 1; colVal <= word2Len; ++colVal) {
                // 保存当前 dp[j] 的值，用于下一次迭代
                int top = dp[colVal]; // 上值
                
                if(word1[rowVal - 1] == word2[colVal - 1]) {
                    // 如果当前字符相同，不需要额外操作
                    dp[colVal] = topLeft;
                } else {
                    // 取三种操作的最小值，dp[j - 1]为左值，赋值给dp[j]让其在下一轮是dp[j - 1]左值
                    dp[colVal] = std::min(topLeft, std::min(top, dp[colVal - 1])) + 1;
                }
                
                // 更新 prev 为当前位置的上一行的值
                topLeft = top; 
            }
        }
        
        return dp[word2Len];
#endif
    }
};
// @lc code=end