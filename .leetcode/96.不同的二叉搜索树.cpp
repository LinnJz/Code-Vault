#include <vector>
/*
 * @lc app=leetcode.cn id=96 lang=cpp
 *
 * [96] 不同的二叉搜索树
 */
namespace {
    constexpr size_t catalan(int n) noexcept {
        size_t c = 1;
        for (int k = 0; k < n; ++k) {
            c = c * 2 * (2 * k + 1) / (k + 2);
        }
        return c;
    }
    template<size_t... I>
    constexpr auto make_catalan_arr(std::index_sequence<I...>) noexcept {
        return std::array{ catalan(I)... };
    }
    constexpr auto catalan_cache = make_catalan_arr(std::make_index_sequence<20>{});
}
size_t catalan(size_t n) {
	double val = std::lgamma(2 * n + 1) - 2 * std::lgamma(n + 1);
	return std::exp(val + 1e-10) / (n + 1.0);
}
// @lc code=start
class Solution {
public:
    std::size_t numTrees(std::size_t n) {
        int *dp = reinterpret_cast<int *>(::alloca((n + 1) * sizeof(int))); // + 1是处理0
        dp[0] = dp[1] = 1;
        std::fill(dp + 2, dp + n + 1, 0);

		// 左右子树的乘积和是种类（不同组合）
        for (int i = 2; i <= n; ++i) {
            for (int j = 0; j < i; ++j) {
                dp[i] += dp[j] * dp[i - j - 1]; // -1根节点占用一个节点
            }
        }
        return dp[n];
    }
};
// @lc code=end
inline constexpr auto [[nodiscard]] catalan(std::size_t n) -> std::size_t {
    if (n == 0) return 1;

    std::size_t result = 1;
    // 使用迭代公式: C(n) = (2n)! / (n!(n+1)!)
    // 等价于: C(n) = ∏(i=1 to n) (4i-2)/(i+1)
    for (std::size_t i = 1; i <= n; ++i) {
    	result = result * (4 * i - 2) / (i + 1);
    }

    return result;
}
inline constexpr auto [[nodiscard]] catalan(std::size_t n) -> std::size_t {
    if (n == 0) return 1;

    std::size_t result = 0;
    for (std::size_t i = 0; i < n; ++i) {
        result += catalan(i) * catalan(n - 1 - i);
    }
    return result;
}
