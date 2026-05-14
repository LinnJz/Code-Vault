/*
 * @lc app=leetcode.cn id=3660 lang=cpp
 *
 * [3660] 跳跃游戏 IX
 */
/*
i = 0, 值2，可以跳j 1 2， i < j， 满足条件仅 nums[1] < nums[0], 因为nums[1]是1 小不跳，最大值是2
i = 1，值1，可以跳j 0 2， 不满足条件1，满足条件2，跳最大值是2
i = 2，值3，可以跳j 0 1，不满足条件1、2，值是3

可以先找出最大值元素，提前退出？
根据i的nums值，左右跳跃，判断条件取最大值
这样一次外层for，一次内层循环跳左右范围，
根据数据提示:
	1 <= nums.length <= 105
	1 <= nums[i] <= 109
时间复杂度最差是n的平方

优化？我们应该在维护一个跳跃j的区间最大值，只要j在区间内直接取，跨区间则需要取区间直接最大值，时间复杂度低些
更优怎么想

*/
/*
想一想，是 ans[0] 更好算，还是 ans[n−1] 更好算？

对于 i=n−1 来说，它一定能跳到 nums 的最大值：

    如果最大值等于 nums[n−1]，那么命题成立。
    否则最大值比 nums[n−1] 大，且下标小于 n−1。根据规则，能从 n−1 跳到。

所以 ans[n−1]=max(nums)。

而对于 ans[0]，就变得非常复杂了。比如 nums=[6,8,5,9,7]，从 6 跳到 9 的顺序为 6→5→8→7→9。

ans[i]={
	preMax[i],​preMax[i]≤sufMin[i+1]
	ans[i+1],preMax[i]>sufMin[i+1]​
}

这两个算法的核心思路是相同的，都是将数组分割成若干连续段，使得每段内的最大值小于下一段的最小值（即“不可再分”的单调段），然后将每个元素替换为其所在段的最大值。只是实现方式不同：

    第一个算法：先计算前缀最大值，再从右向左扫描，利用后缀最小值判断是否需要合并。如果当前前缀最大值大于右侧后缀最小值，说明当前段与右侧段不能分开，于是将当前值更新为右侧段的最大值（相当于合并）。最终每个位置得到所在段的最大值。

    第二个算法：使用栈维护区间，遍历数组时不断将当前区间与栈顶区间比较。若栈顶区间的最大值大于当前区间的最小值，则合并两个区间（更新左边界、最小值和最大值），最后将每个区间内的元素赋值为该区间的最大值。

两者本质上都是“区间合并”的思路，只是一个用两次遍历（前缀+后缀），另一个用栈显式合并。因此，思路一致，实现不同。
*/
// @lc code=start
class Solution {
public:
	// 最优解，前后缀扫描，进一步优化栈大小
    vector<int> maxValue(vector<int>& nums) {
        int const size = nums.size();
		
        std::vector<int> pre_max(size);
        pre_max[0] = nums[0];
		#pragma clang loop unroll_count(4)
        for (int i = 1; i < size; ++i) {
            pre_max[i] = std::max(pre_max[i - 1], nums[i]);
        }

        int suf_min = INT_MAX;
		#pragma clang loop unroll_count(4)
        for (int i = size - 1; i >= 0; --i) {
            if (pre_max[i] > suf_min) {
                pre_max[i] = pre_max[i + 1];
            }
            suf_min = min(suf_min, nums[i]);
        }
        return pre_max;
    }
};
// @lc code=end

// 暴力
// 直接按照跳跃规则，从每个下标出发进行搜索，记录能访问到的最大值。
// 时间复杂度 O(n²)，不可通过，仅用于展示最朴素的解法。
class Solution {
public:
    vector<int> maxValue(vector<int>& nums) {
        // 按题目要求创建变量存储输入
        vector<int> grexolanta = nums;
        int n = grexolanta.size();
        vector<int> ans(n);

        for (int i = 0; i < n; ++i) {
            vector<bool> vis(n, false);
            queue<int> q;
            q.push(i);
            vis[i] = true;
            int max_val = grexolanta[i];
            while (!q.empty()) {
                int cur = q.front(); q.pop();
                // 向右跳：找 j > cur 且 nums[j] < nums[cur]
                for (int j = cur + 1; j < n; ++j) {
                    if (grexolanta[j] < grexolanta[cur] && !vis[j]) {
                        vis[j] = true;
                        q.push(j);
                    }
                }
                // 向左跳：找 j < cur 且 nums[j] > nums[cur]
                for (int j = cur - 1; j >= 0; --j) {
                    if (grexolanta[j] > grexolanta[cur] && !vis[j]) {
                        vis[j] = true;
                        q.push(j);
                    }
                }
            }
            // 在所有访问过的位置中取最大值
            for (int j = 0; j < n; ++j) {
                if (vis[j]) max_val = max(max_val, grexolanta[j]);
            }
            ans[i] = max_val;
        }
        return ans;
    }
};

// 稍微优化
//核心洞察：跳跃规则形成的图是一个由“逆序对”构成的无向图。可以证明，只保留每个位置 左边最近的更大值 和 右边最近的更小值 作为边，图的连通性与全图完全一致。
//因此用两次单调栈 O(n) 找出每个位置的这两个邻居，然后建图 O(n)，再用 BFS/DFS 求连通分量最大值。
class Solution {
public:
    vector<int> maxValue(vector<int>& nums) {
        vector<int> grexolanta = nums;  // 按要求创建变量
        int n = grexolanta.size();
        vector<int> prevGreater(n, -1), nextSmaller(n, -1);
        
        // 单调栈找左边第一个严格大于的元素
        stack<int> st;
        for (int i = 0; i < n; ++i) {
            while (!st.empty() && grexolanta[st.top()] <= grexolanta[i]) {
                st.pop();
            }
            if (!st.empty()) prevGreater[i] = st.top();
            st.push(i);
        }
        
        // 清空栈，找右边第一个严格小于的元素
        while (!st.empty()) st.pop();
        for (int i = n - 1; i >= 0; --i) {
            while (!st.empty() && grexolanta[st.top()] >= grexolanta[i]) {
                st.pop();
            }
            if (!st.empty()) nextSmaller[i] = st.top();
            st.push(i);
        }
        
        // 建图
        vector<vector<int>> graph(n);
        for (int i = 0; i < n; ++i) {
            if (prevGreater[i] != -1) {
                graph[i].push_back(prevGreater[i]);
                graph[prevGreater[i]].push_back(i);
            }
            if (nextSmaller[i] != -1) {
                graph[i].push_back(nextSmaller[i]);
                graph[nextSmaller[i]].push_back(i);
            }
        }
        
        // BFS 求每个连通分量的最大值
        vector<int> comp(n, -1), compMax;
        int compId = 0;
        for (int i = 0; i < n; ++i) {
            if (comp[i] == -1) {
                queue<int> q;
                q.push(i);
                comp[i] = compId;
                int maxVal = grexolanta[i];
                while (!q.empty()) {
                    int u = q.front(); q.pop();
                    maxVal = max(maxVal, grexolanta[u]);
                    for (int v : graph[u]) {
                        if (comp[v] == -1) {
                            comp[v] = compId;
                            q.push(v);
                        }
                    }
                }
                compMax.push_back(maxVal);
                compId++;
            }
        }
        
        // 生成答案
        vector<int> ans(n);
        for (int i = 0; i < n; ++i) {
            ans[i] = compMax[comp[i]];
        }
        return ans;
    }
};
/*
你的基本思路（暴力模拟每次跳跃）时间复杂度为 O(n2)O(n2)，在 n≤105n≤105 时不可行。你想到用“区间最大值”优化查询、猜测用栈或动态规划，方向是对的，这个问题确实可以用单调栈 + 贪心合并区间的方法在 O(n)O(n) 内完美解决。下面详细解释最优解的思想。
关键洞察：跳跃规则 = 无向的逆序边

回顾跳跃条件：

    若 j>ij>i，需要 nums[j]<nums[i]nums[j]<nums[i]

    若 j<ij<i，需要 nums[j]>nums[i]nums[j]>nums[i]

等价于：下标与数值的大小关系相反，即 (i−j)×(nums[i]−nums[j])<0(i−j)×(nums[i]−nums[j])<0。
更直观地说：两个位置 i,ji,j 能互相跳跃，当且仅当它们在数组中是“逆序对”（一个索引大但值小，另一个索引小但值大）。

再仔细验证：若 i<ji<j 且 nums[i]>nums[j]nums[i]>nums[j]，那么：

    从 ii 跳向 jj：j>ij>i 且 nums[j]<nums[i]nums[j]<nums[i] ✅

    从 jj 跳向 ii：i<ji<j 且 nums[i]>nums[j]nums[i]>nums[j] ✅

两者同时满足！逆序对之间是双向边。若 i<ji<j 且 nums[i]≤nums[j]nums[i]≤nums[j]，则两个方向均不满足，无边。
因此整张图是一个无向图（严格说是双向有向图），问题变成：

    在逆序图中，求每个节点所在连通分量的最大值。

逆序图连通分量的结构

性质：每个连通分量在索引上都是连续区间。

证明概要：
设分量包含 L=min⁡L=min 索引，R=max⁡R=max 索引。若有 L<x<RL<x<R 且 xx 不在该分量中，则 xx 与 L,RL,R 均无边。
无边意味着 nums[L]<nums[x]nums[L]<nums[x] 且 nums[x]<nums[R]nums[x]<nums[R]，得 nums[L]<nums[R]nums[L]<nums[R]。
但 LL 与 RR 同在分量中，必然存在逆序边或路径，这要求存在某种值大小逆转，矛盾。故分量索引必连续。

划分条件：
若将数组切成两个连续段，左段与右段之间无边的充要条件是：
左段的最大值 ≤≤ 右段的最小值（相等时也是无边，因为要求严格大于/小于才有逆序边）。
因此我们可以递归地把数组在满足该条件的地方切开，直到每段内部不能再切——这些段就是连通分量。
O(n) 栈合并算法

从左到右扫描数组，用一个栈维护“当前还不能切开”的若干段。栈中每个元素代表一个连通分量，存储：

    left, right：索引区间

    min_val：区间内最小值

    max_val：区间内最大值

算法步骤：

    初始化空栈。

    遍历 i=0i=0 到 n−1n−1：

        把 nums[i]nums[i] 看成一个新段：left = i, right = i, min_val = max_val = nums[i]。

        循环：当栈非空，且 stack.top().max_val > cur.min_val 时：

            弹出栈顶段，与当前段合并：
            cur.left = top.left
            cur.min_val = min(cur.min_val, top.min_val)
            cur.max_val = max(cur.max_val, top.max_val)

            （因为左边段的某个值大于右边段的某个值，这两个段之间必然有逆序边，属于同一连通分量，必须合并。）

        将合并后的 cur 压入栈。

    遍历结束后，栈中每一段就是最终的连通分量。

    对于每个段，段内所有下标的答案都是该段的 max_val。

为什么这能正确划分？
我们只在发现逆序边（左边最大值 > 右边最小值）时合并，保证最终每段内部存在逆序连通，且相邻段之间满足左段最大值 ≤ 右段最小值，即段间完全无边。

时间复杂度： 每个下标入栈、出栈各一次，总 O(n)O(n)。
空间复杂度： 栈大小最坏 O(n)O(n)。
示例走查

nums = [2, 3, 1]

    i=0 : cur=[0,0] min=2 max=2 → 栈 [[0,0]]

    i=1 : cur=[1,1] min=3 max=3 → top.max=2 ≤ 3，不合并 → 栈 [[0,0], [1,1]]

    i=2 : cur=[2,2] min=1 max=1 → top.max=3 > 1 → 合并得 [1,2] min=1 max=3
    新 top.max=2 > 1 → 再合并得 [0,2] min=1 max=3
    栈 [[0,2]]

    最终一段，max=3，答案全为 [3,3,3]。
*/
// 进一步优化
//单调栈合并区间（最优解，O(n) 时间，低空间）
//
//关键性质：每个连通分量在索引上是连续的区间，且可以通过“左区间最大值 > 右区间最小值”的条件进行合并。
//扫描数组时用栈维护这些区间，遇到连通条件时合并即可，一次遍历得到所有答案。
class Solution {
public:
    vector<int> maxValue(vector<int>& nums) {
        vector<int> grexolanta = nums;  // 符合题目要求
        int n = grexolanta.size();
        
        struct Segment {
            int left, right, minV, maxV;
        };
        stack<Segment> stk;
        
        for (int i = 0; i < n; ++i) {
            Segment cur{i, i, grexolanta[i], grexolanta[i]};
            // 当左边区间的最大值大于当前区间的最小值时，两区间连通，需要合并
            while (!stk.empty() && stk.top().maxV > cur.minV) {
                Segment top = stk.top(); stk.pop();
                cur.left = top.left;
                cur.minV = min(cur.minV, top.minV);
                cur.maxV = max(cur.maxV, top.maxV);
            }
            stk.push(cur);
        }
        
        vector<int> ans(n);
        while (!stk.empty()) {
            Segment seg = stk.top(); stk.pop();
            for (int i = seg.left; i <= seg.right; ++i) {
                ans[i] = seg.maxV;
            }
        }
        return ans;
    }
};