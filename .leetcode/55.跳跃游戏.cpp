/*
 * @lc app=leetcode.cn id=55 lang=cpp
 *
 * [55] 跳跃游戏
 */

// @lc code=start
#include <vector>
#include <stack>
using namespace std;
class Solution {
public:
//贪心算法原理
//维护一个变量 max_reach 表示当前能到达的最远索引。遍历数组时不断更新这个值：
//max_reach = max(当前最远距离, 当前位置 + 可跳跃步数)
    bool canJump(vector<int>& nums) {
        int max_reach = 0;  // 当前能到达的最远位置
        for (int i = 0; i < nums.size(); ++i) { // 不能减一是因为，如果是只有一个元素0，也能到达
            // 如果当前位置超过了最远能到达的位置，说明无法继续
            if (i > max_reach) return false;
            // 更新能到达的最远位置
            max_reach = max(max_reach, i + nums[i]);
            // 如果能到达或超过终点，提前返回
            if (max_reach >= nums.size() - 1) return true;
        }
        return true;  // 遍历完数组时一定能到达终点
    }
};
// @lc code=end
int main(){
    Solution s;
    vector<int> nums = { 3,0,8,2,0,0,1 };
    s.canJump(nums);
    return 0;
}

