// 类似题：99. 恢复二叉搜索树
bool isValidBST(TreeNode* root) {
	TreeNode *curr = root, *prev = nullptr;  // 用于记录中序遍历的前一个节点
	bool result = true;
	while (curr) {
		if (TreeNode *predecessor = curr->left; predecessor) {
			while (predecessor->right && predecessor->right != curr) {
				predecessor = predecessor->right;
			}

			if (predecessor->right) {
				predecessor->right = nullptr;
				if (result && prev && prev->val >= curr->val) { // result &&短路径优化
					result = false;
				}
				prev = curr;
				curr = curr->right;
			}
			else {
				predecessor->right = curr;
				curr = curr->left;
			}
		}
		else {
			if (result && prev && prev->val >= curr->val) {
				result = false;
			}
			prev = curr;
			curr = curr->right;
		}
	}

	return result;
}

/*
 * @lc app=leetcode.cn id=98 lang=cpp
 *
 * [98] 验证二叉搜索树
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
    bool isValidBST(TreeNode* root) {
        return helper(root, LONG_MIN, LONG_MAX);
    }
    
private:
    bool helper(TreeNode* node, long lower, long upper) {
        if (!node) return true;
        // 当前节点值必须严格大于下限，严格小于上限
        if (node->val <= lower || node->val >= upper) return false;
        // 左子树的上限是当前节点值，右子树的下限是当前节点值
        return helper(node->left, lower, node->val) && 
               helper(node->right, node->val, upper);
    }
};
// @lc code=end

