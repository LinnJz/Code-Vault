#include <vector>
#include <string>
#include <climits>
/*
状态转移的直观理解
text

字符串: a a b
索引:  0 1 2
dp:   -1 0 0 1

解释：
- dp[1]=0: "a" 不需要分割
- dp[2]=0: "aa" 不需要分割（整个是回文）
- dp[3]=1: "aab" 需要1次分割 → ["aa", "b"]

关键点总结

    dp[i] 含义：前i个字符的最小分割次数

    状态转移：当发现新的回文子串时，尝试用更小的分割次数更新dp值

    dp[0] = -1：技巧性设置，使得整个字符串是回文时分割次数为0

    比较条件：只有当找到更小的分割次数时才更新
*/
class Solution { 
public: 
    int minCut(std::string const& s) {
		ptrdiff_t const size = s.size();
        // dp[i]表示s[0...i-1]的最小分割次数
        auto *dp = reinterpret_cast<int *>(::alloca((size + 1) * sizeof(int)));
		// 空字符串的分割次数为-1，这样dp[i] = dp[j] + 1时，当j=0且整个子串是回文时，分割次数为0
        dp[0] = -1, std::fill(dp + 1, dp + size + 1, INT_MAX);
        
        for (ptrdiff_t center = 0; center < size; ++center) {
            // 奇数长度回文扩展
            //如果从位置 i-j 到 i+j 是回文，那么到位置 i+j+1 的最小分割次数可以更新为 dp[i - j] + 1
            #pragma clang loop unroll_count(8)
            for (int left = center, right = center; left >= 0 && right < size && s[left] == s[right]; --left, ++right) {
                if (int val = dp[left] + 1; val < dp[right + 1]) dp[right + 1] = val;
            }
            // 偶数长度回文扩展
            //如果从位置 i-j+1 到 i+j 是回文，那么到位置 i+j+1 的最小分割次数可以更新为 dp[i - j + 1] + 1
            #pragma clang loop unroll_count(8)
            for (ptrdiff_t left = center, right = center + 1; left >= 0 && right < size && s[left] == s[right]; --left, ++right) {
                if (int val = dp[left] + 1; val < dp[right + 1]) dp[right + 1] = val;
            }
        }
        return dp[size];
    }
};
/* 
 * @lc app=leetcode.cn id=132 lang=cpp 
 * 
 * [132] 分割回文串 II 
 */ 

// @lc code=start 

class Solution { 
public: 
     int minCut(std::string s) {
        std::size_t str_len = s.size();
        
        std::vector<uint8_t> isPalindrome(str_len * str_len, false);
        #pragma GCC unroll 8
        for (std::size_t i = 0; i < str_len; ++i) {
            for (std::size_t j = 0; j <= i; ++j) {
                if (s[i] == s[j]) {
                    isPalindrome[j * str_len + i] = (i - j <= 2) ? true : isPalindrome[(j + 1) * str_len + (i - 1)];
                }
            }
        }
        
        // dp[i]表示s[0...i]的最小分割次数
        std::vector<int> dp(str_len, 0);
        // 初始化：如果s[0..i]本身是回文，分割次数为0，否则初始化为最大可能值
        #pragma GCC unroll 8
        for (std::size_t i = 0; i < str_len; ++i) {
            // 如果s[0...i]本身就是回文串，不需要分割
            if (isPalindrome[i]) {
                dp[i] = 0;
                continue;
            }
            
            dp[i] = i; // 最多分割i次（每个字符都分割）
            // 枚举最后一个分割点j
            for (std::size_t j = 0; j < i; ++j) {
                // 如果s[j+1...i]是回文串，则可以在j处进行分割
                if (isPalindrome[(j + 1) * str_len + i] && (dp[j] + 1) < dp[i]) { // 如果s[j+1..i]是回文
                    dp[i] = dp[j] + 1;
                }
            }
        }
        
        return dp.back();
    }
}; 
// @lc code=end 