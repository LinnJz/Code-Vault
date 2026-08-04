# Clang `__builtin_*` 内建函数中文参考手册

> 来源：[Clang Language Extensions](https://clang.llvm.org/docs/LanguageExtensions.html)（Clang 24.0.0git）
> 说明：本文档截取原文中所有以 `__builtin_` 开头的内容，并为每个内建函数/内建函数族补充中文功能说明与使用示例。

**检测内建函数是否可用**：可用 `__has_builtin(x)` 宏检测，返回 1 表示支持、0 表示不支持。例如：

```c
#if __has_builtin(__builtin_trap)
  __builtin_trap();
#else
  abort();
#endif
```

---

## 目录

- [一、向量内建函数](#一向量内建函数)
- [二、类型特征（Type Trait）内建](#二类型特征type-trait内建)
- [三、优化与控制流提示](#三优化与控制流提示)
- [四、内存分配与地址](#四内存分配与地址)
- [五、位操作内建](#五位操作内建)
- [六、多精度算术与检查算术](#六多精度算术与检查算术)
- [七、浮点内建](#七浮点内建)
- [八、字符串与内存操作内建](#八字符串与内存操作内建)
- [九、变长参数（va_*）内建](#九变长参数va_内建)
- [十、源位置内建](#十源位置内建)
- [十一、对齐内建](#十一对齐内建)
- [十二、协程（Coroutine）内建](#十二协程coroutine内建)
- [十三、WebAssembly 表内建](#十三webassembly-表内建)
- [十四、目标相关内建](#十四目标相关内建)
- [十五、其他通用内建](#十五其他通用内建)
- [十六、支持在常量表达式（constexpr）中使用的内建](#十六支持在常量表达式constexpr中使用的内建)

---

## 一、向量内建函数

### `__builtin_shufflevector`

**功能**：表达通用的向量置换/洗牌（shuffle/swizzle）操作。该内建对 `<xmmintrin.h>` 等目标相关头文件的实现非常重要，并且可以在常量表达式中使用。

**语法**：

```c
__builtin_shufflevector(vec1, vec2, index1, index2, ...)
```

前两个参数是元素类型相同的向量；后续参数是整数索引列表，指定从两个向量中提取哪些元素组成新向量。索引从第一个向量开始顺序编号并延续到第二个向量（如 vec1 为 4 元素向量时，索引 5 表示 vec2 的第 2 个元素）。索引 -1 表示"不在乎"（该位置可被后端自由优化），但 -1 不能在常量表达式中使用。结果向量的元素个数等于索引个数。

**示例**：

```c
// 恒等操作 —— 返回 4 元素向量 v1
__builtin_shufflevector(v1, v1, 0, 1, 2, 3)

// "广播"（Splat）：把 V1 的第 0 个元素复制到结果的所有 4 个位置
__builtin_shufflevector(V1, V1, 0, 0, 0, 0)

// 反转 4 元素向量 V1
__builtin_shufflevector(V1, V1, 3, 2, 1, 0)

// 交替拼接两个 4 元素向量 V1、V2 的每隔一个元素
__builtin_shufflevector(V1, V2, 0, 2, 4, 6)

// 含"不在乎"元素的洗牌（不能在常量表达式中使用）
__builtin_shufflevector(v1, v1, 3, -1, 1, -1)
```

### `__builtin_convertvector`

**功能**：表达通用的向量类型转换操作。输入向量与输出向量类型必须具有相同的元素个数，可移入常量表达式。结果向量的值等价于对第一个参数的每个元素分别做 C 风格强制转换。

**语法**：

```c
__builtin_convertvector(src_vec, dst_vec_type)
```

**示例**：

```c
typedef double vector4double __attribute__((__vector_size__(32)));
typedef float  vector4float  __attribute__((__vector_size__(16)));
typedef short  vector4short  __attribute__((__vector_size__(8)));
vector4float vf; vector4short vs;

// 把 4 个 float 的向量转换为 4 个 double 的向量
__builtin_convertvector(vf, vector4double)
// 等价于：
(vector4double) { (double) vf[0], (double) vf[1], (double) vf[2], (double) vf[3] }

// 把 4 个 short 的向量转换为 4 个 float 的向量
__builtin_convertvector(vs, vector4float)
```

### `__builtin_vectorelements`

**功能**：返回向量的元素个数。对固定尺寸向量（如 `__attribute__((vector_size(N)))` 或 ARM NEON 的 `uint16x8_t`），在编译期返回常量；对可伸缩向量（SVE、RISC-V V），在运行时确定。可用于写"向量类型无关"的循环（如递增循环计数器）。

**示例**：

```c
typedef int v4si __attribute__((__vector_size__(16)));
constexpr size_t n = __builtin_vectorelements(v4si{1,2,3,4}); // 4
```

### 逐元素（Elementwise）内建函数族

**功能**：每个内建返回一个向量，等价于把指定运算逐元素作用于输入。除非特别说明，`operation(±0) = ±0`、`operation(±∞) = ±∞`。整数类型之间不会发生隐式提升，禁止混合不同大小/符号的整数类型。以下内建可在 constexpr 上下文中调用：

`__builtin_elementwise_popcount`、`__builtin_elementwise_bitreverse`、`__builtin_elementwise_add_sat`、`__builtin_elementwise_sub_sat`、`__builtin_elementwise_max`、`__builtin_elementwise_min`、`__builtin_elementwise_abs`、`__builtin_elementwise_clzg`、`__builtin_elementwise_ctzg`、`__builtin_elementwise_fma`

**完整列表**（T 可为整数类型、标准浮点类型、半精度浮点、向量类型；对标量类型等价于单元素向量）：

| 内建函数 | 运算 | 支持的元素类型 |
|---|---|---|
| `T __builtin_elementwise_abs(T x)` | 返回 x 的绝对值；最小负整数的绝对值仍是它本身 | 有符号整数、浮点 |
| `T __builtin_elementwise_fma(T x, T y, T z)` | 融合乘加：(x * y) + z | 浮点 |
| `T __builtin_elementwise_ceil(T x)` | 大于等于 x 的最小整数值 | 浮点 |
| `T __builtin_elementwise_sin(T x)` | 弧度制的正弦 | 浮点 |
| `T __builtin_elementwise_cos(T x)` | 弧度制的余弦 | 浮点 |
| `T __builtin_elementwise_tan(T x)` | 弧度制的正切 | 浮点 |
| `T __builtin_elementwise_asin(T x)` | 弧度制的反正弦 | 浮点 |
| `T __builtin_elementwise_acos(T x)` | 弧度制的反余弦 | 浮点 |
| `T __builtin_elementwise_atan(T x)` | 弧度制的反正切 | 浮点 |
| `T __builtin_elementwise_atan2(T y, T x)` | y/x 的反正切 | 浮点 |
| `T __builtin_elementwise_sinh(T x)` | 双曲正弦 | 浮点 |
| `T __builtin_elementwise_cosh(T x)` | 双曲余弦 | 浮点 |
| `T __builtin_elementwise_tanh(T x)` | 双曲正切 | 浮点 |
| `T __builtin_elementwise_floor(T x)` | 小于等于 x 的最大整数值 | 浮点 |
| `T __builtin_elementwise_log(T x)` | 自然对数 | 浮点 |
| `T __builtin_elementwise_log2(T x)` | 以 2 为底的对数 | 浮点 |
| `T __builtin_elementwise_log10(T x)` | 以 10 为底的对数 | 浮点 |
| `T __builtin_elementwise_popcount(T x)` | x 中 1 的个数 | 整数 |
| `T __builtin_elementwise_pow(T x, T y)` | x 的 y 次方 | 浮点 |
| `T __builtin_elementwise_bitreverse(T x)` | 反转 x 的比特位后得到的整数 | 整数 |
| `T __builtin_elementwise_exp(T x)` | e^x | 浮点 |
| `T __builtin_elementwise_exp2(T x)` | 2^x | 浮点 |
| `T __builtin_elementwise_exp10(T x)` | 10^x | 浮点 |
| `T __builtin_elementwise_ldexp(T x, IntT y)` | x 乘以 2 的 y 次方（y 为与 x 形状匹配的整数类型） | T: 浮点；IntT: 整数 |
| `T __builtin_elementwise_sqrt(T x)` | 平方根 | 浮点 |
| `T __builtin_elementwise_roundeven(T x)` | 四舍五入到最近的整数，遇 0.5 舍入到偶数 | 浮点 |
| `T __builtin_elementwise_round(T x)` | 四舍五入到最近的整数，遇 0.5 远离零舍入 | 浮点 |
| `T __builtin_elementwise_trunc(T x)` | 向零方向取整 | 浮点 |
| `T __builtin_elementwise_nearbyint(T x)` | 按当前舍入方向取整，不产生 inexact 异常 | 浮点 |
| `T __builtin_elementwise_rint(T x)` | 按当前舍入方向取整，可能产生浮点异常 | 浮点 |
| `T __builtin_elementwise_canonicalize(T x)` | 平台特定的浮点数规范编码 | 浮点 |
| `T __builtin_elementwise_copysign(T x, T y)` | x 的绝对值配上 y 的符号 | 浮点 |
| `T __builtin_elementwise_fmod(T x, T y)` | (x/y) 的浮点余数，符号与 x 相同 | 浮点 |
| `T __builtin_elementwise_max(T x, T y)` | 较大的那个；浮点遵循 IEEE 754-2008 maxNum（已弃用，改用 maximum/maxnum） | 整数、浮点 |
| `T __builtin_elementwise_min(T x, T y)` | 较小的那个；浮点遵循 IEEE 754-2008 minNum（已弃用） | 整数、浮点 |
| `T __builtin_elementwise_maxnum(T x, T y)` | 较大的那个，遵循 IEEE 754-2008 maxNum（+0.0 > -0.0） | 浮点 |
| `T __builtin_elementwise_minnum(T x, T y)` | 较小的那个，遵循 IEEE 754-2008 minNum | 浮点 |
| `T __builtin_elementwise_add_sat(T x, T y)` | x + y 饱和加法（结果钳制在可表示范围内） | 整数 |
| `T __builtin_elementwise_sub_sat(T x, T y)` | x - y 饱和减法 | 整数 |
| `T __builtin_elementwise_maximum(T x, T y)` | 较大的那个，遵循 IEEE 754-2019 语义 | 浮点 |
| `T __builtin_elementwise_minimum(T x, T y)` | 较小的那个，遵循 IEEE 754-2019 语义 | 浮点 |
| `T __builtin_elementwise_maximumnum(T x, T y)` | 较大的那个，遵循 IEEE 754-2019 语义 | 浮点 |
| `T __builtin_elementwise_minimumnum(T x, T y)` | 较小的那个，遵循 IEEE 754-2019 语义 | 浮点 |
| `T __builtin_elementwise_fshl(T x, T y, T z)` | 漏斗左移：拼接 x（高位）与 y，左移 z（按位宽取模），取高位 | 整数 |
| `T __builtin_elementwise_fshr(T x, T y, T z)` | 漏斗右移：拼接 x（高位）与 y，右移 z（按位宽取模），取低位 | 整数 |
| `T __builtin_elementwise_clzg(T x[, T y])` | 前导 0 的个数；x 为 0 且提供第二参数时返回第二参数，否则未定义行为 | 整数 |
| `T __builtin_elementwise_ctzg(T x[, T y])` | 尾随 0 的个数；规则同 clzg | 整数 |
| `T __builtin_elementwise_clmul(T x, T y)` | 无进位乘法，返回宽结果的低有效位 | 整数 |
| `T __builtin_elementwise_pext(T x, T m)` | 按掩码 m 提取 x 中的位，连续压缩到结果的低有效位，其余置零 | 整数 |
| `T __builtin_elementwise_pdep(T x, T m)` | 把 x 的低有效位按掩码 m 的位置放置，其余置零 | 整数 |

**示例**：

```c
typedef float float4 __attribute__((ext_vector_type(4)));
float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
float4 b = {4.0f, 3.0f, 2.0f, 1.0f};
float4 c = __builtin_elementwise_max(a, b);   // {4, 3, 3, 4}
float4 d = __builtin_elementwise_sqrt(a);     // {1, 1.4142, 1.732, 2}
int4   e = __builtin_elementwise_add_sat(vi, vj); // 饱和加法
```

### 归约（Reduction）内建函数族

**功能**：每个内建把指定运算以递归"偶-奇成对"方式归约到所有向量元素上（`i*2` 与 `i*2+1` 成对，元素个数非 2 的幂时用中性元素补宽到下一个 2 的幂），返回标量结果。同时支持固定尺寸与可伸缩向量。以下整数归约内建可在 constexpr 中使用：`__builtin_reduce_max`、`__builtin_reduce_min`、`__builtin_reduce_add`、`__builtin_reduce_mul`、`__builtin_reduce_and`、`__builtin_reduce_or`、`__builtin_reduce_xor`。

```c
__builtin_reduce_add([e3, e2, e1, e0]) == __builtin_reduce_add([e3 + e2, e1 + e0])
                                     == (e3 + e2) + (e1 + e0)
```

| 内建函数 | 运算 | 支持的元素类型 |
|---|---|---|
| `ET __builtin_reduce_max(VT a)` | 返回最大元素；浮点结果除非全为 NaN 否则恒为数值 | 整数、浮点 |
| `ET __builtin_reduce_min(VT a)` | 返回最小元素 | 整数、浮点 |
| `ET __builtin_reduce_add(VT a)` | 求和 `+` | 整数 |
| `ET __builtin_reduce_mul(VT a)` | 求积 `*` | 整数 |
| `ET __builtin_reduce_and(VT a)` | 按位与 `&` | 整数 |
| `ET __builtin_reduce_or(VT a)` | 按位或 `\|` | 整数 |
| `ET __builtin_reduce_xor(VT a)` | 按位异或 `^` | 整数 |
| `ET __builtin_reduce_maximum(VT a)` | 最大元素，遵循 IEEE 754-2019 | 浮点 |
| `ET __builtin_reduce_minimum(VT a)` | 最小元素，遵循 IEEE 754-2019 | 浮点 |
| `ET __builtin_reduce_assoc_fadd(VT a[, ET s])` | 可结合浮点加法归约 | 浮点 |
| `ET __builtin_reduce_in_order_fadd(VT a, ET s)` | 有序浮点加法归约：从 lane 0 起按顺序累加，不可重结合 | 浮点 |

**示例**：

```c
typedef int v4si __attribute__((vector_size(16)));
v4si v = {1, 2, 3, 4};
int sum = __builtin_reduce_add(v);      // 10
int max = __builtin_reduce_max(v);      // 4
```

### 掩码（Masked）内建函数族

**功能**：按布尔掩码向量访问内存。第一个参数始终是布尔掩码向量；`__builtin_masked_load` 可带可选的第三个向量参数，作为被掩码关闭的 lane 的结果值。这些内建假定内存非对齐（需要对齐可用 `__builtin_assume_aligned`）。`expand_load`/`compress_store` 将结果存入连续下标（等价于 `if (mask[i]) val[i] = ptr[j++]` 与 `if (mask[i]) ptr[j++] = val[i]`）。`gather`/`scatter` 通过基指针 + 整数索引向量处理非连续内存访问。

**示例**：

```c
using v8b = bool [[clang::ext_vector_type(8)]];
using v8i = int  [[clang::ext_vector_type(8)]];

v8i load(v8b mask, int *ptr) { return __builtin_masked_load(mask, ptr); }
v8i load_expand(v8b mask, int *ptr) { return __builtin_masked_expand_load(mask, ptr); }
void store(v8b mask, v8i val, int *ptr) { __builtin_masked_store(mask, val, ptr); }
void store_compress(v8b mask, v8i val, int *ptr) { __builtin_masked_compress_store(mask, val, ptr); }
v8i gather(v8b mask, v8i idx, int *ptr) { return __builtin_masked_gather(mask, idx, ptr); }
void scatter(v8b mask, v8i val, v8i idx, int *ptr) { __builtin_masked_scatter(mask, idx, val, ptr); }
```

---

## 二、类型特征（Type Trait）内建

### `__builtin_is_implicit_lifetime`

**功能**：C++ 类型特征（GNU、Microsoft 兼容），判断类型是否为隐式生命周期（implicit lifetime）类型。

**示例**：

```cpp
static_assert(__builtin_is_implicit_lifetime(int));
```

### `__builtin_is_virtual_base_of`

**功能**：C++ 类型特征（GNU、Microsoft 兼容），判断一个类是否为另一个类的虚基类。

**示例**：

```cpp
struct A {};
struct B : virtual A {};
static_assert(__builtin_is_virtual_base_of(A, B));
```

### `__builtin_is_cpp_trivially_relocatable`

**功能**：返回类型是否"可平凡重定位"（trivially relocatable），即 C++26 标准 `[meta.unary.prop]` 的定义。注意：重定位时若对象是多态的，动态类型必须是最派生类型；不应拷贝填充字节。已取代已弃用的 `__is_trivially_relocatable`。

**示例**：

```cpp
static_assert(__builtin_is_cpp_trivially_relocatable(std::vector<int>));
```

### `__builtin_lt_synthesizes_from_spaceship` / `__builtin_gt_synthesizes_from_spaceship` / `__builtin_le_synthesizes_from_spaceship` / `__builtin_ge_synthesizes_from_spaceship`

**功能**：Clang 提供的内建，用于判断相应运算符（`<`、`>`、`<=`、`>=`）是否由飞船运算符（`<=>`）合成而来，常用于实现标准库中的空间飞船比较相关检测。

**示例**：

```cpp
static_assert(__builtin_lt_synthesizes_from_spaceship(std::strong_ordering));
```

### `__builtin_structured_binding_size`

**功能**：返回类型 `T` 的结构化绑定大小（即 `auto&& [...p] = declval<T&>();` 中包 `p` 的大小）。若参数不可分解，则 `__builtin_structured_binding_size(T)` 不是合法表达式（SFINAE 友好）。内建数组、内建 SIMD 向量、内建复数类型、tuple-like 类型、可分解的类类型都属于可分解类型。

**示例**：

```cpp
template<std::size_t Idx, typename T>
requires (Idx < __builtin_structured_binding_size(T))
decltype(auto) constexpr get_binding(T&& obj) {
    auto && [...p] = std::forward<T>(obj);
    return p...[Idx];
}
struct S { int a = 0, b = 42; };
static_assert(__builtin_structured_binding_size(S) == 2);
static_assert(get_binding<1>(S{}) == 42);
```

### `__builtin_trivially_relocate`

**功能**：把 `src` 处的 `count` 个可重定位（relocatable）、完整类型 `T` 的对象平凡地重定位到 `dest`，并返回 `dest`。用于实现 `std::trivially_relocate`。

**语法**：

```cpp
T* __builtin_trivially_relocate(T* dest, T* src, size_t count);
```

### 内建类型别名（Builtin type aliases）

**功能**：Clang 提供少量内建别名以提高某些元编程设施（metaprogramming）的吞吐量。

#### `__builtin_common_type`

**功能**：用于实现 `std::common_type` 的模板别名。若 `std::common_type` 应包含 `type` 成员，则它别名为 `HasTypeMember<TheCommonType>`；否则别名为 `HasNoTypeMember`。`BaseTemplate` 通常为 `std::common_type`；`Ts` 为传给 `std::common_type` 的参数。

**语法**：

```cpp
template <template <class... Args> class BaseTemplate,
          template <class TypeMember> class HasTypeMember,
          class HasNoTypeMember,
          class... Ts>
using __builtin_common_type = ...;
```

#### `__builtin_dedup_pack`

**功能**：接收一个模板参数包 `Ts`，产生一个新的未展开参数包，其中包含 `Ts` 中的全部唯一类型（去重），并保持各类型首次出现时的顺序。

**语法**：

```cpp
template <class... Ts>
using __builtin_dedup_pack = ...;
```

---

## 三、优化与控制流提示

### `__builtin_expect`

**功能**：提示优化器第一个参数 `expr` 的值预期等于第二个参数 `val`（常用于 `if`/`switch` 分支预测），总是返回 `expr`。

**语法**：

```c
long __builtin_expect(long expr, long val);
```

**示例**：

```c
if (__builtin_expect(x, 0)) {
   bar();
}
```

### `__builtin_expect_with_probability`

**功能**：与 `__builtin_expect` 类似，但第三个参数给出预期概率 `p`（必须在 `[0.0, 1.0]` 范围内），总是返回 `expr`。

**语法**：

```c
long __builtin_expect_with_probability(long expr, long val, double p);
```

**示例**：

```c
if (__builtin_expect_with_probability(x, 0, .3)) {
   bar();
}
```

### `__builtin_unpredictable`

**功能**：提示硬件（如分支预测器）无法预测该分支条件，期望用于 `if`/`switch` 等控制流条件。

**语法**：

```c
__builtin_unpredictable(long long);
```

**示例**：

```c
if (__builtin_unpredictable(x > 0)) {
   foo();
}
```

### `__builtin_unreachable`

**功能**：声明程序中某个点"永远不可到达"，帮助编译器优化并消除某些警告（如"声明为 noreturn 的函数不应返回"）。该内建本身是完全的未定义行为，编译器据此可生成更优代码。

**示例**：

```c
void myabort(void) __attribute__((noreturn));
void myabort(void) {
  asm("int3");
  __builtin_unreachable();
}
```

### `__builtin_assume`

**功能**：向优化器提供一个被定义为真的布尔不变量。参数表达式本身永不被求值（副作用被丢弃）；若运行时条件被违反，则行为未定义。优化器可利用该信息优化程序。

**示例**：

```c
int foo(int x) {
    __builtin_assume(x != 0);
    // 优化器可利用该不变量短路此检查
    if (x == 0)
          return do_something();
    return do_something_else();
}
```

### `__builtin_assume_separate_storage`

**功能**：告诉优化器两个参数指向分别分配的存储（不同变量定义或不同动态分配）。优化器可据此做别名分析。若两个参数指向同一存储，行为未定义（注意"存储"指最外层分配，因此绝不能传同一结构体内不同字段、同一数组内不同元素的地址）。

**示例**：

```c
int foo(int *x, int *y) {
    __builtin_assume_separate_storage(x, y);
    *x = 0;
    *y = 1;
    // 优化器可以优化成直接返回 0 而无需重新加载 *x
    return *x;
}
```

### `__builtin_assume_dereferenceable`

**功能**：告诉优化器从指针 `P` 起至少 `S`（常量，≥1）字节可解引用（`P` 不一定是非空指针；也不隐含超过 S 字节后不可解引用）。优化器可据此安全地预取/投机加载。

**示例**：

```c
int foo(int *x, int y) {
    __builtin_assume_dereferenceable(x, sizeof(int));
    int z = 0;
    if (y == 1) {
      // 因该假设保证 sizeof(int) 字节可安全投机加载，优化器可无条件执行加载
      z = *x;
    }
    return z;
}
```

### `__builtin_assume_aligned`（GCC 兼容）

**功能**：声明指针具有指定的对齐方式，返回与参数相同类型的对齐后的指针。通常配合 SIMD/掩码内建使用。

**示例**：

```c
float *p = (float *)__builtin_assume_aligned(ptr, 16);
```

### `__builtin_constant_p`（GCC 兼容）

**功能**：在编译期判断参数是否为编译期常量，是则返回 1，否则返回 0。常用于宏中生成常量/非常量两条路径。

**示例**：

```c
#define BIT_MASK(n) (__builtin_constant_p(n) ? ((1 << (n)) - 1) : (compute_mask(n)))
```

### `__builtin_choose_expr`（GCC 兼容）

**功能**：编译期条件选择表达式。第一个参数必须是编译期整数常量；若非零则整体取第二个表达式的类型与值，否则取第三个。只编译被选中的分支。

**示例**：

```c
#define is_const(x) __builtin_choose_expr(__builtin_constant_p(x), 1, 0)
```

### `__builtin_types_compatible_p`（GCC 兼容）

**功能**：判断两个类型是否兼容（在顶层忽略 const/volatile/restrict 限定符），兼容返回 1，否则返回 0。只能用于编译期条件。

**示例**：

```c
__builtin_types_compatible_p(long, unsigned long)  // 类型不同 → 0
__builtin_types_compatible_p(const int, int)       // 兼容 → 1
```

### `__builtin_prefetch`

**功能**：与缓存处理器通信，在使用数据之前把数据提前取入缓存，以避免缓存未命中。`addr` 为要取入缓存的地址；`rw` 表示访问模式（0=读，1=写/读写）；`locality` 表示数据在缓存中的预期持久性（0=用后即弃 ~ 3=复用很多次）。

**语法**：

```c
void __builtin_prefetch(const void *addr, int rw=0, int locality=3);
```

**示例**：

```c
__builtin_prefetch(a + i);
```

### `__builtin_unreachable` 相关 —— trap 内建

| 内建 | 功能 |
|---|---|
| `__builtin_trap()` | 使程序异常终止（lowered 到 `llvm.trap`） |
| `__builtin_debugtrap()` | 使程序停在可被调试器捕获的点（等价于在该行设置断点，lowered 到 `llvm.debugtrap`） |
| `__builtin_arm_trap(payload)` | AArch64 扩展，接受编译期常量 payload，直接编码进 trap 指令（`brk #payload`）供事后检查 |
| `__builtin_verbose_trap(category, reason)` | 异常终止并携带人类可读的终止原因描述（编码进调试信息中的人工内联帧）；需启用调试信息，否则等价于 `__builtin_trap` |

**示例**：

```c
void foo(int* p) {
  if (p == nullptr)
    __builtin_verbose_trap("check null", "Argument must not be null!");
}
```

### `__builtin_allow_runtime_check`

**功能**：返回当前位置的检查是否应被执行，用于实现可被优化器安全移除的 `assert` 类检查。返回值由编译器选项控制（`-mllvm -lower-allow-check-percentile-cutoff-hot=N`、`-mllvm -lower-allow-check-random-rate=P`；都未指定时所有检查都允许）。

**示例**：

```c
if (__builtin_allow_runtime_check("mycheck") && !ExpensiveCheck()) {
   abort();
}
```

### `__builtin_nondeterministic_value`

**功能**：返回一个与参数同类型的合法"非确定性"值（每次调用可能不同），当前支持整数、浮点、向量类型。

**示例**：

```c
int x = __builtin_nondeterministic_value(x);
float y = __builtin_nondeterministic_value(y);
__m256i a = __builtin_nondeterministic_value(a);
```

---

## 四、内存分配与地址

### `__builtin_alloca`

**功能**：在栈上动态分配内存，函数终止时自动释放（受栈分配限制约束）。

**语法**：

```c
__builtin_alloca(size_t n);
```

**示例**：

```c
int foo(size_t n) {
  auto mem = (float*)__builtin_alloca(n * sizeof(float));
  init(mem, n);
  process(mem, n);
  /* mem 在此处自动释放 */
}
```

### `__builtin_alloca_with_align`

**功能**：同 `__builtin_alloca`，但可控制对齐。第二参数为以**位**计的 2 的幂对齐约束（如 `CHAR_BIT * alignof(float)`）。

**语法**：

```c
__builtin_alloca_with_align(size_t n, size_t align);
```

**示例**：

```c
int foo(size_t n) {
  auto mem = (float*)__builtin_alloca_with_align(
                      n * sizeof(float),
                      CHAR_BIT * alignof(float));
  init(mem, n);
  process(mem, n);
  /* mem 在此处自动释放 */
}
```

### `__builtin_addressof`

**功能**：执行内建 `&` 运算符的功能，忽略任何重载的 `operator&`。在 C++11 常量表达式中，这是对重载了 `operator&` 的对象取地址的唯一方法。Clang 自动为其参数加上 `[[clang::lifetimebound]]`。

**示例**：

```cpp
template<typename T> constexpr T *addressof(T &value) {
  return __builtin_addressof(value);
}
```

### `__builtin_offsetof`

**功能**：实现 `offsetof` 宏，计算类型中指定子对象到对象起始处的字节偏移，返回 `size_t`，可用于整数常量表达式。Clang 扩展：C 语言模式下第一个参数可以是新类型的定义（作用域限于包含调用的最近作用域）。

**示例**：

```c
struct S {
  char c;
  int i;
  struct T { float f[2]; } t;
};

const int offset_to_i = __builtin_offsetof(struct S, i);
const int offset_to_subobject = __builtin_offsetof(struct S, t.f[1]);
```

### `__builtin_stack_address`

**功能**：返回分隔当前函数栈空间与"被调用函数可能修改的栈区域"的边界地址（与 GCC 同名内建语义一致）。在某些架构上（如 SPARCv9）需要对栈指针寄存器做调整，该内建会执行必要调整并返回正确的边界地址。

**示例**：

```c
void *sp = __builtin_stack_address();
```

### `__builtin_function_start`

**功能**：接受一个可常量求值为函数的参数，返回函数体的地址。该指针可能与通常的函数地址不同且**不可安全调用**（如启用 `-fsanitize=cfi` 时，常规函数地址是指向 CFI 跳转表的可调用指针，而该内建返回的地址会失败 cfi-icall 检查）。并非所有目标都支持。

**示例**：

```c
void a() {}
void *p = __builtin_function_start(a);
```

### `__builtin_object_size` / `__builtin_dynamic_object_size`

**功能**：返回 `ptr` 之后可访问的字节数 `n`，语义与 GCC 同名内建兼容。`type` 必须是 0~3 的整数常量：

- `type & 2 == 0`：返回最小的 `n`，使访问 `(const char*)ptr + n` 及之后被确认为越界；无更好边界时为 `(size_t)-1`。
- `type & 2 == 2`：返回最大的 `n`，使 0 <= i < n 的访问被确认在界内；无更好边界时为 `(size_t)0`。
- `type & 1 == 0`：与 `ptr` 处于同一存储（同一栈对象/全局/堆分配）即视为在界内。
- `type & 1 == 1`：仅与 `ptr` 指向的同一子对象在界内（数组元素仅同数组内算在界内）。

`__builtin_object_size` 完全在编译期确定；`__builtin_dynamic_object_size` 允许少量运行时求值以获得更精确结果，可作前者的直接替代品。

**示例**：

```c
char small[10], large[100];
bool cond;
// 返回 100：写入超过 100 字节被确认越界
int n100 = __builtin_object_size(cond ? small : large, 0);
// 返回 10：写入 <= 10 字节被确认在界内
int n10  = __builtin_object_size(cond ? small : large, 2);

// 动态版本：buffer 大小编译期未知时，__builtin_object_size 折叠为 -1，
// 而 __builtin_dynamic_object_size 折叠为 size
void copy_into_buffer(size_t size) {
  char* buffer = malloc(size);
  strlcpy(buffer, "some string", __builtin_dynamic_object_size(buffer, 0));
  // 注意：上一行在带安全检查的实现中会预处理展开为：
  // __builtin___strlcpy_chk(buffer, "some string", strlen("some string"),
  //                         __builtin_object_size(buffer, 0))
}
```

### `__builtin_operator_new` / `__builtin_operator_delete`

**功能**（仅 C++）：与直接调用 `::operator new(args)` / `::operator delete(args)` 完全相同，但允许 C++ 标准不允许直接函数调用进行的优化（如消除 new/delete 配对、合并分配），且调用必须解析到可替换的全局（解除）分配函数。用于实现 `std::allocator` 等分配库。

**示例**：

```cpp
void *p = __builtin_operator_new(sizeof(T));
__builtin_operator_delete(p);
// 检测：__has_builtin(__builtin_operator_new) >= 201802L 表示完整支持
```

### `__builtin_get_vtable_pointer`

**功能**：从多态 C++ 类的实例加载并（在相关平台上）认证主 vtable 指针，适用于使用 Pointer Authentication 的平台。被查询对象必须为多态且类型完整。

**示例**：

```cpp
struct PolymorphicClass { virtual ~PolymorphicClass(); };
PolymorphicClass anInstance;
const void* vtablePointer = __builtin_get_vtable_pointer(&anInstance);
```

### `__builtin_call_with_static_chain`

**功能**：以"静态链"调用约定执行调用：把 `ptr` 作为函数指针存入专用寄存器后调用表达式 `expr`（`expr` 必须是非成员静态调用表达式）。某些语言用它实现闭包或嵌套函数。

**示例**：

```c
auto v = __builtin_call_with_static_chain(foo(3), foo);
```

### `__builtin_counted_by_ref`

**功能**：返回指向 `counted_by` 属性所指定计数域的指针。参数必须是带 `counted_by` 属性的柔性数组成员或指针；若参数没有该属性则返回 `(void*)0`。用于防止在使用 `counted_by` 时"先访问柔性数组后设置计数"的常见错误（该内建的返回值不能赋给变量、取地址或传入/传出函数，否则违反边界安全约定）。

**示例**：

```c
#define alloc(P, FAM, COUNT) ({                                        \
   size_t __ignored_assignment;                                        \
   typeof(P) __p = NULL;                                               \
   __p = malloc(MAX(sizeof(*__p),                                     \
                    sizeof(*__p) + sizeof(*__p->FAM) * COUNT));        \
   *_Generic(                                                          \
     __builtin_counted_by_ref(__p->FAM),                               \
       void *: &__ignored_assignment,                                  \
       default: __builtin_counted_by_ref(__p->FAM)) = COUNT;           \
   __p;                                                                \
})
```

### `__builtin_preserve_access_index`

**功能**：指定一段代码区域内的数组下标访问和结构/联合成员访问在 BPF "compile-once run-everywhere" 框架下是可重定位的。需要调试信息（`-g`），否则编译报错。返回类型与参数类型相同。

**示例**：

```c
struct t { int i; int j; union { int a; int b; } c[4]; };
struct t *v = ...;
int *pb = __builtin_preserve_access_index(&v->c[3].b);
__builtin_preserve_access_index(v->j);
```

### `__builtin_invoke`

**功能**：等价于 `std::invoke`（仅 C++ 标准库实现使用）。

**语法**：

```cpp
template <class Callee, class... Args>
decltype(auto) __builtin_invoke(Callee&& callee, Args&&... args);
```

### `__builtin_nontemporal_load` / `__builtin_nontemporal_store`

**功能**：重载内建，用于生成非时态（non-temporal）内存访问（绕过缓存，适合流式访问大数据块）。当前支持的类型：整数类型、浮点类型、向量类型。注意：编译器不保证一定会生成非时态的加载/存储指令。

**语法**：

```c
T   __builtin_nontemporal_load(T *addr);
void __builtin_nontemporal_store(T value, T *addr);
```

**示例**：

```c
float __attribute__((vector_size(16))) v = __builtin_nontemporal_load(ptr);
__builtin_nontemporal_store(v, dst);
```

---

## 五、位操作内建

### `__builtin_bitreverse{8,16,32,64}`

**功能**：反转整数值的比特模式，如 `0b10110110` 变为 `0b01101101`。可在常量表达式中使用。

**示例**：

```c
uint8_t  rev_x = __builtin_bitreverse8(x);
uint16_t rev_x = __builtin_bitreverse16(x);
uint32_t rev_y = __builtin_bitreverse32(y);
uint64_t rev_z = __builtin_bitreverse64(z);
```

### `__builtin_rotateleft8` / `__builtin_rotateleft16` / `__builtin_rotateleft32` / `__builtin_rotateleft64` / `__builtin_rotateright8` / `__builtin_rotateright16` / `__builtin_rotateright32` / `__builtin_rotateright64`

**功能**：把第一个参数按第二个参数指定的位数循环左移/右移。移位量按参数位宽取模（无符号）。如 `0b10000110` 左移 11 位变为 `0b00110100`；右移 3 位变为 `0b11010000`。可在常量表达式中使用。

**示例**：

```c
uint8_t  rot_x = __builtin_rotateleft8(x, y);
uint32_t rot_x = __builtin_rotateleft32(x, y);
uint64_t rot_x = __builtin_rotateright64(x, y);
```

### `__builtin_stdc_rotate_left` / `__builtin_stdc_rotate_right`

**功能**：把 `value` 的位旋转 `count` 位。`value` 必须是无符号整数类型（包括 `_BitInt` 类型），`count` 可为任意整数类型。旋转次数按位宽取模，负数转换为等价的正旋转（如左移 -1 等价于左移 BitWidth-1）。可在常量表达式中使用。

**示例**：

```c
unsigned char rotated_left  = __builtin_stdc_rotate_left((unsigned char)0xB1, 3);
unsigned int  rotated_right = __builtin_stdc_rotate_right(0x12345678U, 8);
unsigned char neg_rotate    = __builtin_stdc_rotate_left((unsigned char)0xB1, -1);
unsigned _BitInt(20) rotated = __builtin_stdc_rotate_left(value, 5);
```

### `__builtin_stdc_*` 位工具（实现 C23 `<stdbit.h>`）

**功能**：实现 C23 `<stdbit.h>` 的全部操作。`T` 为除 `bool` 和枚举类型外的任意无符号整数类型（含 `_BitInt` 类型——这是 Clang 扩展，C23 只要求支持与标准/扩展整数类型宽度一致的位精确整数）。计数/位置查询返回 `unsigned int`；`has_single_bit` 返回 `bool`；`bit_floor`/`bit_ceil` 返回与操作数相同的类型。0 与全 1 情形遵循 C23 定义。全部可在常量表达式中使用。

| 内建函数 | 功能 |
|---|---|
| `unsigned int __builtin_stdc_leading_zeros(T value)` | 前导 0 个数 |
| `unsigned int __builtin_stdc_leading_ones(T value)` | 前导 1 个数 |
| `unsigned int __builtin_stdc_trailing_zeros(T value)` | 尾随 0 个数 |
| `unsigned int __builtin_stdc_trailing_ones(T value)` | 尾随 1 个数 |
| `unsigned int __builtin_stdc_first_leading_zero(T value)` | 第一个前导 0 的位置 |
| `unsigned int __builtin_stdc_first_leading_one(T value)` | 第一个前导 1 的位置 |
| `unsigned int __builtin_stdc_first_trailing_zero(T value)` | 第一个尾随 0 的位置 |
| `unsigned int __builtin_stdc_first_trailing_one(T value)` | 第一个尾随 1 的位置 |
| `unsigned int __builtin_stdc_count_zeros(T value)` | 0 的个数 |
| `unsigned int __builtin_stdc_count_ones(T value)` | 1 的个数 |
| `bool __builtin_stdc_has_single_bit(T value)` | 是否恰有 1 个位为 1（2 的幂） |
| `unsigned int __builtin_stdc_bit_width(T value)` | 表示 value 所需的位数 |
| `T __builtin_stdc_bit_floor(T value)` | 向下取到 2 的幂 |
| `T __builtin_stdc_bit_ceil(T value)` | 向上取到 2 的幂 |

**示例**：

```c
unsigned _BitInt(9) x = 0x11;
unsigned int lz = __builtin_stdc_leading_zeros(x);
unsigned int tz = __builtin_stdc_trailing_zeros(x);
unsigned int fto = __builtin_stdc_first_trailing_one(x);
bool has_one = __builtin_stdc_has_single_bit(x);
unsigned _BitInt(9) ceilv  = __builtin_stdc_bit_ceil((unsigned _BitInt(9))5);
unsigned _BitInt(9) floorv = __builtin_stdc_bit_floor((unsigned _BitInt(9))5);
```

### `__builtin_clz{,l,ll,s}` / `__builtin_ctz{,l,ll,s}` / `__builtin_clrsb{,l,ll}` / `__builtin_ffs{,l,ll}` / `__builtin_parity{,l,ll}` / `__builtin_popcount{,l,ll}`（GCC 兼容）

完整变体列表：`__builtin_clz`、`__builtin_clzl`、`__builtin_clzll`、`__builtin_clzs`、`__builtin_ctz`、`__builtin_ctzl`、`__builtin_ctzll`、`__builtin_ctzs`、`__builtin_clrsb`、`__builtin_clrsbl`、`__builtin_clrsbll`、`__builtin_ffs`、`__builtin_ffsl`、`__builtin_ffsll`、`__builtin_parity`、`__builtin_parityl`、`__builtin_parityll`、`__builtin_popcount`、`__builtin_popcountl`、`__builtin_popcountll`

**功能**（GCC 内建家族，均可用于常量表达式）：

- `clz`：前导 0 的个数（参数为 0 时结果未定义，注意 `clzs` 处理 `short`/`_BitInt` 变体）。
- `ctz`：尾随 0 的个数。
- `clrsb`：返回去掉符号位后前导符号位的个数（冗余符号位计数）。
- `ffs`：第一个（最低位）置 1 的位的位置（从 1 开始；参数为 0 返回 0）。
- `parity`：位 1 个数的奇偶性。
- `popcount`：1 的位数。

**示例**：

```c
int n = __builtin_popcountll(0xF0F0ULL);   // 8
int lz = __builtin_clz(0x00FF);            // 高 16 位为 0 → 16
int p  = __builtin_parity(0b1011);         // 3 个 1 → 1
```

### `__builtin_popcountg` / `__builtin_clzg` / `__builtin_ctzg`

**功能**：`popcount`/`clz`/`ctz` 的类型通用（type-generic）版本，参数可以是任意无符号整数类型（含 `unsigned __int128`、C23 `unsigned _BitInt(N)`）或固定布尔向量。对布尔向量，把向量解释为位域（第 i 个元素是位 i，从最低有效端计数）：`clzg` 返回向量末尾连续 0 元素个数，`ctzg` 返回向量开头连续 0 元素个数。若第一个参数为 0 且提供了可选的 `int` 第二参数，则返回第二参数；若为 0 且未提供第二参数，行为未定义。

**语法**：

```c
int __builtin_popcountg(type x);
int __builtin_clzg(type x[, int fallback]);
int __builtin_ctzg(type x[, int fallback]);
```

**示例**：

```c
unsigned _BitInt(128) z = 7;
int z_pop = __builtin_popcountg(z);   // 3
int z_lz  = __builtin_clzg(z);        // 125
int z_tz  = __builtin_ctzg(z);        // 0
int fb    = __builtin_clzg(0u, 42);   // 参数为 0 → 返回 42
```

### `__builtin_bswap16` / `__builtin_bswap32` / `__builtin_bswap64`

**功能**：字节交换（大小端转换）。`__builtin_bswap16` 把 16 位值的高低位字节交换，`bswap32/64` 类似。可用于常量表达式。

**示例**：

```c
uint32_t swapped = __builtin_bswap32(0x11223344); // 0x44332211
```

---

## 六、多精度算术与检查算术

### 多精度算术内建：`__builtin_addc{,,b,s,l,ll}` / `__builtin_subc{,,b,s,l,ll}`

**功能**：以适合 C 语言的方式暴露多精度算术——带进位/借位的加减法，便于手工构造任意精度加法/减法链。`carryin`/`carryout` 为进位位；`b/s/l/ll` 后缀对应 `unsigned char`/`unsigned short`/`unsigned`/`unsigned long`/`unsigned long long`。均可在常量表达式中使用。

**完整列表**：

```c
unsigned char      __builtin_addcb (unsigned char x, unsigned char y, unsigned char carryin, unsigned char *carryout);
unsigned short     __builtin_addcs (unsigned short x, unsigned short y, unsigned short carryin, unsigned short *carryout);
unsigned           __builtin_addc  (unsigned x, unsigned y, unsigned carryin, unsigned *carryout);
unsigned long      __builtin_addcl (unsigned long x, unsigned long y, unsigned long carryin, unsigned long *carryout);
unsigned long long __builtin_addcll(unsigned long long x, unsigned long long y, unsigned long long carryin, unsigned long long *carryout);
unsigned char      __builtin_subcb (unsigned char x, unsigned char y, unsigned char carryin, unsigned char *carryout);
unsigned short     __builtin_subcs (unsigned short x, unsigned short y, unsigned short carryin, unsigned short *carryout);
unsigned           __builtin_subc  (unsigned x, unsigned y, unsigned carryin, unsigned *carryout);
unsigned long      __builtin_subcl (unsigned long x, unsigned long y, unsigned long carryin, unsigned long *carryout);
unsigned long long __builtin_subcll(unsigned long long x, unsigned long long y, unsigned long long carryin, unsigned long long *carryout);
```

**示例**（多精度加法链）：

```c
unsigned *x, *y, *z, carryin=0, carryout;
z[0] = __builtin_addc(x[0], y[0], carryin, &carryout);
carryin = carryout;
z[1] = __builtin_addc(x[1], y[1], carryin, &carryout);
carryin = carryout;
z[2] = __builtin_addc(x[2], y[2], carryin, &carryout);
carryin = carryout;
z[3] = __builtin_addc(x[3], y[3], carryin, &carryout);
```

### 检查算术内建：`__builtin_{add,sub,mul}_overflow` 及 `__builtin_{uadd,usub,umul,sadd,ssub,smul}{,l,ll}_overflow`

**功能**：为安全关键应用提供快速、易表达的检查算术。每个内建对前两个参数执行指定运算并把结果存入第三个参数（指针）：若结果等于数学正确值则返回 0；否则返回 1，且结果等于数学正确值对 2^k（k 为结果类型位数）取模的唯一值。对所有参数值行为都有良好定义。前三个（`add/sub/mul_overflow`）对任意整数类型（含 bool）通用，操作数不必与结果同类型；其余变体（`u*` 无符号、`s*` 有符号、`l`/`ll` 为 long/long long）会在运算前隐式提升/转换操作数。

**示例**：

```c
errorcode_t security_critical_application(...) {
  unsigned x, y, result;
  ...
  if (__builtin_mul_overflow(x, y, &result))
    return kErrorCodeHackers;
  ...
  use_multiply(result);
  ...
}
```

完整签名列表：

```c
bool __builtin_add_overflow   (type1 x, type2 y, type3 *sum);   // 通用：任意整数类型
bool __builtin_sub_overflow   (type1 x, type2 y, type3 *diff);
bool __builtin_mul_overflow   (type1 x, type2 y, type3 *prod);
bool __builtin_uadd_overflow  (unsigned x, unsigned y, unsigned *sum);
bool __builtin_uaddl_overflow (unsigned long x, unsigned long y, unsigned long *sum);
bool __builtin_uaddll_overflow(unsigned long long x, unsigned long long y, unsigned long long *sum);
bool __builtin_usub_overflow  (unsigned x, unsigned y, unsigned *diff);
bool __builtin_usubl_overflow (unsigned long x, unsigned long y, unsigned long *diff);
bool __builtin_usubll_overflow(unsigned long long x, unsigned long long y, unsigned long long *diff);
bool __builtin_umul_overflow  (unsigned x, unsigned y, unsigned *prod);
bool __builtin_umull_overflow (unsigned long x, unsigned long y, unsigned long *prod);
bool __builtin_umulll_overflow(unsigned long long x, unsigned long long y, unsigned long long *prod);
bool __builtin_sadd_overflow  (int x, int y, int *sum);
bool __builtin_saddl_overflow (long x, long y, long *sum);
bool __builtin_saddll_overflow(long long x, long long y, long long *sum);
bool __builtin_ssub_overflow  (int x, int y, int *diff);
bool __builtin_ssubl_overflow (long x, long y, long *diff);
bool __builtin_ssubll_overflow(long long x, long long y, long long *diff);
bool __builtin_smul_overflow  (int x, int y, int *prod);
bool __builtin_smull_overflow (long x, long y, long *prod);
bool __builtin_smulll_overflow(long long x, long long y, long long *prod);
```

---

## 七、浮点内建

### `__builtin_isfpclass`

**功能**：测试浮点值是否属于指定的浮点类别（`isnan`/`isinf`/`isfinite` 等的通用化）。第一个参数是浮点值（标量或向量），第二个参数是整数常量位掩码，其中每一位代表一个数据类别。该函数绝不引发浮点异常、不规范化输入、不提升参数。

掩码取值：

| 掩码值 | 数据类别 | 宏 |
|---|---|---|
| 0x0001 | Signaling NaN | `__FPCLASS_SNAN` |
| 0x0002 | Quiet NaN | `__FPCLASS_QNAN` |
| 0x0004 | 负无穷 | `__FPCLASS_NEGINF` |
| 0x0008 | 负规格化数 | `__FPCLASS_NEGNORMAL` |
| 0x0010 | 负次规格化数 | `__FPCLASS_NEGSUBNORMAL` |
| 0x0020 | 负零 | `__FPCLASS_NEGZERO` |
| 0x0040 | 正零 | `__FPCLASS_POSZERO` |
| 0x0080 | 正次规格化数 | `__FPCLASS_POSSUBNORMAL` |
| 0x0100 | 正规格化数 | `__FPCLASS_POSNORMAL` |
| 0x0200 | 正无穷 | `__FPCLASS_POSINF` |

**示例**：

```c
if (__builtin_isfpclass(x, 448)) {   // 448 = 0x1C0 = 正零 | 正次规格化 | 正规格化
   // x 是正的有限值
}
// __builtin_isfpclass(x, 3)  == isnan(x)
// __builtin_isfpclass(x, 504) == isfinite(x)
```

### `__builtin_canonicalize{,f,l}`

**功能**：返回浮点数的平台特定规范编码（canonical encoding），对实现 `frexp` 等数值原语很有用。语义参见 LLVM canonicalize 内建。

**语法**：

```c
double     __builtin_canonicalize(double);
float      __builtin_canonicalizef(float);
long double __builtin_canonicalizel(long double);
```

### `__builtin_flt_rounds` / `__builtin_set_flt_rounds`

**功能**：读取/设置当前浮点舍入模式。编码与 C 标准 `FLT_ROUNDS` 相同：0=向零、1=向最近偶数、2=向正无穷、3=向负无穷、4=向最近远离零。`__builtin_set_flt_rounds` 目前仅支持 x86、x86_64、PowerPC、PowerPC64、ARM、AArch64 目标。读取/修改浮点环境并不总是被允许，可能产生意外行为。

**语法**：

```c
int  __builtin_flt_rounds();
void __builtin_set_flt_rounds(int);
```

### 浮点分类内建（GCC 兼容，可用于常量表达式）

| 内建 | 功能 |
|---|---|
| `__builtin_isnan(x)` | 是否为 NaN |
| `__builtin_isinf(x)` | 是否为正/负无穷 |
| `__builtin_isinf_sign(x)` | 是否无穷并带符号 |
| `__builtin_isfinite(x)` | 是否有限 |
| `__builtin_isnormal(x)` | 是否规格化数 |
| `__builtin_fpclassify(x)` | 浮点分类 |
| `__builtin_nan("str")` / `__builtin_nans("str")` | 构造安静 NaN / 发信号 NaN |
| `__builtin_inf()` | 构造无穷 |
| `__builtin_fmax(x,y)` / `__builtin_fmin(x,y)` | 最大值/最小值（NaN 忽略） |

**示例**：

```c
#if __has_constexpr_builtin(__builtin_fmax)
  constexpr
#endif
  double money_fee(double amount) {
      return __builtin_fmax(amount * 0.03, 10.0);
  }
```

---

## 八、字符串与内存操作内建

### 字符串内建（`<string.h>` / `<wchar.h>` 的常量表达式求值支持）

**功能**：Clang 为以下标准库函数提供内建形式（内建名 = 库函数名加 `__builtin_` 前缀），并支持常量表达式求值：`memchr`、`memcmp`（及其弃用的 BSD/POSIX 别名 `bcmp`）、`strchr`、`strcmp`、`strlen`、`strncmp`、`wcschr`、`wcscmp`、`wcslen`、`wcsncmp`、`wmemchr`、`wmemcmp`。`__builtin_mem*` 函数的常量求值支持仅对 `char`、`signed char`、`unsigned char`、`char8_t` 数组提供。可用 `__has_feature(cxx_constexpr_string_builtins)` 检测。

**示例**：

```c
void *p = __builtin_memchr("foobar", 'b', 5);
```

### `__builtin_char_memchr`

**功能**：`__builtin_char_memchr(a, b, c)` 与 `(char*)__builtin_memchr(a, b, c)` 完全相同，区别在于它允许在 C++11 及之后的常量表达式中使用（而一般情况下不允许把 `void*` 强转为 `char*`）。

**语法**：

```c
char *__builtin_char_memchr(const char *haystack, int needle, size_t size);
```

### 内存内建（`memcpy`/`memmove` 等的常量表达式求值支持）

**功能**：Clang 为 `memcpy`、`memmove`、`wmemcpy`、`wmemmove` 提供内建形式（加 `__builtin_` 前缀），并支持常量表达式求值——仅当源与目标是指向相同平凡可复制元素类型的数组指针、给定大小是元素大小的精确倍数且不超过源/目标可访问元素个数时。

**示例**：

```c
__builtin_memcpy(dst, src, sizeof(int) * 4);
```

### `__builtin_memcpy_inline` / `__builtin_memset_inline`

**功能**：保证内联的拷贝/填充原语，作为高效 `memcpy`/`memset` 实现的构建块。与 `__builtin_memcpy`/`__builtin_memset` 相同但保证不调用任何外部函数。适用于实现自定义 libc 版本或没有 libc 的环境。`size` 参数必须是编译期常量；不能在 constexpr 上下文中调用。

**语法**：

```c
void __builtin_memcpy_inline(void *dst, const void *src, size_t size);
void __builtin_memset_inline(void *dst, int value, size_t size);
```

---

## 九、变长参数（va_*）内建

**功能**：一组用于实现 `<stdarg.h>` 的内建：

- `__builtin_va_list`：目标相关的 `va_list` 类型的预定义 typedef。对该类型进行字节级拷贝（memcpy 等）是未定义行为，合法拷贝只能通过 `va_copy` 或 `__builtin_va_copy` 产生。
- `void __builtin_va_start(__builtin_va_list list, <parameter-name>)`：初始化 va_list。`parameter-name` 是省略号前最后一个参数的名称；在 C23 及以后模式可传整数常量 `0`（若省略号前没有参数）。对已初始化的 va_list 调用是未定义行为。
- `void __builtin_c23_va_start(__builtin_va_list list, ...)`：仅 C23 及以后可用；接受零或一个变长参数（若提供，应为省略号前参数名，用于兼容 C23 之前的版本）。提供两个及以上变长参数是错误的。
- `void __builtin_va_end(__builtin_va_list list)`：终结 va_list，之后不可再用（除非重新初始化）。对未初始化的 list 调用是未定义行为。
- `<type-name> __builtin_va_arg(__builtin_va_list list, <type-name>)`：返回下一个变长参数的值。没有下一个变长参数或类型不兼容时行为未定义。
- `void __builtin_va_copy(__builtin_va_list dest, __builtin_va_list src)`：把 dest 初始化为 src 的拷贝。对已初始化的 dest 调用是未定义行为。

**示例**：

```c
#include <stdio.h>
int sum(int count, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, count);
  int s = 0;
  for (int i = 0; i < count; ++i)
    s += __builtin_va_arg(args, int);
  __builtin_va_end(args);
  return s;
}
```

---

## 十、源位置内建

**功能**：支持 C++20 `std::source_location` 的实现。这些内建在"调用点"返回对应的宏值，都是常量表达式：

- `const char *__builtin_FILE()`：`__FILE__`（完整路径）。
- `const char *__builtin_FILE_NAME()`：`__FILE_NAME__`（仅文件名，Clang 独有）。
- `const char *__builtin_FUNCTION()`：`__FUNCTION__`（不在函数作用域时返回空串）。
- `const char *__builtin_FUNCSIG()`：`__FUNCSIG__`（Microsoft 风格签名）。
- `unsigned __builtin_LINE()`：`__LINE__`。
- `unsigned __builtin_COLUMN()`：列号（从第 1 列开始，Clang 独有）。
- `const std::source_location::__impl *__builtin_source_location()`：指向常量静态数据（该类型必须已定义，且恰好包含 `_M_file_name`、`_M_function_name`、`_M_line`、`_M_column` 四个字段；函数名用 `__PRETTY_FUNCTION__`）。

当内建出现在默认函数参数中时，"调用点"是调用者的位置；出现在默认成员初始化器中时是构造函数/聚合初始化的位置。

**示例**：

```c
void my_assert(bool pred, int line = __builtin_LINE(),   // 捕获调用者的行号
               const char* file = __builtin_FILE(),
               const char* function = __builtin_FUNCTION()) {
  if (pred) return;
  printf("%s:%d assertion failed in function %s\n", file, line, function);
  std::abort();
}
```

---

## 十一、对齐内建

**功能**：检查/调整指针和整数的对齐，避免依赖"对指针派生的整数做算术"的实现定义行为，同时保留类型信息并能对对齐值做语义检查。对齐以字节为单位表达，必须是 2 的幂（编译期可判定非 2 的幂会编译失败；运行时非 2 的幂行为未定义）。可用于所有整数类型及（非函数）指针类型；对指针向上/向下对齐的结果必须位于同一底层分配内或其后一位（one past the end）。

- `Type __builtin_align_up(Type value, size_t alignment)`：向上对齐到 alignment 的下一个倍数；已对齐则原样返回。
- `Type __builtin_align_down(Type value, size_t alignment)`：向下对齐。
- `bool __builtin_is_aligned(Type value, size_t alignment)`：判断是否对齐。

**示例**：

```c
char* global_alloc_buffer;
void* my_aligned_allocator(size_t alloc_size, size_t alignment) {
  char* result = __builtin_align_up(global_alloc_buffer, alignment);
  global_alloc_buffer = result + alloc_size;
  return result;
}

void example(char* buffer) {
   if (__builtin_is_aligned(buffer, 64)) {
     do_fast_aligned_copy(buffer);
   } else {
     do_unaligned_copy(buffer);
   }
}

// 也支持整数类型并可在常量表达式中求值
static_assert(__builtin_align_up(123, 64) == 128, "");
static_assert(__builtin_align_down(123u, 64) == 64u, "");
static_assert(!__builtin_is_aligned(123, 64), "");
```

---

## 十二、协程（Coroutine）内建

> 警告：以下为实验性内建，跨 Clang/LLVM 版本的兼容性不保证。

**功能**：支持 P0057 定义的 C++ 协程。前四个内建用于标准库实现 `std::coroutine_handle`：

```c
void  __builtin_coro_resume(void *addr);            // 恢复协程
void  __builtin_coro_destroy(void *addr);           // 销毁协程
bool  __builtin_coro_done(void *addr);              // 协程是否完成
void *__builtin_coro_promise(void *addr, int alignment, bool from_promise); // 协程帧与 promise 互转
```

其他协程内建供 Clang 内部或协程功能开发使用：

```c
size_t __builtin_coro_size()                          // 协程帧大小
void  *__builtin_coro_frame()                         // 当前协程帧
void  *__builtin_coro_free(void *coro_frame)          // 释放协程帧
void  *__builtin_coro_id(int align, void *promise, void *fnaddr, void *parts)
bool   __builtin_coro_alloc()                         // 是否需要堆分配
void  *__builtin_coro_begin(void *memory)             // 开始协程帧生命周期
void   __builtin_coro_end(void *coro_frame, bool unwind)
char   __builtin_coro_suspend(bool final)             // 挂起（返回 -1/0/1 表示行为）
```

`__builtin_suspend` 会向 `llvm.coro.suspend` 内建插入 `token none` 作为第一个参数。注意没有对应 `llvm.coro.save` 的内建（LLVM 在第一个参数为 token none 时会自动插入）。

**示例**：

```cpp
template <> struct coroutine_handle<void> {
  void resume() const { __builtin_coro_resume(ptr); }
  void destroy() const { __builtin_coro_destroy(ptr); }
  bool done() const { return __builtin_coro_done(ptr); }
protected:
  void *ptr;
};
```

---

## 十三、WebAssembly 表内建

### `__builtin_wasm_table_set`

**功能**：向 WebAssembly 表存储引用类型值。三个参数：表、存储索引、要存的引用类型值。无返回值。

**示例**：

```c
static __externref_t table[0];
extern __externref_t JSObj;

void store(int index) {
  __builtin_wasm_table_set(table, index, JSObj);
}
```

### `__builtin_wasm_table_get`

**功能**：`__builtin_wasm_table_set` 的对应操作，从 WebAssembly 表中加载引用类型值。两个参数：表、加载索引，返回加载的引用类型值。

**示例**：

```c
static __externref_t table[0];

__externref_t load(int index) {
  __externref_t Obj = __builtin_wasm_table_get(table, index);
  return Obj;
}
```

### `__builtin_wasm_table_size`

**功能**：返回 WebAssembly 表的当前大小（`size_t`）。

**示例**：

```c
typedef void (*__funcref funcref_t)();
static funcref_t table[0];

size_t getSize() {
  return __builtin_wasm_table_size(table);
}
```

### `__builtin_wasm_table_grow`

**功能**：按指定数量增长 WebAssembly 表。三个参数：表、要存入新表项的引用类型值（初始化值）、增长数量。返回之前的表大小，或 -1（分配空间不足时）。由于 C/C++ 中创建的 WebAssembly 表都是零大小的，通常必须先调用它来增长表。

**示例**：

```c
typedef void (*__funcref funcref_t)();
static funcref_t table[0];

// grow 返回新的表大小，出错时返回 -1
int grow(funcref_t fn, int delta) {
  int prevSize = __builtin_wasm_table_grow(table, fn, delta);
  if (prevSize == -1)
    return -1;
  return prevSize + delta;
}
```

### `__builtin_wasm_table_fill`

**功能**：把 WebAssembly 表的一段区间全部设为给定引用类型值。四个参数：表、起始索引、要设的值、区间大小。无返回值。

**示例**：

```c
static __externref_t table[0];

// 把整张表重置为给定值
void reset(__externref_t Obj) {
  int Size = __builtin_wasm_table_size(table);
  __builtin_wasm_table_fill(table, 0, Obj, Size);
}
```

### `__builtin_wasm_table_copy`

**功能**：把 WebAssembly 表的一段元素复制到（可能重叠的）目标区域。五个参数：目标表、源表、目标起始索引、源起始索引、要复制的元素个数。无返回值。

**示例**：

```c
static __externref_t tableSrc[0];
static __externref_t tableDst[0];

// 把 tableSrc 中 [src, src+nelem-1] 复制到 tableDst 中 [dst, dst+nelem-1]
void copy(int dst, int src, int nelem) {
  __builtin_wasm_table_copy(tableDst, tableSrc, dst, src, nelem);
}
```

---

## 十四、目标相关内建

### ARM/AArch64 低层独占内存内建

**功能**：直接访问实现原子操作的三个关键 ARM 指令（加载独占/存储独占）。支持宽度 ≤64 位的整数（AArch64 上 ≤128 位）、浮点类型、指针类型。注意：编译器不保证不会在 `ldrex` 与其配对的 `strex` 之间插入会清除独占监视器的存储，因此最好不要对自动存储期（栈上）变量使用这些操作。应优先使用更高层的原子原语。

```c
T   __builtin_arm_ldrex(const volatile T *addr);   // 独占加载（ldrex）
T   __builtin_arm_ldaex(const volatile T *addr);   // 获取语义独占加载（ldaex）
int __builtin_arm_strex(T val, volatile T *addr);  // 独占存储（strex），0=成功 1=失败
int __builtin_arm_stlex(T val, volatile T *addr);  // 释放语义独占存储（stlex）
void __builtin_arm_clrex(void);                    // 清除独占监视器（clrex）
```

**示例**：

```c
int __atomic_impl_add(volatile int *p, int delta) {
  for (;;) {
    int old = __builtin_arm_ldrex(p);
    if (__builtin_arm_strex(old + delta, p) == 0)
      return old;
  }
}
```

### PowerPC `__builtin_setrnd`

**功能**：设置浮点舍入模式（PowerPC64/PowerPC64le）。使用整数参数的最低两位：0=就近舍入、1=向零、2=向+∞、3=向-∞。模式参数按 4 取模（如 `__builtin_setrnd(102)` 等于 `__builtin_setrnd(2)`）。

**语法**：

```c
double __builtin_setrnd(int mode);
```

### PowerPC `__builtin_dcbf`

**功能**：数据缓存块刷新——把数据缓存中已修改的块内容复制到主存并从数据缓存中清除该副本。

**语法**：

```c
void __dcbf(const void* addr); /* Data Cache Block Flush */
```

**示例**：

```c
int a = 1;
__builtin_dcbf (&a);
```

### AMDGPU 内建

| 内建 | 功能 |
|---|---|
| `__builtin_amdgcn_fence(ordering, scope[, "local"/"global" ...])` | 发出内存屏障。ordering 为 `__ATOMIC_*`；scope 如 `"workgroup"`、`"agent"`；可选的地址空间名字符串（`"local"`/`"global"`）指定需要排序的地址空间，不提供则栅栏所有地址空间 |
| `__builtin_amdgcn_processor_is("gfxNNNN")` | 查询当前目标处理器是否为指定型号，返回 `__amdgpu_feature_predicate_t`（可隐式转换为 bool） |
| `__builtin_amdgcn_is_invocable(builtin_name)` | 查询当前目标处理器能否调用某内建 |
| `__builtin_amdgcn_ballot_w{32,64}(bool)` | 返回位掩码：当前 wave 中每个活跃（收敛）lane 的布尔参数作为一位，非活跃 lane 为 0；结果是 wave 内统一的 |
| `__builtin_amdgcn_inverse_ballot_w{32,64}(mask)` | 对 wave 统一位掩码，返回当前 lane 位置对应的位（近似 `(mask & (1 << lane_id)) != 0`，但仅在掩码对所有活跃 lane 相同的情况下有定义行为） |
| `__builtin_amdgcn_av_load_b128(v4u *src, int scope)` | 以指定 `__MEMORY_SCOPE_*` 缓存行为加载 4 个 unsigned int 的向量（指针必须指向 global/generic 地址空间；gfx9~gfx12 支持） |
| `__builtin_amdgcn_av_store_b128(v4u *dst, v4u data, int scope)` | 以指定缓存行为存储 4 个 unsigned int 的向量 |

**示例**：

```c
// 栅栏所有地址空间
__builtin_amdgcn_fence(__ATOMIC_SEQ_CST, "workgroup");
// 只栅栏指定地址空间
__builtin_amdgcn_fence(__ATOMIC_SEQ_CST, "workgroup", "local", "global");

if (__builtin_amdgcn_processor_is("gfx1201") ||
    __builtin_amdgcn_is_invocable(__builtin_amdgcn_s_sleep_var))
  __builtin_amdgcn_s_sleep_var(x);

if (__builtin_amdgcn_is_invocable(__builtin_amdgcn_s_wait_event_export_ready))
  __builtin_amdgcn_s_wait_event_export_ready();
else if (__builtin_amdgcn_is_invocable(__builtin_amdgcn_s_ttracedata_imm))
  __builtin_amdgcn_s_ttracedata_imm(1);

while (__builtin_amdgcn_is_invocable(__builtin_amdgcn_global_load_tr_b64_i32)) break;
for (; __builtin_amdgcn_is_invocable(__builtin_amdgcn_permlane64); ++*p) break;
```

### x86/x86-64 段寄存器内存引用

**功能**：把指针标注为地址空间 #256（GS 段）、#257（FS 段）、#258（SS 段），使其代码生成相对于对应段寄存器（非常底层的功能，仅适用于 OS 内核等场景）。GCC 兼容宏 `__seg_fs`/`__seg_gs` 等价，预定义宏 `__SEG_FS`/`__SEG_GS` 表示支持。

**示例**：

```c
#define GS_RELATIVE __attribute__((address_space(256)))
int foo(int GS_RELATIVE *P) {
  return *P;   // 编译为 movl %gs:(%eax), %eax
}
```

---

## 十五、其他通用内建

### `__builtin_readcyclecounter`

**功能**：访问周期计数器寄存器（或类似低延迟高精度的时钟）。返回 `unsigned long long`；计数器的全局/进程/线程特性取决于目标；底层计数器往往很快溢出（数秒量级），因此只应测量小的时间间隔。目标不支持时总是返回 0。即使内建存在，使用可能依赖运行时权限。

**示例**：

```c
unsigned long long t0 = __builtin_readcyclecounter();
do_something();
unsigned long long t1 = __builtin_readcyclecounter();
unsigned long long cycles_to_do_something = t1 - t0; // 假设没有溢出
```

### `__builtin_readsteadycounter`

**功能**：访问固定频率计数器寄存器（类似稳定频率时钟），与 `__builtin_readcyclecounter` 类似但频率固定，适合测量流逝时间。返回 `unsigned long long`；目标不支持时返回 0。不保证任何特定频率，只保证稳定，真实频率需用户自己提供。

**示例**：

```c
unsigned long long t0 = __builtin_readsteadycounter();
do_something();
unsigned long long t1 = __builtin_readsteadycounter();
unsigned long long secs_to_do_something = (t1 - t0) / tick_rate;
```

### `__builtin_cpu_supports`

**功能**：检测运行期 CPU 是否支持字符串参数指定的特性。全部支持返回正整数，否则返回 0。特性名与目标相关：AArch64 上用 `+` 组合，如 `__builtin_cpu_supports("flagm+sha3+lse+rcpc2+fcma+memtag+bti+sme2")`。不支持的特性名会告警并把内建替换为常量 0。

**示例**：

```c
if (__builtin_cpu_supports("sve"))
  sve_code();
```

### `__builtin_dump_struct`

**功能**：打印简单结构的字段及值，用于调试。第一个参数是指向完整记录类型的指针；第二个参数 `f` 是某个可调用表达式（函数对象或重载集）；内建会以"附加参数 + printf 兼容格式串 + 对应实参"的方式调用 `f`（可能多次调用）。Clang 能格式化的内建类型使用合适的格式符；`void*` 用 `%p`、`const char*` 用 `%s`；无法格式化的字段使用 `*%p` 说明符（对应参数为指向字段的指针），便于模板格式化函数实现自定义格式化。无返回值，可用于常量表达式。

**示例**：

```c
struct S {
  int x, y;
  float f;
  struct T { int i; } t;
};

void func(struct S *s) {
  __builtin_dump_struct(s, printf);
}
// 示例输出：
// struct S {
//   int x = 100
//   int y = 42
//   float f = 3.141593
//   struct T t = {
//     int i = 1997
//   }
// }
```

### `__builtin_sycl_unique_stable_name`

**功能**：接收一个类型，产生包含该类型唯一名称的字符串字面量，该名称在分裂编译（split compilation，如 offloading）之间保持稳定，主要支持 SYCL/Data Parallel C++。值完全在编译期计算，可用于常量表达式；编码基于 lambda 在局部声明上下文中的稳定编号顺序。当前实现使用 Itanium name mangling（即使宿主编译使用其他 mangling 方案）。

**语法**：

```cpp
// 为给定类型计算唯一稳定名称
constexpr const char * __builtin_sycl_unique_stable_name( type-id );
```

### `__builtin_available`

**功能**：`@available()` 的 C/C++ 拼写，用于在 C 和 C++ 代码中检查 API 可用性。`@available()` 仅限 Objective-C 使用。

**示例**：

```c
if (__builtin_available(macOS 10.12, iOS 10, *)) {
  use_new_api();
}
```

### `__builtin_complex`

**功能**：GCC 兼容内建，用给定的实部和虚部构造复数（如 `complex float z = __builtin_complex(1.0f, 2.0f);`）。

**示例**：

```c
complex float z = __builtin_complex(1.0f, 2.0f);  // 1 + 2i
```

---

## 十六、支持在常量表达式（constexpr）中使用的内建

以下内建内建（intrinsics）可以在常量表达式中使用：

**多精度**：`__builtin_addcb`、`__builtin_addcs`、`__builtin_addc`、`__builtin_addcl`、`__builtin_addcll`、`__builtin_subcb`、`__builtin_subcs`、`__builtin_subc`、`__builtin_subcl`、`__builtin_subcll`

**位操作**：`__builtin_bitreverse8/16/32/64`、`__builtin_bswap16/32/64`、`__builtin_rotateleft8/16/32/64`、`__builtin_rotateright8/16/32/64`

**计数类**：`__builtin_clrsb{,l,ll}`、`__builtin_clz{,l,ll,s}`、`__builtin_clzg`、`__builtin_ctz{,l,ll,s}`、`__builtin_ctzg`、`__builtin_ffs{,l,ll}`、`__builtin_parity{,l,ll}`、`__builtin_popcount{,l,ll}`、`__builtin_popcountg`

**浮点**：`__builtin_fmax`、`__builtin_fmin`、`__builtin_fpclassify`、`__builtin_inf`、`__builtin_isinf`、`__builtin_isinf_sign`、`__builtin_isfinite`、`__builtin_isnan`、`__builtin_isnormal`、`__builtin_nan`、`__builtin_nans`

**其他**：`__builtin_shufflevector`、`__builtin_convertvector`、`__builtin_dump_struct`、`__builtin_stdc_rotate_left/right`、`__builtin_stdc_*` 位工具、逐元素/归约/掩码向量内建（见各自章节）

---

*文档结束。原文地址：https://clang.llvm.org/docs/LanguageExtensions.html*
