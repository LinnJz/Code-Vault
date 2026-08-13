# CMake与Ninja

1. **CMake（总司令）**：读取 `CMakeLists.txt`，分析项目结构、依赖和系统环境，生成 Ninja 需要的 `build.ninja` 文件。
2. **Ninja（执行尖兵）**：读取 `build.ninja`，根据文件时间戳快速确定哪些部分需要重新编译，然后调度编译器高效按序或并行执行

# `INTERFACE`、`PRIVATE`、`PUBLIC`

## 1. `INTERFACE` 的定位：纯粹为别人服务

`INTERFACE` 的意思是：**这个依赖只为了传递给我的下游使用者，我自己并不需要它。**

典型例子：**一个只有头文件的库（Header-Only Library）**。

```cmake
# 比如一个纯头文件的库 my_math
add_library(my_math INTERFACE)
target_include_directories(my_math INTERFACE include/)  # 下游需要这个头文件目录
target_link_libraries(my_math INTERFACE pthread)        # 下游需要链接pthread，但my_math自身无源码
```

对于 `my_math` 这个目标本身，它没有 `.cpp` 文件需要编译，所以它根本不需要 `pthread` 库，也无需包含路径。但是，任何链接 `my_math` 的可执行文件或库（比如 `app`），都需要 `pthread` 和 `include/` 路径。
 这就是典型的 `INTERFACE` 用法：**我只传递，自己不消耗**。

## 2. `PRIVATE` 的场景：仅我自己要



## 3. `PUBLIC` 的场景：我自己要，下游也要

当一个库既有自己的实现代码（`.cpp`），又希望下游继承相同的依赖时，就必须用 `PUBLIC`（`PRIVATE`与`INTERFACE`的结合）。

举个例子：`my_lib` 是一个普通库，它内部使用了 `Boost::filesystem`（需要链接），并且它的头文件也暴露了 `Boost::filesystem` 的类型（比如某个 public 函数的参数是 `boost::filesystem::path`）。

```cmake
add_library(my_lib STATIC)
target_sources(my_lib PRIVATE src/my_lib.cpp)
target_include_directories(my_lib PUBLIC include/)   # 下游需要这个头文件目录
target_link_libraries(my_lib PUBLIC Boost::filesystem)   # ⬅️ 必须 PUBLIC
```

- **如果这里是 `INTERFACE`**：
  那么 `my_lib` 自己编译链接时，不会链接 `Boost::filesystem`，导致 `src/my_lib.cpp` 中调用 `boost::filesystem` 符号时产生“未定义引用”错误（因为自身没有获得链接指令）。
- **如果这里是 `PRIVATE`**：
  那么 `my_lib` 自己能正常工作，但下游程序（比如 `app` 链接 `my_lib`）在编译自己的代码时，如果包含了 `my_lib` 的头文件（里面用了 `boost::filesystem::path`），就会因为找不到 `Boost::filesystem` 的包含路径或库而失败。
- **必须用 `PUBLIC`**：
  自身编译时得到链接，下游也能继承链接和包含路径。

# CMake常用命令

CMake中所谓的“函数”，通常指其内置的**命令（Command）**。此外，用户也可以使用 `function()` 命令**自定义函数**来实现代码复用。下面按照功能分类，整理了一些最常用的命令。

## 🧱 核心基础命令

这些是构建任何CMake项目都必备的命令。

*   **`cmake_minimum_required(VERSION <min>)`**：指定项目所需的最低CMake版本。这是CMakeLists.txt文件的第一行，用于确保构建过程的兼容性。
*   **`project(<PROJECT_NAME> [VERSION] [LANGUAGES])`**：定义项目名称、版本号和使用的编程语言。例如：`project(MyApp VERSION 1.0 LANGUAGES CXX)`。
*   **`set(<variable> <value>...)`**：设置或创建变量。例如：`set(SOURCES main.cpp utils.cpp)`。
*   **`message([STATUS|WARNING|...] "message")`**：向终端输出信息，常用于调试。`STATUS`输出普通信息，`FATAL_ERROR`会立即终止CMake进程。

## 📁 文件与路径操作

用于管理源文件、头文件路径和目录结构。

*   **`add_executable(<name> [sources...])`**：使用指定的源文件生成可执行文件。
*   **`add_library(<name> [STATIC|SHARED] [sources...])`**：生成静态库(`STATIC`)或动态库(`SHARED`)。
*   **`include_directories(<dirs>...)`**：添加头文件的搜索路径。
*   **`target_include_directories(<target> <INTERFACE|PUBLIC|PRIVATE> <dirs>...)`**：为目标（`add_executable`或`add_library`创建）指定头文件搜索路径，更具现代性和精确性。
*   **`aux_source_directory(<dir> <variable>)`**：将指定目录下的所有源文件（如`.cpp`, `.c`）的列表存入一个变量。
*   **`file(GLOB <variable> <pattern>)`**：通过通配符模式（如`*.cpp`）查找文件并存入变量。**注意**：官方不推荐使用`GLOB`来收集源文件，因为它不会在添加新文件时自动更新。
*   **`add_subdirectory(<dir>)`**：添加一个子目录，并构建其中的`CMakeLists.txt`。
*   **`configure_file(<input> <output>)`**：将输入文件（如`config.h.in`）中的变量替换后，生成输出文件。

## 🎯 目标与属性管理

用于精细控制构建目标（可执行文件或库）的属性。

*   **`target_link_libraries(<target> <libs>...)`**：为指定的目标链接库文件。它应在`add_executable`或`add_library`之后调用。
*   **`link_directories(<dirs>...)`**：添加库文件的搜索路径。现代CMake更推荐使用`find_library`或`find_package`获取库的完整路径。
*   **`set_target_properties(<target>... PROPERTIES <prop1> <value1>...)`**：为一个或多个目标设置属性。例如，可设置动态库的版本号或输出路径。
*   **`install(TARGETS <target>... DESTINATION <dir>)`**：定义构建完成后，如何将目标（可执行文件、库等）安装到指定目录。

## 🔀 流程控制与逻辑

实现条件判断和循环。

*   **条件判断 (`if`, `elseif`, `else`, `endif`)**：根据条件执行不同分支。支持数值、字符串、逻辑运算符、存在性检查等丰富表达式。
    *   `if(DEFINED <var>)`：检查变量是否被定义。
    *   `if(TARGET <name>)`：检查某个构建目标是否已存在。
    *   `if(<var> IN_LIST <list>)`：检查某个值是否在列表中。
*   **循环 (`foreach`, `while`)**：遍历列表或执行重复操作。

## 🔍 查找与依赖管理

用于查找外部库和包。

*   **`find_package(<PackageName> [REQUIRED])`**：查找并加载外部软件包的设置。`REQUIRED`表示该包是必须的，如果找不到则报错。
*   **`find_library(<var> <lib_name> [HINTS <paths>...])`**：查找一个特定的库文件，并将找到的完整路径存入变量。
*   **`option(<option_var> "<help_text>" [initial_value])`**：定义一个用户可配置的布尔型选项。用户可通过`-D<option_var>=ON/OFF`在命令行修改。

## 🧩 自定义函数与宏

用于封装重复逻辑，提高代码复用性。

*   **`function(<name> [arg1 ...]) ... endfunction()`**：定义一个函数。函数有**独立的作用域**，内部定义的变量默认是局部的，不会影响外部。
*   **`macro(<name> [arg1 ...]) ... endmacro()`**：定义一个宏。宏**不创建新的作用域**，它更像是简单的文本替换。

## 🔧 其他实用命令

*   **`add_custom_command` / `add_custom_target`**：添加自定义的构建规则或目标，用于在构建过程中执行外部命令。
*   **`add_test`**：为`ctest`测试工具添加一个测试用例。
*   **`list(APPEND|REMOVE|... )`**：对列表变量进行操作。
*   **`cmake_policy`**：管理CMake的策略，以处理新旧版本之间的行为变化。

💡 **关于“命令”与“函数”**
在CMake的语境中，`set`, `message`, `add_executable`等被称为**命令（Command）**。而**函数（Function）** 通常指用户通过`function()`自定义的、可复用的代码块。不过，在日常交流中，大家也常常会把“命令”统称为“函数”。

另外想提醒一下，CMake发展很快，现代CMake（3.x以上版本）推荐使用更加精确和模块化的`target_*`系列命令（如`target_include_directories`），而不是全局的`include_directories`，以更好地控制依赖的传递性。

如果想了解某个具体命令的详细用法，随时可以再问我。

## 配套分发

```
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
```

* `GNUInstallDirs`：引入标准安装目录变量，符合GNU惯例，避免硬编码，提升可移植性
* `CMakePackageConfigHelpers`：生成包配置文件，支持外部项目find_package(项目)引用

## 构建设置

```cmake
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "" FORCE)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE) # 默认debug
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel") #为“CMake图形界面工具（cmake-gui / ccmake）”和“支持CMake集成的现代IDE设置面板”设计的，当你使用 cmake-gui 或 ccmake 时，输入框会变成下拉菜单，限定你只能选这4个值，防止手误输错
endif()
```

