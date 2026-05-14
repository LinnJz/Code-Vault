bool isValid(string s) {
	if (s.size() & 1) return false;
	static const std::unordered_map<char, char> bracketMap = {
		{')', '('}, {']', '['},  {'}', '{'}
	};

	int top = -1;
	for (char c : s) {
		if (auto it = bracketMap.find(c); it == bracketMap.end()) s[++top] = c;
		else if (top == -1 || s[top--] != it->second) return false;
	}

	return top == -1;
}

bool isValid(string s) {
    // 1. 快速剪枝：奇数长度直接返回false（不可能成对）
    if (s.size() & 1) return false;
    unordered_map<char, char> bracketMap = {
        {')', '('},
        {']', '['},
        {'}', '{'}
    };

    int top = -1; // 模拟栈顶指针（-1=栈空）
    for (char c : s) {
        // 左括号：入栈（top指针上移，当前位置存左括号）
        if (bracketMap.find(c) == bracketMap.end()) {
            s[++top] = c; // 原地存储，无需额外栈
        } 
        // 右括号：检查匹配
        else {
            // 栈空 或 栈顶不匹配 → 无效
            if (top == -1 || s[top] != bracketMap[c]) {
                return false;
            }
            --top; // 匹配成功，栈顶指针下移（模拟出栈）
        }
    }

    return top == -1; // 栈空则全部匹配
}


// 第二次写太乱了
bool isValid(string s) {
    std::stack<char> stk;
    for(int i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '(' || c == '[' || c == '{') {
            stk.emplace(c);
        }
        else {
            if(stk.empty()) {
                return false;
            }
            else {
                char t = stk.top();
                if ((t == '(' && c == ')') || (t == '[' && c == ']') || (t == '{' && c =='}')) {
                    stk.pop();
                }
                else {
                    return false;
                }
            }
        }
    }
    return stk.size() == 0 ? true : false;
}
/*
 * @lc app=leetcode.cn id=20 lang=cpp
 *
 * [20] 有效的括号
 */

// @lc code=start
#include <stack>
using namespace std;
class Solution {
public:
    bool isValid(string s) {
        /*
        if (char top = brackets.top(); 
                top == c - 1 || top == c - 2) 
            {
                brackets.pop();
        }
        */
        /*
        stack<char, vector<char>> brackets;
        for(char c : s) {
            if(brackets.empty() || c == '(' || c == '{' || c == '[') {
                brackets.emplace(c);
                continue;
            }
            char top = brackets.top();
            if (c == ')' && top == '(') {
                brackets.pop();
            }
            else if (c == '}' && top == '{') {
                brackets.pop();
            }
            else if (c == ']' && top == '['){
                brackets.pop();
            }
            else {
                return false;
            }
        }
        return brackets.empty();
        */
        stack<char> brace;
        for (const char& c : s) {
            if (!brace.empty() && (brace.top() == c - 1 || brace.top() == c - 2))
                brace.pop();
            else brace.push(c);
        }
        return !brace.size();
    }
};
// @lc code=end
int main(){
    Solution s;
    string str = "()[]{}}";
    s.isValid(str);
    return 0;
}
