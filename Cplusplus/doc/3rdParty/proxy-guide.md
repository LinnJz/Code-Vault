# C++ proxy 库：核心概念与门面功能指南

## 目录

- [C++ proxy 库：核心概念与门面功能指南](#c-proxy-库核心概念与门面功能指南)
  - [目录](#目录)
  - [一、多态方案对比](#一多态方案对比)
  - [二、proxy vs virtual](#二proxy-vs-virtual)
    - [2.1 核心差异](#21-核心差异)
    - [2.2 性能分析](#22-性能分析)
  - [三、proxy 的调用链与间接次数](#三proxy-的调用链与间接次数)
  - [四、proxy vs Rust `dyn Trait`（胖指针）](#四proxy-vs-rust-dyn-trait胖指针)
  - [五、SBO 大小与自定义](#五sbo-大小与自定义)
    - [5.1 默认大小](#51-默认大小)
    - [5.2 自定义 SBO](#52-自定义-sbo)
  - [六、Pimpl 模式下的 proxy](#六pimpl-模式下的-proxy)
  - [七、门面功能全览（表）](#七门面功能全览表)
    - [7.1 协议（Convention）](#71-协议convention)
    - [7.2 反射（Reflection）](#72-反射reflection)
    - [7.3 生命周期约束](#73-生命周期约束)
    - [7.4 缓冲区控制](#74-缓冲区控制)
    - [7.5 内置 Skills](#75-内置-skills)
    - [7.6 组织与构建](#76-组织与构建)
  - [八、完整示例](#八完整示例)
    - [8.1 定义 dispatch，类比 vtable](#81-定义-dispatch类比-vtable)
    - [8.2 定义 facade（门面）](#82-定义-facade门面)
    - [8.3 定义具体类型](#83-定义具体类型)
    - [8.4 使用](#84-使用)
    - [8.5 自定义反射](#85-自定义反射)
    - [8.6 RTTI](#86-rtti)
    - [8.7 Format 支持](#87-format-支持)
    - [8.8 skills::slim —— 缩小 SBO 为指针大小](#88-skillsslim--缩小-sbo-为指针大小)
    - [8.9 skills::as_view —— 转非拥有视图](#89-skillsas_view--转非拥有视图)
    - [8.10 skills::as_weak —— 转弱引用](#810-skillsas_weak--转弱引用)

---

## 一、多态方案对比

| 维度 | 传统虚接口 | C++ proxy | Rust `dyn Trait` |
|------|-----------|-----------|-------------------|
| **类型要求** | 必须继承基类 | 任意类型，满足约束即可 | 任意类型，`impl Trait` 即可 |
| **对象存储** | 必须指针/引用，通常堆分配 | SBO 内联 or 堆分配，值语义 | `&dyn` 借用 / `Box<dyn>` 必堆分配 |
| **vtable/vptr** | 每个对象带 vptr（8 字节），vtable 在全局 | `meta_` dispatch 表，可内联在 proxy 对象中 | 胖指针第二个 word 是 vptr，始终间接引用 |
| **调用访存链** | `obj → vptr → vtable[n] → fn ptr → call`（2 次访存 + 1 次间接调用） | 最好情况直接加载 dispatcher → 1 次间接调用 | `fat_ptr → vtable[n] → fn ptr → call`（2 次访存 + 1 次间接调用） |
| **内存分配** | 多用堆分配 | SBO 避免小对象堆分配 | `Box` 必堆分配 |
| **类型擦除** | 基类指针 | 编译期生成 dispatch 表，运行期擦除 | 编译器生成 vtable，运行期擦除 |

---

## 二、proxy vs virtual

### 2.1 核心差异

1. **非侵入式设计** — 类型无需继承基类，任意满足约束的类型都可放入 `proxy`
2. **值语义** — proxy 拥有对象，支持拷贝/移动/销毁，类似 `std::function`
3. **SBO（小对象优化）** — 小型对象（默认 ≤16 字节）直接 inline 在 proxy 对象中，零堆分配
4. **无 vptr 开销** — 具体类型本身不携带虚表指针，纯数据
5. **灵活的内存策略** — 通过 `constraint_level` 精细控制拷贝/移动/析构行为

### 2.2 性能分析

| 场景 | proxy | virtual | 谁赢 |
|------|-------|---------|------|
| 原始单次调用 | dispatch 间接跳转 | vtable 间接跳转 | **持平**（都 ~1-2 次间接） |
| 小型对象创建/销毁 | SBO，无堆分配 | `new` + 堆分配 | **proxy 胜** |
| 数组遍历 + 同类型 | SBO 连续内存 | 指针数组，跳转多 | **proxy 胜** |
| 编译期已知类型 | 编译器可去虚化内联 | 难以去虚化 | **proxy 胜** |
| 大型对象 | 退化为堆分配 | 堆分配 | **持平** |

**结论：** 对小型对象（SBO 能装下）**确实有性能提升**，主要来自避免堆分配和更好的内存局部性。原始虚函数调用性能两者本质都是间接函数调用，差距可忽略。proxy 的真正价值在**值语义 + 非侵入式设计 + 灵活的内存策略**，性能增益是附带收益。

---

## 三、proxy 的调用链与间接次数

proxy 的真实间接调用次数是 **1 次**（仅 `call <dispatcher_fn_ptr>`）。

```
proxy_invoke<D, O>(p, args)
  → invoke_impl (编译期确定)
  → p.meta_.operator->()  // 取 dispatch 表或函数指针
  → .invocation_meta<...>::dispatcher  // 函数指针
  → 调用 dispatcher(p, args)  // 间接调用 #1（唯一的一次）
  → invoke_dispatch<P, F, ...>  // P 已知的模板实例
  → get_ptr<P>(p)  // 取实际对象指针
  → D()(obj, args)  // 直接调用（编译期已知，可内联）
```

**关键点（Direct meta）：** 当 meta 表 ≤ `void*[2]`（16 字节，默认最常见情况）时，meta 表直接嵌在 proxy 对象内部。dispatcher 函数指针直接从 proxy 字段加载，无需额外指针追踪。

| 情况 | 取 dispatcher 方式 | 间接调用次数 |
|------|-------------------|------------|
| **Direct meta**（默认 ≤16 字节） | dispatcher 是 proxy 的字段，直接加载 | **1 次** |
| **Indirect meta**（罕见） | meta_ 指向外部静态存储，多一次访存 | 1 次间接调用，前面多一次指针解引用 |
| **virtual** 对比 | vptr → vtable → fn ptr → call | 2 次访存 + 1 次间接调用 |

---

## 四、proxy vs Rust `dyn Trait`（胖指针）

**proxy 的 `proxy<F>` 和 Rust 的 `dyn Trait` 本质上是同一类东西：带 dispatch 表的胖指针。** 但实现细节差异很大：

| 维度 | C++ proxy | Rust `dyn Trait` |
|------|-----------|-------------------|
| **数据存储** | SBO 内联（可避免堆分配） | `&dyn` 是借用，`Box<dyn>` 必堆分配 |
| **dispatch 表位置** | 可以**直接内联在 proxy 对象中**（direct meta），或通过指针间接引用 | 始终**间接引用**（胖指针第二个 word 是 vptr） |
| **胖指针结构** | `{ meta/dispatchers, ptr_[] }`，size = meta 表 + SBO 大小 | `{ data_ptr, vtable_ptr }`，固定 2 words |
| **Dispatch 访存链** | 最好情况：1 load（direct meta）→ 1 call | 固定：2 loads（fat ptr → vtable → fn ptr）→ 1 call |
| **非侵入性** | 任意类型满足约束即可，无需继承 | 任意类型 `impl Trait` 即可，无需继承 |
| **所有权语义** | 值语义，默认 owned | `&dyn` 借用 / `Box<dyn>` 独占所有权 |
| **Runtime reflection** | 有 `proxy_reflect` 可反射具体类型信息 | 有限，可通过 `Any` 但不原生 |
| **SBO 大小控制** | 可自定义 | 不可控 |

**proxy 胜在**：SBO 避免堆分配，direct meta 少一次访存，更灵活的内存策略。

**Rust 胜在**：编译期 object safety 保证，类型系统约束更丰富，无未定义行为。

---

## 五、SBO 大小与自定义

### 5.1 默认大小

默认 SBO = `sizeof(void*[2])` = **16 字节**（64位），对齐 = **8 字节**。

源码（`proxy.h:2069-2074`）：

```cpp
MaxSize == invalid_size ? sizeof(details::ptr_prototype) : MaxSize,
MaxAlign == invalid_size ? alignof(details::ptr_prototype) : MaxAlign,
```

其中 `ptr_prototype = void* [2]`。

超出 16 字节的类型自动退化为堆分配（`proxy<F>` 内部持有指针）。

### 5.2 自定义 SBO

```cpp
using BigFacade = decltype(
    facade_builder::add_convention<DrawDispatch, void() const>()
        .restrict_layout<64>()  // SBO 扩大到 64 字节
        .build
);
```

`restrict_layout<N>` 将 `max_size` 更新为 `min(当前_max_size, N)`。

---

## 六、Pimpl 模式下的 proxy

传统虚接口 + Pimpl：

```cpp
struct Circle : IShape {
    unique_ptr<CircleImpl> impl_;  // 8 字节

    void draw() override { impl_->draw(); }
};

// 堆分配 × 2：
IShape* c = new Circle(args...);   // ① Circle 堆分配
                                   // ② Circle 内部 new CircleImpl
```

proxy + Pimpl：

```cpp
struct Circle {
    unique_ptr<CircleImpl> impl_;  // 8 字节，纯数据

    void draw() { impl_->draw(); }
};

// 堆分配 × 1：
proxy<Draw> c = Circle{args...};  // Circle inline 在 SBO
                                  // 内部只 new CircleImpl
```

| | 传统 `IShape*` | proxy |
|--|--------------|-------|
| 外层对象 | 堆分配 | SBO，**零额外分配** |
| Pimpl 内部 | 堆分配 | 堆分配（不可避免） |
| 虚表开销 | Circle 携带 vptr（16 字节，超 SBO） | Circle 纯数据（8 字节，SBO 够） |
| 对象布局 | 堆 → 堆（双重间接） | 内联 → 堆（少一级间接） |

**SBO 消除的是虚多态带来的外层包装分配，Pimpl 内部的分配由业务设计决定。**

---

## 七、门面功能全览（表）

以下所有功能均在 `basic_facade_builder`（`proxy.h:2012-2085`）上定义。

### 7.1 协议（Convention）

| 方法 | 作用 | 代码示例 |
|------|------|---------|
| `add_convention<D, Os...>` | 添加协议（间接，按指针访问操作数，默认） | `builder.add_convention<DrawD, void() const>` |
| `add_indirect_convention<D, Os...>` | 同上，显式间接 | `builder.add_indirect_convention<DrawD, void() const>` |
| `add_direct_convention<D, Os...>` | 添加协议（直接引用访问操作数） | `builder.add_direct_convention<DrawD, void() const>` |

**直接 vs 间接说明：**

| | 间接（默认） | 直接 |
|--|------------|------|
| **含义** | proxy 持有**指针**（如 `unique_ptr`），调用时解引用 | proxy 持有**值本体**，直接使用 |
| **dispatch 操作数** | dispatcher 收到 `proxy<F>`，内部 `*ptr_` | dispatcher 收到 `proxy<F>`，直接 `ptr_` |
| **accessor 继承自** | `proxy_indirect_accessor<F>` | `proxy<F>` |
| **典型场景** | `proxy<Draw> = make_unique<Circle>()` | `proxy<Draw> = Circle{}` |

```cpp
// Dispatch 定义
struct DrawDispatch {
    template <class T>
    PRO4D_STATIC_CALL(void, const T& self) {
        self.draw();
    }
};

// 直接：proxy 存值本体
using DrawDirect = decltype(
    facade_builder
        .add_direct_convention<DrawDispatch, void() const>()
        .build
);
// proxy<DrawDirect> c = Circle{};

// 间接（默认）：proxy 存指针
using DrawIndirect = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .build
);
// proxy<DrawIndirect> c = make_unique<Circle>();
```

### 7.2 反射（Reflection）

| 方法 | 作用 | 代码示例 |
|------|------|---------|
| `add_reflection<R>` | 添加反射元数据（间接，默认） | `builder.add_reflection<SizeReflector>` |
| `add_indirect_reflection<R>` | 同上，显式间接 | `builder.add_indirect_reflection<R>` |
| `add_direct_reflection<R>` | 添加反射元数据（直接） | `builder.add_direct_reflection<R>` |

反射就是：为每个具体类型 P 在编译期生成一个静态的元数据对象，存入 `refl_meta`，运行时通过 `proxy_reflect<R>(p)` 取出。

```cpp
struct SizeReflector {
    std::size_t size;
    template <class T>
    explicit SizeReflector(std::in_place_type_t<T>) : size(sizeof(T)) {}
};

using WithSize = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_direct_reflection<SizeReflector>
        .build
);

void use() {
    proxy<WithSize> c = Circle{};
    auto& ref = proxy_reflect<SizeReflector>(c);
    // ref.size == sizeof(Circle) == 8
}
```

### 7.3 生命周期约束

| 方法 | 作用 | 取值 |
|------|------|------|
| `support_copy<CL>` | 设置拷贝约束级别 | `none` / `nontrivial` / `nothrow` / `trivial` |
| `support_relocation<CL>` | 设置移动/搬迁约束级别 | 同上 |
| `support_destruction<CL>` | 设置析构约束级别 | 同上 |

`constraint_level` 含义：

| 级别 | 含义 |
|------|------|
| `trivial` | 直接 memcpy，无函数调用开销 |
| `nothrow` | 不抛异常 |
| `nontrivial` | 常规函数调用 |
| `none` | 禁止该操作 |

```cpp
using Lightweight = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .support_copy<constraint_level::trivial>      // memcpy 拷贝
        .support_destruction<constraint_level::trivial> // 不调用析构
        .build
);
```

### 7.4 缓冲区控制

| 方法 | 作用 | 代码示例 |
|------|------|---------|
| `restrict_layout<PtrSize, PtrAlign>` | 限制 SBO 缓冲区大小（默认 16 字节） | `.restrict_layout<32>()` |

```cpp
using SmallSBO = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .restrict_layout<sizeof(void*)>()  // 只留 8 字节 SBO
        .build
);
```

### 7.5 内置 Skills

| Skill | 作用 | 代码示例 |
|-------|------|---------|
| `skills::format<FB>` | 添加 `std::format` 支持 | `.add_skill<skills::format>` |
| `skills::wformat<FB>` | 添加 `std::wformat` 支持 | `.add_skill<skills::wformat>` |
| `skills::rtti<FB>` | 添加运行时类型识别（间接） | `.add_skill<skills::rtti>` |
| `skills::indirect_rtti<FB>` | 同上，显式间接 | `.add_skill<skills::indirect_rtti>` |
| `skills::direct_rtti<FB>` | RTTI 直接版 | `.add_skill<skills::direct_rtti>` |
| `skills::slim<FB>` | SBO 缩小到 `sizeof(void*)` | `.add_skill<skills::slim>` |
| `skills::as_view<FB>` | 添加 `proxy_view` 隐式转换 | `.add_skill<skills::as_view>` |
| `skills::as_weak<FB>` | 添加 `weak_proxy` 隐式转换 | `.add_skill<skills::as_weak>` |

```cpp
// RTTI
using WithRTTI = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_skill<skills::rtti>
        .build
);
void use() {
    proxy<WithRTTI> c = Circle{};
    auto& ti = proxy_typeid(c);  // type_info of Circle
    auto* circle = proxy_cast<Circle>(&c);  // dynamic_cast 等价
}

// Format
using PrettyDraw = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_skill<skills::format>
        .build
);
void use() {
    proxy<PrettyDraw> c = Circle{1.0};
    auto s = std::format("{}", c);
}

// Slim（只存裸指针，不拥有值）
using View = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_skill<skills::slim>
        .build
);

// As view（转为非拥有视图）
using Draw = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_skill<skills::as_view>
        .build
);
void use() {
    proxy<Draw> c = Circle{};
    proxy_view<Draw> v = c;  // 不转移所有权
}

// As weak（弱引用，类似 weak_ptr）
using WeakEnabled = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_skill<skills::as_weak>
        .build
);
```

### 7.6 组织与构建

| 方法 | 作用 | 代码示例 |
|------|------|---------|
| `add_facade<F, WithSubstitution>` | 合并另一个 facade 的全部协议和反射 | `.add_facade<OtherFaade>` |
| `add_skill<Skill>` | 添加 skill 扩展 | `.add_skill<skills::rtti>` |
| `build` | 生成最终的 `facade_impl` | `.build` |

```cpp
// 组合多个 facade
using Drawable = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .build
);
using Serializable = decltype(
    facade_builder
        .add_convention<SerializeDispatch, void(std::ostream&) const>()
        .build
);
using DrawableSerializable = decltype(
    facade_builder
        .add_facade<Drawable>
        .add_facade<Serializable>
        .build
);
```

---

## 八、完整示例

### 8.1 定义 dispatch，类比 vtable

```cpp
// DrawDispatch 替代了虚基类的纯虚函数声明
struct DrawDispatch {
    template <class T>
    PRO4D_STATIC_CALL(void, const T& self) {
        self.draw();
    }
};

struct AreaDispatch {
    template <class T>
    PRO4D_STATIC_CALL(double, const T& self) {
        return self.area();
    }
};
```

### 8.2 定义 facade（门面）

```cpp
using Draw = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_convention<AreaDispatch, double() const>()
        .add_skill<skills::rtti>
        .add_skill<skills::format>
        .add_skill<skills::as_view>
        .support_copy<constraint_level::nontrivial>()
        .build
);
```

### 8.3 定义具体类型

```cpp
struct Circle {
    double radius;

    void draw() const { /* draw circle */ }
    double area() const { return 3.14 * radius * radius; }
};

struct Rectangle {
    double w, h;

    void draw() const { /* draw rectangle */ }
    double area() const { return w * h; }
};
```

### 8.4 使用

```cpp
void render(const proxy<Draw>& obj) {
    obj.draw();
}

int main() {
    // SBO inline，零堆分配（Circle 8 字节 ≤ 16）
    proxy<Draw> c = Circle{1.0};
    proxy<Draw> r = Rectangle{2.0, 3.0};

    render(c);  // 调用 Circle::draw
    render(r);  // 调用 Rectangle::draw

    // 值语义拷贝
    proxy<Draw> c2 = c;
    // c2.draw();  // 独立副本
}
```

### 8.5 自定义反射

```cpp
struct NameReflector {
    std::string_view name;
    template <class T>
    explicit NameReflector(std::in_place_type_t<T>)
        : name([] {
            if constexpr (std::is_same_v<T, Circle>) return "Circle";
            else if constexpr (std::is_same_v<T, Rectangle>) return "Rectangle";
            else return "Unknown";
        }()) {}
};

using NamedDraw = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_direct_reflection<NameReflector>
        .build
);

void use() {
    proxy<NamedDraw> c = Circle{1.0};
    auto& ref = proxy_reflect<NameReflector>(c);
    // ref.name == "Circle"
}
```

### 8.6 RTTI

```cpp
using WithRTTI = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_skill<skills::rtti>
        .build
);

void use() {
    proxy<WithRTTI> c = Circle{1.0};
    auto& ti = proxy_typeid(c);  // 获取 typeid
    auto* ptr = proxy_cast<Circle>(&c);  // 等价 dynamic_cast
    assert(ptr != nullptr);
}
```

### 8.7 Format 支持

```cpp
struct Circle {
    double radius;
    // 实现 std::formatter 特化或 format 成员...
};

using Drawable = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_skill<skills::format>
        .build
);

void use() {
    proxy<Drawable> c = Circle{1.0};
    std::println("{}", c);  // 格式化为字符串
}
```

### 8.8 skills::slim —— 缩小 SBO 为指针大小

```cpp
// slim 将 SBO 缩小到 sizeof(void*) 字节
// proxy 只能存指针（不拥有值），类似原始虚接口的语义
using SlimDraw = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_skill<skills::slim>
        .build
);

void use() {
    Circle c{1.0};
    proxy<SlimDraw> p = &c;  // 存指针，零拷贝
    p.draw();  // 调用 Circle::draw
}
```

### 8.9 skills::as_view —— 转非拥有视图

```cpp
using Draw = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_skill<skills::as_view>
        .build
);

void use() {
    proxy<Draw> owned = Circle{1.0};          // 拥有值
    proxy_view<Draw> view = owned;             // 非拥有视图

    // proxy_view 不参与生命周期管理
    // 等价于 const 引用语义
}
```

### 8.10 skills::as_weak —— 转弱引用

```cpp
using WeakAware = decltype(
    facade_builder
        .add_convention<DrawDispatch, void() const>()
        .add_skill<skills::as_weak>
        .build
);

void use() {
    auto strong = std::make_shared<proxy<WeakAware>>(Circle{1.0});
    weak_proxy<WeakAware> wp = *strong;

    // 锁定检查
    if (auto sp = wp.lock()) {
        sp.draw();  // 对象仍然存活
    }
    strong.reset();
    // wp.lock() 返回空 proxy
}
```
