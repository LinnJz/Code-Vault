/*
 * @lc app=leetcode.cn id=61 lang=cpp
 *
 * [61] 旋转链表
 */

// @lc code=start
class Solution {
public:
    ListNode* rotateRight(ListNode* head, int k) {
        if (!head || !k) [[unlikely]] return nullptr;

        ListNode *curr = head, *tail = nullptr;
        int length = 0;
        while (curr) tail = curr, curr = curr->next, ++length;
        if (length == (k = k % length)) return head;

        curr = head;
        #pragma clang loop unroll_count(4)
        for (int i = 0, end = length - k - 1; i < end; ++i) {
            curr = curr->next;
        }
        tail->next = head;
        head = curr->next;
        curr->next = nullptr;
        return head;
    }
};
// @lc code=end

