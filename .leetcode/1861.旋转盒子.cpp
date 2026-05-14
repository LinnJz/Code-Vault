
/*
 * 解法三（直接在结果矩阵上模拟下落，空间复杂度 O(1)）：
 *
 * 核心思路：利用旋转对应关系，在结果矩阵的每一列上直接模拟重力下落。
 * 1. 建立 n×m 结果矩阵 ans 并初始化为 '.'。
 * 2. 原矩阵的第 i 行旋转后变为 ans 的第 (m-1-i) 列，原行中列索引 j 变为 ans 中的行索引 j。
 *    因此遍历结果矩阵的每一列 c（对应原行 i = m-1-c），使用指针 writeRow 从底部 (n-1) 
 *    向上标识可放置石头的行。
 * 3. 对该列，从原行的最右侧向左遍历（j 从 n-1 到 0），等价于在 ans 的该列中从下向上处理：
 *    - 若 box[i][j] == '*'：将障碍物放到 ans[j][c]，并将 writeRow 跳到其上方 j-1。
 *    - 若 box[i][j] == '#'：将石头放入 ans[writeRow][c]，writeRow 上移。
 *    - 空位 '.' 无操作（ans 默认即为空）。
 * 4. 遍历完所有列后，ans 即为最终结果。
 *
 * 额外空间：仅常数个变量，O(1)。
 */
vector<vector<char>>
rotateTheBox(vector<vector<char>> &box)
{
  int m = box.size(), n = box[0].size();
  // ans 为 n x m，全部初始化为 '.'
  vector<vector<char>> ans(n, vector<char>(m, '.'));

  // 结果矩阵的每一列 c，对应原矩阵的第 i = m - 1 - c 行
  for (int c = 0; c < m; ++c) // ans的列
  {
    int i        = m - 1 - c; // 对应的原行号，ans第0列对应box的最后一行
    int writeRow = n - 1;     // 当前列可放石头的底部行
    // 从原行的最右侧向左遍历，对应 ans 列从下向上
    for (int j = n - 1; j >= 0; --j) // ans的行
    {
      if (box[i][j] == '*')
      {
        ans[j][c] = '*';   // 障碍物直接放到旋转后位置
        writeRow  = j - 1; // 后续石头只能落在障碍物上方
      }
      else if (box[i][j] == '#')
      {
        ans[writeRow][c] = '#'; // 石头落到当前可放位置
        --writeRow;
      }
      // '.' 无需处理，ans 默认就是 '.'
    }
  }
  return ans;
}

/*
 * 解法二（原地压实 + 旋转，空间复杂度 O(1)）：
 *
 * 核心思路：先在原矩阵的每一行内模拟“石头向右沉降”，再统一旋转得到答案。
 * 1. 逐行处理，使用指针 writePos 从右向左指示当前可放置石头的位置。
 *    - 遇到障碍物 '*'：writePos 重置为障碍物左侧一列。
 *    - 遇到石头 '#'：将其移到 writePos 处（原位置放 '.'），writePos 左移。
 *    - 空位 '.' 忽略。
 *    遍历完一行后，该行所有石头均已靠右堆积，障碍物保持原位。
 * 2. 对整个 box 完成上述压实后，按照旋转公式 (i, j) -> (j, m-1-i) 填入新矩阵 ans。
 * 3. 返回 ans。
 *
 * 额外空间：除结果矩阵外只使用了常数个变量，O(1)。
 */
vector<vector<char>>
rotateTheBox(vector<vector<char>> &box)
{
  int m = box.size(), n = box[0].size();
  // 1. 每一行向右压实
  for (int i = 0; i < m; ++i)
  {
    int writePos = n - 1; // 当前可放置石头的列
    for (int j = n - 1; j >= 0; --j)
    {
      if (box[i][j] == '*')
      {
        writePos = j - 1; // 障碍物不能跨越
      }
      else if (box[i][j] == '#')
      {
        box[i][j]        = '.'; // 原来的位置清空
        box[i][writePos] = '#'; // 放到堆积位置
        --writePos;
      }
    }
  }
  // 2. 顺时针旋转 90° 填入答案
  vector<vector<char>> ans(n, vector<char>(m));
  for (int i = 0; i < m; ++i)
    for (int j = 0; j < n; ++j)
      ans[j][m - 1 - i] = box[i][j];
  return ans;
}
/*
 * @lc app=leetcode.cn id=1861 lang=cpp
 *
 * [1861] 旋转盒子
 */
 
// @lc code=start

class Solution
{
public:
  /*
 * 解法一（区间统计法，空间复杂度 O(m*n)）：
 *
 * 核心思路：将重力下落转化为“每一行内石头向右侧障碍物或边界堆积”。
 * 1. 遍历原矩阵每一行，用障碍物（及虚拟边界 -1 和 n）将行分割成若干连续区间。
 *    对每个区间记录其左右边界及区间内的石头数量（结构体 Interval）。
 * 2. 建立临时矩阵 tempBox（初始全 '.'），并将障碍物 '*' 直接复制过去。
 * 3. 根据区间信息，在 tempBox 的每个区间内，从右边界向左填充对应数量的石头 '#'。
 * 4. 将 tempBox 顺时针旋转 90 度，即 (i, j) -> (j, m-1-i)，填入结果矩阵 ans 并返回。
 *
 * 额外空间：rowIntervals 数组最坏 O(m*n)，tempBox 为 O(m*n)，总额外 O(m*n)。
 */
  vector<vector<char>> rotateTheBox(vector<vector<char>> &box)
  {
    int m = box.size(), n = box[0].size();

    // 存储每一行的区间信息：每个区间 [start, end) 和该区间内的石头数
    struct Interval
    {
      int start;  // 区间左边界（包含），-1 代表虚拟左障碍物
      int end;    // 区间右边界（不包含），n 代表虚拟右障碍物
      int stones; // 该区间内 '#' 的数量
    };

    vector<vector<Interval>> rowIntervals(m); // 每行多个区间

    // 1. 扫描每一行，构建区间
    for (int i = 0; i < m; ++i)
    {
      int lastObstacle = -1; // 上一个障碍物的列索引（虚拟左边界为 -1）
      int stoneCount   = 0;
      for (int j = 0; j <= n; ++j)
      {
        if (j == n || box[i][j] == '*')
        {
          // 遇到障碍物或右边界，保存区间
          rowIntervals[i].push_back({ lastObstacle, j, stoneCount });
          lastObstacle = j;
          stoneCount   = 0;
        }
        else if (box[i][j] == '#')
        {
          ++stoneCount;
        }
      }
    }

    // 2. 构建旋转后的结果矩阵
    vector<vector<char>> ans(n, vector<char>(m, '.'));

    // 对于原矩阵每个格子 (i, j)，旋转后位置为 (j, m-1-i)
    // 我们需要根据区间信息，在原矩阵的行内放置石头（靠右堆积），然后旋转填充
    // 可以先构造一个临时矩阵来放“压实后”的原矩阵，再旋转；或者直接映射到 ans。
    // 这里为了清晰，先构建压实后的原矩阵 tempBox，再旋转填入 ans。
    vector<vector<char>> tempBox(m, vector<char>(n, '.'));
    // 放置障碍物到 tempBox
    for (int i = 0; i < m; ++i)
      for (int j = 0; j < n; ++j)
        if (box[i][j] == '*')
          tempBox[i][j] = '*';

    // 根据区间填充石头
    for (int i = 0; i < m; ++i)
    {
      for (const auto &inv : rowIntervals[i])
      {
        int rightBound = inv.end; // 障碍物或右边界所在列
        int stones     = inv.stones;
        // 从右边界向左填充 stones 个石头
        for (int k = 1; k <= stones; ++k)
        {
          tempBox[i][rightBound - k] = '#';
        }
      }
    }

    // 旋转 tempBox 填入 ans
    for (int i = 0; i < m; ++i)
      for (int j = 0; j < n; ++j)
        ans[j][m - 1 - i] = tempBox[i][j];

    return ans;
  }
};

// @lc code=end

