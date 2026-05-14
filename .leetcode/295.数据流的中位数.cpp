class MedianFinder {
public:
    MedianFinder() {
        minPart.reserve(1024);
        maxPart.reserve(1024);
    }
    
    void addNum(int num) {
        if (minPart.empty() || num <= minPart[0]) {
            minPart.push_back(num);
			// 保持堆
            std::push_heap(minPart.begin(), minPart.end(), std::less<>{}); // 和sort对比，less在sort是递增，堆排递增是大根堆，最小部分
        }
        else {
            maxPart.push_back(num);
            std::push_heap(maxPart.begin(), maxPart.end(), std::greater<>{});
        }

        if (minPart.size() > maxPart.size() + 1) {
			// 下沉拿出堆顶元素
            std::pop_heap(minPart.begin(), minPart.end(), std::less<>{});

            maxPart.push_back(minPart.back());
            std::push_heap(maxPart.begin(), maxPart.end(), std::greater<>{});

			// 别忘记弹出
            minPart.pop_back();
        }
        else if (maxPart.size() > minPart.size()){
            std::pop_heap(maxPart.begin(), maxPart.end(), std::greater<>{});

            minPart.push_back(maxPart.back());
            std::push_heap(minPart.begin(), minPart.end(), std::less<>{});

            maxPart.pop_back();
        }
    }
    
    double findMedian() {
        if (minPart.size() == maxPart.size() + 1) {
            return minPart[0];
        }
        return (minPart[0] + maxPart[0]) / 2.0;
    }
private:
    std::vector<int> minPart; // 大根堆，得到递增序列，是最小部分
    std::vector<int> maxPart; // 小根堆，得到递减序列，是最大部分
};

/*
 * @lc app=leetcode.cn id=295 lang=cpp
 *
 * [295] 数据流的中位数
 */

// @lc code=start
#include <queue>
#include <array>
#include <ranges>
using namespace std;
class MedianFinder {
public:
    MedianFinder() {}
    
    void addNum(int num) {
        // 保证 max_heap 的大小 >= min_heap
        if (max_heap.empty() || num <= max_heap.top()) {
            max_heap.push(num);
        } else {
            min_heap.push(num);
        }
        
        // 平衡堆大小，确保 max_heap.size() >= min_heap.size() 且差值不超过1
        if (max_heap.size() > min_heap.size() + 1) {
            min_heap.push(max_heap.top());
            max_heap.pop();
        } else if (min_heap.size() > max_heap.size()) {
            max_heap.push(min_heap.top());
            min_heap.pop();
        }
    }
    
    double findMedian() {
        if (max_heap.size() > min_heap.size()) {
            return max_heap.top();
        } else {
            return (max_heap.top() + min_heap.top()) / 2.0;
        }
    }

private:
    priority_queue<int> max_heap; // 存放较小的一半（最大堆）
    priority_queue<int, vector<int>, greater<int>> min_heap; // 存放较大的一半（最小堆）
    /*
    O(nlogn)
public:
    MedianFinder() {
        
    }
    
    void addNum(int num) {
        _data.push_back(num);
        if(_data.size() % 2 == 1) median++;
    }
    
    double findMedian() {
        priority_queue<int, vector<int>, greater<int>> min_heap{_data.begin(), _data.end()};
        int count = -1; double median_num = 0;
        while (count != median)
        {
            median_num = min_heap.top();
            min_heap.pop();
            count++;
        }
        return _data.size() % 2 == 1 ? median_num : (median_num + min_heap.top()) / 2;
    }
private:
    int median = -1;
    vector<int> _data;
    */
};

/**
 * Your MedianFinder object will be instantiated and called as such:
 * MedianFinder* obj = new MedianFinder();
 * obj->addNum(num);
 * double param_2 = obj->findMedian();
 */
// @lc code=end
int main()
{
    MedianFinder obj;
    obj.addNum(6);
    auto r1 = obj.findMedian();
    obj.addNum(10);
    auto r2 = obj.findMedian();
    obj.addNum(2);
    auto r3 = obj.findMedian();
    obj.addNum(6);
    auto r4 = obj.findMedian();
    obj.addNum(5);
    auto r5 = obj.findMedian();
    obj.addNum(0);
    auto r6 = obj.findMedian();
    return 0;
}

