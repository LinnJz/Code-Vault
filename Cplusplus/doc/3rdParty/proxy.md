# MS proxy v4

# 接口

根据提供的 `proxy.h` 和 `proxy_macros.h` 头文件，`pro::v4` 命名空间及其相关作用域中提供了以下公共接口。所有接口均位于 `namespace pro::inline v4` 内，部分扩展位于 `pro::v4::skills` 子命名空间中。

## 1. 核心概念

| 概念                                                         | 说明                                                |
| ------------------------------------------------------------ | --------------------------------------------------- |
| `template<class F> concept facade`                           | 判定类型 `F` 是否为一个合法的 `facade`              |
| `template<class P, class F> concept proxiable`               | 判定类型 `P` 是否可以被 `proxy<F>` 包装             |
| `template<class T, class F> concept inplace_proxiable_target` | 判定 `details::inplace_ptr<T>` 是否满足 `proxiable` |
| `template<class T, class F> concept proxiable_target`        | 判定 `T` 是否可作为 `proxy_view` 的目标类型         |

## 2. 核心类型

| 类型                                                         | 说明                                                     |
| ------------------------------------------------------------ | -------------------------------------------------------- |
| `template<facade F> class proxy`                             | 主要类型擦除包装器，拥有对象所有权                       |
| `template<facade F> class proxy_indirect_accessor`           | 间接访问器，用于支持 `proxy_view` 等                     |
| `template<facade F> using proxy_view = proxy<observer_facade<F>>` | 非拥有视图，包装对象的引用                               |
| `template<facade F> using weak_proxy = proxy<weak_facade<F>>` | 弱引用代理，支持 `lock()`                                |
| `struct substitution_dispatch`                               | 支持类型转换和替换的调度器                               |
| `struct implicit_conversion_dispatch`                        | 隐式转换调度器                                           |
| `struct explicit_conversion_dispatch`                        | 显式转换调度器                                           |
| `using conversion_dispatch = explicit_conversion_dispatch`   | 默认转换调度器别名                                       |
| `template<details::sign Sign, bool Rhs = false> struct operator_dispatch` | 运算符重载调度器特化（如 `+`, `-`, `++`, `[]`, `()` 等） |
| `template<class D> struct weak_dispatch`                     | 弱调度包装器，未实现操作时抛出 `not_implemented`         |
| `class not_implemented`                                      | 异常类型，继承自 `std::exception`                        |
| `class bad_proxy_cast`                                       | 类型转换失败异常，继承自 `std::bad_cast`（需要 RTTI）    |

## 3. Facade 构建器

| 类型                                                         | 说明                                 |
| ------------------------------------------------------------ | ------------------------------------ |
| `template<class Cs, class Rs, std::size_t MaxSize, std::size_t MaxAlign, constraint_level Copyability, constraint_level Relocatability, constraint_level Destructibility> struct basic_facade_builder` | 基础 `facade` 构建器                 |
| `using facade_builder = basic_facade_builder<...>`           | 默认起始构建器，所有约束为未指定状态 |
| `enum class constraint_level { none, nontrivial, nothrow, trivial }` | 生命周期能力级别                     |

`basic_facade_builder` 成员类型（均为返回新构建器的变换）：

- `add_indirect_convention<D, Os...>`
- `add_direct_convention<D, Os...>`
- `add_convention<D, Os...>`（同 `add_indirect_convention`）
- `add_indirect_reflection<R>`
- `add_direct_reflection<R>`
- `add_reflection<R>`（同 `add_indirect_reflection`）
- `add_facade<F, WithSubstitution = false>`
- `add_facade_with_substitution<F>`
- `restrict_layout<PtrSize, PtrAlign>`
- `support_copy<CL>`
- `support_relocation<CL>`
- `support_destruction<CL>`
- `add_skill<Skill>`（如 `skills::rtti`）
- `using build`：最终生成的 `facade` 类型

## 4. 对象创建函数

以下函数均在 `pro::v4` 命名空间内，且要求 `__STDC_HOSTED__`（托管实现）时可用（除 `make_proxy_inplace` 和 `make_proxy_view` 外）。

### 就地构造（无需堆分配）
- `template<facade F, class T, class... Args> constexpr proxy<F> make_proxy_inplace(Args&&... args)`
- `template<facade F, class T, class U, class... Args> constexpr proxy<F> make_proxy_inplace(std::initializer_list<U> il, Args&&... args)`
- `template<facade F, class T> constexpr proxy<F> make_proxy_inplace(T&& value)`

### 视图构造（非拥有）
- `template<facade F, class T> constexpr proxy_view<F> make_proxy_view(T& value)`

### 分配器支持（堆分配）
- `template<facade F, class T, class Alloc, class... Args> constexpr proxy<F> allocate_proxy(const Alloc& alloc, Args&&... args)`
- `template<facade F, class T, class Alloc, class U, class... Args> constexpr proxy<F> allocate_proxy(const Alloc& alloc, std::initializer_list<U> il, Args&&... args)`
- `template<facade F, class Alloc, class T> constexpr proxy<F> allocate_proxy(const Alloc& alloc, T&& value)`
- `template<facade F, class T, class... Args> constexpr proxy<F> make_proxy(Args&&... args)`
- `template<facade F, class T, class U, class... Args> constexpr proxy<F> make_proxy(std::initializer_list<U> il, Args&&... args)`
- `template<facade F, class T> constexpr proxy<F> make_proxy(T&& value)`

### 共享所有权（引用计数）
- `template<facade F, class T, class Alloc, class... Args> constexpr proxy<F> allocate_proxy_shared(const Alloc& alloc, Args&&... args)`
- `template<facade F, class T, class Alloc, class U, class... Args> constexpr proxy<F> allocate_proxy_shared(const Alloc& alloc, std::initializer_list<U> il, Args&&... args)`
- `template<facade F, class Alloc, class T> constexpr proxy<F> allocate_proxy_shared(const Alloc& alloc, T&& value)`
- `template<facade F, class T, class... Args> constexpr proxy<F> make_proxy_shared(Args&&... args)`
- `template<facade F, class T, class U, class... Args> constexpr proxy<F> make_proxy_shared(std::initializer_list<U> il, Args&&... args)`
- `template<facade F, class T> constexpr proxy<F> make_proxy_shared(T&& value)`

## 5. 核心操作函数

| 函数                                                         | 说明                                                      |
| ------------------------------------------------------------ | --------------------------------------------------------- |
| `template<class D, class O, class P, class... Args> decltype(auto) invoke(P&& p, Args&&... args)` | 通过约定 `O` 和调度器 `D` 调用 `proxy`（与 `proxy` 友元） |
| `template<class R, class P> const R& reflect(const P& p)`    | 获取反射器 `R` 的引用                                     |
| `void swap(proxy<F>& lhs, proxy<F>& rhs)`                    | 交换两个 `proxy`（通过友元）                              |
| `bool operator==(const proxy<F>& lhs, std::nullptr_t)`       | 与空指针比较                                              |

### RTTI 相关（需 `__cpp_rtti`）
- `template<class T, class Self> T proxy_cast(Self self)` | 安全向下转换，失败抛 `bad_proxy_cast` |
- `template<class T, class Self> T* proxy_cast(Self* self) noexcept` | 指针版本，失败返回 `nullptr` |
- `template<class Self> const std::type_info& proxy_typeid(const Self& self)` | 获取存储对象的 `type_info` |

## 6. 技能（Skills）—— 位于 `namespace pro::v4::skills`

| 技能                                     | 说明                                         |
| ---------------------------------------- | -------------------------------------------- |
| `template<class FB> using format`        | 添加 `std::formatter` 支持（`char` 版本）    |
| `template<class FB> using wformat`       | 添加 `std::formatter` 支持（`wchar_t` 版本） |
| `template<class FB> using indirect_rtti` | 添加间接 RTTI 支持                           |
| `template<class FB> using direct_rtti`   | 添加直接 RTTI 支持                           |
| `template<class FB> using rtti`          | 同 `indirect_rtti`                           |
| `template<class FB> using slim`          | 限制布局为单个指针大小                       |
| `template<class FB> using as_view`       | 添加转换为 `proxy_view` 的能力               |
| `template<class FB> using as_weak`       | 添加转换为 `weak_proxy` 的能力               |

## 7. 类型特征（Traits）

| 特征                                                         | 说明                     |
| ------------------------------------------------------------ | ------------------------ |
| `template<class T> struct is_bitwise_trivially_relocatable`  | 用于优化移动操作，可特化 |
| `template<class T> inline constexpr bool is_bitwise_trivially_relocatable_v` | 对应的变量模板           |

库为以下类型预定义了 `is_bitwise_trivially_relocatable` 为 `true_type`：
- `std::unique_ptr<T, D>`
- `std::shared_ptr<T>`
- `std::weak_ptr<T>`
- `details::allocated_ptr<T, Alloc>`
- `details::compact_ptr<T, Alloc>`
- `details::shared_compact_ptr<T, Alloc>`
- `details::strong_compact_ptr<T, Alloc>`
- `details::weak_compact_ptr<T, Alloc>`

## 8. 宏（来自 `proxy_macros.h`，用于简化调度器定义）

| 宏                                                           | 说明                           |
| ------------------------------------------------------------ | ------------------------------ |
| `PRO4_DEF_MEM_DISPATCH(name, ...)`                           | 定义一个成员函数风格调度器     |
| `PRO4_DEF_FREE_DISPATCH(name, ...)`                          | 定义一个自由函数风格调度器     |
| `PRO4_DEF_FREE_AS_MEM_DISPATCH(name, ...)`                   | 将自由函数伪装成成员函数调度器 |
| 以及对应的无版本宏 `PRO_DEF_MEM_DISPATCH` 等（用于兼容旧版本，但会触发歧义检查） |                                |

## 9. 已弃用接口（Deprecated）

以下函数标记为 `[[deprecated]]`，建议直接使用无前缀的 `invoke` 和 `reflect`：

- `template<class D, class O, facade F, class... Args> details::ret_t<O> proxy_invoke(proxy<F>& p, Args&&... args)`
- `template<class D, class O, facade F, class... Args> details::ret_t<O> proxy_invoke(const proxy<F>& p, Args&&... args)`
- `template<class D, class O, facade F, class... Args> details::ret_t<O> proxy_invoke(proxy<F>&& p, Args&&... args)`
- `template<class D, class O, facade F, class... Args> details::ret_t<O> proxy_invoke(const proxy<F>&& p, Args&&... args)`
- 类似的重载用于 `proxy_indirect_accessor<F>`
- `template<class R, facade F> const R& proxy_reflect(const proxy<F>& p)`
- `template<class R, facade F> const R& proxy_reflect(const proxy_indirect_accessor<F>& p)`

## 10. 标准库适配器（可选）

如果定义了 `PRO4D_HAS_FORMAT`（需要 C++20 `<format>` 或 libc++ 17+），则在 `namespace std` 中提供：

- `template<class T, class CharT> struct formatter<T, CharT>` 特化，使得 `pro::v4::proxy` 及启用了 `skills::format` 的类型支持 `std::format`。

---

以上列表涵盖了 `proxy` 库公开的所有主要接口。注意：以 `details` 或 `_impl` 命名的内部类型、`operator_dispatch` 的显式特化实例（如 `operator+`）未列为公共接口，但在使用 `PRO4_DEF_*_DISPATCH` 宏时会自动生成对应的特化，属于内部实现细节。

# 设计原理

`proxy` 是一个现代 C++（C++20）的类型擦除库，它的设计目标是提供一种 **零开销、可扩展、类型安全** 的多态包装器，能够统一地处理各种不同的具体类型，同时允许用户自定义要暴露的接口（即“约定”）和运行时类型信息（即“反射”）。其核心原理可以概括为：**基于 `facade` 的描述性接口，通过编译时生成的调度表（vtable 风格）和内存布局控制，在满足特定约束的前提下，将任意符合 `proxiable` 概念的具体类型包装进固定大小的缓冲区内，并支持直接或间接的所有权管理。**

下面从几个关键设计维度解释其原理：

## 1. 核心抽象：Facade（外观）与 Convention（约定）

`proxy` 通过 **`facade`** 来描述包装后对象能支持哪些操作。一个 `facade` 包含两部分：
- **约定**：一组函数签名（例如 `void draw() const`，`int size() const`）。每个签名代表一个可调用的“方法”。这些签名可以带有不同的 `const`、引用限定符、`noexcept`，还可以是模板化的（通过 `facade_aware_overload_t`）。
- **反射**：一组用于访问类型信息的“反射器”类型（例如获取 `std::type_info`）。

用户通过 `facade_builder` 组合这些约定和反射来构建自己的 `facade` 类型。

**设计原理**：将“多态接口”从继承关系中解放出来，允许对任意无关的类型进行统一的操作，并且不要求目标类型继承自某个基类。同时，接口可以由多个独立的约定拼装而成，实现了接口的细粒度复用。

## 2. 类型擦除机制：静态调度表 + 就地存储

`proxy<F>` 内部由两部分组成：
- `std::byte ptr_[F::max_size]`：对齐后的原始存储，用于就地存储具体对象（**直接存储模式**），或者存储一个指针（**间接存储模式**）。
- `meta_ptr<Meta>`：一个指向“元数据”的指针，该元数据实际上是一个 **编译时生成的调度表**（类似于 vtable），包含了该具体类型实现每个约定和反射的具体函数指针。

当一个具体类型 `P` 被放入 `proxy` 时（例如通过 `proxy{std::in_place_type<P>, args...}`），库会做以下事情：
1. 在 `ptr_` 上构造 `P` 对象（若 `P` 尺寸和对齐满足 `facade` 的限制，否则会编译错误或转而使用间接分配）。
2. 根据 `P` 和 `facade`，生成一个静态的元数据对象 `Meta`（类型为 `facade_traits<F>::meta`）。该元数据包含每个约定对应的具体函数指针（例如 `void draw(const P&)` 等），以及每个反射器的构造信息。
3. 将这个元数据对象的地址存入 `meta_`，使得所有 `proxy` 实例共享同一份调度表（**零开销**，类似虚函数但无继承）。

**设计原理**：
- 避免存储每个对象的虚函数表指针，而是为每个具体类型生成一个全局的常量元数据，所有相同 `(P, F)` 对的 `proxy` 实例共享它，节省内存。
- 就地存储（`max_size` 可配置）尽可能避免堆分配，满足 **零开销** 原则；对于不满足尺寸/对齐要求的类型，库内部会 fallback 到动态分配（通过 `allocated_ptr` 等辅助包装器）。
- 通过 `constexpr` 和 `consteval` 在编译期验证类型是否满足 `facade` 的所有约束（如 `copyability`、`relocatability`、`destructibility` 等），不符合时产生清晰的静态断言。

## 3. 调度与访问：`invoke` 和 `accessor`

`proxy` 不直接暴露成员函数，而是通过 **非成员函数 `invoke<D, O>(proxy, args...)`** 来调用某个约定。这里：
- `D` 是一个 **调度器类型**（例如 `draw_dispatch`），它定义了如何将 `proxy` 中的具体对象转换为可调用实体。
- `O` 是约定签名（例如 `void() const`），用于匹配正确的重载。

对于每一个约定，库通过宏自动生成一个 **`accessor`** 模板特化。例如对于 `R(Args...) const` 签名，会生成：
```cpp
template <class P, class D, class R, class... Args>
struct accessor<P, D, R(Args...) const> {
  R operator()(Args... args) const {
    return reinterpret_invoke<P, D, R>(...);
  }
};
```
这个 `accessor` 被注入到 `proxy` 的继承链中，使得用户可以像调用成员函数一样写 `proxy.xxx(args)` —— 这实际上是通过 `accessor` 的 `operator()` 来调用 `invoke`。同时，`accessor` 还负责处理引用的转发、异常说明、`noexcept` 传播等细节。

**设计原理**：
- 将**调用约定的实现**与**具体类型**解耦，允许调度器自由地决定如何从 `proxy` 中提取实际对象（直接解引用或间接解引用）。
- 利用 CRTP 和多重继承为 `proxy` 动态添加一组“方法”，而无需改变 `proxy` 的类定义。
- 通过 `reinterpret_invoke` 等内部函数实现安全的类型转换，并在必要时进行生命期管理（例如右值调用后自动重置 `proxy`）。

## 4. 所有权管理：直接/间接、复制/移动/析构约束

`facade` 中可以通过 `support_copy`、`support_relocation`、`support_destruction` 指定对具体类型的 **最低能力要求**：
- `constraint_level::none`：不需要该操作。
- `nontrivial`：需要操作可用，但不要求 `noexcept`。
- `nothrow`：需要 `noexcept` 版本。
- `trivial`：需要平凡版本（如 `is_trivially_copy_constructible`）。

库会根据这些约束生成相应的元数据条目（例如 `copy_dispatch`、`relocate_dispatch`、`destroy_dispatch`），并在 `proxy` 的复制/移动/析构函数中调用它们。对于 `trivial` 级别，`proxy` 会直接使用 `memcpy` 等底层操作，避免函数间接调用开销。

**设计原理**：将对象生命期管理的开销降低到与原始类型相同。例如如果类型是 `trivially_copyable` 且 `facade` 要求 `trivial`，则 `proxy` 的复制就是简单的 `memcpy`，没有额外的函数调用。

## 5. 扩展性：Dispatch 与 `substitution_dispatch`

`proxy` 不仅支持固定的函数签名，还支持 **“替换调度”**。例如 `substitution_dispatch` 允许在 `proxy` 之间进行类型转换（例如从 `proxy<F1>` 转换为 `proxy<F2>`），以及将任意可平凡 relocate 的类型转换为新的 `proxy`。这为 `proxy_view` 和 `weak_proxy` 等扩展提供了基础。

`proxy_view<F>` 利用了 `observer_ptr` 包装一个现有对象的引用，并将其作为具体的“指针类型”放入 `proxy`，从而**不拥有对象**，只提供视图。`weak_proxy<F>` 则通过 `strong_compact_ptr` / `weak_compact_ptr` 实现弱引用语义，支持 `lock()` 方法。

**设计原理**：将“所有权模型”也抽象为一种可替换的调度策略，无需修改 `proxy` 核心代码，只需提供新的 `dispatch` 类型和相应的 `accessor` 即可。

## 6. 宏与兼容性

`proxy_macros.h` 处理了不同编译器、不同 C++ 版本之间的差异（如 `static operator()`、异常支持、空基类优化等），并提供了一组宏来简化调度器和访问器的定义。同时，它通过版本宏 `__msft_lib_proxy4` 和废弃的 `__msft_lib_proxy` 检测避免多个版本混用。

## 总结：设计哲学

整个 `proxy` 的设计体现了几条现代 C++ 库设计的核心理念：
- **零开销抽象**：在不使用继承和虚函数的情况下实现运行期多态，通过静态生成的调度表和就地存储达到与手写 switch 或函数指针相当的性能。
- **类型安全**：所有操作在编译期检查类型是否符合 facade 的约定，并提供清晰的错误信息。
- **可组合性**：facade 可以像乐高一样通过 `builder` 拼装，也可以从现有 facade 继承/扩展。
- **Awareness of C++20 特性**：充分利用 `concepts`、`constexpr`、`noexcept`、`constinit` 等语言特性，将运行期开销降到最低。

因此，`proxy` 适用于需要通用类型擦除且对性能有高要求的场景，例如插件系统、统一绘图接口、序列化、属性系统等，可视为 `std::function` 的全方位升级版。

# 示例目标

下面通过一个典型的“绘图”示例，完整演示 `proxy` 库的使用流程，并同步解释每一步背后的设计原理。



定义一个统一的接口（`facade`），能够：
- 绘制图形（`draw()`）
- 计算面积（`area() const`）
- 可选地支持 RTTI（获取类型信息）

然后创建 `proxy` 对象来包装 `Circle` 和 `Rectangle`，并统一调用这些操作。

## 完整代码（C++20）

```cpp
#include "proxy.h"   // 包含整个 proxy 库
#include <iostream>
#include <format>

// 1. 定义调度器（dispatch）类型，用来实现具体操作
struct DrawDispatch {
    template<typename T>
    void operator()(const T& self) const {
        self.draw();
    }
};

struct AreaDispatch {
    template<typename T>
    double operator()(const T& self) const {
        return self.area();
    }
};

// 2. 定义 facade：组合约定（conventions）和反射（reflection）
namespace Facades {
    // 使用 facade_builder 构建
    struct Drawable : pro::facade_builder
        ::add_convention<DrawDispatch, void()>          // draw()
        ::add_convention<AreaDispatch, double() const>  // area() const
        ::support_copy<pro::constraint_level::nontrivial>   // 允许拷贝，但不要求 noexcept
        ::support_relocation<pro::constraint_level::nothrow> // 移动不抛异常
        ::support_destruction<pro::constraint_level::nothrow> // 析构不抛异常
        ::build {};
    
    // 高级：加入 RTTI 支持（通过 skills::rtti）
    struct DrawableWithRTTI : pro::facade_builder
        ::add_convention<DrawDispatch, void()>
        ::add_convention<AreaDispatch, double() const>
        ::add_skill<pro::skills::rtti>   // 注入 typeid 支持
        ::build {};
}

// 3. 具体类型：无需继承任何基类
struct Circle {
    double radius;
    void draw() const { std::cout << "Drawing a circle\n"; }
    double area() const { return 3.14159 * radius * radius; }
};

struct Rectangle {
    double width, height;
    void draw() const { std::cout << "Drawing a rectangle\n"; }
    double area() const { return width * height; }
};

int main() {
    using namespace pro;
    
    // 4. 创建 proxy 对象，包装具体类型
    proxy<Facades::Drawable> p1 = make_proxy_inplace<Facades::Drawable, Circle>(5.0);
    proxy<Facades::Drawable> p2 = make_proxy_inplace<Facades::Drawable, Rectangle>(4.0, 6.0);
    
    // 统一调用
    invoke<DrawDispatch>(p1);   // 输出: Drawing a circle
    std::cout << "Area: " << invoke<AreaDispatch>(p1) << '\n';  // 78.5397
    
    invoke<DrawDispatch>(p2);   // Drawing a rectangle
    std::cout << "Area: " << invoke<AreaDispatch>(p2) << '\n';  // 24
    
    // 5. 使用 RTTI 版本
    proxy<Facades::DrawableWithRTTI> p3 = make_proxy_inplace<Facades::DrawableWithRTTI, Circle>(2.0);
    if (typeid(Circle) == proxy_typeid(p3)) {
        std::cout << "p3 holds a Circle\n";
    }
    
    // 6. 直接通过 operator() 调用（如果 accessor 生成）
    // 注意：proxy 本身不提供 draw() 成员，但可通过宏生成的 accessor 支持类似成员语法
    // 下面使用非成员 invoke 是标准方式；也可以使用 PRO_DEF_MEM_DISPATCH 宏来定义成员风格
    
    return 0;
}
```

## 关键点与设计原理解析

### 1. **Facade 描述了“需要什么”**
`facade_builder::add_convention<Dispatch, Signature>` 声明了接口中的每一个操作。  
`Signature` 可以是 `void()`、`double() const`、`void(int) &` 等，支持 `const`、`&`、`&&`、`noexcept` 限定符。  
**设计原理**：接口不再绑定于某个基类，而是以“约束集合”的形式存在，可以灵活组合。

### 2. **Dispatch 类型（调度器）定义“如何实现”**
`DrawDispatch` 和 `AreaDispatch` 是函数对象类型，它们通过 `operator()` 模板来调用具体类型的方法。  
在编译期，`proxy` 会为每个 `(dispatch, signature)` 对生成一个函数指针，指向一个特化的 `reinterpret_invoke` 函数，该函数内部调用 `dispatch`。  
**设计原理**：调用逻辑与类型擦除解耦，允许用户完全控制如何从 `proxy` 中提取具体对象（直接还是间接，是否需要生命周期管理）。

### 3. **`make_proxy_inplace` 就地存储对象**
`make_proxy_inplace<Facade, T>(args...)` 会在 `proxy` 的内部字节数组 `ptr_` 上直接构造 `T` 对象，前提是 `sizeof(T) <= facade::max_size` 且对齐满足要求。  
如果尺寸或对齐超出，库会自动 fallback 到堆分配（通过 `allocated_ptr`）。  
**设计原理**：零开销抽象——大部分情况都使用就地存储，避免堆分配；且因为 `facade` 可配置 `max_size`，用户可以为不同场景平衡内存与性能。

### 4. **`invoke<Dispatch>(proxy, ...)` 执行调用**
`invoke` 是一个非成员函数，它通过 `proxy` 内部的 `meta_` 找到对应签名的函数指针，然后调用之。该函数指针最终会调用 `Dispatch::operator()`，但中间会处理 `*this` 的引用类型（左值/右值/const）以及异常规范。  
**设计原理**：运行期多态完全通过函数指针表实现，没有虚函数开销，且类型擦除后的调用性能接近直接调用。

### 5. **复制、移动、析构的策略**
`facade` 中通过 `support_copy<constraint_level::nontrivial>` 等指定了对包装类型的最低能力要求。  
例如 `nontrivial` 表示包装的类型必须可拷贝，但允许拷贝操作不保证 `noexcept`。  
库会为每个能力等级生成对应的 `copy_dispatch` / `relocate_dispatch` / `destroy_dispatch` 元数据。  
**设计原理**：将生命期操作也纳入类型擦除系统，使得 `proxy` 本身的复制/移动行为可以适应不同类型的需求，同时当类型支持平凡操作时可以优化为 `memcpy`。

### 6. **RTTI 支持**
通过 `add_skill<pro::skills::rtti>`，`facade` 会添加一个间接约定（`void(proxy_cast_context)`）和一个反射器 `proxy_typeid_reflector`。  
`proxy_typeid(p)` 返回存储的具体类型的 `std::type_info`，`proxy_cast<T>(p)` 可以进行安全的向下转换。  
**设计原理**：反射能力也是描述性接口的一部分，只有在 `facade` 明确要求时才加入，避免额外开销。

### 7. **宏 `PRO_DEF_MEM_DISPATCH`（未在示例中展示）**
如果需要让 `proxy` 拥有类似 `.draw()` 的成员函数语法，可以使用库提供的宏：
```cpp
PRO_DEF_MEM_DISPATCH(DrawDispatch, draw);
```
这会在 `proxy` 的继承链中注入一个 `draw()` 成员函数，内部调用 `invoke<DrawDispatch>`。  
**设计原理**：通过宏和 `accessor` 继承链，为用户提供可选的自然语法，同时不破坏核心实现。

## 总结

这个库的设计核心是：**将传统面向对象的多态拆分为“接口描述（facade）”和“实现绑定（dispatch）”两个可独立演化的维度**。  
- `facade` 声明需要什么操作（签名、所有权约束、反射）。  
- `dispatch` 定义如何将操作映射到具体类型的方法上。  
- 编译期生成调度表和存储布局，运行期以极低成本完成类型擦除和调用。  

这使得 `proxy` 既具备 `std::function` 的类型擦除能力，又能达到接近手写虚函数表的性能，同时支持丰富的自定义扩展（如观察者、弱引用、格式化等）。

# 有趣的设计

## 类型builder

这种 `::` 链式调用（例如 `facade_builder::add_convention<A>::add_convention<B>::build`）是 **C++ 模板元编程（TMP）中的“类型级链式调用”**，完全在编译期完成，不涉及任何运行时对象。它的核心技术支持包括：

1. **模板别名（alias template）**  
2. **类模板的静态成员类型（嵌套 using）**  
3. **可变参数模板与类型列表**  
4. **递归模板实例化与惰性求值**

下面用一个简化模型来解释这种链条是如何一步步连接的。

---

## 1. 基本思想：每个“方法”都返回一个新类型

传统运行时链式调用（如 `obj.method1().method2()`）依赖对象方法返回自身或新对象。在类型级别，我们无法“调用”函数，但可以**访问嵌套类型**。语法 `Type::NestedType` 可以层层嵌套，只要每个 `NestedType` 本身又是一个带有更多嵌套类型的类。

`basic_facade_builder` 的设计就是：  
- 它是一个类模板，拥有多个模板参数（比如已累积的约定列表 `Cs`、反射列表 `Rs`、尺寸限制等）。  
- 它的每一个“操作”（如 `add_convention`）实际上是一个**模板别名**（`using`），这个别名展开成一个**新的 `basic_facade_builder` 特化**，其模板参数是更新后的结果。

例如（简化版）：

```cpp
template <class Cs, class Rs, size_t MaxSize>
struct basic_facade_builder {
    // 假设 Cs 是 std::tuple<...> 类型
    template <class D, class... Os>
    using add_convention = basic_facade_builder<
        typename details::add_conv_t<Cs, conv_impl<false, D, Os...>>,
        Rs, MaxSize
    >;
    
    using build = facade_impl<Cs, Rs, MaxSize, ...>;
};
```

当写下 `facade_builder::add_convention<A>::add_convention<B>` 时：

- `facade_builder` 是一个具体的 `basic_facade_builder` 特化（通常 `Cs = std::tuple<>`）。
- `facade_builder::add_convention<A>` 是一个别名，它指向 `basic_facade_builder<Cs', Rs, MaxSize>` 这个新类型。
- 在这个新类型中，又有一个 `add_convention` 别名，可以继续使用 `::add_convention<B>`。
- 最后 `::build` 是最终类型中的别名，它展开成真正的 `facade` 类型。

整个过程**不创建任何对象**，只是编译期对类型别名的层层展开。

---

## 2. 具体技术拆解

### 2.1 模板别名（alias template）

在 C++11 之后，我们可以写：

```cpp
template <class T>
using alias = SomeOtherType<T>;
```

这允许 `alias<X>` 等同于 `SomeOtherType<X>`，且可以出现在 `::` 右侧。  
`basic_facade_builder` 内部大量使用了这种技术，例如：

```cpp
template <class D, details::extended_overload... Os>
using add_convention = add_indirect_convention<D, Os...>;
```

这里 `add_indirect_convention` 本身也是一个模板别名，最终展开成一个新的 `basic_facade_builder` 特化。

### 2.2 类型列表与递归组合

链式调用需要逐步“累积”状态（比如之前添加的约定列表）。这通常通过 `std::tuple` 或类似结构来存储类型列表。每次 `add_convention` 都会把新的约定类型 **追加** 到已有的 `std::tuple` 中。

代码中使用了 `details::add_conv_t<Cs, conv_impl<...>>`，其内部通过递归模板将新类型加入元组。例如（简化）：

```cpp
template <class Tuple, class New>
struct add_to_tuple;
template <class... Ts, class New>
struct add_to_tuple<std::tuple<Ts...>, New> {
    using type = std::tuple<Ts..., New>;
};
```

然后 `add_convention` 就通过 `typename add_to_tuple<Cs, New>::type` 得到新的类型列表。

### 2.3 惰性实例化与编译期计算

C++ 模板只有在“需要完整类型”时才会实例化。当你写 `Builder::add_convention<A>` 时，编译器只需要知道 `add_convention` 是一个别名，它指向某个类型；并不需要立即递归实例化所有内部细节。  
这允许链式调用任意长度，因为编译器只是不断展开别名，直到最后 `::build` 才真正需要生成最终的 `facade` 类型。

---

## 3. 与运行时链式调用的本质区别

| 特性     | 运行时链式调用 | 类型级链式调用（本库） |
| -------- | -------------- | ---------------------- |
| 操作对象 | 对象实例       | 类型（类模板特化）     |
| 运算符   | `.` 或 `->`    | `::`                   |
| 返回类型 | 对象（或引用） | 类型（别名）           |
| 执行时机 | 运行时         | 编译期                 |
| 状态存储 | 对象成员变量   | 模板参数（如 `Cs`）    |

所以 `pro::facade_builder::add_convention<X>::add_convention<Y>::build` 实际上是在编译时构造一个复杂的类型，等价于：

```cpp
using Builder1 = basic_facade_builder<std::tuple<>, ...>;
using Builder2 = Builder1::add_convention<X>;   // 别名展开
using Builder3 = Builder2::add_convention<Y>;   // 别名展开
using MyFacade = Builder3::build;               // 最终类型
```

每一步都是对类型别名的解析，没有生成任何可执行代码（除了最终的 `facade` 类型定义）。

---

## 4. 为什么能写 `::` 而不是 `.`？

因为 `facade_builder` 是一个**类型**，不是对象。C++ 允许通过 `类型::嵌套类型` 访问类内部的类型成员（`using` 定义的别名或嵌套类）。  
这就像写 `std::vector<int>::size_type` 一样自然。

如果试图写成 `facade_builder.add_convention`，则会报错，因为 `.` 只能用于对象，而 `facade_builder` 是个类型。

---

## 5. 这种设计的优点

- **纯编译期配置**：无运行时开销，`facade` 的定义完全静态。
- **可读性高**：链式语法比嵌套模板（如 `basic_facade_builder<add_conv_t<...>>`）更直观。
- **易于扩展**：新增一个“设置”只需在 `basic_facade_builder` 中添加一个新的 `using` 别名。
- **类型安全**：编译器会检查每个步骤的类型一致性（例如不能把 `constraint_level::none` 传给期待 `std::size_t` 的参数）。

---

## 总结

`proxy` 库中 `facade_builder::add_convention<...>::support_copy<...>::build` 这种 `::` 链式调用的底层技术支持是 **C++ 模板元编程**：
- 使用**模板别名**作为“类型函数”，返回新类型。
- 使用**嵌套类型访问** (`::`) 来串联这些类型函数。
- 通过**类型列表**（如 `std::tuple`）累积状态。
- 最终生成一个描述完备的 `facade` 类型，供 `proxy<F>` 使用。

这种模式也被称为 **“编译期 Builder 模式”** 或 **“类型级链式调用”**，是 C++ 模板库设计中常见的技巧（例如 `std::tuple` 的各种操作、Boost.MPL、C++20 的 `std::views` 链式调用底层也类似，但后者是对象范畴）。