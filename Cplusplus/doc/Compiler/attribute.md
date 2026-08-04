##  `[[assume( 表达式 )]]`

https://cppreference.cn/w/cpp/language/attributes/assume

指定假定给定的表达式在给定点始终评估为 true，以允许编译器根据给定的信息进行优化。

# MSVC

## `[[msvc::forceinline_calls]]`

Microsoft 特定属性  可以放置在语句或块之上或之前。 它会导致内联启发式尝试  该语句或块中的所有调用：

```cpp
void f() {
    [[msvc::forceinline_calls]]
    {
        foo();
        bar();
    }
    ...
    [[msvc::forceinline_calls]]
    bar();
    
    foo();
}
```

对  `foo` 的第一次调用以及对`bar`的两次调用都被视为声明为 。 对`foo`的第二次调用不会被视为 。

## `[[msvc::intrinsic]]`

 属性对其应用到的函数有三个约束：

- 该函数不能具有递归性；它的主体必须只有一个 return 语句，该语句带有从参数类型到返回类型的 。
- 该函数只能接受单个参数。
-  编译器选项是必需的。 （默认情况下， 及更高版本的选项意味着 。）

Microsoft 特定的  属性告知编译器内联一个元函数，该元函数充当从参数类型到返回类型的命名转换。  当该属性出现在函数定义中时，编译器会用简单的强制转换替换对该函数的所有调用。 Visual Studio 2022 版本 17.5 预览版 2  及更高版本中提供了 `[[msvc::intrinsic]]` 属性。 此属性仅适用于其后面的特定函数。

* 示例

在此示例代码中，应用于  函数的  属性使编译器将该函数的调用替换为其主体中的内联静态强制转换：		 

```cpp
template <typename T>
[[msvc::intrinsic]] T&& my_move(T&& t) { return static_cast<T&&>(t); }

void f() {
    int i = 0;
    i = my_move(i);
}
```

## `[[msvc::musttail]]`

该  属性在 MSVC 生成工具版本 14.50 中引入，是强制实施尾调用优化的试验性 x64 仅Microsoft特定属性。  应用于限定返回语句时，它会指示编译器发出调用作为结尾调用。 如果编译器无法发出结尾调用，则会生成编译错误。 该   属性强制实施结尾调用，而不是将函数内联。

 要求：

- 调用方和被调用方必须具有匹配的返回类型。
- 调用约定必须兼容。
- 尾部调用必须是调用函数中的最终作。
- 被调用方不能使用比调用函数更多的堆栈空间。
- 如果传递了四个以上的整数参数，则调用函数必须为其他参数分配足够的堆栈空间。
- 使用  或  优化级别进行编译。



* 示例

Tail 调用是编译器优化，当函数调用是在返回之前执行的最后一个作时，该优化是可能的。 重用当前函数的堆栈帧，而不是创建新的堆栈帧来调用函数。 这可减少堆栈使用率并提高性能，尤其是在递归方案中。

在以下代码中，  应用于  使控制直接 传输到的控制的属性。 到达语句时，其结果直接提供给调用方，即。 这将替换内联传入或调用和返回之前返回到  。 尾部调用优化无需  在完成后重新获得控制  。 这是一种性能优化，可减少堆栈使用率，在递归方案中特别有用。	 			 		 		 	  

```cpp
// compile with /O2
#include <iostream>

int increment(int x)
{
    return x + 1;
}

int incrementIfPositive(int x)
{
    if (x > 0)
    {
        [[msvc::musttail]]
        return increment(x);
	}
    return -1;
}

int main()
{
    int result = incrementIfPositive(42);
    if (result < 0)
    {
        return -1;
    }

    std::cout << result; // outputs 43
    return 0;
}
```

## `[[msvc::noinline]]`

当放置在函数声明之前时，Microsoft 特定属性  与  具有相同的含义。

## `[[msvc::noinline_calls]]`

Microsoft 特定属性  的用法与  相同。 它可以放置在任何语句或块之前。 它不是强制内联该块中的所有调用，而是对应用到的范围禁用内联。

# GCC

