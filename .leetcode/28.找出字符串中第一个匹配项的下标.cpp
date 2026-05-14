int strStr(string const &haystack, string const &needle) {
	if (haystack.size() < needle.size()) return -1;

	for (int i = 0; i <= haystack.size() - needle.size(); ++i) {
		if (std::string_view { haystack.data() + i, needle.size() } == needle) {
			return i;
		}
	}
	return -1;
}
#include<string>
/*
 * @lc app=leetcode.cn id=28 lang=cpp
 *
 * [28] 找出字符串中第一个匹配项的下标
 */

// @lc code=start
class Solution {
public:
// 暴力
    int strStr(string haystack, string needle) {
        if (haystack.size() < needle.size()) return -1;
        int pos = 0;
        for ( ; pos < haystack.size(); ++pos) {
            int left = pos, right = 0;

            for ( ; left < haystack.size() && right < needle.size(); ++left) {
                if (haystack[left] != needle[right]) break;
                ++right;
            }
            if (right == needle.size()) return left - right;
        }
        return -1;
    }
// 暴力稍微优化
    int strStr(string haystack, string needle) {
        const int hay_len = haystack.size();
        const int need_len = needle.size();
        
        if (hay_len < need_len) return -1;
        // 外层循环仅遍历到剩余长度 ≥ need_len 的位置，消除冗余
        for (int pos = 0; pos <= hay_len - need_len; ++pos) {
            int right = 0;
            // 内层先比对字符，再递增，减少多余的边界判断
            for (; right < need_len; ++right) {
                if (haystack[pos + right] != needle[right]) {
                    break;
                }
            }
            if (right == need_len) {
                return pos; // 直接返回起始下标，无需计算 left - right
            }
        }
        return -1;
    }
// 标准库字符串自带
    int strStr(string haystack, string needle) {
        auto pos = haystack.find(needle);
        return std::string::npos == pos ? -1 : pos;
    }
// 标准库算法，
    int strStr(string haystack, string needle) {
        auto pos = std::search(text.begin(), text.end(), std::default_searcher(needle.begin(), needle.end()));
        return pos != haystack.end() ? std::distance(haystack.begin(), pos) : -1;
    }
// KMP
    int strStr(const string& haystack, const string& needle) {
        const size_t mainLen = haystack.size(), patternLen = needle.size();
        if (mainLen < patternLen) [[unlikely]] return -1;

        std::vector<int> next(patternLen, 0);

        for (int i = 1, j = 0; i < patternLen; ++i) {
            while (j > 0 && needle[i] != needle[j]) j = next[j - 1];

            if (needle[i] == needle[j]) ++j;

            next[i] = j;
        }

        for (int i = 0, j = 0; i < mainLen; ++i) {
            while (j > 0 && haystack[i] != needle[j]) j = next[j - 1];

            if (haystack[i] == needle[j]) ++j;

            if (j == patternLen) return i - patternLen + 1;
        }
        return -1;
    }

private:
// Sunday 坏字符表实现
template <class Value_t> struct Sunday_bad_char_table {
  static constexpr size_t Table_size = 256; // 针对 char/unsigned char

  template <class RanItPat>
  static void Build_table(
      typename std::iterator_traits<RanItPat>::difference_type *Table,
      const RanItPat Pat_first,
      const typename std::iterator_traits<RanItPat>::difference_type Pat_size) {
    // 初始化所有条目为模式长度 + 1
    using Diff = typename std::iterator_traits<RanItPat>::difference_type;
    const auto Default_shift = Pat_size + 1;
    for (size_t Idx = 0; Idx < Table_size; ++Idx) {
      Table[Idx] = Default_shift;
    }

    // 填充实际的偏移值
    for (Diff Idx = 0; Idx < Pat_size; ++Idx) {
      const auto Char_val = static_cast<unsigned char>(*(Pat_first + Idx));
      Table[Char_val] = Pat_size - Idx;
    }
  }
};

// 验证迭代器范围有效性（跨平台实现）
template <class RanIt> void Adl_verify_range(RanIt first, RanIt last) {
  static_assert(std::random_access_iterator<RanIt>,
                "Sunday search requires random access iterators");
  if (first > last) {
    throw std::invalid_argument("Invalid iterator range: first > last");
  }
}

// 获取迭代器底层指针（跨平台实现）
template <class RanIt> auto Get_unwrapped(RanIt it) -> decltype(&*it) {
  return &*it;
}

// 从底层指针调整迭代器位置（跨平台实现）
template <class RanIt, class Pointer>
void Seek_wrapped(RanIt &it, Pointer ptr, RanIt base) {
  using Diff = typename std::iterator_traits<RanIt>::difference_type;
  it = base + static_cast<Diff>(ptr - Get_unwrapped(base));
}

template <class RanItHaystack, class RanItPat, class Pred_eq>
std::pair<RanItHaystack, RanItHaystack> Sunday_search(
    const RanItPat Pat_first,
    const typename std::iterator_traits<RanItPat>::difference_type Pat_size,
    RanItHaystack First, RanItHaystack Last, Pred_eq &Eq) {
  // Sunday 字符串搜索算法
  using ValuePat = typename std::iterator_traits<RanItPat>::value_type;
  using ValueHay = typename std::iterator_traits<RanItHaystack>::value_type;
  static_assert(std::is_same_v<ValuePat, ValueHay>,
                "sunday_search requires matching iterator value types");
  using Diff = typename std::iterator_traits<RanItPat>::difference_type;

  Adl_verify_range(First, Last);
  auto UFirst = Get_unwrapped(First);
  const auto ULast = Get_unwrapped(Last);

  if (Pat_size == 0) { return {First, First}; }

  const auto UPat_first = Get_unwrapped(Pat_first);
  const auto Haystack_size = static_cast<Diff>(ULast - UFirst);

  if (Haystack_size < Pat_size) {
    Seek_wrapped(Last, ULast, First);
    Seek_wrapped(First, ULast, First);
    return {First, Last};
  }

  // 构建坏字符表
  Diff Bad_char_table[Sunday_bad_char_table<ValuePat>::Table_size];
  Sunday_bad_char_table<ValuePat>::Build_table(Bad_char_table, Pat_first,
                                               Pat_size);

  auto Text_pos = UFirst;

  while (Text_pos <= ULast - Pat_size) {
    // 比较模式与当前窗口
    Diff Match_pos = 0;
    while (Match_pos < Pat_size &&
           Eq(*(UPat_first + Match_pos), *(Text_pos + Match_pos))) {
      ++Match_pos;
    }

    if (Match_pos == Pat_size) {
      // 找到匹配
      Seek_wrapped(Last, Text_pos + Pat_size, First);
      Seek_wrapped(First, Text_pos, First);
      return {First, Last};
    }

    // 使用 Sunday 算法计算偏移
    const auto Next_pos = Text_pos + Pat_size;
    if (Next_pos < ULast) {
      const auto Next_char = static_cast<unsigned char>(*Next_pos);
      Text_pos += Bad_char_table[Next_char];
    } else {
      break; // 没有更多字符可检查
    }
  }

  Seek_wrapped(Last, ULast, First);
  Seek_wrapped(First, ULast, First);
  return {First, Last};
}

template <class RanItPat, class Pred_eq = std::equal_to<>>
class sunday_searcher {
public:
  sunday_searcher(const RanItPat First, const RanItPat Last,
                  Pred_eq Eq = Pred_eq())
      : Pat_first(First),
        Pat_size(static_cast<
                 typename std::iterator_traits<RanItPat>::difference_type>(
            Get_unwrapped(Last) - Get_unwrapped(First))),
        Eq(std::move(Eq)) {
    // 预处理模式，用于 Sunday 字符串搜索算法
    Adl_verify_range(First, Last);
  }

  template <class RanItHaystack>
  [[nodiscard]] std::pair<RanItHaystack, RanItHaystack>
  operator()(const RanItHaystack First, const RanItHaystack Last) const {
    // 在 [First, Last) 中搜索预处理的模式
    if (Pat_size == 0) { return {First, First}; }

    return Sunday_search(Pat_first, Pat_size, First, Last, Eq);
  }

private:
  RanItPat Pat_first;
  typename std::iterator_traits<RanItPat>::difference_type Pat_size;
  Pred_eq Eq;
};
};
// @lc code=end

