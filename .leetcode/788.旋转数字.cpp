/*
 * @lc app=leetcode.cn id=788 lang=cpp
 *
 * [788] 旋转数字
 */
/*
根据题目的要求，一个数是好数，当且仅当：

    数中没有出现 3,4,7；

    数中至少出现一次 2 或 5 或 6 或 9；

    对于 0,1,8 则没有要求。
*/
// @lc code=start
class Solution {
public:
    int rotatedDigits(int n) {
        static constexpr int digits[10] { 2, 2, 0, 1, 1, 0, 0, 1, 2, 0 };

        int count = 0;

        #pragma clang loop unroll_count(4)
        for (int i = 1; i <= n; ++i)
        {
            bool diff = false;
            int num = i;
            while (num > 0) // 必须扫描完
            {
                if (int val = digits[num % 10]; val == 1) 
                {
                    goto next_iteration; 不满足直接goto减少一点开销
                }
                else if (val == 0)
                {
                    diff = true;
                }
                num /= 10;
            }
            count += diff;
        next_iteration:
        }
        return count;
    }
};
// @lc code=end

