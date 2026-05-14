/*
 * @lc app=leetcode.cn id=93 lang=cpp
 *
 * [93] 复原 IP 地址
 * 
 */
/*
    4 全1   1种情况
    5 三个1 一个2       4种情况
    6 三个1 一个3       4种情况  | 二个1 二个2  1122 1212 1221 2112 2121 2211 6种情况
    7 二个1 一个2 一个3     1123 1132 1231 1213 1312 1321 2113 2131 2311 3211 3121 3112 12种情况
    8 二个1 二个3     6种情况     | 四个2 1种情况 | 一个1 二个2 一个3 12种情况
    9 一个1 一个2 二个3   12种情况  | 三个2 一个3 4种情况
    10 一个1 三个3 4种情况       | 二个2 二个3    6种情况
    11 一个2 三个3 4种情况
    12 四个3    1种情况

    如果有前缀0但不是0，IP地址格式错误
*/
// @lc code=start
class Solution {
public:
    std::vector<std::string> restoreIpAddresses(std::string s) {
        int len = s.length();
        if (len < 4 || len > 12) return {};
        
        std::vector<std::string> result; result.reserve(4);
        
		// 代码缺陷，没有处理len < k的情况，使得isValid必须使用segment.empty()判断，实际上没必要
		// 但因为没处理 len < k ,相减后是一个极大的无符号正整数，segment.empty()恰好掩盖了这个错误，能够AC
		// 属于是误打误撞，正确写法见下面注释，当然也可以像以下使用&& i < len;等 for条件处理更好
		
		
        // 枚举所有可能的分割点， for循环的ijk是字符串大小
        for (int i = 1; i <= 3 && i < len; ++i) {
            for (int j = i + 1; j <= i + 3 && j < len; ++j) {
                for (int k = j + 1; k <= j + 3 && k < len; ++k) {
					/*
		for (int i = 1; i <= 3; ++i) {
            for (int j = i + 1; j <= i + 3; ++j) {
                for (int k = j + 1; k <= j + 3; ++k) {
                    int len4 = len - k; 
                    if (len4 < 1 || len4 > 3) continue; // 最后一段长度必须合法
					
					*/
                    // 使用string_view避免拷贝
                    std::string_view part1(s.data(), i);
                    std::string_view part2(s.data() + i, j - i);
                    std::string_view part3(s.data() + j, k - j);
                    std::string_view part4(s.data() + k, len - k);
                    
                    if (isValid(part1) && isValid(part2) && isValid(part3) && isValid(part4)) {
                        // 直接构造结果字符串，避免中间string创建
                        result.push_back(buildIp(part1, part2, part3, part4));
                    }
                }
            }
        }
        
        return result;
    }

private:
    bool isValid(std::string_view segment) {
		/*
        // 段长度必须在1-3之间，外层调用已处理
        if (segment.empty() || segment.length() > 3) return false;
        */
		
		
		// 不能有前导0，除非段本身就是"0"
		// 此时 segment 长度必为 1~3 ，无需检查 empty()
        if (segment[0] == '0' && segment.length() > 1) return false;
        
        // 必须能转换为0-255之间的数字
        int num = 0;
        for (char c : segment) {
            if (c < '0' || c > '9') return false;
            num = num * 10 + (c - '0');
        }
        return num >= 0 && num <= 255;
    }
    
    std::string buildIp(std::string_view part1, std::string_view part2, 
                       std::string_view part3, std::string_view part4) {
        // 预分配足够空间，避免重新分配
        std::string ip;
        ip.reserve(part1.length() + part2.length() + part3.length() + part4.length() + 3);
        
        // 直接追加数据，避免创建临时字符串
        ip.append(part1.data(), part1.length());
        ip += '.';
        ip.append(part2.data(), part2.length());
        ip += '.';
        ip.append(part3.data(), part3.length());
        ip += '.';
        ip.append(part4.data(), part4.length());
        
        return ip;
    }
};
// @lc code=end

class Solution {
public:
    vector<string> restoreIpAddresses(string s) {
        vector<string> result;
        vector<int> path;  // 存储分割点的位置
        
        backtrack(s, 0, 0, path, result);
        return result;
    }
    
private:
    void backtrack(const string& s, int start, int segments, vector<int>& path, vector<string>& result) {
        // 已经找到3个分割点（即有4段）
        if (segments == 3) {
            // 检查最后一段是否有效
            if (isValid(s, start, s.length())) {
                // 构建IP地址
                string ip = s.substr(0, path[0]) + "." +
                           s.substr(path[0], path[1] - path[0]) + "." +
                           s.substr(path[1], path[2] - path[1]) + "." +
                           s.substr(path[2]);
                result.push_back(ip);
            }
            return;
        }
        
        // 尝试不同的分割位置
        for (int i = 1; i <= 3; i++) {
            int end = start + i;
            if (end > s.length()) break;
            
            if (isValid(s, start, end)) {
                path.push_back(end);
                backtrack(s, end, segments + 1, path, result);
                path.pop_back();  // 回溯
            }
        }
    }
    
    bool isValid(const string& s, int start, int end) {
        int len = end - start;
        if (len <= 0 || len > 3) return false;
        
        // 前导0检查
        if (s[start] == '0' && len > 1) return false;
        
        // 数值范围检查
        int num = 0;
        for (int i = start; i < end; i++) {
            num = num * 10 + (s[i] - '0');
        }
        return num <= 255;
    }
};