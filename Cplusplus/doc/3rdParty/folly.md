# chrono

## Hardware.h

这段代码来自 Folly 库，提供了几种获取硬件时间戳（主要是 CPU 时间戳计数器 TSC）的方法，用于高精度性能测量。下面逐一解释每个函数的作用、实现原理和使用场景。

---

### 1. `hardware_timestamp()`

**作用**：直接读取当前 CPU 的时间戳计数器（TSC）或一个近似值，返回 `std::uint64_t` 类型的时间戳。  
**特点**：  
- **速度快**，通常只需一个 `rdtsc` 指令。  
- **没有添加任何内存屏障或编译器屏障**，因此编译器可能将相邻的指令重新排序，导致测量结果不准确。  
- CPU 也可能将 `rdtsc` 与周围的加载/存储指令进行流水线化，使得读取的时间并非真正“此刻”的时刻。

**实现**：  
- **MSVC + x86/x64**：使用 `__rdtsc()` 内联函数。  
- **GCC/clang + x86/x64**：使用 `__builtin_ia32_rdtsc()`。  
- **ARM64**：通过内联汇编读取系统寄存器 `cntvct_el0`（虚拟计数器）。  
- **其他平台**：回退到 `std::chrono::steady_clock::now()`（以纳秒为单位，但精度和速度不如硬件 TSC）。

**适用场景**：  
- 粗略的时间测量，不要求防止重排序。  
- 测量较长区段，编译器重排序影响不大。  
- 不适用于微基准测试（Microbenchmarking）中精确测量极短代码段。

---

### 2. `hardware_timestamp_measurement_start()`

**作用**：开始一次精确的测量，返回起始时间戳。  
**特点**：  
- **防止编译器重排序**：通过内联汇编的 `"memory"` 破坏描述（或 MSVC 的 `_ReadWriteBarrier`）告诉编译器不能将任何内存访问越过这个调用。  
- **防止 CPU 流水线化加载操作**：在 x86 上使用 `lfence` 指令，确保在读取 TSC 之前，所有之前的加载指令已经完成，并且 `rdtsc` 不会被提前执行。  
- **不阻止存储操作**：已经发出的存储可能仍然在后台写入内存（即 `lfence` 只序列化加载，不序列化存储）。这意味着函数不会阻塞存储指令的完成，适用于典型测量场景——你关心的是指令执行时间，而不是等待所有存储写完。

**实现细节（x86/x64）**：  
- **MSVC**：  
  ```
  _ReadWriteBarrier();          // 编译器屏障
  _mm_lfence();                 // CPU 加载屏障
  _ReadWriteBarrier();
  auto ret = __rdtsc();
  _ReadWriteBarrier();
  _mm_lfence();
  _ReadWriteBarrier();
  ```
- **GCC/clang**：内联汇编包含 `lfence; rdtsc; ...; lfence`，并设置了 `"memory"` clobber，确保编译器不重排内存访问。

**非 x86 平台**：回退到 `steady_clock::now()`（无法实现严格的序列化语义）。

**适用场景**：  
- 微基准测试中需要精确测量一段代码的开始时间。  
- 需要确保测量前所有的加载指令都已经完成（例如加载被测函数参数）。

---

### 3. `hardware_timestamp_measurement_stop()`

**作用**：结束一次精确测量，返回结束时间戳。  
**特点**：  
- 类似 `_start`，但在 x86 上使用 `rdtscp` 指令而不是 `rdtsc`。`rdtscp` 是序列化指令，它会等待之前的所有指令（包括存储）完成，并且读取一个处理器 ID 作为辅助信息（虽然这里未使用该 ID）。  
- **唯一能确保存储指令也完成**：`rdtscp` 在读取 TSC 之前会等待所有之前的指令（包括存储）全局可见。这一点与 `lfence + rdtsc` 不同（后者不等待存储完成）。  
- **使用 `lfence`**：在 `rdtscp` 之后仍然加上 `lfence` 是为了防止后续指令提前执行（尽管 `rdtscp` 本身已经是序列化指令，但加上 `lfence` 是保守且安全的做法）。  
- 同样包含编译器屏障 (`"memory"` clobber)。

**实现细节（x86/x64）**：  
- **MSVC**：使用 `__rdtscp(&aux)` 并配合读写屏障。  
- **GCC/clang**：内联汇编执行 `rdtscp; shl $32, %%rdx; or %%rdx, %[ret]; lfence`。

**非 x86**：回退到 `steady_clock::now()`。

**适用场景**：  
- 微基准测试中需要精确测量一段代码的结束时间。  
- 需要确保被测区域内的所有指令（包括存储）都已经完成，避免测量结果包含后台的存储操作。

---

### 组合使用示例

```cpp
uint64_t start = folly::hardware_timestamp_measurement_start();
// 被测代码块
uint64_t end = folly::hardware_timestamp_measurement_stop();
uint64_t elapsed = end - start;  // 时钟周期数或纳秒数
```

这种配对可以最大程度减少重排序和指令并发对测量结果的干扰。

---

### 总结对比

| 函数                                     | 防止编译器重排 | 防止 CPU 重排（加载） | 等待存储完成 | 性能开销 |
| ---------------------------------------- | -------------- | --------------------- | ------------ | -------- |
| `hardware_timestamp()`                   | ❌              | ❌                     | ❌            | 极低     |
| `hardware_timestamp_measurement_start()` | ✅              | ✅（`lfence`）         | ❌            | 中       |
| `hardware_timestamp_measurement_stop()`  | ✅              | ✅（`rdtscp`内置）     | ✅            | 较高     |

因此，进行精确的微基准测试时，推荐成对使用 `_start` 和 `_stop`。如果只是粗略计时或长时间运行的任务，可以直接使用 `hardware_timestamp()`。

## Conv.h

这段代码是 Folly 库中提供的一个转换层，用于 **`std::chrono` 时间类型（duration / time_point）与 POSIX 时间结构体（`struct timespec`、`struct timeval`）之间的安全、精确、带溢出检测的相互转换**。

### 核心目的

当你在 C++ 中使用 `<chrono>` 进行时间处理，但需要调用系统 API（如 `clock_gettime`、`nanosleep`、`gettimeofday`、`select` 等）时，就需要在这两种时间表示法之间来回转换。直接使用强制转换很容易导致：
- 整数溢出（例如将超过 `time_t` 范围的纳秒数赋值给 `tv_sec`）
- 精度丢失（例如 `duration` 的精度高于 `timeval` 的微秒）
- 负数处理错误（POSIX 要求 `tv_nsec` 和 `tv_usec` 必须为非负，且小于 1 秒）

Folly 的这个模块通过 **`tryTo` / `to`** 函数族解决了这些问题。

### 主要功能

#### 1. `std::chrono::duration` ↔ `timespec` / `timeval`
- **duration → POSIX 结构**：将任意精度的 `duration` 拆分为 `{秒, 纳秒/微秒}`，并自动处理正负数、进位、溢出。
- **POSIX 结构 → duration**：将 `tv_sec` 和 `tv_nsec` / `tv_usec` 组合成指定 `duration` 类型，并检查结果是否超出目标类型的表示范围。

#### 2. `std::chrono::time_point` ↔ POSIX 结构
- `time_point` 本质上是 `duration` 的包装，转换时直接转换其 `time_since_epoch()`。

#### 3. 支持多种 duration 粒度
- 纳秒、微秒、毫秒、秒、分钟、小时等任意 `std::ratio` 组合。
- 整数和浮点数表示均受支持。

#### 4. 错误处理
- **`tryTo`** 返回 `folly::Expected<T, ConversionCode>`，错误代码区分正溢出、负溢出、非法值等。
- **`to`** 直接返回值或在失败时抛出异常。

### 实现亮点

- **溢出检查**：针对 `time_t`（通常为 64 位有符号整数）和目标 duration 的 `rep` 类型分别做范围验证。
- **负值规范化**：当从 POSIX 结构转换时，如果 `tv_nsec` 为负或 ≥1秒，会自动调整到合法范围（类似 `nanosleep` 的行为）。
- **使用中间类型**：对于非常规 `period`（如 `ratio<1, 3>`），先通过 `IntermediateDuration` 转换，避免精度丢失和溢出。
- **浮点处理**：对浮点 `rep` 放宽检查，但保留舍入行为（使用 round-to-nearest 的 IEEE 语义）。

### 使用示例

```cpp
#include <folly/chrono/Conv.h>

// duration -> timespec
auto dur = std::chrono::nanoseconds(123456789);
auto ts = folly::to<struct timespec>(dur);  // 成功则 ts = {0, 123456789}

// timespec -> duration
struct timespec input{1, 500000000}; // 1.5 秒
auto dur2 = folly::tryTo<std::chrono::milliseconds>(input);
if (dur2.hasValue()) {
    std::cout << dur2.value().count() << " ms\n"; // 1500
}

// 溢出检查
struct timespec big{std::numeric_limits<time_t>::max(), 999999999};
auto tiny = folly::tryTo<std::chrono::seconds>(big);
// 返回 ConversionCode::POSITIVE_OVERFLOW，因为 seconds 可能无法表示那么大的值
```

### 总结

**这个代码是 C++ chrono 与 POSIX 时间 API 之间的安全桥接层**。它避免了手动进行单位换算和边界检查时常见的错误，使得在系统编程中混合使用这两种时间表示法变得简单、可靠。

# codec

## Uuid.h

**高性能的 UUID 解析器**，用于将标准格式的 UUID 字符串（36 个字符，形如 `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`）快速转换为 16 字节的二进制表示。

- **输入**：36 字符的 UUID 字符串（如 `"123e4567-e89b-12d3-a456-426614174000"`）
- **输出**：16 字节的二进制数据（例如 `0x12, 0x3e, 0x45, ...`）
- **校验**：检查字符串长度、连字符位置、所有字符是否为合法的十六进制数字（0-9, a-f, A-F）
- **返回值**：`UuidParseCode` 枚举（`SUCCESS`, `WRONG_LENGTH`, `INVALID_CHAR`）

## Hex.h

提供了**十六进制字符（'0'-'9', 'a'-'f', 'A'-'F'）的高效识别和转换工具**，主要用于快速解析十六进制字符串。核心功能包括：

- **判断字符是否为十六进制数字**（`hex_is_digit`）
- **将十六进制字符解码为 0-15 的数值**（`hex_decode_digit`），对非法字符返回高位设置的值（0x80 以上）
- **提供无分支的原始解码**（`hex_decode_digit_raw`），用于已知输入合法的场景，速度更快

















concurrency

compression

channels

cli