/*
 * @lc app=leetcode.cn id=994 lang=cpp
 *
 * [994] 腐烂的橘子
 */

// @lc code=start
#include <vector>
#include <queue>
#include <array>
class Solution {
public:
    int orangesRotting(std::vector<std::vector<int>>& grid) {
        struct Point { int X, Y; };
        int ROWS { static_cast<int>(grid.size()) }, COLS { static_cast<int>(grid.front().size()) };

        std::queue<Point> q;
        int fresh = 0;

        // 初始化队列，记录初始腐烂的橘子，并统计新鲜橘子的数量
        for (int row = 0; row < ROWS; ++row) {
            #pragma GCC unroll 8
            for (int col = 0; col < COLS; ++col) {
                if (grid[row][col] == 2) q.emplace(row, col);
                else if (grid[row][col] == 1) ++fresh;
            }
        }

        // 如果没有新鲜橘子，直接返回0
        if (fresh == 0) return 0;

        // 方向数组，用于四个方向的扩展
        static std::array<Point, 4> dirs{Point{-1, 0}, Point{1, 0}, Point{0, -1}, Point{0, 1}};
		// 初始化时间, -1是因为最后一趟如果都是腐烂橘子仍旧++，如果是0还需要额外处理，-1开始避免这样的额外处理
        int time = -1; 

        while (!q.empty()) {
            int size = q.size(); // 不能简化，因为emplace会让size改变，从而for错误
            ++time; // 每分钟处理一层

            for (int i = 0; i < size; ++i) {
                auto [x, y] = q.front();
                q.pop();
                #pragma GCC unroll 4
                for (auto [dx, dy] : dirs) {
                    int nx = x + dx;
                    int ny = y + dy;

                    // 检查新坐标是否在网格范围内且为新鲜橘子
                    if (nx >= 0 && nx < ROWS && ny >= 0 && ny < COLS && grid[nx][ny] == 1) {
                        grid[nx][ny] = 2; // 标记为腐烂
                        q.emplace(nx, ny);
                        --fresh; // 减少新鲜橘子的计数
                    }
                }
            }
        }

        // 如果还有新鲜橘子剩余，返回-1，否则返回时间
        return fresh == 0 ? time : -1;
    }
};
// @lc code=end

