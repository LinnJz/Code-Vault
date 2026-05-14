
// 队列始终维护递减的第几大数字
vector<int> maxSlidingWindow(vector<int>& nums, int k) {
	// 预分配 vector 模拟队列，容量固定为 nums.size()（足够，不会越界）
	int *q = reinterpret_cast<int *>(::alloca(nums.size() * sizeof(int)));
	int head = 0, tail = 0;  // [head, tail) 为有效区间
	
	// 初始化前 k 个元素
	for (int i = 0; i < k; ++i) {
		 // 弹出队尾所有小于当前值的元素
		while (head < tail && nums[i] > q[tail - 1]) --tail;
		q[tail++] = nums[i];
	}
	std::vector<int> result; result.reserve(nums.size() - k + 1);
	result.emplace_back(q[head]); // 第一个窗口最大值

	for (int i = k; i < nums.size(); ++i) {
		// 若窗口左边界值等于队首元素，则队首出队（逻辑删除）
		if (head < tail && nums[i - k] == q[head]) ++head;

		// 维护单调递减队列：弹出队尾所有小于当前值的元素
		while (head < tail && nums[i] > q[tail - 1]) --tail;
		q[tail++] = nums[i];

		result.emplace_back(q[head]);
	}
	return result;
}
// 观察题目给定例子，滑动窗口移动过程
// 1 2 3
//   4 5 6
// 观察它很像一个队列，我们需要做到是 每次移动都头部pop，尾部push，然后求队列最大值
// 队列应该怎么设置，优先队列无法pop正确的位置，因为堆顶是最大值，你要pop首部元素
// 所以使用单调队列始终维护最大值
vector<int> maxSlidingWindow(vector<int>& nums, int k) {
	std::vector<int> result; result.reserve(nums.size() - k + 1);
	std::deque<int> q;

	for (int i = 0; i < nums.size(); ++i) {

		// 如果使用连续内存模拟queue
		// while (head < tail && q[head] < i - k + 1)
		// while (head < tail && nums[q[tail - 1]] < nums[i])
			
		while (!q.empty() && (i - k + 1) > q.front()) q.pop_front(); 
		while (!q.empty() && nums[q.back()] < nums[i]) q.pop_back(); 

		q.emplace_back(i);
		if (i >= k - 1) {
			result.emplace_back(nums[q.front()]);
		}
	}
	return result;
}
vector<int> maxSlidingWindow(vector<int>& nums, int k) {
	std::deque<int> q;
	for (int i = 0; i < k; ++i) {
		while (!q.empty() && nums[i] > q.back()) q.pop_back();
		q.emplace_back(nums[i]);
	}
	std::vector<int> result; result.reserve(nums.size() - k + 1);
	result.emplace_back(q.front());

	for (int i = k; i < nums.size(); ++i) {
		if (!q.empty() && nums[i - k] == q.front()) q.pop_front();
		while (!q.empty() && nums[i] > q.back()) q.pop_back();

		q.emplace_back(nums[i]);

		result.emplace_back(q.front());
	}
	return result;
}
// multiset排序， nlogn级别
class Solution {
public:
    std::vector<int> maxSlidingWindow(std::vector<int>& nums, int k) {
        if (nums.empty() || k <= 0) return {};
        
        std::vector<int> res;
        res.reserve(nums.size() - k + 1);
        std::multiset<int> window;
        
        // 初始化第一个窗口
        for (int i = 0; i < k; ++i) {
            window.insert(nums[i]);
        }
        res.push_back(*window.rbegin()); // rbegin() 指向最大元素
        
        // 滑动窗口
        for (int i = k; i < nums.size(); ++i) {
            // 删除移出窗口的元素
            window.erase(window.find(nums[i - k]));
            // 添加新进入窗口的元素
            window.insert(nums[i]);
            // 获取当前窗口的最大值
            res.push_back(*window.rbegin());
        }
        
        return res;
    }
};


class Solution {
public:
// 最坏情况下 O(n×k)，当数组是递减序列时，每次都需要重新计算最大值，会超时
// 思路
/*
nums = [1, 3, -1, -3, 5, 3, 6, 7], k = 3

窗口1: [1, 3, -1]
  current_max = max(1,3,-1) = 3
  res = [3]

窗口2: [3, -1, -3]
  新元素 = -3, 移出元素 = 1
  1 != 3, -3 < 3, 保持current_max = 3
  res = [3, 3]

窗口3: [-1, -3, 5]
  新元素 = 5, 移出元素 = 3
  3 == 3, 需要重新找max(-1,-3,5) = 5
  res = [3, 3, 5]

窗口4: [-3, 5, 3]
  新元素 = 3, 移出元素 = -1
  -1 != 5, 3 < 5, 保持current_max = 5
  res = [3, 3, 5, 5]

窗口5: [5, 3, 6]
  新元素 = 6, 移出元素 = -3
  -3 != 5, 6 > 5, 更新current_max = 6
  res = [3, 3, 5, 5, 6]

窗口6: [3, 6, 7]
  新元素 = 7, 移出元素 = 5
  5 != 6, 7 > 6, 更新current_max = 7
  res = [3, 3, 5, 5, 6, 7]
  */
 // 暴力
     std::vector<int> maxSlidingWindow(std::vector<int>& nums, int k) {
        std::vector<int> res;
        res.reserve(nums.size() - k + 1);
        
        for(auto it = nums.begin(); it != nums.end() - k + 1; ++it)
        {
            res.emplace_back(*std::max_element(it, it + k));
        }
        return res;
    }
// 暴力优化
    std::vector<int> maxSlidingWindow(std::vector<int>& nums, int k) {
        if (nums.empty() || k <= 0) return {};
        
        std::vector<int> res;
        res.reserve(nums.size() - k + 1);
        
        // 处理第一个窗口
        auto current_max_it = std::max_element(nums.begin(), nums.begin() + k);
        int current_max = *current_max_it;
        res.push_back(current_max);
        
        // 处理后续窗口
        for (auto it = nums.begin() + 1; it != nums.end() - k + 1; ++it) {
            int new_element = *(it + k - 1);  // 新加入窗口的元素
            int removed_element = *(it - 1);  // 移出窗口的元素
            
            if (new_element >= current_max) {
                // 如果新元素大于等于当前最大值，直接更新
                current_max = new_element;
            } else if (removed_element == current_max) {
                // 如果移出的元素是当前最大值，需要重新找最大值
                current_max_it = std::max_element(it, it + k);
                current_max = *current_max_it;
            }
            // 否则保持当前最大值不变
            
            res.push_back(current_max);
        }
        
        return res;
    }
};

/*
 * @lc app=leetcode.cn id=239 lang=cpp
 *
 * [239] 滑动窗口最大值
 */

// @lc code=start
#include <vector>
#include <deque>

using namespace std;

auto findTwoLargest(const vector<int>& nums) 
-> tuple<int,int,int,int>
{
    int max1_val = nums[0], max1_idx = 0;
    int max2_val = nums[0], max2_idx = 0; // 初始化为第一个元素，后续调整

    if (nums.size() >= 2) {
        if (nums[1] > max1_val) {
            max1_val = nums[1];
            max1_idx = 1;
            max2_val = nums[0];
            max2_idx = 0;
        } else {
            max2_val = nums[1];
            max2_idx = 1;
        }

        for (int i = 2; i < nums.size(); ++i) {
            if (nums[i] > max1_val) {
                max2_val = max1_val;
                max2_idx = max1_idx;
                max1_val = nums[i];
                max1_idx = i;
            } else if (nums[i] > max2_val || (nums[i] == max1_val && max2_val < max1_val)) {
                max2_val = nums[i];
                max2_idx = i;
            }
        }
    }
    return {max1_val, max1_idx, max2_val, max2_idx};
}
class Solution {
public:
    vector<int> maxSlidingWindow(vector<int>& nums, int k) {
        if (nums.empty() || k <= 0) return {};
        
        std::vector<int> res;
        res.reserve(nums.size() - k + 1);
#ifdef BRUTE_FORCE
        // 处理初始窗口
        vector<int> initWindow(nums.begin(), nums.begin() + k);
        auto [max1_val, max1_idx, max2_val, max2_idx] = findTwoLargest(initWindow);
        result.push_back(max1_val);

        for (int i = 1; i <= length - k; ++i) {
            int newElement = nums[i + k - 1];
            int outOfWindowIdx = i - 1;

            // 如果最大值被移出窗口，需要更新
            if (max1_idx == outOfWindowIdx) {
                // 需要重新找最大值
                vector<int> currentWindow(nums.begin() + i, nums.begin() + i + k);
                auto [newMax1, newMax1Idx, newMax2, newMax2Idx] = findTwoLargest(currentWindow);
                max1_val = newMax1;
                max1_idx = newMax1Idx + i; // 调整索引为原数组中的位置
                max2_val = newMax2;
                max2_idx = newMax2Idx + i;
            } else {
                // 检查新元素是否影响当前最大值或次大值
                if (newElement >= max1_val) {
                    max2_val = max1_val;
                    max2_idx = max1_idx;
                    max1_val = newElement;
                    max1_idx = i + k - 1;
                } else if (newElement > max2_val) {
                    max2_val = newElement;
                    max2_idx = i + k - 1;
                }
            }

            result.push_back(max1_val);
        }
#elif defined(deque)

        std::deque<int> dq; // 存储索引，而不是值，按对应值从大到小排序
        int win_size = k - 1;
        for (int i = 0; i < nums.size(); ++i) {
            // 移除不在当前窗口内的元素索引
            while (!dq.empty() && dq.front() <= i - k) {
                dq.pop_front();
            }
            
            // 从队列尾部移除所有小于当前元素的索引
            // 因为这些元素不可能是当前或未来窗口的最大值
            while (!dq.empty() && nums[dq.back()] < nums[i]) {
                dq.pop_back();
            }
            
            // 将当前元素索引加入队列
            dq.push_back(i);
            
            // 当窗口完全形成时，记录最大值
            if (i >= win_size) {
                res.push_back(nums[dq.front()]);
            }
        }
#else
        std::deque<int> dq; // 存储索引，按对应值从大到小排序
        
        // Lambda 函数：维护双端队列的单调性
        auto maintainDeque = [&](int index) {
            // 从队列尾部移除所有小于当前元素的索引
            while (!dq.empty() && nums[dq.back()] < nums[index]) {
                dq.pop_back();
            }
            // 将当前元素索引加入队列
            dq.push_back(index);
        };
        
        // Lambda 函数：移除过期元素
        auto removeExpired = [&](int currentIndex) {
            while (!dq.empty() && dq.front() <= currentIndex - k) {
                dq.pop_front();
            }
        };
        
        // 第一阶段：填充初始窗口 (0 到 k-1)
        for (int i = 0; i < k; ++i) {
            maintainDeque(i);
        }
        // 记录第一个窗口的最大值
        res.push_back(nums[dq.front()]);
        
        // 第二阶段：滑动窗口 (k 到 nums.size()-1)
        for (int i = k; i < nums.size(); ++i) {
            removeExpired(i);
            maintainDeque(i);
            res.push_back(nums[dq.front()]);
        }
#endif
        return res;
    }
};
// @lc code=end
int main() {
    Solution s;
    vector<int> nums = { -7,-8,7,5,7,1,6,0 };
    int k = 4;
    s.maxSlidingWindow(nums, k);
    return 0;
}

