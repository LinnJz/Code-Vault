# MSVC/GCC/CLANG预处理命令官网

https://learn.microsoft.com/zh-cn/cpp/preprocessor/pragma-directives-and-the-pragma-keyword?view=msvc-170

https://gcc.gnu.org/onlinedocs/gcc/Pragmas.html

https://clang.llvm.org/docs/LanguageExtensions.html

---

# MSVC `#pragma` 编译指令指南

## 前言

> **作者**：微软、小猪啊呜
> **原文链接**：[https://www.jianshu.com/p/3cef414630f9](https://www.jianshu.com/p/3cef414630f9)  
> （本文在原文基础上进行了文字扩展和示例增强，内容涵盖所有常用 `#pragma` 指令。）

## 综述

`#pragma` 指令是每个编译器在保留 C 和 C++ 语言的整体兼容性时，提供不同机器和操作系统特定功能的扩展。编译指令是机器或操作系统特有的，并且不同的编译器通常存在差异。

语法如下：

```cpp
#pragma token_string   // token_string 为参数
```

- `#` 必须是编译指令的第一个非空白字符，`#` 和 `pragma` 之间可以有任意数量的空白符。
- `#pragma` 后面的内容可以是任何编译器能够作为预处理符号分析的文本。`#pragma` 的参数类似于宏扩展。
- 如果参数无法识别，编译器会抛出一个警告后继续编译。

示例代码：

```cpp
#pragma once            // 正确
  #pragma once          // 正确
# pragma once           // 正确
;#pragma once           // 错误：预处理命令必须以第一个非空白字符开始
// error C2014: preprocessor command must start as first nonwhite space
```

> **注意**：为了提供新的预处理功能，或者为编译程序提供由实现定义的信息，编译指示可以用在一个条件语句内。  
> 请参阅 [Pragma 指令和 `__Pragma` 关键字](https://docs.microsoft.com/en-us/cpp/preprocessor/pragma-directives-and-the-pragma-keyword)。

C 和 C++ 编译器可以识别下列编译指令（按字母顺序排列）：

| 指令                    | 指令               | 指令              | 指令                |
| ----------------------- | ------------------ | ----------------- | ------------------- |
| alloc_text              | auto_inline        | bss_seg           | check_stack         |
| code_seg                | comment            | component         | conform             |
| const_seg               | data_seg           | deprecated        | detect_mismatch     |
| execution_character_set | fenv_access        | float_control     | fp_contract         |
| function                | hdrstop            | include_alias     | init_seg            |
| inline_depth            | inline_recursion   | intrinsic         | loop                |
| make_public             | managed, unmanaged | message           | omp                 |
| once                    | optimize           | pack              | pointers_to_members |
| pop_macro               | push_macro         | region, endregion | runtime_checks      |
| section                 | setlocale          | strict_gs_check   | vtordisp            |
| warning                 |                    |                   |                     |

---

## 1. `alloc_text`

### 语法

```cpp
#pragma alloc_text( "textsection", function1, ... )
```

### 作用

命名特定函数驻留的代码段（section）。

### 备注

- 必须出现在函数声明和函数定义**之间**。
- 不处理 C++ 成员函数或重载函数。它仅能应用在以 C 连接方式声明的函数（用 `extern "C"` 连接）。如果将该指令运用在具有 C++ 连接方式的函数时，将出现编译错误。
- 由于函数寻址不支持 `__based` 形式，所以需要通过该编译指令来指定代码段。`textsection` 指定的名字应该由双引号括起来。
- 引用的函数必须与该指令处于同一模块中，否则可能无法捕获编译器将未定义的函数编译到不同的代码段中的错误。即使程序通常还能正常运行，但是函数并没有分配到指定的代码段中。
- 该编译指令**不能**用在函数体内部。

### 示例代码

```cpp
// 声明外部 C 函数
extern "C" void func1(void);
extern "C" void func2(void);

// 指定 func1 和 func2 存放在 .mycode 段中
#pragma alloc_text(".mycode", func1, func2)

// 定义函数
void func1(void) {
    // 函数体
}

void func2(void) {
    // 函数体
}

int main() {
    func1();
    func2();
    return 0;
}
```

> **说明**：使用 `dumpbin /SECTION:.mycode` 可以查看该段的内容。

---

## 2. `auto_inline`

### 语法

```cpp
#pragma auto_inline( [{on | off}] )
```

### 作用

打开（`on`）或关闭（`off`）编译器将普通函数自动转换为内联函数的功能。

### 备注

- 不能出现在函数定义内部，需要写在函数定义之前或之后。
- 将在其出现以后的**第一个函数定义**开始起作用。
- 对显式的 `inline` 函数不起作用。

### 示例代码

```cpp
#pragma auto_inline(off)   // 关闭自动内联

// 此函数不会被自动内联（即使编译器优化选项允许）
void func1() {
    int a = 0;
    a++;
}

#pragma auto_inline(on)    // 重新打开自动内联

// 此函数可能被自动内联
void func2() {
    int b = 0;
    b++;
}

inline void func3() {
    // 显式内联不受 auto_inline 影响
}

int main() {
    func1();
    func2();
    func3();
    return 0;
}
```

---

## 3. `bss_seg`

### 语法

```cpp
#pragma bss_seg( [ [ { push | pop }, ] [ identifier, ] ] [ "segment-name" [, "segment-class" ] )
```

### 作用

指定 OBJ 文件中存放**未初始化变量**的段。

### 备注

- OBJ 文件中存放未初始化变量的默认段为 `.bss`。
- 在一些情况下，将未初始化变量存储在一个特定的段中可以提高加载速度。
- 不带参数的 `bss_seg` 将段重置为 `.bss`。
- `push`：将一条段记录压入编译堆栈，可以带 `identifier` 和 `segment-name` 参数。
- `pop`：将编译堆栈顶的记录弹出。
- `identifier`：可选参数。当使用 `push` 时指定压入编译堆栈的记录标示符；当使用 `pop` 时，弹出从栈顶到该标示符记录之间的所有元素。它可以使一条 `pop` 指令弹出多条 `push` 记录。
- `segment-name`：段名。当使用 `pop` 指令之后，弹出的段名将作为新的激活段。
- `segment-class`：段类，用于兼容 C++ 2.0 之前的版本，已废弃。

### 示例代码

```cpp
// pragma_directive_bss_seg.cpp
int i;               // 默认存放在 .bss 段

#pragma bss_seg(".my_data1")
int j;               // 存放在 "my_data1" 段

#pragma bss_seg(push, stack1, ".my_data2")
int l;               // 存放在 "my_data2" 段

#pragma bss_seg(pop, stack1)   // 弹出 stack1，恢复之前的段
int m;               // 存放在 ".my_data1" 段

#pragma bss_seg()    // 重置为默认 .bss 段
int n;               // 存放在 .bss 段

int main() {
    return 0;
}
```

> **查看段信息**：在命令行使用 `dumpbin /SECTION:.my_data1 test.obj` 可以验证。

---

## 4. `check_stack`

### 语法

```cpp
#pragma check_stack([ {on | off}] )
#pragma check_stack{+ | -}
```

### 作用

打开（`on`/`+`）或关闭（`off`/`-`）栈检查（检测栈溢出）。

### 备注

- `check_stack` 在无参数情况下，栈检查恢复到默认行为（由 `/Gs` 编译选项决定）。
- 该编译指令将在其出现之后的**第一个函数**开始生效。
- 栈检查不是宏或者内联函数的一部分。
- `#pragma check_stack` 和 `/Gs` 选项的互相作用情况如下表：

| 语法                                                  | 是否使用 `/Gs` 选项 | 行为                     |
| ----------------------------------------------------- | ------------------- | ------------------------ |
| `#pragma check_stack()` 或 `#pragma check_stack`      | 是                  | **关闭**后续函数的栈检查 |
| `#pragma check_stack()` 或 `#pragma check_stack`      | 否                  | **打开**后续函数的栈检查 |
| `#pragma check_stack(on)` 或 `#pragma check_stack +`  | 是或否              | **打开**后续函数的栈检查 |
| `#pragma check_stack(off)` 或 `#pragma check_stack -` | 是或否              | **关闭**后续函数的栈检查 |

> **注意**：前两条行为看起来“相反”，实际是因为默认行为与 `/Gs` 有关：当没有显式指定时，`check_stack` 会取反当前 `/Gs` 的设置。

### 示例代码

```cpp
// 编译选项: /Gs (默认开启栈检查)
#pragma check_stack()      // 由于 /Gs 开启，此指令关闭栈检查
void func1() {
    char buf[1024];
    buf[0] = 0;            // 不会进行栈检查
}

#pragma check_stack(on)    // 显式开启栈检查
void func2() {
    char buf[1024];
    buf[0] = 0;            // 会进行栈检查
}

#pragma check_stack(off)   // 显式关闭栈检查
void func3() {
    char buf[1024];
    buf[0] = 0;            // 不会进行栈检查
}

int main() {
    func1();
    func2();
    func3();
    return 0;
}
```

---

## 5. `code_seg`

### 语法

```cpp
#pragma code_seg( [ [ { push | pop}, ] [ identifier, ] ] [ "segment-name" [, "segment-class" ] )
```

### 作用

指定 OBJ 文件中存放**函数代码**的段。

### 备注

- OBJ 文件中存放代码的默认段是 `.text`。
- 其它用法与 `bss_seg` 类似（`push`/`pop`/`identifier`/`segment-name`）。
- 通常用于将不同模块的代码分开存放，以便进行内存管理或优化。

### 示例代码

```cpp
// pragma_directive_code_seg.cpp
void func1() {}      // 默认存放在 .text 段

#pragma code_seg(".my_code1")
void func2() {}      // 存放在 my_code1 段

#pragma code_seg(push, r1, ".my_code2")
void func3() {}      // 存放在 my_code2 段

#pragma code_seg(pop, r1)
void func4() {}      // 恢复为 my_code1 段

#pragma code_seg()   // 重置为默认 .text 段
void func5() {}      // 存放在 .text 段

int main() {}
```

---

## 6. `comment`

### 语法

```cpp
#pragma comment( comment-type [, commentstring] )
```

### 作用

将描述记录嵌入到目标文件或可执行文件中。

### 备注

- `comment-type` 是一个预定义标识符，用于指定注释的类型，可以是 `compiler`、`exestr`、`lib`、`linker` 和 `user` 五个标识符之一。
- `commentstring` 是可选字段，用于为一些 `comment-type` 提供附加的信息。因为 `commentstring` 是一个字符串，所以它适用字符串的所有规则（例如转义字符、嵌入的引号 `"` 和字符串连接）。

各类型详细说明：

| 类型       | 作用                                                         | 示例                                                         |
| ---------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| `compiler` | 将编译器名称和版本号信息存放到目标文件中，连接器（linker）会忽略该描述记录。如果提供了 `commentstring`，编译器会生成警告。 | `#pragma comment(compiler)`                                  |
| `exestr`   | 将 `commentstring` 存放到目标文件中，并且在连接时存放到可执行文件中。可执行文件运行后不会加载该字符串到内存，但是该字符串可以被能够在文件中搜索可打印字符串的程序检索到（例如版本号）。 | `#pragma comment(exestr, "Version 1.0")`                     |
| `lib`      | 将静态链接库信息放置到目标文件中。`commentstring` 必须包含库名或路径。链接器会像在命令行中输入一样搜索该库名。可以在一个源文件中放置多个库搜索信息。 | `#pragma comment(lib, "user32.lib")`                         |
| `linker`   | 在目标文件中放置连接程序选项。可以用来指定链接选项，例如强制包含符号：`#pragma comment(linker, "/include:__mySymbol")`。只有部分选项可用（如 `/DEFAULTLIB`、`/EXPORT`、`/INCLUDE`、`/MANIFESTDEPENDENCY`、`/MERGE`、`/SECTION`）。 | `#pragma comment(linker, "/MERGE:.rdata=.text")`             |
| `user`     | 将普通描述信息存放在目标文件中，链接器将忽略该描述信息。`commentstring` 可以嵌套连接字符串宏。 | `#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)` |

### 示例代码

```cpp
#include <iostream>

// 嵌入编译器版本信息
#pragma comment(compiler)

// 嵌入库依赖
#pragma comment(lib, "winmm.lib")

// 嵌入链接器指令：强制导出函数
#pragma comment(linker, "/EXPORT:MyFunction")

// 嵌入用户信息，包含编译日期时间
#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)

int main() {
    std::cout << "Hello, world!" << std::endl;
    return 0;
}
```

> **验证**：使用 `dumpbin /HEADERS test.exe` 可以看到嵌入的注释信息。

---

## 7. `component`

### 语法

```cpp
#pragma component( browser, { on | off }[, references [, name ]] )
#pragma component( minrebuild, on | off )
#pragma component( mintypeinfo, on | off )
```

### 作用

控制对源文件中浏览信息（browser information）或依赖信息的收集。

### 备注

#### 浏览器信息（Browser）

- 可以打开或关闭收集，也可以指定在收集信息时忽略特定名称或类型。
- 若要打开浏览信息的收集，必须先启用浏览信息功能（如 Visual Studio 中的“生成浏览信息”选项）。
- `references` 选项可以有也可以没有 `name` 参数。使用没有 `name` 参数的 `references` 选项将打开或者关闭引用信息的收集（然而继续收集其它浏览信息）。
- 使用有 `name` 和 `off` 参数的 `references` 选项将阻止从浏览信息窗口中出现引用到的名字。用这个语法可以忽略不感兴趣的名字和类型，从而减少浏览信息文件的大小。
- 若要防止预处理器扩展 `name`（如将 `NULL` 扩展为 `0`），请使用引号将其包含。

```cpp
// 关闭所有浏览信息收集
#pragma component(browser, off)

// 关闭引用信息收集（但其他浏览信息继续）
#pragma component(browser, off, references)

// 关闭 DWORD 类型的引用信息收集
#pragma component(browser, off, references, DWORD)

// 打开 DWORD 类型的引用信息收集
#pragma component(browser, on, references, DWORD)

// 防止预处理器扩展 name（如 NULL 扩展为 0）
#pragma component(browser, off, references, "NULL")
```

#### 最小化重建（Minimal Rebuild）

- Visual C++ 的最小化重建功能（`/Gm`）要求编译器创建并保存需要大量磁盘空间的 C++ 类依赖信息。
- 使用 `#pragma component(minrebuild, off)` 关闭信息收集，`#pragma component(minrebuild, on)` 重新打开。

#### 减少类型信息（Reduce Type Information）

- `mintypeinfo` 选项将减少指定区域的调试信息。此信息的量相当大，会影响 `.pdb` 和 `.obj` 文件。
- **不能**在 `mintypeinfo` 区域中调试类和结构。
- 使用 `mintypeinfo` 选项可帮助避免以下警告：
  ```
  LINK : warning LNK4018: too many type indexes in PDB "filename", discarding subsequent type information
  ```

> **更多信息**：参阅 [Enable Minimal Rebuild (`/Gm`)](https://docs.microsoft.com/en-us/cpp/build/reference/gm-enable-minimal-rebuild) 编译选项。

---

## 8. `conform`

### 语法

```cpp
#pragma conform(name [, show ] [, on | off ] [ [, push | pop ] [, identifier ] ] )
```

### 作用

指定 `/Zc:forScope` 编译器选项的运行时行为（强制标准 C++ 作用域规则）。

### 备注

- `name`：指定要修改的编译器选项的名称。唯一有效的 `name` 为 `forScope`。
- `show`（可选）：将 `name` 的当前设置（`true` 或 `false`）在编译期间以警告消息的方式显示。
- `on`、`off`（可选）：将 `name` 设置为 `on` 启用 `/Zc:forScope` 编译器选项。默认值为 `off`。
- `push`（可选）：将 `name` 的当前值推送到内部编译器堆栈。如果指定 `identifier`，则可为要推送到堆栈的 `name` 指定 `on` 或 `off` 值。
- `pop`（可选）：将 `name` 的值设置为位于内部编译器堆栈顶部的值，然后弹出堆栈。如果使用 `pop` 指定 `identifier`，则堆栈将弹回直到找到具有 `identifier` 的记录（也会弹出）；堆栈上的下一记录中的 `name` 的当前值将变为 `name` 的新值。如果使用不在堆栈上的记录中的 `identifier` 指定 `pop`，则将忽略 `pop`。
- `identifier`（可选）：可以与 `push` 或 `pop` 命令包含在一起。如果使用了 `identifier`，则还可以使用 `on` 或 `off` 说明符。

### 示例代码

```cpp
// pragma_directive_conform.cpp
// 编译选项: /W1
// 预期产生警告 C4811

#pragma conform(forScope, show)   // 显示当前设置（通常为 off）

#pragma conform(forScope, push, x, on)   // 压入并设置为 on
#pragma conform(forScope, show)          // 显示 on

#pragma conform(forScope, pop, x)        // 弹出
#pragma conform(forScope, show)          // 显示恢复为 off

int main() {
    for (int i = 0; i < 10; ++i) {
        // 标准 C++ 要求 i 的作用域仅限于 for 循环内
    }
    // i = 0;  // 如果 conform(forScope) 为 off，这行可能通过；为 on 则报错
    return 0;
}
```

---

## 9. `const_seg`

### 语法

```cpp
#pragma const_seg( [ [ { push | pop}, ] [ identifier, ] ] [ "segment-name" [, "segment-class" ] )
```

### 作用

指定 OBJ 文件中存放 **`const` 变量**的段。

### 备注

- OBJ 文件中存放 `const` 变量的默认段为 `.rdata`。
- 某些 `const` 变量（如标量）会自动内联到代码流中，内联代码不会存放在 `.rdata` 中。
- 在 `const_seg` 中定义需要动态初始化的对象会导致**未定义的行为**。
- 不带参数的 `#pragma const_seg` 会将段重置为 `.rdata`。

### 示例代码

```cpp
// pragma_directive_const_seg.cpp
// 编译选项: /EHsc
#include <iostream>

const int i = 7;               // 内联，不存储在 .rdata 中
const char sz1[] = "test1";    // 存储在 .rdata 中

#pragma const_seg(".my_const1")
const char sz2[] = "test2";    // 存储在 .my_const1 中

#pragma const_seg(push, stack1, ".my_const2")
const char sz3[] = "test3";    // 存储在 .my_const2 中

#pragma const_seg(pop, stack1) // 弹出 stack1
const char sz4[] = "test4";    // 存储在 .my_const1 中

#pragma const_seg()            // 重置为 .rdata
const char sz5[] = "test5";    // 存储在 .rdata 中

int main() {
    // 必须引用 const 数据才会被放入 .obj 文件
    std::cout << sz1 << sz2 << sz3 << sz4 << sz5 << std::endl;
    return 0;
}
```

---

## 10. `data_seg`

### 语法

```cpp
#pragma data_seg( [ [ { push | pop }, ] [ identifier, ] ] [ "segment-name" [, "segment-class" ] )
```

### 作用

指定 OBJ 文件中存放**已初始化变量**的数据段。

### 备注

- OBJ 文件中存放已初始化变量的默认段为 `.data`。
- 未初始化的变量被视为初始化为零并且存储在 `.bss` 中。
- 不带参数的 `data_seg` 将段重置为 `.data`。

### 示例代码

```cpp
// pragma_directive_data_seg.cpp
int h = 1;                     // 存储在 .data
int i = 0;                     // 未初始化，存储在 .bss

#pragma data_seg(".my_data1")
int j = 1;                     // 存储在 "my_data1"

#pragma data_seg(push, stack1, ".my_data2")
int l = 2;                     // 存储在 "my_data2"

#pragma data_seg(pop, stack1)  // 弹出 stack1，恢复为 .my_data1
int m = 3;                     // 存储在 "my_data1"

#pragma data_seg()             // 重置为默认 .data
int n = 4;                     // 存储在 .data

int main() { return 0; }
```

---

## 11. `deprecated`

### 语法

```cpp
#pragma deprecated( identifier1 [, identifier2, ...] )
```

### 作用

指示函数、类型或任何其他标识符已**废弃**（deprecated）。当编译器遇到废弃的符号时，会抛出警告 **C4995**。

### 备注

- 可以废弃宏名称，但是需要将宏名称包含在**引号**内，否则宏将展开。
- 可以使用 `__declspec(deprecated)` 修饰符废弃重载函数。

### 示例代码

```cpp
// pragma_directive_deprecated.cpp
// 编译选项: /W3
#include <stdio.h>

void oldFunc1(void) {}
void oldFunc2(void) {}

// 废弃这两个函数
#pragma deprecated(oldFunc1, oldFunc2)

// 废弃宏（注意引号）
#define OLD_MACRO 42
#pragma deprecated("OLD_MACRO")

int main() {
    oldFunc1();   // 警告 C4995: 'oldFunc1' was declared deprecated
    oldFunc2();   // 警告 C4995
    int x = OLD_MACRO;  // 警告 C4995
    return 0;
}
```

```cpp
// pragma_directive_deprecated2.cpp
#pragma deprecated(X)   // 废弃类 X
class X {               // 警告 C4995
public:
    void f() {}
};

int main() {
    X x;   // 警告 C4995
    return 0;
}
```

---

## 12. `detect_mismatch`

### 语法

```cpp
#pragma detect_mismatch( "name", "value" )
```

### 作用

将记录放在一个对象中。链接器将检查这些记录中的潜在不匹配项。如果链接项目时包含 `name` 相同但 `value` 不同的两个对象，则链接器将抛出错误 **LNK2038**。

### 备注

- 使用 `detect_mismatch` 可防止连接中存在不一致的对象文件（例如库版本冲突）。
- `name` 和 `value` 都是字符串，遵循转义字符和连接的字符串规则。同时区分大小写，不能包含逗号、等号、引号或 null 字符。

### 示例代码

```cpp
// 文件: file_a.cpp
#pragma detect_mismatch("myLib_version", "9")
int main() {
    return 0;
}
```

```cpp
// 文件: file_b.cpp
#pragma detect_mismatch("myLib_version", "1")
void someFunction() {}
```

如果使用命令行编译这两个文件：
```bash
cl file_a.cpp file_b.cpp
```
将会收到链接错误：
```
LNK2038: mismatch detected for 'myLib_version': value '9' doesn't match value '1' in file_b.obj
```

> **用途**：常用于确保库与主程序使用相同版本的内部数据结构。

---

## 13. `execution_character_set`

### 语法

```cpp
#pragma execution_character_set("target")
```

### 作用

指定运行时字符集（执行字符集）。

### 备注

- `target` 指定字符集名称，目前只支持 `"utf-8"`。
- **该指令在 Visual Studio 2015 Update 2 中已废弃**。建议使用编译选项 `/execution-charset:utf-8` 或 `/utf-8` 配合 `u8` 前缀来指定字符集。
- 默认情况下编译器采用系统当前字符集对源文件进行编码。在未指定编码集的情况下，在不同的电脑上进行编译可能会引起编译警告和错误。

### 示例代码

```cpp
#pragma execution_character_set("utf-8")

#include <iostream>
int main() {
    // 字符串字面量将按 UTF-8 编码存储
    const char* str = "你好，世界";
    std::cout << str << std::endl;
    return 0;
}
```

> **替代方案**（推荐）：
```bash
cl /utf-8 test.cpp
```

---

## 14. `fenv_access`

### 语法

```cpp
#pragma fenv_access [ON | OFF]
```

### 作用

禁用（`ON`）或启用（`OFF`）可能更改浮点状态标志和模式更改的优化。

### 备注

- 默认情况下，`fenv_access` 为 `OFF`。
- 有关浮点行为的详细信息，请参阅 `/fp`（指定浮点行为）编译选项。
- `fenv_access` 影响的优化类型包括：
  - 全局公共子表达式消除
  - 代码移动
  - 常量折叠

### 示例代码

```cpp
// pragma_directive_fenv_access_x86.cpp
// 编译选项: /O2 /fp:precise
// 处理器: x86
#include <stdio.h>
#include <float.h>
#include <errno.h>

#pragma fenv_access (on)

int main() {
    double z, b = 0.1, t = 0.1;
    unsigned int currentControl;
    errno_t err;

    // 设置浮点控制字为 24 位精度（单精度）
    err = _controlfp_s(&currentControl, _PC_24, _MCW_PC);
    if (err != 0) {
        printf_s("_controlfp_s failed!\n");
        return -1;
    }

    z = b * t;
    printf_s("out = %.15e\n", z);   // 输出: out = 9.999999776482582e-003
    return 0;
}
```

如果注释掉 `#pragma fenv_access (on)`，编译器会进行常量折叠，直接计算 `0.1*0.1=0.01`，输出会变为：
```
out = 1.000000000000000e-002
```

---

## 15. `float_control`

### 语法

```cpp
float_control( value, setting [push] | push | pop )
```

### 作用

指定函数的浮点行为，控制精确语义和异常语义。

### 备注

- `value`：可以是 `precise` 或 `except`。
- `setting`：可以是 `on` 或 `off`。
- 如果 `value` 是 `precise`，则同时设置 `precise` 和 `except` 为 `setting` 的值。
- `except` 只有在 `precise` 为 `on` 的情况下才能设置为 `on`。
- `push`：将当前的 `float_control` 设置压入编译器内部堆栈。
- `pop`：从内部编译器堆栈顶部移除 `float_control` 设置，使其成为新的 `float_control` 设置。
- 当 `except` 打开时，无法关闭 `float_control precise`。同样，当 `fenv_access` 打开时，无法关闭 `precise`。

### 示例代码

```cpp
// 从严格模式转换到快速模式
#pragma float_control(except, off)
#pragma fenv_access(off)
#pragma float_control(precise, off)
#pragma fp_contract(on)   // Itanium 处理器需要

// 从快速模式转换到严格模式
#pragma float_control(precise, on)
#pragma fenv_access(on)
#pragma float_control(except, on)
#pragma fp_contract(off)  // Itanium 处理器需要
```

```cpp
// 捕获浮点数溢出异常
// pragma_directive_float_control.cpp
// 编译选项: /EHa
#include <stdio.h>
#include <float.h>

double func() {
    return 1.1e75;   // 溢出 double 范围
}

#pragma float_control(except, on)

int main() {
    float u[1];
    unsigned int currentControl;
    errno_t err;

    // 启用溢出异常掩码
    err = _controlfp_s(&currentControl, ~_EM_OVERFLOW, _MCW_EM);
    if (err != 0) {
        printf_s("_controlfp_s failed!\n");
        return 1;
    }

    try {
        u[0] = func();
        printf_s("Fail: no exception\n");
        return 1;
    }
    catch (...) {
        printf_s("Pass: overflow exception caught\n");
        return 0;
    }
}
```

---

## 16. `fp_contract`

### 语法

```cpp
#pragma fp_contract [ON | OFF]
```

### 作用

决定是否使用浮点数**缩写形式**（contract），例如将 `a*b+c` 合并为一条 FMA（融合乘加）指令。

### 备注

- 默认情况下，`fp_contract` 为 `ON`。
- 关闭时，编译器会严格按表达式原样生成代码，不进行合并优化。

### 示例代码

```cpp
// pragma_directive_fp_contract.cpp
// 编译选项: /O2 /fp:fast
#include <stdio.h>
#include <float.h>

#pragma fp_contract(off)

int main() {
    double z, b, t;

    for (int i = 0; i < 5; i++) {
        b = i * 5.5;
        t = i * 56.025;
        // 关闭缩写后，乘法与加法分别执行，精度更高但速度稍慢
        z = t * i + b;
        printf("i=%d, out=%.15e\n", i, z);
    }
    return 0;
}
```

输出示例：
```
i=0, out=0.000000000000000e+000
i=1, out=6.152500152587891e+001
i=2, out=2.351000061035156e+002
i=3, out=5.207249755859375e+002
i=4, out=9.184000122070312e+002
```

如果打开 `fp_contract(on)`，结果可能略有不同（取决于处理器是否支持 FMA）。

---

## 17. `function`

### 语法

```cpp
#pragma function( function1 [, function2, ...] )
```

### 作用

强制对参数列表中的函数进行**显式函数调用**（而不是内联）。

### 备注

- 如果使用了 `#pragma intrinsic`（或 `/Oi` 编译选项）指示编译器生成内联函数，则可以使用 `#pragma function` 来强制某些函数采用普通调用。
- 一旦设定了 `function` 编译指令，它将在包含指定函数的第一个函数定义中生效。效果持续到源文件结尾或遇到 `intrinsic` 编译指令指定相同的函数。
- `function` 仅能在函数的外部使用（全局级别）。

### 示例代码

```cpp
// pragma_directive_function.cpp
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// 使用内联形式
#pragma intrinsic(memset, strlen)

char* set_str_after_word(char* string, char ch) {
    int len = strlen(string);   // 使用内联版本
    int i;
    for (i = 0; i < len; i++) {
        if (isspace(*(string + i)))
            break;
    }
    for (; i < len; i++)
        *(string + i) = ch;
    return string;
}

// 强制 strlen 使用普通调用版本
#pragma function(strlen)

char* set_str(char* string, char ch) {
    // memset 仍是内联，但 strlen 调用库函数
    return (char*)memset(string, ch, strlen(string));
}

int main() {
    char str[20] = "Now is the time";
    printf("After set_str_after_word: '%s'\n", set_str_after_word(str, '*'));
    printf("After set_str: '%s'\n", set_str(str, '!'));
    return 0;
}
```

输出：
```
After set_str_after_word: 'Now************'
After set_str: '!!!!!!!!!!!!!!!!!!!'
```

---

## 18. `hdrstop`

### 语法

```cpp
#pragma hdrstop [( "filename" )]
```

### 作用

提供对预编译头文件名和编译状态的保存位置的额外控制。

### 备注

- `filename` 是要使用或创建的预编译头文件的名称（取决于指定 `/Yu` 还是 `/Yc`）。
- 如果 `filename` 不包含路径说明，则假定预编译头文件与源文件处于同一目录中。
- 当用 `/Yc` 编译时，如果 C 或 C++ 文件中包含了一个 `hdrstop` 编译指令，则编译器保存**编译指令之前**的编译状态。编译指令之后的编译状态不被保存。
- `filename` 指定了预编译状态保存的文件名。文件名是字符串，因此受 C/C++ 的字符串约束。必须通过引号同时使用转义符（反斜杠）来指定目录名称。例如：
  ```cpp
  #pragma hdrstop( "c:\\projects\\include\\myinc.pch" )
  ```
- 预编译头文件的名称根据以下规则按优先顺序决定：
  1. `/Fp` 编译选项的参数
  2. `#pragma hdrstop` 的 `filename` 参数
  3. 扩展名为 `.PCH` 的源文件基名称
- 如果 `/Yc` 和 `/Yu` 以及 `hdrstop` 都未指定文件名，则将源文件的基名称用作预编译头文件的名称。
- 可以使用预处理命令来执行宏替换，如下所示：
  ```cpp
  #define INCLUDE_PATH "c:\\progra~1\\devstsu~1\\vc\\include\\"
  #define PCH_FNAME "PROG.PCH"
  #pragma hdrstop( INCLUDE_PATH PCH_FNAME )
  ```
- `hdrstop` 必须出现在任何数据或函数声明/定义的外部。
- `hdrstop` 必须在源文件而不是头文件中指定。

### 示例代码

```cpp
// 文件: main.cpp
#include <windows.h>
#include "myheader.h"

__inline void Disp(char* szToDisplay) {
    // 显示字符串的代码
}

// 预编译头状态保存至此（之前包含的头文件和内联定义）
#pragma hdrstop("my.pch")

// 此后的代码不会被包含在预编译头中
int main() {
    Disp("Hello");
    return 0;
}
```

配合编译命令：
```bash
cl /Ycmy.pch /Fpmy.pch main.cpp   # 创建预编译头
cl /Yumy.pch /Fpmy.pch main.cpp   # 使用预编译头
```

---

## 19. `include_alias`

### 语法

```cpp
#pragma include_alias( "long_filename", "short_filename" )
#pragma include_alias( <long_filename>, <short_filename> )
```

### 作用

指定 `short_filename` 用于 `long_filename` 的别名。

### 备注

- 有些文件系统允许长度超过 8.3 FAT 文件系统限制的文件名。因为较长的头文件名的前 8 个字符可能不是唯一的，因此编译器不能简单地将较长的名称截断为 8.3。
- 当编译器遇到 `long_filename` 字符串时，都将用 `short_filename` 进行替换，并改为查找 `short_filename` 头文件。
- 该指令必须在 `#include` 指令之前出现。
- 别名必须要完全符合规范，包括大小写、拼写、双引号和尖括号的使用。
- `include_alias` 对文件名进行简单的字符串匹配，不会对文件名进行其它校验。
- `include_alias` 不会替换 `/Yu` 和 `/Yc` 以及 `hdrstop` 指令参数的头文件名。
- 可以使用 `include_alias` 将任何头文件名映射到另一个头文件名。
- 不要将用双引号括起来的文件名与用尖括号括起的文件名混淆使用。
- 以下指令将报错：`#pragma include_alias(<header.h>, "header.h")`。
- `include_alias` 不支持传递性。

### 示例代码

```cpp
// 假设头文件名很长且前8字符不唯一
#pragma include_alias( "AppleSystemHeaderQuickdraw.h", "quickdra.h" )
#pragma include_alias( "AppleSystemHeaderFruit.h", "fruit.h" )
#pragma include_alias( "GraphicsMenu.h", "gramenu.h" )

// 实际包含时，编译器会映射到短文件名
#include "AppleSystemHeaderQuickdraw.h"   // 实际包含 quickdra.h
#include "AppleSystemHeaderFruit.h"       // 实际包含 fruit.h
#include "GraphicsMenu.h"                 // 实际包含 gramenu.h
```

```cpp
// 映射不同路径
#pragma include_alias( "api.h", "c:\\version1.0\\api.h" )
#include "api.h"   // 实际包含 c:\version1.0\api.h
```

```cpp
// 错误示例：引号与尖括号不匹配
#pragma include_alias(<stdio.h>, "newstdio.h")  // 正确
#include <stdio.h>   // 会映射到 newstdio.h
#include "stdio.h"   // 不会映射，因为引号不匹配
```

---

## 20. `init_seg`

### 语法

```cpp
#pragma init_seg({ compiler | lib | user | "section-name" [, func-name] })
```

### 作用

指定影响启动代码的执行顺序的关键字或代码段，控制全局静态对象的构造顺序。

### 备注

- 本节中 `segment` 和 `section` 的含义是可互换的。
- 由于全局静态对象的初始化可能涉及代码执行，因此必须指定一个关键字用于确定对象的构造时间。
- `init_seg` 在动态链接库（DLL）或需要初始化的库中使用特别重要。
- `init_seg` 的选项如下：
  - `compiler`：保留给 Microsoft C 运行库初始化使用，该组中的对象会**最先**构造。
  - `lib`：用于第三方类库的初始化，该组中的对象在 `compiler` 之后构造。
  - `user`：可供任何用户使用，该组中的对象**最后**构造。
  - `section-name`：显式指定初始化的段名。该段中的对象通过显式构造函数构造，并且对象地址存放在该段中。该段中存放着用于初始化该段之后的全局变量的辅助函数指针。
  - `func-name`：指定程序退出时替换 `atexit` 的函数。在自己的退出函数中可以控制不同模块的析构顺序。这个函数必须具有和 `atexit` 函数相同的形式：`int funcname(void (__cdecl *)(void));`
- 可以通过显式指定段名称来延迟对象的初始化，但是必须为每个静态对象显式调用构造函数进行初始化。
- `func-name` 不需要用引号包含。
- 各个对象存放在由 `xxx_seg` 编译指令指定的段中。
- 默认情况下，`init_seg` 部分是只读的。如果段名称是 `.CRT`，则编译器会自动将段属性修改为只读，即使段标志为读写。
- 每个源文件中只能出现一次 `init_seg`。
- 模块中声明的对象不会由 C 运行时自动初始化，需要我们主动调用。如果对象没有用户定义的构造函数，系统会生成默认构造函数，但是还是需要我们主动调用。

### 示例代码

```cpp
// pragma_directive_init_seg.cpp
#include <stdio.h>
#pragma warning(disable : 4075)

typedef void (__cdecl *PF)(void);
int cxpf = 0;      // 析构函数数量
PF pfx[200];       // 析构函数指针数组

int myexit(PF pf) {
    pfx[cxpf++] = pf;
    return 0;
}

struct A {
    A() { puts("A()"); }
    ~A() { puts("~A()"); }
};

struct B {
    B() { puts("B()"); }
    ~B() { puts("~B()"); }
};

struct C {
    C() { puts("C()"); }
    ~C() { puts("~C()"); }
};

// 由 CRT 启动代码构造和析构（在 pragma init_seg 之前）
A aaaa;

// 创建自定义段，用于存放初始化函数指针
#pragma section(".mine$a", read)
__declspec(allocate(".mine$a")) const PF InitSegStart = (PF)1;

#pragma section(".mine$z", read)
__declspec(allocate(".mine$z")) const PF InitSegEnd = (PF)1;

void InitializeObjects() {
    const PF *x = &InitSegStart;
    for (++x; x < &InitSegEnd; ++x)
        if (*x) (*x)();
}

void DestroyObjects() {
    while (cxpf > 0) {
        --cxpf;
        (pfx[cxpf])();
    }
}

// 指定初始化段和自定义退出函数
#pragma init_seg(".mine$m", myexit)

B bbbb;
C cccc;

int main() {
    InitializeObjects();
    DestroyObjects();
    return 0;
}
```

输出（顺序）：
```
A()
B()
C()
~C()
~B()
~A()
```

> **注意**：`A` 的构造发生在 `init_seg` 之前，因此它最先构造、最后析构。

---

## 21. `inline_depth`

### 语法

```cpp
#pragma inline_depth( [n] )
```

### 作用

指定函数的内联深度，超过深度 `n` 的内联扩展均转为函数调用。

### 备注

- `inline_depth` 的控制范围是用 `inline`、`__inline` 标记或在 `/Ob2` 编译选项下能够自动内联的函数。
- `n` 的取值范围是 `[0, 255]`，`0` 代表禁用内联，`255` 表示不限制内联深度。未指定 `n` 值情况下默认为 `254`。
- `inline_depth` 可以控制一系列函数调用的内联深度。例如，假设内联深度是 4，如果 A 调用 B 然后调用 C，所有的 3 次调用都将做内联扩展。如果设置深度为 2，则只有 A 和 B 被扩展，而 C 仍然作为函数调用。
- 要使用该指令，必须设置编译选项为 `/Ob1` 或 `/Ob2`，并且在该指令设定内联深度后的**第一个函数**生效。
- 嵌套的内联深度设置只能递减，不能递增。如果内联深度为 6，同时后续通过 `inline_depth` 编译指令设置为 8，则内联深度仍保持为 6。
- `inline_depth` 对使用 `__forceinline` 标记的函数无效。
- 递归函数的内联深度为 16。

### 示例代码

```cpp
// 编译选项: /Ob2
#pragma inline_depth(2)

void funcC() {
    // 深度 3，不会被内联
}

void funcB() {
    funcC();   // 深度 2，funcC 不会被内联
}

void funcA() {
    funcB();   // 深度 1，funcB 可能被内联，但 funcC 不会
}

#pragma inline_depth(0)   // 禁用内联
void funcD() {
    // 所有内联被禁止
}
```

---

## 22. `inline_recursion`

### 语法

```cpp
#pragma inline_recursion( [{on | off}] )
```

### 作用

控制直接或间接递归调用函数的内联展开。

### 备注

- `inline_recursion` 的控制范围是用 `inline`、`__inline` 标记或在 `/Ob2` 编译选项下能够自动内联的函数。
- 要使用该指令，必须设置编译选项为 `/Ob1` 或 `/Ob2`，并且在该指令设定开关后的**第一个函数**生效。
- 默认情况下开关为 `off`。
- 如果开关为 `off`，且内联函数调用自身，则该函数只展开一次。
- 如果开关为 `on`，则该函数将展开多次，直至达到使用 `inline_depth` 设定的内联深度。

### 示例代码

```cpp
// 编译选项: /Ob2
#pragma inline_recursion(on)
#pragma inline_depth(3)

inline int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);   // 递归内联，最多展开 3 层
}

int main() {
    int x = factorial(5);   // 内联展开 3 层后，剩余调用变为函数调用
    return 0;
}
```

---

## 23. `intrinsic`

### 语法

```cpp
#pragma intrinsic( function1 [, function2, ...] )
```

### 作用

指定参数列表中的函数为**内建函数**（intrinsic function），编译器可以将其替换为特殊的内部代码，通常比普通函数调用更高效。

### 备注

- `intrinsic` 告诉编译器函数行为明确，如果性能更好的情况下可以直接调用函数而无需采用内联展开。
- 在 `intrinsic` 指令出现之后的第一个包含内建函数的函数定义开始生效。作用效果持续到源文件末尾或者遇到 `function` 编译指令指定相同的函数。
- `intrinsic` 只能在函数定义外使用（全局层次）。
- 下面是具有内建形式的常见库函数：

| 函数       | 函数      | 函数     |
| ---------- | --------- | -------- |
| `_disable` | `_outp`   | `fabs`   |
| `_enable`  | `_outpw`  | `labs`   |
| `_inp`     | `_rotl`   | `memcmp` |
| `_inpw`    | `_rotr`   | `memcpy` |
| `_lrotl`   | `_strset` | `memset` |
| `_lrotr`   | `abs`     | `strcat` |
| `strcmp`   | `strcpy`  | `strlen` |

- 使用 `intrinsic` 函数的程序运行速度更快，因为它没有函数调用的开销，但是由于插入了额外的代码，程序体积会更大。
- 下列浮点数函数不具有真正的 `intrinsic` 形式，它们可以直接将浮点参数传到浮点芯片中，而不是将参数压入程序堆栈：`acos`、`cosh`、`pow`、`tanh`、`asin`、`fmod`、`sinh`。
- 当指定编译选项 `/Oi`、`/Og` 和 `/fp:fast`（或任何包含 `/Og` 的选项：`/Ox`、`/O1` 和 `/O2`）时，下面列出的浮点函数将具有真正的 `intrinsic` 形式：`atan`、`exp`、`log10`、`sqrt`、`atan2`、`log`、`sin`、`tan`、`cos`。

### 示例代码

```cpp
// pragma_directive_intrinsic.cpp
// 处理器: x86
#include <dos.h>   // 定义 _disable, _enable
#pragma intrinsic(_disable)
#pragma intrinsic(_enable)

void f1(void) {
    _disable();    // 内建形式，直接生成 CLI 指令
    // 此处不应被中断的关键代码
    _enable();     // 内建形式，直接生成 STI 指令
}

int main() {
    f1();
    return 0;
}
```

---

## 24. `loop`

### 语法

```cpp
#pragma loop( hint_parallel(n) )
#pragma loop( no_vector )
#pragma loop( ivdep )
```

### 作用

控制对循环代码的自动并行化和自动向量化（主要用于 Intel 编译器及 Visual C++ 的 OpenMP 支持）。

### 备注

- `hint_parallel(n)`：提示编译器对循环代码进行 `n` 线程并行。`n` 为 0 时采用最大数量的线程。该指令只是一个提示，并不是命令，编译器不能保证将循环并行化。
- 如果循环具有数据依赖或结构化问题（例如循环存储了循环体外的标量），将不会并行化。
- 只有指定 `/Qpar` 编译选项时，`hint_parallel` 才会生效。
- `no_vector`：默认情况下，自动向量化是打开的。使用该指令可以禁止对该循环进行向量化。
- `ivdep`：提示编译器忽略该循环的向量依赖（即告诉编译器循环没有循环携带的依赖）。
- `loop` 编译指令需要放在紧挨着循环体之前，不能放在循环体内，并且对接下来的循环生效。
- 可以对同一个循环体指定多个 `loop` 选项，但是每个选项都要使用单独一条指令。

### 示例代码

```cpp
// 编译选项: /Qpar /arch:AVX2
#pragma loop(hint_parallel(4))
#pragma loop(ivdep)
for (int i = 0; i < 10000; ++i) {
    a[i] = b[i] * c[i] + d[i];
}

#pragma loop(no_vector)
for (int j = 0; j < 1000; ++j) {
    // 该循环不会被自动向量化
    sum += data[j];
}
```

---

## 25. `make_public`

### 语法

```cpp
#pragma make_public(type)
```

### 作用

指示本机类型应具有**公共程序集可访问性**（在 `/clr` 编译时）。

### 备注

- `type`：需要具有公共程序集可访问性的类型名称。
- 如果要引用的本机类型来自无法更改的 `.h` 文件，则 `make_public` 会很有用。
- 若要在带有公共程序集可见性的类型中使用公共函数签名中的本机类型，则本机类型还必须具有公共程序集可访问性，否则编译器将发出警告 C4692。
- `make_public` 必须在全局范围内使用，并且仅在编译指令指定处到源文件末尾有效。
- 可以隐式或显式将本机类型设为私有；详细信息请参阅[类型可见性](https://docs.microsoft.com/en-us/cpp/dotnet/how-to-define-and-consume-classes-and-structs-cpp-cli#type-visibility)。

### 示例代码

```cpp
// make_public_pragma.h
struct Native_Struct_1 { int i; };
struct Native_Struct_2 { int i; };
```

```cpp
// make_public_pragma.cpp
// 编译选项: /c /clr /W1
#pragma warning(default : 4692)
#include "make_public_pragma.h"

#pragma make_public(Native_Struct_1)   // 使 Native_Struct_1 成为公共类型

public ref struct A {
    void Test(Native_Struct_1 u) { u.i = 0; }   // OK，Native_Struct_1 是公共的
    void Test(Native_Struct_2 u) { u.i = 0; }   // 警告 C4692: 本机类型 Native_Struct_2 不是公共的
};
```

---

## 26. `managed`, `unmanaged`

### 语法

```cpp
#pragma managed
#pragma unmanaged
#pragma managed([push,] on | off)
#pragma managed(pop)
```

### 作用

启用函数级控制以将函数编译为托管（.NET）或非托管（本机）函数。

### 备注

- `/clr` 编译器选项提供了用于将函数编译为托管或非托管函数的模块级控制。
- 非托管函数将在本机平台编译成机器码，并且由 CLR 传给本机平台运行。
- 启用 `/clr` 编译选项后，默认情况下将函数编译为托管函数。
- 该编译指令需要放在函数体**之前**，不能放在函数体内。
- 该编译指令必须放在 `#include` 指令之后。
- 如果未设定 `/clr` 编译选项，编译器将忽略 `managed` 和 `unmanaged` 编译指令。
- 该编译指令可以在模板函数实例化之后确定该函数是否为托管函数。
- 更多详细信息请参阅[混合程序集的初始化](https://docs.microsoft.com/en-us/cpp/dotnet/initialization-of-mixed-assemblies)。

### 示例代码

```cpp
// pragma_directives_managed_unmanaged.cpp
// 编译选项: /clr
#include <stdio.h>

// func1 是托管函数（默认）
void func1() {
    System::Console::WriteLine("In managed function.");
}

// 将托管状态压入堆栈，并设置为非托管
#pragma managed(push, off)

// func2 是非托管函数
void func2() {
    printf("In unmanaged function.\n");
}

// 恢复之前的托管状态（弹出）
#pragma managed(pop)

// main 是托管函数
int main() {
    func1();   // 托管输出
    func2();   // 非托管输出
    return 0;
}
```

输出：
```
In managed function.
In unmanaged function.
```

---

## 27. `message`

### 语法

```cpp
#pragma message( messagestring )
```

### 作用

在不中断编译的情况下将字符串输出到标准输出窗口（例如 Visual Studio 的输出窗口或命令行）。

### 备注

- 该编译指令的典型用法是输出信息性消息，例如报告编译配置或版本。
- `messagestring` 可以是宏，也可以是字符串和宏的拼接。
- 在该编译指令中使用的宏应该返回字符串类型，否则需要主动将宏返回信息转换成字符串类型。

### 示例代码

```cpp
// pragma_directives_message1.cpp
// 编译选项: /LD
#if _M_IX86 >= 500
#pragma message("_M_IX86 >= 500")   // 显示消息
#endif

#pragma message("")

#pragma message("Compiling " __FILE__)
#pragma message("Last modified on " __TIMESTAMP__)

#pragma message("")

// 带行号的消息
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#pragma message(__FILE__ "[" STRING(__LINE__) "]: test")

int main() {
    return 0;
}
```

在编译时输出：
```
_M_IX86 >= 500

Compiling pragma_directives_message1.cpp
Last modified on Thu Apr 16 14:30:00 2026

pragma_directives_message1.cpp[15]: test
```

---

## 28. `omp`

### 语法

```cpp
#pragma omp directive
```

### 作用

将一个或多个 OpenMP 指令与任何可选指令子集一起使用，用于并行编程。

### 示例（简要）

```cpp
#include <omp.h>
#include <stdio.h>

int main() {
    #pragma omp parallel num_threads(4)
    {
        printf("Hello from thread %d\n", omp_get_thread_num());
    }
    return 0;
}
```

> **注意**：使用 OpenMP 需要在编译时指定 `/openmp` 选项。

---

## 29. `once`

### 语法

```cpp
#pragma once
```

### 作用

指定该文件在编译源文件时仅被编译器包含（打开）一次。这是一种防止头文件重复包含的机制。

### 备注

- 该编译指令可以减少编译次数，通常被称为“多次包含优化”（multiple-include optimization）。
- 效果类似于传统的 `#include guard` 手法（使用 `#ifndef` ... `#define` ... `#endif`）。
- 该编译指令还有助于避免违反单一定义原则（要求所有模板、类型、函数和对象在代码中的定义不得超过一个）。
- 如果要将代码移植到不支持该编译指令的编译器平台，则建议使用传统的 `#include guard` 手法。

### 示例代码

```cpp
// header.h
#pragma once
// 此文件中的内容在每个编译单元中只被包含一次
struct MyStruct {
    int x;
};
```

等价于传统写法：
```cpp
// header.h
#ifndef HEADER_H_
#define HEADER_H_
struct MyStruct {
    int x;
};
#endif
```

---

## 30. `optimize`

### 语法

```cpp
#pragma optimize( "[optimization-list]", {on | off} )
```

### 作用

指定编译器对函数的优化类型。

### 备注

- 该编译指令必须放在函数体外部，并且对该指令设定后的函数生效。
- `on` 和 `off` 用于打开或关闭 `optimization-list` 指定的优化类型。
- `optimization-list` 可以是下表中显示的 0 个或多个参数，与 `/O` 编译选项参数相同：

| 参数       | 优化类型                               |
| ---------- | -------------------------------------- |
| `g`        | 启用全局优化                           |
| `s` 或 `t` | 指定短（`s`）或快（`t`）的机器代码序列 |
| `y`        | 在程序堆栈上生成帧指针                 |

- 特殊形式：`optimization-list` 为 `""` 空字符串，表示重置为默认优化。

### 示例代码

```cpp
// 等价于 /Os 编译选项（优化代码大小）
#pragma optimize("ts", on)

void func1() {
    // 优化大小和速度
}

// 关闭所有优化
#pragma optimize("", off)
void func2() {
    // 无优化，便于调试
}

// 重新打开优化
#pragma optimize("", on)
void func3() {
    // 恢复默认优化
}
```

---

## 31. `pack`

### 语法

```cpp
#pragma pack( [ show ] | [ push | pop ] [, identifier ] , n )
```

### 作用

指定结构、联合和类成员的对齐方式（字节对齐）。

### 备注

- 紧凑压缩一个类就是将类成员变量在内存中紧挨着存放，也就是说某些成员变量的对齐边界比该变量类型的默认情况要小。例如 `UINT64` 类型默认是 8 字节对齐的，通过 `pack` 可以指定为 4 字节对齐。
- `pack` 指令提供了数据声明级别的控制，而 `/Zp` 编译选项提供了模块级别的控制。
- `pack` 指令对设定之后的第一个 `struct`、`union` 或者 `class` 声明生效，该指令对定义无效。
- 不带参数的 `pack` 指令默认将 `/Zp` 设置的对齐字节数作为 `n` 参数。当 `/Zp` 未设定时，默认 8 字节对齐。
- 修改数据结构的对齐方式之后，可以减少内存的使用，但可能会降低性能或由于未对齐访问引起硬件异常，可以通过 `SetErrorMode` 来修改异常行为模式。
- `show`：可选参数，通过警告消息显示当前的对齐方式。
- `push`：可选参数，将当前对齐方式压入编译堆栈，设置后续对齐方式为 `n`，`n` 未指定则采用当前对齐方式。
- `pop`：可选参数，弹出编译堆栈顶部的元素，如果 `n` 未设定，则将弹出的顶部元素作为新的对齐方式，否则对齐方式为 `n`。例如：`#pragma pack(pop, 16)`。如果 `pop` 指令指定了 `identifier`（如 `#pragma pack(pop, r1)`），则弹出 `r1` 之前包括 `r1` 的所有元素，并将新的栈顶元素作为对齐方式。如果未找到 `identifier`，则忽略该指令。
- `identifier`：可选参数，指定压入堆栈或弹出堆栈的标示符。
- `n`：可选参数，指定用于对齐的值，单位为字节。有效值是 `1`、`2`、`4`、`8` 和 `16`。

### 示例代码

```cpp
#include <stddef.h>
#include <stdio.h>

struct S {
    int i;      // size 4
    short j;    // size 2
    double k;   // size 8
};
// 默认对齐（通常为 8），成员偏移：i=0, j=4, k=8，结构大小 16

#pragma pack(2)   // 设置为 2 字节对齐
struct T {
    int i;
    short j;
    double k;
};
// 对齐为 2，成员偏移：i=0, j=4, k=6，结构大小 14

int main() {
    printf("S: offset(i)=%zu, offset(j)=%zu, offset(k)=%zu, size=%zu\n",
           offsetof(S, i), offsetof(S, j), offsetof(S, k), sizeof(S));
    printf("T: offset(i)=%zu, offset(j)=%zu, offset(k)=%zu, size=%zu\n",
           offsetof(T, i), offsetof(T, j), offsetof(T, k), sizeof(T));
    return 0;
}
```

输出：
```
S: offset(i)=0, offset(j)=4, offset(k)=8, size=16
T: offset(i)=0, offset(j)=4, offset(k)=6, size=14
```

```cpp
// 显示当前对齐值
#pragma pack(show)   // 警告 C4810: value of pragma pack(show) == 8

#pragma pack(push, r1, 16)
#pragma pack(show)   // 显示 16

#pragma pack(pop, r1)
#pragma pack(show)   // 恢复为 8
```

---

## 32. `pointers_to_members`

### 语法

```cpp
#pragma pointers_to_members( pointer-declaration, [most-general-representation] )
```

### 作用

指定能否在类定义之前声明指向类成员的指针，以及类成员是否控制指针大小以及如何解析类成员指针。

### 备注

- 该编译指令功能等价于 `/vmx` 编译选项或继承关键字。
- `pointer-declaration`：指定在关联的函数定义之前还是之后声明指向成员的指针。参数如下：

| 参数              | 注释                                                         |
| ----------------- | ------------------------------------------------------------ |
| `full_generality` | 生成安全（有时并非最佳）代码。如果在关联的类定义之前声明指向成员的任何指针，请使用 `full_generality`。此参数始终使用 `most-general-representation` 参数所指定的指针表示形式。等价于 `/vmg`。 |
| `best_case`       | 对指向成员的所有指针使用最佳表示形式生成安全的最佳代码。**要求**在声明指向类成员的指针之前定义此类。系统默认为 `best_case`。 |

- `most-general-representation`：指定了最小指针表示形式，用于编译器可以安全地引用指向编译单元中的类成员的任何指针。参数如下：

| 参数                   | 注释                                                         |
| ---------------------- | ------------------------------------------------------------ |
| `single_inheritance`   | 最常见的表示形式为单一继承（指向成员函数的指针）。如果类定义（已为其声明指向成员的指针）的继承模型为多重继承或虚拟继承，则会导致出现错误。 |
| `multiple_inheritance` | 最常见的表示形式为多重继承（指向成员函数的指针）。如果类定义（已为其声明指向成员的指针）的继承模型为虚拟继承，则会导致出现错误。 |
| `virtual_inheritance`  | 最常见的表示形式为虚拟继承（指向成员函数的指针）。不会导致错误。这是 `#pragma pointers_to_members(full_generality)` 的默认参数。 |

> **建议**：将该编译指令放在要使用的源文件中，并且放置在所有 `#include` 之后，避免影响其它源文件。

### 示例代码

```cpp
// 指定单一继承模型
#pragma pointers_to_members(full_generality, single_inheritance)

class Base { public: virtual void f(); };
class Derived : public Base { public: void f(); };

// 指向成员函数的指针可以安全使用
void (Derived::*pmf)() = &Derived::f;
```

---

## 33. `pop_macro` 与 `push_macro`

### 语法

```cpp
#pragma push_macro("macro_name")
#pragma pop_macro("macro_name")
```

### 作用

- `push_macro`：将 `macro_name` 宏的当前值压入堆栈顶端。
- `pop_macro`：将 `macro_name` 宏的值设置为堆栈顶部的值，并弹出。

### 备注

- 使用 `pop_macro` 之前必须先使用 `push_macro`。

### 示例代码

```cpp
// pragma_directives_push_pop_macro.cpp
#include <stdio.h>

#define X 1
#define Y 2

int main() {
    printf("X=%d, Y=%d\n", X, Y);   // X=1, Y=2

    #define Y 3   // 重新定义 Y，会触发警告 C4005
    #pragma push_macro("Y")   // 保存当前 Y 的值（3）
    #pragma push_macro("X")   // 保存当前 X 的值（1）

    #define X 2   // 临时改变 X
    printf("X=%d, Y=%d\n", X, Y);   // X=2, Y=3

    #pragma pop_macro("X")    // 恢复 X 为 1
    printf("X=%d, Y=%d\n", X, Y);   // X=1, Y=3

    #pragma pop_macro("Y")    // 恢复 Y 为 2
    printf("X=%d, Y=%d\n", X, Y);   // X=1, Y=2

    return 0;
}
```

输出：
```
X=1, Y=2
X=2, Y=3
X=1, Y=3
X=1, Y=2
```

---

## 34. `region`, `endregion`

### 语法

```cpp
#pragma region name
#pragma endregion comment
```

### 作用

利用 `#pragma region` 指定在 Visual Studio 的大纲功能（代码折叠）中可展开或折叠的代码块。

### 备注

- `comment`：（可选参数），在代码编辑器中显示的注释。
- `name`：（可选参数），区域名称，此名称将在代码编辑器中显示。
- `#pragma endregion` 标记 `#pragma region` 块的结尾。两者必须配合使用。

### 示例代码

```cpp
#pragma region Utility Functions
void PrintHello() {
    printf("Hello\n");
}
void PrintWorld() {
    printf("World\n");
}
#pragma endregion Utility Functions

int main() {
    PrintHello();
    PrintWorld();
    return 0;
}
```

在 Visual Studio 中，`Utility Functions` 区域可以折叠或展开。

---

## 35. `runtime_checks`

### 语法

```cpp
#pragma runtime_checks( "[runtime_checks]", {restore | off} )
```

### 作用

禁用或还原 `/RTC` 编译选项设置的运行时检查。

### 备注

- 在 `/RTC` 编译选项未启用时不能使用该编译指令还原运行时检查。例如：如果未指定 `/RTCs`，则指定 `#pragma runtime_checks( "s", restore)` 也不会启用堆栈帧校验。
- 该编译指令必须放在函数体外部，而且在该指令之后的**第一个函数定义**开始生效。
- `restore` 和 `off` 分别代表打开和关闭指定的运行时检查。参数取值如下：

| 参数 | 运行时检查的类型                               |
| ---- | ---------------------------------------------- |
| `s`  | 启用堆栈（帧）校验                             |
| `c`  | 提示某个数值向较小数据类型赋值时会导致数据丢失 |
| `u`  | 提示某个变量在定义之前被使用                   |

- 上表中的参数与 `/RTC` 编译选项的参数相同。例如：`#pragma runtime_checks( "sc", restore )`。
- 特殊形式：参数 `runtime_checks` 为空字符串 `""`，对应功能如下：
  - `off`：禁用上表中的运行时错误检查。
  - `restore`：重置运行时错误检查为 `/RTC` 编译选项中指定的类型。

### 示例代码

```cpp
// 编译选项: /RTCsu
#pragma runtime_checks("", off)   // 禁用所有运行时检查
void NoCheckFunc() {
    int x;
    int y = x;   // 不会报告使用未初始化变量
}

#pragma runtime_checks("", restore)  // 恢复 /RTC 设置
void CheckFunc() {
    int x;
    int y = x;   // 将报告 C4700: 使用了未初始化的局部变量
}
```

---

## 36. `section`

### 语法

```cpp
#pragma section( "section-name" [, attributes] )
```

### 作用

在 OBJ 文件中创建一个新的段。

### 备注

- 此处 `segment` 和 `section` 的概念是等价的。
- 一旦定义了一个段，将在编译的剩余部分生效，但是必须通过 `__declspec(allocate)` 来指定段信息，否则将没有数据存放在该段。
- `section-name`：必须参数，指定段名。该名不能与标准段名冲突。参见 `/SECTION` 查看不能使用的段名。
- `attributes`：可选参数，指定段属性，各个属性间用逗号分隔。可选属性如下：

| 属性      | 注释                                        |
| --------- | ------------------------------------------- |
| `read`    | 可读                                        |
| `write`   | 可写                                        |
| `execute` | 可执行                                      |
| `shared`  | 共享                                        |
| `nopage`  | 不可分页，对于 Win32 设备驱动程序很有用     |
| `nocache` | 不可缓存，对于 Win32 设备驱动程序很有用     |
| `discard` | 可丢弃，对于 Win32 设备驱动程序很有用       |
| `remove`  | 非常驻内存，仅适用于虚拟设备驱动程序（VxD） |

### 示例代码

```cpp
// pragma_section.cpp
#pragma section("mysec", read, write)
// 使用 __declspec(allocate) 将变量放入 mysec 段
__declspec(allocate("mysec")) int i = 0;

// 没有使用 allocate 的变量仍放在默认数据段
int j = 0;

int main() {
    i = 10;
    return 0;
}
```

使用 `dumpbin /SECTION:mysec test.obj` 可以查看段的内容。

---

## 37. `setlocale`

### 语法

```cpp
#pragma setlocale( "[locale-string]" )
```

### 作用

定义在转换宽字符常量和字符串时使用的区域设置（国家/地区和语言）。

### 备注

- 由于编译可能在不同的区域设置环境下进行，而不同区域将多字节转换为宽字符的算法可能存在差异，因此可以使用 `setlocale` 来指定编译时的区域设置。这样可以保证宽字符能够以正确的格式保存。
- `locale-string` 的默认值是 `""`。
- `"C"` 区域设置会将字符串中的每个字符作为 `wchar_t`（`unsigned short`）映射到 `"C"` 区域设置的值。
- 其它有效的参数是[语言字符串列表](https://docs.microsoft.com/en-us/cpp/c-runtime-library/locale-names-languages-and-country-region-strings)中的区域选项。

### 示例代码

```cpp
#pragma setlocale("dutch")   // 使用荷兰语区域设置

const wchar_t* str = L"voorbeeld";   // 宽字符串按荷兰语编码转换
```

> **注意**：能否处理指定的语言字符串取决于计算机是否支持对应的代码页（code page）和语言 ID。

---

## 38. `strict_gs_check`

### 语法

```cpp
#pragma strict_gs_check([push,] on)
#pragma strict_gs_check([push,] off)
#pragma strict_gs_check(pop)
```

### 作用

提供加强型的安全检测，在函数堆栈中插入随机 Cookie 以便于检测某些类别的基于堆栈的缓冲区溢出。

### 备注

- 默认情况下，`/GS`（缓冲区安全检查）编译选项不会为所有函数插入 Cookie 进行检测。
- 必须启动 `/GS` 编译选项后才能使用 `strict_gs_check` 编译指令。
- 该编译指令一般用于存在潜在危害的数据的代码模块。
- 该编译指令的攻击性较强，可以应用于可能不需要保护的函数，但是为了尽可能降低对应用程序的影响，它通常会进行优化。
- 即使使用了该编译指令，也要尽可能编写安全的代码，确保代码中不存在缓冲区溢出。

### 示例代码

```cpp
// pragma_strict_gs_check.cpp
// 编译选项: /GS /c

#pragma strict_gs_check(on)

void** ReverseArray(void** pData, size_t cData) {
    // 此缓冲区可能被溢出
    void* pReversed[20];

    // 反转数组到临时缓冲区
    for (size_t j = 0, i = cData; i; --i, ++j) {
        // 潜在缓冲区溢出风险（如果 cData > 20）
        pReversed[j] = pData[i];
    }

    // 拷贝回输入/输出缓冲区
    for (size_t i = 0; i < cData; ++i)
        pData[i] = pReversed[i];

    return pData;
}
```

由于启用了 `strict_gs_check`，编译器会在函数栈中插入 Cookie，当发生溢出时会在运行时检测到并终止程序。

---

## 39. `vtordisp`

### 语法

```cpp
#pragma vtordisp([push,] n)
#pragma vtordisp(pop)
#pragma vtordisp()
#pragma vtordisp([push,] {on | off})
```

### 作用

控制构造/析构偏移成员是否包含 `vtordisp` 隐藏字段。该字段用于解决虚拟继承中虚函数调用的正确性问题。

### 备注

- `push`：将当前的 `vtordisp` 设置值压入内部编译堆栈，并将新的 `vtordisp` 设置为 `n`。如果未指定 `n` 的值，则不改变 `vtordisp` 的设置。
- `pop`：将内部编译堆栈顶的元素弹出，并将其设置为新的 `vtordisp` 设置。
- `n`：指定 `vtordisp` 设置的值。可以是 `0`、`1` 或 `2`，分别对应于 `/vd0`、`/vd1` 和 `/vd2` 三个编译选项。更多信息请参阅 [`/vd`（禁用构造偏移）](https://docs.microsoft.com/en-us/cpp/build/reference/vd-disable-construction-displacements)。
- `on`：等价于 `#pragma vtordisp(1)`。
- `off`：等价于 `#pragma vtordisp(0)`。
- 该编译指令只能用于具有虚拟基类的代码。产生 `vtordisp` 的条件如下：
  - 虚继承中派生类重写了基类的虚函数。
  - 派生类在构造函数或析构函数中通过虚基类的指针调用了重写的基类函数。
- 该编译指令会影响它之后类的对象模型。功能等价于 `/vd0`、`/vd1` 和 `/vd2` 编译选项，但是作用范围不同。编译选项作用于整个模块，编译指令的作用范围根据设置的打开与关闭相关。
- 默认设置为 `on`（1），在必要时启用 `vtordisp` 字段。
- 设置为 `2` 时将对所有具有虚函数的虚基类启用 `vtordisp` 字段。
- `vtordisp(2)` 可以确保 `dynamic_cast` 在部分构造的对象上正常工作。更多信息请参阅[编译器警告（等级 1）C4436](https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4436)。

### 示例代码

```cpp
#pragma vtordisp(push, 2)   // 对所有虚基类启用 vtordisp
class VBase {
public:
    virtual void f() {}
};

class Derived : virtual public VBase {
public:
    void f() override {}
    Derived() {
        // 在构造函数中通过虚基类指针调用虚函数
        VBase* p = this;
        p->f();   // 需要 vtordisp 来正确调整 this 指针
    }
};
#pragma vtordisp(pop)
```

---

## 40. `warning`

### 语法

```cpp
#pragma warning( warning-specifier : warning-number-list [; warning-specifier : warning-number-list...] )
#pragma warning( push[ ,n ] )
#pragma warning( pop )
```

### 作用

选择性修改编译警告消息的行为（禁用、启用、提升为错误等）。

### 备注

- `warning-specifier`：警告描述符，同一编译指令中可以指定多个，有效参数如下：

| 警告描述符         | 注释                                                         |
| ------------------ | ------------------------------------------------------------ |
| `1`, `2`, `3`, `4` | 指定警告级别。同时会启用默认情况下处于关闭状态的指定警告。   |
| `default`          | 重置警告级别为默认值。同时会启用默认情况下处于关闭状态的指定警告。警告将在其默认存档级别生成。 |
| `disable`          | 禁用指定的警告。                                             |
| `error`            | 将指定警告报告为错误。                                       |
| `once`             | 只显示一次指定的警告消息。                                   |
| `suppress`         | 将当前警告设置压入堆栈，禁用下一行的指定警告，然后弹出警告堆栈，重置警告设置。 |

- `warning-number-list`：参数可包含多个警告编号，用空格或逗号分隔。
- 编译器会将 0-999 区间的告警编号默认加上 4000，变成 4000-4999。
- 在函数左大括号已经生效的警告状态，在函数体其余部分也生效。
- 4700-4900 区间内的警告与代码生成相关，使用 `warning` 编译指令修改大于 4699 的警告状态会在函数体末尾之后生效。
- 函数体中只有最后一个 `warning` 编译指令会在函数体末尾生效。
- `push`：保存每个警告的当前警告状态并压入编译栈，可选设置警告级别为 `n`。
- `pop`：弹出压入编译栈的最后一个警告状态，同时撤销 `push` 和 `pop` 之间的所有修改，还原每个警告的状态。
- 编写头文件时可以使用 `push` 和 `pop` 来确保外部对警告状态的修改不会影响到当前文件。

### 示例代码

```cpp
// 禁用警告 4507 和 4034，仅显示一次 4385，将 164 视为错误
#pragma warning( disable : 4507 34; once : 4385; error : 164 )

// 等价于：
#pragma warning( disable : 4507 34 )
#pragma warning( once : 4385 )
#pragma warning( error : 164 )
```

```cpp
// 函数体内警告状态示例
#pragma warning(disable:4700)   // 禁用“使用未初始化变量”警告
void Test() {
    int x;
    int y = x;   // 不会产生 C4700
    #pragma warning(default:4700)   // 恢复，但仅在函数结束后生效
}
int main() {
    int x;
    int y = x;   // 这里会产生 C4700
}
```

```cpp
// 使用 push/pop 保护头文件
// header.h
#pragma warning(push, 3)   // 保存当前状态，并将警告级别设为 3
// ... 头文件内容 ...
#pragma warning(pop)       // 恢复原始状态
```

```cpp
// 使用 suppress 抑制下一行警告
#pragma warning(suppress: 4700)
int x; int y = x;   // 这一行不会产生 C4700
int a; int b = a;   // 这一行仍会产生 C4700
```

---

根据实际开发经验，上述 40 个预处理命令中，**最常用**的主要集中在以下几类。我会按使用频率从高到低列出，并说明典型使用场景。

# 使用频率

## 1、高频使用（几乎每个项目都会用到）

| 指令              | 典型用途                                     | 示例                                          |
| ----------------- | -------------------------------------------- | --------------------------------------------- |
| `#pragma once`    | 防止头文件重复包含（替代 `#ifndef` 守卫）    | 放在每个头文件开头                            |
| `#pragma warning` | 控制编译器警告（禁用、提升为错误、临时压制） | 第三方库头文件前后用 `push/pop` 屏蔽警告      |
| `#pragma comment` | 自动链接库、指定链接器选项、嵌入版本信息     | `#pragma comment(lib, "Ws2_32.lib")`          |
| `#pragma pack`    | 控制结构体/类成员对齐（网络协议、文件解析）  | `#pragma pack(push, 1)` / `#pragma pack(pop)` |
| `#pragma message` | 编译时输出自定义信息（调试、版本提示）       | `#pragma message("Compiling " __FILE__)`      |

## 2、中频使用（特定场景下常用）

| 指令                                     | 典型用途                           | 示例                                        |
| ---------------------------------------- | ---------------------------------- | ------------------------------------------- |
| `#pragma region` / `#pragma endregion`   | 代码折叠（组织长文件）             | 将相关函数分组折叠                          |
| `#pragma deprecated`                     | 标记旧函数/宏为废弃，触发编译警告  | 库升级时提示用户迁移                        |
| `#pragma optimize`                       | 局部关闭优化（调试某个函数）       | `#pragma optimize("", off)`                 |
| `#pragma intrinsic` / `#pragma function` | 强制使用内建函数或禁止内联         | `#pragma intrinsic(memset)`                 |
| `#pragma detect_mismatch`                | 检测库版本不一致                   | 防止链接错误版本的静态库                    |
| `#pragma comment(linker, ...)`           | 传递链接器指令（如导出表、合并段） | `#pragma comment(linker, "/EXPORT:MyFunc")` |

## 3、低频使用（系统/驱动/特殊优化）

| 指令                                                         | 典型用途                                      |
| ------------------------------------------------------------ | --------------------------------------------- |
| `#pragma code_seg` / `#pragma data_seg` / `#pragma const_seg` / `#pragma bss_seg` | 将代码/数据放入自定义段（驱动、内存管理）     |
| `#pragma alloc_text`                                         | 指定函数所在代码段（驱动开发）                |
| `#pragma init_seg`                                           | 控制全局对象构造顺序（DLL/插件）              |
| `#pragma check_stack`                                        | 局部控制栈检查（嵌入式/驱动）                 |
| `#pragma runtime_checks`                                     | 局部控制 `/RTC` 运行时检查（调试）            |
| `#pragma float_control` / `#pragma fenv_access` / `#pragma fp_contract` | 精细控制浮点行为（数值计算）                  |
| `#pragma managed` / `#pragma unmanaged`                      | C++/CLI 混合编程                              |
| `#pragma make_public`                                        | 使本机类型对 .NET 程序集可见（C++/CLI）       |
| `#pragma vtordisp`                                           | 解决虚继承中构造/析构时的指针偏移问题（罕见） |
| `#pragma hdrstop`                                            | 控制预编译头位置（大型项目）                  |
| `#pragma include_alias`                                      | 映射长文件名头文件（老旧文件系统）            |
| `#pragma loop`                                               | 提示循环并行化/向量化（高性能计算）           |
| `#pragma omp`                                                | OpenMP 并行编程                               |
| `#pragma section`                                            | 创建自定义段（驱动/底层）                     |
| `#pragma setlocale`                                          | 指定宽字符转换区域（国际化）                  |
| `#pragma strict_gs_check`                                    | 强制堆栈 Cookie 保护（高安全模块）            |
| `#pragma pointers_to_members`                                | 控制成员指针表示（兼容旧代码）                |
| `#pragma conform`                                            | 强制标准 `for` 循环作用域（兼容性）           |
| `#pragma component`                                          | 控制浏览信息收集（已过时）                    |
| `#pragma auto_inline`                                        | 控制自动内联（老式优化）                      |
| `#pragma inline_depth` / `#pragma inline_recursion`          | 精细内联控制（极少手动调整）                  |
| `#pragma execution_character_set`                            | 已废弃，被 `/utf-8` 替代                      |
| `#pragma push_macro` / `#pragma pop_macro`                   | 临时保存/恢复宏定义（极少用）                 |

# 注意事项（上述预处理命令大多数都是MSVC特有的）

## 1、表格一览

上面那 40 条指令里，大部分是微软 MSVC 的特色，真正三大编译器都支持的，其实只有一小部分。

我把它们分成了**三大编译器都支持**、**GCC/Clang 支持**以及 **MSVC 独有**三类，整理成了表格，你可以参考一下：

| 类别                             | 指令 (Pragma)             | GCC  | Clang | MSVC | 备注与说明                                                   |
| :------------------------------- | :------------------------ | :--: | :---: | :--: | :----------------------------------------------------------- |
| **三大编译器广泛支持**           | `once`                    |  ✅   |   ✅   |  ✅   | 所有主流编译器都支持。                                       |
|                                  | `pack`                    |  ✅   |   ✅   |  ✅   | 所有主流编译器都支持。                                       |
|                                  | `message`                 |  ✅   |   ✅   |  ✅   | 三大编译器均支持。                                           |
|                                  | `comment`                 |  ✅   |   ✅   |  ✅   | GCC 和 Clang 的 Windows 版本支持此指令。                     |
|                                  | `push_macro`/`pop_macro`  |  ✅   |   ✅   |  ✅   | GCC 和 Clang 为兼容 MSVC 而实现。                            |
|                                  | `warning`                 |  ⚠️   |   ✅   |  ✅   | MSVC 和 Clang 支持。GCC 通过 `#pragma GCC diagnostic` 实现类似功能。 |
|                                  | `region`/`endregion`      |  ✅   |   ✅   |  ✅   | Clang 和 GCC 在编译时**忽略**此指令。                        |
|                                  | `optimize`                |  ✅   |   ✅   |  ✅   | 三大编译器都支持 (<br />GCC使用<br />`#pragma GCC optimize`(“O2”), <br />优化代码片段<br />#pragma GCC reset_options<br /><br />Clang使用<br />#pragma clang optimize on)<br />优化代码片段<br />#pragma clang optimize off) |
|                                  | `loop`                    |  ✅   |   ✅   |  ✅   | 为循环优化提供提示。<br />GCC使用<br />#pragma GCC unroll N<br />#pragma GCC novector<br />#pragma GCC ivdep<br /><br />Clang使用<br />\#pragma clang loop vectorize(enable) interleave(enable)<br />\#pragma clang loop vectorize(disable) interleave(disable)<br />\#pragma clang loop vectorize_width(4) interleave_count(2)<br />\#pragma clang loop unroll_count(4) |
|                                  | `omp`                     |  ✅   |   ✅   |  ✅   | OpenMP 标准，需要编译选项开启。                              |
| **GCC/Clang 支持 (非MSVC)**      | `GCC`                     |  ✅   |   ✅   |  ❌   | GCC/Clang 特有的指令。                                       |
|                                  | `STDC FENV_ACCESS`        |  ✅   |   ✅   |  ❌   | C99/C11 标准中定义的浮点环境访问控制，GCC/Clang 支持，MSVC 不支持。 |
| **MSVC 独有 (GCC/Clang 不支持)** | `alloc_text`              |  ❌   |   ❌   |  ✅   | MSVC 特有，用于指定函数代码段。                              |
|                                  | `auto_inline`             |  ❌   |   ❌   |  ✅   | MSVC 特有，控制自动内联扩展。                                |
|                                  | `bss_seg`                 |  ❌   |   ❌   |  ✅   | MSVC 特有，指定未初始化变量的存储段。                        |
|                                  | `check_stack`             |  ❌   |   ❌   |  ✅   | MSVC 特有，控制栈探测。                                      |
|                                  | `code_seg`                |  ❌   |   ❌   |  ✅   | MSVC 特有，指定代码段。                                      |
|                                  | `component`               |  ❌   |   ❌   |  ✅   | MSVC 特有，控制浏览信息收集。                                |
|                                  | `conform`                 |  ❌   |   ❌   |  ✅   | MSVC 特有，用于 `for` 循环作用域控制。                       |
|                                  | `const_seg`               |  ❌   |   ❌   |  ✅   | MSVC 特有，指定常量数据的存储段。                            |
|                                  | `data_seg`                |  ❌   |   ❌   |  ✅   | MSVC 特有，指定已初始化数据的存储段。                        |
|                                  | `deprecated`              |  ❌   |   ❌   |  ✅   | MSVC 特有，标记符号为已弃用。                                |
|                                  | `detect_mismatch`         |  ❌   |   ❌   |  ✅   | MSVC 特有，在对象中插入记录供链接器检查不匹配。              |
|                                  | `execution_character_set` |  ❌   |   ❌   |  ✅   | MSVC 特有，用于指定执行字符集。                              |
|                                  | `fenv_access`             |  ❌   |   ❌   |  ✅   | MSVC 特有，控制对浮点环境标志的访问。                        |
|                                  | `float_control`           |  ❌   |   ❌   |  ✅   | MSVC 特有，控制浮点行为。                                    |
|                                  | `fp_contract`             |  ❌   |   ❌   |  ✅   | MSVC 特有，控制浮点表达式收缩。                              |
|                                  | `function`                |  ❌   |   ❌   |  ✅   | MSVC 特有，强制使用函数调用而非内建函数。                    |
|                                  | `hdrstop`                 |  ❌   |   ❌   |  ✅   | MSVC 特有，控制预编译头文件位置。                            |
|                                  | `include_alias`           |  ❌   |   ❌   |  ✅   | MSVC 特有，映射长文件名头文件。                              |
|                                  | `init_seg`                |  ❌   |   ❌   |  ✅   | MSVC 特有，控制全局对象构造顺序。                            |
|                                  | `inline_depth`            |  ❌   |   ❌   |  ✅   | MSVC 特有，控制内联展开的深度。                              |
|                                  | `inline_recursion`        |  ❌   |   ❌   |  ✅   | MSVC 特有，控制递归函数的内联。                              |
|                                  | `intrinsic`               |  ❌   |   ❌   |  ✅   | MSVC 特有，指定函数为内建函数。                              |
|                                  | `make_public`             |  ❌   |   ❌   |  ✅   | MSVC 特有，用于 C++/CLI 编程。                               |
|                                  | `managed`/`unmanaged`     |  ❌   |   ❌   |  ✅   | MSVC 特有，用于 C++/CLI 混合编程。                           |
|                                  | `pointers_to_members`     |  ❌   |   ❌   |  ✅   | MSVC 特有，控制指向类成员指针的表示。                        |
|                                  | `runtime_checks`          |  ❌   |   ❌   |  ✅   | MSVC 特有，控制运行时错误检查。                              |
|                                  | `section`                 |  ❌   |   ❌   |  ✅   | MSVC 特有，在 OBJ 文件中创建新段。                           |
|                                  | `setlocale`               |  ❌   |   ❌   |  ✅   | MSVC 特有，指定编译时区域设置。                              |
|                                  | `strict_gs_check`         |  ❌   |   ❌   |  ✅   | MSVC 特有，强制执行 GS 缓冲区安全检查。                      |
|                                  | `vtordisp`                |  ❌   |   ❌   |  ✅   | MSVC 特有，控制隐藏的 vtordisp 字段的生成。                  |
|                                  |                           |      |       |      |                                                              |

> **请注意**：`#pragma warning` 在三大编译器中的支持情况比较特殊。MSVC 和 Clang 都支持它，但 **GCC 原生不支持**，它提供的是另一套诊断控制系统 `#pragma GCC diagnostic`。另外，需要OpenMP支持时，可以用 `#pragma omp`，不过记得在编译时开启 `/openmp` (MSVC) 或 `-fopenmp` (GCC/Clang) 选项。

## 2、如何写出可移植的代码？

建议在代码里用条件编译，根据编译器自动选择正确的指令，比如：

```cpp
#if defined(_MSC_VER)
    #pragma warning(disable: 4996)      // 针对 MSVC
#elif defined(__clang__)
    #pragma clang diagnostic ignored "-Wdeprecated-declarations" // 针对 Clang
#elif defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"   // 针对 GCC
#endif
```
