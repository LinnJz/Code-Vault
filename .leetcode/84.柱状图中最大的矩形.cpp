int largestRectangleArea(vector<int>& heights) {
	size_t const size = heights.size();
	int *stk = reinterpret_cast<int *>(::alloca((size + 2) * sizeof(int)));
	stk[0] = -1, stk[1] = 0; // -1哨兵，头插0直接入栈（处理单调递减情况），不修改heights

	int maxArea = 0, top = 1;
	for (auto i = 0uz; i < size; ++i) {
		while (stk[top] != -1) {
			// 防止访存heights[stk[top]]两次？是否有必要这样写待考究，脑测这样写更优？
			// -O2 下编译器会使用 公共子表达式消除CSE进行优化，其实没必要写if else，这里保留
			if (int height = heights[stk[top]]; height > heights[i]) {
				if (int currArea = (i - stk[--top] - 1) * height; currArea > maxArea)
					maxArea = currArea;
			} else break;
		}

		stk[++top] = i;
	}
/*
// 单调递增情况，这种写法无法覆盖[2,1,2]
if (top == size + 1) [[unlikely]] {
	#pragma clang loop unroll_count(8)
	for (int i = 0; i < size; ++i) {
		if (int currArea = heights[i] * (size - i); currArea > maxArea)
			maxArea = currArea;
	}
}
*/
// 处理单调递减以及部分特殊用例（最终形成的stk栈是 单调递减的 -1，如 2 1 -1 还需要一个0进行计算），不需要heights尾插0
	while (top > 0) {
		int height = heights[stk[top]];
		--top;
		int left = stk[top];
		if (int currArea = (size - left - 1) * height; currArea > maxArea) 
			maxArea = currArea;
	}
	return maxArea;
}

int largestRectangleArea(vector<int>& heights) {
	heights.emplace_back(0);

	int *stk = reinterpret_cast<int *>(::alloca((heights.size() + 2) * sizeof(int)));
	stk[0] = -1, stk[1] = 0;

	int maxArea = 0, top = 1;
	for (size_t i = 0; i < heights.size(); ++i) {
		while (stk[top] != -1) {
			if (int height = heights[stk[top]]; height > heights[i]){
				if (int currArea = (i - stk[--top] - 1) * height; currArea > maxArea)
					maxArea = currArea;
			}
			else break;
		}

		stk[++top] = i;
	}
	return maxArea;
}
//  ————————————
  0 8 6 4 2 0 -1| 栈存储的是索引，这里方便比较我们以值展示
//  ————————————
i 5 4 3 2 1 0

8 * (5 - 3 - 1) = 8
6 * (5 - 2 - 1) = 12
4 * (5 - 1 - 1) = 12
2 * (5 - 0 - 1) = 8

// 柱子向左右扩展找到第一个比他小的柱子，就能计算面积
int largestRectangleArea(vector<int>& heights) {
	heights.emplace_back(0); // 处理 heights 2 4 6 8 递增情况，一直加入栈无法计算，不在heights头插0，因为时间复杂度高

	// 维持一个单调递减栈，当前元素 栈顶元素 栈顶的下方一个元素，找到当前元素、栈顶的下方一个元素均小于栈顶元素
	// 我们就能够计算面积
	std::vector<int> stk; stk.reserve(heights.size() + 2);
	stk.insert(stk.begin(), {-1, 0}); // 栈头插 0 处理 heights递减情况，一直弹出无法计算，-1表示栈空，如果不加-1会遗漏元素
	int maxArea = 0;
	for (int i = 0; i < heights.size(); ++i) { // i 从0开始，如果是头插heights0，则从1开始
		while (stk.back() != -1) {
			if (int prev_height = heights[stk.back()]; prev_height > heights[i]) {
				stk.pop_back();
				if (int currArea = (i - stk.back() - 1) * prev_height; currArea > maxArea)
					maxArea = currArea;
			}
			else break;
		}

		stk.emplace_back(i);
	}
	return maxArea;
}

// 双指针
int largestRectangleArea(std::vector<int>& heights) {
    if (heights.empty()) return 0;
    int n = heights.size();
    
    // 预处理左边界：left[i]表示i左侧第一个比heights[i]小的元素下标
    std::vector<int> left(n, -1);
    for (int i = 1; i < n; ++i) {
        int j = i - 1;
        // 跳过所有比当前柱子高的元素，直接跳转到更小的元素（剪枝）
        while (j >= 0 && heights[j] >= heights[i]) {
            j = left[j]; // 利用已预处理的结果，避免重复遍历
        }
        left[i] = j;
    }
    
    // 预处理右边界：right[i]表示i右侧第一个比heights[i]小的元素下标
    std::vector<int> right(n, n);
    for (int i = n - 2; i >= 0; --i) {
        int j = i + 1;
        // 同理，跳过所有比当前柱子高的元素
        while (j < n && heights[j] >= heights[i]) {
            j = right[j]; // 利用已预处理的结果
        }
        right[i] = j;
    }
    
    // 计算每个柱子的最大面积，取最大值
    int maxArea = 0;
    for (int i = 0; i < n; ++i) {
        int area = heights[i] * (right[i] - left[i] - 1);
        maxArea = std::max(maxArea, area);
    }
    
    return maxArea;
}

/*
 * @lc app=leetcode.cn id=84 lang=cpp
 *
 * [84] 柱状图中最大的矩形
 */

// @lc code=start
#include <stack>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iostream> 
using namespace std;
/*
## 解题思路详解
### 问题分析
对于每个柱子，我们需要确定以它的高度为矩形高度时，能够扩展的最大宽度。关键是找到每个柱子左右两侧第一个比它矮的柱子，这样就能确定这个柱子能扩展的最大范围。

### 单调栈原理
单调栈是解决这类问题的有效工具，它具有以下特点：

1. 栈中元素保持单调性（这里使用单调递增栈）
2. 当遇到破坏单调性的元素时，栈顶元素出栈并计算结果
### 算法步骤详解
1. 预处理 ：在数组前后添加高度为0的柱子作为哨兵，简化边界处理
   
   - 前哨兵确保所有柱子都有左边界
   - 后哨兵确保所有柱子最终都会被处理
2. 遍历柱子 ：从左到右遍历每个柱子
   
   - 维护一个单调递增栈，栈中存储柱子的索引
   - 当前柱子高度大于等于栈顶柱子高度时，直接入栈
   - 当前柱子高度小于栈顶柱子高度时，开始处理栈顶元素
3. 计算面积 ：当遇到高度小于栈顶的柱子时
   
   - 弹出栈顶元素，记录其高度为 currHeight
   - 此时新的栈顶元素是左侧第一个比 currHeight 小的柱子
   - 当前遍历到的柱子是右侧第一个比 currHeight 小的柱子
   - 计算宽度： width = i - st.top() - 1
   - 计算面积： area = currHeight * width
   - 更新最大面积
4. 重复处理 ：继续检查栈顶元素，直到栈为空或栈顶元素高度小于等于当前柱子高度
5. 当前柱子入栈 ：处理完所有需要出栈的元素后，将当前柱子入栈
### 为什么这种方法有效？
1. 单调性保证 ：栈中索引对应的高度始终保持递增，这确保了我们能够正确找到每个柱子左侧第一个比它矮的柱子
2. 出栈时机 ：当遇到高度小于栈顶的柱子时，我们知道：
   
   - 栈顶柱子的右边界已确定（当前遍历到的柱子）
   - 栈顶柱子的左边界也已确定（新的栈顶元素）
   - 此时可以精确计算以栈顶柱子高度为高的最大矩形面积
3. 哨兵作用 ：
   
   - 开始的0高度哨兵确保栈永不为空，简化代码逻辑
   - 结束的0高度哨兵确保所有元素都会被处理（因为最后一个元素可能无法触发出栈条件）
### 时间复杂度分析
- 时间复杂度：O(n)，每个元素最多入栈和出栈各一次
- 空间复杂度：O(n)，栈的大小最大为n
### 示例演示
以输入 [3,2,5,6,1,3] 为例：

1. 添加哨兵后变为 [0,3,2,5,6,1,3,0]
2. 遍历过程中栈的变化：
   
   - 初始：栈=[0]
   - i=1, heights[1]=3 > heights[0]=0, 入栈：栈=[0,1]
   - i=2, heights[2]=2 < heights[1]=3, 出栈计算面积：3×1=3, 栈=[0,2]
   - i=3, heights[3]=5 > heights[2]=2, 入栈：栈=[0,2,3]
   - i=4, heights[4]=6 > heights[3]=5, 入栈：栈=[0,2,3,4]
   - i=5, heights[5]=1 < heights[4]=6, 连续出栈计算面积：
     - 6×1=6
     - 5×2=10
     - 2×4=8
     - 栈=[0,5]
   - i=6, heights[6]=3 > heights[5]=1, 入栈：栈=[0,5,6]
   - i=7, heights[7]=0 < heights[6]=3, 连续出栈计算面积：
     - 3×1=3
     - 1×6=6
3. 最大面积为10
*/
class Solution {
public:
    int largestRectangleArea(vector<int>& heights) {
        int n = heights.size();
        if (n == 0) return 0;
        
        // 在heights数组前后分别添加高度为0的柱子，简化边界处理
        heights.insert(heights.begin(), 0);
        heights.push_back(0);
        n += 2;
        
        stack<int> st; // 单调递增栈，存储柱子的索引
        st.push(0);    // 先将第一个元素（高度为0的哨兵）入栈
        
        int maxArea = 0;
        
        // 从左到右遍历所有柱子
        for (int i = 1; i < n; ++i) {
            // 当前柱子高度小于栈顶柱子高度时，计算栈顶柱子能构成的最大矩形面积
            while (!st.empty() && heights[i] < heights[st.top()]) {
                int currHeight = heights[st.top()];
                st.pop(); // 弹出栈顶元素
                
                // 计算宽度：当前位置 - 新栈顶位置 - 1
                // 新栈顶元素是左边第一个小于currHeight的位置
                int width = i - st.top() - 1;
                
                // 更新最大面积
                maxArea = max(maxArea, currHeight * width);
            }
            
            // 当前柱子入栈
            st.push(i);
        }
        
        return maxArea;
    }
};
// @lc code=end

int main(){
    Solution s;
    vector<int> heights = {3,2,5,6,1,3};
    int res = s.largestRectangleArea(heights);
    cout << "最大矩形面积: " << res << endl;
    return 0;
}