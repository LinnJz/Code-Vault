template<class T, size_t N>
constexpr decltype(auto) make_array(T&& value) noexcept {
	return []<size_t ...Is>(T&& value, std::index_sequence<Is...>) 
	{
		return std::array { (void(Is), std::forward<T>(value))... }; // 逗号表达式进行包展开
	}(std::forward<T>(value), std::make_index_sequence<N>());
}

constexpr int lengthOfLongestSubstring(std::string const &s) noexcept {
	size_t const size = s.size();
	if (size <= 1) [[unlikely]] return size;

	// 查找表作为滑动窗口， ASCII映射下标，元素是字符串字符char位置索引
	auto ASCII_TABLE = make_array<int, 128>(-1);

	int maxLen = 0;
	// 双指针更新滑动窗口
	#pragma clang loop unroll_count(4)
	for (int left = 0, right = 0; right < size; ++right) {
		int const charIdx = s[right];
		int const recorded = ASCII_TABLE[charIdx] + 1; // 查找表是 -1 初始化，left是0，所以 + 1，另一层含义 遇到重复元素需要更新下标
		ASCII_TABLE[charIdx] = right;
		
		if (recorded > left) left = recorded;  // 如果我们先前记录过，说明不为-1，说明重复，更新left
		if (int currLen = right - left + 1; currLen > maxLen) maxLen = currLen;
	}
	return maxLen;
}

/*
 * @lc app=leetcode.cn id=3 lang=cpp
 *
 * [3] 无重复字符的最长子串
 */

// @lc code=start
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <set>
using namespace std;
class Solution {
public:
// abcdecfgh
// 滑动窗口加回溯
/*
    int lengthOfLongestSubstring(string s) {
        if (s.length() <= 1) return static_cast<int>(s.size());
        auto it = s.begin();
        unordered_map<char, decltype(it)> window;
        int res{0};
        while(it != s.end()) {
            if(window.contains(*it)) {
                res = max(res, static_cast<int>(window.size()));
                it = window[*it] + 1;
                window.clear();
            }
            window.emplace(*it, it);
            ++it;
        }
        res = max(res, static_cast<int>(window.size()));
        return res;
    }
*/
// 双指针优化
/*
    int lengthOfLongestSubstring(string s) {
        int n = s.size();
        if (n <= 1) return n;
        
        unordered_set<char> charSet;
        int left = 0, right = 0;
        int maxLen = 0;
        
        while (right < n) {
            if (charSet.contains(s[right])) {
                charSet.erase(s[left]);
                ++left;
            } 
            else {
                charSet.insert(s[right]);
                maxLen = max(maxLen, right - left + 1);
                ++right;
            }
        }
        return maxLen;
    }
*/
// 连续内存，跳跃left指针，减少hash时间优化
    int lengthOfLongestSubstring(string s) {
        int n = s.size();
        if (n <= 1) return n;
        
        vector<int> charSet(256, -1);
        int left = 0, right = 0;
        int maxLen = 0;

        while(right < n) {
            left = max(left, charSet[s[right]] + 1);

            charSet[s[right]] = right;

            maxLen = max(maxLen, right - left + 1);
            ++right;
        }
        return maxLen;
    }
};
// @lc code=end
/*
 * @lc app=leetcode.cn id=3 lang=cpp
 *
 * [3] 无重复字符的最长子串
 */

// @lc code=start
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <ranges>
#include <algorithm>
#include <iostream>
#include <format>
using namespace std;
#define CODE_REVIEW
#ifdef CODE_REVIEW
class Solution {
public:
    int lengthOfLongestSubstring(string s) {
        for (int left = 0, right = 0; right < s.size(); ++right) {
            //第一次进入for循环是空子串，不满足状态
            //right加入窗口
            while ()
            {
                //根据情况收集答案
                //Left移出窗口
            }
            //不满足状态（根据情况收集答案）
        }
        return ;
    }
};
// @lc code=end
#else

class Solution {
public:
    /// <describe>
    /// 滑动窗口
    /// 给一个字符串，找最长无重复字符的子串
    /// </describe>
    /*
    * 思路：
    * 子串意味着连续,本题要求字符不重复，意味着最长无重复字符的子串一定在去重后的字符串中
    * 如vqblqcb，vqblc，最长无重复字符子串是blqc和qblc一样（无需考虑顺序，我们这里只统计长度）
    * 如何设置滑动窗口？双指针设置，
    * 固定left，移动right统计最大长度，循环条件，right小于字符串长度
    * 指针移动：初始是移动right，并插入字符到unordered_set，如果set内包含了重复字符，擦除左边的字符
    * 更新left指针到重复字符的下一个，即++
    * 总体是滑动窗口遇到重复左侧缩减，右侧始终扩大，扩大时要计算最长无重复字符的子串
    * vqblqcb
    * v
    * vq
    *  qb
    *  qbl
    *   blqc
    *    lqcb
    */
    /*
    * 上述思路是通用写法，本题要求，是英文、数字、字符，ASCII码，可以优化
    * 使用数组，128个数，初始设置-1，代表字符，给每个字符（下标）赋值字符串对应字符的下标，通过right的移动实现
    * left指针则是通过max进行跳跃到正确的位置（因为128个数中间可能存在-1，即不连续）
    * 然后通用通过指针更新最长无重复字符的子串
    */
    constexpr auto initialize_array(int value) {
        std::array<int, 128> arr{};
        for (auto& e : arr) e = value; // 内部循环，但对用户透明
        return arr;
    }
    template <std::size_t... I>
    constexpr std::array<int, sizeof...(I)> generate_array(int value, std::index_sequence<I...>) {
        return { ((void)I, -1)... }; // 生成 128 个 -1
    }
    int lengthOfLongestSubstring(string s) {
        // 通用写法
        /*unordered_set<char> char_set;
        int left = 0, right = 0, max_length = 0;
        while (right < s.size()) {
            if (char_set.contains(s[right])) {
                char_set.erase(s[left]);
                left++;
            }
            else {
                char_set.insert(s[right]);
                right++;
                max_length = max(max_length, right - left);
            }
        }
        return max_length;*/
        array<int, 128> last_occurrence = std::move(generate_array(-1, std::make_index_sequence<128>{})); // ASCII 字符集
        int max_len = 0, left = 0;

        for (int right = 0; right < s.size(); ++right) {
            // 若字符已存在且在当前窗口内，跳跃左指针
            left = max(left, last_occurrence[s[right]] + 1);//取下标

            // 更新字符的最后出现位置
            last_occurrence[s[right]] = right;

            // 计算当前窗口长度
            max_len = max(max_len, right - left + 1);
        }
        return max_len;
    }
};
#endif
/*
class Solution {
public:
    // 子串意味着连续
    int lengthOfLongestSubstring(string s) {
        // a 0 3
        // b 1 4 6 7
        // c 2 5

        // p 0
        // w 1 2 5
        // k 3
        // e 4
        if (s.empty()) return 0;
        // unordered_map<char, unordered_set<size_t>> char_positions;
        // unordered_map<char, size_t> one_longest_str;
        // size_t unichar_total_num = 0;
        // size_t longest_length = 0;
        // size_t temp_length = 0;
        // {
        //     unichar_total_num = unordered_set<char>{ s.begin(), s.end() }.size();
        // }
        // one_longest_str.reserve(unichar_total_num);
        // for (auto [i, c] : s | std::views::enumerate) {
        //     char_positions[c].insert(i);
        //     if (one_longest_str.contains(c))
        //     {
        //         if (char_positions[c].contains(i - 1))
        //         {
        //             temp_length = 1;
        //             one_longest_str.clear();
        //             one_longest_str[c] = i - 1;
        //         }
        //         else
        //         {
                    
        //         }
        //     }
        //     else 
        //     {
        //         ++temp_length;
        //         one_longest_str[c] = i - 1;
        //         longest_length = max(temp_length, longest_length);
        //         if (longest_length == unichar_total_num)
        //             break;
        //     }
        // }
        size_t unichar_total_num = 0;
        {
            unichar_total_num = unordered_set<char>{ s.begin(), s.end() }.size();
        }
        const size_t str_length = s.size();
        size_t longest_length = 0;
        size_t left = 0;
        unordered_map<char, size_t> window_char_i;
        unordered_map<size_t, char> window_i_char;
        window_char_i.reserve(str_length);
        window_i_char.reserve(str_length);
        for(auto&& [i, c] : s | views::enumerate)
        {
            if(window_char_i.contains(c))
            {
                if (size_t rechar_i = window_char_i[c];
                    window_i_char.contains(rechar_i))
                {
                    if (window_i_char.contains(rechar_i + 1)) {
                        longest_length = 
                        //rechar_i - window_char_i[window_i_char[rechar_i + 1]] != 1 ? max(longest_length, i - left) : 
                            i == str_length - 1 ? max(longest_length, i - left + 1) : max(longest_length, i - left);
                        if (longest_length == unichar_total_num) break;
                    }
                }
                left = max(window_char_i[c] + 1, left);
                window_char_i[c] = i;
            }
            else
            {
                // 插入
                window_char_i[c] = i;
                window_i_char[i] = c;
                longest_length = max(longest_length, i - left + 1);
                if (longest_length == unichar_total_num) break;
            }
        }
        return (int)longest_length;
    }
};
int main()
{
    Solution obj;
    obj.lengthOfLongestSubstring("vqblqcb");
    unordered_map<string, size_t> strs = { {"abcabcbb", 3}, {"bbbbb", 1}, {"pwwkew", 3}, {" ", 1}, {"aab", 2}, {"dvdf", 3}, {"tmmzuxt", 5}, {"uqinntq", 4}
    , {"aabaab!bb", 3}, {"vbxpvwkkteaiob", 7}, {"ckilbkd", 5}, {"vqblqcb", 4}};
    for (auto&& [key, value] : strs)
    {
        size_t my_length = obj.lengthOfLongestSubstring(key);
        cout << format("{} —— string {} standard length is {}, my length is {}", value == my_length, key, value, my_length) << endl;
    }
    return 0;
}
*/