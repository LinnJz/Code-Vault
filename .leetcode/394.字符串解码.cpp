string decodeString(string const &s) {
    struct BracketStr {
        size_t repeat;
        size_t prefixLen;
    };
    size_t size = s.size(), digit = 0;

    // 栈数组模拟，替代 std::vector<BracketStr>
    BracketStr* stack = reinterpret_cast<BracketStr*>(::alloca(sizeof(BracketStr) * size));
    int top = 0;  // 栈顶指针，指向下一个空闲位置

    std::string decodeStr; decodeStr.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        if (char c = s[i]; c >= '0' && c <= '9') {
            digit = digit * 10 + c - '0';
        }
        else if (c == '[') {
            stack[top++] = {digit, decodeStr.size()};  // 入栈
            digit = 0;
        }
        else if (c == ']') {
            --top;  // 出栈
            auto [repeat, prefixLen] = stack[top];
            for (size_t bracketStrSize = decodeStr.size() - prefixLen, start = 0; start < repeat - 1; ++start) {
                decodeStr.append(decodeStr.substr(prefixLen, bracketStrSize));
            }
        } 
        else {
            decodeStr.push_back(c);
        }
    }
    return decodeStr;
}
class Solution {
public:
    string decodeString(string s) {
        // 栈元素：存储【重复次数】和【当前已解码字符串的前缀长度】
        struct BracketStr {
            int repeat{0};       // 重复次数
            int prefix_len{0};   // 到'['为止，已解码字符串的长度（前缀长度）
        };
        std::vector<BracketStr> stk; 
        stk.reserve(s.size());  // 预分配空间，优化性能
        std::string decode_str;        
        decode_str.reserve(s.size() << 1); // 预分配足够空间
        int curr_num = 0;

        #pragma clang loop unroll_count(4)
        for (int i = 0; i < s.size(); ++i) {
            // 处理数字（修复原代码digit < 9的bug，改为<=9）
            if (char c = s[i]; c >= '0' && c <= '9') {
                digit = digit * 10 + c - '0';
            }
            // 处理左括号：记录重复次数 + 当前已解码字符串的前缀长度
            else if (c == '[') {
                stk.emplace_back(curr_num, static_cast<int>(decode_str.size()));
                curr_num = 0; // 重置数字，准备处理括号内的内容
            }
            // 处理右括号：解码当前层级的内容（核心逻辑）
            else if (c == ']') {
                auto [repeat, prefix_len] = stk.back();
                stk.pop_back();
                
                /*
                // 步骤1：提取需要重复的子串（前缀长度后的部分）
                std::string sub = decode_str.substr(prefix_len);
                // 移动char，相当于没移动
                //std::string sub(std::make_move_iterator(decode_str.begin() + prefix_len),
                //    std::make_move_iterator(decode_str.end()));
                // 步骤2：截断到前缀长度，清空待重复的部分
                decode_str.resize(prefix_len);
                // 步骤3：重复指定次数并追加到已解码字符串
                for (int count = 0; count < repeat; ++count) {
                    decode_str += sub;
                }
                */
                // 减一 优化写法
                std::string sub = decode_str.substr(prefix_len);
                #pragma clang loop unroll_count(4)
                for (int count = 0; count < repeat - 1; ++count) decode_str.append(sub);
                
                // 性能高效，但是必须确保reserve容量充足，否则扩容视图索引失效
                // std::string_view suffix(decode_str.data() + prefix_len, decode_str.size() - prefix_len);
                // for (int count = 0; count < repeat - 1; ++count) decode_str.append(suffix);
            }
            // 处理普通字符：直接追加到已解码字符串
            else {
                decode_str.push_back(c);
            }
        }
        return decode_str;
    }
};

/*
 * @lc app=leetcode.cn id=394 lang=cpp
 *
 * [394] 字符串解码
 */

// @lc code=start
#include <stack>
#include <ranges>
class Solution {
public:
    std::string decodeString(std::string s) {
        struct DecodeStringData {
            int bra_str_repeat;
            std::string bra_prev_str;
        };
        std::stack<DecodeStringData> stk; // 存储(重复次数, 前序字符串)
        std::string decode_str; decode_str.reserve(16);
        int curr_num{ 0 };
        #pragma GCC unroll 4
        for(char c : s)
        {
            if(int digit = c - '0'; digit > -1 && digit < 10) {
                curr_num = curr_num * 10 + digit; // 处理多位数
            }
            else if (c == '[') {
                // 压栈并重置状态
                stk.push({curr_num, decode_str});
                curr_num = 0;
                decode_str.clear();
            }
            else if (c == ']') {
                // auto & dec_str_data = stk.top();
                // int repeat = dec_str_data.bra_str_repeat;

                // // 生成重复字符串并拼接前序字符串
                // std::string repeat_str; repeat_str.reserve(repeat * decode_str.size());
                // #pragma GCC unroll 4
                // for(int i = 0; i < repeat; ++i) repeat_str += decode_str;
                // decode_str = std::move(dec_str_data.bra_prev_str + repeat_str);

                // stk.pop();
                auto [repeat, prev_str] = std::move(stk.back());
                stk.pop_back();

                std::string repeat_str; 
                repeat_str.reserve(prev_str.size() + repeat * decode_str.size());
                repeat_str += prev_str;
                #pragma GCC unroll 4
                for(int i = 0; i < repeat; ++i) repeat_str += decode_str;
                decode_str = std::move(repeat_str);
            }
            else {
                decode_str.push_back(c); // 收集当前层字符
            }
        }

        return decode_str;
    }
};

// @lc code=end

/*仅能处理非嵌套的，嵌套的逻辑稍有问题，有豆包完成的思路代码见上*/
std::string decodeString(const std::string& s) {
    struct BracketStr {
        int repeat{ 0 };
        short position{ 0 };
        bool is_nested{ false };
    };
    std::vector<BracketStr> stk; stk.reserve(s.size() >> 1);
    std::string decode_str; decode_str.reserve(s.size() << 1);
    int curr_num = 0;
    bool is_out_bracket = true;
    struct A {
        short first{ 0 };
        short last{ 0 };
    } a;
    bool isNested = false;
    std::string nested_str;
    for (int i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (int digit = c - '0'; digit >= 0 && digit < 9) {
            curr_num = curr_num * 10 + digit;
            is_out_bracket = false;
        }
        else if (c == '[') {
            stk.emplace_back(curr_num, i + 1, !stk.empty());
            curr_num = 0;
        }
        else if (c == ']') {
            auto [repeat, position, is_nested] = stk.back();
            if (isNested) {
                nested_str = s.substr(position, a.first - position) + nested_str + s.substr(a.last, i - a.last);
                for (int count = 0; count < repeat; ++count) decode_str.append(nested_str);
            }
            else {
                if (is_nested) {
                    a.first = position - 1 - std::to_string(repeat).size();
                    a.last = i + 1;
                    isNested = is_nested;
                    for (int count = 0; count < repeat; ++count) nested_str.append(s.substr(position, i - position));
                }
                else {
                    for (int count = 0; count < repeat; ++count) decode_str += s.substr(position, i - position);
                }
            }
            stk.pop_back();
            isNested = !stk.empty();
            is_out_bracket = stk.empty();
        }
        else if (is_out_bracket) {
            decode_str.push_back(c);
        }
    }
    return decode_str;
}
