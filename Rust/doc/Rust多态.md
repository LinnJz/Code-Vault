在 Rust 中，虽然确实没有传统的面向对象继承，但多态（polymorphism）不仅可以实现，而且有多种非常强大且安全的方式。你提到的“接口依赖注入”的思路，在 Rust 中是通过 **Trait**（特质）来实现的，这正是 Rust 实现多态的核心。

Rust 的多态主要分为两种：**静态分发**（Static Dispatch）和 **动态分发**（Dynamic Dispatch）。

---

### 1. 静态分发（通过泛型 + Trait Bound）

这是 Rust 中最常用的多态形式。它类似于 C++ 的模板，在编译时确定具体类型，通过**单态化**（Monomorphization）为每个使用的具体类型生成一份独立的代码。

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

### 2. 动态分发（通过 Trait 对象）

当需要在运行时确定具体类型（例如在集合中存放不同的类型，或者实现真正的依赖注入），就需要动态分发。使用 `&dyn Trait` 或 `Box<dyn Trait>`。

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

### 3. “依赖注入”在 Rust 中的实现

你提到的“接口依赖注入”在 Rust 中非常自然，通常通过**泛型**或**Trait 对象**作为结构体字段来实现。

#### 方式一：泛型参数（静态注入，零开销）

```rust
trait Logger {
    fn log(&self, msg: &str);
}

struct ConsoleLogger;
impl Logger for ConsoleLogger {
    fn log(&self, msg: &str) { println!("[LOG] {}", msg); }
}

struct Application<L: Logger> {
    logger: L,
}

impl<L: Logger> Application<L> {
    fn new(logger: L) -> Self {
        Application { logger }
    }
    fn do_work(&self) {
        self.logger.log("Working...");
    }
}

fn main() {
    let app = Application::new(ConsoleLogger);
    app.do_work();
}
```

#### 方式二：Trait 对象（动态注入，更灵活）

```rust
// 使用上面的 Logger trait 和 ConsoleLogger

struct Application {
    logger: Box<dyn Logger>, // 动态分发
}

impl Application {
    fn new(logger: Box<dyn Logger>) -> Self {
        Application { logger }
    }
    fn do_work(&self) {
        self.logger.log("Working...");
    }
}

// 可以轻松替换实现
struct FileLogger;
impl Logger for FileLogger {
    fn log(&self, msg: &str) { println!("[FILE] {}", msg); }
}

fn main() {
    let app = Application::new(Box::new(ConsoleLogger));
    app.do_work();
    
    let app2 = Application::new(Box::new(FileLogger));
    app2.do_work();
}
```

### 总结对比

| 特性         | 继承 + 虚方法 (Java/C++)   | Rust 静态分发      | Rust 动态分发            |
| ------------ | -------------------------- | ------------------ | ------------------------ |
| **多态方式** | 继承基类，重写虚函数       | 泛型 + Trait Bound | Trait 对象 (`dyn Trait`) |
| **绑定时机** | 运行时（虚表）             | 编译时             | 运行时                   |
| **性能**     | 有虚函数开销               | 零开销，可内联     | 有虚表开销，不可内联     |
| **灵活性**   | 较高（可运行时替换）       | 低（编译时固定）   | 高                       |
| **代码体积** | 一份代码                   | 多份单态化代码     | 一份代码                 |
| **安全性**   | 可能继承错误，易出菱形问题 | 编译期完全检查     | 编译期完全检查           |

### 核心思想

Rust 的哲学是 **“组合优于继承”** 和 **“编译期零成本抽象”**。通过 **Trait** 定义行为契约，通过**泛型**实现静态多态（高性能），通过 **Trait 对象**实现动态多态（灵活性）。依赖注入也完全基于这套系统，要么在编译期注入具体类型（泛型），要么在运行时注入 Trait 对象。

这种方式避免了传统 OOP 中复杂的继承层级、脆弱的基类问题，同时保持了极高的性能和安全性。你完全可以像使用接口一样使用 Trait，只是在实现多态时需要显式选择静态或动态分发。