string minWindow(string const &s, string const &t) {
	size_t const sLen = s.size(), tLen = t.size();
	if (tLen > sLen) [[unlikely]] return {};

	//window 数组存储的是“差额”（剩余需求）
	std::array<int, 128> window{};
	// #pragma clang loop unroll_count(8)
	// for (char c : t) ++window[c];

	int minSubstrPos = 0, minSubstrLen = INT_MAX, mainVaild = 0, patternVaild = 0;
	
	// 去除得到真实unique字符个数
	// #pragma clang loop unroll_count(8)
	// for (size_t i = 0; i < 128; ++i) patternVaild += (window[i] > 0);
	
	#pragma clang loop unroll_count(8)
	for (char c : t) patternVaild += (++window[c] == 1);
	
	// 滑动窗口
	#pragma clang loop unroll_count(4)
	for (size_t left = 0, right = 0; right < sLen; ++right) {
		// 右指针扩张（right）：只关心“是否补完”，执行 --window[s[right]]，意思是“消耗掉一个需求”。
		//结论：重复字符的“数量要求”被编码进了 window 的数值里。mainValid 只在“数量达标”的那一瞬间增加。
		// char add = s[right];
		mainVaild += (--window[s[right]] == 0); // 如果s的字符在 t存在，--t的字符表示匹配，如果等于0说明满足一个unique字符
		
		// 考虑AABC找 ABC情况，mainValid和patternValid都是去重的字符个数，AABC是3匹配，从左到右，left移出了最左A，得到ABC仍旧匹配进行计算
		while (mainVaild == patternVaild) {
			if (int currLen = right - left + 1; currLen < minSubstrLen) {
				minSubstrLen = currLen, minSubstrPos = left;
			}
			//左指针收缩（left）：只关心“是否缺货”，执行 ++window[s[left]]，意思是“还回去一个需求”。
			// char del = s[left++];
			mainVaild -= (++window[s[left++]] > 0); // 收缩，删掉s的left，意味着该字符需要重新加回来，如果>0不满足unique字符个数（==0），说明有差异-1不能继续循环了
		}
	}
	return minSubstrLen != INT_MAX ? s.substr(minSubstrPos, minSubstrLen) : std::string{};
}

class Solution {
public:
    string minWindow(string s, string t) {
        size_t sLen{ s.size() }, tLen{ t.size() };
        if(sLen < tLen) return {};

        // need：记录t中每个字符的需要数量（ASCII码范围0-127，故数组大小128）
        // window：记录滑动窗口中每个字符的当前数量
        std::array<size_t, 128> need{}, window{};

        // 遍历t，统计每个字符的需要数量
        #pragma clang loop unroll_count(8)
        for(char c : t) ++need[c];
        
        // valid：窗口中满足「数量要求」的字符种类数（比如t需要2个A，窗口有2个A则valid+1）
        // required：t中需要满足的字符种类总数（去重后的字符数）
        // start：最小覆盖子串的起始位置
        // minLen：最小覆盖子串的长度（初始化为INT_MAX，方便后续更新）
        size_t valid{ 0 }, required{ 0 }, start{ 0 }, minLen{ INT_MAX };

        // 统计t中有多少种不同的字符（即需要满足的种类数）
        #pragma clang loop unroll_count(8)
        for (int i = 0; i < 128; ++i) if (need[i] > 0) ++required;
 
        #pragma clang loop unroll_count(4)
        for(size_t left = 0, right = 0; right < sLen; ++right) {
            // 右指针扩张窗口：将当前字符纳入窗口
            char add = s[right];
            // 如果当前字符是t需要的字符，字符计数+1，若恰好等于需要的数量valid+1
            if (need[add] > 0) {
                if (++window[add] == need[add])  ++valid;
            }
            // AABC ABC
            //左指针收缩窗口：当窗口满足所有字符需求（valid==required）时，尝试缩小窗口找最小值
            while (valid == required) {
                // 若比已记录的最小长度更小 → 更新最小长度和起始位置
                if (int currLen = right - left + 1; currLen < minLen) {
                    minLen = currLen;
                    start = left;
                }

                // 移出左指针指向的字符（收缩窗口）
                char del = s[left++];
                // 如果被移出的字符是t需要的字符，窗口中该字符计数-1（延迟）
                if (need[del] > 0) {
                    // 若移出前该字符数量恰好满足要求 → 移出后不再满足，valid-1
                    if (window[del]-- == need[del]) --valid;
                }
            }
        }

        return minLen == INT_MAX ? std::string{} : s.substr(start, minLen);
    }
};

/*
 * @lc app=leetcode.cn id=76 lang=cpp
 *
 * [76] 最小覆盖子串
 */

// @lc code=start
class Solution {
public:
    string minWindow(string s, string t) {
        if(s.length() < t.length()) return {};
#ifdef USE_HASHMAP
        unordered_map<char, int> need;   // 存储t中各字符的出现次数
        unordered_map<char, int> window; // 存储当前窗口中各字符的出现次数
#else
        int need[128] = {0};  
        int window[128] = {0};
#endif
        // 统计t中的字符需求
        for (char c : t) ++need[c];
        
        int left = 0, right = 0;         // 滑动窗口的左右指针
        int valid = 0;                   // 记录窗口中满足need条件的字符种类数
        int start = 0;                   // 最小覆盖子串的起始索引
        int min_len = INT_MAX;           // 最小覆盖子串的长度

#ifndef USE_HASHMAP
        int required = 0;                // 记录t中不同的字符种类数
        for (int i = 0; i < 128; ++i) {
            if (need[i] > 0) ++required;
        }
#endif
        while (right < s.size()) {
            // 扩大右边界，将字符加入窗口
            char c = s[right];
            ++right;
            // 若当前字符是t中的字符，更新窗口计数
#ifdef USE_HASHMAP
            if (auto fc = need.find(c); fc != need.end()) {
#else
            if (need[c] > 0) {
#endif
                ++window[c];
                // 若当前字符数量达到需求，有效种类+1
#ifdef USE_HASHMAP
                if (window[c] == fc->second) {
#else
                if (window[c] == need[c]) {
#endif
                    ++valid;
                }
            }
            
            // 当窗口满足所有字符需求时，尝试收缩左边界
#ifdef USE_HASHMAP
            while (valid == need.size()) {
#else
            while (valid == required) {
#endif
                // 更新最小窗口
                if (right - left < min_len) {
                    start = left;
                    min_len = right - left;
                }
                // 移出左边界的字符
                char d = s[left];
                ++left;
#ifdef USE_HASHMAP
                // 若移出的字符是t中的字符，更新窗口计数
                if (auto fd = need.find(d); fd != need.end()) {
                    // 若移出前该字符数量正好满足需求，有效种类-1
                    if (window[d] == fd->second) {
#else
                if (need[d] > 0) {
                    if (window[d] == need[d]) {
#endif
                        --valid;
                    }
                    --window[d];
                }
            }
        }
        // 返回最小覆盖子串，若不存在则返回空字符串
        return min_len == INT_MAX ? std::string{} : s.substr(start, min_len);
    }
};
// @lc code=end
/*
 * @lc app=leetcode.cn id=76 lang=cpp
 *
 * [76] 最小覆盖子串
 */

// @lc code=start
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <ranges>
#include <deque>

using namespace std;
class Solution {
public:
    /*
    * 输入：s = "ADOBECODEBANCC", t = "ABCC"
    * 输出："BANCC"
    * 解释：最小覆盖子串 "BANCC" 包含来自字符串 t 的 'A'、'B' 和 'C'。
    * A 0 10
    * B 3 9
    * C 5 12 13
    * A B C B A  C  C
    * 0 3 5 9 10 12 13
    * ABCC
    * 
    * 输入：s = "cabwefgewcwaefgcf", t = "cae"
    * 输出："cae"
    * 解释：最小覆盖子串 "cae" 包含来自字符串 t 的 'c'、'a' 和 'e'。
    * a 1 11
    * c 0 9 15
    * e 4 7 12
    * c a e e c a  e  c
    * 0 1 4 7 9 11 12 15
    统计字符需求：使用哈希表need记录字符串t中每个字符的出现次数。
    初始化滑动窗口：使用left和right双指针表示窗口的左右边界，valid变量记录当前窗口中满足t需求的字符种类数。
    扩大右边界：遍历字符串s，将字符纳入窗口。若该字符在t中，则更新窗口计数。当某个字符的数量恰好满足需求时，valid加1。
    收缩左边界：当窗口满足所有字符需求时（valid == need.size()），尝试左移左边界以缩小窗口，寻找更小的覆盖子串。每次左移时，若移出的是t中的字符，则更新窗口计数，并在其数量不足需求时减少valid。
    更新最小窗口：在每次收缩左边界时，检查当前窗口是否更小，更新最小窗口的起始位置和长度。
    */
   string minWindow(string s, string t) {
        if(s.length() < t.length()) return {};
        unordered_map<char, int> need;   // 存储t中各字符的出现次数
        unordered_map<char, int> window; // 存储当前窗口中各字符的出现次数
        // 统计t中的字符需求
        for (char c : t) ++need[c];
        
        int left = 0, right = 0;         // 滑动窗口的左右指针
        int valid = 0;                   // 记录窗口中满足need条件的字符种类数
        int start = 0;                   // 最小覆盖子串的起始索引
        int min_len = INT_MAX;           // 最小覆盖子串的长度
        
        while (right < s.size()) {
            // 扩大右边界，将字符加入窗口
            char c = s[right];
            ++right;
            // 若当前字符是t中的字符，更新窗口计数
            if (auto fc = need.find(c); fc != need.end()) {
                ++window[c];
                // 若当前字符数量达到需求，有效种类+1
                if (window[c] == fc->second) {
                    ++valid;
                }
            }
            
            // 当窗口满足所有字符需求时，尝试收缩左边界
            while (valid == need.size()) {
                // 更新最小窗口
                if (right - left < min_len) {
                    start = left;
                    min_len = right - left;
                }
                // 移出左边界的字符
                char d = s[left];
                ++left;
                // 若移出的字符是t中的字符，更新窗口计数
                if (auto fd = need.find(d); fd != need.end()) {
                    // 若移出前该字符数量正好满足需求，有效种类-1
                    if (window[d] == fd->second) {
                        --valid;
                    }
                    --window[d];
                }
            }
        }
        // 返回最小覆盖子串，若不存在则返回空字符串
        return min_len == INT_MAX ? std::string{} : s.substr(start, min_len);
    }
    /*
    string minWindow(string s, string t) {
        const size_t t_length = t.size();
        const size_t s_length = s.size();
        if (s_length < t_length) return {};
        if (s == t) return s;
        unordered_set<char> set_with_t{ t.begin(), t.end() };
        vector<pair<char, size_t>> s_filtered{};
        s_filtered.reserve(s_length);
        for (auto&& [i, c] : s | views::enumerate)
        {
            if (!set_with_t.contains(c)) continue;
            s_filtered.emplace_back(c, i);
        }
        string result{};
        array<int, 26> pattern_window{};
        array<int, 26> main_window{};

        for (auto&& [i, c] : t | views::enumerate) {
            ++pattern_window[c - 'a'];                  // 初始化模式串计数
            ++main_window[s_filtered[i].first - 'a'];   // 初始化窗口计数
        }
        if (main_window == pattern_window) result = s.substr(0, s_filtered[t_length - 1].second + 1);
        // 滑动窗口
        for (int i = t_length; i < s_filtered.size(); ++i) {
            --main_window[s_filtered[i - t_length].first - 'a'];           // 移出左边界字符
            ++main_window[s_filtered[i].first - 'a'];                       // 移入右边界字符
            if (main_window == pattern_window) {
                size_t left = s_filtered[i - t_length + 1].second;
                size_t n = s_filtered[i].second - left + 1;
                if()
                result = s.substr(left, n);
            }
        }
        return result;
    }
    */
    /*
    // 只能处理左右两侧边界，思考情况不完全。
    string minWindow(string s, string t) {
        size_t t_length = t.size();
        if (s.size() < t_length) return {};
        if (s == t) return s;
        unordered_map<char, size_t> statistical_dict;
        statistical_dict.reserve(t_length > 51 ? 52 : t_length);
        for (char c : t) statistical_dict[c] = statistical_dict.contains(c) ? statistical_dict[c] + 1 : 1;
        unordered_map<char, deque<size_t>> location_dict;
        unordered_map<char, deque<size_t>> location_dict_copy;
        location_dict.reserve(statistical_dict.size());
        deque<size_t> window;
        for (auto&& [i, c] : s | views::enumerate)
        {
            if (!statistical_dict.contains(c)) continue;
            location_dict[c].push_back(i);
            location_dict_copy[c].push_back(i);
            window.push_back(i);
        }
        if (window.empty()) return {};
        const size_t left = 0;
        const size_t right = window.back();
        size_t pointer = 0;
        string min_string{};
        for (auto it = window.begin(); it != window.end(); ++it) {
            char c = s[*it];
            location_dict[c].pop_front();
            pointer = *it;
            if (location_dict[c].size() < statistical_dict[c]) {
                break;
            }
        };
        min_string = s.substr(pointer, right - pointer + 1);
        for (auto it = window.rbegin(); it != window.rend(); ++it) {
            char c = s[*it];
            location_dict_copy[c].pop_back();
            pointer = *it;
            if (location_dict_copy[c].size() < statistical_dict[c]) {
                break;
            }
        }
        min_string = s.substr(left, pointer + 1);
        return (pointer + 1 - left) > min_string.size() ? min_string : s.substr(left, pointer + 1);
    }
    */
};
// @lc code=end
int main()
{
    Solution s;
    s.minWindow("cabwefgewcwaefgcf", "cae");
    return 0;
}
