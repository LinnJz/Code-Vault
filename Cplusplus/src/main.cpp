#include <iostream>

// ========== 1. 三大编译器都支持的指令 ==========
#ifdef _MSC_VER
// MSVC 和 clang-cl 都会进入此块，并执行 #pragma message
#  pragma message("编译信息: 这是一个 #pragma message 示例")
#endif

// ========== 2. MSVC 特有的指令 ==========
#ifdef _MSC_VER
#  pragma warning(push, 1)        // 保存当前警告状态，并将警告级别设为1
#  pragma warning(disable : 4100) // 禁用"未引用形参"警告
#endif

// 一个带参数的函数，用于测试 #pragma warning(disable: 4100)
void
test_unused_param(int x, int y)
{
  // y 故意未使用，用以触发 C4100 警告
  std::cout << "x = " << x << std::endl;
}

#ifdef _MSC_VER
#  pragma warning(pop) // 恢复之前的警告状态
#endif

// 定义测试结构体，用于观察 #pragma pack 的效果
struct StructDefault
{
  char a; // 1 byte
  int b;  // 4 bytes (通常会在 a 之后填充3个字节)
}; // 默认对齐下，sizeof(StructDefault) 通常为 8

#pragma pack(push, 1) // 设置1字节对齐，紧凑排列

struct StructPacked
{
  char a; // 1 byte
  int b;  // 4 bytes (紧接着 a，无填充)
}; // 1字节对齐下，sizeof(StructPacked) 为 5

#pragma pack(pop) // 恢复之前的对齐设置


// ========== 3. Clang 特有的指令 ==========
#ifdef __clang__
// Clang 的诊断系统
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdeprecated-declarations" // 忽略“使用了被弃用的声明”的警告
#  pragma clang diagnostic pop

void
test_clang_loop(int *a, int *b, int *c, int n)
{
#  pragma clang loop vectorize(enable) interleave(enable) unroll_count(8)
  for (int i = 0; i < n; ++i)
  {
    a[i] = b[i] + c[i];
  }
}
#endif

// 辅助函数，用于在汇编层面观察 #pragma pack 的效果
void
printSizes()
{
  std::cout << "sizeof(StructDefault): " << sizeof(StructDefault) << std::endl;
  std::cout << "sizeof(StructPacked): " << sizeof(StructPacked) << std::endl;
}

int
main()
{
  test_unused_param(1, 2);
  printSizes();

  // 调用 Clang 特有的循环函数
  const int N = 10;
  int a[N], b[N], c[N];
  for (int i = 0; i < N; ++i)
  {
    b[i] = c[i] = i;
  }
  test_clang_loop(a, b, c, N);
  constexpr int x = __cplusplus;
  return 0;
}
