# Dart 语言核心特性详解与多语言对比指南

## 前言：Dart 语言定位与学习优势

Dart 是由 Google 开发的通用编程语言，兼具静态类型语言的安全性和动态语言的灵活性。对于具有 Java、C#、C++ 和 Rust 基础的开发者而言，学习 Dart 可以通过语言特性的横向对比快速掌握核心概念。本指南将系统梳理 Dart 关键特性，通过与主流语言的多维度对比，帮助开发者建立知识迁移桥梁，同时标注易混淆点和最佳实践。

## 一、变量声明与类型系统

### 1.1 变量声明关键字对比

| 关键字 | Dart 特性 | 对应语言等效特性 | 核心差异 |
|--------|-----------|------------------|----------|
| `var` | 类型推断，运行时确定类型，变量可重新赋值 | C++ `auto`、Rust `let mut` | Dart 是动态类型推断，C++ 是编译时推断；Rust 默认不可变需显式 `mut` |
| `const` | 编译时常量，值必须在编译期确定 | C++ `constexpr`、Rust `const` | Dart `const` 要求所有嵌套值均为编译时常量 |
| `final` | 运行时常量，只能赋值一次 | C++ `const`、Rust `let` | 初始化时机不同：Dart `final` 可在运行时确定值 |

**代码示例对比：**

```dart
// Dart
var x = 42;        // 推断为 int，可重新赋值
x = 50;            // 合法
// x = "hello";    // 错误：类型不匹配

const y = 42;      // 编译时常量
// const z = DateTime.now(); // 错误：非编译时常量

final a = 42;      // 运行时常量
final b = DateTime.now(); // 合法：运行时确定
// a = 50;         // 错误：不可重新赋值
```

```cpp
// C++ 对应实现
auto x = 42;       // 编译时推断为 int
x = 50;            // 合法
// x = "hello";    // 错误

constexpr int y = 42; // 编译时常量
// constexpr auto z = time(nullptr); // 错误

const int a = 42;  // 编译时常量
const int b = getValue(); // 运行时确定（C++11 后）
// a = 50;         // 错误
```

```rust
// Rust 对应实现
let mut x = 42;    // 可变绑定，推断为 i32
x = 50;            // 合法
// x = "hello";    // 错误

const Y: i32 = 42; // 编译时常量

let a = 42;        // 不可变绑定
// a = 50;         // 错误
```

**易错点说明：**
- Dart 中 `const` 变量必须由编译时常量表达式初始化，调用返回动态值的函数（如 `DateTime.now()`）会导致编译错误
- `final` 变量在首次使用时初始化，而 `const` 变量在编译时就已确定值
- 与 Rust 不同，Dart 的 `var` 声明的变量默认是可变的，无需显式关键字

### 1.2 类型系统层次结构

Dart 采用单根继承体系，所有类型最终继承自 `Object`，形成清晰的类型层次：

```
Object (所有类的基类)
  │
  ├── dynamic (特殊类型：关闭静态检查)
  │
  ├── void (无返回类型)
  │
  └── 具体类型 (int, String, List, 等)
```

**类型特性对比：**

| 类型 | 特性 | 与其他语言对比 | 使用场景 |
|------|------|----------------|----------|
| `Object` | 所有类型的基类，仅能调用 `Object` 方法 | 类似 Java `Object`、C# `object` | 通用类型容器，需要显式转换才能使用具体类型方法 |
| `dynamic` | 禁用静态类型检查，允许调用任何方法 | 类似 C# `dynamic`、JavaScript 变量 | 与动态语言交互、处理 JSON 等无类型数据 |
| 具体类型 | 编译时类型检查，提供完整方法集 | 与强类型语言行为一致 | 大多数场景，确保类型安全 |

**代码示例：**

```dart
Object obj = "hello";      // 静态类型为 Object
dynamic dyn = "hello";     // 静态类型为 dynamic
var inferred = "hello";    // 推断为 String

// Object 类型限制
print(obj.toString());     // 合法：Object 有 toString 方法
// print(obj.length);      // 错误：Object 无 length 属性

// dynamic 类型特性
print(dyn.length);         // 编译通过，运行时正确
dyn = 42;
// print(dyn.length);      // 编译通过，运行时错误：int 无 length

// 类型推断优势
print(inferred.length);    // 编译和运行时都正确
// inferred = 42;          // 错误：类型不匹配
```

**最佳实践：**
- 优先使用类型推断（`var`）而非 `dynamic`，保留静态类型检查的安全性
- 仅在必要时使用 `dynamic`（如 JSON 解析），并配合运行时类型检查
- 对 `Object` 类型变量使用 `is` 关键字检查后再转换，避免运行时错误

## 二、数值类型系统

### 2.1 基础数值类型

Dart 提供统一的数值类型体系，与其他语言的对比关系如下：

| Dart 类型 | 长度 | 对应其他语言类型 | 特性 |
|-----------|------|------------------|------|
| `int` | 64 位 | C++ `int64_t`、C# `long`、Java `long`、Rust `i64` | 任意大小整数（超出 64 位自动转为 BigInt） |
| `double` | 64 位 | C++ `double`、C# `double`、Java `double`、Rust `f64` | 双精度浮点数 |
| `num` | - | 无直接对应 | `int` 和 `double` 的父类，统一处理数值类型 |

**代码示例：**

```dart
// 基础数值类型
int integer = 42;
double floating = 3.14;

// num 类型的灵活性
num a = 10;      // int 赋值给 num
num b = 3.14;    // double 赋值给 num

// 运算时类型提升
var sum = a + b; // 结果为 double (13.14)
```

**与其他语言的关键区别：**
- Dart 不区分 `short`/`int`/`long`（如 C#/Java）或 `i8`/`i16`/`i32`（如 Rust），统一用 `int` 表示整数
- 提供 `num` 作为数值类型的共同父类，便于编写通用数值算法
- 不支持隐式类型转换，必须显式调用转换方法

### 2.2 数值转换规则

Dart 对数值类型转换有严格限制，与其他语言对比：

| 转换方向 | Dart 语法 | C++ 行为 | C# 行为 | Rust 行为 |
|----------|-----------|----------|---------|-----------|
| int → double | `intValue.toDouble()` | 隐式转换 | 隐式转换 | 需显式 `as f64` |
| double → int | `doubleValue.toInt()` | 隐式截断 | 需显式转换 | 需显式 `as i64` 或 `round()` |

**代码示例：**

```dart
int intVal = 10;
double doubleVal = 3.14;

// 正确转换
double fromInt = intVal.toDouble();
int fromDouble = doubleVal.toInt(); // 截断为 3

// 错误示例
// double wrong1 = intVal;        // 错误：无隐式转换
// int wrong2 = doubleVal;        // 错误：无隐式转换
```

### 2.3 二进制数据处理

对于需要精确控制内存布局的场景，Dart 提供 `dart:typed_data` 库，与其他语言的对应关系：

| Dart 类型 | 对应 C++ 类型 | 对应 Rust 类型 | 用途 |
|-----------|---------------|----------------|------|
| `Int8List` | `int8_t[]` | `Vec<i8>` | 8 位有符号整数数组 |
| `Uint8List` | `uint8_t[]` | `Vec<u8>` | 8 位无符号整数数组 |
| `Int16List` | `int16_t[]` | `Vec<i16>` | 16 位有符号整数数组 |
| `Int32List` | `int32_t[]` | `Vec<i32>` | 32 位有符号整数数组 |
| `Int64List` | `int64_t[]` | `Vec<i64>` | 64 位有符号整数数组 |
| `Float32List` | `float[]` | `Vec<f32>` | 32 位浮点数数组 |
| `Float64List` | `double[]` | `Vec<f64>` | 64 位浮点数数组 |
| `ByteData` | `struct` + 指针操作 | `Vec<u8>` + 字节操作 | 随机访问的字节缓冲区 |

**代码示例：**

```dart
import 'dart:typed_data';

void main() {
  // 创建特定类型的数值数组
  Uint8List uint8List = Uint8List(10); // 8位无符号整数
  Int32List int32List = Int32List(10); // 32位有符号整数
  
  // 赋值操作
  uint8List[0] = 255; // 最大8位无符号值
  int32List[0] = 2147483647; // 最大32位有符号值
  
  // 字节数据随机访问
  ByteData byteData = ByteData(8);
  byteData.setInt32(0, 1000, Endian.little); // 偏移0处写入32位整数
  byteData.setFloat32(4, 3.14, Endian.little); // 偏移4处写入32位浮点数
}
```

**使用场景：**
- 网络协议解析
- 二进制文件处理
- 图形图像处理
- 与原生代码交互时的数据传递

## 三、空安全机制

Dart 2.12 引入的空安全机制是语言的重要特性，与 C#、Rust 的对比关系如下：

### 3.1 空安全操作符对比表

| 操作符 | Dart 语法 | C# 对应语法 | Rust 对应语法 | 功能描述 |
|--------|-----------|-------------|---------------|----------|
| 可空类型声明 | `Type?` | `Type?` | `Option<Type>` | 声明可能为空的变量 |
| 安全调用 | `?.` | `?.` | `and_then`/`map` 链式调用 | 为空时不执行方法调用 |
| 空合并 | `??` | `??` | `unwrap_or` | 为空时使用默认值 |
| 空合并赋值 | `??=` | `??=` | `get_or_insert` | 为空时赋值 |
| 非空断言 | `!` | `!` | `unwrap()` | 断言变量不为空 |
| 类型转换 | `as?` | `as?` | `downcast` 模式匹配 | 安全的类型转换 |

### 3.2 可空类型声明

**多语言对比示例：**

```dart
// Dart
String? name;      // 可空字符串
String name2;      // 非空字符串（必须初始化）

int? age = null;   // 明确赋值为 null
// int age2 = null; // 错误：非空类型不能为 null
```

```csharp
// C#
string? name;      // 可空字符串
string name2;      // 非空字符串（有警告）

int? age = null;   // 可空值类型
// int age2 = null; // 错误
```

```rust
// Rust
let name: Option<String> = None; // 可空字符串
let name2: String = "hello".to_string(); // 非空字符串

let age: Option<i32> = None; // 可空整数
let age2: i32 = 30; // 非空整数
```

**关键区别：**
- Dart/C# 使用 `?` 后缀标记可空类型，语法简洁
- Rust 使用 `Option<T>` 枚举类型，强制处理空值情况，类型安全性更高
- Dart 非空类型必须初始化，而 C# 允许未初始化的非空引用类型（有警告）

### 3.3 安全调用与空合并

**链式安全调用示例：**

```dart
// Dart
class User {
  Address? address;
}

class Address {
  String? city;
}

String getUserCity(User? user) {
  // 安全调用链 + 空合并
  return user?.address?.city ?? "未知城市";
}
```

```csharp
// C# 等效实现
class User {
    public Address? Address { get; set; }
}

class Address {
    public string? City { get; set; }
}

string GetUserCity(User? user) {
    return user?.Address?.City ?? "未知城市";
}
```

```rust
// Rust 等效实现
struct User {
    address: Option<Address>,
}

struct Address {
    city: Option<String>,
}

fn get_user_city(user: Option<User>) -> String {
    user
        .and_then(|u| u.address)  // 类似 ?. 操作符
        .and_then(|a| a.city)
        .unwrap_or_else(|| "未知城市".to_string())
}
```

### 3.4 非空断言的风险

非空断言（`!`）在各语言中的行为对比：

```dart
// Dart
String? getName() => Random().nextDouble() > 0.5 ? "Alice" : null;

void main() {
  String? name = getName();
  String sureName = name!;  // 50% 概率抛出运行时错误
  
  // 安全替代方案
  if (name != null) {
    String safeName = name;  // 类型提升，无需断言
  }
}
```

```csharp
// C#
string? GetName() => new Random().NextDouble() > 0.5 ? "Alice" : null;

void Main() {
  string? name = GetName();
  string sureName = name!;  // 50% 概率抛出 NullReferenceException
}
```

```rust
// Rust
fn get_name() -> Option<String> {
    if rand::random::<f64>() > 0.5 {
        Some("Alice".to_string())
    } else {
        None
    }
}

fn main() {
    let name = get_name();
    let sure_name = name.unwrap();  // 50% 概率 panic
    
    // 安全处理
    match name {
        Some(n) => println!("Name: {}", n),
        None => println!("No name"),
    }
}
```

**风险提示：**
- Dart/C# 的非空断言仅在编译时消除警告，运行时仍可能出错
- Rust 的 `unwrap()` 同样会 panic，但语言设计鼓励使用模式匹配处理空值
- 最佳实践：优先使用条件判断（类型提升）而非非空断言

### 3.5 空安全最佳实践

1. **利用类型提升**
```dart
void printLength(String? text) {
  if (text != null) {
    // text 自动提升为非空 String
    print(text.length);  // 无需使用 !
  }
}
```

2. **限制非空断言的使用范围**
```dart
String processName(String? name) {
  if (name == null) {
    throw ArgumentError("name 不能为空");
  }
  // 此处 name 已确定非空，可安全使用
  return name.toUpperCase();
}
```

3. **优先使用空合并提供默认值**
```dart
String getDisplayName(String? userName) {
  return userName ?? "匿名用户";
}
```

4. **集合空安全处理**
```dart
void processList(List<int>? numbers) {
  // 确保集合非空且创建副本避免外部修改
  List<int> safeNumbers = List.from(numbers ?? []);
  // 处理集合...
}
```

## 四、函数参数系统

Dart 的函数参数系统兼具灵活性和可读性，支持多种参数形式，与其他语言对比：

### 4.1 参数类型对比表

| 参数类型 | Dart 语法 | C++ 实现方式 | C# 语法 | 特点 |
|----------|-----------|--------------|---------|------|
| 必需位置参数 | `(param1, param2)` | 常规参数 | 常规参数 | 必须按顺序传递 |
| 可选位置参数 | `([param1, param2 = default])` | 默认参数（从右向左） | 可选参数（`param = default`） | 可省略，必须按顺序传递 |
| 可选命名参数 | `({param1, param2 = default})` | 结构体/Builder 模式模拟 | 命名参数（调用时 `param: value`） | 可省略，传递时需指定参数名 |
| 必需命名参数 | `({required param})` | 无直接对应 | 无直接对应（需特性实现） | 必须传递，可指定参数名 |

### 4.2 可选位置参数

```dart
// Dart 可选位置参数
void printMessage(String message, [String? prefix, String suffix = "!"]) {
  print('${prefix ?? ""}$message$suffix');
}

void main() {
  printMessage("Hello");                // "Hello!"
  printMessage("Hello", "Info: ");      // "Info: Hello!"
  printMessage("Hello", "Info: ", "?"); // "Info: Hello?"
}
```

```cpp
// C++ 等效实现（默认参数）
#include <iostream>
#include <string>

void printMessage(const std::string& message,
                  const std::string& prefix = "",
                  const std::string& suffix = "!") {
    std::cout << prefix << message << suffix << std::endl;
}

int main() {
    printMessage("Hello");
    printMessage("Hello", "Info: ");
    printMessage("Hello", "Info: ", "?");
    return 0;
}
```

```csharp
// C# 等效实现
using System;

class Program {
    static void PrintMessage(string message, 
                           string prefix = "", 
                           string suffix = "!") {
        Console.WriteLine($"{prefix}{message}{suffix}");
    }
    
    static void Main() {
        PrintMessage("Hello");
        PrintMessage("Hello", "Info: ");
        PrintMessage("Hello", "Info: ", "?");
    }
}
```

**关键区别：**
- Dart 和 C# 允许可选参数在任意位置，C++ 要求默认参数必须从右向左排列
- 调用时都必须按位置传递，无法跳过中间参数

### 4.3 可选命名参数

Dart 的命名参数是其特色特性，提供出色的可读性：

```dart
// Dart 命名参数
void configureApp({
  String theme = 'light',
  int fontSize = 14,
  bool darkMode = false,
  required String language, // 必需命名参数
}) {
  print('Theme: $theme, Font: $fontSize, Lang: $language');
}

void main() {
  configureApp(language: 'zh');  // 仅传递必需参数
  
  configureApp(
    language: 'en',
    theme: 'dark',
    fontSize: 16,
  );
  
  // 命名参数可乱序
  configureApp(
    fontSize: 18,
    theme: 'system',
    language: 'fr',
  );
}
```

```csharp
// C# 命名参数调用（无必需标记）
using System;

class Program {
    static void ConfigureApp(string language,
                           string theme = "light",
                           int fontSize = 14,
                           bool darkMode = false) {
        Console.WriteLine($"Theme: {theme}, Font: {fontSize}, Lang: {language}");
    }
    
    static void Main() {
        ConfigureApp("zh");
        
        ConfigureApp("en", "dark", 16);
        
        // C# 命名参数调用
        ConfigureApp(
            "fr", 
            fontSize: 18, 
            theme: "system"
        );
    }
}
```

```cpp
// C++ 模拟命名参数（使用结构体）
#include <iostream>
#include <string>

struct Config {
    std::string theme = "light";
    int fontSize = 14;
    bool darkMode = false;
};

void configureApp(const std::string& language, const Config& config = Config()) {
    std::cout << "Theme: " << config.theme 
              << ", Font: " << config.fontSize 
              << ", Lang: " << language << std::endl;
}

int main() {
    configureApp("zh");
    
    Config cfg;
    cfg.theme = "dark";
    cfg.fontSize = 16;
    configureApp("en", cfg);
    
    return 0;
}
```

**优势分析：**
- 命名参数提高代码可读性，尤其参数较多时
- 可选择性传递参数，无需记忆参数顺序
- `required` 关键字明确标记必须传递的参数，编译器强制检查

### 4.4 参数默认值限制

各语言对参数默认值的限制对比：

| 语言 | 默认值限制 | 示例 |
|------|------------|------|
| Dart | 可以是任意表达式，包括 `const` 构造函数调用 | `{Duration timeout = const Duration(seconds: 30)}` |
| C++ | 必须是编译时常量表达式 | `int timeout = 3000` |
| C# | 必须是编译时常量表达式 | `int timeout = 3000` |
| Rust | 函数参数不支持默认值，需通过重载或 `Option` 实现 | `fn func(timeout: Option<u64>) { ... }` |

**Dart 灵活默认值示例：**

```dart
class ApiClient {
  final String baseUrl;
  final Duration timeout;
  final Map<String, String> headers;
  
  // 复杂默认值示例
  ApiClient({
    required this.baseUrl,
    this.timeout = const Duration(seconds: 30),
    this.headers = const {
      'Content-Type': 'application/json',
      'Accept': 'application/json',
    },
  });
}
```

### 4.5 混合参数类型

Dart 允许在同一函数中混合使用位置参数和命名参数，需遵循位置参数在前的规则：

```dart
// 混合参数示例
void sendEmail(
  String to,                  // 必需位置参数
  String subject,             // 必需位置参数
  [String body = ''],         // 可选位置参数
  {
    String cc = '',           // 可选命名参数
    String bcc = '',          // 可选命名参数
    bool urgent = false,      // 可选命名参数
  }
) {
  print('To: $to, Subject: $subject');
  if (body.isNotEmpty) print('Body: $body');
  if (cc.isNotEmpty) print('CC: $cc');
  if (urgent) print('URGENT!');
}

void main() {
  sendEmail('alice@example.com', 'Meeting');
  
  sendEmail(
    'bob@example.com', 
    'Report', 
    'Please review',
    cc: 'manager@example.com',
    urgent: true
  );
}
```

**调用规则：**
1. 位置参数必须按顺序在命名参数之前传递
2. 可选位置参数不能在必需位置参数之后
3. 命名参数可以按任意顺序传递

## 五、容器与迭代器

Dart 的容器类型和迭代操作与 C++20 范围（ranges）有相似之处，均采用惰性计算策略：

### 5.1 容器类型对比

| Dart 容器 | 对应 C++ 容器 | 对应 C# 容器 | 对应 Rust 容器 | 特性 |
|-----------|---------------|--------------|----------------|------|
| `List` | `std::vector` | `List<T>` | `Vec<T>` | 动态数组 |
| `Set` | `std::unordered_set` | `HashSet<T>` | `HashSet<T>` | 无序不重复集合 |
| `Map` | `std::unordered_map` | `Dictionary<TKey, TValue>` | `HashMap<K, V>` | 键值对集合 |
| `Iterable` | `std::ranges::range` | `IEnumerable<T>` | `Iterator` | 惰性迭代序列 |

### 5.2 迭代器方法对比

Dart 的 `Iterable` 提供丰富的操作方法，与其他语言的算法对比：

| Dart 方法 | C++ 算法 | C# LINQ | Rust 迭代器方法 | 功能 |
|-----------|----------|---------|----------------|------|
| `forEach` | `std::for_each` | `ForEach` | `for_each` | 遍历元素 |
| `map` | `std::ranges::transform` | `Select` | `map` | 转换元素 |
| `where` | `std::ranges::filter` | `Where` | `filter` | 过滤元素 |
| `any` | `std::ranges::any_of` | `Any` | `any` | 存在满足条件的元素 |
| `every` | `std::ranges::all_of` | `All` | `all` | 所有元素满足条件 |
| `firstWhere` | `std::ranges::find_if` | `FirstOrDefault` | `find` | 查找第一个满足条件的元素 |
| `takeWhile` | 无直接对应 | `TakeWhile` | `take_while` | 取元素直到条件为假 |
| `skipWhile` | 无直接对应 | `SkipWhile` | `skip_while` | 跳过元素直到条件为假 |

**链式操作示例：**

```dart
// Dart 链式操作
void main() {
  List<int> numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
  
  var result = numbers
    .where((n) => n % 2 == 0)    // 过滤偶数
    .map((n) => n * 2)           // 翻倍
    .where((n) => n > 10)        // 过滤大于10的
    .toList();                   // 转为列表
  
  print(result); // [12, 14, 16, 18, 20]
}
```

```cpp
// C++20 范围操作
#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    auto even = numbers | std::views::filter([](int n) { return n % 2 == 0; });
    auto doubled = even | std::views::transform([](int n) { return n * 2; });
    auto filtered = doubled | std::views::filter([](int n) { return n > 10; });
    
    std::vector<int> result(filter.begin(), filter.end());
    
    for (int n : result) {
        std::cout << n << " "; // 12 14 16 18 20
    }
    return 0;
}
```

```csharp
// C# LINQ
using System;
using System.Linq;

class Program {
    static void Main() {
        int[] numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        
        var result = numbers
            .Where(n => n % 2 == 0)
            .Select(n => n * 2)
            .Where(n => n > 10)
            .ToList();
        
        Console.WriteLine(string.Join(" ", result)); // 12 14 16 18 20
    }
}
```

```rust
// Rust 迭代器
fn main() {
    let numbers = vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    
    let result: Vec<i32> = numbers
        .into_iter()
        .filter(|n| n % 2 == 0)
        .map(|n| n * 2)
        .filter(|n| n > &10)
        .collect();
    
    println!("{:?}", result); // [12, 14, 16, 18, 20]
}
```

**惰性计算说明：**
- Dart 的 `Iterable` 操作是惰性的，仅在需要结果时（如调用 `toList()`）才执行计算
- 这与 C++ 范围视图、C# LINQ 和 Rust 迭代器的行为一致
- 优势：避免中间集合的创建，提高性能
- 注意：多次迭代同一 `Iterable` 会导致多次计算，必要时使用 `toList()` 缓存结果

### 5.3 Dart 独有的容器方法

Dart 提供一些其他语言不常见的便捷方法：

1. `takeWhile` 与 `skipWhile`
```dart
void main() {
  List<int> numbers = [1, 2, 3, 4, 5, 2, 1];
  
  // 取元素直到条件为假
  var taken = numbers.takeWhile((n) => n < 4).toList();
  print(taken); // [1, 2, 3]
  
  // 跳过元素直到条件为假
  var skipped = numbers.skipWhile((n) => n < 4).toList();
  print(skipped); // [4, 5, 2, 1]
}
```

2. `expand` 方法（展平集合）
```dart
void main() {
  List<List<int>> nested = [[1, 2], [3, 4], [5, 6]];
  
  // 展平嵌套列表
  var flattened = nested.expand((list) => list).toList();
  print(flattened); // [1, 2, 3, 4, 5, 6]
  
  // 生成新序列
  var expanded = [1, 2, 3].expand((n) => [n, n*2]).toList();
  print(expanded); // [1, 2, 2, 4, 3, 6]
}
```

3. `followedBy` 方法（连接集合）
```dart
void main() {
  var first = [1, 2, 3];
  var second = [4, 5, 6];
  
  // 连接两个集合
  var combined = first.followedBy(second).toList();
  print(combined); // [1, 2, 3, 4, 5, 6]
}
```

## 六、动态类型与类型转换

Dart 提供多种处理动态类型的方式，各有适用场景：

### 6.1 动态类型对比

| 类型 | 特性 | 对应其他语言 | 适用场景 |
|------|------|--------------|----------|
| `dynamic` | 禁用静态检查，运行时解析方法 | C# `dynamic`、JavaScript 变量 | 与动态系统交互、JSON 处理 |
| `Object?` | 所有类型的基类，需显式转换 | C++ `std::any`、Java `Object` | 通用类型容器，保留类型检查 |
| 联合类型 | Dart 3.0+ 引入，有限的类型集合 | C++ `std::variant`、Rust 枚举 | 明确的多类型场景 |

### 6.2 `dynamic` vs `Object?`

```dart
void main() {
  // dynamic 类型
  dynamic dyn = "hello";
  print(dyn.length); // 编译通过，运行时正确
  dyn = 42;
  // print(dyn.length); // 编译通过，运行时错误
  
  // Object? 类型
  Object? obj = "hello";
  // print(obj.length); // 编译错误：Object 无 length 属性
  if (obj is String) {
    print(obj.length); // 类型检查后安全使用
  }
  
  obj = 42;
  if (obj is int) {
    print(obj + 10); // 安全使用 int 方法
  }
}
```

```cpp
// C++ std::any 对比
#include <any>
#include <string>
#include <iostream>

int main() {
    std::any any_val = std::string("hello");
    
    // 必须显式转换
    if (any_val.type() == typeid(std::string)) {
        auto str = std::any_cast<std::string>(any_val);
        std::cout << str.size() << std::endl;
    }
    
    any_val = 42;
    if (any_val.type() == typeid(int)) {
        auto num = std::any_cast<int>(any_val);
        std::cout << num + 10 << std::endl;
    }
    
    return 0;
}
```

```csharp
// C# dynamic vs object
using System;

class Program {
    static void Main() {
        // dynamic 类型
        dynamic dyn = "hello";
        Console.WriteLine(dyn.Length); // 正确
        dyn = 42;
        // Console.WriteLine(dyn.Length); // 运行时错误
        
        // object 类型
        object obj = "hello";
        // Console.WriteLine(obj.Length); // 编译错误
        if (obj is string str) {
            Console.WriteLine(str.Length); // 正确
        }
        
        obj = 42;
        if (obj is int num) {
            Console.WriteLine(num + 10); // 正确
        }
    }
}
```

**关键区别：**
- `dynamic` 关闭静态检查，编译时允许任何操作，运行时才验证
- `Object?` 保留静态检查，必须通过类型转换才能使用具体类型方法
- 性能：`Object?` 转换开销较低，`dynamic` 因动态分派开销较高

### 6.3 联合类型（Dart 3.0+）

Dart 3.0 引入的密封类（sealed class）和模式匹配提供了类型安全的联合类型：

```dart
// 联合类型模拟
sealed class Value {} // 密封类，限制继承范围
class IntValue extends Value { final int value; IntValue(this.value); }
class StringValue extends Value { final String value; StringValue(this.value); }
class ListValue extends Value { final List<int> value; ListValue(this.value); }

void processValue(Value value) {
  switch (value) {
    case IntValue(:final value):
      print('整数: $value');
    case StringValue(:final value):
      print('字符串: $value (长度: ${value.length})');
    case ListValue(:final value):
      print('列表: $value (大小: ${value.length})');
  }
}

void main() {
  processValue(IntValue(42));
  processValue(StringValue("hello"));
  processValue(ListValue([1, 2, 3]));
}
```

```cpp
// C++ std::variant 对应实现
#include <variant>
#include <string>
#include <vector>
#include <iostream>

int main() {
  using Value = std::variant<int, std::string, std::vector<int>>;
  
  auto processValue = [](const Value& value) {
    std::visit([](auto&& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, int>) {
        std::cout << "整数: " << arg << std::endl;
      } else if constexpr (std::is_same_v<T, std::string>) {
        std::cout << "字符串: " << arg << " (长度: " << arg.size() << ")" << std::endl;
      } else if constexpr (std::is_same_v<T, std::vector<int>>) {
        std::cout << "向量: [";
        for (size_t i = 0; i < arg.size(); ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << arg[i];
        }
        std::cout << "] (大小: " << arg.size() << ")" << std::endl;
      }
    }, value);
  };
  
  processValue(42);
  processValue(std::string("hello"));
  processValue(std::vector<int>{1, 2, 3});
  
  return 0;
}
```

```rust
// Rust 枚举对应实现
enum Value {
    Int(i32),
    String(String),
    List(Vec<i32>),
}

fn process_value(value: Value) {
    match value {
        Value::Int(val) => println!("整数: {}", val),
        Value::String(val) => println!("字符串: {} (长度: {})", val, val.len()),
        Value::List(val) => {
            print!("列表: [");
            for (i, num) in val.iter().enumerate() {
                if i > 0 { print!(", "); }
                print!("{}", num);
            }
            println!("] (大小: {})", val.len());
        }
    }
}

fn main() {
    process_value(Value::Int(42));
    process_value(Value::String("hello".to_string()));
    process_value(Value::List(vec![1, 2, 3]));
}
```

**优势分析：**
- 联合类型提供比 `dynamic` 更安全的多类型处理方式
- 编译器确保所有可能的类型都被处理（密封类 + switch 模式匹配）
- 相比 C++ `std::variant`，Dart 语法更简洁，模式匹配更直观

### 6.4 类型转换最佳实践

1. **安全类型转换**
```dart
void safeCast(Object? obj) {
  // 推荐：使用 is 检查后直接转换
  if (obj is String) {
    String str = obj; // 自动转换，无需 as
    print(str.length);
  }
  
  // 也可使用 as? 进行安全转换
  String? str = obj as? String;
  if (str != null) {
    print(str.length);
  }
}
```

2. **避免过度使用 `dynamic`**
```dart
// 不推荐
dynamic parseJson(dynamic json) {
  return json['name']; // 无类型检查
}

// 推荐
class User {
  final String name;
  User(this.name);
  
  // 类型安全的 JSON 解析
  static User? fromJson(Map<String, dynamic> json) {
    if (json.containsKey('name') && json['name'] is String) {
      return User(json['name']);
    }
    return null;
  }
}
```

3. **集合类型转换**
```dart
void castCollection(List<dynamic> dynamicList) {
  // 安全转换列表元素
  List<String> stringList = dynamicList
      .where((e) => e is String)
      .cast<String>()
      .toList();
}
```

## 七、综合场景示例

### 7.1 API 客户端设计

对比不同语言实现同一 API 客户端的方式，展示 Dart 特性优势：

```dart
// Dart 实现
import 'dart:convert';
import 'dart:io';

class ApiClient {
  final String baseUrl;
  final Duration timeout;
  final Map<String, String> headers;
  
  // 使用命名参数和默认值
  ApiClient({
    required this.baseUrl,
    this.timeout = const Duration(seconds: 30),
    this.headers = const {
      'Content-Type': 'application/json',
    },
  });
  
  // 通用 GET 请求方法
  Future<T?> get<T>(
    String path, {
    Map<String, dynamic>? queryParameters,
    required T Function(Map<String, dynamic>) parser,
  }) async {
    try {
      // 构建 URL
      final uri = Uri.parse('$baseUrl$path').replace(
        queryParameters: queryParameters?.map(
          (k, v) => MapEntry(k, v.toString()),
        ),
      );
      
      // 发送请求
      final response = await HttpClient()
          .getUrl(uri)
          .then((request) {
            // 添加 headers
            headers.forEach((k, v) => request.headers.add(k, v));
            return request.close();
          })
          .timeout(timeout);
      
      // 处理响应
      if (response.statusCode == HttpStatus.ok) {
        final json = jsonDecode(await response.transform(utf8.decoder).join());
        return parser(json as Map<String, dynamic>);
      }
      return null;
    } catch (_) {
      return null;
    }
  }
}

// 使用示例
class User {
  final int id;
  final String name;
  
  User({required this.id, required this.name});
  
  static User fromJson(Map<String, dynamic> json) {
    return User(
      id: json['id'] as int,
      name: json['name'] as String,
    );
  }
}

void main() async {
  final client = ApiClient(
    baseUrl: 'https://api.example.com',
    timeout: Duration(seconds: 60),
  );
  
  final user = await client.get(
    '/users/1',
    parser: User.fromJson,
  );
  
  print(user?.name);
}
```

### 7.2 数据处理管道

展示 Dart 容器操作的简洁性：

```dart
// 数据处理示例
class Product {
  final String name;
  final double price;
  final List<String> categories;
  final bool inStock;
  
  Product({
    required this.name,
    required this.price,
    required this.categories,
    required this.inStock,
  });
}

void main() {
  // 模拟产品数据
  final products = [
    Product(name: 'Laptop', price: 999.99, categories: ['electronics'], inStock: true),
    Product(name: 'Mouse', price: 25.50, categories: ['electronics', 'accessories'], inStock: true),
    Product(name: 'Keyboard', price: 49.99, categories: ['electronics', 'accessories'], inStock: false),
    Product(name: 'Desk', price: 199.99, categories: ['furniture'], inStock: true),
  ];
  
  // 数据处理管道
  final result = products
    // 过滤有库存的电子产品
    .where((p) => p.inStock && p.categories.contains('electronics'))
    // 应用 10% 折扣
    .map((p) => Product(
      name: p.name,
      price: p.price * 0.9,
      categories: p.categories,
      inStock: p.inStock,
    ))
    // 按价格排序
    .toList()
    ..sort((a, b) => a.price.compareTo(b.price))
    // 提取名称和折扣价
    .map((p) => '${p.name}: \$${p.price.toStringAsFixed(2)}')
    .toList();
  
  // 输出结果
  result.forEach(print);
  // 输出:
  // Mouse: $22.95
  // Keyboard: $44.99 (注意：原数据中 Keyboard 无库存，所以不会出现)
  // Laptop: $899.99
}
```

## 八、常见错误与陷阱

### 8.1 变量声明错误

1. **混淆 `const` 和 `final`**
```dart
// 错误示例
final currentTime = DateTime.now(); // 正确：运行时确定
// const currentTime = DateTime.now(); // 错误：不是编译时常量

// 正确使用
class Constants {
  static const apiUrl = 'https://api.example.com'; // 编译时常量
  static final appVersion = '1.0.0'; // 可在运行时修改（但不推荐）
}
```

2. **错误的可空类型处理**
```dart
// 错误示例
String? getName() => null;

void main() {
  String name = getName()!; // 运行时错误：空值断言失败
  
  // 正确处理
  String? nullableName = getName();
  String safeName = nullableName ?? "默认名称";
}
```

### 8.2 类型转换错误

1. **数值类型转换**
```dart
// 错误示例
int intVal = 10;
double doubleVal = 3.14;

// doubleVal = intVal; // 错误：无隐式转换
// intVal = doubleVal; // 错误：无隐式转换

// 正确转换
doubleVal = intVal.toDouble();
intVal = doubleVal.toInt(); // 截断小数部分
```

2. **集合类型转换**
```dart
// 错误示例
List<dynamic> dynamicList = [1, 2, 3];
// List<int> intList = dynamicList; // 错误：类型不匹配

// 正确转换
List<int> intList = dynamicList.whereType<int>().toList();
```

### 8.3 函数参数错误

1. **可选参数顺序错误**
```dart
// 错误示例
void greet(String name, [int age, String title = "Mr."]) {
  // 错误：带默认值的可选参数必须放在最后
}

// 正确定义
void greet(String name, [String title = "Mr.", int? age]) {
  // 正确：无默认值的可选参数放在最后
}
```

2. **命名参数与位置参数混淆**
```dart
void configure({String theme = "light", int fontSize = 14}) {}

void main() {
  // 错误调用
  // configure("dark", 16); // 错误：命名参数必须带参数名
  
  // 正确调用
  configure(theme: "dark", fontSize: 16);
}
```

### 8.4 迭代器使用错误

1. **多次迭代惰性序列**
```dart
void main() {
  List<int> numbers = [1, 2, 3, 4, 5];
  
  // 惰性序列
  var evenNumbers = numbers.where((n) => n % 2 == 0);
  
  // 错误：多次迭代导致多次计算
  print(evenNumbers.length); // 2
  print(evenNumbers.toList()); // [2, 4]
  
  // 正确：缓存结果
  var evenList = evenNumbers.toList();
  print(evenList.length); // 2
  print(evenList); // [2, 4]
}
```

2. **修改正在迭代的集合**
```dart
void main() {
  List<int> numbers = [1, 2, 3, 4, 5];
  
  // 错误：迭代时修改集合
  for (var n in numbers) {
    if (n % 2 == 0) {
      // numbers.remove(n); // 导致未定义行为
    }
  }
  
  // 正确：迭代副本，修改原集合
  for (var n in List.from(numbers)) {
    if (n % 2 == 0) {
      numbers.remove(n);
    }
  }
}
```

## 九、总结与迁移建议

### 9.1 语言特性横向对比

| 特性领域 | Dart 特点 | 与其他语言的关键区别 |
|----------|-----------|----------------------|
| 类型系统 | 强类型，支持类型推断和动态类型 | 比 C++/Rust 更灵活，比 JavaScript 更安全 |
| 空安全 | `?` 标记可空类型，`?.` 安全调用 | 语法类似 C#，安全性介于 C# 和 Rust 之间 |
| 函数参数 | 支持位置参数、命名参数和默认值 | 命名参数语法比 C# 更清晰，比 C++ 更灵活 |
| 容器操作 | 惰性迭代器，丰富的链式操作 | 类似 C++20 ranges 和 C# LINQ，语法更简洁 |
| 动态类型 | `dynamic` 类型和模式匹配 | 比 C++ `std::any` 更易用，比 JavaScript 更安全 |

### 9.2 从其他语言迁移的注意事项

1. **从 C# 迁移**
   - 优势：语法相似，空安全操作符几乎相同
   - 注意：Dart 的 `var` 默认可变，无需 `mut` 关键字；命名参数使用 `{}` 而非调用时指定

2. **从 C++ 迁移**
   - 优势：类型系统概念相似，容器操作类似 C++20 ranges
   - 注意：Dart 无指针操作；内存自动管理；函数参数系统更灵活

3. **从 Rust 迁移**
   - 优势：迭代器模式相似，模式匹配功能强大
   - 注意：Dart 无所有权系统；空安全不如 Rust 严格；`!` 比 `unwrap()` 更危险

4. **从 Java 迁移**
   - 优势：类和继承概念相似
   - 注意：Dart 支持函数作为一等公民；异步编程模型不同；空安全更完善

### 9.3 最佳实践清单

1. **类型使用**
   - 优先使用 `var` 进行类型推断，提高代码简洁性
   - 限制 `dynamic` 使用，优先使用具体类型或 `Object?`
   - 利用 Dart 3.0+ 的模式匹配处理联合类型场景

2. **空安全**
   - 充分利用 `?.` 和 `??` 减少空值检查代码
   - 避免滥用 `!` 非空断言，优先使用类型提升
   - 为可空变量提供合理默认值

3. **函数设计**
   - 参数较多时优先使用命名参数提高可读性
   - 用 `required` 标记必须传递的命名参数
   - 合理使用默认值减少函数重载

4. **容器操作**
   - 利用链式操作简化集合处理逻辑
   - 注意惰性迭代器的特性，必要时缓存结果
   - 优先使用不可变集合（`const` 构造函数）提高性能

通过本指南的系统梳理，开发者可以充分利用已有编程语言知识，快速掌握 Dart 核心特性，并理解其与其他语言的异同点，从而高效编写安全、简洁的 Dart 代码。