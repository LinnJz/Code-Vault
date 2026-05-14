
#include <vector>
#include <string>
/* 
 * @lc app=leetcode.cn id=131 lang=cpp 
 * 
 * [131] 分割回文串 
 */ 

// @lc code=start 
/*
    dp 预处理计算空间有一定开销，性能略差于直接的方法，不过差距不是很大
    如果使用二维数组那么另当别论
    https://quick-bench.com/q/UDShpDQEri7EgSj6_wWSVEfoYR4
*/
class Solution {
public:
	// 决策树，回溯枚举所有分割点
    std::vector<std::vector<std::string>> partition(std::string s) {
        std::vector<std::vector<std::string>> result;   result.reserve(4);
        std::vector<std::string> currSet;   currSet.reserve(s.length());
        
        backtrack(s, result, currSet, s.begin());
        return result;
    }

    void backtrack(const auto& s, auto& result, auto& currSet, auto start) {
		// 分割点结束
        if (start == s.end()) {
            result.push_back(currSet);
            return;
        }
        
        for(auto it = start; it != s.end(); ++it) {
            if(isPalindrome(start, std::next(it))) {
                currSet.emplace_back(start, std::next(it));
                backtrack(s, result, currSet, std::next(it));
                currSet.pop_back();
            }
        }
    }

    template<typename RanIter>
    bool isPalindrome(RanIter begin, RanIter end) {
        #pragma GCC unroll 4
        for(--end ; begin < end; ++begin, --end) {
            if(*begin != *end) return false;
        }
        return true;
    }
};

class Solution {
public:
    std::vector<std::vector<std::string>> partition(std::string s) {
        std::size_t str_len = s.size();
        
        // 预处理所有子串是否为回文
        std::vector<uint8_t> dp(str_len * str_len, false);
        #pragma GCC unroll 4
        for (std::size_t i = 0; i < str_len; ++i) {
            for (std::size_t j = 0; j <= i; ++j) {
                if (s[i] != s[j]) continue;
                // 单个字符或两个相同字符
                dp[j * str_len + i] = (i - j <= 2) ? true : dp[(j + 1) * str_len + (i - 1)];
            }
        }
        std::vector<std::vector<std::string>> result;   result.reserve(4);
        std::vector<std::string> currSet;   currSet.reserve(str_len);
        // 回溯搜索所有可能的分割方案
        backtrack(s, dp, result, currSet, 0, str_len);
        return result;
    }

    void backtrack(const auto& s, const auto& dp, auto& result, auto& currSet, std::size_t start, std::size_t str_len) {
        if (start == str_len) {
            result.push_back(currSet);
            return;
        }

        std::size_t base = str_len * start;
        // 尝试分割点
        for(std::size_t end = start; end < str_len; ++end) {
            // 如果s[start...i]是回文串，则可以在i后面进行分割
            if(dp[base + end]) {
                // 将当前回文子串加入分割方案
                currSet.emplace_back(s.begin() + start, s.begin() + end + 1);
                backtrack(s, dp, result, currSet, end + 1, str_len);
                currSet.pop_back();
            }
        }
    }

};
// @lc code=end 