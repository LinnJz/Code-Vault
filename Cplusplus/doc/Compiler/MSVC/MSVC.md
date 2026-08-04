# sanitizer

Sanitizers是一系列用于检测内存错误、数据竞争、未初始化内存等问题的工具。下面逐一介绍每个头文件的作用：

1. **allocator_interface.h**
   提供与内存分配器相关的接口，用于与Sanitizer的内存分配器交互，比如替换默认的malloc/free等。
2. **asan_interface.h**
   提供AddressSanitizer（ASan）的接口。ASan主要用于检测内存错误，如堆栈缓冲区溢出、使用释放后的内存等。这个头文件包含了一些函数，用于手动管理内存的毒化（poisoning）和解除毒化（unpoisoning），以及其它ASan相关操作。
3. **common_interface_defs.h**
   定义了一些Sanitizers共用的接口和宏，例如共享的类型定义和函数属性。
4. **coverage_interface.h**
   提供代码覆盖率的接口，用于在运行时收集代码覆盖信息，通常与模糊测试（fuzzing）结合使用。
5. **dfsan_interface.h**
   提供DataFlowSanitizer（DFSan）的接口。DFSan是一种动态数据流分析工具，用于跟踪数据的流动，检测敏感数据（如污点数据）的传播。
6. **hwasan_interface.h**
   提供Hardware-assisted AddressSanitizer（HWASan）的接口。HWASan是ASan的硬件辅助版本，利用硬件特性（如ARM的MTE）来检测内存错误，具有较低的性能开销。
7. **linux_syscall_hooks.h**
   包含Linux系统调用的钩子函数，用于在Linux平台上拦截系统调用，以便Sanitizer进行监控。
8. **lsan_interface.h**
   提供LeakSanitizer（LSan）的接口。LSan用于检测内存泄漏，这个头文件包含了一些函数，用于手动启动泄漏检测、忽略某些内存块等。
9. **msan_interface.h**
   提供MemorySanitizer（MSan）的接口。MSan用于检测未初始化的内存读取，这个头文件包含了一些函数，用于标记内存为已初始化或检查内存是否已初始化。
10. **netbsd_syscall_hooks.h**
    包含NetBSD系统调用的钩子函数，用于在NetBSD平台上拦截系统调用，以便Sanitizer进行监控。
11. **scudo_interface.h**
    提供Scudo分配器的接口。Scudo是一个用户态的内存分配器，旨在提供安全性和性能，用于缓解内存损坏漏洞。
12. **tsan_interface.h**
    提供ThreadSanitizer（TSan）的接口。TSan用于检测数据竞争和多线程相关的错误，这个头文件包含了一些函数，用于手动注解线程相关的操作（如锁、信号量等）。
13. **tsan_interface_atomic.h**
    提供TSan对原子操作的接口。用于在TSan中处理原子操作，确保原子操作的正确性并检测相关的竞争条件。

这些头文件通常用于在代码中直接与Sanitizer交互，例如手动标记内存区域、忽略某些检查、收集覆盖信息等。它们为开发者提供了更细粒度的控制 over the sanitizers' behavior.

# fuzzer

根据头文件注释和代码分析，我来详细说明`FuzzedDataProvider`的功能和使用方法。

## 文件功能概述

`FuzzedDataProvider` 是一个用于模糊测试(fuzzing)的实用工具类，主要功能是：

- **确定性数据分割**：将输入的字节数组按需拆分成各种类型的数据
- **可重现性**：相同的输入数据和相同的调用顺序会产生相同的输出
- **类型安全**：提供类型安全的接口来消费数据
- **内存安全**：自动处理缓冲区边界，防止越界访问

## 类和方法功能说明

### 构造函数
```cpp
FuzzedDataProvider(const uint8_t *data, size_t size)
```
包装字节数组供后续使用，数据必须比FuzzedDataProvider对象生命周期更长

### 字节向量方法
- `ConsumeBytes<T>()` - 消费指定字节数，返回vector
- `ConsumeBytesWithTerminator()` - 消费字节并在末尾添加终止符
- `ConsumeRemainingBytes()` - 消费所有剩余字节

### 字符串方法
- `ConsumeBytesAsString()` - 消费字节作为字符串
- `ConsumeRandomLengthString()` - 生成随机长度字符串
- `ConsumeRemainingBytesAsString()` - 消费所有剩余字节作为字符串

### 数值方法
- `ConsumeIntegral()` - 消费整数（类型范围）
- `ConsumeIntegralInRange()` - 消费指定范围的整数
- `ConsumeFloatingPoint()` - 消费浮点数
- `ConsumeFloatingPointInRange()` - 消费指定范围的浮点数
- `ConsumeProbability()` - 消费0-1之间的概率值
- `ConsumeBool()` - 消费布尔值

### 选择方法
- `ConsumeEnum()` - 从枚举中选择值
- `PickValueInArray()` - 从数组中选择值

### 工具方法
- `ConsumeData()` - 直接写入数据到指针
- `remaining_bytes()` - 获取剩余字节数

## 完整使用示例

```cpp
#include "FuzzedDataProvider.h"
#include <print>
#include <vector>
#include <array>
#include <string>

// 示例枚举
enum class Color { Red, Green, Blue, Yellow, kMaxValue = Yellow };

// 被测试的函数
void processData(const std::vector<uint8_t>& data, int value, const std::string& str, bool flag) {
    std::print("Processing - Data size: {}, Value: {}, String: '{}', Flag: {}\n", 
               data.size(), value, str, flag);
}

bool validateInput(int a, double b, Color color) {
    std::print("Validating - Int: {}, Double: {:.2f}, Color: {}\n", 
               a, b, static_cast<int>(color));
    return a > 0 && b >= 0.0;
}

int main() {
    // 模拟模糊测试输入数据
    std::vector<uint8_t> fuzz_data = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        'H', 'e', 'l', 'l', 'o', 0x00, 0xFF, 0x7F,
        0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49
    };
    
    FuzzedDataProvider provider(fuzz_data.data(), fuzz_data.size());
    
    std::print("初始剩余字节: {}\n", provider.remaining_bytes());
    
    // 1. 消费字节向量
    auto bytes1 = provider.ConsumeBytes<uint8_t>(4);
    std::print("消费4字节: ");
    for (auto b : bytes1) {
        std::print("{:02x} ", b);
    }
    std::print("\n");
    
    // 2. 消费带终止符的字节
    auto bytes2 = provider.ConsumeBytesWithTerminator<uint8_t>(3, 0xAA);
    std::print("消费3字节(带终止符): ");
    for (auto b : bytes2) {
        std::print("{:02x} ", b);
    }
    std::print("\n");
    
    // 3. 消费字符串
    auto str1 = provider.ConsumeBytesAsString(5);
    std::print("消费5字节字符串: '{}'\n", str1);
    
    // 4. 消费随机长度字符串
    auto str2 = provider.ConsumeRandomLengthString(10);
    std::print("消费随机长度字符串: '{}'\n", str2);
    
    // 5. 消费整数
    auto int8_val = provider.ConsumeIntegral<int8_t>();
    auto uint32_val = provider.ConsumeIntegral<uint32_t>();
    std::print("消费int8: {}, uint32: {}\n", int8_val, uint32_val);
    
    // 6. 消费范围内的整数
    auto int_in_range = provider.ConsumeIntegralInRange(-100, 100);
    std::print("消费范围内整数[-100,100]: {}\n", int_in_range);
    
    // 7. 消费浮点数
    auto float_val = provider.ConsumeFloatingPoint<float>();
    auto double_in_range = provider.ConsumeFloatingPointInRange(0.0, 1.0);
    std::print("消费float: {:.3f}, double范围[0,1]: {:.3f}\n", float_val, double_in_range);
    
    // 8. 消费概率值
    auto prob = provider.ConsumeProbability<double>();
    std::print("消费概率值: {:.3f}\n", prob);
    
    // 9. 消费布尔值
    auto bool_val = provider.ConsumeBool();
    std::print("消费布尔值: {}\n", bool_val);
    
    // 10. 消费枚举值
    auto color = provider.ConsumeEnum<Color>();
    std::print("消费枚举值: {}\n", static_cast<int>(color));
    
    // 11. 从数组中选择值
    std::array<int, 5> options = {10, 20, 30, 40, 50};
    auto picked = provider.PickValueInArray(options);
    std::print("从数组选择的值: {}\n", picked);
    
    // 12. 直接消费数据到缓冲区
    std::array<uint8_t, 4> buffer{};
    size_t bytes_written = provider.ConsumeData(buffer.data(), 2);
    std::print("直接消费数据: 写入{}字节 - ", bytes_written);
    for (size_t i = 0; i < bytes_written; ++i) {
        std::print("{:02x} ", buffer[i]);
    }
    std::print("\n");
    
    // 13. 消费剩余所有字节
    auto remaining_bytes = provider.ConsumeRemainingBytes<uint8_t>();
    std::print("消费剩余{}字节: ", remaining_bytes.size());
    for (auto b : remaining_bytes) {
        std::print("{:02x} ", b);
    }
    std::print("\n");
    
    std::print("最终剩余字节: {}\n", provider.remaining_bytes());
    
    return 0;
}
```

## 模糊测试目标示例

```cpp
#include "FuzzedDataProvider.h"
#include <print>

// 典型的模糊测试目标函数
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    FuzzedDataProvider provider(data, size);
    
    try {
        // 从模糊输入中提取各种测试数据
        auto test_bytes = provider.ConsumeBytes<uint8_t>(
            provider.ConsumeIntegralInRange<size_t>(0, 100));
        
        int test_int = provider.ConsumeIntegralInRange(-1000, 1000);
        std::string test_str = provider.ConsumeRandomLengthString(50);
        double test_double = provider.ConsumeFloatingPointInRange(0.0, 100.0);
        bool test_bool = provider.ConsumeBool();
        
        // 调用被测试的函数
        // yourFunctionUnderTest(test_bytes, test_int, test_str, test_double, test_bool);
        
        std::print("Fuzzing - Bytes: {}, Int: {}, Str: '{}', Double: {:.2f}, Bool: {}\n",
                   test_bytes.size(), test_int, test_str, test_double, test_bool);
                   
    } catch (...) {
        // 捕获所有异常，模糊测试应该继续运行
    }
    
    return 0;
}
```

## 关键特性说明

1. **确定性**：相同的输入和调用顺序产生相同输出
2. **边界安全**：自动处理数据不足的情况
3. **类型丰富**：支持各种基础类型和容器
4. **内存友好**：使用vector等RAII类型避免内存泄漏

这个工具极大简化了模糊测试中的数据提取和类型转换工作，让测试逻辑更清晰。

# Core

1. agents.h: 用于异步代理编程，是并发运行时（Concurrency Runtime）的一部分，提供用于构建异步应用程序的类和函数。
2. agile.h: 用于支持敏捷对象（Agile Object），这是一种可以在不同线程间安全传递的COM对象，通常用于Windows运行时（WinRT）编程。
3. ammintrin.h: 用于SSE4a指令集的 intrinsics 函数，SSE4a是AMD处理器支持的指令集。
4. amp.h: 用于C++ AMP（Accelerated Massive Parallelism），是一种用于异构并行计算的库。
5. amprt.h: C++ AMP运行时头文件，提供C++ AMP运行时的支持。
6. amprt_exceptions.h: C++ AMP异常处理相关。
7. amp_graphics.h: 用于C++ AMP的图形编程支持。
8. amp_math.h: 提供C++ AMP的数学函数。
9. amp_short_vectors.h: 提供C++ AMP的短向量类型，用于表示小向量（如float2、float3等）并支持并行操作。
10. arm64intr.h: 用于ARM64架构的 intrinsics 函数。
11. arm64_neon.h: 用于ARM64架构的NEON SIMD指令的 intrinsics 函数。
12. armintr.h: 用于ARM架构的 intrinsics 函数。
13. arm_neon.h: 用于ARM架构的NEON SIMD指令的 intrinsics 函数。
14. castguard.h: 用于支持强制转换保护（CastGuard），是一种安全特性。
15. cfguard.h: 控制流保护（Control Flow Guard）相关，用于安全开发。
16. collection.h: 提供Windows运行时集合类的支持。
17. comdef.h: 用于COM编程，定义了一些宏和类，如_bstr_t、_com_ptr_t等，简化COM对象的使用。
18. comdefsp.h: COM定义的补充。
19. comip.h: 用于COM接口指针的智能指针，如_com_ptr_t。
20. comutil.h: COM实用工具，提供了一些用于COM编程的辅助函数，如字符串转换等。
21. concrt.h: 并发运行时（Concurrency Runtime）的主要头文件，用于任务并行、并行算法等。
22. concrtrm.h: 并发运行时的资源管理器。
23. concurrencysal.h: 用于并发编程的SAL（源代码注释语言）注解。
24. concurrent_priority_queue.h: 并发优先级队列，是并发运行时的一部分。
25. concurrent_queue.h: 并发队列，支持多线程并发访问。
26. concurrent_unordered_map.h: 并发无序映射，线程安全的哈希表。
27. concurrent_unordered_set.h: 并发无序集合，线程安全的哈希集合。
28. concurrent_vector.h: 并发向量，线程安全的动态数组。
29. crtdefs.h: C运行时定义，包含一些基本类型和宏的定义。
30. crtversion.h: C运行时版本信息。
31. delayimp.h: 延迟加载DLL的支持，用于编写延迟加载导入库的代码。
32. dloadsup.h: 延迟加载辅助函数。
33. dvec.h: 用于动态向量化的 intrinsics 函数，与SIMD相关。
34. eh.h: 异常处理（Exception Handling）相关。
35. ehdata.h: 异常处理数据。
36. ehdata4.h: 异常处理数据（用于x64）。
37. ehdata4_export.h: 异常处理数据的导出定义。
38. ehdata_forceinclude.h: 强制包含异常处理数据。
39. ehdata_values.h: 异常处理数据的值。
40. emmintrin.h: SSE2指令集的 intrinsics 函数。
41. excpt.h: 异常处理相关，定义了异常处理宏和函数。
42. fvec.h: 用于浮点向量化的 intrinsics 函数。
43. gcroot.h: 用于C++/CLI编程，表示垃圾回收根（GC Root）。
44. immintrin.h: Intel的AVX指令集的 intrinsics 函数，也包括SSE等。
45. internal_concurrent_hash.h: 内部并发哈希表，用于实现并发容器。
46. internal_split_ordered_list.h: 内部拆分有序列表，用于实现并发容器。
47. intrin.h: 编译器 intrinsics 函数，包括各种架构特定的内置函数。
48. intrin0.h: 编译器 intrinsics 函数的内部头文件。
49. intrin0.inl.h: 编译器 intrinsics 函数的内联实现。
50. invkprxy.h: 调用代理（Invocation Proxy），用于COM编程。
51. isa_availability.h: 指令集架构可用性，用于检测CPU支持的指令集。
52. iso646.h: 定义了一些运算符的替代表示，如and、or、not等。
53. ivec.h: 用于整数向量化的 intrinsics 函数。
54. limits.h: 定义各种数据类型的取值范围。
55. mm3dnow.h: 3DNow!指令集的 intrinsics 函数。
56. mmintrin.h: MMX指令集的 intrinsics 函数。
57. nmmintrin.h: SSE4.2指令集的 intrinsics 函数。
58. omp.h: OpenMP支持，用于共享内存并行编程。
59. omp_llvm.h: LLVM版本的OpenMP支持。
60. pgobootrun.h: 用于按配置优化（PGO）的启动运行。
61. pmmintrin.h: SSE3指令集的 intrinsics 函数。
62. ppl.h: 并行模式库（Parallel Patterns Library），用于任务并行。
63. pplawait.h: PPL的等待支持。
64. pplcancellation_token.h: PPL的取消令牌，用于取消任务。
65. pplinterface.h: PPL接口。
66. ppltasks.h: PPL任务，用于创建和管理异步任务。
67. ppltaskscheduler.h: PPL任务调度器。
68. pplwin.h: PPL的Windows特定支持。
69. rtcapi.h: 运行时错误检查（Run-Time Error Checking）API。
70. rttidata.h: 运行时类型信息（RTTI）数据。
71. sal.h: 源代码注释语言（Source Annotation Language），用于代码分析。
72. setjmp.h: 非局部跳转，用于异常处理等。
73. setjmpex.h: 扩展的setjmp，支持更复杂的跳转。
74. smmintrin.h: SSE4.1指令集的 intrinsics 函数。
75. srv.h: 用于服务编程。
76. stdarg.h: 可变参数支持。
77. stdatomic.h: C11原子操作支持。
78. stdbool.h: 定义布尔类型。
79. stdint.h: 定义固定宽度的整数类型。
80. threads.h: C11线程支持。
81. tmmintrin.h: SSSE3指令集的 intrinsics 函数。
82. use_ansi.h: 用于强制使用ANSI标准。
83. vadefs.h: 可变参数定义。
84. varargs.h: 可变参数支持（旧式）。
85. vcclr.h: C++/CLI支持，用于.NET互操作。
86. vccorlib.h: Windows运行时组件的C++/CX支持。
87. vcruntime.h: Visual C++运行时库的主要头文件。
88. vcruntime_c11_atomic_support.h: C11原子操作的支持。
89. vcruntime_c11_stdatomic.h: C11标准原子操作的支持。
90. vcruntime_exception.h: 运行时异常支持。
91. vcruntime_new.h: 动态内存分配（new和delete）的支持。
92. vcruntime_new_debug.h: 调试版本的动态内存分配支持。
93. vcruntime_startup.h: 运行时启动代码。
94. vcruntime_string.h: 字符串操作函数。
95. vcruntime_typeinfo.h: 类型信息支持。
96. wmmintrin.h: AES指令集的 intrinsics 函数。
97. xatomic.h: 扩展的原子操作支持。
98. xatomic_wait.h: 扩展的原子等待操作。
99. xbit_ops.h: 位操作函数。
100. xcall_once.h: 扩展的call_once实现。
101. xcharconv.h: 扩展的字符转换函数。
102. xcharconv_ryu.h: 使用Ryu算法进行字符转换。
103. xcharconv_ryu_tables.h: Ryu算法转换表。
104. xcharconv_tables.h: 字符转换表。
105. xerrc.h: 扩展的错误代码。
106. xfilesystem_abi.h: 文件系统ABI定义。
107. xkeycheck.h: 关键字检查。
108. xmmintrin.h: SSE指令集的 intrinsics 函数。
109. xnode_handle.h: 节点句柄，用于容器节点操作。
110. xpolymorphic_allocator.h: 多态分配器。
111. xsmf_control.h: 默认成员函数控制。
112. xthreads.h: 扩展的线程支持。
113. xtimec.h: 扩展的时间支持。
114. xxamp.h: C++ AMP的扩展支持。
115. xxamp_inl.h: C++ AMP的内联实现。
116. ymath.h: 数学函数。
117. yvals.h: 用于定义编译器特定的宏和类型。
118. yvals_core.h: 核心编译器支持。
119. zmmintrin.h: AVX-512指令集的 intrinsics 函数。

## 并发运行时和并行库

1. **agents.h** - 异步代理库，用于构建基于数据流的并发应用
2. **concrt.h** - 并发运行时核心头文件，提供任务并行和并行算法
3. **concrtrm.h** - 并发运行时资源管理器
4. **concurrencysal.h** - 并发相关的源代码注解
5. **concurrent_priority_queue.h** - 线程安全的优先级队列容器
6. **concurrent_queue.h** - 线程安全的队列容器
7. **concurrent_unordered_map.h** - 线程安全的哈希映射容器
8. **concurrent_unordered_set.h** - 线程安全的哈希集合容器
9. **concurrent_vector.h** - 线程安全的动态数组容器
10. **internal_concurrent_hash.h** - 内部并发哈希表实现
11. **internal_split_ordered_list.h** - 内部拆分有序列表实现

## C++ AMP (加速大规模并行)

1. **amp.h** - C++ AMP主头文件，用于GPU加速计算
2. **amprt.h** - AMP运行时支持
3. **amprt_exceptions.h** - AMP异常处理
4. **amp_graphics.h** - AMP图形编程支持
5. **amp_math.h** - AMP数学函数库
6. **amp_short_vectors.h** - AMP短向量类型(如float2, float3等)
7. **xxamp.h** - AMP扩展支持
8. **xxamp_inl.h** - AMP内联实现

## SIMD指令集 intrinsics

1. **ammintrin.h** - SSE4a指令集intrinsics (AMD)
2. **emmintrin.h** - SSE2指令集intrinsics
3. **immintrin.h** - AVX指令集intrinsics
4. **mm3dnow.h** - 3DNow!指令集intrinsics (AMD)
5. **mmintrin.h** - MMX指令集intrinsics
6. **nmmintrin.h** - SSE4.2指令集intrinsics
7. **pmmintrin.h** - SSE3指令集intrinsics
8. **smmintrin.h** - SSE4.1指令集intrinsics
9. **tmmintrin.h** - SSSE3指令集intrinsics
10. **wmmintrin.h** - AES指令集intrinsics
11. **zmmintrin.h** - AVX-512指令集intrinsics

## ARM架构支持

1. **arm64intr.h** - ARM64架构intrinsics
2. **arm64_neon.h** - ARM64 NEON SIMD intrinsics
3. **armintr.h** - ARM架构intrinsics
4. **arm_neon.h** - ARM NEON SIMD intrinsics

## COM和Windows运行时支持

1. **agile.h** - 敏捷对象支持，用于跨线程COM对象
2. **comdef.h** - COM定义和智能指针(如_bstr_t, _com_ptr_t)
3. **comdefsp.h** - COM定义补充
4. **comip.h** - COM接口指针支持
5. **comutil.h** - COM工具函数(字符串转换等)
6. **invkprxy.h** - 调用代理支持

## 并行模式库(PPL)

1. **ppl.h** - 并行模式库主头文件
2. **pplawait.h** - PPL异步等待支持
3. **pplcancellation_token.h** - PPL取消令牌
4. **pplinterface.h** - PPL接口定义
5. **ppltasks.h** - PPL任务并行
6. **ppltaskscheduler.h** - PPL任务调度器
7. **pplwin.h** - PPL Windows特定支持

## 运行时和核心支持

1. **castguard.h** - 类型转换保护机制
2. **cfguard.h** - 控制流保护
3. **crtdefs.h** - C运行时定义
4. **crtversion.h** - C运行时版本信息
5. **delayimp.h** - 延迟加载DLL支持
6. **dloadsup.h** - 延迟加载支持函数
7. **eh.h** - 异常处理支持
8. **ehdata.h** - 异常处理数据
9. **ehdata4.h** - 异常处理数据(x64)
10. **ehdata4_export.h** - 异常处理数据导出
11. **ehdata_forceinclude.h** - 异常处理强制包含
12. **ehdata_values.h** - 异常处理值定义
13. **excpt.h** - 异常处理相关定义
14. **isa_availability.h** - 指令集架构可用性检测
15. **pgobootrun.h** - 按配置优化启动运行
16. **rtcapi.h** - 运行时错误检查API
17. **rttidata.h** - 运行时类型信息数据

## 向量化支持

1. **dvec.h** - 双精度向量支持
2. **fvec.h** - 单精度向量支持
3. **ivec.h** - 整数向量支持
4. **xmmintrin.h** - SSE指令集intrinsics

## .NET和CLI互操作

1. **gcroot.h** - GC根引用，用于C++/CLI
2. **vcclr.h** - C++/CLI支持
3. **vccorlib.h** - Windows运行时组件支持

## 标准扩展和实现细节

1. **collection.h** - 集合类支持
2. **iso646.h** - 运算符替代表示(and, or, not等)
3. **limits.h** - 数值界限定义
4. **omp.h** - OpenMP并行编程支持
5. **omp_llvm.h** - LLVM OpenMP支持
6. **sal.h** - 源代码注解语言
7. **setjmp.h** - 非局部跳转
8. **setjmpex.h** - 扩展的非局部跳转
9. **srv.h** - 服务相关定义
10. **stdarg.h** - 可变参数支持
11. **stdatomic.h** - C11原子操作支持
12. **stdbool.h** - 布尔类型支持
13. **stdint.h** - 固定宽度整数类型
14. **threads.h** - C11线程支持
15. **use_ansi.h** - ANSI兼容性支持
16. **vadefs.h** - 可变参数定义
17. **varargs.h** - 可变参数支持(旧式)

## Visual C++运行时

1. **vcruntime.h** - VC++运行时主头文件
2. **vcruntime_c11_atomic_support.h** - C11原子操作运行时支持
3. **vcruntime_c11_stdatomic.h** - C11标准原子操作支持
4. **vcruntime_exception.h** - 异常处理运行时支持
5. **vcruntime_new.h** - 动态内存分配运行时支持
6. **vcruntime_new_debug.h** - 调试版本内存分配支持
7. **vcruntime_startup.h** - 运行时启动代码
8. **vcruntime_string.h** - 字符串操作运行时支持
9. **vcruntime_typeinfo.h** - 类型信息运行时支持

## 现代C++扩展

1. **xatomic.h** - 扩展原子操作
2. **xatomic_wait.h** - 原子等待操作
3. **xbit_ops.h** - 位操作扩展
4. **xcall_once.h** - 扩展call_once实现
5. **xcharconv.h** - 字符转换扩展
6. **xcharconv_ryu.h** - Ryu字符转换算法
7. **xcharconv_ryu_tables.h** - Ryu算法转换表
8. **xcharconv_tables.h** - 字符转换表
9. **xerrc.h** - 扩展错误代码
10. **xfilesystem_abi.h** - 文件系统ABI定义
11. **xkeycheck.h** - 关键字检查
12. **xnode_handle.h** - 节点句柄支持
13. **xpolymorphic_allocator.h** - 多态分配器
14. **xsmf_control.h** - 特殊成员函数控制
15. **xthreads.h** - 扩展线程支持
16. **xtimec.h** - 时间操作扩展

## 数学和核心库

1. **ymath.h** - 数学函数实现
2. **yvals.h** - 实现定义的值和宏
3. **yvals_core.h** - 核心实现定义