/*
 * @lc app=leetcode.cn id=396 lang=cpp
 *
 * [396] 旋转函数
 */
 
 /*
### 原理说明

- **初始加权和**：$$P_0 = \sum_{i=0}^{n-1} i \cdot a_i$$，总元素和 $$T = \sum_{i=0}^{n-1} a_i$$。

- **循环右移后的递推**：若当前序列最后一个元素为 \(x\)，则右移一次后的新加权和为：
  $$
  P_{\text{next}} = P_{\text{current}} + T - n \cdot x
  $$
  推导：右移相当于最末元素 \(x\) 的权重从 \(n-1\) 变为 \(0\)，其他每个元素权重增加 \(1\)，因此变化量为 $$- (n-1)x + \sum_{j \neq last} 1 \cdot a_j = -nx + T$$。

- 每次只需常数时间计算，总复杂度 $$O(n)$$，远优于每次重新计算的 $$O(n^2)$$。

### 扩展说明

如果需求是**循环左移**（开头的元素移到末尾），递推公式变为：
$$
P_{\text{left}} = P_{\text{current}} - T + n \cdot x
$$
其中 \(x\) 是当前序列的第一个元素。代码稍作调整即可。
 */

// @lc code=start
class Solution {
public:
    int maxRotateFunction(std::vector<int>& nums) 
    {
        int Size = nums.size() - 1, Total = 0, Weighted_sum = 0;
        #pragma clang loop unroll_count(8) vectorize(enable)
        for (int I = 0; I <= Size; ++I) 
        {
            Total += nums[I];
            Weighted_sum += I * nums[I];
        }
        
        int Max = Weighted_sum;
        #pragma clang loop unroll_count(8)
        for (int I = 0; I < Size; ++I) 
        {
            if ((Weighted_sum = Weighted_sum + Total - nums.size() * nums[Size - I]) > Max)
            {
                Max = Weighted_sum;
            }
        }
        return Max;
    }
};
// @lc code=end

