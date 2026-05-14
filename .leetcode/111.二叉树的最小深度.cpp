
// BFS 层序遍历
class Solution {
public:
    int minDepth(TreeNode* root) {
        // 处理空树情况
        if (root == nullptr) return 0;
        
        // 使用队列进行BFS（广度优先搜索）
        // 队列中存储待处理的树节点
        std::queue<TreeNode*> q;
        q.push(root);  // 从根节点开始
        
        int depth = 1;  // 当前深度，根节点深度为1
        
        // 当队列不为空时继续处理
        while (!q.empty()) {
            // 记录当前层的节点数量
            int levelSize = q.size();
            
            // 遍历当前层的所有节点
            for (int i = 0; i < levelSize; ++i) {
                // 从队列头部取出一个节点
                TreeNode* node = q.front();
                q.pop();
                
                // 检查当前节点是否为叶子节点
                // 叶子节点：没有左子节点且没有右子节点
                if (node->left == nullptr && node->right == nullptr) {
                    return depth;  // 找到第一个叶子节点，立即返回当前深度
                    // 这是最小深度，因为BFS按层遍历，先遇到的叶子节点深度最小
                }
                
                // 如果当前节点不是叶子节点，将其非空子节点加入队列
                // 左子节点不为空，加入队列
                if (node->left != nullptr) {
                    q.push(node->left);
                }
                // 右子节点不为空，加入队列
                if (node->right != nullptr) {
                    q.push(node->right);
                }
            }
            
            // 完成当前层所有节点的处理，深度加1
            // 准备处理下一层节点
            ++depth;
        }
        
        return depth;  // 理论上不会执行到这里，因为循环中一定会遇到叶子节点
    }
};

// morris 遍历，无递归栈开销，但实际效率最低
class Solution {
public:
    int minDepth(TreeNode* root) {
		if (!root) return 0;

        int currDepth = 0, minDepth = INT_MAX;
        TreeNode *curr = root;
        while (curr) {
            if (TreeNode *predecessor = curr->left; predecessor) {
                int rightPathDepth = 1;
                while (predecessor->right && predecessor->right != curr)
                    predecessor = predecessor->right, ++rightPathDepth;
                
                if (predecessor->right) {
					// 第二次访问，断开临时链接
                    predecessor->right = nullptr;
					// 检查前驱节点是否为叶子节点
                    if (!predecessor->left && minDepth > currDepth) {
                        minDepth = currDepth;
                    }
					// 调整深度并转向右子树
                    currDepth -= rightPathDepth;
                    curr = curr->right;
                }
                else {
					// 第一次访问，建立临时链接
                    predecessor->right = curr;
                    ++currDepth;
                    curr = curr->left;
                }
            }
            else {
				// 检查当前节点是否为叶子节点，隐含curr->left是空，注意minDepth > ++currDepth在前，!curr->right在后，否则短路径优化导致currDepth不能自增
                if (minDepth > ++currDepth && !curr->right) {
                    minDepth = currDepth;
                }
                curr = curr->right;
            }
        }
        return minDepth;
    }
};
/*
 * @lc app=leetcode.cn id=111 lang=cpp
 *
 * [111] 二叉树的最小深度
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
    int minDepth(TreeNode* root) {
        if (!root) return 0;
        
        // 左子树为空，只计算右子树深度
        if (!root->left) return 1 + minDepth(root->right);
        // 右子树为空，只计算左子树深度
        if (!root->right) return 1 + minDepth(root->left);
        
        // 左右子树均存在，取较小值
        return 1 + min(minDepth(root->left), minDepth(root->right));
    }
};
// @lc code=end

