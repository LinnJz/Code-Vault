int numDistinct(string const &s, string const &t) {
	ptrdiff_t const sLen = s.size(), tLen = t.size();
	if (sLen < tLen) [[unlikely]] return 0;

	auto *dp = reinterpret_cast<uint32_t *>(::alloca((tLen + 1) * sizeof(uint32_t)));
	dp[0] = 1, std::fill(dp + 1, dp + tLen + 1, 0);

	for (ptrdiff_t i = 1; i <= sLen; ++i) {
		// 最多能够匹配t大小的字符，因为我们要找的是s中t的个数
		for (ptrdiff_t j = std::min(i, tLen); j >= std::max<ptrdiff_t>(tLen - sLen + i , 1); --j) {
			if (s[i - 1] == t[j - 1]) dp[j] += dp[j - 1];
		}
	}
	return dp[tLen];
}
/*
 * @lc app=leetcode.cn id=115 lang=cpp
 *
 * [115] 不同的子序列
 */

// @lc code=start
class Solution {
public:
	// dp[i][j] 含义 以 i-1 为结尾的s中 有 以 j-1 为结尾的t的个数为
	// 操作只有删除s字符不删除t，因为求s中t的个数
	// s: bagg ，t: bag
    int numDistinct(string s, string t) {
        int sLen = s.size(), tLen = t.size();
        if (sLen < tLen) return 0;
        
        // 使用一维数组，大小为 tLen + 1
        std::vector<std::size_t> dp(tLen + 1, 0); // + 1 
        dp[0] = 1;  // 空字符串是任何字符串的子序列
        /*
    1. std::min(i, tLen)
    含义：当前外层循环到s的前i个字符时，最多只能匹配t的前min(i, tLen)个字符
    原因：不能用s中更少的字符去匹配t中更多的字符
    举例：如果i=3（处理了s的前3个字符），那么最多只能匹配t的前3个字符
    2. std::max(tLen - sLen + i, 1)
    这是关键剪枝逻辑，含义是：当前需要匹配的t的最小起始位置
    推导过程：
        剩余字符数分析：
            s中还有 sLen - i 个字符未处理

            t中还有 tLen - j 个字符需要匹配（当前要匹配t[j]）

        可行性条件：
        要想成功匹配，必须满足：s中剩余字符数 ≥ t中剩余字符数
        sLen - i ≥ tLen - j

        整理得：
        j ≥ tLen - sLen + i

    代码中的简化：
        代码中使用了 tLen - sLen + i，因为内层循环是 j--（从大到小），所以当 j < tLen - sLen + i 时，剩余的字符肯定不够完成匹配。
    与1取最大值：
        避免j的下界小于1（因为j=0表示空字符串，已经单独处理）
    dp[0] = 1 表示空字符串的匹配情况
    
    
    举例说明：
    假设 s = "rabbbit" (长度7)，t = "rabbit" (长度6)
    第一次循环 (i=1)：
        std::min(1, 6) = 1
        std::max(6-7+1, 1) = std::max(0, 1) = 1
        只计算 j=1（匹配第一个字符）

    第三次循环 (i=3)：
        std::min(3, 6) = 3
        std::max(6-7+3, 1) = std::max(2, 1) = 2
        计算 j=3, 2（跳过不可能的情况）

    为什么能这样剪枝？
    这种剪枝基于一个贪心思想：
        如果s中剩余的字符数 < t中还需要匹配的字符数，那么肯定无法完成匹配
        因此，可以提前跳过这些不可能的情况
        */
        for (int i = 1; i <= sLen; ++i) {
			// 只要是从左上和上方且使用滚动数组的，一定
            // 从后往前遍历，避免覆盖需要的数据
            #pragma clang loop unroll_count(8)
            // 内层循环从min(i, tLen)到1，减少不必要的计算，max确保剩余字符足够完成匹配
            for (int j = std::min(i, tLen); j >= std::max(tLen - sLen + i, 1); --j) {
                if (s[i - 1] == t[j - 1]) {
                    dp[j] = dp[j] + dp[j - 1]; // 取决于左上方和上方
                }
                // 不相等时，取决于上方，dp[j] 保持不变（相当于二维数组中的 dp[i-1][j]）
            }
        }
        return dp.back();
    }
};
// @lc code=end
    int numDistinct(string s, string t) {
        int s1Len = s.size(), s2Len = t.size();
        if (s1Len < s2Len) return 0;

        std::vector<std::vector<std::size_t>> dp(s1Len + 1, std::vector<std::size_t>(s2Len + 1, 0));
        for (int i = 0; i < s.size(); i++) dp[i][0] = 1;

        for(int i = 1; i <= s1Len; ++i) {
            for(int j = 1; j <= s2Len; ++j) {
                if (s[i - 1] == t[j - 1]) dp[i][j] = dp[i - 1][j - 1] + dp[i - 1][j];
                else dp[i][j] = dp[i - 1][j];
            }
        }
        return dp.back().back();
    }

    int numDistinct(string s, string t) {
        int s1Len = s.size(), s2Len = t.size();
        if (s1Len < s2Len) return 0;

        // 使用二维向量更清晰，或者正确计算一维索引
        std::vector<std::size_t> dp((s1Len + 1) * (s2Len + 1), 0);
        
        // 初始化：当t为空字符串时，s的任何子序列都包含1个空字符串
        #pragma clang loop unroll_count(8)
        for (int i = 0; i <= s1Len; ++i) {
            dp[i * (s2Len + 1)] = 1;  // dp[i][0] = 1
        }
        
        for(int i = 1; i <= s1Len; ++i) {
            #pragma clang loop unroll_count(4)
            for(int j = 1; j <= s2Len; ++j) {
                // 正确的索引计算：i * 列数 + j
                int idx = i * (s2Len + 1) + j;
                int prevIdx1 = (i - 1) * (s2Len + 1) + j;      // dp[i-1][j]
                int prevIdx2 = (i - 1) * (s2Len + 1) + (j - 1); // dp[i-1][j-1]
                
                if (s[i - 1] == t[j - 1]) {
                    dp[idx] = dp[prevIdx2] + dp[prevIdx1];
                } else {
                    dp[idx] = dp[prevIdx1];
                }
            }
        }
        
        return dp.back();//dp[s1Len * (s2Len + 1) + s2Len];  // dp[s1Len][s2Len]
    }
