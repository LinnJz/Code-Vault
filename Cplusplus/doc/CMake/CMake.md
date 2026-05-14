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