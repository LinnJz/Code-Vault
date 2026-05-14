/*
 * @lc app=leetcode.cn id=104 lang=cpp
 *
 * [104] 二叉树的最大深度
 */

// 三种情况到达curr，左右父亲到达，Morris设置的左孩子的最右节点的right 指向curr

// 左右孩子向下，++preLevel
// curr 指向 Morris设置的左孩子的最右节点的right，则curr的depth 等于 最右节点的depth - curr到达最右节点的rightLen它们的路径长度
// 第二次来到子树的根的时候，能够判断 mostRight是不是叶子节点

class Solution {
public:
    int maxDepth(TreeNode* root) {
        if (!root) return 0;

        int currDepth = 0, maxDepth = 0;
        TreeNode *curr = root;
        while (curr) {
            if (TreeNode *predecessor = curr->left; predecessor) {
                // 找到当前节点在中序遍历下的前驱节点
                int rightPathDepth = 1;
				
                // 寻找左子树的最右节点
                while (predecessor->right && predecessor->right != curr)
                    predecessor = predecessor->right, ++rightPathDepth;
                
                if (predecessor->right) {
                    // 第二次访问，断开临时链接
                    predecessor->right = nullptr;
                    // 调整深度并转向右子树
                    currDepth -= rightPathDepth;
                    curr = curr->right;
                }
                else {
                    // 第一次访问，建立临时链接
                    predecessor->right = curr;
                    if (++currDepth > maxDepth) maxDepth = currDepth;
                    curr = curr->left;
                }
            }
            else {
				// 没有左子树，直接转向右子树
                if (++currDepth > maxDepth) maxDepth = currDepth;
                curr = curr->right;
            }
        }
        return maxDepth;
    }
};
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
    int maxDepth(TreeNode* root) {
       if (root == nullptr) return 0;
       return max(maxDepth(root->left), maxDepth(root->right)) + 1;
    }
};
// @lc code=end

