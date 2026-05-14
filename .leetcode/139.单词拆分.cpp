#include "header.h"
/*
 * @lc app=leetcode.cn id=139 lang=cpp
 *
 * [139] 单词拆分
 */
///< 本题本质：树形结构，枚举，dp优化去除不必要的检查

// 暴力一：模式串匹配，匹配成功，擦除s对应区域，直到s是empty返回true，否则false
// 暴力二：穷举字典内词的组合，结束条件拼接长度不能大于s长度，长度等于比较相等返回true
// 以此思考dp：
/*
### 从暴力一优化为动态规划
暴力一是模式串匹配，匹配成功后擦除对应区域，直到字符串为空。这种方法可以优化为：

1. 定义状态： dp[i] 表示字符串s的前i个字符是否可以被拆分成wordDict中的单词
2. 状态转移方程：对于每个位置j (0 ≤ j < i)，如果 dp[j] == true 且s[j...i-1]在wordDict中，则 dp[i] = true
3. 初始状态： dp[0] = true （空字符串可以被拆分）
### 从暴力二优化为动态规划
暴力二是穷举字典内词的组合。这种方法同样可以优化为上述动态规划方法，因为我们关心的是能否拆分，而不是具体如何拆分。

### DP数组长度确定
DP数组的长度应该是 s.length() + 1 ，因为我们需要考虑空字符串的情况（dp[0]）。
*/
// @lc code=start
class Solution {
public:
    bool wordBreak(std::string s, std::vector<std::string> wordDict) {
        std::unordered_set<std::string_view> wordSet; wordSet.reserve(wordDict.size());
        ptrdiff_t maxWordSize = 0, size = s.size();
        for (auto const &str : wordDict) {
            wordSet.emplace(str.data(), str.size());
			// 计算wordDict中最长单词的长度

            if (str.size() > maxWordSize) maxWordSize = str.size();
        }
		/*
		——————
		  i  j
		如果 i j 在字符串内，且dp[i] 是true，说明 从0到j是true
		*/
		
        // 创建dp数组，dp[i]表示s的前i个字符是否可以被拆分
        bool *dp = reinterpret_cast<bool *>(::alloca(size + 1));
        dp[0] = true, std::fill(dp + 1, dp + size + 1, false); // dp[0]必须因为不是true没法更新
		// 背包
		for (ptrdiff_t j = 1; j <= size; ++j) {
            // 只检查不超过maxLength长度的子串
            // 如果 j - i > maxLength，则 s[i..j-1] 一定不是词典中的单词，检查它毫无意义
            // 并且可以通过dp[i] 快速检查先前区间是否是true
			// 物品
            for (ptrdiff_t i = std::max(0z, j - maxWordSize); i < j; ++i) {
                // 如果s的前j个字符可以被拆分，且s[j...i-1]在wordDict中
                if (dp[i] && wordSet.contains(std::string_view(s.data() + i, j - i))) {
                    dp[j] = true;
                }
            }
        }
        return dp.back();
    }
};
// @lc code=end