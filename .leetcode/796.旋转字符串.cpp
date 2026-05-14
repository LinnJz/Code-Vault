/*
 * @lc app=leetcode.cn id=796 lang=cpp
 *
 * [796] 旋转字符串
 */

// @lc code=start
class Solution {
public:
    bool rotateString(string const &s, string const &goal) noexcept {
        if (s.size() != goal.size()) return false;

        char *buffer = reinterpret_cast<char *>(::alloca(2 * s.size() + 1 * sizeof(char)));

        ::memcpy(buffer, s.data(), s.size() * sizeof(char));
        ::memcpy(buffer + s.size(), s.data(), s.size() * sizeof(char));
        buffer[2 * s.size()] = '\0';

        return ::strstr(buffer, goal.c_str()) != nullptr;
    }
};
// @lc code=end

