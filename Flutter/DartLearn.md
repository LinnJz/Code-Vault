# Dart

## var

Dart 的 `var` 相当于 C++ `auto` 和 Rust `let`

- Dart `var`：类型推断，运行时确定类型
- C++ `auto`：编译时类型推断
- Rust `let`：类型推断，默认不可变（需要 `mut` 才可变）

**示例对比：**

```dart
// Dart
var x = 42;           // 类型推断为 int，可以重新赋值
x = 50;              // 允许
// x = "hello";      // 错误：类型不匹配
```



```cpp
// C++
auto x = 42;         // 编译时推断为 int
x = 50;              // 允许
// x = "hello";      // 错误：类型不匹配
```



```rust
// Rust
let x = 42;          // 类型推断为 i32，不可变
// x = 50;           // 错误：默认不可变
let mut y = 42;      // 可变
y = 50;              // 允许
```

## const

Dart `const` 不完全等于 C++ `constexpr`，但是类似

* `constexpr`声明的变量一定是编译期间求值，如果声明`constexpr`函数赋值给一个非`constexpr`的变量，则不是编译时常量
* 应该和`consteval`相等总是编译器求值

**更准确的对应是：**

- Dart `const` = **C++ `constexpr`（编译时常量）**
- Dart `const` = **Rust `const`（编译时常量）**



Dart 的 `const` 表示**编译时常量**，值在编译时就必须确定：

```dart
const x = 42;                      // 正确
const y = DateTime.now().second;   // 错误！不是编译时常量
```



C++ 中对应的是 `constexpr`：

```cpp
constexpr int x = 42;              // 编译时常量
// constexpr int y = time(nullptr); // 错误：不是编译时常量
```

## final

Dart `final` 不完全等于 C++ `const`

**更准确的对应是：**

- Dart `final` = **C++ `const`（运行时常量，单次赋值）**
- Dart `final` = **Rust `let`（单次绑定，默认不可变）**



Dart 的 `final` 表示**运行时常量**，只能赋值一次：

```dart
final x = 42;                     // 正确
final y = DateTime.now().second;  // 正确！运行时确定
// x = 50;                        // 错误：final 不能重新赋值
```



C++ 中对应的是 `const`：

```cpp
const int x = 42;                 // 编译时常量
const int y = getValue();         // 运行时确定，但不能修改
```

## num

Dart 的 `num` 类型是为了解决一个特定的问题而设计的：**统一处理整数和浮点数的数值运算**。这在 Dart 的设计中有几个重要的原因：

### 1. **与 JavaScript 的互操作性**

Dart 最初是为了替代 JavaScript 而设计的，需要与 JavaScript 的数值系统兼容。JavaScript 只有一种数值类型 `Number`（双精度浮点数），但 Dart 想提供更好的性能，所以区分了 `int` 和 `double`，同时用 `num` 作为它们的父类。

```dart
// 可以接受 int 或 double
num x = 10;     // int
num y = 3.14;   // double
```

### 2. **灵活的数值运算**

`num` 类型允许编写通用的数值算法，这些算法可以处理整数和浮点数，而不需要重载或泛型约束。

```dart
// 通用函数，可处理 int 或 double
T sum<T extends num>(List<T> numbers) {
  T total = numbers[0];
  for (int i = 1; i < numbers.length; i++) {
    total = (total + numbers[i]) as T;  // 注意：需要类型转换
  }
  return total;
}

// 都可以工作
print(sum<int>([1, 2, 3]));        // 6
print(sum<double>([1.5, 2.5]));    // 4.0
```

### 3. **避免自动类型转换的歧义**

有些语言（如 JavaScript、PHP）会自动进行隐式类型转换，这可能导致意外的行为。Dart 通过 `num` 类型提供了明确的转换路径：

```dart
num a = 10;      // int
num b = 3.14;    // double

// 错误不能，这样由父类转到子类，需调用内置转换方法
int intValue = b;
double doubleValue = a;

// 明确的转换方法
int intValue = a.toInt();
double doubleValue = a.toDouble();

int int_val = 10;
double double_val = 3.14;

int_val = double_val; // 错误，不允许缩窄转换，需要调用toInt
double_val = int_val; // 错误，不允许宽化转换，需要调用toDouble

// 运算时类型会提升
var result = a + b;  // result 是 double（13.14）
```

## int（64位）/double（64位）

### **与 C++/Rust 等语言的对比**

```dart
// Dart - 简单直接
int counter = 0;
double price = 9.99;

// C++ - 需要选择
int counter = 0;          // 通常 32 位
int64_t bigCounter = 0;   // 明确 64 位
uint32_t flags = 0;       // 无符号 32 位
unsigned char byte = 0xFF; // 字节数据

// Rust - 更明确
let counter: i32 = 0;     // 32 位有符号
let big_counter: i64 = 0; // 64 位有符号
let flags: u32 = 0;       // 32 位无符号
let byte: u8 = 0xFF;      // 8 位无符号
```

**表格**

| 语言     | 整数类型             | 浮点类型           | 统一数值类型       |
| -------- | -------------------- | ------------------ | ------------------ |
| **Dart** | `int`                | `double`           | `num`（父类）      |
| **C++**  | `int`, `int32_t`, 等 | `float`, `double`  | 无（需要模板）     |
| **Rust** | `i32`, `i64`, 等     | `f32`, `f64`       | 无（需要泛型）     |
| **Java** | `int`, `Integer`     | `double`, `Double` | `Number`（抽象类） |
| **C#**   | `int`                | `double`           | 无（但有数值转换） |

### **二进制数据处理（`dart:typed_data`）**

```
import 'dart:typed_data';

void main() {
  // 创建特定大小的整数
  Int8List int8List = Int8List(10);    // 8 位有符号整数
  Uint8List uint8List = Uint8List(10); // 8 位无符号整数
  Int16List int16List = Int16List(10); // 16 位有符号整数
  Int32List int32List = Int32List(10); // 32 位有符号整数
  Int64List int64List = Int64List(10); // 64 位有符号整数
  Float32List float32List = Float32List(10); // 32 位浮点数
  Float64List float64List = Float64List(10); // 64 位浮点数（= double）
  
  // 读写数据
  uint8List[0] = 255;      // 最大 8 位无符号值
  int32List[0] = 2147483647; // 最大 32 位有符号值
  
  // 字节数据操作
  ByteData byteData = ByteData(8);
  byteData.setInt32(0, 1000, Endian.little);
  byteData.setFloat32(4, 3.14, Endian.little);
}
```

## 容器函数

dart的 Iterable 是惰性的，和C++的ranges类似

| Dart 方法    | 最接近的 C++ 算法 | 功能描述                 | 关键区别     |
| ------------ | ----------------- | ------------------------ | ------------ |
| `forEach`    | `std::for_each`   | 遍历每个元素             | 几乎相同     |
| `every`      | `std::all_of`     | 检查所有元素是否满足条件 | 几乎相同     |
| `where`      | `std::copy_if`    | 过滤满足条件的元素       | 返回类型不同 |
| `any`        | `std::any_of`     | 检查是否有元素满足条件   | 几乎相同     |
| `firstWhere` | `std::find_if`    | 查找第一个满足条件的元素 | 几乎相同     |
| `map`        | `std::transform`  | 将序列元素进行转换       | 几乎相同     |



```C++
// Dart 独有的方便方法
List<int> numbers = [1, 2, 3, 4, 5, 2, 1];

// takeWhile - 取元素直到条件为假
var taken = numbers.takeWhile((n) => n < 4).toList();
print(taken); // [1, 2, 3]

// skipWhile - 跳过元素直到条件为假
var skipped = numbers.skipWhile((n) => n < 4).toList();
print(skipped); // [4, 5, 2, 1]

// Dart - 流畅的链式调用
var result = numbers
  .where((n) => n % 2 == 0)
  .map((n) => n * 2)
  .where((n) => n > 5)
  .toList();
print(result); // [8, 12] (如果 numbers = [1, 2, 3, 4, 5, 6])

// C++ - 需要中间变量或使用 ranges (C++20)
#include <ranges>
namespace views = std::views;

auto result = numbers 
  | views::filter([](int n) { return n % 2 == 0; })
  | views::transform([](int n) { return n * 2; })
  | views::filter([](int n) { return n > 5; });
// 需要再复制到 vector
```

## dynamic 

相似点：都可以持有任意类型的值

不同点

### 1. **类型检查时机**

- **Dart `dynamic`**：在运行时进行类型检查。你可以调用任何方法，但如果运行时类型不支持，会抛出异常。
- **C++ `std::any`**：在编译时不知道类型，但你必须通过`std::any_cast`来获取值，如果类型不匹配，会抛出`std::bad_any_cast`异常。

**Dart:**

```dart
dynamic obj = 'hello';
print(obj.length); // 5，因为String有length属性
obj = 42;
print(obj.length); // 运行时错误：int没有length属性
```



**C++:**

```cpp
std::any obj = std::string("hello");
std::cout << std::any_cast<std::string>(obj).length() << std::endl; // 5
obj = 42;
// std::cout << std::any_cast<std::string>(obj).length() << std::endl; // 抛出std::bad_any_cast
```



### 2. **性能**

- **Dart `dynamic`**：由于是动态类型，每次调用都可能涉及运行时查找，有一定开销。
- **C++ `std::any`**：使用类型擦除，通常实现为小对象优化，但`std::any_cast`和类型检查也有开销。

### 3. **使用模式**

- **Dart `dynamic`**：通常用于与JavaScript互操作、处理JSON、或避免复杂的类型声明。
- **C++ `std::any`**：通常用于需要存储任意类型但又不使用模板的情况，比如在容器中存储不同类型。

### 更接近的类比：C++`std::any` vs Dart `Object?`

实际上，Dart中所有类型的基类是`Object`，所以`Object?`（可空Object）也可以持有任意类型，但你需要进行显式类型转换。而`dynamic`是特殊的，它禁用静态类型检查。

实际上，Dart中所有类型的基类是`Object`，所以`Object?`（可空Object）也可以持有任意类型，但你需要进行显式类型转换。而`dynamic`是特殊的，它禁用静态类型检查。

**Dart:**

```dart
Object? obj = 42;
print(obj as int); // 需要显式转换

dynamic dyn = 42;
print(dyn); // 不需要转换，但运行时检查
```



**C++:**

```cpp
std::any value = 42;
int i = std::any_cast<int>(value); // 需要显式转换
```



### 总结

| 特性             | Dart `dynamic`    | C++ `std::any`    | Dart `Object?`   |
| ---------------- | ----------------- | ----------------- | ---------------- |
| 可以存储任意类型 | ✅                 | ✅                 | ✅                |
| 需要显式转换     | ❌（但运行时检查） | ✅                 | ✅                |
| 编译时类型检查   | ❌                 | ❌（但转换时检查） | ✅                |
| 运行时类型检查   | ✅                 | ✅（通过any_cast） | ✅（通过as）      |
| 性能开销         | 较高（动态查找）  | 中等（类型擦除）  | 低（但需要转换） |

## Dart 的类型系统层次

```text
Object (所有类的基类)
  │
  ├── dynamic (特殊类型：关闭静态检查)
  │
  ├── void (无返回类型)
  │
  └── 具体类型 (int, String, List, 等)
```



```dart
// 各种类型的比较
Object obj = "hello";      // 静态类型 Object
dynamic dyn = "hello";     // 静态类型 dynamic
var inferred = "hello";    // 静态类型 String (推断)

// 1. Object - 只能调用 Object 的方法
print(obj.toString());     // OK
// print(obj.length);      // 编译错误：Object 没有 length

// 2. dynamic - 可以调用任何方法，运行时检查
print(dyn.length);         // 编译通过，运行时 OK
dyn = 42;
// print(dyn.length);      // 编译通过，运行时错误

// 3. 类型推断 - 最安全
print(inferred.length);    // 编译通过，运行时 OK
// inferred = 42;          // 编译错误：类型不匹配
```

##  Dart 的联合类型 和C++对比

**`std::variant` 示例 (更像 Dart 的联合类型)：**

```cpp
#include <variant>
#include <string>
#include <vector>
#include <iostream>

int main() {
  // variant 类似于 Dart 的有限类型集合
  std::variant<int, std::string, std::vector<int>> value;
  
  value = 42;
  value = "hello";
  value = std::vector<int>{1, 2, 3};
  
  // 访问时需要知道可能的类型
  std::visit([](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, int>) {
      std::cout << "整数: " << arg << std::endl;
    } else if constexpr (std::is_same_v<T, std::string>) {
      std::cout << "字符串: " << arg << std::endl;
    } else if constexpr (std::is_same_v<T, std::vector<int>>) {
      std::cout << "向量大小: " << arg.size() << std::endl;
    }
  }, value);
  
  return 0;
}
```



**对应的 Dart 代码 (更简单)：**

```dart
// Dart 没有直接等价 variant，但可以这样模拟
dynamic value = 42;
value = "hello";
value = [1, 2, 3];

// 或者使用联合类型（Dart 3 引入）
sealed class Value {}
class IntValue extends Value { final int value; IntValue(this.value); }
class StringValue extends Value { final String value; StringValue(this.value); }
class ListValue extends Value { final List<int> value; ListValue(this.value); }

Value val = IntValue(42);
val = StringValue("hello");
val = ListValue([1, 2, 3]);

// 使用模式匹配
switch (val) {
  case IntValue(:final value):
    print('整数: $value');
  case StringValue(:final value):
    print('字符串: $value');
  case ListValue(:final value):
    print('列表大小: ${value.length}');
}
```

# ? ?.  ?? !. 和Rust与C#对比

与 C# 和 Rust 的空值处理有很多相似之处

这是一个非常好的问题！Dart 的空安全操作符确实与 C# 和 Rust 的空值处理有很多相似之处，但也有一些重要区别。

## 快速对比表

| 操作符   | Dart    | C#      | Rust           | 功能描述           |
| -------- | ------- | ------- | -------------- | ------------------ |
| 可空类型 | `Type?` | `Type?` | `Option<Type>` | 声明可能为空的类型 |
| 安全调用 | `?.`    | `?.`    | 无直接等价     | 为空时不调用       |
| 空合并   | `??`    | `??`    | `.unwrap_or()` | 为空时提供默认值   |
| 非空断言 | `!`     | `!`     | `.unwrap()`    | 断言不为空         |
| 强制转换 | `as?`   | `as?`   | 无直接等价     | 安全类型转换       |

## 详细对比

### 1. **可空类型声明**

**Dart:**
```dart
String? name;      // 可能为空的字符串
String name2;      // 非空字符串（Dart 2.12+）

int? age = null;   // 明确可空
int age2 = null;   // 编译错误：不能为 null
```

**C# (8.0+):**
```csharp
string? name;      // 可能为空的字符串
string name2;      // 非空字符串（有警告）

int? age = null;   // 可空值类型
int age2 = null;   // 编译错误
```

**Rust:**
```rust
let name: Option<String> = None;    // 可能为空的字符串
let name2: String = "hello".to_string();  // 非空字符串

let age: Option<i32> = None;        // 可能为空的整数
let age2: i32 = 30;                 // 非空整数
```

**关键区别：**
- Dart/C#：使用 `?` 后缀语法
- Rust：使用 `Option<T>` 枚举类型，更类型安全

### 2. **安全调用操作符 (`?.`)**

**Dart:**
```dart
class User {
  String? name;
  Address? address;
}

class Address {
  String? city;
}

void main() {
  User? user;
  
  // 安全调用链
  String? city = user?.address?.city;
  print(city); // null，不会崩溃
  
  // 对比普通调用
  // String city2 = user.address.city; // 运行时错误！
  
  // 结合方法调用
  int? length = user?.name?.length;
}
```

**C#:**
```csharp
class User {
    public string? Name { get; set; }
    public Address? Address { get; set; }
}

class Address {
    public string? City { get; set; }
}

public static void Main() {
    User? user = null;
    
    // 安全调用链
    string? city = user?.Address?.City;
    Console.WriteLine(city); // null，不会崩溃
    
    // 结合方法调用
    int? length = user?.Name?.Length;
}
```

**Rust:** 没有直接等价的操作符，但可以使用模式匹配或 `Option` 的方法链

```rust
struct User {
    name: Option<String>,
    address: Option<Address>,
}

struct Address {
    city: Option<String>,
}

fn main() {
    let user: Option<User> = None;
    
    // 使用 map 链（类似安全调用）
    let city: Option<String> = user
        .and_then(|u| u.address)   // 类似 ?.
        .and_then(|a| a.city);
    
    println!("{:?}", city); // None
    
    // 使用 ? 操作符（在返回 Option 的函数中）
    fn get_city(user: Option<User>) -> Option<String> {
        user?.address?.city  // Rust 的 ? 用于 Option/Result
    }
}
```

### 3. **空合并操作符 (`??`)**

**Dart:**
```dart
void main() {
  String? name;
  
  // 空合并
  String displayName = name ?? "匿名用户";
  print(displayName); // "匿名用户"
  
  // 赋值简写
  name ??= "默认名称";
  
  // 链式使用
  String? firstName;
  String? lastName;
  String fullName = firstName ?? lastName ?? "无名氏";
}
```

**C#:**
```csharp
public static void Main() {
    string? name = null;
    
    // 空合并
    string displayName = name ?? "匿名用户";
    Console.WriteLine(displayName); // "匿名用户"
    
    // C# 8.0+ 的空合并赋值
    name ??= "默认名称";
    
    // 空合并链
    string? firstName = null;
    string? lastName = null;
    string fullName = firstName ?? lastName ?? "无名氏";
}
```

**Rust:** 使用 `Option` 的方法

```rust
fn main() {
    let name: Option<String> = None;
    
    // unwrap_or - 类似 ??
    let display_name = name.unwrap_or("匿名用户".to_string());
    println!("{}", display_name); // "匿名用户"
    
    // or - 提供另一个 Option
    let name2: Option<String> = None;
    let result = name.or(Some("默认名称".to_string()));
    
    // unwrap_or_else - 延迟计算默认值
    let name3: Option<String> = None;
    let display_name2 = name3.unwrap_or_else(|| {
        println!("计算默认值");
        "计算出的默认值".to_string()
    });
}
```

### 4. **非空断言操作符 (`!`)**

**Dart:**
```dart
void main() {
  String? name = getName();
  
  // 断言不为空
  String sureName = name!;  // 如果为 null 会抛出异常
  print(sureName.length);
  
  // 方法调用
  name!.toUpperCase();
  
  // 危险的使用
  String? nullable = null;
  // String notNull = nullable!; // 运行时错误: Null check error
}

String? getName() => Math.random() > 0.5 ? "Alice" : null;
```

**C#:**
```csharp
public static void Main() {
    string? name = GetName();
    
    // 非空断言
    string sureName = name!;  // 告诉编译器"我知道这不是null"
    Console.WriteLine(sureName.Length);
    
    // 危险：实际上可能是null
    string? nullable = null;
    // string notNull = nullable!; // 编译警告，但可以通过
    // Console.WriteLine(notNull.Length); // 运行时NullReferenceException
}

static string? GetName() => new Random().NextDouble() > 0.5 ? "Alice" : null;
```

**Rust:** 使用 `unwrap()` 或 `expect()`

```rust
fn main() {
    let name: Option<String> = get_name();
    
    // unwrap - 类似 !，None时panic
    let sure_name = name.unwrap();  // 如果为None会panic
    println!("{}", sure_name.len());
    
    // expect - 带错误消息的unwrap
    let sure_name2 = name.expect("name应该是Some");
    
    // 更安全的方式：匹配处理
    match name {
        Some(n) => println!("Name: {}", n),
        None => println!("No name provided"),
    }
    
    // if let 语法
    if let Some(n) = name {
        println!("Name: {}", n);
    }
}

fn get_name() -> Option<String> {
    if rand::random::<f64>() > 0.5 {
        Some("Alice".to_string())
    } else {
        None
    }
}
```

### 5. **空安全转换 (`as?`)**

**Dart:**
```dart
void main() {
  Object obj = "hello";
  
  // 安全转换
  String? str = obj as String?;  // 成功
  print(str); // "hello"
  
  Object obj2 = 42;
  String? str2 = obj2 as String?;  // 运行时错误
  // String? str3 = obj2 as? String; // 正确写法，但Dart没有as?
  
  // 实际使用 try-cast
  if (obj2 is String) {
    String str3 = obj2 as String; // 安全，因为已经检查过
  }
}
```

**C#:**
```csharp
public static void Main() {
    object obj = "hello";
    
    // 安全转换
    string? str = obj as string;  // as 操作符返回 null 如果失败
    Console.WriteLine(str); // "hello"
    
    object obj2 = 42;
    string? str2 = obj2 as string;  // 返回 null，不抛异常
    Console.WriteLine(str2); // null
    
    // 模式匹配 (C# 7+)
    if (obj2 is string str3) {
        Console.WriteLine(str3); // 不会执行
    }
}
```

**Rust:** 使用模式匹配或 `downcast`

```rust
use std::any::Any;

fn main() {
    let obj: Box<dyn Any> = Box::new("hello".to_string());
    
    // 向下转换
    if let Some(str) = obj.downcast_ref::<String>() {
        println!("String: {}", str);
    }
    
    let obj2: Box<dyn Any> = Box::new(42);
    
    if let Some(str) = obj2.downcast_ref::<String>() {
        println!("String: {}", str); // 不会执行
    } else if let Some(num) = obj2.downcast_ref::<i32>() {
        println!("i32: {}", num); // 执行这个
    }
}
```

## 三语言对比总结

### 设计哲学差异：

1. **Dart**：提供简洁的操作符语法，平衡安全性和便利性
2. **C#**：与 Dart 最相似，语法几乎相同，但更早引入
3. **Rust**：最严格，使用类型系统（`Option`）强制处理空值

### 安全性等级：

```rust
// Rust - 最安全（编译时强制处理）
let name: Option<String> = get_name();
// 必须处理 None 情况
match name {
    Some(n) => use_name(n),
    None => handle_no_name(),
}

// Dart/C# - 编译时警告，运行时可能出错
String? name = getName();
String usedName = name!;  // 可能运行时崩溃
```

### 实际代码对比

**场景：获取用户所在城市，提供默认值**

```dart
// Dart
class User {
  Address? address;
}

class Address {
  String? city;
}

String getUserCity(User? user) {
  return user?.address?.city ?? "未知城市";
}
```

```csharp
// C#
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
// Rust
struct User {
    address: Option<Address>,
}

struct Address {
    city: Option<String>,
}

fn get_user_city(user: Option<User>) -> String {
    user
        .and_then(|u| u.address)
        .and_then(|a| a.city)
        .unwrap_or_else(|| "未知城市".to_string())
}

// 或使用模式匹配
fn get_user_city_match(user: Option<User>) -> String {
    match user {
        Some(u) => match u.address {
            Some(a) => match a.city {
                Some(c) => c,
                None => "未知城市".to_string(),
            },
            None => "未知城市".to_string(),
        },
        None => "未知城市".to_string(),
    }
}

// 或使用 ? 操作符（函数返回 Result/Option）
fn get_user_city_opt(user: Option<User>) -> Option<String> {
    Some(user?.address?.city?)
}
```

### 性能考虑

1. **Rust**：`Option<T>` 是零成本抽象，编译时常量优化
2. **Dart/C#**：运行时检查，有轻微开销但通常可忽略
3. **空检查成本**：所有语言都有运行时检查成本

### 迁移和学习建议

| 从哪个语言来    | 学习 Dart 空安全 | 注意事项                               |
| --------------- | ---------------- | -------------------------------------- |
| **C#**          | 非常容易         | 语法几乎相同，概念完全对应             |
| **Rust**        | 需要调整思维     | Dart 的 `!` 比 Rust 的 `unwrap()` 危险 |
| **Java/Kotlin** | 中等难度         | Kotlin 也有类似 `?.` 和 `?:`           |
| **JavaScript**  | 需要适应         | JS 没有真正的空安全，TypeScript 类似   |

## 最佳实践

### 1. **优先使用安全操作符**
```dart
// 好
String city = user?.address?.city ?? "未知";

// 避免（除非确定不为空）
String city = user!.address!.city!;
```

### 2. **限制 `!` 的使用**
```dart
// 只在确定的地方使用 !
String processName(String? name) {
  if (name == null) {
    throw ArgumentError('name不能为空');
  }
  // 这里可以安全使用 !
  return name!.toUpperCase();
  
  // 或者更好的方式
  return name.toUpperCase(); // 类型提升，自动非空
}
```

### 3. **利用类型提升**
```dart
void printLength(String? text) {
  if (text != null) {
    // 这里 text 自动提升为 String
    print(text.length); // 不需要 text!
  }
}
```

### 4. **Rust 风格的空安全（在 Dart 中）**
```dart
// 模仿 Rust 的 Result/Option 模式
class Option<T> {
  final T? _value;
  
  Option._(this._value);
  
  factory Option.some(T value) => Option._(value);
  factory Option.none() => Option._(null);
  
  bool get isSome => _value != null;
  bool get isNone => _value == null;
  
  T unwrap() {
    if (_value == null) throw StateError('Option is None');
    return _value!;
  }
  
  T unwrapOr(T defaultValue) => _value ?? defaultValue;
  
  Option<U> map<U>(U Function(T) f) {
    return _value == null 
      ? Option.none()
      : Option.some(f(_value!));
  }
}
```

## 总结

Dart 的空安全操作符与 C# 非常相似，几乎可以一一对应。与 Rust 相比，Dart 提供了更简洁的语法但牺牲了一些类型安全性。关键区别在于：

1. **语法相似度**：Dart ≈ C# > Rust
2. **类型安全性**：Rust > Dart ≈ C#
3. **学习曲线**：Rust > Dart ≈ C#
4. **实用性**：根据项目需求选择

对于大多数应用开发，Dart/C# 的空安全操作符提供了良好的平衡：足够的编译时检查，简洁的语法，合理的运行时安全性。对于系统编程或最高安全要求的场景，Rust 的 `Option<T>` 模式更可靠。

# Dart 函数可选参数 vs C++ vs C#

Dart 的函数参数系统非常灵活，支持**可选位置参数**、**可选命名参数**和**默认值**，与 C++ 和 C# 各有异同。

## 1. Dart 的参数系统

### 1.1 **可选位置参数** (`[]`)

```dart
// 可选位置参数 - 用 [] 表示
void printMessage(String message, [String? prefix, String suffix = "!"]) {
  print('${prefix ?? ""}$message$suffix');
}

void main() {
  printMessage("Hello");                // "Hello!"
  printMessage("Hello", "Info: ");      // "Info: Hello!"
  printMessage("Hello", "Info: ", "?"); // "Info: Hello?"
  
  // 必须按顺序传递
  // printMessage("Hello", "?"); // 错误：? 会传给 prefix 参数
}
```

### 1.2 **可选命名参数** (`{}`)

```dart
// 可选命名参数 - 用 {} 表示
void configureApp({
  String theme = 'light',
  int fontSize = 14,
  bool darkMode = false,
  required String language,  // required 表示必须传递
}) {
  print('Theme: $theme, Font: $fontSize, Dark: $darkMode, Lang: $language');
}

void main() {
  configureApp(language: 'zh');  // 必须传递 language
  configureApp(
    language: 'en',
    theme: 'dark',
    fontSize: 16,
  );
  
  // 命名参数可以乱序
  configureApp(
    fontSize: 18,
    theme: 'system',
    language: 'fr',
  );
}
```

### 1.3 **混合使用**

```dart
// 混合：位置参数 + 命名参数
void sendEmail(
  String to,
  String subject, [
  String body = '',  // 可选位置参数
  {
    String cc = '',
    String bcc = '',
    bool urgent = false,  // 可选命名参数
  }
]) {
  print('To: $to, Subject: $subject');
  if (body.isNotEmpty) print('Body: $body');
  if (cc.isNotEmpty) print('CC: $cc');
  if (urgent) print('URGENT!');
}

void main() {
  sendEmail('alice@example.com', 'Meeting');
  sendEmail('bob@example.com', 'Report', 'Please review', 
            cc: 'manager@example.com', urgent: true);
}
```

## 2. C++ 的参数默认值

### 2.1 **默认参数值**

```cpp
// C++ - 默认参数值（必须从右向左连续）
#include <iostream>
#include <string>

// 默认参数必须在声明中指定
void printMessage(const std::string& message, 
                  const std::string& prefix = "",
                  const std::string& suffix = "!") {
    std::cout << prefix << message << suffix << std::endl;
}

// 函数重载替代可选参数
void configureApp(const std::string& language) {
    configureApp(language, "light", 14, false);
}

void configureApp(const std::string& language, 
                  const std::string& theme,
                  int fontSize = 14,
                  bool darkMode = false) {
    std::cout << "Theme: " << theme 
              << ", Font: " << fontSize 
              << ", Dark: " << std::boolalpha << darkMode 
              << ", Lang: " << language << std::endl;
}

int main() {
    printMessage("Hello");               // "Hello!"
    printMessage("Hello", "Info: ");     // "Info: Hello!"
    printMessage("Hello", "Info: ", "?");// "Info: Hello?"
    
    configureApp("zh");
    configureApp("en", "dark", 16);      // fontSize=16, darkMode=false
    
    return 0;
}
```

### 2.2 **C++20 命名参数（模拟）**

```cpp
// C++ 没有原生命名参数，但可以模拟
#include <iostream>
#include <string>

struct Config {
    std::string theme = "light";
    int fontSize = 14;
    bool darkMode = false;
    
    // 链式调用设置器
    Config& setTheme(const std::string& t) { theme = t; return *this; }
    Config& setFontSize(int fs) { fontSize = fs; return *this; }
    Config& setDarkMode(bool dm) { darkMode = dm; return *this; }
};

void configureApp(const std::string& language, const Config& config = Config()) {
    std::cout << "Theme: " << config.theme 
              << ", Font: " << config.fontSize 
              << ", Dark: " << std::boolalpha << config.darkMode 
              << ", Lang: " << language << std::endl;
}

// 或者使用 Builder 模式
class ConfigBuilder {
    std::string theme = "light";
    int fontSize = 14;
    bool darkMode = false;
    
public:
    ConfigBuilder& withTheme(const std::string& t) { theme = t; return *this; }
    ConfigBuilder& withFontSize(int fs) { fontSize = fs; return *this; }
    ConfigBuilder& withDarkMode(bool dm) { darkMode = dm; return *this; }
    
    Config build() const { return Config{theme, fontSize, darkMode}; }
};

int main() {
    // 方法1：使用结构体
    configureApp("zh", Config{}.setTheme("dark").setFontSize(16));
    
    // 方法2：使用Builder模式
    configureApp("en", ConfigBuilder{}
        .withTheme("dark")
        .withFontSize(18)
        .withDarkMode(true)
        .build());
    
    return 0;
}
```

## 3. C# 的可选参数

### 3.1 **C# 4.0+ 可选和命名参数**

```csharp
using System;

class Program {
    // C# 可选参数和命名参数
    static void PrintMessage(string message, string prefix = "", string suffix = "!") {
        Console.WriteLine($"{prefix}{message}{suffix}");
    }
    
    // 命名参数
    static void ConfigureApp(string language, 
                             string theme = "light", 
                             int fontSize = 14, 
                             bool darkMode = false) {
        Console.WriteLine($"Theme: {theme}, Font: {fontSize}, Dark: {darkMode}, Lang: {language}");
    }
    
    // 可选参数 + 命名参数混合
    static void SendEmail(string to, 
                          string subject, 
                          string body = "", 
                          string cc = "", 
                          string bcc = "", 
                          bool urgent = false) {
        Console.WriteLine($"To: {to}, Subject: {subject}");
        if (!string.IsNullOrEmpty(body)) Console.WriteLine($"Body: {body}");
        if (!string.IsNullOrEmpty(cc)) Console.WriteLine($"CC: {cc}");
        if (urgent) Console.WriteLine("URGENT!");
    }
    
    static void Main() {
        // 可选参数
        PrintMessage("Hello");                // "Hello!"
        PrintMessage("Hello", "Info: ");      // "Info: Hello!"
        PrintMessage("Hello", suffix: "?");   // "Hello?" - 使用命名参数跳过 prefix
        
        // 命名参数可以乱序
        ConfigureApp("zh");
        ConfigureApp("en", fontSize: 16, theme: "dark");
        ConfigureApp(language: "fr", theme: "system", darkMode: true);
        
        // 混合使用
        SendEmail("alice@example.com", "Meeting");
        SendEmail("bob@example.com", "Report", 
                  cc: "manager@example.com", 
                  urgent: true,
                  body: "Please review");
        
        // C# 8.0+ 的简化调用
        SendEmail(to: "charlie@example.com", 
                  subject: "Update", 
                  body: "Important update");
    }
}
```

### 3.2 **C# 可选参数的限制**

```csharp
// C# 可选参数必须是编译时常量
class Calculator {
    // 合法 - 编译时常量
    public double Calculate(double x, double y = 0.0, int precision = 2) {
        return Math.Round(x + y, precision);
    }
    
    // 合法 - 常量表达式
    public double Calculate2(double x, double y = default, 
                             string mode = nameof(Calculate)) {
        return x + y;
    }
    
    // 非法 - 运行时值
    // public void SetTimeout(int milliseconds = GetDefaultTimeout()) { } // 错误
    
    // 解决方法：使用 Nullable 或重载
    public void SetTimeout(int? milliseconds = null) {
        int timeout = milliseconds ?? GetDefaultTimeout();
        Console.WriteLine($"Timeout: {timeout}ms");
    }
    
    private int GetDefaultTimeout() => 5000;
}
```

## 4. 三语言对比表

| 特性             | Dart         | C++          | C#               | 说明          |
| ---------------- | ------------ | ------------ | ---------------- | ------------- |
| **可选位置参数** | ✅ `[param]`  | ✅ 默认参数   | ✅ 默认参数       | Dart 最灵活   |
| **可选命名参数** | ✅ `{param}`  | ❌（可模拟）  | ✅ `param: value` | C# 语法不同   |
| **参数默认值**   | ✅ 任意表达式 | ✅ 编译时常量 | ✅ 编译时常量     | C++/C# 有限制 |
| **必需参数标记** | ✅ `required` | ❌            | ❌（可用特性）    | Dart 特有     |
| **参数顺序**     | 位置→命名    | 严格         | 位置→命名        | 类似          |
| **默认值位置**   | 任意位置     | 从右向左     | 任意位置         | C++ 最严格    |

## 5. 实际使用场景对比

### 5.1 **配置对象模式**

**Dart（最简洁）：**
```dart
// Dart - 直接使用命名参数
class ApiClient {
  final String baseUrl;
  final Duration timeout;
  final Map<String, String> headers;
  
  ApiClient({
    required this.baseUrl,
    this.timeout = const Duration(seconds: 30),
    this.headers = const {},
  });
  
  void call() {
    print('Calling $baseUrl with timeout $timeout');
  }
}

void main() {
  final client = ApiClient(
    baseUrl: 'https://api.example.com',
    timeout: Duration(seconds: 60),
    headers: {'Authorization': 'Bearer token'},
  );
}
```

**C++（最繁琐）：**
```cpp
// C++ - 需要 Builder 模式或大量重载
#include <iostream>
#include <string>
#include <map>
#include <chrono>

class ApiClient {
    std::string baseUrl;
    std::chrono::seconds timeout;
    std::map<std::string, std::string> headers;
    
public:
    // Builder 模式
    class Builder {
        std::string baseUrl_;
        std::chrono::seconds timeout_{30};
        std::map<std::string, std::string> headers_;
        
    public:
        Builder(const std::string& baseUrl) : baseUrl_(baseUrl) {}
        
        Builder& setTimeout(std::chrono::seconds timeout) {
            timeout_ = timeout;
            return *this;
        }
        
        Builder& addHeader(const std::string& key, const std::string& value) {
            headers_[key] = value;
            return *this;
        }
        
        ApiClient build() const {
            return ApiClient(baseUrl_, timeout_, headers_);
        }
    };
    
private:
    ApiClient(const std::string& url, 
              std::chrono::seconds timeout,
              const std::map<std::string, std::string>& headers)
        : baseUrl(url), timeout(timeout), headers(headers) {}
    
public:
    void call() const {
        std::cout << "Calling " << baseUrl 
                  << " with timeout " << timeout.count() << "s" << std::endl;
    }
};

int main() {
    auto client = ApiClient::Builder("https://api.example.com")
        .setTimeout(std::chrono::seconds(60))
        .addHeader("Authorization", "Bearer token")
        .build();
    
    client.call();
    
    return 0;
}
```

**C#（折中方案）：**
```csharp
// C# - 使用可选参数或对象初始化器
using System;
using System.Collections.Generic;

class ApiClient {
    public string BaseUrl { get; }
    public TimeSpan Timeout { get; }
    public Dictionary<string, string> Headers { get; }
    
    // 方法1：使用可选参数
    public ApiClient(string baseUrl, 
                     TimeSpan? timeout = null,
                     Dictionary<string, string>? headers = null) {
        BaseUrl = baseUrl;
        Timeout = timeout ?? TimeSpan.FromSeconds(30);
        Headers = headers ?? new Dictionary<string, string>();
    }
    
    // 方法2：使用 Builder/Fluent API
    public class Builder {
        private readonly string baseUrl;
        private TimeSpan timeout = TimeSpan.FromSeconds(30);
        private Dictionary<string, string> headers = new();
        
        public Builder(string baseUrl) {
            this.baseUrl = baseUrl;
        }
        
        public Builder WithTimeout(TimeSpan timeout) {
            this.timeout = timeout;
            return this;
        }
        
        public Builder WithHeader(string key, string value) {
            headers[key] = value;
            return this;
        }
        
        public ApiClient Build() {
            return new ApiClient(baseUrl, timeout, headers);
        }
    }
    
    public void Call() {
        Console.WriteLine($"Calling {BaseUrl} with timeout {Timeout.TotalSeconds}s");
    }
}

class Program {
    static void Main() {
        // 方法1：使用命名参数
        var client1 = new ApiClient(
            baseUrl: "https://api.example.com",
            timeout: TimeSpan.FromSeconds(60),
            headers: new Dictionary<string, string> { 
                ["Authorization"] = "Bearer token" 
            });
        
        // 方法2：使用 Builder
        var client2 = new ApiClient.Builder("https://api.example.com")
            .WithTimeout(TimeSpan.FromSeconds(60))
            .WithHeader("Authorization", "Bearer token")
            .Build();
    }
}
```

### 5.2 **构造函数重载对比**

```dart
// Dart - 使用命名参数工厂方法
class Rectangle {
  final double width;
  final double height;
  final Color color;
  
  Rectangle({
    required this.width,
    required this.height,
    this.color = Colors.black,
  });
  
  // 工厂方法替代重载
  factory Rectangle.square(double side, {Color color = Colors.black}) {
    return Rectangle(width: side, height: side, color: color);
  }
  
  factory Rectangle.fromJson(Map<String, dynamic> json) {
    return Rectangle(
      width: json['width'],
      height: json['height'],
      color: Color(json['color']),
    );
  }
}
```

```cpp
// C++ - 使用重载
#include <iostream>
#include <string>

class Rectangle {
    double width;
    double height;
    std::string color;
    
public:
    // 重载构造函数
    Rectangle(double w, double h, const std::string& c = "black")
        : width(w), height(h), color(c) {}
    
    Rectangle(double side)  // 正方形
        : Rectangle(side, side) {}
    
    // 静态工厂方法
    static Rectangle fromJson(const Json& json) {
        return Rectangle(json["width"], json["height"], json["color"]);
    }
};
```

```csharp
// C# - 使用可选参数或重载
using System;

class Rectangle {
    public double Width { get; }
    public double Height { get; }
    public string Color { get; }
    
    // 方法1：使用可选参数
    public Rectangle(double width, double height, string color = "black") {
        Width = width;
        Height = height;
        Color = color;
    }
    
    // 方法2：使用重载
    public Rectangle(double side) : this(side, side) { }
    
    public Rectangle(double side, string color) : this(side, side, color) { }
    
    // 静态工厂方法
    public static Rectangle FromJson(dynamic json) {
        return new Rectangle(
            width: (double)json.width,
            height: (double)json.height,
            color: (string)json.color
        );
    }
}
```

## 6. 最佳实践建议

### **Dart 最佳实践：**
```dart
// 1. 优先使用命名参数，提高可读性
void sendNotification({
  required String title,
  required String body,
  NotificationType type = NotificationType.info,
  Duration? duration,
  bool vibrate = true,
}) {
  // ...
}

// 2. 使用 required 标记必需参数
class DatabaseConfig {
  final String host;
  final int port;
  final String? username;
  final String? password;
  
  DatabaseConfig({
    required this.host,
    required this.port,
    this.username,
    this.password,
  });
}

// 3. 避免过多的可选参数（>4个考虑使用配置对象）
class ApiRequestConfig {
  final Map<String, String> headers;
  final Duration timeout;
  final bool retry;
  final int maxRetries;
  
  ApiRequestConfig({
    this.headers = const {},
    this.timeout = const Duration(seconds: 30),
    this.retry = false,
    this.maxRetries = 3,
  });
}
```

### **C++ 最佳实践：**
```cpp
// 1. 使用结构体传递多个可选参数
struct Config {
    std::string theme = "light";
    int fontSize = 14;
    bool darkMode = false;
};

void configureApp(const std::string& language, const Config& config = {});

// 2. 使用 Builder 模式复杂配置
auto client = HttpClient::Builder("https://example.com")
    .withTimeout(std::chrono::seconds(30))
    .withRetries(3)
    .build();

// 3. 使用重载而不是太多默认参数
class Logger {
public:
    void log(const std::string& message);
    void log(const std::string& message, LogLevel level);
    void log(const std::string& message, LogLevel level, 
             const std::string& component);
};
```

### **C# 最佳实践：**
```csharp
// 1. 使用命名参数提高可读性
var result = Calculate(
    principal: 1000,
    rate: 0.05,
    years: 10,
    compoundMonthly: true);

// 2. 使用可选参数简化常见调用
public void SendEmail(string to, string subject, 
                      string body = "", 
                      bool isHtml = false)
{
    // ...
}

// 3. 过多参数时使用对象初始化器
var options = new EmailOptions {
    To = "user@example.com",
    Subject = "Welcome",
    Body = "Welcome to our service",
    IsHtml = true,
    Attachments = new List<Attachment>()
};
SendEmail(options);
```

## 7. 性能考虑

### **编译时 vs 运行时：**
- **C++/C#**：默认值在编译时确定，性能零开销
- **Dart**：默认值可以是运行时表达式，有轻微开销

### **示例对比：**
```dart
// Dart - 默认值可以是运行时计算
void processData({
  List<int> data = const [],
  DateTime timestamp = _defaultTimestamp(),  // 运行时计算
}) {
  // ...
}

DateTime _defaultTimestamp() => DateTime.now();
```

```cpp
// C++ - 默认值必须是编译时常量
void processData(
    const std::vector<int>& data = {},  // 空列表
    std::time_t timestamp = std::time(nullptr)) {  // 编译错误！不是常量
    // 必须使用重载
}

// 正确做法
void processData(const std::vector<int>& data) {
    processData(data, std::time(nullptr));
}

void processData(const std::vector<int>& data, std::time_t timestamp) {
    // ...
}
```

```csharp
// C# - 默认值必须是编译时常量
void ProcessData(
    List<int> data = null!,  // 使用 null 默认值
    DateTime timestamp = default) {  // default(DateTime)
    data ??= new List<int>();
    timestamp = timestamp == default ? DateTime.Now : timestamp;
}
```

## 总结

1. **灵活性**：Dart > C# > C++
   - Dart 支持位置和命名可选参数，默认值可以是任意表达式
   - C# 支持命名参数，但默认值必须是常量
   - C++ 只有位置默认参数，且必须从右向左

2. **可读性**：Dart ≈ C# > C++
   - Dart 和 C# 的命名参数使调用更清晰
   - C++ 需要额外模式（Builder/结构体）来达到类似效果

3. **类型安全**：三者都提供良好的类型安全
   - Dart 的 `required` 关键字明确标记必需参数
   - C# 和 C++ 依赖编译器检查

4. **适用场景**：
   - **Dart**：适合配置丰富、API 易用的应用开发
   - **C++**：适合性能关键、接口稳定的系统编程
   - **C#**：适合企业应用、需要向后兼容的库开发

选择哪种方式取决于语言特性和项目需求，但所有语言都提供了解决复杂参数传递的方案。

# Dart的lambda和C#与JAVA的对比

**是的，Dart 的 lambda 表达式与 C# 和 Java 非常相似，尤其是 C#！** 它们都受到了函数式编程的影响，语法上有许多共通之处。

## 快速对比表

| 特性         | Dart              | C#     | Java   | 说明                                 |
| ------------ | ----------------- | ------ | ------ | ------------------------------------ |
| Lambda 箭头  | `=>`              | `=>`   | `->`   | Dart 和 C# 使用 `=>`，Java 使用 `->` |
| 参数类型推断 | ✅                 | ✅      | ✅      | 都可以省略类型                       |
| 单参数括号   | 可省略            | 可省略 | 可省略 | 都支持省略单个参数的括号             |
| 方法引用     | `::` (Dart 2.15+) | `::`   | `::`   | 都支持双冒号语法                     |
| 闭包捕获     | ✅                 | ✅      | ✅      | 都支持捕获外部变量                   |
| 高阶函数     | ✅                 | ✅      | ✅      | 都支持函数作为参数/返回值            |

## 详细语法对比

### 1. **基本 Lambda 语法**

**Dart:**
```dart
// 基本 lambda 表达式
(int x, int y) => x + y;

// 类型推断
var add = (x, y) => x + y;

// 多语句 lambda（必须用大括号）
var process = (String name) {
  print('Processing $name');
  return name.toUpperCase();
};

// 单参数可省略括号
var square = x => x * x;
var greet = name => 'Hello, $name!';
```

**C#:**
```csharp
// 基本 lambda 表达式
(int x, int y) => x + y;

// 类型推断
var add = (int x, int y) => x + y;  // C# 需要显式类型或 var

// 多语句 lambda
var process = (string name) => {
    Console.WriteLine($"Processing {name}");
    return name.ToUpper();
};

// 单参数可省略括号（但通常不省略）
var square = (int x) => x * x;
var greet = (string name) => $"Hello, {name}!";
```

**Java:**
```java
// 基本 lambda 表达式（Java 8+）
(int x, int y) -> x + y;

// 类型推断
BinaryOperator<Integer> add = (x, y) -> x + y;

// 多语句 lambda
Function<String, String> process = (String name) -> {
    System.out.println("Processing " + name);
    return name.toUpperCase();
};

// 单参数可省略括号
Function<Integer, Integer> square = x -> x * x;
Function<String, String> greet = name -> "Hello, " + name + "!";
```

### 2. **作为函数参数**

**Dart:**
```dart
void main() {
  List<int> numbers = [1, 2, 3, 4, 5];
  
  // map - 转换
  var doubled = numbers.map((x) => x * 2);
  print(doubled.toList()); // [2, 4, 6, 8, 10]
  
  // where - 过滤
  var evens = numbers.where((x) => x % 2 == 0);
  print(evens.toList()); // [2, 4]
  
  // forEach - 遍历
  numbers.forEach((x) => print(x));
  
  // reduce - 聚合
  var sum = numbers.reduce((total, x) => total + x);
  print(sum); // 15
  
  // 更复杂的 lambda
  numbers.sort((a, b) => b.compareTo(a)); // 降序排序
}
```

**C#:**
```csharp
using System;
using System.Collections.Generic;
using System.Linq;

class Program {
    static void Main() {
        List<int> numbers = new List<int> { 1, 2, 3, 4, 5 };
        
        // Select - 转换（相当于 map）
        var doubled = numbers.Select(x => x * 2);
        Console.WriteLine(string.Join(", ", doubled)); // 2, 4, 6, 8, 10
        
        // Where - 过滤
        var evens = numbers.Where(x => x % 2 == 0);
        Console.WriteLine(string.Join(", ", evens)); // 2, 4
        
        // ForEach - 遍历（List 的方法，不是 LINQ）
        numbers.ForEach(x => Console.WriteLine(x));
        
        // Aggregate - 聚合（相当于 reduce）
        var sum = numbers.Aggregate((total, x) => total + x);
        Console.WriteLine(sum); // 15
        
        // Sort - 排序
        numbers.Sort((a, b) => b.CompareTo(a)); // 降序排序
    }
}
```

**Java:**
```java
import java.util.*;
import java.util.stream.Collectors;

public class Main {
    public static void main(String[] args) {
        List<Integer> numbers = Arrays.asList(1, 2, 3, 4, 5);
        
        // map - 转换
        List<Integer> doubled = numbers.stream()
            .map(x -> x * 2)
            .collect(Collectors.toList());
        System.out.println(doubled); // [2, 4, 6, 8, 10]
        
        // filter - 过滤
        List<Integer> evens = numbers.stream()
            .filter(x -> x % 2 == 0)
            .collect(Collectors.toList());
        System.out.println(evens); // [2, 4]
        
        // forEach - 遍历
        numbers.forEach(x -> System.out.println(x));
        
        // reduce - 聚合
        Optional<Integer> sum = numbers.stream()
            .reduce((total, x) -> total + x);
        System.out.println(sum.orElse(0)); // 15
        
        // sort - 排序
        numbers.sort((a, b) -> b.compareTo(a)); // 降序排序
    }
}
```

### 3. **闭包（捕获外部变量）**

**Dart:**
```dart
void main() {
  int counter = 0;
  
  // 闭包捕获外部变量
  Function increment = () {
    counter++;  // 捕获并修改 counter
    return counter;
  };
  
  print(increment()); // 1
  print(increment()); // 2
  print(increment()); // 3
  
  // 创建多个闭包
  Function createMultiplier(int factor) {
    return (int x) => x * factor;  // 捕获 factor
  }
  
  var doubleIt = createMultiplier(2);
  var tripleIt = createMultiplier(3);
  
  print(doubleIt(5)); // 10
  print(tripleIt(5)); // 15
}
```

**C#:**
```csharp
using System;

class Program {
    static void Main() {
        int counter = 0;
        
        // 闭包捕获外部变量
        Func<int> increment = () => {
            counter++;  // 捕获并修改 counter
            return counter;
        };
        
        Console.WriteLine(increment()); // 1
        Console.WriteLine(increment()); // 2
        Console.WriteLine(increment()); // 3
        
        // 创建多个闭包
        Func<int, int> CreateMultiplier(int factor) {
            return x => x * factor;  // 捕获 factor
        }
        
        var doubleIt = CreateMultiplier(2);
        var tripleIt = CreateMultiplier(3);
        
        Console.WriteLine(doubleIt(5)); // 10
        Console.WriteLine(tripleIt(5)); // 15
    }
}
```

**Java:**
```java
import java.util.function.*;

public class Main {
    public static void main(String[] args) {
        // Java 中需要将外部变量声明为 final 或 effectively final
        int[] counter = {0};  // 使用数组模拟可变捕获
        
        // 闭包捕获外部变量
        IntSupplier increment = () -> {
            counter[0]++;  // 修改数组元素
            return counter[0];
        };
        
        System.out.println(increment.getAsInt()); // 1
        System.out.println(increment.getAsInt()); // 2
        System.out.println(increment.getAsInt()); // 3
        
        // 创建多个闭包
        IntFunction<IntUnaryOperator> createMultiplier = factor -> 
            x -> x * factor;  // 捕获 factor
        
        IntUnaryOperator doubleIt = createMultiplier.apply(2);
        IntUnaryOperator tripleIt = createMultiplier.apply(3);
        
        System.out.println(doubleIt.applyAsInt(5)); // 10
        System.out.println(tripleIt.applyAsInt(5)); // 15
    }
}
```

### 4. **方法引用（函数指针风格）**

**Dart (2.15+):**
```dart
class Person {
  final String name;
  final int age;
  
  Person(this.name, this.age);
  
  void sayHello() => print('Hello, I am $name');
  static void staticMethod() => print('Static method');
}

void main() {
  var people = [
    Person('Alice', 30),
    Person('Bob', 25),
    Person('Charlie', 35),
  ];
  
  // 实例方法引用
  people.forEach((p) => p.sayHello());  // 传统 lambda
  people.forEach(Person.sayHello);      // 方法引用 (Dart 2.15+)
  
  // 静态方法引用
  var staticFunc = Person.staticMethod;
  staticFunc();  // 调用静态方法
  
  // 用作回调
  void processPerson(void Function(Person) callback) {
    for (var person in people) {
      callback(person);
    }
  }
  
  processPerson(Person.sayHello);  // 传递方法引用
}
```

**C#:**
```csharp
using System;
using System.Collections.Generic;

class Person {
    public string Name { get; set; }
    public int Age { get; set; }
    
    public Person(string name, int age) {
        Name = name;
        Age = age;
    }
    
    public void SayHello() => Console.WriteLine($"Hello, I am {Name}");
    public static void StaticMethod() => Console.WriteLine("Static method");
}

class Program {
    static void Main() {
        var people = new List<Person> {
            new Person("Alice", 30),
            new Person("Bob", 25),
            new Person("Charlie", 35),
        };
        
        // 实例方法引用
        people.ForEach(p => p.SayHello());  // 传统 lambda
        people.ForEach(p => PersonExtensions.SayHello(p)); // 需要扩展方法
        // C# 直接方法引用不太方便，通常用 lambda
        
        // 静态方法引用
        Action staticFunc = Person.StaticMethod;
        staticFunc();
        
        // 方法组转换（C# 2.0+）
        people.ForEach(Console.WriteLine);  // 如果 Person 有合适的 ToString
        
        // 使用委托
        Action<Person> callback = p => p.SayHello();
        people.ForEach(callback);
    }
}
```

**Java:**
```java
import java.util.*;

class Person {
    private String name;
    private int age;
    
    public Person(String name, int age) {
        this.name = name;
        this.age = age;
    }
    
    public void sayHello() {
        System.out.println("Hello, I am " + name);
    }
    
    public static void staticMethod() {
        System.out.println("Static method");
    }
    
    public String getName() { return name; }
    
    public static void main(String[] args) {
        List<Person> people = Arrays.asList(
            new Person("Alice", 30),
            new Person("Bob", 25),
            new Person("Charlie", 35)
        );
        
        // 实例方法引用
        people.forEach(p -> p.sayHello());      // 传统 lambda
        people.forEach(Person::sayHello);       // 方法引用
        
        // 静态方法引用
        Runnable staticFunc = Person::staticMethod;
        staticFunc.run();
        
        // 构造方法引用
        Supplier<Person> personFactory = Person::new;
        Person newPerson = personFactory.get();
        
        // 参数方法引用
        people.stream()
            .map(Person::getName)  // 相当于 p -> p.getName()
            .forEach(System.out::println);
    }
}
```

### 5. **高阶函数（函数返回函数）**

**Dart:**
```dart
// 返回函数的函数
Function createAdder(int addBy) {
  return (int x) => x + addBy;  // 闭包捕获 addBy
}

// 接受函数作为参数的函数
int applyOperation(int x, int y, int Function(int, int) operation) {
  return operation(x, y);
}

void main() {
  var add5 = createAdder(5);
  print(add5(10)); // 15
  
  var add10 = createAdder(10);
  print(add10(20)); // 30
  
  // 使用函数参数
  var result = applyOperation(5, 3, (a, b) => a + b);
  print(result); // 8
  
  result = applyOperation(5, 3, (a, b) => a * b);
  print(result); // 15
}
```

**C#:**
```csharp
using System;

class Program {
    // 返回函数的函数
    static Func<int, int> CreateAdder(int addBy) {
        return x => x + addBy;  // 闭包捕获 addBy
    }
    
    // 接受函数作为参数的函数
    static int ApplyOperation(int x, int y, Func<int, int, int> operation) {
        return operation(x, y);
    }
    
    static void Main() {
        var add5 = CreateAdder(5);
        Console.WriteLine(add5(10)); // 15
        
        var add10 = CreateAdder(10);
        Console.WriteLine(add10(20)); // 30
        
        // 使用函数参数
        var result = ApplyOperation(5, 3, (a, b) => a + b);
        Console.WriteLine(result); // 8
        
        result = ApplyOperation(5, 3, (a, b) => a * b);
        Console.WriteLine(result); // 15
    }
}
```

**Java:**
```java
import java.util.function.*;

public class Main {
    // 返回函数的函数
    static IntUnaryOperator createAdder(int addBy) {
        return x -> x + addBy;  // 闭包捕获 addBy
    }
    
    // 接受函数作为参数的函数
    static int applyOperation(int x, int y, IntBinaryOperator operation) {
        return operation.applyAsInt(x, y);
    }
    
    public static void main(String[] args) {
        IntUnaryOperator add5 = createAdder(5);
        System.out.println(add5.applyAsInt(10)); // 15
        
        IntUnaryOperator add10 = createAdder(10);
        System.out.println(add10.applyAsInt(20)); // 30
        
        // 使用函数参数
        int result = applyOperation(5, 3, (a, b) -> a + b);
        System.out.println(result); // 8
        
        result = applyOperation(5, 3, (a, b) -> a * b);
        System.out.println(result); // 15
    }
}
```

## 特殊语法差异

### 1. **Dart 特有的简洁语法**

```dart
// 1. 单行函数体可省略大括号和 return
int add(int a, int b) => a + b;  // 等同于 { return a + b; }

// 2. 匿名函数作为参数时的极简写法
void main() {
  List<int> numbers = [1, 2, 3];
  
  // 如果参数是函数且只有一个参数，括号可省略
  numbers.forEach((n) => print(n));
  numbers.forEach(n => print(n));  // 省略括号
  
  // 当函数体只有单个方法调用时的极简写法
  numbers.forEach(print);  // Dart 早期版本需要 print 函数的引用
}

// 3. 级联操作符配合 lambda
class Person {
  String name = '';
  int age = 0;
  
  void sayHello() => print('Hello, $name');
}

void main() {
  var person = Person()
    ..name = 'Alice'
    ..age = 30
    ..sayHello();
}
```

### 2. **C# 特有的 Lambda 特性**

```csharp
// 1. Expression-bodied 成员（类似 Dart 的 =>）
public class Person {
    public string Name { get; set; }
    public int Age => DateTime.Now.Year - BirthYear;  // 计算属性
    
    public void SayHello() => Console.WriteLine($"Hello, {Name}");
}

// 2. Local functions（Dart 和 Java 也有类似概念）
void ProcessData() {
    int Multiply(int x, int y) => x * y;  // 局部函数
    
    var result = Multiply(5, 3);
    Console.WriteLine(result);
}

// 3. LINQ 查询语法（Dart 没有直接等价）
var adults = from person in people
             where person.Age >= 18
             select person.Name;
```

### 3. **Java 特有的 Lambda 特性**

```java
// 1. 函数式接口（Single Abstract Method）
@FunctionalInterface  // 注解，非必需
interface Calculator {
    int calculate(int a, int b);
}

// 2. 方法引用更多形式
List<String> names = Arrays.asList("Alice", "Bob");
names.forEach(System.out::println);  // 实例方法引用
names.stream()
    .map(String::toUpperCase)        // 实例方法引用（无参数）
    .forEach(System.out::println);

// 3. Stream API 的丰富操作
List<Integer> numbers = Arrays.asList(1, 2, 3, 4, 5);
int sum = numbers.stream()
    .filter(n -> n % 2 == 0)
    .map(n -> n * 2)
    .reduce(0, Integer::sum);
```

## 三语言 Lambda 对比总结

| 方面           | Dart           | C#               | Java                | 总结                          |
| -------------- | -------------- | ---------------- | ------------------- | ----------------------------- |
| **语法相似度** | 与 C# 非常相似 | 与 Dart 非常相似 | 略有不同            | Dart 和 C# 几乎可以互换       |
| **学习曲线**   | 简单           | 简单             | 中等                | Java 的泛型和函数式接口更复杂 |
| **类型系统**   | 可选类型       | 强类型           | 强类型              | 都支持类型推断                |
| **生态系统**   | Flutter 为主   | .NET 广泛        | 企业级广泛          | 各有优势领域                  |
| **性能**       | JIT/AOT        | JIT/AOT          | JIT                 | 都经过高度优化                |
| **异步支持**   | `async/await`  | `async/await`    | `CompletableFuture` | Dart 和 C# 更相似             |

## 迁移建议

### 从 C# 迁移到 Dart：
```csharp
// C#
var result = collection.Where(x => x > 5)
                      .Select(x => x * 2)
                      .ToList();
```

```dart
// Dart - 几乎一样
var result = collection.where((x) => x > 5)
                      .map((x) => x * 2)
                      .toList();
```

### 从 Java 迁移到 Dart：
```java
// Java
list.stream()
    .filter(x -> x > 5)
    .map(x -> x * 2)
    .collect(Collectors.toList());
```

```dart
// Dart - 更简洁
list.where((x) => x > 5)
    .map((x) => x * 2)
    .toList();
```

### 从 Dart 迁移到 C#/Java：
- **到 C#**：基本语法不变，注意 LINQ 查询语法
- **到 Java**：`=>` 改为 `->`，学习 Stream API

## 最佳实践

### 通用最佳实践：
```dart
// 1. 保持 lambda 简短
// 好
items.where((item) => item.isValid && item.isActive);

// 不好（应该提取为命名函数）
items.where((item) {
  var isValid = validateItem(item);
  var isActive = checkActivity(item);
  return isValid && isActive && item.price > 100;
});

// 2. 使用有意义的参数名
// 好
users.map((user) => user.name);
// 不好
users.map((u) => u.name);

// 3. 避免嵌套过深
// 好
var activeUserNames = users
    .where((user) => user.isActive)
    .map((user) => user.name)
    .toList();

// 不好
var result = users.where((u) => u.isActive).map((u) => u.name).toList();
```

## 结论

**Dart 的 lambda 与 C# 的 lambda 极其相似**，几乎可以无缝转换。与 Java 相比，主要区别在于箭头符号（`=>` vs `->`）和一些 API 设计上的差异。

**关键相似点：**
1. 都使用箭头语法表示简洁函数
2. 都支持类型推断和闭包
3. 都支持方法引用
4. 都有丰富的集合操作 API

**主要差异：**
1. **箭头符号**：Dart/C# 用 `=>`，Java 用 `->`
2. **生态系统**：Dart 主要用于 Flutter，C#/.NET 和 Java 更广泛
3. **API 风格**：Dart 的集合 API 更像 C# 的 LINQ

如果你熟悉 C# 或 Java 的 lambda，学习 Dart 的 lambda 会非常容易！

# lambda闭包捕获

### **C++ 与 Dart捕获方式对比**

| 特性         | Dart           | C++               | 说明                        |
| ------------ | -------------- | ----------------- | --------------------------- |
| **捕获声明** | 隐式自动捕获   | 显式 `[]` 声明    | C++ 必须明确指定捕获什么    |
| **默认捕获** | 引用捕获       | 无默认，必须指定  | C++ 没有默认行为            |
| **值捕获**   | 需手动拷贝     | `[=]` 或 `[var]`  | C++ 可以值捕获              |
| **引用捕获** | 默认就是       | `[&]` 或 `[&var]` | C++ 需要 `&`                |
| **mutable**  | 不需要         | `mutable` 关键字  | Dart 闭包总是可修改捕获变量 |
| **修改捕获** | 可修改引用对象 | 取决于捕获方式    | Dart 更灵活                 |

## 3. Dart 的陷阱和解决方案

### 3.1 **问题1：循环中的闭包陷阱**

这是最常见的问题，与 JavaScript 类似：

dart

```
void loopTrap() {
  List<Function> closures = [];
  
  for (var i = 0; i < 3; i++) {
    // 所有闭包捕获的是同一个 i 的引用
    closures.add(() => print(i));
  }
  
  // 执行时 i 已经变成 3
  closures.forEach((f) => f());  // 输出: 3, 3, 3
}

// 解决方案1：使用局部变量拷贝值
void loopSolution1() {
  List<Function> closures = [];
  
  for (var i = 0; i < 3; i++) {
    var captured = i;  // 创建局部拷贝
    closures.add(() => print(captured));
  }
  
  closures.forEach((f) => f());  // 输出: 0, 1, 2
}

// 解决方案2：使用 IIFE（立即执行函数表达式）
void loopSolution2() {
  List<Function> closures = [];
  
  for (var i = 0; i < 3; i++) {
    (() {
      var captured = i;  // 每个闭包有自己的作用域
      closures.add(() => print(captured));
    })();
  }
  
  closures.forEach((f) => f());  // 输出: 0, 1, 2
}
```



### 3.2 **问题2：延迟执行导致的值变化**

dart

```
void delayTrap() async {
  int value = 0;
  
  // 延迟执行闭包
  Future.delayed(Duration(seconds: 1), () {
    print('Delayed: $value');  // 捕获的是 value 的引用
  });
  
  value = 100;  // 修改值
  // 1秒后输出: Delayed: 100
}

// 解决方案：创建快照
void delaySolution() async {
  int value = 0;
  int snapshot = value;  // 创建当前值的拷贝
  
  Future.delayed(Duration(seconds: 1), () {
    print('Delayed: $snapshot');  // 使用快照
  });
  
  value = 100;  // 修改原始值
  // 1秒后输出: Delayed: 0
}
```



### 3.3 **问题3：修改基本类型的值**

dart

```
void modifyTrap() {
  int count = 0;
  
  // 错误：不能修改捕获的基本类型变量
  // var increment = () => count++;  // 编译错误
  
  // 解决方案1：使用对象包装
  class Counter {
    int value = 0;
  }
  
  Counter counter = Counter();
  var increment = () => counter.value++;
  
  increment();
  print(counter.value);  // 1
  
  // 解决方案2：使用数组包装
  List<int> countList = [0];
  var increment2 = () => countList[0]++;
  
  increment2();
  print(countList[0]);  // 1
  
  // 解决方案3：返回新值
  var increment3 = () {
    int oldValue = count;
    count = oldValue + 1;
    return count;
  };
}
```

## 2. Lambda 引用捕获的生命周期问题

### 2.1 **闭包延长对象生命周期**

dart

```
class ExpensiveResource {
  final String name;
  
  ExpensiveResource(this.name) {
    print('创建: $name');
  }
  
  @override
  void finalize() {
    print('销毁: $name');
  }
}

void main() {
  // 场景 1：闭包延长生命周期
  Function closure;
  
  {
    var resource = ExpensiveResource('临时资源');
    closure = () {
      print('使用: ${resource.name}');
    };
    
    // resource 本应在此作用域结束被回收
    // 但被 closure 捕获，所以不会立即回收
  } // 作用域结束
  
  closure();  // 仍然可以访问 resource
  // 输出: "使用: 临时资源"
  
  closure = null;  // 现在 resource 可以被 GC 回收
  // 可能输出: "销毁: 临时资源" (GC 时机不确定)
}
```



### 2.2 **意外的生命周期延长**

dart

```
import 'dart:async';

void lifecycleTrap() {
  // 大对象被闭包意外保持
  List<int> bigData = List.generate(1000000, (i) => i);
  
  // 闭包只使用了 bigData.length，但捕获了整个列表
  var getLength = () => bigData.length;
  
  // 我们不再需要 bigData，但以为它会被回收
  bigData = [];
  
  // 实际上 bigData 的原始数据仍在内存中！
  // 因为 getLength 闭包仍然持有对原始 bigData 的引用
  print('Length: ${getLength()}');  // 仍然输出 1000000
  
  // 需要显式释放
  getLength = null;
  // 现在原始 bigData 数据才可以被 GC
}
```



### 2.3 **事件监听器的内存泄漏**

dart

```
import 'dart:async';

class EventEmitter {
  final _listeners = <Function>[];
  
  void addListener(Function listener) {
    _listeners.add(listener);
  }
  
  void emit() {
    for (var listener in _listeners) {
      listener();
    }
  }
  
  void clearListeners() {
    _listeners.clear();
  }
}

void memoryLeakExample() {
  var emitter = EventEmitter();
  var data = List.generate(10000, (i) => 'Data $i');
  
  // 常见错误：忘记移除监听器
  emitter.addListener(() {
    print('数据长度: ${data.length}');
  });
  
  // 即使不再需要 data，它也不会被回收
  // 因为监听器闭包仍然持有引用
  
  data = [];  // 以为释放了内存，实际上没有
  
  // 正确做法：需要移除监听器
  emitter.clearListeners();
  // 现在 data 可以被回收
}
```

# 引用迭代器不允许修改后使用

```
void dartCollectionSafety() {
  List<int> list = [1, 2, 3, 4, 5];
  
  // 1. 直接引用元素
  var elementRef = list[0];
  print('元素引用: $elementRef');  // 1
  
  // 扩容
  for (int i = 0; i < 100; i++) {
    list.add(i);
  }
  
  // 引用仍然有效
  print('扩容后元素引用: $elementRef');  // 1，仍然有效
  
  // 2. 迭代器安全
  var iterator = list.iterator;
  
  try {
    while (iterator.moveNext()) {
      print('当前元素: ${iterator.current}');
      
      // 在迭代中修改集合 - 抛出异常
      if (iterator.current == 3) {
        list.add(100);  // 抛出 ConcurrentModificationError
      }
    }
  } catch (e) {
    print('迭代时修改错误: $e');
  }
  
  // 3. 安全的迭代方式
  for (var item in list.toList()) {  // 创建副本
    if (item == 3) {
      list.add(100);  // 安全，因为迭代的是副本
    }
  }
}
```

## 命名构造函数

## 成员可访问性

默认为public，如果需要private，则在成员变量或者采用函数的名字前加上下划线前缀，dart会自动识别

# Dart中类的继承和Java类似

● 定义：继承是拥有父类的属性和方法
●特点：dart属于单继承，一个类只能拥有一个直接父类，子类拥有父类所有的属性和方法

●语法：class类名extends父类
●重写：子类可通过@override注解重写父类方法，扩展其行为（不需要标记父类方法为virtual和C++不一样）
●注意：子类不会继承父类构造函数，子类必须通过super关键字调用父类构造函数确保父类正确初始化

●super语法：子类构造函数(可选命名参数):super({参数))类似C++的委托构造，C++/Qt常用这种方式:QWidget(parent)



* abstract声明抽象类
* 子类 implements 实现抽象类

达到接口复用的逻辑，和java一样



特有 依赖注入 组合的方式，类似ECS游戏开发的模型，事先实现一个功能，当前类又不想添加新方法，只需要with

Dart中类的混入`mixin`
●定义：Dart允许在不使用传统继承的情况下，向类中添加新的功能
●方式：使用mixin关键字定义一个对象
●方式：使用with关键字将定义的对象混入到当前对象
●特点：**一个类支持with多个mixin，调用优先级遵循“后来居上”原则，即后混入的会覆盖先混入的同名方法**
●需求：让一个学生类和一个老师类都拥有唱歌的方法



# 容器泛型

如果声明一个容器不 标记其存储的实际类型， 那么默认为所有类型

如 `List list = []` 等价于C++的`std::vector<std::any> vec{};`



# dart多线程

●介绍：Dart是单线程语言，即同时只能做一件事，遇到耗时任务就会造成程序阻塞，此时需要异步编程
●定义：Dart采用单线程+事件循环的机制完成耗时任务的处理

微任务队列：Future.microtask()
事件队列：Future、Future.delayed()、I/O操作(件、网络)等

## Dart中异步编程-Future和C++的std::future对比

•介绍：Future代表一个异步操作的最终结果.
●状态：Uncompleted（等待）、Completed with a value(成功)、Completed with aerror(失败)
●创建：
Future(0) 0)
●执行成功：不抛出异常-成功状态-then(()()

●执行失败：throw Exception()-失败状态-catchError((){)

Dart中的Future和C++的std::future都是用于表示异步操作结果的类型，但它们在设计和使用上有一些差异。下面我们将从多个方面进行比较。

### 1. 基本概念

- **Dart Future**: 代表一个异步操作的最终结果，它有三种状态：未完成（Uncompleted）、成功完成（Completed with a  value）、失败完成（Completed with an  error）。Future是Dart异步编程的核心，通常与async/await一起使用。
- **C++ std::future**: 表示一个异步操作的结果，它可以在未来的某个时间点获取。通常与std::async、std::promise等一起使用。状态上，它也可以表示未完成、成功完成（有值）和失败（异常）。

### 2. 创建方式

**Dart**:

dart

```
// 通过Future构造函数创建
Future<String> future = Future(() {
  // 执行异步操作
  return "Hello, Future!";
});

// 通过async函数返回Future
Future<String> fetchData() async {
  // 模拟异步操作
  await Future.delayed(Duration(seconds: 1));
  return "Data";
}
```



**C++**:

cpp

```
#include <future>
#include <iostream>

// 通过std::async创建
std::future<std::string> future = std::async([]() {
    // 执行异步操作
    return std::string("Hello, Future!");
});

// 通过std::promise创建
std::promise<std::string> promise;
std::future<std::string> future = promise.get_future();
// 在某个线程中设置值
promise.set_value("Hello, Future!");
```



### 3. 获取结果

**Dart**:

dart

```
// 使用then和catchError
future.then((value) {
  print("成功: $value");
}).catchError((error) {
  print("失败: $error");
});

// 使用async/await
try {
  var value = await future;
  print("成功: $value");
} catch (error) {
  print("失败: $error");
}
```



**C++**:

cpp

```
// 使用get()获取结果，会阻塞直到结果可用
try {
    std::string value = future.get(); // 如果异步操作抛出异常，get()会重新抛出
    std::cout << "成功: " << value << std::endl;
} catch (const std::exception& e) {
    std::cout << "失败: " << e.what() << std::endl;
}

// 使用wait()等待完成，但不获取结果
future.wait(); // 阻塞直到完成
```



### 4. 异常处理

- **Dart**: Future可以通过catchError捕获异常，或者在async/await中使用try/catch。
- **C++**: std::future在调用get()时，如果异步操作中抛出了异常，则get()会重新抛出该异常。

### 5. 链式调用

**Dart**的Future支持链式调用，类似于Promise：

dart

```
Future<String> future = fetchData();
future
    .then((data) => process(data))
    .then((processed) => print(processed))
    .catchError((error) => print(error));
```



**C++**的std::future本身不支持链式调用，但可以通过返回future的函数手动链接，或者使用第三方库（如Futures from Facebook's Folly库）。

### 6. 组合多个Future

**Dart**:

dart

```
// 等待多个Future完成
Future.wait([future1, future2, future3])
    .then((List<dynamic> values) => print(values));

// 等待任意一个Future完成
Future.any([future1, future2, future3])
    .then((value) => print(value));
```



**C++**:

cpp

```
// 使用when_all和when_any（C++20引入）
std::future<int> future1 = std::async([](){ return 1; });
std::future<int> future2 = std::async([](){ return 2; });

// when_all返回一个future，当所有future完成时，它包含一个future的元组
auto all_future = std::when_all(future1.share(), future2.share());
all_future.then([](std::tuple<std::future<int>, std::future<int>> futures) {
    // 处理结果
});

// when_any返回一个future，当任意一个future完成时，它包含一个索引和future的元组
auto any_future = std::when_any(future1.share(), future2.share());
```



### 7. 超时设置

**Dart**:

dart

```
future.timeout(Duration(seconds: 5))
    .then((value) => print(value))
    .catchError((error) => print("超时或错误: $error"));
```



**C++**:

cpp

```
// 使用wait_for等待一段时间
std::future_status status = future.wait_for(std::chrono::seconds(5));
if (status == std::future_status::ready) {
    // 完成
} else if (status == std::future_status::timeout) {
    // 超时
} else {
    // 延迟（可能不会遇到）
}
```



### 8. 总结对比

| 特性           | Dart Future                            | C++ std::future                       |
| -------------- | -------------------------------------- | ------------------------------------- |
| 创建方式       | Future构造函数、async函数              | std::async、std::promise              |
| 获取结果       | then/catchError、async/await           | get()、wait()                         |
| 异常处理       | catchError、try/catch                  | get()重新抛出异常                     |
| 链式调用       | 支持                                   | 不支持（需手动或使用第三方库）        |
| 组合多个Future | Future.wait、Future.any                | std::when_all、std::when_any（C++20） |
| 超时设置       | timeout方法                            | wait_for                              |
| 线程模型       | 单线程事件循环，异步操作通常在同一线程 | 多线程，异步操作可在不同线程          |

### 9. 补充：Dart中的async/await

Dart的async/await语法使得异步代码看起来像同步代码，这大大简化了异步编程。例如：

dart

```
Future<void> printData() async {
  try {
    var data = await fetchData();
    print(data);
  } catch (e) {
    print('错误: $e');
  }
}
```



在C++中，虽然也有协程（C++20），但使用方式与Dart有所不同，通常需要更多的样板代码。

### 10. 性能考虑

- **Dart**的Future是基于事件循环的，适用于I/O密集型操作，避免了线程切换的开销。
- **C++**的std::future通常与线程一起使用，适用于CPU密集型操作，可以利用多核。

### 11. 示例：从网络获取数据

**Dart**:

dart

```
import 'dart:convert';
import 'package:http/http.dart' as http;

Future<void> fetchUserData() async {
  try {
    var response = await http.get(Uri.parse('https://api.example.com/user'));
    if (response.statusCode == 200) {
      var user = jsonDecode(response.body);
      print(user);
    } else {
      throw Exception('Failed to load data');
    }
  } catch (e) {
    print('Error: $e');
  }
}
```



**C++**:

cpp

```
#include <future>
#include <iostream>
#include <curl/curl.h>

std::string fetchData(const std::string& url) {
    // 使用CURL进行网络请求
    CURL* curl = curl_easy_init();
    // 设置URL等
    // 执行请求并返回数据
}

int main() {
    std::future<std::string> future = std::async(fetchData, "https://api.example.com/user");
    try {
        std::string data = future.get();
        std::cout << data << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    return 0;
}
```

### 结论

Dart的Future和C++的std::future都是处理异步操作的有效工具，但它们在设计哲学和用法上有显著不同。Dart的Future更接近于JavaScript的Promise，注重链式调用和事件循环；而C++的std::future则更注重与多线程的结合，提供了更底层的控制。根据应用场景选择合适的工具是非常重要的。

# Dart `Future` vs C++ `std::future` 对比

这是一个很好的对比！Dart 的 `Future` 和 C++ 的 `std::future` 都是处理异步操作的核心机制，但它们在设计理念、使用方式和能力上有显著区别。

## 1. 核心概念对比

| 特性         | Dart `Future`        | C++ `std::future`  | 说明                                  |
| ------------ | -------------------- | ------------------ | ------------------------------------- |
| **设计目的** | 事件循环，单线程异步 | 多线程并行计算     | Dart 为 I/O 密集型，C++ 为 CPU 密集型 |
| **执行模型** | 单线程事件循环       | 多线程并行执行     | Dart 避免线程切换，C++ 利用多核       |
| **状态**     | 三种明确状态         | 主要是两种状态     | Dart 状态机更清晰                     |
| **异常处理** | 内置异常传播         | 异常传播到 `get()` | 两者都支持异常                        |
| **组合能力** | 丰富的组合器         | 基础组合功能       | Dart `Future` 更强大                  |
| **链式调用** | ✅ 原生支持           | ⚠️ 需要 C++20       | Dart 语法更优雅                       |

## 2. 状态模型对比

### Dart `Future` 的三种状态：
```dart
// Dart Future 的三种状态
Future<void> exampleFuture() async {
  // 1. Uncompleted (等待)
  Future<int> future = computeAsync();
  
  // 2. Completed with a value (成功)
  future.then((value) {
    print('成功: $value');
  });
  
  // 3. Completed with an error (失败)
  future.catchError((error) {
    print('失败: $error');
  });
  
  // 或者使用 async/await
  try {
    int value = await future;  // 等待完成
    print('成功: $value');
  } catch (e) {
    print('失败: $e');
  }
}
```

### C++ `std::future` 的状态：
```cpp
// C++ std::future 的状态
#include <future>
#include <iostream>

void exampleFuture() {
  // 启动异步任务
  std::future<int> future = std::async([]() {
    // 模拟计算
    if (/* 成功 */) {
      return 42;
    } else {
      throw std::runtime_error("计算失败");
    }
  });
  
  try {
    // 等待并获取结果（阻塞）
    int value = future.get();  // 如果已失败会抛出异常
    
    // future 现在无效（移动语义）
    std::cout << "成功: " << value << std::endl;
  } catch (const std::exception& e) {
    std::cout << "失败: " << e.what() << std::endl;
  }
  
  // 状态检查
  std::future<int> future2 = std::async([]() { return 100; });
  
  if (future2.valid()) {
    std::cout << "future 有效" << std::endl;
  }
  
  // 等待状态（不阻塞）
  auto status = future2.wait_for(std::chrono::seconds(0));
  if (status == std::future_status::ready) {
    std::cout << "已就绪" << std::endl;
  } else if (status == std::future_status::timeout) {
    std::cout << "超时（未就绪）" << std::endl;
  } else if (status == std::future_status::deferred) {
    std::cout << "延迟执行" << std::endl;
  }
}
```

## 3. 创建方式对比

### Dart `Future` 创建：
```dart
// 1. Future 构造函数
Future<String> future1 = Future(() {
  // 异步计算
  return "结果";
});

// 2. Future.value - 立即完成的 Future
Future<String> future2 = Future.value("立即值");

// 3. Future.error - 立即失败的 Future
Future<String> future3 = Future.error(Exception("错误"));

// 4. Future.delayed - 延迟执行
Future<String> future4 = Future.delayed(
  Duration(seconds: 2),
  () => "延迟结果"
);

// 5. async 函数自动返回 Future
Future<int> computeAsync() async {
  await Future.delayed(Duration(seconds: 1));
  return 42;
}
```

### C++ `std::future` 创建：
```cpp
#include <future>
#include <thread>
#include <iostream>

// 1. std::async - 最常用
std::future<int> future1 = std::async(std::launch::async, []() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 42;
});

// 2. std::promise + std::future
std::promise<int> promise;
std::future<int> future2 = promise.get_future();

// 在另一个线程中设置值
std::thread([&promise]() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    promise.set_value(100);
}).detach();

// 3. std::packaged_task
std::packaged_task<int()> task([]() {
    return 200;
});
std::future<int> future3 = task.get_future();

// 在另一个线程执行
std::thread(std::move(task)).detach();
```

## 4. 执行成功/失败处理对比

### Dart 的成功处理：
```dart
void dartSuccessHandling() {
  // 创建 Future
  Future<int> compute() async {
    await Future.delayed(Duration(seconds: 1));
    return 42;
  }
  
  // 方法1: then - 成功回调
  compute().then((value) {
    print('成功: $value');
  });
  
  // 方法2: async/await
  try {
    int value = await compute();
    print('成功: $value');
  } catch (e) {
    // 处理错误
  }
  
  // 方法3: whenComplete (类似 finally)
  compute()
    .then((value) => print('成功: $value'))
    .whenComplete(() => print('完成'));
}
```

### Dart 的失败处理：
```dart
void dartErrorHandling() {
  // 创建会失败的 Future
  Future<int> computeWithError() async {
    await Future.delayed(Duration(seconds: 1));
    throw Exception('计算失败');
  }
  
  // 方法1: catchError
  computeWithError()
    .then((value) => print('成功: $value'))
    .catchError((error, stackTrace) {
      print('错误: $error');
      print('堆栈: $stackTrace');
    });
  
  // 方法2: async/await with try-catch
  try {
    int value = await computeWithError();
    print('成功: $value');
  } catch (e, s) {
    print('错误: $e');
    print('堆栈: $s');
  }
  
  // 方法3: onError 处理特定异常
  computeWithError()
    .then((value) => print('成功: $value'))
    .onError((error, stackTrace) {
      if (error is FormatException) {
        return -1;  // 返回默认值
      }
      throw error;  // 重新抛出
    })
    .then(print);
}
```

### C++ 的成功/失败处理：
```cpp
#include <future>
#include <iostream>
#include <stdexcept>

void cppFutureHandling() {
  // 创建异步任务
  auto compute = []() -> int {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (rand() % 2 == 0) {
      return 42;
    } else {
      throw std::runtime_error("计算失败");
    }
  };
  
  // 方法1: get() 阻塞等待并处理异常
  std::future<int> future = std::async(std::launch::async, compute);
  
  try {
    int value = future.get();  // 阻塞，可能抛出异常
    std::cout << "成功: " << value << std::endl;
  } catch (const std::exception& e) {
    std::cout << "错误: " << e.what() << std::endl;
  }
  
  // 方法2: 检查状态后再获取
  std::future<int> future2 = std::async(std::launch::async, compute);
  
  // 等待一段时间
  auto status = future2.wait_for(std::chrono::seconds(2));
  
  if (status == std::future_status::ready) {
    try {
      int value = future2.get();
      std::cout << "成功: " << value << std::endl;
    } catch (...) {
      std::cout << "任务抛出异常" << std::endl;
    }
  } else {
    std::cout << "任务未完成" << std::endl;
  }
  
  // 方法3: 使用 shared_future 多个等待者
  std::shared_future<int> shared = std::async(std::launch::async, compute);
  
  // 多个线程可以等待同一个结果
  auto waiter1 = [shared]() {
    try {
      std::cout << "等待者1: " << shared.get() << std::endl;
    } catch (...) {
      std::cout << "等待者1捕获异常" << std::endl;
    }
  };
  
  auto waiter2 = [shared]() {
    try {
      std::cout << "等待者2: " << shared.get() << std::endl;
    } catch (...) {
      std::cout << "等待者2捕获异常" << std::endl;
    }
  };
  
  std::thread t1(waiter1);
  std::thread t2(waiter2);
  t1.join();
  t2.join();
}
```

## 5. 链式调用和组合对比

### Dart 的链式调用（非常强大）：
```dart
void dartChaining() {
  Future<int> fetchUserId() async {
    await Future.delayed(Duration(milliseconds: 500));
    return 123;
  }
  
  Future<String> fetchUserName(int userId) async {
    await Future.delayed(Duration(milliseconds: 500));
    return "用户$userId";
  }
  
  Future<String> fetchUserDetails(String name) async {
    await Future.delayed(Duration(milliseconds: 500));
    return "$name 的详细信息";
  }
  
  // 链式调用 - 类似 Promise
  fetchUserId()
    .then((userId) {
      print('获取到用户ID: $userId');
      return fetchUserName(userId);
    })
    .then((userName) {
      print('获取到用户名: $userName');
      return fetchUserDetails(userName);
    })
    .then((details) {
      print('获取到用户详情: $details');
    })
    .catchError((error) {
      print('链中出错: $error');
    });
  
  // 使用 async/await 更清晰
  try {
    int userId = await fetchUserId();
    String userName = await fetchUserName(userId);
    String details = await fetchUserDetails(userName);
    print('最终结果: $details');
  } catch (e) {
    print('错误: $e');
  }
}
```

### C++ 的链式调用（C++20 之前有限）：
```cpp
// C++20 之前需要手动链接
void cppChainingOld() {
  auto fetchUserId = []() -> std::future<int> {
    return std::async([]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      return 123;
    });
  };
  
  auto fetchUserName = [](int userId) -> std::future<std::string> {
    return std::async([userId]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      return "用户" + std::to_string(userId);
    });
  };
  
  // 手动链接 future
  std::future<int> idFuture = fetchUserId();
  
  // 需要手动处理异常传播
  try {
    int userId = idFuture.get();
    std::future<std::string> nameFuture = fetchUserName(userId);
    
    try {
      std::string userName = nameFuture.get();
      std::cout << "用户名: " << userName << std::endl;
    } catch (const std::exception& e) {
      std::cout << "获取用户名失败: " << e.what() << std::endl;
    }
  } catch (const std::exception& e) {
    std::cout << "获取用户ID失败: " << e.what() << std::endl;
  }
}
```

### C++20 的 `then` 支持：
```cpp
// C++20 引入了 then（但支持有限）
#include <future>

void cpp20Then() {
  std::future<int> future = std::async([]() {
    return 42;
  });
  
  // C++20: then 方法
  auto nextFuture = future.then([](std::future<int> f) {
    try {
      int value = f.get();
      return value * 2;
    } catch (...) {
      return -1;
    }
  });
  
  // 还需要一个 .then 来处理字符串转换
  auto stringFuture = nextFuture.then([](std::future<int> f) {
    try {
      int value = f.get();
      return std::to_string(value);
    } catch (...) {
      return std::string("错误");
    }
  });
  
  // 最终获取
  try {
    std::string result = stringFuture.get();
    std::cout << "结果: " << result << std::endl;
  } catch (...) {
    std::cout << "最终失败" << std::endl;
  }
}
```

## 6. Future 组合器对比

### Dart 强大的组合器：
```dart
void dartCombinators() {
  Future<int> task1() async {
    await Future.delayed(Duration(seconds: 1));
    return 1;
  }
  
  Future<int> task2() async {
    await Future.delayed(Duration(seconds: 2));
    return 2;
  }
  
  Future<int> task3() async {
    await Future.delayed(Duration(seconds: 3));
    throw Exception('任务3失败');
  }
  
  // 1. Future.wait - 等待所有完成
  Future.wait([task1(), task2(), task3()])
    .then((List<int> results) {
      print('所有完成: $results');
    })
    .catchError((error) {
      print('有任务失败: $error');  // 只要有一个失败，就进入这里
    });
  
  // 2. Future.any - 第一个完成的
  Future.any([task1(), task2(), task3()])
    .then((value) {
      print('第一个完成的值: $value');
    })
    .catchError((error) {
      print('第一个完成的任务失败了: $error');
    });
  
  // 3. Future.doWhile - 循环执行
  int count = 0;
  Future.doWhile(() {
    count++;
    print('执行第 $count 次');
    return Future.delayed(Duration(seconds: 1))
      .then((_) => count < 3);  // 条件为 true 继续循环
  });
  
  // 4. Future.forEach - 为每个元素执行异步操作
  List<int> items = [1, 2, 3, 4, 5];
  Future.forEach(items, (int item) async {
    await Future.delayed(Duration(milliseconds: 100));
    print('处理: $item');
  });
  
  // 5. Future.microtask - 微任务队列
  Future.microtask(() {
    print('微任务执行');
  });
}
```

### C++ 的组合功能（C++20）：
```cpp
#include <future>
#include <vector>
#include <iostream>

void cppCombinators() {
  auto task1 = []() -> int {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 1;
  };
  
  auto task2 = []() -> int {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 2;
  };
  
  // C++20: when_all - 等待所有
  std::future<int> f1 = std::async(std::launch::async, task1);
  std::future<int> f2 = std::async(std::launch::async, task2);
  
  auto allFuture = std::when_all(f1.share(), f2.share());
  
  allFuture.then([](std::tuple<std::shared_future<int>, 
                               std::shared_future<int>> futures) {
    try {
      int result1 = std::get<0>(futures).get();
      int result2 = std::get<1>(futures).get();
      std::cout << "所有完成: " << result1 << ", " << result2 << std::endl;
    } catch (...) {
      std::cout << "有任务失败" << std::endl;
    }
  });
  
  // C++20: when_any - 第一个完成
  auto anyFuture = std::when_any(f1.share(), f2.share());
  
  anyFuture.then([](std::pair<std::size_t, 
                              std::tuple<std::shared_future<int>, 
                                         std::shared_future<int>>> result) {
    std::size_t index = result.first;
    auto& futures = result.second;
    
    try {
      if (index == 0) {
        int value = std::get<0>(futures).get();
        std::cout << "任务1先完成: " << value << std::endl;
      } else {
        int value = std::get<1>(futures).get();
        std::cout << "任务2先完成: " << value << std::endl;
      }
    } catch (...) {
      std::cout << "先完成的任务失败了" << std::endl;
    }
  });
}
```

## 7. 实际应用场景对比

### Dart - 网络请求链：
```dart
import 'dart:convert';
import 'package:http/http.dart' as http;

Future<void> dartHttpExample() async {
  // 1. 获取用户ID
  Future<int> getUserId() async {
    var response = await http.get(Uri.parse('https://api.example.com/user/id'));
    return jsonDecode(response.body)['id'];
  }
  
  // 2. 获取用户信息
  Future<Map<String, dynamic>> getUserInfo(int userId) async {
    var response = await http.get(
      Uri.parse('https://api.example.com/user/$userId')
    );
    return jsonDecode(response.body);
  }
  
  // 3. 获取用户的朋友列表
  Future<List<String>> getUserFriends(int userId) async {
    var response = await http.get(
      Uri.parse('https://api.example.com/user/$userId/friends')
    );
    List<dynamic> data = jsonDecode(response.body);
    return data.map((item) => item['name'] as String).toList();
  }
  
  // 优雅的链式调用
  try {
    int userId = await getUserId();
    
    // 并行执行两个请求
    var userFuture = getUserInfo(userId);
    var friendsFuture = getUserFriends(userId);
    
    // 等待两者都完成
    var results = await Future.wait([userFuture, friendsFuture]);
    
    var userInfo = results[0] as Map<String, dynamic>;
    var friends = results[1] as List<String>;
    
    print('用户: ${userInfo['name']}');
    print('朋友数: ${friends.length}');
  } catch (e) {
    print('请求失败: $e');
  }
}
```

### C++ - 并行计算：
```cpp
#include <future>
#include <vector>
#include <iostream>
#include <numeric>
#include <cmath>

void cppParallelExample() {
  // 并行计算质数
  auto isPrime = [](int n) -> bool {
    if (n <= 1) return false;
    for (int i = 2; i <= std::sqrt(n); i++) {
      if (n % i == 0) return false;
    }
    return true;
  };
  
  // 并行计算范围内的质数数量
  auto countPrimes = [&isPrime](int start, int end) -> int {
    int count = 0;
    for (int i = start; i <= end; i++) {
      if (isPrime(i)) count++;
    }
    return count;
  };
  
  const int totalNumbers = 1000000;
  const int threadCount = 4;
  const int chunkSize = totalNumbers / threadCount;
  
  std::vector<std::future<int>> futures;
  
  // 启动多个并行任务
  for (int i = 0; i < threadCount; i++) {
    int start = i * chunkSize;
    int end = (i == threadCount - 1) ? totalNumbers : (i + 1) * chunkSize - 1;
    
    futures.push_back(std::async(std::launch::async, countPrimes, start, end));
  }
  
  // 收集结果
  int totalPrimes = 0;
  for (auto& future : futures) {
    try {
      totalPrimes += future.get();  // 阻塞等待每个结果
    } catch (const std::exception& e) {
      std::cout << "计算错误: " << e.what() << std::endl;
    }
  }
  
  std::cout << "1到" << totalNumbers << "之间的质数数量: " 
            << totalPrimes << std::endl;
  
  // 更高级：使用 reduce 模式
  std::vector<int> numbers(totalNumbers);
  std::iota(numbers.begin(), numbers.end(), 1);
  
  // 并行累加
  auto parallelSum = [&]() -> int {
    std::vector<std::future<int>> sumFutures;
    
    for (int i = 0; i < threadCount; i++) {
      int start = i * chunkSize;
      int end = (i == threadCount - 1) ? totalNumbers : (i + 1) * chunkSize;
      
      sumFutures.push_back(std::async(std::launch::async, [&, start, end]() {
        int sum = 0;
        for (int j = start; j < end; j++) {
          sum += numbers[j];
        }
        return sum;
      }));
    }
    
    int total = 0;
    for (auto& f : sumFutures) {
      total += f.get();
    }
    return total;
  };
  
  std::cout << "并行求和: " << parallelSum() << std::endl;
}
```

## 8. 性能与内存对比

### Dart `Future` 性能特点：
```dart
void dartPerformance() {
  // Dart Future 基于事件循环，适合 I/O 密集型
  
  // 1. 微任务 vs 宏任务
  print('开始');
  
  Future.microtask(() => print('微任务1'));
  Future(() => print('宏任务1'));
  
  Future.microtask(() => print('微任务2'));
  Future(() => print('宏任务2'));
  
  print('结束');
  // 输出顺序: 开始, 结束, 微任务1, 微任务2, 宏任务1, 宏任务2
  
  // 2. 避免 Future 嵌套过深
  // 不好：嵌套 Future
  Future(() {
    return Future(() {
      return Future(() => 42);
    });
  });
  
  // 好：扁平化
  Future(() => 42)
    .then((value) => value * 2)
    .then((value) => value.toString());
  
  // 3. 使用 Completer 手动控制
  Completer<int> completer = Completer();
  
  // 稍后完成
  Timer(Duration(seconds: 1), () {
    completer.complete(100);
    // 或 completer.completeError(Exception('错误'));
  });
  
  completer.future.then(print);
}
```

### C++ `std::future` 性能特点：
```cpp
#include <future>
#include <chrono>
#include <iostream>

void cppPerformance() {
  // 1. 启动策略
  auto start = std::chrono::high_resolution_clock::now();
  
  // std::launch::async - 立即启动新线程
  auto f1 = std::async(std::launch::async, []() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 1;
  });
  
  // std::launch::deferred - 延迟，调用 get() 时执行
  auto f2 = std::async(std::launch::deferred, []() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 2;
  });
  
  // std::launch::async | std::launch::deferred - 由实现决定
  auto f3 = std::async(std::launch::async | std::launch::deferred, []() {
    return 3;
  });
  
  // 2. 测量性能
  int result1 = f1.get();  // 已经异步执行
  
  auto mid = std::chrono::high_resolution_clock::now();
  auto elapsed1 = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
  std::cout << "f1 耗时: " << elapsed1.count() << "ms" << std::endl;
  
  int result2 = f2.get();  // 现在才开始执行
  
  auto end = std::chrono::high_resolution_clock::now();
  auto elapsed2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid);
  std::cout << "f2 耗时: " << elapsed2.count() << "ms" << std::endl;
  
  // 3. 线程开销考虑
  // 小任务可能不值得创建线程
  auto smallTask = []() { return 42; };
  
  // 不好的做法：小任务也异步
  for (int i = 0; i < 1000; i++) {
    auto f = std::async(std::launch::async, smallTask);  // 线程创建开销大
    f.get();
  }
  
  // 好的做法：批量处理或使用线程池
}
```

## 9. 错误处理模式对比

### Dart 的错误处理模式：
```dart
Future<void> dartErrorPatterns() {
  // 模式1: 链式错误处理
  Future<int> riskyOperation() async {
    if (Random().nextBool()) {
      throw Exception('随机失败');
    }
    return 42;
  }
  
  riskyOperation()
    .then((value) => print('成功: $value'))
    .catchError((e) => print('捕获: $e'), test: (e) => e is FormatException)
    .catchError((e) => print('其他错误: $e'));
  
  // 模式2: 错误恢复
  Future<int> fetchWithRetry(int retries) async {
    for (int i = 0; i < retries; i++) {
      try {
        return await riskyOperation();
      } catch (e) {
        print('第 ${i + 1} 次重试失败');
        if (i == retries - 1) rethrow;
        await Future.delayed(Duration(seconds: 1));
      }
    }
    throw StateError('不应到达此处');
  }
  
  // 模式3: 超时处理
  Future<int> fetchWithTimeout() async {
    return await riskyOperation()
      .timeout(Duration(seconds: 2), onTimeout: () {
        print('超时，返回默认值');
        return -1;
      });
  }
  
  // 模式4: 多个 Future 的错误处理
  Future.wait([
    riskyOperation(),
    riskyOperation(),
    riskyOperation(),
  ], eagerError: true)  // 第一个错误就立即返回
    .then(print)
    .catchError((e) => print('其中一个失败: $e'));
  
  return Future.value();
}
```

### C++ 的错误处理模式：
```cpp
#include <future>
#include <stdexcept>
#include <iostream>

void cppErrorPatterns() {
  // 模式1: 异常传播
  auto riskyOp = []() -> int {
    if (rand() % 2 == 0) {
      throw std::runtime_error("随机失败");
    }
    return 42;
  };
  
  std::future<int> f = std::async(std::launch::async, riskyOp);
  
  try {
    int result = f.get();
    std::cout << "成功: " << result << std::endl;
  } catch (const std::runtime_error& e) {
    std::cout << "运行时错误: " << e.what() << std::endl;
  } catch (const std::exception& e) {
    std::cout << "标准异常: " << e.what() << std::endl;
  } catch (...) {
    std::cout << "未知异常" << std::endl;
  }
  
  // 模式2: 超时处理
  std::future<int> f2 = std::async(std::launch::async, []() {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 100;
  });
  
  auto status = f2.wait_for(std::chrono::seconds(2));
  if (status == std::future_status::ready) {
    try {
      int value = f2.get();
      std::cout << "及时完成: " << value << std::endl;
    } catch (...) {
      std::cout << "任务异常" << std::endl;
    }
  } else {
    std::cout << "超时，任务取消中..." << std::endl;
    // C++ 没有直接取消 future 的方法
  }
  
  // 模式3: 带错误码的 future（避免异常）
  std::promise<std::pair<bool, int>> promise;
  std::future<std::pair<bool, int>> f3 = promise.get_future();
  
  std::thread([&promise]() {
    try {
      int value = riskyOp();
      promise.set_value({true, value});  // 成功
    } catch (...) {
      promise.set_value({false, -1});    // 失败
    }
  }).detach();
  
  auto [success, value] = f3.get();
  if (success) {
    std::cout << "成功，值: " << value << std::endl;
  } else {
    std::cout << "失败" << std::endl;
  }
}
```

## 10. 总结对比表

| 特性         | Dart `Future`    | C++ `std::future`           | 推荐场景                      |
| ------------ | ---------------- | --------------------------- | ----------------------------- |
| **执行模型** | 单线程事件循环   | 多线程并行                  | Dart: I/O 操作; C++: CPU 计算 |
| **创建方式** | 多种工厂方法     | async/promise/packaged_task | Dart 更简洁                   |
| **链式调用** | ✅ 原生优雅支持   | ⚠️ C++20 有限支持            | Dart 更适合复杂异步流         |
| **错误处理** | 链式 catchError  | try-catch on get()          | 两者都有效                    |
| **组合能力** | 丰富组合器       | C++20 when_all/any          | Dart 更强大                   |
| **取消支持** | ❌ 无内置         | ❌ 无内置                    | 两者都需要手动实现            |
| **超时处理** | ✅ timeout()      | ✅ wait_for()                | 两者都支持                    |
| **性能特点** | 低开销，适合 I/O | 高开销，适合计算            | 根据任务类型选择              |
| **内存安全** | 自动 GC          | 手动/RAII                   | Dart 更安全                   |
| **学习曲线** | 较低             | 较高                        | Dart 更易上手                 |

## 关键结论

1. **设计哲学不同**：
   - Dart `Future`：为单线程事件循环设计，强调非阻塞 I/O
   - C++ `std::future`：为多线程并行计算设计，强调 CPU 利用率

2. **使用场景不同**：
   - 用 Dart `Future` 处理网络请求、文件 I/O、UI 更新
   - 用 C++ `std::future` 处理数学计算、图像处理、数据分析

3. **开发体验不同**：
   - Dart 的 `async/await` 语法让异步代码像同步一样易读
   - C++ 需要更多样板代码，但 C++20 有改进

4. **选择建议**：
   - Flutter/前端开发：Dart `Future`
   - 高性能计算/游戏引擎：C++ `std::future`
   - 混合场景：可能需要两者结合（如 Dart 调用 C++ 计算）

两者都是优秀的异步编程工具，选择哪个取决于你的具体需求和技术栈。

## async/await和C#对比

●介绍：除了通过then/catchError的方式，还可以通过async/await来实现异步编程
●特点：await总是等到后面的Future执行成功，才执行下方逻辑，async必须配套await出现

# Dart 与 C# 的 async/await 对比

**Dart 和 C# 的 async/await 非常相似**，因为 Dart 的设计深受 C# 影响。两者都提供了相似的语法，但在实现细节和运行时行为上有一些重要区别。

## 1. 语法相似性对比

### 1.1 **基本语法几乎相同**

**Dart:**
```dart
Future<String> fetchData() async {
  await Future.delayed(Duration(seconds: 1));
  return 'Dart 数据';
}

void main() async {
  print('开始');
  var data = await fetchData();
  print('获取到: $data');
}
```

**C#:**
```csharp
async Task<string> FetchDataAsync()
{
    await Task.Delay(1000);
    return "C# 数据";
}

static async Task Main()
{
    Console.WriteLine("开始");
    var data = await FetchDataAsync();
    Console.WriteLine($"获取到: {data}");
}
```

### 1.2 **异步方法的返回类型**

| 返回类型     | Dart           | C#                    | 说明     |
| ------------ | -------------- | --------------------- | -------- |
| **有返回值** | `Future<T>`    | `Task<T>`             | 对应关系 |
| **无返回值** | `Future<void>` | `Task`                | 对应关系 |
| **同步值**   | `T`            | `T` 或 `ValueTask<T>` | 特殊情况 |

## 2. 核心相同点

### 2.1 **语法糖本质**
两者都是编译器语法糖，将异步代码转换为状态机：

```dart
// Dart - 编译器转换示例（概念上）
Future<int> compute() async {
  var a = await getA();
  var b = await getB();
  return a + b;
}

// 被转换为类似：
Future<int> compute() {
  return getA().then((a) {
    return getB().then((b) {
      return a + b;
    });
  });
}
```

```csharp
// C# - 类似转换
async Task<int> ComputeAsync()
{
    var a = await GetAAsync();
    var b = await GetBAsync();
    return a + b;
}

// 被转换为状态机类
```

### 2.2 **异常处理机制相同**
```dart
// Dart
Future<int> riskyOperation() async {
  try {
    var result = await fetchData();
    return result;
  } catch (e) {
    print('Dart 错误: $e');
    return -1;
  } finally {
    print('Dart 清理');
  }
}
```

```csharp
// C#
async Task<int> RiskyOperationAsync()
{
    try
    {
        var result = await FetchDataAsync();
        return result;
    }
    catch (Exception e)
    {
        Console.WriteLine($"C# 错误: {e}");
        return -1;
    }
    finally
    {
        Console.WriteLine("C# 清理");
    }
}
```

### 2.3 **支持多个 await**
```dart
// Dart
Future<List<String>> fetchAll() async {
  var user = await fetchUser();
  var posts = await fetchPosts(user.id);
  var comments = await fetchComments(posts.first.id);
  return [user.name, posts.first.title, comments.first.text];
}
```

```csharp
// C#
async Task<List<string>> FetchAllAsync()
{
    var user = await FetchUserAsync();
    var posts = await FetchPostsAsync(user.Id);
    var comments = await FetchCommentsAsync(posts.First().Id);
    return new List<string> { user.Name, posts.First().Title, comments.First().Text };
}
```

## 3. 关键不同点

### 3.1 **执行模型和线程差异**

**Dart - 单线程事件循环：**
```dart
import 'dart:async';

void dartExecutionModel() async {
  print('Dart 开始');
  
  // Dart 在单线程中运行
  print('线程: ${Isolate.current.hashCode}');
  
  // async/await 不创建新线程
  await Future.delayed(Duration(seconds: 1));
  
  print('仍在同一线程: ${Isolate.current.hashCode}');
  
  // 微任务队列 vs 事件队列
  scheduleMicrotask(() => print('微任务'));
  
  Future(() => print('事件队列任务'));
  
  print('结束');
  // 输出顺序: 开始, 线程..., 结束, 微任务, 事件队列任务
}
```

**C# - 多线程执行：**
```csharp
using System;
using System.Threading;
using System.Threading.Tasks;

class CSharpExecutionModel
{
    static async Task Main()
    {
        Console.WriteLine("C# 开始");
        
        // C# 可能在不同线程上恢复
        Console.WriteLine($"线程1: {Thread.CurrentThread.ManagedThreadId}");
        
        await Task.Delay(1000);
        
        // 可能在不同线程上恢复（除非有 SynchronizationContext）
        Console.WriteLine($"线程2: {Thread.CurrentThread.ManagedThreadId}");
        
        // ConfigureAwait(false) 允许在线程池线程恢复
        await Task.Delay(1000).ConfigureAwait(false);
        Console.WriteLine($"线程3: {Thread.CurrentThread.ManagedThreadId}");
        
        // ConfigureAwait(true) 尝试在原始上下文恢复
        await Task.Delay(1000).ConfigureAwait(true);
        Console.WriteLine($"线程4: {Thread.CurrentThread.ManagedThreadId}");
    }
}
```

### 3.2 **取消机制对比**

**Dart - 没有内置取消令牌：**
```dart
import 'dart:async';

class CancellationToken {
  bool _cancelled = false;
  bool get isCancelled => _cancelled;
  
  void cancel() => _cancelled = true;
}

Future<String> fetchWithCancellation(CancellationToken token) async {
  for (int i = 0; i < 10; i++) {
    // 手动检查取消
    if (token.isCancelled) {
      throw CancelledException();
    }
    
    await Future.delayed(Duration(milliseconds: 100));
  }
  return "完成";
}

// 使用 Completer 模拟取消
Future<String> cancellableOperation() {
  var completer = Completer<String>();
  var timer = Timer(Duration(seconds: 2), () {
    if (!completer.isCompleted) {
      completer.complete("超时完成");
    }
  });
  
  // 模拟操作
  Future.delayed(Duration(seconds: 5)).then((_) {
    if (!completer.isCompleted) {
      completer.complete("操作完成");
    }
  });
  
  return completer.future;
}
```

**C# - 内置 CancellationToken：**
```csharp
using System;
using System.Threading;
using System.Threading.Tasks;

class CSharpCancellation
{
    static async Task<string> FetchWithCancellationAsync(CancellationToken cancellationToken)
    {
        for (int i = 0; i < 10; i++)
        {
            // 检查取消请求
            cancellationToken.ThrowIfCancellationRequested();
            
            await Task.Delay(100, cancellationToken);
        }
        return "完成";
    }
    
    static async Task Main()
    {
        var cts = new CancellationTokenSource();
        
        // 3秒后取消
        cts.CancelAfter(3000);
        
        try
        {
            var result = await FetchWithCancellationAsync(cts.Token);
            Console.WriteLine(result);
        }
        catch (OperationCanceledException)
        {
            Console.WriteLine("操作被取消");
        }
    }
}
```

### 3.3 **值任务（ValueTask）对比**

**C# 有 ValueTask 优化：**
```csharp
// C# - ValueTask 避免堆分配
async ValueTask<int> ComputeAsync()
{
    // 如果结果立即可得，避免 Task 分配
    if (cache.TryGetValue(key, out var value))
    {
        return value;  // 没有异步操作，直接返回值
    }
    
    // 需要异步操作时才创建 Task
    return await FetchFromNetworkAsync();
}
```

**Dart 只有 Future：**
```dart
// Dart - 总是返回 Future，没有 ValueTask 等价物
Future<int> compute() async {
  // 即使结果立即可得，也返回 Future
  if (cache.containsKey(key)) {
    return cache[key]!;  // 仍然返回 Future<int>
  }
  
  return await fetchFromNetwork();
}

// Dart 优化：使用 Future.value 避免额外包装
Future<int> computeOptimized() async {
  if (cache.containsKey(key)) {
    return Future.value(cache[key]!);  // 直接返回已完成的 Future
  }
  
  return await fetchFromNetwork();
}
```

### 3.4 **异步迭代器语法**

**Dart - Stream 和 async*：**
```dart
// Dart 异步生成器
Stream<int> countAsync(int max) async* {
  for (int i = 0; i < max; i++) {
    await Future.delayed(Duration(milliseconds: 100));
    yield i;  // 产生值到 Stream
  }
}

// 使用 await for 消费
void main() async {
  await for (var number in countAsync(5)) {
    print('Dart: $number');
  }
}
```

**C# - IAsyncEnumerable：**
```csharp
// C# 异步枚举
async IAsyncEnumerable<int> CountAsync(int max)
{
    for (int i = 0; i < max; i++)
    {
        await Task.Delay(100);
        yield return i;  // 产生值到 IAsyncEnumerable
    }
}

// 使用 await foreach 消费
static async Task Main()
{
    await foreach (var number in CountAsync(5))
    {
        Console.WriteLine($"C#: {number}");
    }
}
```

### 3.5 **同步上下文（SynchronizationContext）**

**C# 有同步上下文概念：**
```csharp
// C# 同步上下文控制恢复位置
async Task UpdateUIAsync()
{
    // 默认情况下，await 后尝试在 UI 线程恢复
    var data = await FetchDataAsync();  // 在后台线程执行
    
    // 这里会在 UI 线程执行（如果有 SynchronizationContext）
    label.Text = data;
    
    // 使用 ConfigureAwait(false) 避免回到 UI 线程
    var moreData = await FetchMoreDataAsync().ConfigureAwait(false);
    // 这里在线程池线程执行
    
    // 需要手动切换回 UI 线程
    Dispatcher.Invoke(() => label.Text = moreData);
}
```

**Dart 没有同步上下文，但有类似机制：**
```dart
// Dart - 使用 scheduleTask 在特定 zone 执行
import 'dart:async';

void updateUI() async {
  var data = await fetchData();  // 在事件循环中执行
  
  // 在 Flutter 中，WidgetsBinding 提供类似功能
  WidgetsBinding.instance.addPostFrameCallback((_) {
    // 确保在下一帧执行（类似 UI 线程）
    setState(() {
      _data = data;
    });
  });
  
  // 或者使用 runOnUiThread（Flutter 插件）
  // 在原生插件开发中可能需要
}
```

## 4. 性能优化对比

### 4.1 **热路径优化**

**Dart - 手动优化：**
```dart
Future<int> computeHotPath() async {
  // 情况1：经常命中缓存
  if (_cache != null) {
    return _cache!;  // 同步返回，但仍然是 Future
  }
  
  // 情况2：需要计算
  return _computeAsync();
  
  // Dart 没有 ValueTask，但编译器会优化常见的 async/await 模式
}

// 避免不必要的 async/await
Future<int> computeWithoutAsync() {
  // 如果不使用 await，不需要标记为 async
  if (_cache != null) {
    return Future.value(_cache!);
  }
  return _computeAsync();
}
```

**C# - 自动和手动优化：**
```csharp
// C# 编译器优化和手动优化
async ValueTask<int> ComputeHotPathAsync()
{
    // 使用 ValueTask 避免堆分配
    if (_cache.TryGetValue(out var result))
    {
        return result;  // 同步返回 ValueTask<int>
    }
    
    // 需要异步时才使用 async
    return await ComputeAsync();
}

// C# 编译器会将某些 async 方法优化为同步路径
```

### 4.2 **状态机开销**

**Dart 状态机特点：**
```dart
// Dart 状态机相对轻量
Future<int> dartStateMachine() async {
  var a = 1;           // 状态0
  await step1();       // 状态1
  var b = a + 2;       // 状态2
  await step2();       // 状态3
  return b;            // 状态4
}

// Dart 状态机存储在堆上，有 GC 压力
```

**C# 状态机特点：**
```csharp
// C# 状态机更复杂但优化更好
async Task<int> CSharpStateMachineAsync()
{
    var a = 1;            // 状态0
    await Step1Async();   // 状态1
    var b = a + 2;        // 状态2
    await Step2Async();   // 状态3
    return b;             // 状态4
}

// C# 状态机是 struct，可能分配在栈上，减少 GC 压力
// 但 async Task 方法的状态机最终会分配到堆上
// async ValueTask 方法的状态机可能完全在栈上
```

## 5. 实际应用场景对比

### 5.1 **网络请求**

**Dart (使用 http 包):**
```dart
import 'package:http/http.dart' as http;

Future<User> fetchUser(int id) async {
  try {
    final response = await http.get(
      Uri.parse('https://api.example.com/users/$id'),
    );
    
    if (response.statusCode == 200) {
      return User.fromJson(jsonDecode(response.body));
    } else {
      throw HttpException('请求失败: ${response.statusCode}');
    }
  } on SocketException {
    throw NoNetworkException();
  } on TimeoutException {
    throw TimeoutException('请求超时');
  }
}
```

**C# (使用 HttpClient):**
```csharp
using System.Net.Http;
using System.Text.Json;

public async Task<User> FetchUserAsync(int id, CancellationToken cancellationToken = default)
{
    try
    {
        using var response = await _httpClient.GetAsync(
            $"https://api.example.com/users/{id}", 
            cancellationToken
        );
        
        response.EnsureSuccessStatusCode();
        
        var json = await response.Content.ReadAsStringAsync(cancellationToken);
        return JsonSerializer.Deserialize<User>(json);
    }
    catch (HttpRequestException ex) when (ex.StatusCode == HttpStatusCode.NotFound)
    {
        throw new UserNotFoundException(id);
    }
    catch (TaskCanceledException) when (!cancellationToken.IsCancellationRequested)
    {
        throw new TimeoutException("请求超时");
    }
    catch (OperationCanceledException)
    {
        throw;  // 传播取消异常
    }
}
```

### 5.2 **数据库访问**

**Dart (使用 sqflite):**
```dart
import 'package:sqflite/sqflite.dart';

Future<List<User>> getUsers() async {
  final db = await openDatabase('my_db.db');
  
  try {
    final List<Map<String, dynamic>> maps = await db.query('users');
    return List.generate(maps.length, (i) {
      return User(
        id: maps[i]['id'],
        name: maps[i]['name'],
        email: maps[i]['email'],
      );
    });
  } finally {
    await db.close();
  }
}
```

**C# (使用 Dapper):**
```csharp
using Dapper;
using System.Data.SqlClient;

public async Task<List<User>> GetUsersAsync(CancellationToken cancellationToken = default)
{
    await using var connection = new SqlConnection(_connectionString);
    await connection.OpenAsync(cancellationToken);
    
    var users = await connection.QueryAsync<User>(
        "SELECT * FROM Users",
        cancellationToken: cancellationToken
    );
    
    return users.AsList();
}

// 使用异步事务
public async Task TransferAsync(int fromId, int toId, decimal amount)
{
    await using var connection = new SqlConnection(_connectionString);
    await connection.OpenAsync();
    
    await using var transaction = await connection.BeginTransactionAsync();
    
    try
    {
        await connection.ExecuteAsync(
            "UPDATE Accounts SET Balance = Balance - @Amount WHERE Id = @Id",
            new { Amount = amount, Id = fromId },
            transaction: transaction
        );
        
        await connection.ExecuteAsync(
            "UPDATE Accounts SET Balance = Balance + @Amount WHERE Id = @Id",
            new { Amount = amount, Id = toId },
            transaction: transaction
        );
        
        await transaction.CommitAsync();
    }
    catch
    {
        await transaction.RollbackAsync();
        throw;
    }
}
```

### 5.3 **并行处理**

**Dart - 使用 Future.wait:**
```dart
Future<Map<String, dynamic>> fetchAllData() async {
  // 并行启动多个请求
  var userFuture = fetchUser();
  var postsFuture = fetchPosts();
  var commentsFuture = fetchComments();
  
  // 等待所有完成
  var results = await Future.wait([
    userFuture,
    postsFuture,
    commentsFuture,
  ]);
  
  return {
    'user': results[0],
    'posts': results[1],
    'comments': results[2],
  };
}
```

**C# - 使用 Task.WhenAll:**
```csharp
public async Task<AllData> FetchAllDataAsync(CancellationToken cancellationToken = default)
{
    // 并行启动多个请求
    var userTask = FetchUserAsync(cancellationToken);
    var postsTask = FetchPostsAsync(cancellationToken);
    var commentsTask = FetchCommentsAsync(cancellationToken);
    
    // 等待所有完成
    await Task.WhenAll(userTask, postsTask, commentsTask);
    
    return new AllData
    {
        User = await userTask,        // 结果已就绪，不会等待
        Posts = await postsTask,
        Comments = await commentsTask,
    };
}
```

## 6. 错误模式对比

### 6.1 **死锁风险**

**C# 容易发生死锁（在 UI 线程上同步等待）：**
```csharp
// C# 错误示例：死锁！
async Task<string> GetDataAsync()
{
    await Task.Delay(1000);
    return "数据";
}

void Button_Click(object sender, EventArgs e)
{
    // 在 UI 线程上同步等待异步方法 -> 死锁！
    var data = GetDataAsync().Result;  // 不要这样做！
    
    // 正确做法：使用 async void 事件处理程序
    // async void Button_ClickAsync(object sender, EventArgs e)
    // {
    //     var data = await GetDataAsync();
    // }
}
```

**Dart 不容易死锁（单线程事件循环）：**
```dart
// Dart 没有相同死锁问题，但可能阻塞事件循环
Future<String> getData() async {
  await Future.delayed(Duration(seconds: 1));
  return '数据';
}

void buttonClick() {
  // 在 Dart 中，这会阻塞事件循环，但不会死锁
  var data = getData().then((value) {
    print(value);
  });
  
  // 或者使用 async 函数
  // void buttonClick() async {
  //   var data = await getData();
  //   print(data);
  // }
}
```

### 6.2 **异常传播差异**

**Dart 异常传播：**
```dart
Future<void> dartExceptionPropagation() async {
  try {
    await throwAsync();
  } catch (e) {
    print('捕获: $e');  // 会捕获到
  }
}

Future<void> throwAsync() async {
  throw Exception('Dart 异常');
}

// 未捕获的异常会成为 Future 的错误
void unhandledException() {
  throwAsync().then((_) {
    print('成功');  // 不会执行
  }).catchError((e) {
    print('捕获: $e');  // 必须显式处理
  });
}
```

**C# 异常传播：**
```csharp
async Task CSharpExceptionPropagationAsync()
{
    try
    {
        await ThrowAsync();
    }
    catch (Exception ex)
    {
        Console.WriteLine($"捕获: {ex}");  // 会捕获到
    }
}

async Task ThrowAsync()
{
    throw new InvalidOperationException("C# 异常");
}

// 未观察到的异常可能被忽略（.NET 4.0 之前）
// 现代 .NET 中，未观察到的 Task 异常会触发 TaskScheduler.UnobservedTaskException
```

## 7. 调试和诊断对比

### 7.1 **调试体验**

**Dart 调试：**
```dart
// Dart DevTools 提供异步调试支持
Future<void> debugDartAsync() async {
  print('步骤1');
  await Future.delayed(Duration(milliseconds: 100));
  
  // 在 DevTools 中可以看到异步调用栈
  print('步骤2');
  await nestedAsync();
}

Future<void> nestedAsync() async {
  await Future.delayed(Duration(milliseconds: 50));
  throw Exception('调试异常');  // 调用栈会显示 async 层次
}
```

**C# 调试：**
```csharp
// Visual Studio 提供异步调试工具
async Task DebugCSharpAsync()
{
    Console.WriteLine("步骤1");
    await Task.Delay(100);
    
    // Visual Studio 的 Parallel Stacks 窗口显示异步调用
    Console.WriteLine("步骤2");
    await NestedAsync();
}

async Task NestedAsync()
{
    await Task.Delay(50);
    throw new InvalidOperationException("调试异常");
    // 调用栈显示状态机信息
}
```

### 7.2 **性能分析**

**Dart 分析：**
```dart
import 'dart:developer';

Future<void> profileDartAsync() async {
  Timeline.startSync('异步操作');
  
  try {
    await expensiveAsyncOperation();
    await anotherAsyncOperation();
  } finally {
    Timeline.finishSync();
  }
}
```

**C# 分析：**
```csharp
using System.Diagnostics;

async Task ProfileCSharpAsync()
{
    var stopwatch = Stopwatch.StartNew();
    
    await ExpensiveAsyncOperation();
    await AnotherAsyncOperation();
    
    stopwatch.Stop();
    Console.WriteLine($"耗时: {stopwatch.ElapsedMilliseconds}ms");
    
    // 使用 Visual Studio Profiler 分析异步性能
}
```

## 8. 总结对比表

| 特性           | Dart async/await                  | C# async/await             | 说明               |
| -------------- | --------------------------------- | -------------------------- | ------------------ |
| **语法相似度** | 95% 相同                          | 参考实现                   | 学习成本极低       |
| **执行模型**   | 单线程事件循环                    | 多线程线程池               | 根本区别           |
| **返回类型**   | `Future<T>`                       | `Task<T>` / `ValueTask<T>` | C# 更优化          |
| **取消支持**   | 无内置，需手动                    | `CancellationToken` 内置   | C# 更完善          |
| **同步上下文** | 无，但有 Zone                     | `SynchronizationContext`   | C# 更精细控制      |
| **异步迭代器** | `Stream` + `async*`               | `IAsyncEnumerable<T>`      | 概念相同，语法不同 |
| **错误处理**   | `try/catch` + `Future.catchError` | `try/catch`                | 基本相同           |
| **状态机分配** | 堆分配                            | 可能栈分配（ValueTask）    | C# 性能更好        |
| **死锁风险**   | 低（单线程）                      | 高（需注意）               | C# 更易出错        |
| **调试支持**   | Dart DevTools                     | Visual Studio 强大工具     | 都很好             |
| **性能优化**   | 编译器优化                        | 编译器 + JIT 优化          | C# 更成熟          |

## 关键结论

1. **语法几乎相同**：如果你会 C# 的 async/await，Dart 的很容易上手
2. **执行模型不同**：Dart 单线程 vs C# 多线程，这是最大区别
3. **C# 功能更丰富**：CancellationToken、ValueTask、ConfigureAwait 等
4. **Dart 更简单**：单线程模型减少了并发复杂性
5. **性能考虑**：C# 有更多优化手段，但 Dart 对移动端更友好

**迁移建议：**
- 从 C# 到 Dart：注意取消机制和线程模型的差异
- 从 Dart 到 C#：学习 CancellationToken 和 ConfigureAwait 的使用
- 两者都避免同步等待异步方法（`.Result` 或 `.then` 阻塞）

两种语言的 async/await 都是优秀的异步编程模型，选择哪个主要取决于你的技术栈和目标平台。
