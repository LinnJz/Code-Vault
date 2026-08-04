TreeNode* buildTree(vector<int>& preorder, vector<int>& inorder) {
    if (preorder.empty()) return nullptr;
    
    TreeNode* root = new TreeNode(preorder[0]);
    stack<TreeNode*> stk;
    stk.push(root);
    
    for (int inorderIndex = 0, i = 1; i < preorder.size(); i++) {
        TreeNode* node = stk.top();
        if (node->val != inorder[inorderIndex]) {
            node->left = new TreeNode(preorder[i]);
            stk.push(node->left);
        } else {
            while (!stk.empty() && stk.top()->val == inorder[inorderIndex]) {
                node = stk.top();
                stk.pop();
                inorderIndex++;
            }
            node->right = new TreeNode(preorder[i]);
            stk.push(node->right);
        }
    }
    
    return root;
}

TreeNode* buildTree(vector<int>& preorder, vector<int>& inorder) {
    std::unordered_map<int, int> inorder_map;
    for (int i = 0; i < inorder.size(); i++) {
        inorder_map[inorder[i]] = i;
    }
    return build(preorder, 0, preorder.size() - 1, inorder, 0, inorder.size() - 1, inorder_map);
}

TreeNode* build(vector<int>& preorder, int preStart, int preEnd, 
                vector<int>& inorder, int inStart, int inEnd,
                unordered_map<int, int>& inorder_map) {
    if (preStart > preEnd || inStart > inEnd) return nullptr;

    TreeNode* root = new TreeNode(preorder[preStart]);
    int rootIdx = inorder_map[root->val];
    int leftSubtreeSize = rootIdx - inStart;

    root->left = build(preorder, preStart + 1, preStart + leftSubtreeSize, 
                        inorder, inStart, rootIdx - 1, inorder_map);
    root->right = build(preorder, preStart + leftSubtreeSize + 1, preEnd,
                        inorder, rootIdx + 1, inEnd, inorder_map);

    return root;
}
/*
 * @lc app=leetcode.cn id=105 lang=cpp
 *
 * [105] 从前序与中序遍历序列构造二叉树
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
 /*
 leftSubtreeSize - 1 是用来计算左子树在前序数组中的结束索引的偏移量。

原因：

    前序遍历的结构是：[根节点] + [左子树的前序遍历] + [右子树的前序遍历]。

    当前根节点在 preorder[preStart]，所以左子树的前序遍历从 preStart + 1 开始。

    左子树的节点个数为 leftSubtreeSize（由中序数组计算得出）。

    因此，左子树在前序数组中的区间为 [preStart + 1, preStart + leftSubtreeSize]，其结束索引 = preStart + leftSubtreeSize。

    在代码中，结束索引被写为 preStart + 1 + (leftSubtreeSize - 1)，即 preStart + leftSubtreeSize。这里的 leftSubtreeSize - 1 是从起始索引 preStart + 1 出发，要覆盖 leftSubtreeSize 个元素所需的“最后一项的偏移量”（因为起始偏移为1，加上 leftSubtreeSize-1 正好是第 leftSubtreeSize 个元素）。

所以，- 1 是为了正确计算包含 leftSubtreeSize 个节点的区间的右边界，确保不会多取或少取节点。
 */
class Solution {
public:
    TreeNode* buildTree(vector<int>& preorder, vector<int>& inorder) {
        return build(preorder, 0, preorder.size() - 1, inorder, 0, inorder.size() - 1);
    }
    
private:
    TreeNode* build(vector<int>& pre, int preStart, int preEnd, 
                    vector<int>& in, int inStart, int inEnd) {
        if (preStart > preEnd || inStart > inEnd) return nullptr;
        
        TreeNode* root = new TreeNode(pre[preStart]); // 前序首元素为根
        
        // 在中序中找到根的位置
        int inRoot = inStart;
        while (in[inRoot] != root->val) ++inRoot;
        
        int leftSubtreeSize = inRoot - inStart; // 左子树节点数
        
        // 递归构建左子树：前序左范围[preStart+1, preStart+leftSubtreeSize]
        root->left = build(pre, preStart + 1, preStart + leftSubtreeSize, 
                          in, inStart, inRoot - 1);
        // 递归构建右子树：前序右范围[preStart+leftSubtreeSize+1, preEnd]
        root->right = build(pre, preStart + leftSubtreeSize + 1, preEnd,
                           in, inRoot + 1, inEnd);
        return root;
    }
};
// @lc code=end

