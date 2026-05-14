
/*
 * @lc app=leetcode.cn id=22 lang=cpp
 *
 * [22] 括号生成
 */

// @lc code=start
#include <vector>
#include <string>
using namespace std;

// 定义使用的解法
#define BACKTRACKING
//#define DYNAMIC_PROGRAMMING
namespace {
    constexpr uint32_t catalan(int n) noexcept {
        uint32_t c = 1;
        for (int k = 0; k < n; ++k) {
            c = c * 2 * (2 * k + 1) / (k + 2);
        }
        return c;
    }
    template<size_t... I>
    constexpr auto make_catalan_arr(std::index_sequence<I...>) noexcept {
        return std::array{ catalan(I)... };
    }
    constexpr auto catalan_cache = make_catalan_arr(std::make_index_sequence<9>{});
}
class Solution {
public:
    std::size_t catalan_lgamma(std::size_t n) {
        if (n == 0) return 1;
        double log_comb = std::lgamma(2 * n + 1) - 2 * std::lgamma(n + 1);
        return std::exp(log_comb + 1e-14) / (n + 1.0);
    }
    vector<string> generateParenthesis(int n) {
#ifdef BACKTRACKING
        // 回溯法解决括号生成问题
        std::vector<std::string> result; // 存储所有有效的括号组合
        result.reserve(catalan_lgamma(n));
        std::string currSet; // 当前正在构建的括号组合
        currSet.reserve(n * 2);
        
        // 调用回溯函数生成所有组合
        backtrack(result, currSet, 0, 0, n);
        
        return result;
#elif defined(DYNAMIC_PROGRAMMING)
        // 括号的个数符合卡特兰数，既然可以使用动态规划计算卡特兰数那么也可以使用动态规划得到括号  
        
        // dp[i]表示i对括号的所有有效组合
        std::vector<std::vector<std::string>> dp(n + 1);
        dp[0] = {""}, dp[1] = {"()"}; // 0对括号只有一种情况，即空字符串

        for(int i = 2; i <= n; ++i) {
            std::vector<std::string> current; current.reserve(catalan_log(i));
            // 对于i对括号，我们可以将其分解为j对括号和i-j-1对括号的组合
            for(int j = 0; j < i; ++j) {
                // 遍历dp[j]中的每个组合
                #pragma GCC unroll 4
                for (const string& inner : dp[j]) {
                    // 遍历dp[i-j-1]中的每个组合
                    #pragma GCC unroll 4
                    for (const string& outer : dp[i - 1 - j]) {
                        // 将左边的组合、一对括号和右边的组合拼接起来
                        // 形式为: (left)right
                        current.emplace_back("(" + inner + ")" + outer);
                    }
                }
            }
            dp[i] = std::move(current);
        }
        return dp.back();
#endif
    }

private:
#ifdef BACKTRACKING
    // 回溯函数
    // result: 存储结果的数组
    // current: 当前构建的字符串
    // openCount: 已使用的左括号数量
    // closeCount: 已使用的右括号数量
    // n: 括号对数
    void backtrack(vector<string>& result, string& current, int openCount, int closeCount, int n) {
        // 如果当前字符串长度等于2n，说明已经使用了n对括号，将其加入结果
        if (current.length() == n * 2) {
            result.push_back(current);
            return;
        }
        
        // 如果左括号数量小于n，可以添加左括号
        if (openCount < n) {
            current.push_back('(');
            backtrack(result, current, openCount + 1, closeCount, n);
            current.pop_back();  // 回溯，移除刚添加的左括号
        }
        
        // 如果右括号数量小于左括号数量，可以添加右括号
        if (closeCount < openCount) {
            current.push_back(')');
            backtrack(result, current, openCount, closeCount + 1, n);
            current.pop_back();  // 回溯，移除刚添加的右括号
        }
    }
#endif
};
// @lc code=end