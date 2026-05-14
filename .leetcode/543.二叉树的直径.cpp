/*
错误的思想，最长路径不一定是根节点的左右子树高度相加，而是每一个节点作为根节点的时候左右子树高度相加的最大值
int diameterOfBinaryTree(TreeNode* root) {
    if (!root) return 0;
    if (!root->left) return height(root->right);
    if (!root->right) return height(root->left);

    return height(root->right) + height(root->left);
}

int height(TreeNode* root){
    if (!root) return 0;

    int lLen = height(root->left);
    int rLen = height(root->right);
    return std::max(lLen, rLen) + 1;
}
*/
/*
 * @lc app=leetcode.cn id=543 lang=cpp
 *
 * [543] 二叉树的直径
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
     // 递归函数：计算子树深度，并更新最大直径
    int depth(TreeNode* node, int& max_diameter) {
        if (!node) return 0;
        
        // 递归计算左右子树深度
        int left_depth = depth(node->left, max_diameter);
        int right_depth = depth(node->right, max_diameter);
        
        // 当前节点的直径 = 左深度 + 右深度
        max_diameter = max(max_diameter, left_depth + right_depth); // 要的是边数而不是节点数，不加1
        
        // 返回当前子树深度（左右最大深度 + 1）
        return max(left_depth, right_depth) + 1;
    }
    int diameterOfBinaryTree(TreeNode* root) {
        int max_diameter = 0; // 维护最大直径值
        depth(root, max_diameter); // 递归计算深度并更新直径
        return max_diameter;
    }
};
// @lc code=end

