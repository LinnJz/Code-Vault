/*
 * @lc app=leetcode.cn id=188 lang=cpp
 *
 * [188] 买卖股票的最佳时机 IV
 */

// @lc code=start
class Solution {
public:
    int maxProfit(int k, vector<int>& prices) {
        int size = (k + 1) * sizeof(int);
        int *buy = reinterpret_cast<int *>(::alloca(size));
		// memset只能填充0和-1，否则造成数据不匹配，使用fill填充-prices[0]
		/*
		原理：按字节填充
		memset 的原型是 void *memset(void *s, int c, size_t n);。它的核心行为是：

			将 c（一个 int 值）强制转换为 unsigned char 类型，只保留其最低的 8 个二进制位（一个字节）。

			然后，从地址 s 开始，向后的 n 个字节，每个字节都被赋值为这个转换后的字节值。

		关键点：它不管指针 s 是 int* 还是 char*，它就朴实地一个字节一个字节地填充。
		为什么 0 和 -1 是特例？

		1. 填充 0：完美工作

			(unsigned char)0 的二进制是 00000000。

			连续填充 n 个字节的 00000000，无论你将这些字节解释为 int (4字节)、double (8字节) 还是任何结构体，其每一位都是 0。所以逻辑值和内存值都是 0。这是通用的、期望的结果。

		2. 填充 -1：意外生效（仅限于整数类型）

		这得益于补码表示法。

			在绝大多数现代计算机中，负数使用补码存储。

			-1 的补码表示，在所有整数类型（char, short, int, long long）中，都是所有位为 1。

				例如，32位 int 的 -1：11111111 11111111 11111111 11111111。

				8位 signed char 的 -1：11111111。

			(unsigned char)-1 的二进制正好是 11111111。

			用 memset 把每个字节都设为 11111111 后，当你把这整块内存解释为一个 int、long 等有符号整数时，它又恰好是那个整型的 -1 的补码表示。

			注意：对于浮点数（如 float、double），全 1 位模式是 NaN（非数）或一个特殊值，不是 -1.0。所以对非整数类型无效。
		*/
        std::fill(buy, buy + k + 1, -prices[0]);

        int *sell = reinterpret_cast<int *>(::alloca(size));
        ::memset(sell, 0, size);        

        for (int i = 1; i < prices.size(); ++i) {
			#pragma clang loop unroll_count(4)
            for (int j = 1; j <= k; ++j) {
                if (int val = sell[j - 1] - prices[i]; val > buy[j]) buy[j] = val;
                if (int val = buy[j] + prices[i]; val > sell[j]) sell[j] = val;
            }
        }

        return sell[k];
    }
};
// @lc code=end

