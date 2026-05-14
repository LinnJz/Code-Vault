// 头插法易懂
ListNode* swapPairs(ListNode* head) {
    if (!head || !head->next) return head;

    ListNode dummy(0, nullptr);
    ListNode *dummyPtr = &dummy, *headPtr = head, *dummyTail = nullptr;
    int round = -1;
    while(headPtr) {
        ++round;
        ListNode* temp = headPtr;
        headPtr = headPtr->next;
        temp->next = dummyPtr->next;
        dummyPtr->next = temp;
        if (round & 1) dummyPtr = dummyTail;
        else dummyTail = dummyPtr->next;
    }
    return dummy.next;
}

 /*
 ptr prev next temp 
-1  1    2    nullptr(3)

-1->2->1->3->4

1   3    4    nullptr(nullptr)
-1>2->1->4->3->nullptr

3   nullptr
 */
class Solution {
public:
    // 双指针交换前后
    ListNode* swapPairs(ListNode* head) {
        if(!head || !head->next) return head;

        ListNode dummy(-1, nullptr);

        ListNode* ptr = &dummy;
        ListNode* prev = head;
        ListNode* next = head->next;

        while(next) {
            ptr->next = next;
            prev->next = next->next;
            next->next = prev;

            ptr = prev;
            prev = prev->next;
            if(!prev) break;
            next = prev->next;
        }

        return dummy.next;
    }
};
/*
 * @lc app=leetcode.cn id=24 lang=cpp
 *
 * [24] 两两交换链表中的节点
 */

// @lc code=start
/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     ListNode *next;
 *     ListNode() : val(0), next(nullptr) {}
 *     ListNode(int x) : val(x), next(nullptr) {}
 *     ListNode(int x, ListNode *next) : val(x), next(next) {}
 * };
 */
class Solution {
public:
    ListNode* swapPairs(ListNode* head) {
         // 创建哑节点简化头节点处理
        ListNode dummy(0);
        dummy.next = head;
        ListNode* prev = &dummy; // prev始终指向待交换节点的前驱
        
        while (prev->next && prev->next->next) {
            // 初始化当前要交换的两个节点
            ListNode* curr = prev->next;
            ListNode* next = curr->next;
            // null  1      2       3
            // pre   curr   next    
            // null -> 2
            // 1 -> 3
            // 2 -> 1
            // null 2 1 3
            // 交换节点
            prev->next = next;         // Step1: 前驱指向第二个节点
            curr->next = next->next;   // Step2: 第一个节点指向后续链表
            next->next = curr;         // Step3: 第二个节点指向第一个节点
            
            // 移动prev到下一组的前驱位置（即当前的curr）
            prev = curr;
        }
        
        return dummy.next; // 返回新链表的头节点
    }
};
// @lc code=end

