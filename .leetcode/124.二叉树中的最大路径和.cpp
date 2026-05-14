/*
 * @lc app=leetcode.cn id=124 lang=cpp
 *
 * [124] 二叉树中的最大路径和
 */

// @lc code=start
/**
 * Definition for a binary tree node.
 * struct TreeNode {
 *     int val;
 *     TreeNode *left;
 *     TreeNode *right;
 *     TreeNode() : val(0), left(nullptr), right(nullptr) {}
 *     TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
 *     TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
 * };
 */
class Solution {
public:
/*
- 在数组中，我们寻找连续子数组的最大和
- 在二叉树中，我们寻找任意两个节点之间路径的最大和
关键思路 ：
- 使用递归后序遍历二叉树
- 对于每个节点，计算以该节点为根的子树中的最大路径和
- 同时维护一个全局变量来记录所有可能路径中的最大和
maxGain 函数 ：
- 计算从当前节点出发的最大贡献值（向上、向左或向右的一条路径）
- 如果子树的贡献为负，则不选择该子树（贡献值取0）
- 对于每个节点，考虑以该节点为顶点的路径（左子树-当前节点-右子树）
- 更新全局最大路径和
- 返回当前节点能为父节点提供的最大贡献（只能选择一条路径）
*/
    int maxPathSum(TreeNode* root) {
        int maxSum = INT_MIN; // 初始化全局最大路径和为最小整数
        maxGain(root, maxSum);
        return maxSum;
    }
    
private:
    // 计算从当前节点出发的最大路径和（只能选择一条路径：向上、向左或向右）
    int maxGain(TreeNode* node, int& maxSum) {
        if (!node) return 0; // 空节点贡献为0
        
		/*
		左右子树的路径和只要比0小，那么不管当前节点的值是-，0，+，加上左右子树的最大负数，路径仍旧变小
		所以需要和0取最大
		*/
		
        // 递归计算左右子树的最大贡献值
        // 只有在最大贡献值大于0时，才会选取对应子节点
        int leftGain = max(maxGain(node->left, maxSum), 0);
        int rightGain = max(maxGain(node->right, maxSum), 0);
        
        // 当前节点的最大路径和 = 当前节点值 + 左子树贡献 + 右子树贡献
        // 这是一个完整的路径，更新全局最大值
        int priceNewPath = node->val + leftGain + rightGain;
        maxSum = max(maxSum, priceNewPath);
        
        // 返回当前节点能为父节点提供的最大贡献
        // 注意：对于父节点，只能选择一条路径（向左或向右）
        return node->val + max(leftGain, rightGain);
    }
};
// @lc code=end

