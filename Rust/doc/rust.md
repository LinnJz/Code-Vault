# Rust 官方资源

- **中文版 (Rust 参考手册)**：[`https://rustwiki.org/zh-CN/reference/attributes.html`](https://rustwiki.org/zh-CN/reference/attributes.html)
- **英文版 (Rust Reference)**：[`https://doc.rust-lang.org/reference/attributes.html`](https://doc.rust-lang.org/reference/attributes.html)

# 属性

> Rust的所有属性大致可以分为四大类：**内建属性、宏属性、派生宏辅助属性和外部工具属性**。想查阅最全的列表，最好是直接参考官方文档。
>

属性按功能可以分为以下几大类：

- **条件编译 (`cfg`, `cfg_attr`)**：用于控制代码是否参与编译。
- **Lint 检查 (`allow`, `warn`, `deny`, `forbid`)**：用于控制编译器警告和错误的级别。
- **Crate 相关 (`crate_type`, `crate_name`, `no_std` 等)**：用于配置整个 crate 的元数据和行为。
- **函数与模块 (`test`, `bench`, `inline`, `cold` 等)**：用于单元测试、基准测试和函数优化。
- **代码生成与派生 (`derive`, `automatically_derived`, `repr` 等)**：用于自动实现 trait 和指定类型的内存布局。
- **宏 (`macro_export`, `macro_use`, `proc_macro` 等)**：用于定义和使用各种宏。
- **外部语言接口 (`link`, `link_name`, `no_mangle` 等)**：用于与外部非 Rust 代码进行交互。
- **诊断与弃用 (`must_use`, `deprecated`, `doc`)**：用于提供编译器诊断信息。
- **宏属性（或称为属性宏）**：可以自定义的、类似函数的属性。
- **派生宏辅助属性**：用于结构体或枚举的字段，为派生宏提供额外数据。
- **外部工具属性**：由 `rustfmt`、`clippy` 等外部工具识别和使用的属性。

> **补充说明**：`#[foo]` 是外部属性，用于其后的项；`#![foo]` 是内部属性，作用于其所在的项。

# 生命周期

生命周期说明符（Lifetime Specifier）是 Rust 引用系统中最核心、也最独特的部分。它的主要作用是**告诉编译器“这个引用能活多久”**，以便在编译时确保所有引用在使用时都指向有效数据，避免出现悬垂引用（dangling reference）。

生命周期说明符以**撇号 `'` 开头**，通常是一个小写字母或下划线，例如 `'a`、`'b`、`'static` 等。它们本身不改变引用的存活时间，只是**用于建立引用之间、以及引用与数据所有者之间的存活关系（约束）**。

下面从几个方面详细介绍。

---

## 1. 生命周期说明符的语法位置

生命周期参数通常出现在**泛型参数列表**中，并在引用类型中使用：

```rust
fn longest<'a>(x: &'a str, y: &'a str) -> &'a str {
    if x.len() > y.len() { x } else { y }
}
```

这里 `'a` 是一个生命周期参数，它表示：`x`、`y` 以及返回值都必须活得至少和 `'a` 一样长，并且三者的实际存活时间取交集。

---

## 2. 常见的生命周期说明符

### 2.1 普通生命周期参数（`'a`, `'b`, …）

- 没有内在含义，仅用于**建立约束**。
- 可以同时使用多个生命周期参数，建立更复杂的关系（如 `'a: 'b` 表示 `'a` 比 `'b` 活得长）。

### 2.2 特殊生命周期 `'static`

- `'static` 表示**整个程序运行期间**都有效。
- 它有两种常见情况：
  1. **字符串字面量**（如 `let s: &'static str = "Hello";`），其数据被硬编码在可执行文件的只读段中。
  2. **通过 `Box::leak` 或 `Vec::leak` 等泄漏出来的引用**，它们的生命周期也是 `'static`。

> 注意：`'static` 并不意味着值真的永远存在，而是指**该引用所指向的数据拥有静态生命周期**（例如全局变量、字面量、泄漏的内存）。

---

## 3. 生命周期说明符的作用范围

生命周期标注本身**不会延长变量的寿命**，它只是告诉编译器：

- “这个引用必须存活至少某个范围”。
- “这两个引用的存活范围存在某种约束关系”。

编译器在函数调用点会根据实际传入的引用，自动推断出具体的生命周期（通过**生命周期省略规则**减少手动标注）。

---

## 4. 生命周期省略规则

为了提高开发效率，Rust 编译器在特定情况下可以自动推断生命周期，无需手动标注。这叫做**生命周期省略规则**。

规则适用于函数和方法：

1. 每个**输入的引用**如果没有标注生命周期，则分配一个不同的泛型参数。
   - `fn foo(x: &i32)` → `fn foo<'a>(x: &'a i32)`
2. 如果只有一个输入引用，那么所有输出引用都得到该生命周期。
   - `fn foo(x: &i32) -> &i32` → `fn foo<'a>(x: &'a i32) -> &'a i32`
3. 如果有多个输入引用，但其中一个是 `&self` 或 `&mut self`（即方法），那么所有输出引用都得到 `self` 的生命周期。
   - `impl MyType { fn get(&self, x: &i32) -> &i32 }` → `get<'a, 'b>(&'a self, &'b i32) -> &'a i32`

不符合上述规则时，必须手动标注生命周期。

---

## 5. 结构体中的生命周期说明符

当结构体包含引用字段时，必须为每个引用字段标注生命周期，以确保结构体实例不会持有悬垂引用：

```rust
struct Book<'a> {
    title: &'a str,
    author: &'a str,
}
```

这意味着 `Book` 实例的存活时间不能超过 `title` 和 `author` 所指向的字符串数据的存活时间。

---

## 6. 生命周期约束与子类型

可以使用 `'a: 'b` 表示 `'a` 比 `'b` 活得长（`'a` 是 `'b` 的子类型）。例如：

```rust
fn foo<'a, 'b: 'a>(x: &'b i32, y: &'a i32) -> &'a i32 {
    y
}
```

这种约束常用于迭代器、容器等复杂数据结构。

---

## 7. 匿名生命周期 `'_`

Rust 2018 引入了匿名生命周期 `'_`，用于**推断生命周期**，避免为简单情况命名：

```rust
fn foo(x: &'_ i32) -> &'_ i32 { x }
// 等价于 fn foo(x: &i32) -> &i32
```

在结构体字段中也可以使用：

```rust
struct Book<'_> {
    title: &'_ str,
}
```

匿名生命周期会让编译器自动根据上下文推断，但不能在多个地方同时使用且需要约束关系时使用。

---

## 8. 常见误区澄清

- **生命周期并不是作用域**：虽然通常引用不会活出变量的作用域，但生命周期参数是更抽象的概念，它描述了**引用有效的时间段**（可能是几个作用域的并集或交集）。
- **生命周期标注不会改变代码逻辑**：它只是用于满足编译器的借用检查，运行时没有任何开销。
- **`'static` 不是万能药**：滥用 `'static` 会导致内存泄漏或不必要的长生命周期限制。

---

## 总结

| 概念               | 说明                                                 |
| ------------------ | ---------------------------------------------------- |
| **生命周期说明符** | `'a`, `'static`, `'_`，用来标注引用的存活时间关系    |
| **主要目的**       | 避免悬垂引用，保证内存安全                           |
| **使用位置**       | 函数签名、结构体定义、枚举定义、类型别名中的引用类型 |
| **省略规则**       | 常见情况可自动推断，减少手动标注                     |
| **特殊生命周期**   | `'static` 表示整个程序周期；`'_` 表示省略生命周期    |

# 多态

## 1. 静态多态

通过**泛型** + **trait bound** 实现。编译器在编译时会为每个具体类型生成单独的函数/结构体副本（**单态化**），调用在编译期确定，零运行时开销。

- **优点**：没有运行时开销，函数可以内联，性能极高。
- **缺点**：生成的二进制体积可能变大（每个类型一份代码），编译时间稍长。

```rust
// 定义“接口” - Trait
trait Speak {
    fn speak(&self);
}

// 实现“接口”的类型
struct Dog;
struct Cat;

impl Speak for Dog {
    fn speak(&self) {
        println!("Woof!");
    }
}

impl Speak for Cat {
    fn speak(&self) {
        println!("Meow!");
    }
}

// 静态分发：泛型函数，任何实现了 Speak 的类型都可以传入
fn make_sound<T: Speak>(animal: T) {
    animal.speak();
}

fn main() {
    let dog = Dog;
    let cat = Cat;
    make_sound(dog); // 编译时生成 make_sound::<Dog>
    make_sound(cat); // 编译时生成 make_sound::<Cat>
}
```



## 2. 动态多态

通过 **trait 对象** `dyn Trait` 实现。使用引用或智能指针（如 `&dyn Trait`、`Box<dyn Trait>`），内部包含指向实例的指针和虚表（vtable）。调用时动态查找方法，有轻微运行时开销。

- **实现原理**：Trait 对象是一个胖指针（fat pointer），包含数据指针和虚表（vtable）指针。调用方法时通过虚表动态查找。
- **优点**：真正的运行时多态，灵活，可以收集不同类型。
- **缺点**：有轻微的运行时开销（动态分发、无法内联）。

```rust
// 使用上面的 Speak trait 和 Dog、Cat 类型

// 动态分发：接受 Trait 对象
fn make_sound_dyn(animal: &dyn Speak) {
    animal.speak();
}

fn main() {
    let dog = Dog;
    let cat = Cat;
    make_sound_dyn(&dog);
    make_sound_dyn(&cat);

    // 混合类型的集合 - 只能使用动态分发
    let animals: Vec<Box<dyn Speak>> = vec![Box::new(Dog), Box::new(Cat)];
    for animal in animals {
        animal.speak();
    }
}
```



### (1) 为何需要 `&`、`Box` —— 类型大小确定

Rust 要求所有在编译时已知大小的类型（即实现 `Sized` trait 的类型）才能直接作为值使用。而 `dyn Trait` 是一个**动态大小类型（DST, Dynamically Sized Type）**，因为具体实现的类型可能不同，编译器无法知道它占用多少字节。例如 `dyn Speak` 本身没有固定大小，不能直接放在栈上或作为函数参数的值类型。

```rust
// 错误：无法确定 dyn Speak 的大小
// let animal: dyn Speak = Dog;
// fn broken(animal: dyn Speak) {}
```

为了能操作 DST，必须将其放在某种指针后面，使指针本身大小固定（在 64 位系统上为 8 字节）。常见的包装方式有：

- `&dyn Trait`：共享引用，指向已有的具体类型实例。
- `&mut dyn Trait`：可变引用。
- `Box<dyn Trait>`：拥有所有权的智能指针，在堆上存储实例。
- `Rc<dyn Trait>` / `Arc<dyn Trait>`：引用计数智能指针，允许多重所有权。

这些指针类型都实现了 `Sized`，因此可以正常传递、存储。同时，它们都会自动成为**胖指针**（见上文），携带数据指针和虚表指针。换句话说：**动态多态需要间接访问，而引用/智能指针提供了这种间接层，同时解决了 DST 的大小不确定问题。**

```rust
fn works_ref(animal: &dyn Speak) { animal.speak(); }
fn works_box(animal: Box<dyn Speak>) { animal.speak(); }

let dog = Dog;
works_ref(&dog);
works_box(Box::new(Cat));   // Box::new 在堆上分配，转换为 trait 对象
```



### (2) Fat指针

在 Rust 中，指向 trait 对象的指针（如 `&dyn Trait`、`Box<dyn Trait>`、`Rc<dyn Trait>`）是一个 **胖指针**，它包含了两个指针大小的数据：

1. **数据指针**（data pointer）：指向具体实例的数据。
2. **虚表指针**（vtable pointer）：指向一个静态生成的虚表（vtable），虚表中存储了该 trait 实现的所有方法指针（以及可能的大小、对齐等信息）。

内存布局示意（64 位系统）：

```text
Fat pointer (16 bytes) -> [ data_ptr: 8 bytes, vtable_ptr: 8 bytes ]
```

调用 trait 对象上的方法时（例如 `obj.method()`），编译器生成的代码大致如下（以 `&dyn Trait` 为例）：

1. 从胖指针中读取虚表指针 —— **无需解引用**（胖指针本身已在栈或寄存器中）。
2. 从虚表指针指向的内存中读取对应方法的函数指针 —— **1 次解引用**（访问虚表）。
3. 调用该函数，并将数据指针作为 `self` 参数传入 —— 调用本身不增加额外解引用，但方法内部对 `self` 的访问会解引用数据指针，这部分属于方法自身的逻辑，不算动态分发的开销。

因此，**每次动态方法调用恰好需要 1 次间接寻址（解引用）**，用于从虚表中提取函数指针。这与 C++ 的虚表机制开销相同。

> 注意：如果考虑多层间接（例如 `&&dyn Trait`），则解引用次数会相应增加，但这属于引用链的长度，并非多态本身的固有开销。

总结：Rust 动态多态通过虚表调用方法时，**需要 1 次解引用**（虚表寻址），无额外的“引用截断”。



在 C++ 中，通过基类指针或引用调用虚函数时，需要 **2 次间接寻址（解引用）**：

1. **从对象指针获取虚表指针（vptr）**：第一次解引用。例如 `p->vptr`。
2. **从虚表中获取函数指针**：第二次解引用。即 `*(vptr + offset)`。

对比：

| 语言 | 对象表示                            | 虚表查找所需解引用次数          |
| ---- | ----------------------------------- | ------------------------------- |
| Rust | 胖指针（数据指针 + 虚表指针）       | 1 次（直接取虚表中的函数指针）  |
| C++  | 对象内隐式 vptr（通过对象指针访问） | 2 次（先取 vptr，再取函数指针） |

> 注意：C++ 的虚函数调用还有一次隐式的 `this` 指针传递，这不属于寻址开销。如果考虑多继承或虚继承，情况更复杂（可能涉及调整 this 指针），但基本间接寻址次数仍至少为 2 次。

因此，Rust 的动态分发通常比 C++ 的虚函数调用少一次内存间接访问，这得益于显式的胖指针设计。

### (3) 与C++多态对比

Rust 的动态多态（`dyn Trait`）与 C++ 的虚函数机制在底层实现上有相似之处，但内存布局、对象模型和安全性存在关键差异。而所谓的 **Fat 指针**（胖指针）正是 Rust 实现动态多态的核心数据结构。

| 方面               | Rust (`dyn Trait`)                                           | C++ (虚函数)                                                 |
| ------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| **虚表归属**       | 虚表是**独立于对象**的，每个 `dyn Trait` 指针携带自己的虚表指针。 | 虚表是**类级别**的，每个对象内部藏有一个虚表指针（vptr）。   |
| **内存布局**       | 对象本身**不包含**虚表指针。胖指针额外存储虚表地址。对象数据连续，无隐藏开销。 | 对象内部**隐含 vptr**（通常位于对象起始位置），对象大小会增加一个指针。 |
| **单对象多 trait** | 同一个对象可以有不同的 trait 对象（例如 `&dyn TraitA` 和 `&dyn TraitB`），每个胖指针的虚表不同，但数据指针指向同一对象。 | 一个对象只能有一份虚表（单继承下），多态仅限于该继承链。多继承时对象包含多个 vptr。 |
| **多继承支持**     | 无多继承，但可通过多个 trait 组合实现类似功能，且可以独立地将对象转换为不同的 trait 对象。 | 支持多继承，对象布局复杂（多个基类子对象，多个 vptr）。      |
| **对象安全性**     | 并不是所有 trait 都可以成为 trait 对象（需要对象安全）。例如包含 `Self: Sized` 或泛型方法的 trait 不能 `dyn`。 | 任何有虚函数的类都可以被多态使用，但纯虚类不能实例化。       |
| **类型擦除方式**   | 显式使用 `dyn Trait`，类型明确是 trait 对象。                | 通过基类指针/引用隐式发生，类型被擦除为基类。                |
| **调用开销**       | 通过胖指针的虚表**一次间接调用**。                           | 通过对象的 vptr 找到虚表，再调用（**共两次间接寻址**）。     |
| **安全性**         | 编译期严格检查生命周期、可变性等。 `dyn Trait` 指针不能随意转换回具体类型（除非使用 `Any`）。 | 可以使用 `dynamic_cast` 向下转型，可能失败或危险。           |
| **对象生命周期**   | 胖指针不管理生命周期（`&dyn Trait` 需要生命周期标注）；`Box<dyn Trait>` 拥有所有权。 | 裸指针/引用不管理生命周期，智能指针（如 `shared_ptr`）可以管理。 |

* **示例**

1. Rust 中的胖指针

```rust
trait Animal { fn sound(&self); }
struct Dog;
impl Animal for Dog { fn sound(&self) { println!("Woof"); } }

fn main() {
    let d = Dog;
    let animal: &dyn Animal = &d;   // 胖指针：数据指向 d，虚表指向 Animal 对 Dog 的实现
    animal.sound();
}
```

2. C++ 中的 vptr

```cpp
class Animal { public: virtual void sound() = 0; };
class Dog : public Animal { public: void sound() override { cout << "Woof"; } };

int main() {
    Dog d;
    Animal* animal = &d;   // 指针指向对象，对象内部包含 vptr
    animal->sound();
}
```
在 C++ 中，`Dog` 对象内部会有一个隐藏的 `__vptr` 指向 `Animal` 类的虚表（实际是 `Dog` 覆盖后的虚表）。而 Rust 中 `Dog` 对象本身没有 vptr，vptr 存在于胖指针中。



## 3. 特殊形式：操作符重载（特设多态）

通过实现 `std::ops` 中的 trait（如 `Add`）来实现同一操作符对不同类型的不同行为，也是一种静态多态。

Rust 不支持随意重载运算符，但允许通过实现 `std::ops` 模块中的特定 trait 来为自定义类型**有选择地重载**运算符。这属于**特设多态（ad-hoc polymorphism）**：同一个运算符对不同类型表现出不同的行为，但行为是由实现 trait 时静态确定的（编译期单态化）。

例如，重载加法运算符 `+` 需要为类型实现 `std::ops::Add` trait：

rust

```
use std::ops::Add;

#[derive(Debug)]
struct Point {
    x: i32,
    y: i32,
}

impl Add for Point {
    type Output = Point;

    fn add(self, other: Point) -> Point {
        Point {
            x: self.x + other.x,
            y: self.y + other.y,
        }
    }
}

fn main() {
    let p1 = Point { x: 1, y: 2 };
    let p2 = Point { x: 3, y: 4 };
    let p3 = p1 + p2;   // 这里调用了 Add::add
    println!("{:?}", p3); // Point { x: 4, y: 6 }
}
```



常见可重载的运算符及其对应的 trait：

| 运算符      | Trait       | 方法签名示例                             |
| ----------- | ----------- | ---------------------------------------- |
| `+`         | `Add`       | `fn add(self, rhs: RHS) -> Self::Output` |
| `-`         | `Sub`       | 类似 `Add`                               |
| `*`         | `Mul`       |                                          |
| `/`         | `Div`       |                                          |
| `%`         | `Rem`       |                                          |
| `&`         | `BitAnd`    |                                          |
| `|`         | `BitOr`     |                                          |
| `^`         | `BitXor`    |                                          |
| `<<`        | `Shl`       |                                          |
| `>>`        | `Shr`       |                                          |
| `-`（一元） | `Neg`       | `fn neg(self) -> Self::Output`           |
| `!`（一元） | `Not`       |                                          |
| `+=`        | `AddAssign` | `fn add_assign(&mut self, rhs: RHS)`     |

注意：运算符重载是**静态多态**的一种表现，因为编译器会根据具体类型静态分发到对应的 trait 实现，没有运行时开销。但它又与普通的泛型 trait bound 略有不同：它允许直接使用运算符符号，提升了代码可读性，本质上仍然是编译期单态化。

# 闭包

下面是对你笔记的**完善与结构化补充**，重点围绕 **1. Fn、FnMut、FnOnce** 和 **2. 闭包类型 trait object** 这两个部分展开，同时保留你原有的精彩内容（比如 `dyn` 的解释、对比表格、不捕获闭包转 `fn` 等）。

---

## 1. Fn、FnMut、FnOnce

这三个 trait 代表了闭包**捕获环境的方式**以及**可被调用的次数/方式**。每个闭包会根据其**内部对捕获变量的操作**，自动实现其中一个或多个 trait。

| Trait    | 捕获方式          | 对捕获变量的操作               | 调用要求                             | 可调用次数     |
| -------- | ----------------- | ------------------------------ | ------------------------------------ | -------------- |
| `FnOnce` | 按值移动 (`move`) | 消耗（consume）变量            | 只能调用一次，因为变量被 move 进闭包 | 一次           |
| `FnMut`  | 可变引用 (`&mut`) | 修改捕获的变量                 | 调用时需要 `&mut self`               | 多次（可修改） |
| `Fn`     | 不可变引用 (`&`)  | 只读访问捕获的变量（或不捕获） | 调用时需要 `&self`                   | 多次（不可改） |

### (1) 三者关系（继承层次）

- `Fn` 继承自 `FnMut`，`FnMut` 继承自 `FnOnce`。
- 任何实现了 `Fn` 的闭包，自动也实现了 `FnMut` 和 `FnOnce`。
- 任何实现了 `FnMut` 的闭包，自动也实现了 `FnOnce`。

```rust
// 从严格到宽松： Fn  ⊂  FnMut  ⊂  FnOnce
// 也就是说，需要一个 FnOnce 的地方，可以传 FnMut 或 Fn 闭包
// 需要一个 FnMut  的地方，可以传 Fn 闭包
// 需要一个 Fn     的地方，不能传 FnMut 或 FnOnce 闭包（因为它们可能修改或消费环境）
```

### (2) 闭包自动实现哪个 trait？

编译器根据闭包体中对捕获变量的操作来决定：

```rust
let x = 5;
let y = String::from("hello");

// 1. 只读访问 → 实现 Fn
let read_only = || println!("{} {}", x, y);
read_only(); read_only();   // 可以多次调用

// 2. 修改捕获的变量 → 实现 FnMut
let mut z = 10;
let mut modify = || { z += 1; println!("{}", z); };
modify(); modify();         // 可以多次调用，但需要 mut

// 3. 消耗捕获的变量（move 且使用后不再需要）→ 实现 FnOnce
let consume = move || {
    drop(y);                // y 被移动进闭包并被消耗
    println!("{}", x);
};
consume();                  // 只能调用一次
// consume();               // 错误：使用了被移动的值
```

### (3) `move` 关键字的作用

- `move` 强制闭包**按值捕获**所有使用的变量（转移所有权）。
- 即使不使用 `move`，当闭包体中对某个变量执行了消耗操作（例如 `drop`、转移所有权），编译器也会自动推导为 `FnOnce`。
- `move` 常用于将闭包传递给新线程，确保闭包内部拥有自己的数据副本。

```rust
let s = String::from("hello");
// 不加 move：会借用 s，可能受限于生命周期
let print = move || println!("{}", s);   // s 被移动到闭包内
// println!("{}", s);                   // 错误：s 已移动
```

---

## 2. 闭包类型 trait object

每个闭包都有一个**匿名且不可写出的具体类型**，即便是两个签名完全相同的闭包，也属于不同类型：

```rust
let add1 = |x| x + 1;
let add2 = |x| x + 1;
// add1 和 add2 的类型不同
```

这就带来一个实际问题：当我们想**在运行时存储多个不同的闭包**（比如放入 `Vec`），或者**从函数返回一个闭包**，就不能直接写具体类型，而必须使用 **trait object**。

### (1) 创建 trait object 的方式

由于 `Fn`、`FnMut`、`FnOnce` 都是 trait（**动态大小类型，DST**），必须将它们放在某种指针后面（`&`、`Box`、`Rc` 等），并用 `dyn` 关键字标记：

```rust
// 1. 引用 trait object
fn call_it(f: &dyn Fn(i32) -> i32, x: i32) -> i32 {
    f(x)
}

// 2. 堆分配的 trait object（最常用）
let closures: Vec<Box<dyn Fn(i32) -> i32>> = vec![
    Box::new(|x| x + 1),
    Box::new(|x| x * 2),
    Box::new(|x| x - 3),
];

// 3. 在结构体中使用
struct Handler {
    callback: Box<dyn Fn(String)>,
}
```

> **为什么必须用 `Box<dyn Trait>`？**  
> 因为 `dyn Trait` 的大小在编译期未知，而 `Vec` 的元素大小必须固定。`Box<dyn Trait>` 是指针（固定大小），指向堆上的具体闭包。

### (2) 从函数返回闭包的两种方法

| 方法                        | 示例                                                         | 特点                                                         |
| --------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| **静态分发 (`impl Trait`)** | `fn make_adder(x: i32) -> impl Fn(i32) -> i32 { move \|y\| x + y }` | 返回**唯一具体类型**（但不用写出来），无额外开销，不能返回不同类型的闭包 |
| **动态分发 (`Box<dyn>`)**   | `fn make_adder(x: i32) -> Box<dyn Fn(i32) -> i32> { Box::new(move \|y\| x + y) }` | 可以返回不同闭包，但有少量堆分配和虚表开销                   |

```rust
// 示例：根据条件返回不同闭包
fn get_operation(op: &str) -> Box<dyn Fn(i32, i32) -> i32> {
    match op {
        "add" => Box::new(|a, b| a + b),
        "sub" => Box::new(|a, b| a - b),
        _ => Box::new(|a, _| a),
    }
}
```

### (3) Trait object 的对象安全性

不是所有 trait 都能做成 trait object。只有**对象安全**的 trait 才能使用 `dyn`。`Fn`、`FnMut`、`FnOnce` 都是对象安全的（因为它们没有 `Self: Sized` 要求，也不包含泛型方法）。但如果你自定义 trait，就需要满足：

- 方法不能有泛型参数
- 方法不能返回 `Self`（除非 `Self: Sized`）

> 闭包的三个 trait 都满足对象安全性，所以可以放心使用 `dyn Fn*`。

### (4) 性能考虑

- **静态分发（泛型/`impl Trait`）**：零成本抽象，编译器为每种类型生成特化代码，没有虚表开销。
- **动态分发（trait object）**：每次调用需要一次虚表间接寻址（通常开销很小，可忽略），并且堆分配（如果用 `Box`）会增加内存分配成本。在性能敏感的循环中，应优先考虑静态分发。

### (5) 为什么要加dyn

在 Rust 中，闭包的类型是**匿名且独有的**（每个闭包都有自己无法写出的类型）。当你想要：

- 在运行时**存储不同类型的闭包**（例如放在 `Vec` 中）
- 或者从函数**返回一个闭包**
- 或者使用**动态分发**（trait object）

就需要用到 `dyn Fn(i32) -> i32` 这样的 **trait object**，而 `dyn` 是它的语法标记。



因为闭包本身并不是一个固定的类型，但它们都实现了 `Fn`、`FnMut` 或 `FnOnce` trait。所以你可以把闭包当作一个 **trait object**（例如 `&dyn Fn(i32)->i32` 或 `Box<dyn Fn(i32)->i32>`）来使用。`dyn` 关键字从 Rust 2018 开始是**强制要求**的，用来显式标记这是一个动态分发的 trait object，而不是静态分发的泛型参数。



这就好比Rust的动态多态，因为**闭包能够捕获变量**，每个闭包类型名字不一样且大小不一样，但一样的是提供的匿名闭包函数签名的参数个数和类型和返回值，`Fn(i32) -> i32`



#### ① 对比：静态分发（泛型） vs 动态分发（`dyn`）

| 方式         | 代码示例                                                     | 特点                                                         |
| ------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| **静态分发** | `fn call(f: impl Fn(i32)->i32)` 或 `fn call<T: Fn(i32)->i32>(f: T)` | 编译器为每个闭包生成单独的函数副本，无运行时开销，但无法在运行时混合不同类型 |
| **动态分发** | `fn call(f: &dyn Fn(i32)->i32)` 或 `Box<dyn Fn(i32)->i32>`   | 使用虚表（vtable）动态分发，有少量运行时开销，但可以处理多种不同类型的闭包 |

#### ② 典型场景

```rust
fn main() {
    let closures: Vec<Box<dyn Fn(i32) -> i32>> = vec![
        Box::new(|x| x + 1),
        Box::new(|x| x * 2),
    ];

    for c in closures {
        println!("{}", c(5));
    }
}
```

如果不用 `dyn`，直接写 `Vec<Fn(i32)->i32>` 是错误的，因为 `Fn` 是一个 trait，而 trait 的大小在编译时未知，必须放在指针后面（`&`、`Box`、`Rc` 等），并且用 `dyn` 标明。

#### ③ 总结

- **`dyn Fn(i32) -> i32`** 是一个 **trait object 类型**，表示任何实现了 `Fn(i32)->i32` 的具体类型（包括闭包）。
- **需要 `dyn`** 是因为 Rust 要求显式区分静态分发（泛型/`impl Trait`）和动态分发（trait object）。
- 不用 `dyn` 而直接写 `Fn(i32)->i32` 在老版本中曾表示 trait object，但自从 Rust 2018 起已废弃，必须加上 `dyn` 以增加清晰度。



### (6). 闭包不捕获环境是函数指针

**如果闭包不捕获环境（即“非捕获闭包”），它可以直接作为函数指针（`fn` 类型）使用，完全不需要 `dyn`。**

#### ① 具体原因

- 不捕获环境的闭包，其内部没有存储任何外部变量，它的行为与普通函数完全相同。
- Rust 允许这种闭包**自动强制转换**为 `fn` 函数指针类型（例如 `fn(i32) -> i32`）。
- `fn` 类型本身是具体类型（大小已知），不需要动态分发，因此无需 `dyn` 关键字。

#### ② 示例对比

```rust
// 不捕获环境的闭包
let add_one = |x| x + 1;
let ptr: fn(i32) -> i32 = add_one; // 自动转换

// 此时完全可以像函数指针一样使用
let result = ptr(5); // 6

// 不需要写 Box<dyn Fn(i32)->i32>
fn call(f: fn(i32) -> i32, x: i32) -> i32 {
    f(x)
}
call(add_one, 10); // 工作正常
```

## 3. 总结（新增）

- 闭包**捕获了环境**（此时无法转换为 `fn`，因为需要携带捕获的数据）。
- 你需要**运行时多态**，例如在 `Vec` 中存放多个不同类型的闭包（有些捕获环境，有些不捕获），且无法统一为 `fn` 类型。

| 场景                                         | 推荐方式                                        |
| -------------------------------------------- | ----------------------------------------------- |
| 单个闭包作为参数，且希望性能最优             | `impl Fn(i32) -> i32` （静态分发）              |
| 多个不同闭包存储在集合中（`Vec`、`HashMap`） | `Box<dyn Fn(i32) -> i32>` （动态分发）          |
| 从函数返回一个闭包（单一类型）               | `-> impl Fn(i32) -> i32` （静态分发，无堆分配） |
| 从函数返回不同闭包（运行时决定）             | `-> Box<dyn Fn(i32) -> i32>` （动态分发）       |
| 闭包不捕获任何环境，并且不需要所有权转移     | 直接使用 `fn` 类型（函数指针），最简单最高效    |
| 闭包需要在线程间共享所有权                   | 使用 `move` + `Arc<dyn Fn() + Send + Sync>`     |

---



# 智能指针

在 Rust 中，指针是一个包含内存地址的变量。普通引用（`&`）是最简单的指针类型，但它们只能借用数据，并不拥有数据的所有权。**智能指针**（Smart Pointer）则是一类数据结构，它们不仅像指针一样指向内存地址，还拥有额外的元数据和能力——最常见的是**所有权管理**（拥有数据并在离开作用域时自动释放）以及实现一些特定的内存管理策略。

Rust 标准库提供了多种智能指针，每个都有其独特的内存管理语义。本章将逐一介绍 `Box<T>`、`Rc<T>`、`Arc<T>`、`Cell<T>`、`RefCell<T>`、`Mutex<T>`、`RwLock<T>` 和 `Cow<'a, B>`。

---

## 1. `Box<T>`

`Box<T>` 提供了 Rust 中最简单的堆分配形式，通常称为“装箱”。它将数据存储在堆上，同时在栈上保留一个指向该数据的指针。`Box<T>` **拥有**它所指向的数据的所有权——当 `Box` 离开作用域时，其内部的数据也会被自动 drop 释放。

### (1) 结构定义

```rust
pub struct Box<T: ?Sized>(Unique<T>);
```

`Box<T>` 在底层本质上是对裸指针 `*mut T` 的安全封装，编译器在语义层面跟踪它的生命周期。`Box` 本身的大小等于一个裸指针的大小（通常为 8 字节在 64 位系统上），与实际存储的数据大小无关。

### (2) 内存布局

对于**非零大小**（non-zero-sized）的类型，`Box` 会使用全局分配器 `Global` 来执行内存分配。`Box` 与使用相同分配器分配的裸指针之间可以进行安全的双向转换。

对于**零大小类型**（zero-sized types, ZSTs，如 `()`、`struct MyUnit;`），`Box` 不会进行任何堆分配。零大小类型不需要分配实际内存空间，因为所有该类型的实例在语义上都是等同的，不需要在堆上存储区分它们的数据。

### (3) 使用场景

- **将大型数据结构从栈移到堆上**：栈的大小是有限的，当需要在栈帧中容纳一个体积很大的结构体时，可以使用 `Box` 将其分配到堆上，栈上只留下一个小的指针。这也是 Rust 中常见的大对象转移方式。
- **构建递归数据结构**：在 Rust 中，递归类型（如链表、树）的字段如果直接包含自身，会导致编译器无法在编译时确定类型的大小（无限递归）。使用 `Box` 将递归部分放置在堆上，栈上的父结构只持有一个固定大小的指针，从而解决了这一问题。经典的例子是 `Cons` 链表：

```rust
enum List<T> {
    Cons(T, Box<List<T>>),
    Nil,
}
```

如果没有 `Box`，`enum` 的大小将无限膨胀，导致编译错误。

- **封装动态分配的资源**，独占所有权，确保离开作用域时自动清理。
- **实现 DSTs(动态类型大小 Dynamically Sized Types) / trait object 的动态分发**：当需要将实现了同一 trait 的不同类型放在同一个容器中时（如 `Vec<Box<dyn Trait>>`），`Box<dyn Trait>` 是实现这一目标的常用方式。
- **Owned slice 区别于`&`的Borrowed slice**

### (4) 关键特性

- `Box<T>` 遵循 Rust 的所有权规则：一个值在任一时刻有且只有一个所有者。
- `Box<T>` 提供了 **Deref** （解引用）trait 的实现，使得可以直接通过 `*` 运算符访问内部的值。
- `Box<T>` 实现了 `Drop`，在离开作用域时自动释放堆上分配的内存。
- `Box` 本身是**零运行时开销**的，其销毁逻辑完全由编译器生成的 `Drop` 实现完成，不需要任何引用计数或锁机制。
- `Box<T>` 保证其分配的字节数永远不会超过 `isize::MAX` 字节。

### (5) 示例

```rust
// 将值从栈上移到堆上
let val: u8 = 5;
let boxed: Box<u8> = Box::new(val);

// 通过解引用将值从堆上移回栈上
let val_back: u8 = *boxed;

// 递归链表示例
#[derive(Debug)]
enum List<T> {
    Cons(T, Box<List<T>>),
    Nil,
}

let list: List<i32> = List::Cons(1, Box::new(List::Cons(2, Box::new(List::Nil))));
// 输出: Cons(1, Cons(2, Nil))
```

---

## 2. `Rc<T>`

`Rc<T>` 是 **Reference Counted** 的缩写，提供了堆上分配的值的**共享所有权**机制。当多个部分需要同一份数据的所有权，但在编译时无法确定哪个部分会最后使用完数据时，`Rc<T>` 就派上了用场。

`Rc<T>` **仅适用于单线程场景**，它的引用计数操作不是原子性的，因此没有线程同步的开销，性能较高，但线程不安全。

### (1) 结构定义

`Rc<T>` 在底层是一个结构体，内部包含指向 `RcBox<T>`（即引用计数和数据）的指针。

`Rc<T>` 不实现 `Send` 和 `Sync` trait，这意味着它不能被安全地在线程之间传递。

`Rc<T>` 提供了两个引用计数：

- **强引用计数**（strong count）：表示有多少个活跃的 `Rc` 指针指向该数据。当强引用计数归零时，数据会被 drop 并释放。
- **弱引用计数**（weak count）：由 `Weak<T>` 类型维护，用于打破循环引用（见下文）。

### (2) 内存布局

`Rc<T>` 的布局类似于在堆上分配一个包含两个引用计数器和实际数据 `T` 的结构体。所有指向同一个分配的 `Rc` 指针都共享同一份引用计数器。当最后一个 `Rc` 被 drop 时，计数器从 1 变为 0，`Rc` 会释放其底层资源。

`Rc<T>` 没有实现 `DerefMut`，**仅用于不可变数据**，或使用`Rc::get_mut()`(有限制：恰好仅有一个强引用，0个弱引用，否则返回None)，因为多个 `Rc` 指针共享同一个数据，通过 `&mut` 修改数据将违反 Rust 的借用规则。如果需要修改，可以结合 `RefCell<T>` 使用（参见下文）。

### (3) 使用场景

- **图结构或抽象语法树（AST）中共享节点**：在构建复杂数据结构时，多个父节点可能需要引用同一个子节点。
- **单线程 GUI 框架中共享组件状态**：UI 组件树中的多个组件可能需要引用同一份状态数据。
- **多个逻辑实体引用同一份底层数据的场景**。

### (4) 循环引用与 `Weak<T>`

`Rc<T>` 的一个经典陷阱是**循环引用导致的内存泄漏**：如果两个对象互相持有对方的 `Rc` 引用，它们的引用计数永远不会降为零，即使已经没有外部指针指向它们，数据也无法被释放。

为了解决这一问题，Rust 提供了 `Weak<T>` 类型——一种**弱引用**，它**不参与引用计数**。使用 `Weak` 不会增加强引用计数，因此不会阻止数据被释放。通过 `Weak::upgrade()` 方法，可以尝试获取一个 `Rc` 强引用——如果数据已经被释放，则返回 `None`。

`Weak<T>` 的典型用途是**打破父节点对子节点的强引用循环**：父节点持有一个 `Rc<T>` 指向子节点（强引用），而子节点持有一个 `Weak<T>` 指回父节点（弱引用），这样父节点 drop 时不会因为子节点的反向引用而导致计数无法归零。

### (5) 示例

```rust
use std::rc::Rc;

let a = Rc::new(5);
let b = Rc::clone(&a);  // 增加引用计数

println!("引用计数: {}", Rc::strong_count(&a)); // 输出: 2

// Weak 示例：避免循环引用
use std::rc::Weak;

struct Node {
    value: i32,
    parent: RefCell<Weak<Node>>,
    children: RefCell<Vec<Rc<Node>>>,
}
```

---

## 3. `Arc<T>`

`Arc<T>` 是 **Atomic Reference Counted**（原子引用计数）的缩写，是 `Rc<T>` 的**线程安全**版本。

### (1) 结构定义

与 `Rc<T>` 类似，`Arc<T>` 在底层是一个指向堆上分配的 `ArcInner<T>` 的指针，区别在于引用计数使用原子类型 `AtomicUsize` 进行操作，以保证在多线程环境下引用计数的正确性：

```rust
pub struct Arc<T> {
    ptr: NonNull<ArcInner<T>>,
    phantom: PhantomData<ArcInner<T>>,
}

struct ArcInner<T> {
    strong: atomic::AtomicUsize,   // 原子性的强引用计数
    weak: atomic::AtomicUsize,     // 原子性的弱引用计数
    data: T,
}
```

- `NonNull<T>` 确保指针永不为空，并声明了对 `T` 的协变性（covariance）。
- `PhantomData<ArcInner<T>>` 用于向 drop checker 正确传达所有权信息，告诉编译器该结构体在某种程度上拥有 `ArcInner<T>`（从而拥有 `T`）。

### (2) 内存布局

`Arc<T>` 的实现将引用计数和数据放置在**同一块堆分配的内存**中，这使得访问引用计数和数据都非常高效。引用计数与数据在内存中是紧挨在一起的，因此 `Arc` 和 `Weak` 都是普通指针（而非胖指针）。

### (3) 使用场景

- **多线程共享所有权**：当需要在多个线程之间共享数据的所有权时，`Arc<T>` 是首选。
- **并发读取共享数据**：`Arc<T>` 通常与内部可变性类型（如 `Mutex<T>` 或 `RwLock<T>`）结合使用，以实现线程安全的共享可变状态。

由于原子操作涉及 CPU 层面的内存屏障与同步指令，`Arc<T>` 的性能略低于单线程的 `Rc<T>`。

### (4) 关于 `Arc::get_mut()`

`Arc::get_mut(&mut self)` 方法可以获得一个 `&mut T`，**但有一个重要前提**：只有当当前 `Arc` 是唯一的强引用（即 `strong_count == 1`）并且没有弱引用时，它才会返回 `Some(&mut T)`；否则返回 `None`。当满足此条件时，`get_mut` 实际上获得了唯一所有权，因此可以安全地修改数据，而无需考虑其他引用的存在。

### (5) 示例

```rust
use std::sync::Arc;
use std::thread;

let data = Arc::new(vec![1, 2, 3]);

let mut handles = vec![];
for _ in 0..3 {
    let data_clone = Arc::clone(&data);
    let handle = thread::spawn(move || {
        println!("{:?}", data_clone);
    });
    handles.push(handle);
}

for handle in handles {
    handle.join().unwrap();
}
```

---

## 4. `Cell<T>`

`Cell<T>` 是 Rust 标准库中最简单的**内部可变性**（interior mutability）类型。它允许通过共享引用（`&Cell<T>`）修改内部的值，而无需获取可变引用 `&mut T`。

### (1) 结构定义

`Cell<T>` 在底层基于 `UnsafeCell<T>` 构建。`UnsafeCell<T>` 是 Rust 中内部可变性的核心原语——它选择退出（opt-out）了 `&T` 的不可变性保证，允许共享引用指向可变的数据。所有允许内部可变性的类型，包括 `Cell<T>`、`RefCell<T>` 和 `OnceCell<T>`，内部都使用了 `UnsafeCell` 来包装它们的数据。

```rust
#[repr(transparent)]
pub struct Cell<T> {
    value: UnsafeCell<T>,
}
```

`repr(transparent)` 表示 `Cell<T>` 在内存布局上与内部的 `UnsafeCell<T>` 完全相同，确保安全代码可以正确地进行类型转换。

### (2) 内部可变性

Rust 的内存安全基于核心规则：给定一个对象 `T`，只能同时具有以下两种情况之一——多个不可变引用（`&T`）或一个可变引用（`&mut T`），但绝不能同时拥有。

**继承可变性**（inherited mutability）：这是 Rust 默认的行为——可变性由引用的类型决定，拥有 `&mut T` 才能修改，拥有 `&T` 则不能修改。

**内部可变性**（interior mutability）：这是 `Cell` 等类型提供的替代方案——即使只持有共享引用 `&T`，也可以修改内部的值。这是通过 `unsafe` 代码在数据结构内部安全地弯曲 Rust 的借用规则实现的。

### (3) `Cell<T>` 的工作原理

`Cell<T>` 通过**将值移入和移出 cell** 来实现内部可变性。换言之，永远无法获取内部值的 `&mut T`；如果不将值替换为其他内容，也无法直接获取值本身。这两个约束确保了永远不会同时存在一个以上的指向内部值的引用。

`Cell<T>` 的核心方法：

- `get(&self) -> T`：返回内部值的副本（只适用于实现了 `Copy` 的类型）。
- `set(&self, val: T)`：将内部值替换为新值，丢弃旧值。
- `replace(&self, val: T) -> T`：替换当前内部值并返回被替换的值。
- `take(&self) -> T`：用 `Default::default()` 替换并返回原值（适用于实现了 `Default` 的类型）。
- `into_inner(self) -> T`：消耗 `Cell`，返回内部值。

### (4) 使用场景

- 实现单线程中需要共享引用的可变状态，且值的类型实现了 `Copy` 或大小可管理时。
- 作为结构体字段为对象提供内部可变性。
- 需要在多组不可变引用之间共享一个小的可修改值。

### (5) 示例

```rust
use std::cell::Cell;

let c = Cell::new(42);
c.set(100);
println!("值: {}", c.get());  // 输出: 100

// 对于实现了 Copy 的类型，get 直接返回值的副本
let x = c.get();
```

---

## 5. `RefCell<T>`

`RefCell<T>` 是另一个提供内部可变性的类型，但与 `Cell<T>` 不同的是，它允许**借用其内部值的引用**，而不是通过移入/移出值来修改。

### (1) 结构定义

`RefCell<T>` 同样基于 `UnsafeCell<T>` 构建，并在运行时跟踪借用规则——有多少个活跃的不可变借用（`Ref<T>`）和是否存在一个可变借用（`RefMut<T>`）。

```rust
pub struct RefCell<T: ?Sized> {
    borrow: Cell<BorrowFlag>,
    value: UnsafeCell<T>,
}
```

`BorrowFlag` 是一个内部使用的标志，用于追踪当前活跃借用的数量（不可变借用的计数和是否有一个可变借用）。

### (2) 运行时借用规则检查

与 `Box<T>` 和普通引用的**编译时**借用规则检查不同，`RefCell<T>` 将其借用规则的违反检查推迟到**运行时**。

- 如果违反借用规则（例如，在已经有一个可变借用的情况下尝试获取另一个借用，或同时获取多个可变借用），`RefCell<T>` 会触发 **panic** 并使程序退出，而不是产生编译器错误。
- 编译时检查的优势是错误能更早地被发现，并且没有运行时性能开销。但它的劣势是静态分析是**保守的**（conservative）——某些在运行时实际上是安全的模式，编译器可能会拒绝编译。
- `RefCell<T>` 允许这些本来在编译时被禁止但在运行时安全的模式。当你能够确定代码符合借用规则，但编译器无法静态理解时，`RefCell<T>` 就非常有价值。

### (3) 核心 API

- `borrow(&self) -> Ref<T>`：获取一个不可变引用（运行时检查，如果当前已存在可变借用则 panic）。
- `borrow_mut(&self) -> RefMut<T>`：获取一个可变引用（运行时检查，如果当前已存在任何活跃借用则 panic）。
- `try_borrow(&self) -> Result<Ref<T>, BorrowError>`：尝试获取不可变引用，失败时返回 `Err` 而非 panic。
- `try_borrow_mut(&self) -> Result<RefMut<T>, BorrowMutError>`：尝试获取可变引用，失败时返回 `Err`。

`Ref<T>` 和 `RefMut<T>` 实现了 `Deref`（以及 `RefMut` 实现了 `DerefMut`），可以直接像使用普通引用一样访问内部的值。当这些 guard 离开作用域时，借用计数会自动减少。

### (4) 与 `Rc<T>` 的组合

`Rc<T>` 提供了共享所有权，但数据是只读的；`RefCell<T>` 提供了内部可变性，但只有单一所有者。`Rc<RefCell<T>>` 的组合模式实现了**拥有多个所有者且允许修改**的共享数据，**仅适用于单线程**。

| 类型             | 所有者数量 | 可变性           |
| ---------------- | ---------- | ---------------- |
| `Box<T>`         | 单一所有者 | 编译时可变性     |
| `Rc<T>`          | 多个所有者 | 只读             |
| `RefCell<T>`     | 单一所有者 | 运行时内部可变性 |
| `Rc<RefCell<T>>` | 多个所有者 | 运行时内部可变性 |

### (5) 示例

```rust
use std::cell::RefCell;

let cell = RefCell::new(42);

// 可以同时拥有多个不可变借用
{
    let r1 = cell.borrow();
    let r2 = cell.borrow();
    println!("r1: {}, r2: {}", *r1, *r2);
} // 借用在这里结束

// 可变借用
{
    let mut r = cell.borrow_mut();
    *r += 1;
}
println!("最终值: {}", cell.borrow()); // 输出: 43

// Rc<RefCell<T>> 示例
use std::rc::Rc;
let shared_data = Rc::new(RefCell::new(10));
let data2 = Rc::clone(&shared_data);
*shared_data.borrow_mut() = 20;
println!("data2: {}", data2.borrow()); // 输出: 20
```

### (6) 何时使用

- 当需要确保代码在运行时遵守借用规则，但编译器因保守的静态分析无法验证其安全性时。
- 当需要在单线程环境中实现内部可变性，且需要借用引用（而不是直接拷贝值）时。
- `RefCell<T>` 和 `Rc<T>` 一样，也**只适用于单线程**。在并发场景下，应使用 `Mutex<T>` 或 `RwLock<T>`。

---

## 6. `Mutex<T>`

`Mutex<T>`（互斥量）是 Rust 标准库中用于线程安全的共享可变状态的主要同步原语之一。它保证在任一时刻，只有一个线程可以访问被保护的数据。

### (1) 结构定义

在 Rust 标准库中，`Mutex<T>` 是一个围绕操作系统原生互斥量的跨平台封装，其内部包含了被保护的数据和一个同步原语。

```rust
pub struct Mutex<T: ?Sized> {
    // 内部实现依赖于具体的操作系统，通常包含：
    // - 一个操作系统级别的互斥量
    // - 被保护的内部数据（通过 UnsafeCell 包装）
}
```

与 `RefCell<T>` 类似，`Mutex<T>` 也实现了**内部可变性**——通过 `lock()` 方法获取的 `MutexGuard<T>` 实现了 `DerefMut`，可以直接对被保护的数据进行修改，而 `Mutex` 本身可以被共享引用 `&Mutex<T>` 持有。

### (2) 工作方式

- `Mutex::new(data)` 创建互斥量，并将数据的所有权移交给它。
- `lock()` 方法尝试获取锁，如果锁被其他线程持有，当前线程会**阻塞**直到锁可用，然后返回一个 `MutexGuard<T>`（防护对象）。
- `try_lock()` 方法尝试获取锁，如果锁已被占用则立即返回 `Err`（非阻塞）。
- `MutexGuard<T>` 是一个 RAII 防护（Resource Acquisition Is Initialization）——它实现了 `Deref` 和 `DerefMut`，允许直接访问内部的数据。当 `MutexGuard` 离开作用域（被 drop）时，锁会自动释放。

### (3) 中毒（Poisoning）

Rust 的 `Mutex<T>` 具有**中毒**（poisoning）机制：如果线程在持有锁的过程中发生 `panic`，`Mutex` 会变为“中毒”状态。后续尝试 `lock()` 的线程会得到 `Err`（返回 `PoisonError`），因为被保护的数据可能处于不一致的状态。

中毒机制允许开发者决定如何处理这种不一致——可以 `unwrap()`（panic 传播）、`expect()`，也可以调用 `into_inner()` 直接恢复数据。

### (4) 示例

```rust
use std::sync::Mutex;
use std::thread;

let counter = Mutex::new(0);
let mut handles = vec![];

for _ in 0..10 {
    let handle = thread::spawn(move || {
        let mut num = counter.lock().unwrap();  // 阻塞直到获取锁
        *num += 1;
    });  // guard 离开作用域时自动释放锁
    handles.push(handle);
}

for handle in handles {
    handle.join().unwrap();
}

println!("Result: {}", *counter.lock().unwrap()); // 输出: 10
```

### (5) 性能考虑

- `Mutex` 的加锁和释放涉及系统调用（对于 OS 级别的互斥量），开销相对较大。
- 为了最大化性能，应尽量减小锁的持有范围（即 `lock()` 和 `drop` guard 之间的代码段尽量短）。
- 对于**读多写少**的场景，`RwLock<T>` 可能比 `Mutex` 更高效。

---

## 7. `RwLock<T>`

`RwLock<T>` 是读写锁（reader-writer lock），是 `Mutex<T>` 的一个变种，它将获取锁的访问类型区分为**读**（共享）和**写**（独占）。在任何时刻，`RwLock` 允许任意数量的 reader 同时持有锁，但**最多只有一个 writer** 持有锁。

### (1) 结构定义

```rust
pub struct RwLock<T: ?Sized> {
    // 内部实现依赖于操作系统，包含：
    // - 共享读访问的状态
    // - 独占写访问的状态
    // - 被保护的数据（通过 UnsafeCell 包装）
}
```

类型参数 `T` 必须同时满足 `Send`（可以在线程间安全转移）和 `Sync`（可以通过共享引用安全访问）。

### (2) 核心 API

- `read(&self) -> LockResult<RwLockReadGuard<T>>`：获取共享读锁。多个线程可以同时获取读锁，读锁之间不会互相阻塞。
- `write(&self) -> LockResult<RwLockWriteGuard<T>>`：获取独占写锁。当 writer 持有锁时，所有 reader 和其他 writer 都会被阻塞。
- `try_read(&self) -> TryLockResult<RwLockReadGuard<T>>`：尝试获取读锁，非阻塞。
- `try_write(&self) -> TryLockResult<RwLockWriteGuard<T>>`：尝试获取写锁，非阻塞。

- `RwLockReadGuard<T>` 实现了 `Deref`，允许只读访问。
- `RwLockWriteGuard<T>` 实现了 `DerefMut`，允许可变访问。

### (3) 锁的优先级与注意事项

锁的优先级策略依赖于底层操作系统的实现，Rust 的标准 `RwLock` 不保证特定的优先级策略。这可能会导致在某些实现中出现**写饥饿**（writer starvation）或**读饥饿**（reader starvation）的问题。

**潜在的死锁**：如果 reader 持有了一个读锁，然后尝试获取写锁（在同一线程），会导致死锁——因为写锁需要等待所有读锁释放，但读锁正是被当前线程持有的。

### (4) 中毒（Poisoning）

与 `Mutex<T>` 类似，`RwLock<T>` 也支持中毒机制。但是有一个重要区别：只有当 panic **发生在独占锁定状态**（即写模式）下时，`RwLock` 才会中毒；如果 panic 发生在任何 reader 中，锁**不会**中毒。

### (5) 与 `Mutex<T>` 的对比

| 特性             | `Mutex<T>`                           | `RwLock<T>`                                  |
| ---------------- | ------------------------------------ | -------------------------------------------- |
| 访问模式         | 不分读写，所有获取锁的操作都是独占的 | 读共享（多个 reader），写独占（一个 writer） |
| 读多写少场景性能 | 一般（所有线程都会被阻塞）           | 优秀（多个 reader 可并发）                   |
| 写频繁场景性能   | 相近                                 | 可能稍差（因为额外的读写状态管理）           |
| 实现复杂度       | 简单，开销低                         | 稍高                                         |
| 适用场景         | 读写频率相近，或写操作较多           | 读操作远多于写操作的场景                     |

### (6) 示例

```rust
use std::sync::RwLock;

let lock = RwLock::new(5);

// 可以有多个 reader
{
    let r1 = lock.read().unwrap();
    let r2 = lock.read().unwrap();
    assert_eq!(*r1, 5);
    assert_eq!(*r2, 5);
} // reader 在这里 drop，释放锁

// writer 独占访问
{
    let mut w = lock.write().unwrap();
    *w += 1;
    assert_eq!(*w, 6);
} // writer 离开作用域时自动释放锁
```

---

## 8. `Cow<'a, B>`

`Cow<'a, B>` 是 **Clone on Write**（写时克隆）的缩写，是 Rust 中一个相当独特的智能指针。它允许以**借用**（borrowed）的形式持有数据，只有在真正需要修改时才会进行克隆，并变为**拥有**（owned）状态。

### (1) 结构定义

`Cow` 实际上是一个枚举（enum），而不是一个结构体：

```rust
pub enum Cow<'a, B: ?Sized + 'a + ToOwned> {
    Borrowed(&'a B),           // 借用的变体（不拥有数据）
    Owned(<B as ToOwned>::Owned),  // 拥有的变体（拥有数据）
}
```

- `B` 是基础类型，必须实现 `ToOwned` trait——它定义了如何从 `&B` 创建一个 `Owned` 类型的值。
- `Borrowed(&'a B)` 变体直接借用一个现有数据，不复制。
- `Owned(<B as ToOwned>::Owned)` 变体拥有数据（其类型通常是与 `B` 对应的拥有权类型，如 `String` 对 `str`）。

### (2) `ToOwned` 和 `Borrow` 的关系

- `ToOwned` trait 定义了如何将借用数据 `&B` 转换为拥有所有权的 `Owned` 类型。
- `Borrow<Borrowed>` trait 定义了如何从 `Owned` 类型借回 `&B`。
- 例如，`str` 实现了 `ToOwned<Owned = String>`，而 `String` 实现了 `Borrow<str>`。

这种配对关系保证了 `Cow` 的两侧可以无缝转换。

### (3) 核心能力

- **实现 `Deref`**：无论 `Cow` 当前是 `Borrowed` 还是 `Owned` 变体，都可以像 `&B` 一样直接调用其方法（通过 `Deref` 自动解引用）。
- **`to_mut(&self) -> &mut <B as ToOwned>::Owned`**：如果需要修改数据，调用 `to_mut()` 可以确保获得一个可变引用。如果当前是 `Borrowed` 变体，它会自动克隆并转换为 `Owned` 变体，然后将内部所有权转移到 `Owned` 上。这就是“写时克隆”的核心所在。
- **`into_owned(self) -> <B as ToOwned>::Owned`**：无论当前是哪个变体，最终都会返回一个拥有的值（如果是 `Borrowed` 变体则进行克隆）。

### (4) 使用场景

**① 字符串处理（`Cow<str>`）**

比如实现一个将字符串首字母大写的函数。如果字符串的首字母已经是大写，可以直接借用原字符串；如果需要修改，才克隆生成新字符串：

```rust
use std::borrow::Cow;

fn to_title(mut s: Cow<str>) -> Cow<str> {
    if let Some(first) = s.chars().next() {
        if !first.is_uppercase() {
            return Cow::Owned(
                first.to_uppercase().collect::<String>() + &s[1..]
            );
        }
    }
    s
}
```

**② 解析数据时的零拷贝优化**

比如在解析 JSON 或 URL 编码时，如果没有特殊字符需要转义，可以直接使用原字符串的引用；只有在遇到需要解码（如 `%20` 转空格）等场景时，才生成新的拥有所有权的字符串。

**③ 缓存或返回值的统一接口**

通过 `Cow`，函数可以统一返回借用数据或拥有数据，对外提供一致的接口，而不需要复制不必要的开销。

### (5) 示例

```rust
use std::borrow::Cow;

// 处理输入：如果是 &str，则不复制；如果是 String，则直接使用
fn process_data(data: Cow<str>) {
    let processed = data.to_uppercase();  // 如果 data 是 Borrowed，这里会触发复制
    println!("{}", processed);
}

fn main() {
    let borrowed = "hello";
    let owned = String::from("world");

    process_data(Cow::Borrowed(borrowed));  // 无复制
    process_data(Cow::Owned(owned));        // 所有权已转移，无额外复制
}

// 写时克隆的典型用法：非修改路径零开销
fn maybe_modify(s: &str) -> Cow<str> {
    if s.contains(' ') {
        // 需要修改：转为 Owned 并修改
        let mut owned = s.to_string();
        owned.push('!');
        Cow::Owned(owned)
    } else {
        // 无需修改：直接借用原字符串
        Cow::Borrowed(s)
    }
}
```

### (6) 性能与注意事项

- **优势**：在“读多写少”的场景下，`Cow` 可以显著减少不必要的内存分配和复制开销。
- **注意事项**：
  - `Borrowed` 变体的生命周期必须合理管理，不能超过原始借用的数据的有效期。
  - 即使没有写入操作，某些情况下（如调用 `into_owned()`）也会触发克隆和新的分配。
  - 在 `Cow` 上调用 `to_mut()` 会无条件确保返回可变引用——如果是 `Borrowed` 变体会触发克隆。因此应该谨慎使用，仅在确实需要修改时才调用。

---

## 智能指针速查表

| 类型         | 所有权特性 | 可变性                            | 线程安全           | 主要用途                       |
| ------------ | ---------- | --------------------------------- | ------------------ | ------------------------------ |
| `Box<T>`     | 单一所有权 | 编译时（通过 `&mut`）             | 可跨线程（`Send`） | 堆分配、递归类型、trait object |
| `Rc<T>`      | 共享所有权 | 只读（除非结合 `RefCell`）        | ❌ 仅单线程         | 单线程中的多个所有者           |
| `Arc<T>`     | 共享所有权 | 只读（除非结合 `Mutex`/`RwLock`） | ✅ 多线程           | 多线程中的共享所有权           |
| `Cell<T>`    | 单一或共享 | 内部可变性（通过 `get`/`set`）    | ❌ 仅单线程         | 小而简单的值的内部可变性       |
| `RefCell<T>` | 单一所有权 | 运行时检查的内部可变性            | ❌ 仅单线程         | 需要借用的内部可变性           |
| `Mutex<T>`   | 单一所有权 | 运行时阻塞的内部可变性            | ✅ 多线程           | 多线程互斥访问                 |
| `RwLock<T>`  | 单一所有权 | 读/写区分的内部可变性             | ✅ 多线程           | 读多写少的并发场景             |
| `Cow<'a, B>` | 借用或拥有 | 写时克隆                          | 取决于内部 `B`     | 延迟克隆优化                   |

------



# 模块

Rust 的模块系统帮助你组织代码、控制可见性（私有/公开）、管理路径。在 Rust 中，**每个 `.rs` 文件**默认就是一个模块。模块可以嵌套，用来组织代码、控制可见性。

## 1. `mod` —— 声明模块

**作用**：告诉 Rust “我要用这个模块了”，并让 Rust 去找对应的文件。

`mod` 用于**定义**一个新的模块。有两种用法：

- **内联模块**：如果模块是内联的（写在同一个文件里），`mod` 后面直接跟大括号，`mod name { ... }` 直接在当前位置定义模块内容。
- **文件模块**：如果模块在单独的文件里，`mod` 只是声明，文件内容自动成为模块内容，`mod name;` 告诉编译器从外部文件加载模块内容。  
  文件查找规则（Rust 2018 及之后）：
  - 当前目录下的 `name.rs`
  - 当前目录下的 `name/mod.rs`（老风格，仍可用）

**示例：**

```rust
// 方法1：内联模块
mod math {
    pub fn add(a: i32, b: i32) -> i32 {
        a + b
    }
}

// 方法2：声明外部文件模块（假设有 math.rs 或 math/mod.rs）
mod math;   // 这会去寻找 math.rs 或 math/mod.rs
```

## 2. `pub mod` —— 公开模块

**作用**：让外部可以访问这个模块。没有 `pub` 的模块是私有的，只能在当前 crate 内部使用。

默认情况下，`mod` 声明的模块是**私有**的，只有父模块及同模块的代码能访问。  
加上 `pub` 后，模块对外部可见（但模块内部的项仍需单独标记 `pub` 才能被外部使用）。

```rust
// lib.rs
pub mod network;   // 外部crate可以访问 network 模块
mod private;       // 外部无法访问 private 模块
```

## 3. `pub` —— 公开项

**作用**：用在函数、结构体、字段、枚举等前面，表示“公开”。

- 模块内的项默认是私有的（只有父模块和同模块内可访问）。
- 加上 `pub` 可以让其他模块（甚至外部 crate）访问。

**示例：**

```rust
mod my_module {
    fn private_func() {}          // 只有 my_module 内部能用
    pub fn public_func() {}       // 任何地方都能用（前提是模块本身也可见）
}
```

**注意**：结构体字段的可见性需要单独标注：

```rust
pub struct Person {
    pub name: String,   // 公开字段
    age: u32,           // 私有字段，只能在本模块内访问
}
```

## 4. `use` —— 引入路径
**作用**：将某个路径绑定到当前作用域，避免每次写完整路径。可以配合 `as` 重命名。

**示例：**

```rust
// 不使用 use
crate::network::connect();

// 使用 use
use crate::network::connect;
connect();
```

## 5. `pub use` —— 重新导出
**作用**：把某个项引入当前模块，并**对外公开**。这叫“重新导出”（re-export）。

- 外部使用者可以用你定义的短路径访问到深层模块的内容，而无需知道原始路径。
- 常用于统一/简化 API 入口。

**示例：**

```rust
// lib.rs
mod network;
pub use network::connect;   // 外部可以直接用 crate::connect

// 外部代码
use my_crate::connect;   // 不用再写 my_crate::network::connect
```

## 6. `mod.rs`
在 Rust 2015 老 Rust 代码风格中，如果一个模块包含子模块，则模块根文件必须命名为 `mod.rs`。例如 `foo/mod.rs` 表示 `foo` 模块，其子模块放在 `foo/` 目录下。  

在中，如果一个模块有子模块，就需要 `mod.rs` 文件。
例如：模块 `network` 有子模块 `tcp`，目录结构：

```text
src/
├── network/
│   ├── mod.rs      // 定义 network 模块的内容，并声明子模块
│   └── tcp.rs
└── main.rs
```

`network/mod.rs` 内容：

```rust
pub mod tcp;   // 声明子模块 tcp
```

**Rust 2018 以后推荐使用同名文件**（见下面解释），例如：直接使用 `foo.rs` 作为模块根，子模块放在 `foo/` 目录下，不再强制使用 `mod.rs`，但 `mod.rs` 依然有效。

## 7. `properties.rs` —— 普通模块文件

 `properties.rs`，其实就是**一个普通的模块文件**，文件名叫 `properties.rs`，里面写代码。
例如：

```text
src/
├── properties.rs
└── main.rs
```

在 `main.rs` 中：

```rust
mod properties;   // 引入 properties.rs 作为模块
use properties::some_func;
```

**注意**：如果模块还需要子模块，最好用 `mod.rs` 或同名的目录。Rust 2018 支持在 `properties.rs` 旁边放 `properties/` 目录来放子模块，但文件名是 `properties.rs` 本身。

## 8. `super::` —— 父模块路径
**作用**：在模块中访问**上一级模块**的项（类似文件系统的 `..`）。

**示例：**

```rust
mod parent {
    const X: i32 = 10;
    mod child {
        fn get_x() -> i32 {
            super::X   // 访问父模块的 X
        }
    }
}
```

## 9. `crate::` —— crate 根路径
**作用**：从当前 crate 的根模块（`main.rs` 或 `lib.rs`）开始**绝对路径**。

**示例：**

```rust
// 在任意文件里
crate::network::connect();
```

相当于 C++ 中项目根命名空间。

---

## 完整示例场景
假设我们要编写一个网络库 `my_net`，包含配置管理、TCP 协议支持、工具函数。需要对外提供简洁的 API，内部实现清晰模块化。

### 项目结构（现代风格）
```
my_net/
├── Cargo.toml
└── src/
    ├── lib.rs             (crate 根)
    ├── config.rs          (配置模块)
    ├── protocol.rs        (协议模块根)
    └── protocol/          (协议子模块目录)
        └── tcp.rs         (TCP 实现)
```

### 逐步代码实现

#### `src/lib.rs` —— crate 根
```rust
// 声明模块（私有或公开）
mod config;                // 私有模块，从 config.rs 加载
pub mod protocol;          // 公开模块，从 protocol.rs 加载

// 使用 pub use 重新导出，简化对外 API
pub use config::Config;                // 将 Config 提升到 crate 根
pub use protocol::tcp::TcpStream;      // 直接导出深层模块的类型

// 内部工具函数（私有）
fn internal_helper() -> u32 {
    42
}

// 对外公开的初始化函数
pub fn init() {
    // 使用绝对路径访问内部模块
    let _cfg = crate::config::load();
    // 使用相对路径访问兄弟模块
    let _ = protocol::handshake();
    println!("Library initialized");
}
```

#### `src/config.rs` —— 配置模块
```rust
// 公开结构体，但字段私有
pub struct Config {
    url: String,          // 私有字段
    timeout: u64,
}

impl Config {
    // 公开构造函数
    pub fn new(url: &str, timeout: u64) -> Self {
        Config {
            url: url.to_string(),
            timeout,
        }
    }

    // 公开方法
    pub fn url(&self) -> &str {
        &self.url
    }
}

// 公开函数
pub fn load() -> Config {
    Config::new("default.example.com", 30)
}

// 私有函数，仅本模块可见
fn validate() -> bool {
    true
}
```

#### `src/protocol.rs` —— 协议模块根
```rust
// 声明子模块 tcp (从 protocol/tcp.rs 加载)
pub mod tcp;

// 公共函数
pub fn handshake() {
    println!("Protocol handshake");
    // 使用 super:: 访问父模块 (crate 根) 中的私有函数
    let val = super::internal_helper();
    println!("Helper value: {}", val);
}
```

#### `src/protocol/tcp.rs` —— TCP 子模块
```rust
// 使用绝对路径访问 crate 根下的配置模块
use crate::config::{self, Config};

// 对外公开的结构体
pub struct TcpStream {
    addr: String,
    config: Config,
}

impl TcpStream {
    // 公开构造函数
    pub fn connect(addr: &str) -> Self {
        // 使用父模块的父模块（super::super）访问 crate 根
        let cfg = super::super::config::load();  // 等价于 crate::config::load()
        TcpStream {
            addr: addr.to_string(),
            config: cfg,
        }
    }

    pub fn send(&self, data: &[u8]) -> Result<usize, std::io::Error> {
        println!("Sending {} bytes to {}", data.len(), self.addr);
        Ok(data.len())
    }
}

// 私有辅助函数
fn checksum(data: &[u8]) -> u16 {
    data.iter().fold(0u16, |a, &b| a.wrapping_add(b as u16))
}
```

---

## 各个关键字的用法说明

### `mod` 与 `pub mod`
- `mod config;` 声明私有模块。外部不能写 `use my_net::config::Config`，但 crate 内部可以。
- `pub mod protocol;` 声明公开模块，外部可以 `use my_net::protocol::tcp::TcpStream`，但通常我们用 `pub use` 简化。

### `pub` 控制可见性
- 未标记 `pub` 的函数、类型、字段都是私有的，仅在当前模块及其子模块内可见。
- 子模块可以访问父模块的私有项，但兄弟模块之间不能（除非通过 `pub(crate)` 或 `pub(super)` 折衷）。
- 示例中 `config::Config` 的字段私有，必须通过 `new()` 和 `url()` 方法访问。

### `use` 引入路径
- `use crate::config::{self, Config};` 同时引入模块和类型。
- 在 `tcp.rs` 中，如果不用 `use`，每次都要写 `crate::config::Config`，非常冗长。

### `pub use` 重新导出
- `pub use config::Config;` 将 `Config` 从私有模块 `config` 提升到 crate 根。外部用户可以直接写：
  ```rust
  use my_net::Config;   // 而不是 my_net::config::Config
  ```
- `pub use protocol::tcp::TcpStream;` 将深层类型直接暴露在 crate 根。用户只需：
  ```rust
  use my_net::TcpStream;
  ```
  完全隐藏了内部目录结构 `protocol::tcp`。
- **重新导出是构建优雅公共 API 的关键**：你可以随意重构内部模块结构，只要通过 `pub use` 保持相同的外部路径，就不会破坏调用者。

### `mod.rs` 是什么？
假设你用老风格：`protocol/mod.rs` 代替 `protocol.rs`。那么 `protocol/mod.rs` 内容与上面的 `protocol.rs` 完全相同。  
**现代推荐**：避免 `mod.rs`，因为多个 `mod.rs` 文件在 IDE 中不易区分，且 `mod.rs` 本身语义不直观。使用 `protocol.rs` + `protocol/` 目录更加清晰。

### `super::` 和 `crate::`
- `super::`：在 `protocol.rs` 中调用 `super::internal_helper()`，因为 `protocol` 的父模块是 crate 根。
- `crate::`：在 `tcp.rs` 中使用 `crate::config::load()`，无论当前模块层级多深，都能准确访问根模块。
- `super::super` 也可以从 `tcp` 访问 crate 根（`super` 到 `protocol`，再 `super` 到根），但推荐使用 `crate::` 更清晰。

---

## 外部使用示例
其他 crate 使用 `my_net` 时：
```rust
use my_net::{Config, TcpStream, init};  // 全部从根直接导入

fn main() {
    init();   // 使用公开函数
    let cfg = Config::new("example.com", 10);
    let stream = TcpStream::connect(cfg.url());
    stream.send(b"hello").unwrap();
}
```
注意：用户完全不需要知道 `config`、`protocol`、`tcp` 这些内部模块的存在。

---

## 总结表

| 概念      | 作用                                                 | 示例                            | 大白话解释                           |
| --------- | ---------------------------------------------------- | ------------------------------- | ------------------------------------ |
| `mod`     | 声明模块（从文件或内联）                             | `mod foo;` 或 `mod foo { ... }` | “我有个模块”，可以内联或找文件       |
| `pub mod` | 声明公开模块                                         | `pub mod foo;`                  | “我把这个模块公开给外面看”           |
| `pub`     | 将项（函数、类型等）标记为公开                       | `pub fn bar() {}`               | “这项（函数、类型等）是公开的”       |
| `use`     | 在当前作用域绑定路径，节省书写                       | `use crate::foo::bar;`          | “把路径缩短，方便用”                 |
| `pub use` | 重新导出，使外部可通过当前模块访问该路径             | `pub use crate::foo::Bar;`      | “我引入一个东西，顺便替它公开给别人” |
| `mod.rs`  | 老风格中表示模块根文件（现在推荐 `foo.rs` + `foo/`） | 不推荐，仅需了解遗留代码        | 老式目录入口，定义模块和子模块       |
| `super::` | 相对路径，访问父模块                                 | `super::helper()`               | “我爸爸模块里的东西”                 |
| `crate::` | 绝对路径，从 crate 根开始                            | `crate::config::load()`         | “我家根目录下的东西”                 |

------



# 宏

宏（Macro）是 Rust 中最强大的元编程工具之一，它允许你在编译时生成或转换代码。Rust 的宏系统分为两大类：**声明宏**（Declarative Macros）和**过程宏**（Procedural Macros）。理解宏机制的核心在于把握编译器对 token 流的操作过程——无论是声明宏还是过程宏，本质上都是对 `TokenStream` 的变换，而非文本替换。

---

## 1. 声明宏（Declarative Macros）

声明宏使用 `macro_rules!` 关键字定义，通过**模式匹配**的方式匹配输入的 Rust 代码，并生成相应的代码替换。声明宏也被称为"示例宏"（macros by example），其工作方式类似于 `match` 表达式——宏将输入的 token 树与若干条规则进行匹配，匹配成功后将替换为该规则对应的输出代码。

### (1) 基本语法

声明宏的定义包含一个名称和一条或多条规则，每条规则由**匹配器**（matcher）和**转换器**（transcriber）组成：

```rust
macro_rules! macro_name {
    // 规则1: 匹配器 => 转换器
    (matcher1) => {
        transcriber1
    };
    // 规则2: 匹配器 => 转换器
    (matcher2) => {
        transcriber2
    };
    // 更多规则...
}
```

- **`macro_rules!`**：声明宏定义的内置宏。
- **匹配器（matcher）**：描述希望匹配的语法模式，可以包含字面 token、元变量和重复模式。
- **转换器（transcriber）**：匹配成功后生成的代码，可以引用匹配器中绑定的元变量。
- 规则的匹配顺序与 `match` 表达式相同，**从上到下依次尝试，一旦匹配成功就立即展开，不再尝试后续规则**。

匹配器和转换器的定界符可以使用 `()`、`[]` 或 `{}`，三种风格都是合法的，可根据场景选择。

### (2) 元变量与片段分类符

在匹配器中，`$name:fragment` 用于匹配符合指定语法的 Rust 代码片段，并将其绑定到元变量 `$name` 上。`fragment` 指定了该元变量可以匹配的语法类型，称为**片段分类符**（Fragment Specifier）。

常用的片段分类符包括：

| 分类符     | 匹配内容                                     | 示例                                                         |
| :--------- | :------------------------------------------- | :----------------------------------------------------------- |
| `ident`    | 标识符或关键字                               | `x`, `my_func`, `Result` (但不能匹配 `crate`, `self`, `super`, `Self` 这几个特殊标识符) |
| `expr`     | 表达式                                       | `1 + 2`, `vec![1, 2]`, `x > y`, 以及不带尾随分号的块 `{ let x = 1; x + 2 }` |
| `ty`       | 类型                                         | `i32`, `Vec<String>`, `&'static str`                         |
| `path`     | 路径                                         | `std::io::Write`, `crate::foo::bar`                          |
| `pat`      | 模式                                         | `Some(x)`, `a@1..=5`, `(x, y)`                               |
| `stmt`     | 语句（注意: 匹配器中的分号必须显式写出）     | `let x = 5;`, `x = y + 2;`                                   |
| `block`    | 块表达式                                     | `{ let x = 1; x + 2 }`                                       |
| `item`     | 程序项（模块项、函数、结构体、枚举等）       | `fn foo() {}`, `struct Bar;`, `use std::io;`                 |
| `tt`       | Token 树（单个 token 或括号内的 token 序列） | 任何单个 token 或 `()`, `[]`, `{}` 内的内容，是最通用的分类符 |
| `literal`  | 字面量表达式（包含前缀 `-` 的整数）          | `"hello"`, `42`, `-5`, `b'a'`                                |
| `meta`     | 属性（attribute）中的内容                    | `derive(Debug)`, `cfg(target_os = "linux")`                  |
| `lifetime` | 生命周期 token                               | `'a`, `'static`, `'_`                                        |
| `vis`      | 可见性限定符（可能为空）                     | `pub`, `pub(crate)`, `pub(super)`                            |

```rust
macro_rules! example {
    // 匹配一个标识符和一个类型
    ($name:ident, $type:ty) => {
        let $name: $type;
    };
}

example!(counter, i32);  // 展开为: let counter: i32;
```

**关于 `stmt` 分类符的重要细节**：`stmt` 分类符匹配语句（如 `let x = 5;`），但匹配器中的分号会被视为常规分隔符而非语句的一部分。当在 `stmt` 分类符后紧接分号时，该分号被解释为重复模式的分隔符（见下文），而非语句的结尾。

**特殊元变量 `$crate`**：在声明宏内部，`$crate` 是一个特殊的元变量，它会展开为定义该宏的 crate 的路径。这样，即使宏在其他 crate 中被调用，宏内部引用的该 crate 的其他项也能正确解析。

### (3) 重复模式（Repetition）

声明宏通过 `$( ... )` 加上重复运算符来处理可变数量的输入。这是声明宏最强大的特性之一，也是标准库 `vec!` 宏能够接受任意数量元素的基石。

**重复运算符**：

| 运算符 | 含义           | 示例                                           |
| :----- | :------------- | :--------------------------------------------- |
| `*`    | 匹配零次或多次 | `my_macro!()` 和 `my_macro!(1, 2, 3)` 均可匹配 |
| `+`    | 匹配一次或多次 | 至少需要一次                                   |
| `?`    | 匹配零次或一次 | 用于处理可选参数                               |

在 `$( ... )` 和运算符之间可以放置一个**分隔符**（separator token），最常用的是逗号（`,`）和分号（`;`）。例如 `$( $x:expr ),*` 表示用逗号分隔的零个或多个表达式。

```rust
macro_rules! my_vec {
    // 匹配空列表
    () => {
        Vec::new()
    };
    // 匹配一个或多个表达式，用逗号分隔
    ($($element:expr),+) => {
        {
            let mut vec = Vec::new();
            $( vec.push($element); )+
            vec
        }
    };
}
```

重复模式可以**嵌套使用**，以处理更加复杂的结构，例如匹配键值对列表：

```rust
macro_rules! map {
    // 匹配零个或多个键值对，格式为 $key => $value，键值对之间用逗号分隔
    ($($key:expr => $value:expr),* $(,)?) => {{
        let mut map = std::collections::HashMap::new();
        $( map.insert($key, $value); )*
        map
    }};
}

let map = map! {
    "a" => 1,
    "b" => 2,
};
```

上述示例中使用了 `$(,)?` 来处理可选的尾随逗号——这是 Rust 2018 风格中常见的特性，可以提升用户体验。

`?` 运算符的一个常见应用场景是处理可选的尾随分隔符：

```rust
macro_rules! vec_with_optional_trailing_comma {
    ($($x:expr),* $(,)?) => {{
        let mut v = Vec::new();
        $( v.push($x); )*
        v
    }};
}

// 以下三种调用都合法
let v1 = vec_with_optional_trailing_comma![];
let v2 = vec_with_optional_trailing_comma![1, 2, 3];
let v3 = vec_with_optional_trailing_comma![1, 2, 3,];  // 尾随逗号被 ? 处理
```

### (4) 卫生性（Hygiene）

Rust 的声明宏是**卫生宏**（hygienic macros），编译器会自动处理变量名和作用域，确保宏展开后不会意外捕获或破坏外部变量，**得益于`{}`求值表达式返回，变量遮蔽特性**。

对比 C/C++ 的宏（纯文本替换）：

```c
// C 语言宏的问题示例
#define ADD(a, b) a + b
int result = ADD(1 + 2, 3 + 4) * 2;
// 展开为: 1 + 2 + 3 + 4 * 2 = 15 （而非预期的 12）
```

Rust 的卫生宏会正确处理：

```rust
macro_rules! add {
    ($a:expr, $b:expr) => { $a + $b };
}
let result = add!(1 + 2, 3 + 4) * 2;  // 展开为: (1 + 2 + 3 + 4) * 2 = 20
// 编译器自动添加了括号，确保计算顺序符合预期
```

卫生宏还意味着宏内部定义的**标识符**（如中间变量名）不会泄露到调用作用域，宏外部的标识符也不会意外地被宏内部的同名标识符覆盖。这一特性依赖于每个标识符的 `Span` 信息——编译器为不同的语法上下文赋予不同的 `Span`，从而在展开时进行作用域隔离。

### (5) 使用 `cargo expand` 观测宏展开

调试宏的最佳方法是使用 `cargo expand` 工具查看宏展开后的代码。安装方式：

```bash
cargo install cargo-expand
```

该工具依赖 nightly 工具链，即便默认工具链不是 nightly 也会自动寻找并使用它。此外，工具会调用 `rustfmt` 格式化展开后的代码，使结果更易读。若未安装 `rustfmt`，可通过 `rustup component add rustfmt` 安装。

**使用示例**：

```bash
# 展开整个 crate
cargo expand

# 展开特定的测试目标
cargo expand --test test_something

# 不加格式化（更紧凑的输出）
cargo expand --ugly

# 仅展开指定的模块、类型或函数
cargo expand path::to::module
```

查看以下代码的展开结果：

```rust
macro_rules! add {
    ($a:expr, $b:expr) => { $a + $b };
}

fn main() {
    let a = 10;
    let b = 22;
    let _res1 = add!(a, b);
    let _res2 = add!(a + 1, b);
    let _res3 = add!(a * 2, b + 3);
}
```

执行 `cargo expand` 后的输出：

```rust
fn main() {
    let a = 10;
    let b = 22;
    let _res1 = a + b;
    let _res2 = a + 1 + b;
    let _res3 = a * 2 + (b + 3);
}
```

可以看到，展开后的代码中表达式被正确处理，且 `b + 3` 被自动添加了括号。

**注意事项**：由于展开过程涉及标识符名称重写（hygiene）和 span 调整，将展开后的代码视为可编译的独立代码通常是不安全的，应当将其仅作为调试辅助手段。例如，某些只有在宏展开上下文中才合法的构造在展开后可能无法编译。

### (6) 导出与作用域

默认情况下，声明宏只在定义它的模块内部可见。要使宏在模块外（包括 crate 外部）可用，需要使用 `#[macro_export]` 属性。

```rust
// lib.rs
#[macro_export]
macro_rules! my_macro {
    () => {
        println!("Hello from macro!");
    };
}
```

**作用域的重要特性**：

- 使用 `#[macro_export]` 导出后，宏会被添加到 crate 的根命名空间，无论你从哪个模块导出，宏都位于 crate 的根下，称为"全局可用"。
- 在其他 crate 中使用时，无需通过 `crate_name::my_macro!()` 形式调用，可直接写 `my_macro!()`。
- `#[macro_export]` 的宏定义会在 crate 根和所有模块中生效，但不支持指定可见性修饰符（如 `pub`）。

在 crate 内部，可以通过 `#[macro_use]` 属性将其他模块的宏导入到当前作用域：

```rust
#[macro_use]
mod inner {
    macro_rules! inner_macro {
        () => { println!("inner"); };
    }
}

fn main() {
    inner_macro!();  // 可以调用
}
```

**使用场景提示**：

- 当需要在多个不同的模块或 crate 中共享同一个宏定义时，在定义处添加 `#[macro_export]`
- 当需要将 crate 内部辅助宏暴露给同一 crate 的其他模块但不对外公开时，使用 `#[macro_use]` 导入即可，无需添加 `#[macro_export]`

### (7) 递归宏

声明宏支持**递归调用**，这使得处理嵌套结构变得简单。例如，实现一个计算表达式树中的最大值等逻辑的标准库 `vec!` 宏本质上是非递归的；但我们也可以构造递归宏来处理更复杂的数据结构：

```rust
macro_rules! sum {
    // 终止条件：单个表达式
    ($a:expr) => { $a };
    // 递归步骤：提取第一个表达式，对剩余部分继续求和
    ($a:expr, $($rest:expr),*) => {
        $a + sum!($($rest),*)
    };
}

fn main() {
    let result = sum!(1, 2, 3, 4);  // 展开为 1 + 2 + 3 + 4 = 10
    println!("{}", result);
}
```

**递归宏的核心要素**：必须有一个明确的**终止规则**（基例），否则递归永远不会结束，编译器会报告递归展开深度超过限制。

### (8) 与函数的区别

| 特性     | 宏                             | 函数                              |
| :------- | :----------------------------- | :-------------------------------- |
| 定义时机 | 调用**前**必须定义或引入作用域 | 可在任何位置定义和调用            |
| 可变参数 | 支持（通过重复模式）           | 不支持（Rust 函数不支持可变参数） |
| 代码生成 | 编译时                         | 运行时                            |
| 卫生性   | 是（卫生宏）                   | 不适用                            |
| 调试难度 | 较高（可使用 `cargo expand`）  | 较低（标准调试器）                |

### (9) 高级模式：标识符拼接与元变量特性

声明宏允许通过 `concat!` 等宏组合动态生成标识符，但元变量本身不能直接被拼接。以下是一个生成 getter/setter 方法的宏：

```rust
macro_rules! impl_getter {
    ($struct_name:ident, $field:ident, $type:ty) => {
        impl $struct_name {
            pub fn $field(&self) -> &$type {
                &self.$field
            }
        }
    };
}
```

一个更复杂的示例——生成多个结构体及其 `new` 方法：

```rust
macro_rules! make_struct_with_new {
    (
        $( struct $struct_name:ident {
            $($field:ident : $field_type:ty),* $(,)?
        } ),* $(,)?
    ) => {
        $(
            struct $struct_name {
                $($field: $field_type),*
            }
            impl $struct_name {
                fn new($($field: $field_type),*) -> Self {
                    Self {
                        $($field),*
                    }
                }
            }
        )*
    };
}
```

这个宏的匹配器展示了重复模式的嵌套结构：`*,?` 用于处理每种分隔符的可选尾随逗号，最外层重复模式处理多个结构体定义，内层重复模式处理每个结构体中的多个字段。

**捕获注意事项**：元变量捕获的 token 序列会被隐式地分组（好比被不可见的括号包裹），这会影响元变量在上下文中的解析方式，因此不能简单地将任意元变量直接拼接到不同的语法位置中。

### (10) 运算符优先级处理

声明宏在展开时会自动处理运算符优先级，为生成的代码添加必要的括号，因此无需像 C 语言宏那样手动添加括号来避免优先级问题。这与声明宏在 token 级别而非文本级别上操作有关——Rust 的宏扩展发生在语法分析之后，拥有完整的 AST 上下文。

```rust
macro_rules! square {
    ($x:expr) => { $x * $x };
}

let n = square!(1 + 2);   // 展开为 (1 + 2) * (1 + 2) = 9
// 而不是 1 + 2 * 1 + 2 = 5
```

### (11) 标准库宏解析

#### ① map

```
macro_rules! map {
    ( $($key:expr => $value:expr),* $(,)? ) => {
        {
            let mut m = ::std::collections::HashMap::new();
            $( m.insert($key, $value); )*
            m
        }
    };
}
```

##### 匹配模式分析

`$($key:expr => $value:expr),*` 匹配零个或多个 `表达式 => 表达式` 对，对之间用逗号分隔。

例如：

- `map!{}` → 匹配成功（0 对）
- `map!{ "a" => 1 }` → 匹配成功
- `map!{ "a" => 1, "b" => 2 }` → 匹配成功

##### 末尾的 `$(,)?` 是什么意思？

`$(,)?` 表示：零个或一个逗号。
它允许用户**在最后一个键值对后面也写一个逗号**。这是 Rust 中常见的风格（数组、结构体等允许尾随逗号）。
`?` 是重复次数：0 或 1 次。`*` 是 0 或多次，`+` 是 1 或多次。

为什么不能写成 `, *`？
因为 `, *` 会被解析为“逗号后面跟一个 `*` 运算符”，不符合重复模式语法。重复模式必须用 `$( ... )` 包裹，并且 `?` / `*` / `+` 紧跟在 `)` 后面。

所以 `$(,)?` 整体表示：**可选的一个逗号**。它和前面的 `$($key:expr => $value:expr),*` 是**并列**的两个重复模式，中间没有分隔符。Rust 允许在 `macro_rules!` 的模式里连续写多个重复段，只要它们能无歧义匹配。

实际匹配例子：

- `map!{ "a" => 1, }` → 前面的 `$($key:expr => $value:expr),*` 匹配了 `"a" => 1`，然后 `$(,)?` 匹配了那个多余的逗号。
- 如果没有 `$(,)?`，`map!{ "a" => 1, }` 会匹配失败，因为逗号没有对应的表达式了。

##### 生成代码

和 `my_vec!` 非常相似：创建一个空的 `HashMap`，然后对每个捕获到的 `($key, $value)` 对生成一个 `m.insert($key, $value);` 语句，最后返回 `m`。

------

##### 为什么 `$($elem:expr),*` 要包一层 `$()`？能不能直接写 `$elem:expr,*`？

不能。因为 `$elem:expr` 只是一个元变量，它代表**单个**表达式。
要表示“重复多个”，就必须用 `$( ... )` 作为容器，里面放你想重复的模式（可以包含多个元变量和固定符号）。
`$($elem:expr),*` 的解析是：

- `$(` 开始一个重复块
- 里面是一个模式 `$elem:expr`
- `),` 表示重复块之间的分隔符是逗号
- `*` 表示重复次数

这是 Rust 宏系统的固定语法，借鉴自 Scheme 等语言的语法模式。

#### ② vec

```
macro_rules! vec {
    // 规则1：匹配 my_vec![1, 2, 3] 这种逗号列表
    ($($elem:expr),*) => {
        {
            let mut v = Vec::new();
            $( v.push($elem); )*
            v
        }
    };
    
    // 规则2：匹配 my_vec![0; 5] 这种重复初始化
    ($elem:expr; $n:expr) => {
        std::vec::from_elem($elem, $n)
    };
}
```

##### 规则1 详解

- **匹配模式** `($($elem:expr),*)`
  它匹配零个或多个表达式，用逗号分隔。例如：

  - `my_vec![]` → 匹配成功，`$elem` 没有值（重复次数为 0）
  - `my_vec![1]` → 匹配成功，`$elem` 捕获 `1`
  - `my_vec![1, 2, "hello"]` → 匹配成功，`$elem` 捕获了 3 个表达式（分别是 `1`, `2`, `"hello"`）

- **生成代码**（右边部分）

  rust

  ```
  {
      let mut v = Vec::new();
      $( v.push($elem); )*
      v
  }
  ```

  

  - 外层 `{ ... }` 是一个块表达式。在 Rust 中，块内最后一个表达式（不加分号）就是块的值。
  - 所以 `v` 作为最后一个表达式，会把创建好的 `Vec` 作为宏调用结果返回。
  - `$( v.push($elem); )*` 表示：对于每个捕获到的 `$elem`，生成一条 `v.push(该表达式);` 语句。重复次数与左边捕获的表达式个数相同。

  举例：调用 `my_vec![1, 2, 3]` 展开后得到：

  rust

  ```
  {
      let mut v = Vec::new();
      v.push(1);
      v.push(2);
      v.push(3);
      v
  }
  ```

  

**为什么需要一个 `v`？**
因为我们要构造一个 `Vec` 并返回它。没有 `v` 的话，块的值就是最后一个语句 `v.push(...)` 的值，那是 `()` 不是 `Vec`。所以先创建 `v`，最后单独写一个 `v` 作为返回值。

**为什么 `$( v.push($elem); )\*` 里面没有逗号？**
因为这里不是生成列表，而是生成多条语句，每条以分号结尾。重复模式的分隔符 `,` 只在**左边模式**中用来解析用户输入的列表；右边的展开可以自由安排。

##### 规则2 详解

`($elem:expr; $n:expr)` 匹配类似 `my_vec![0; 5]` 的写法。
右边直接用标准库的 `std::vec::from_elem` 来创建重复元素的 `Vec`，这是一种更高效的实现。

---

## 2. 过程宏（Procedural Macros）

过程宏是一种更强大的宏形式，它允许在编译时执行 Rust 代码，操作输入 token 流（`TokenStream`），并生成新的 token 流。过程宏分为三种类型：派生宏、类属性宏和类函数宏。

**过程宏的核心约束**：过程宏必须定义在**独立的 `proc-macro` 类型的 crate 中**，因为过程宏是编译时插件，需要被编译器单独编译和加载。

在 `Cargo.toml` 中配置过程宏 crate：

```toml
[lib]
proc-macro = true

[dependencies]
syn = "2.0"
quote = "1.0"
```

**`proc_macro` crate 与核心 API**：过程宏 crate 必须依赖编译器提供的 `proc_macro` crate，它提供了 `TokenStream` 类型（token 序列）以及 `Ident`、`Literal`、`Span` 等基础类型。过程宏的本质是在 token 流上进行操作，而不是直接操作 AST，这保证了接口的稳定性。所有类型的 token 都有一个关联的 `Span`，用于标识源代码范围，主要用于错误报告。我们可以通过 `syn` 库将 `TokenStream` 解析为 Rust AST，通过 `quote` 库将 AST 重新生成 `TokenStream`。

### (1) 派生宏（Derive Macro）

派生宏通过 `#[derive(...)]` 语法为结构体、枚举等类型自动实现 trait。派生宏的处理器函数需要添加 `#[proc_macro_derive]` 属性。

**标准库示例：`#[derive(Debug)]`**

当编写 `#[derive(Debug)] struct Point { x: i32, y: i32 }` 时，`Debug` 派生宏会为 `Point` 自动生成 `fmt::Debug` trait 的实现代码，使用类似的过程宏内部逻辑。

**自定义派生宏示例（标准库 Serde 的派生宏）**：

Serde 库通过派生宏 `#[derive(Serialize, Deserialize)]` 为类型自动生成序列化和反序列化的实现，极大地减少了样板代码。`Serialize` 派生宏会读取结构体字段的类型信息，在编译时生成与该类型匹配的序列化逻辑。

```rust
use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize)]
struct Person {
    name: String,
    age: u8,
    email: String,
}
```

只需这一个派生宏，`Person` 结构体即可自动支持 JSON、YAML、Postcard 等多种数据格式的序列化和反序列化。Serde 派生宏内部的工作方式大致为：`Serialize` 派生宏遍历输入类型的所有字段，对于每个字段，根据其类型生成相应的序列化调用，最终输出完整的 trait 实现代码。

**派生宏的辅助属性（helper attributes）**：派生宏可以定义自己的辅助属性，允许用户在标注派生宏时对代码生成过程进行精细化控制。例如 Serde 的 `#[serde(rename_all = "camelCase")]`，这部分属性的处理逻辑完全由派生宏实现。

**`#[proc_macro_derive]` 的基本结构**：

```rust
use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, DeriveInput};

#[proc_macro_derive(MyTrait)]
pub fn my_trait_derive(input: TokenStream) -> TokenStream {
    // 解析输入的 TokenStream 为 AST
    let ast = parse_macro_input!(input as DeriveInput);
    let name = &ast.ident;
    // 生成实现代码
    let gen = quote! {
        impl MyTrait for #name {
            fn my_method(&self) {
                println!("MyTrait implementation for {}", stringify!(#name));
            }
        }
    };
    gen.into()
}
```

### (2) 类属性宏（Attribute Macro）

类属性宏允许自定义属性，可以附加到函数、结构体、模块等代码项上，在编译时对代码进行修改或增强。

**标准库示例：`#[test]` 属性宏**

Rust 的 `#[test]` 属性宏会将标注的函数包裹在测试运行器的调用上下文中——该属性宏修改函数的签名和内容，将其注册到测试框架中，从而在 `cargo test` 命令执行时被识别和执行。

**类属性宏定义**：

```rust
use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, ItemFn};

#[proc_macro_attribute]
pub fn my_attribute(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let input = parse_macro_input!(item as ItemFn);
    let fn_name = &input.sig.ident;
    let expanded = quote! {
        #input
        fn main() {
            println!("Before calling {}...", stringify!(#fn_name));
            #fn_name();
            println!("After calling {}!", stringify!(#fn_name));
        }
    };
    TokenStream::from(expanded)
}
```

**使用示例**：

```rust
#[my_attribute]
fn say_hello() {
    println!("Hello from say_hello!");
}
```

**属性宏的常见应用场景**：

- **Web 框架路由定义**：如 `#[get("/")]`，接收路径作为属性参数，生成路由注册代码。
- **测试框架**：`#[test]` 属性宏执行测试注册。
- **日志/追踪**：`#[instrument]` 自动为函数添加 tracing 日志。
- **Tauri 命令定义**：`#[tauri::command]` 将普通函数暴露给前端调用。

**属性参数的处理**：属性宏的第一个参数 `_attr: TokenStream` 接收属性括号内的内容（如 `#[my_attr(param1, param2)]` 中的 `param1, param2`）。可以通过 `syn` 进一步解析这些参数，例如解析为 `Meta`、`LitStr` 等类型，根据参数值改变生成的代码行为。

### (3) 类函数宏（Function-like Macro）

类函数宏看起来像普通函数调用，但它的输入和输出都是 `TokenStream`，在编译时执行。

**标准库示例：`println!` 与 `format!`**

当调用 `println!("Hello, {}!", name)` 时，`println!` 宏会解析格式字符串和参数列表，在编译时生成相应的格式化代码，并调用标准输出。不同于普通函数，`println!` 支持可变数量的参数，且在编译时检查格式字符串与参数类型的匹配关系。

**类函数宏定义**：

```rust
use proc_macro::TokenStream;
use quote::quote;

#[proc_macro]
pub fn make_answer(_item: TokenStream) -> TokenStream {
    quote! {
        42
    }.into()
}
```

**使用示例**：

```rust
fn main() {
    let answer = make_answer!();
    println!("{}", answer);  // 输出 42
}
```

**类函数宏与声明宏的区别**：

| 特性     | 类函数过程宏                      | 声明宏                     |
| :------- | :-------------------------------- | :------------------------- |
| 输入     | TokenStream（任意 token 序列）    | 必须符合匹配器中的语法模式 |
| 解析能力 | 可以执行任意 Rust 代码解析 AST    | 基于模式匹配               |
| 复杂度   | 可实现任意复杂逻辑                | 受限于模式匹配表达能力     |
| 调用语法 | `foo!()`、`foo![]`、`foo!{}` 均可 | 同样支持多种定界符         |

类函数宏常用于**实现自定义 DSL**、**编译时计算**和**代码注入**等场景。例如，`include_str!` 和 `include_bytes!` 是标准库中的类函数宏，它们能够在编译时将外部文件的内容嵌入到二进制中。

### (4) 过程宏开发的最佳实践与调试

1. **使用 `syn` 和 `quote` 库**：几乎所有 Rust 过程宏都依赖于 `syn`（解析 Rust 代码为 AST）和 `quote`（根据 AST 生成 TokenStream）。

   - `syn` 提供了 `parse_macro_input!` 宏和 `DeriveInput`、`ItemFn`、`ItemStruct`、`ItemEnum` 等 AST 类型，支持完整解析 Rust 语法。
   - `quote!` 宏允许在代码中使用 `#var` 语法插入变量，极大地简化了代码生成过程。

2. **错误处理**：过程宏中应尽量使用 `syn::parse_macro_input!` 在解析失败时提供准确的错误信息，而不是直接 `panic!`。也可以使用 `syn::Error` 在解析后进行精确的错误报告。`proc_macro::Diagnostic` 类型可以在 nightly 环境下提供更丰富的诊断信息（包括警告和帮助信息）。

3. **使用 `cargo expand`** 调试过程宏的输出。

4. **测试宏**：在 `tests/` 目录下编写测试用例，构建一个包含宏调用的测试 crate，然后使用 `cargo expand` 验证输出，或结合 `trybuild`、`cargo test` 进行自动化回归测试。

5. **避免过度使用**：宏虽然强大，但会使代码难以阅读和调试。在可以用函数或泛型解决的场景，优先使用这些常规语言特性。官方文档中也强调，编写良好的宏应保持简单、有良好的注释，并在复杂场景下优先选择 `syn` 和 `quote` 等成熟库。

6. **Span 的精细控制**：`Span` 携带源代码位置信息，合理设置输出 token 的 `Span` 可以让编译器在报错时指向宏调用处而不是宏展开内部。`quote_spanned!` 宏允许为生成的每个 token 指定特定的 Span，这对提升用户体验非常有帮助。

---

## 宏速查表

| 宏类型   | 关键字/属性               | 调用方式                          | 主要用途                   |
| :------- | :------------------------ | :-------------------------------- | :------------------------- |
| 声明宏   | `macro_rules!`            | `name!()` / `name![]` / `name!{}` | 模式匹配代码生成，可变参数 |
| 派生宏   | `#[proc_macro_derive]`    | `#[derive(Trait)]`                | 自动实现 trait             |
| 类属性宏 | `#[proc_macro_attribute]` | `#[attribute]`                    | 为代码项添加属性，修改代码 |
| 类函数宏 | `#[proc_macro]`           | `name!()`                         | 编译时函数式代码生成       |

# 杂谈——解引用

你感到混乱很正常，因为 Rust 的引用和解引用规则很精细，但大部分是自动的。我把**所有场景**分成两大类：**自动发生**的和**需要手动写 `*`** 的。

## 一、自动解引用（编译器替你加 `*`）

自动解引用只在几种特定情况下发生，不需要你写 `*`。

### 1. 方法调用时（最常遇到）

当你用 `.` 调用方法时，Rust 会**自动解引用**（以及自动引用）来满足方法签名。

- 如果方法需要 `&self`，编译器会自动加 `&`。
- 如果方法需要 `self`，但你有 `&T`，编译器会尝试自动解引用（通过 `Deref`）去拿到 `T`。

```rust
let x = Box::new(42);
// Box<i32> 没有 .abs() 方法，但 i32 有。
// 编译器会：先解引用 Box → i32，然后调用 i32::abs
let y = x.abs();   // 等价于 (*x).abs()
```

**规则**：编译器会沿着 `Deref` 链一直解引用，直到能找到那个方法。

### 2. 强制多态（Deref coercion）

在期望 `&T` 的地方，如果你提供了 `&U`，且 `U` 实现了 `Deref<Target=T>`，则自动将 `&U` 转换为 `&T`。

```rust
fn take_ref(s: &str) { ... }
let s = String::from("hello");
take_ref(&s);   // &String 自动变成了 &str，因为 String 实现了 Deref<Target=str>
```

这也属于自动解引用（把 `String` 解引用为 `str`）。

### 3. 运算符重载（`+`、`==`、`*` 等）

标准库为 `&T` 实现了很多 `trait`（例如 `Add`）。当你使用运算符时，编译器会根据需要自动解引用。

```rust
let a = Box::new(10);
let b = Box::new(20);
let c = a + b;   // 自动解引用 Box 里的 i32，然后调用 i32::add
```

### 4. 索引操作符 `[]`

`index` 方法通常接受 `&self`，当你使用 `container[idx]` 时，编译器会自动解引用容器（如果有必要）并调用 `Index::index`。

```rust
let v = vec![1, 2, 3];
let x = v[0];      // v 没有实现 Index，但 Vec 实现了，不会自动解引用 v 本身。
                   // 这里只是举例：v 不会自动解引用，但返回的值可能会自动解引用？
// 更明显的智能指针：
let b = Box::new([1, 2, 3]);
let y = b[1];      // 自动解引用 Box 到数组，然后索引
```

### 5. `match` 和 `if let` 的模式中，`&` 模式会**自动解引用**（反直觉？）

当你匹配一个引用时，Rust 会自动解引用，但你需要用 `&` 模式来捕获。

```rust
let x = &42;
match x {
    &val => println!("{}", val),  // val 是 i32，这里 & 模式解引用了 x
}
```

实际上这个“解引用”是模式匹配的功能，不是运算符自动解引用。但你可以认为编译器帮你做了解引用。

### 6. `?` 运算符对 `Result`/`Option` 的转换（涉及解引用？不直接）

`?` 会调用 `From::from` 和 `Into::into`，可能会涉及 `Deref` 强制转换，但通常不直接称为自动解引用。

## 二、需要手动加 `*` 的场景

当你**明确想要拿到引用背后的值**时，需要显式解引用。

### 1. 从引用或智能指针中取出值（拷贝或移动）

```rust
let r = &42;
let v = *r;            // v 现在是 42

let b = Box::new(5);
let x = *b;            // x 现在是 5（移动了值，因为 i32 是 Copy 所以是拷贝）
```

### 2. 调用接受 `self` 而不是 `&self` 的方法

如果方法需要 `self`（取得所有权），而你只有一个引用，就需要先解引用。

```rust
let s = String::from("hello");
let r = &s;
// r 不能调用 into_bytes()，因为 into_bytes 需要 self
let bytes = (*r).into_bytes();   // 先解引用得到 s，再移动调用
```

但通常你会直接使用 `s.into_bytes()` 或者 `r.clone().into_bytes()`。

### 3. 赋值给非引用的变量时，需要解引用

```rust
let mut x = 10;
let y = &mut x;
*y = 20;               // 需要 *y 才能修改 x
```

### 4. 运算符需要值时（而不是引用时）

虽然大多数运算符会自动解引用，但如果你写 `*a + *b` 这种，那是你手动决定解引用。但自动场景已经覆盖了。

### 5. 模式匹配中显式解引用

```rust
let x = &Some(42);
match x {
    &Some(v) => println!("{}", v),  // & 模式解引用
    // 或者
    Some(v) => ...,   // 如果 x 是 &Option，这里会报错，需要 &Some
}
```

也可以使用 `ref` 和 `ref mut`，但那是取引用，反过来。

### 6. 作为函数参数传递时，如果函数需要 T 而你只有 &T，通常需要手动解引用（或者用 clone）

```rust
fn takes_ownership(x: String) {}
let s = String::from("hi");
let rs = &s;
// takes_ownership(rs);   // 错误
takes_ownership((*rs).clone());   // 手动解引用并克隆
```

## 三、总结表：常见类型与是否需要手动 `*`

| 类型 / 场景                     | 自动解引用？            | 何时需要手动 `*`                                             |
| ------------------------------- | ----------------------- | ------------------------------------------------------------ |
| 调用方法 (`.method()`)          | ✅ 自动（通过 Deref 链） | 基本不需要                                                   |
| 强制多态 (`&T` 传给 `&U`)       | ✅ 自动                  | 不                                                           |
| 运算符 (`+`, `==` 等)           | ✅ 自动                  | 极少，除非想强制按值运算                                     |
| 索引 `[]`                       | ✅ 自动（容器解引用）    | 不                                                           |
| 模式匹配中的 `&`                | ✅ 部分（匹配时自动）    | 用 `*` 也可以，但不常用                                      |
| 读取引用后的值 (`let v = *r`)   | ❌ 必须手动              | 需要 `*`                                                     |
| 修改可变引用的值 (`*ptr = val`) | ❌ 必须手动              | 需要 `*`                                                     |
| 调用 `self` 方法（移动）        | ❌ 手动                  | 需要 `*`                                                     |
| 函数参数（期望 `T`）            | ❌ 手动                  | 需要 `*` + 克隆或移动                                        |
| 比较引用与值 (`if *r == 5`)     | ❌ 手动                  | 需要 `*`（除非自动解引用已触发，但比较时两边都是引用会自动解引用？实际上 `==` 会调用 `PartialEq`，`&T` 实现了 `PartialEq<T>`，所以可以直接 `r == &5`。这里容易混。） |

> **关键记忆点**：  
> - **`.` 和运算符**：编译器尽量帮你自动解引用，让你少写 `*`。  
> - **赋值和传值（想要拿到值本身）**：必须写 `*`。  
> - **方法调用**：几乎永远不需要写 `*`，编译器会沿着 `Deref` 链找。

如果你遇到编译错误说“类型不匹配”，先想一想你是想用**引用**还是**值**，然后决定要不要手动 `*`。用多了自然会形成直觉。