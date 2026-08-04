class Solution {
public:
    // 把每一个下标 的 单元格 都看作一个木桶，下标的值代表木桶底部厚度，木桶有左右边界，是否能够装水取决于左右边界是否大于木桶底部厚度
    // 双指针遍历左右，记录左右最大高度，并且根据当前的左右高度进行计算，如果当前左高度小于右高度，说明木桶容量取决于左边最大，反之
    int trap(vector<int>& height) {
        int result{ 0 }, left{ 0 }, right( height.size() - 1 );
        int leftHeightMax{ 0 }, rightHeightMax{ 0 };
        
        while(left < right) {
            int currLeftHeight = height[left], currRightHeight = height[right];

            if (leftHeightMax < currLeftHeight) leftHeightMax = currLeftHeight;
            if (rightHeightMax < currRightHeight) rightHeightMax = currRightHeight;
            // 为什么能够 leftHeightMax - currLeftHeight
			// 因为双指针向中间靠拢，我们通过计算两侧最大值，然后根据当前的双指针比较
			// 即使中间部分未知高度（木桶一侧的高currHeight，但是不知道另一侧的高未遍历），但是因为我们知道了当前全局最高的木板
			// 所以可以贪心，最小先收集当前的水，然后不断向中间这样做，每次收集一小部分最终汇总
			
			// 全局最大总是有当前的curr更新而来，所以 leftHeightMax总是大于currLeftHeight，rightHeightMax总是大于currRightHeight
			// 当currLeftHeight < currRightHeight时，确定currRightHeight木板，然后根据leftHeightMax计算当前最小收集水
            if (currLeftHeight < currRightHeight) {
                result += leftHeightMax - currLeftHeight;
                ++left;
            }
            else {
                result += rightHeightMax - currRightHeight;
                --right;
            }
        }
        return result;
    }
};
/*
 * @lc app=leetcode.cn id=42 lang=cpp
 *
 * [42] 接雨水
 */
#include <stack>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
// @lc code=start
class Solution {
public:
    int trap(vector<int>& height) {
        if (height.size() < 3) return 0;

        int blackRectAreaSum{ 0 }, whiteRectAreaSum{ 0 }, maxHeight{ 0 };
        auto left = height.begin(), right = height.end() - 1, maxHeightPos = height.begin();
        for (; left < right; ++left, --right)
        {
            if (*left > maxHeight && *left >= *right) {
                maxHeight = *left;
                maxHeightPos = left;
            }
            else if (*right > maxHeight && *right > *left) {
                maxHeight = *right;
                maxHeightPos = right;
            }
            blackRectAreaSum += *left + *right;
        }
        if (left == right) { // height.size() & 1
            if (*left > maxHeight) maxHeight = *left, maxHeightPos = left;
            blackRectAreaSum += *left;
        }

        left = height.begin(), right = height.end() - 1;
        int currVal = *left;
        while (left <= maxHeightPos)
        {
            if (currVal < *left)
            {
                // 高 * 低
                whiteRectAreaSum += (*left - currVal) * std::distance(height.begin(), left);
                // 更新当前 黑色矩形 进入下一轮计算白色矩形和循环
                currVal = *left;
            }
            ++left;
        }
        currVal = *right;
        while (right >= maxHeightPos)
        {
            if (currVal < *right)
            {
                whiteRectAreaSum += (*right - currVal) * std::distance(right, height.end() - 1);
                currVal = *right;
            }
            --right;
        }
        // [0,2,0] 总 3 * 2 - 黑2 - 白2*2
        return static_cast<int>(height.size()) * maxHeight - blackRectAreaSum - whiteRectAreaSum;
    }
};
 // @lc code=end