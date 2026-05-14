class Solution {
private:
    template<typename BiIter, typename Compare>
    void insert_sort(BiIter start, BiIter end, Compare comp)
    {
        if (first == last) [[unlikely]] return;
        #pragma GCC unroll 8
        for (auto current = std::next(start); current != end; ++current)
        {
            auto value = std::move(*current);
            auto hole = current;

            auto prev = hole;

            while(prev != start && comp(value, *std::prev(prev)))
            {
                --prev;
                *hole = std::move(*prev);
                hole = prev;
            }
            *hole = std::move(value);
        }
    }

    template<typename RanIter, typename Compare>
    void hoare_quick_sort(RanIter start, RanIter end, Compare comp)
    {
        const auto size = std::distance(start, end);
        if (size <= 32) {
            insert_sort(start, end, comp);
            return;
        }

        // 三数取中法选择枢轴, 确保 left ≤ mid ≤ right, 防止有序情况分布不均匀
        auto left = first, right = std::prev(last), mid = left + (right - left) / 2;
        if (comp(*mid, *left)) std::iter_swap(mid, left);
        if (comp(*right, *left)) std::iter_swap(right, left);
        if (comp(*right, *mid)) std::iter_swap(right, mid);

        // 优化分区循环的边界检查，如果不移动枢轴 需要额外处理枢轴位置 要跳过枢轴本身
        auto pivot_pos = std::prev(right); // 倒数第二个位置
        std::iter_swap(mid, pivot_pos); // 将枢轴放到倒数第二个位置
        auto& pivot_val = *pivot_pos;
        /*
1. 使用 j >= left 的情况：
    当 j == left 时，条件为真，会检查 comp(pivot_val, *left)
    如果 comp(pivot_val, *left) 为真，j 会继续减1，导致 j < left（越界）
    在您的实现中，由于有三数取中保证 *left <= pivot_val，所以 comp(pivot_val, *left) 为假，循环会停止
    优点：检查了 left 位置的元素，确保划分完整性
    风险：如果未来修改代码（如去掉三数取中），可能导致越界

2. 使用 j > left 的情况：
    当 j == left 时，条件为假，循环立即停止
    j 不会减到 left 之前，避免了越界风险
    在您的三数取中保证下，left 位置的元素不会被错误处理
    优点：更安全，避免越界
    缺点：不检查 left 位置的元素（但在当前实现中这不是问题）
        */
        // left(i) ≤ pivot_pos(j) ≤ right, 已有序情况下，我们从i的next，j的prev开始
        auto i = std::prev(left), j = pivot_pos;
        while (true) {
            do { ++i; } while (comp(*i, pivot_val)); // 找比枢轴大
            do { --j; } while (j > left && comp(pivot_val, *j)); // 找比枢轴小
            if (i >= j) break;
            std::iter_swap(i, j);
        }

        // 将枢轴放到正确位置
        std::iter_swap(i, pivot_pos); // 此时i的值是枢轴值
        // 通过先递归较小的子数组保证了最坏情况下的深度仍然为 O(log n)，防止栈溢出。
		// 这是一种尾递归优化思想的应用，但不是严格的尾递归
        if (std::distance(left, i) < std::distance(i, right)) {
            hoare_quick_sort(left, i, comp);
            hoare_quick_sort(std::next(i), end, comp); // 注意end是开区间
        } else {
            hoare_quick_sort(std::next(i), end, comp);
            hoare_quick_sort(left, i, comp);
        }
    }
public:
    std::vector<int> sortArray(std::vector<int>& nums) {
        hoare_quick_sort(nums.begin(), nums.end(), std::less<>{});
        return nums;
    }
};
/*
 * @lc app=leetcode.cn id=912 lang=cpp
 *
 * [912] 排序数组
 */

// @lc code=start
class Solution {
public:
    vector<int> sortArray(vector<int>& nums) {
        _sort(nums.begin(), nums.end(), std::less<>{});
        return nums;
    }
private:
    template<typename BidirectionalIterator, typename Compare>
    void _insertion_sort(BidirectionalIterator first, BidirectionalIterator last, Compare comp) {
        if (first == last) return; // 如果范围为空，直接返回

        for (auto current = std::next(first); current != last; ++current) {
            // 将要排序的元素临时移动出来，留出一个“空位”
            auto value = std::move(*current);
            auto hole = current; // ‘空位’的位置

            if (comp(value, *first)) {
                // 情况1：新元素比首元素还小，它将是最新的首元素
                // 将整个已排序区间[first, hole)向后移动一个位置
                std::move_backward(first, hole, std::next(hole));
                *first = std::move(value);
            }
            else {
                // 情况2：新元素需要插入到已排序区的中间某个位置
                // 从‘空位’的前一个元素开始向前遍历，寻找插入点
                auto prev = std::prev(current);
                while (comp(value, *prev)) {
                    // 将元素向后移动，空位向前移动
                    *hole = std::move(*prev);
                    hole = prev;
                    if (prev == first) break; // 防止 prev 迭代器向前越界
                    --prev;
                }
                // 将值插入找到的空位
                *hole = std::move(value);
            }
        }
    }

    template <typename RandomAccessIterator, typename Compare>
    void _choose_pivot(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last, Compare comp) {
        // 对 first, mid, (last-1) 三个位置的元素进行排序，确保中位数在 mid 位置
        // 这有助于选择更好的分区点，避免快排在有序数组上出现最坏情况。
        auto& a = *first;
        auto& b = *mid;
        auto& c = *(last - 1);

        if (comp(b, a)) std::swap(a, b);
        if (comp(c, b)) std::swap(b, c);
        if (comp(b, a)) std::swap(a, b);
        // 现在，b（即 *mid）是三个值的中位数
    }

    template <typename RandomAccessIterator, typename Compare>
    std::pair<RandomAccessIterator, RandomAccessIterator>
        _partition(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
        // 1. 选择基准点
        auto mid = first + ((last - first) >> 1);
        _choose_pivot(first, mid, last, comp);
        auto pivot = *mid; // 基准值

        // 2. 初始化指针。
        // p_first 和 p_last 将最终指向“等于基准”的区间 [p_first, p_last)
        // 但在此过程中，它们会先用于扩展“等于基准”的区间
        auto p_first = mid;
        auto p_last = std::next(mid);

        // 3. 向左右扩展，初步确定“等于基准”的连续区域
        //    这步优化对于有很多重复元素的情况很有用
        while (first < p_first && !static_cast<bool>(comp(*std::prev(p_first), *p_first)) && !comp(*p_first, *(p_first - 1))) {
            --p_first;
        }
        while (p_last < last && !static_cast<bool>(comp(*p_last, *p_first)) && !comp(*p_first, *p_last)) {
            ++p_last;
        }

        // 4. 现在进行真正的三路分区
// g_first: 当前正在处理的“未确认区域”的右半部分的起始位置（通常指向一个“可能大于”基准的元素）
        // g_last:  当前正在处理的“未确认区域”的左半部分的结束位置（通常指向一个“可能小于”基准的元素的下一个位置）
        auto g_first = p_last;
        auto g_last = p_first;

        while (true) {
            // 4a. 从左向右扫描右侧的“未确认区域” (g_first 到 last-1)
            // 目标：找到一个不属于“大于基准”区域的元素，并将其归类
            for (; g_first < last; ++g_first) {
                // 情况 1: 当前元素 *g_first 严格大于基准值
                if (static_cast<bool>(comp(*p_first, *g_first))) {
                    continue; // 它本就该在右侧的“大于区”，继续向右扫描
                }
                // 情况 2: 当前元素 *g_first 严格小于基准值
                else if (comp(*g_first, *p_first)) {
                    break; // 找到了一个应该被交换到左侧“小于区”的元素，跳出循环等待后续交换
                }
                // 情况 3: 当前元素 *g_first 等于基准值
                // 且当前“等于区”的右边界 p_last 还未扩展到这个位置
                else if (p_last != g_first) {
                    // 将这个等于基准的元素交换到“等于区”的右侧末端 (p_last 指向的位置)
                    std::swap(*p_last, *g_first);
                    ++p_last; // 扩展“等于区”的右边界
                    // 注意：交换后，原 *g_first 位置的新元素（来自原 *p_last）仍需后续检查，
                    // 但本次循环继续向右扫描，该新元素将在后续的循环中被检查
                }
                // 情况 4: 当前元素 *g_first 等于基准值
                // 且它正好紧邻着当前的“等于区”右边界 (p_last == g_first)
                else {
                    // 它自然就在“等于区”的右侧末端，只需简单地扩展右边界即可
                    ++p_last;
                    // 循环继续，g_first 也会自增，继续检查下一个元素
                }
            }

            // 4b. 从右向左扫描左侧的“未确认区域” (first 到 g_last-1)
            // 目标：找到一个不属于“小于基准”区域的元素，并将其归类
            for (; first < g_last; --g_last) {
                // 使用 g_last_prev 指向当前实际被检查的元素 (g_last 的前一个位置)
                const auto g_last_prev = std::prev(g_last);
                // 情况 1: 当前元素 *g_last_prev 严格小于基准值
                if (static_cast<bool>(comp(*g_last_prev, *p_first))) {
                    // 它本就该在左侧的“小于区”，继续向左扫描
                    continue;
                }
                // 情况 2: 当前元素 *g_last_prev 严格大于基准值
                else if (comp(*p_first, *g_last_prev)) {
                    // 找到了一个应该被交换到右侧“大于区”的元素，跳出循环等待后续交换
                    break;
                }
                // 情况 3: 当前元素 *g_last_prev 等于基准值
                // 且当前“等于区”的左边界 p_first 的前一个位置不是这个位置
                else if (--p_first != g_last_prev) { // 先扩展“等于区”左边界
                    // 将这个等于基准的元素交换到“等于区”的左侧末端 (p_first 指向的位置)
                    std::swap(*p_first, *g_last_prev); // intentional ADL
                    // 交换后，原 *g_last_prev 位置的新元素（来自原 *p_first）仍需后续检查，
                    // 但本次循环继续向左扫描，该新元素将在后续的循环中被检查
                }
                // 情况 4: 当前元素 *g_last_prev 等于基准值
                // 且它正好紧邻着当前的“等于区”左边界 (扩展后的 p_first 就是 g_last_prev)
                // 那么不需要交换，因为扩展左边界 (--p_first) 后，它自然就在“等于区”的左侧末端
                // 因此这个 else 分支不需要做任何操作，循环继续向左扫描
            }

            // 4c. 检查分区是否完成
            if (g_last == first && g_first == last) {
                // 分区完成，返回“等于基准”的区间 [p_first, p_last)
                return { p_first, p_last };
            }

            // 4d. 处理边界情况并进行交换
            if (g_last == first) { // 左侧已没有“小于区”的元素，但右侧还有未处理的
                if (p_last != g_first) {
                    std::swap(*p_first, *p_last); // 将“等于区”最右的元素与p_first交换
                }
                ++p_last;
                // 现在将找到的“小于”基准的元素（在g_first）交换到左侧
                std::swap(*p_first, *g_first);
                ++p_first;
                ++g_first;
            }
            else if (g_first == last) { // 右侧已没有“大于区”的元素，但左侧还有未处理的
                --g_last;
                if (g_last != --p_first) { // 注意：--p_first
                    std::swap(*g_last, *p_first);
                }
                std::swap(*p_first, *(--p_last)); // 注意：--p_last
            }
            else {
                // 一般情况：交换在左侧找到的“大于”基准的元素和右侧找到的“小于”基准的元素
                --g_last;
                std::swap(*g_first, *g_last);
                ++g_first;
            }
        }
    }

    template <typename RandomAccessIterator, typename Compare>
    void _introsort(RandomAccessIterator first, RandomAccessIterator last, std::size_t depth_limit, Compare comp) {
        constexpr decltype(last - first) INSERTION_SORT_THRESHOLD = 32; // 阈值，小于该大小的数组使用插入排序
        while (last - first > INSERTION_SORT_THRESHOLD) { // 改用循环处理一侧，减少递归深度
            if (depth_limit == 0) {
                // 递归深度过大，退化为堆排序，保证最坏情况时间复杂度为O(n log n)
                std::make_heap(first, last, comp);
                std::sort_heap(first, last, comp);
                return;
            }
            depth_limit = (depth_limit >> 1) + (depth_limit >> 2);

            // 进行三路分区
            auto [eq_begin, eq_end] = _partition(first, last, comp);

            // 递归处理元素较少的那个分区，对另一个分区进行尾递归优化（即循环处理）
            if (eq_begin - first < last - eq_end) {
                // 左侧（小于基准的部分）元素较少，先递归排序左侧
                _introsort(first, eq_begin, depth_limit, comp);
                // 然后对右侧（大于基准的部分）进行循环处理（尾递归优化）
                first = eq_end;
            }
            else {
                // 右侧（大于基准的部分）元素较少，先递归排序右侧
                _introsort(eq_end, last, depth_limit, comp);
                // 然后对左侧（小于基准的部分）进行循环处理（尾递归优化）
                last = eq_begin;
            }
        } // end while

        // 对小的子数组使用插入排序，因为在小数组上它的常数因子更小
        _insertion_sort(first, last, comp);
    }

    template <typename RandomAccessIterator, typename Compare>
    void _sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
        if (first == last) return;
        _introsort(first, last, last - first, comp);
    }
};
// @lc code=end

