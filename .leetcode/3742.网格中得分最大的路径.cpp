/*
 * @lc app=leetcode.cn id=3742 lang=cpp
 *
 * [3742] 网格中得分最大的路径
 */
 
 /*
 具体含义：

    dp[i][j][c] 表示从起点 (0,0) 走到 (i,j)，且路径中非零格子（数值不为 0）的个数恰好为 c 时，能够获得的最大得分（格子数值之和）。

    如果某格子数值为 0，则 c 不变；否则 c 增加 1。

    最终答案就是所有 c ≤ k 中 dp[rows-1][cols-1][c] 的最大值（如果不可达则为 -1）。
 */

// @lc code=start
class Solution {
public:
    int maxPathScore(vector<vector<int>>& grid, int k) {
        int ROWS = grid.size(), COLS = grid[0].size();
        // 增加一个维度：额外允许 k+2 种状态？实际上 Chunk_count = k+2 可能是为了下标方便
        // 代码中访问时用了 (Col * Chunk_count + Next_k) + Chunk_count 和 +0，说明每个列有 (k+2) 个状态
        int Chunk_count = k + 2;
        // 每个“块”的大小：每一列保存 k+2 个 int，加上一列作为边界？这里多申请了 (COLS+1) 个块
        int Chunks_size = (COLS + 1) * sizeof(int) * Chunk_count;

        // 当前行 / 下一行的 DP 数组，用 alloca 在栈上分配，速度快
        int* Chunks_cur = (int*)::alloca(Chunks_size);
        int* Chunks_next = (int*)::alloca(Chunks_size);
        // 将整个数组初始化为一个很小的负数（0x80 表示 int 的最小负数，相当于 -2147483648）
        ::memset(Chunks_cur, 0x80, Chunks_size);
        ::memset(Chunks_next, 0x80, Chunks_size);

        // 初始化最右下角下一行的边界：当走到最后一格之外时，得分 0 且剩余步数任意
        // 这里实际上是设置从虚拟的“右下角之外”出发，允许任意剩余次数 k，得分为 0
        #pragma clang loop unroll_count(8)
        for (int I = 0; I <= k; ++I) {
            // 下标：(COLS-1) 列，第 I 个状态
            // 因为 Chunks_next 的形状是 (COLS+1) * Chunk_count，所以 [(COLS-1)*Chunk_count + I] 对应最后一列的第 I 个状态
            Chunks_next[(COLS - 1) * Chunk_count + I] = 0;
        }

        // 从最后一行向前递推（从下往上，从右往左）
        for (int Row = ROWS - 1; Row >= 0; --Row) {
            for (int Col = COLS - 1; Col >= 0; --Col) {
                int Score = grid[Row][Col];
                int Cost = Score != 0;   // 非零格子消耗 1 次机会，零格子消耗 0

                // 对于当前格子，考虑所有可能已使用的次数 Curr_k（从 k 到 0）
                // 注意这里 Curr_k 的含义：到达当前格子时已经使用过的非零格子次数
                // 那么下一步（右或下）将使用 Next_k = Curr_k + Cost
                #pragma clang loop unroll_count(8)
                for (int Curr_k = k; Curr_k >= 0; --Curr_k) {
                    int Next_k = Curr_k + Cost;
                    if (Next_k > k) continue;   // 超限则跳过

                    // Right: 从右边格子转移过来
                    // Chunks_cur 存储的是当前行（Row）右侧列（Col+1）的 DP 值
                    // 因为列下标是 Col+1，所以索引为 ((Col+1) * Chunk_count + Next_k)
                    int Right = Chunks_cur[((Col * Chunk_count) + Next_k) + Chunk_count];
                    // Down: 从下边格子转移过来
                    // Chunks_next 存储的是下一行（Row+1）相同列（Col）的 DP 值
                    int Down  = Chunks_next[((Col * Chunk_count) + Next_k)];
                    // 当前格子 DP[Col][Curr_k] = Score + max(Right, Down)
                    Chunks_cur[(Col * Chunk_count) + Curr_k] = Score + max(Right, Down);
                }
            }
            // 当前行计算完毕，交换 cur 和 next，下一轮循环将计算上一行
            std::swap(Chunks_cur, Chunks_next);
            // 将新的 cur（原 next）清为极小值，准备填充下一行的数据
            ::memset(Chunks_cur, 0x80, Chunks_size);
        }

        // 最后 Chunks_next 中存储的是第一行第一列的结果（因为最后一次交换后，next 是 cur）
        // 但要访问第一行第一列且已使用次数 0 的得分
        return Chunks_next[0] >= 0 ? Chunks_next[0] : -1;
    }
};
// @lc code=end
class Solution {
public:
  int
  maxPathScore(std::vector<std::vector<int>> &grid, int k)
  {
    int ROWS = grid.size(), COLS = grid[0].size();
	// 裁剪k, 路径只能向右或向下移动。从 (0,0) 到 (ROWS-1, COLS-1) 必须走 (ROWS-1) 次向下和 (COLS-1) 次向右，总步数是 (ROWS-1) + (COLS-1) = ROWS + COLS - 2
	if (int Max_spent = ROWS + COLS - 2; k > Max_spent) k = Max_spent;

    // 每个单元存储的信息：对于每个列位置，有 (k+1) 种已使用次数的状态
    int  Chunk_size = (k + 1);
    // 分配 (COLS+1) * (k+1) 个 int，多一列作为左边的虚拟边界，便于处理第一列
    int  Chunks_size = (COLS + 1) * Chunk_size * sizeof(int);
    int *Chunk = reinterpret_cast<int *>(::alloca(Chunks_size));
    // 初始化为 -1，表示不可达
    ::memset(Chunk, -1, Chunks_size);

    // 虚拟的“起始列左边”的初始状态：从第一列左侧（列索引 0）开始，已使用次数 0 的得分为 0
    // 因为 Chunk[0] 对应第 0 列第 0 个状态（但实际上我们按列 1..COLS 计算，而列 0 作为左边界）
    Chunk[Chunk_size] = 0;   // 注意这里 Chunk_size 是步长，下标 Chunk_size 对应第一列的第一个状态

    // 按行遍历（从上到下）
    for (int I = 0; I < ROWS; ++I)
    {
      // 按列遍历（从左到右），列号从 1 到 COLS，便于统一索引公式
      for (int J = 1; J <= COLS; ++J)
      {
        int val = grid[I][J - 1];   // 当前格子数值
        if (val == 0)
        {
          // 当前格子数值为 0，不消耗次数，所以可以保留原次数转移过来
          #pragma clang loop unroll_count(4)
          for (int K = 0; K <= k; ++K)
          {
            int const Index = J * Chunk_size + K;
            // 从左边格子（同行的前一列）转移过来，使用同样的次数 K（因为无消耗）
            // 注意：这里写成了 Chunk[Index] = max(Chunk[Index], Chunk[Index - Chunk_size])
            // Index - Chunk_size 就是 (J-1)*Chunk_size + K，左边格子的相同状态
            Chunk[Index]    = std::max(Chunk[Index], Chunk[Index - Chunk_size]);
          }
        }
        else
        {
          // 当前格子数值非零，需要消耗一次机会（次数 +1）
          #pragma clang loop unroll_count(4)
          for (int K = k; K > 0; --K)
          {
            int const Index = J * Chunk_size + K;
            // 可以从左边格子（同行的前一列）且已使用次数 K-1 转移过来
            // 或者从上边格子（上一行的当前列）且已使用次数 K-1 转移过来
            // 注意：Chunk[Index - 1] 对应左边格子，Chunk[Index - 1 - Chunk_size] 对应上边格子（因为减 Chunk_size 表示上一行）
            int const Score = std::max(Chunk[Index - 1], Chunk[Index - 1 - Chunk_size]);
            // 如果可达（Score != -1），则加上当前格子的数值；否则仍为 -1
            Chunk[Index]    = Score != -1 ? Score + val : Score;
          }
          // 已使用次数为 0 的状态不可能到达当前非零格子（因为消耗了一次，次数不能为负）
          Chunk[J * Chunk_size] = -1;
        }
      }
    }

    // 最终答案：最后一列（COLS）中所有已使用次数 K (0..k) 的最大值
    int Max_score = -1;
    #pragma clang loop unroll_count(4)
    for (int K = 0; K <= k; ++K)
    {
      Max_score = std::max(Max_score, Chunk[COLS * Chunk_size + K]);
    }
    return Max_score;
  }
};