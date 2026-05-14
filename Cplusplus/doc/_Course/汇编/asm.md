# C++编写汇编方式

在C++中编写汇编代码主要有两种方式：**内联汇编**和**分离汇编文件**。以下是详细介绍：

## 1. 内联汇编 (Inline Assembly)

### GCC/Clang语法 (AT&T语法)
```cpp
#include <iostream>

int main() {
    int a = 10, b = 20, result;
    
    // 基本内联汇编
    asm volatile(
        "addl %1, %2;"    // 汇编指令
        "movl %2, %0;"    // 将结果移动到result
        : "=r"(result)     // 输出操作数
        : "r"(a), "r"(b)   // 输入操作数
    );
    
    std::cout << "Result: " << result << std::endl;
    return 0;
}
```

### MSVC语法 (Intel语法)
```cpp
#include <iostream>

int main() {
    int a = 10, b = 20, result;
    
    __asm {
        mov eax, a    ; Intel语法
        add eax, b
        mov result, eax
    }
    
    std::cout << "Result: " << result << std::endl;
    return 0;
}
```

## 2. 扩展内联汇编示例

```cpp
#include <iostream>

int main() {
    // 使用内联汇编实现乘法
    int a = 5, b = 6, product;
    
    asm volatile (
        "imull %%ebx, %%eax;"  // eax = eax * ebx
        : "=a"(product)        // 输出到product（通过eax）
        : "a"(a), "b"(b)       // a输入到eax，b输入到ebx
        :                      // 没有破坏的寄存器
    );
    
    std::cout << "5 * 6 = " << product << std::endl;
    
    // 读取时间戳计数器
    unsigned long long tsc;
    asm volatile ("rdtsc" : "=A"(tsc));
    std::cout << "Timestamp: " << tsc << std::endl;
    
    return 0;
}
```

## 3. 分离汇编文件

### C++主文件 (main.cpp)
```cpp
#include <iostream>

// 声明外部汇编函数
extern "C" {
    int add_numbers(int a, int b);
    void print_message(const char* msg);
}

int main() {
    int sum = add_numbers(10, 20);
    std::cout << "Sum: " << sum << std::endl;
    
    print_message("Hello from C++!");
    return 0;
}
```

### 汇编文件 (asm_functions.asm for NASM)
```assembly
section .data
    msg db "Hello from Assembly!", 0Ah, 0

section .text
global add_numbers
global print_message

add_numbers:
    mov eax, edi    ; 第一个参数（System V调用约定）
    add eax, esi    ; 加上第二个参数
    ret

print_message:
    ; 系统调用：write(1, msg, length)
    mov rax, 1      ; sys_write
    mov rdi, 1      ; stdout
    mov rsi, msg
    mov rdx, 20     ; 消息长度
    syscall
    ret
```

## 4. 实用技巧和注意事项

### 操作数约束
```cpp
asm volatile (
    "指令 %[输出名], %[输入名]"
    : [输出名] "=r"(c_var)      // 输出
    : [输入名] "r"(a_var)       // 输入
    : "cc", "memory"           // 破坏列表
);
```

### 常用约束符
- `"r"`：通用寄存器
- `"m"`：内存操作数
- `"i"`：立即数
- `"g"`：寄存器、内存或立即数

### 实际应用示例
```cpp
// 原子操作
void atomic_increment(int* value) {
    asm volatile(
        "lock incl %0"
        : "+m"(*value)
        :
        : "cc"
    );
}

// 获取CPU信息
void get_cpuid() {
    int info[4];
    asm volatile(
        "cpuid"
        : "=a"(info[0]), "=b"(info[1]), "=c"(info[2]), "=d"(info[3])
        : "a"(0)
    );
}

// 内存屏障
void memory_barrier() {
    asm volatile("mfence" ::: "memory");
}
```

## 5. 编译和链接

### 使用GCC编译
```bash
# 内联汇编直接编译
g++ -o program main.cpp

# 分离汇编文件（使用NASM和GCC）
nasm -f elf64 asm_functions.asm -o asm_functions.o
g++ -o program main.cpp asm_functions.o
```

### 使用MSVC编译
```bash
# 内联汇编直接编译（x86 only）
cl /EHsc main.cpp

# x64不支持内联汇编，需要使用MASM
ml64 /c asm_functions.asm
cl /EHsc main.cpp asm_functions.obj
```

## 6. 最佳实践

1. **谨慎使用**：现代编译器优化通常比手写汇编更好
2. **性能分析**：先用profiler找到真正的热点
3. **平台兼容**：注意x86/x64、Windows/Linux的差异
4. **可读性**：添加详细注释
5. **测试**：充分测试，特别是边界情况

## 适用场景
- 性能关键的热点代码
- 访问特殊硬件寄存器
- 实现原子操作
- 系统调用和中断处理
- 加密算法优化

记住：**优先使用C++特性，只在必要时才使用汇编**。现代编译器优化通常能生成高效的机器代码，手写汇编容易出错且难以维护。

# 汇编语言指令和寄存器（X86_64为例）详解

## 一、x86/x64寄存器

### 1. 通用寄存器（32位/64位）

```
32位       64位          主要用途
------------------------------------------------------------------------
EAX        RAX           累加器，函数返回值，算术运算
EBX        RBX           基址寄存器，数据指针
ECX        RCX           计数寄存器，循环计数，字符串操作
EDX        RDX           数据寄存器，I/O指针，乘除运算
ESI        RSI           源索引寄存器，字符串/数组源指针
EDI        RDI           目的索引寄存器，字符串/数组目的指针
EBP        RBP           基址指针，函数栈帧基址
ESP        RSP           栈指针，指向当前栈顶
------------------------------------------------------------------------
R8D-R15D   R8-R15        扩展通用寄存器（64位新增）
```

### 2. 段寄存器（16位）

| 寄存器 | 用途                          |
| ------ | ----------------------------- |
| CS     | 代码段                        |
| DS     | 数据段                        |
| SS     | 栈段                          |
| ES     | 附加数据段                    |
| FS     | 附加数据段（Windows线程信息） |
| GS     | 附加数据段（Linux内核数据）   |

### 3. 标志寄存器

```
32位：EFLAGS，64位：RFLAGS

位  标志     名称       含义
-------------------------------------------------------------
0    CF      进位标志   算术操作产生进位/借位
2    PF      奇偶标志   结果低8位中1的个数为偶数
4    AF      调整标志   BCD码运算调整
6    ZF      零标志    结果为零
7    SF      符号标志   结果为负
8    TF      陷阱标志   单步执行
9    IF      中断标志   允许中断
10   DF      方向标志   字符串操作方向（0=递增，1=递减）
11   OF      溢出标志   有符号溢出
```

### 4. 控制寄存器（x64）

```
CR0：系统控制（保护模式、分页等）
CR2：页故障线性地址
CR3：页目录基址
CR4：扩展功能（SSE、VMX等）
```

### 5. 指令指针寄存器

- **RIP** (64位) / **EIP** (32位) / **IP** (16位)
  指向下一条要执行的指令。

### 6. 浮点寄存器

- **XMM0 - XMM15** (128位，用于SSE指令)
- **YMM0 - YMM15** (256位，用于AVX指令)
- **ZMM0 - ZMM31** (512位，用于AVX-512指令)

## 二、常用汇编指令

### 1. 数据传输指令

**MOV**：将数据从源操作数复制到目的操作数
例如：`mov rax, rbx` 将RBX的值复制到RAX

**PUSH**：将操作数压入栈
例如：`push rax`

**POP**：从栈中弹出数据到操作数
例如：`pop rax`

**LEA**：加载有效地址（计算地址但不访问内存）
例如：`lea rax, [rbx+rcx*4]`

```assembly
; 基本数据传输
MOV  dest, src      ; 移动数据
MOVZX dest, src     ; 零扩展移动
MOVSX dest, src     ; 符号扩展移动

; 栈操作
PUSH src            ; 压栈
POP  dest           ; 出栈
PUSHF/POPF          ; 标志寄存器压栈/出栈

; 数据交换
XCHG op1, op2       ; 交换数据
BSWAP reg           ; 字节序反转（32/64位）
```

### 2. 算术运算指令

**ADD**：加法
例如：`add rax, rbx`  (RAX = RAX + RBX)

**SUB**：减法
例如：`sub rax, rbx`  (RAX = RAX - RBX)

**INC**：加1
例如：`inc rax`

**DEC**：减1

**MUL**：无符号乘法（隐含使用RAX）
例如：`mul rbx`  (RDX:RAX = RAX * RBX)

**IMUL**：有符号乘法

**DIV**：无符号除法（隐含使用RDX:RAX作为被除数）
例如：`div rbx`  (RAX = 商, RDX = 余数)

**IDIV**：有符号除法

```assembly
; 加减法
ADD  dest, src      ; 加法
ADC  dest, src      ; 带进位加法
SUB  dest, src      ; 减法
SBB  dest, src      ; 带借位减法
INC  dest           ; 加1
DEC  dest           ; 减1
NEG  dest           ; 取负

; 乘除法
MUL  src           ; 无符号乘法（AL/AX/EAX/RAX * src）
IMUL src           ; 有符号乘法
IMUL dest, src     ; dest = dest * src
IMUL dest, src, imm ; dest = src * 立即数

DIV  src           ; 无符号除法
IDIV src           ; 有符号除法

; 比较指令
CMP  op1, op2      ; op1 - op2，设置标志位
TEST op1, op2      ; op1 & op2，设置标志位
```

### 3. 逻辑运算指令

**AND**：按位与

**OR**：按位或

**XOR**：按位异或

**NOT**：按位取反

**SHL**：左移

**SHR**：逻辑右移

**SAR**：算术右移

```assembly
AND  dest, src      ; 按位与
OR   dest, src      ; 按位或
XOR  dest, src      ; 按位异或
NOT  dest           ; 按位取反

; 移位指令
SHL  dest, count    ; 逻辑左移
SHR  dest, count    ; 逻辑右移
SAL  dest, count    ; 算术左移（同SHL）
SAR  dest, count    ; 算术右移（符号扩展）

ROL  dest, count    ; 循环左移
ROR  dest, count    ; 循环右移
RCL  dest, count    ; 带进位循环左移
RCR  dest, count    ; 带进位循环右移
```

### 4. 控制转移指令

**JMP**：无条件跳转
例如：`jmp label`

**JE/JZ**：等于/为零时跳转

**JNE/JNZ**：不等于/不为零时跳转

**JG**：有符号大于时跳转

**JL**：有符号小于时跳转

**JA**：无符号高于时跳转

**JB**：无符号低于时跳转

**CALL**：调用子程序

**RET**：从子程序返回

**CMP**：比较两个操作数（相当于SUB但不保存结果，只设置标志位）
例如：`cmp rax, rbx`

**TEST**：测试操作数（相当于AND但不保存结果，只设置标志位）
例如：`test rax, rax`  (常用于测试RAX是否为0)

```assembly
; 无条件跳转
JMP  label          ; 跳转到标签
JMP  reg/mem        ; 跳转到寄存器/内存地址

; 条件跳转（基于标志位）
JE/JZ  label        ; 等于/为零时跳转   (ZF=1)
JNE/JNZ label       ; 不等于/非零跳转   (ZF=0)
JG/JNLE label       ; 有符号大于跳转    (ZF=0且SF=OF)
JGE/JNL label       ; 有符号大于等于跳转 (SF=OF)
JL/JNGE label       ; 有符号小于跳转    (SF≠OF)
JLE/JNG label       ; 有符号小于等于跳转 (ZF=1或SF≠OF)

JA/JNBE label       ; 无符号高于跳转    (CF=0且ZF=0)
JAE/JNB label       ; 无符号高于等于跳转 (CF=0)
JB/JNAE label       ; 无符号低于跳转    (CF=1)
JBE/JNA label       ; 无符号低于等于跳转 (CF=1或ZF=1)

JC   label          ; 进位跳转         (CF=1)
JNC  label          ; 无进位跳转       (CF=0)
JO   label          ; 溢出跳转         (OF=1)
JNO  label          ; 无溢出跳转       (OF=0)
JS   label          ; 符号跳转         (SF=1)
JNS  label          ; 无符号跳转       (SF=0)

; 循环控制
LOOP label          ; ECX/RCX减1，非零则循环
LOOPZ/LOOPE label   ; ECX减1，非零且ZF=1则循环
LOOPNZ/LOOPNE label ; ECX减1，非零且ZF=0则循环

; 函数调用
CALL label          ; 调用函数
RET                 ; 从函数返回
RET  imm            ; 返回并清理栈空间
```

### 5. 字符串操作指令

**MOVS**：移动字符串（从源地址到目的地址）

**CMPS**：比较字符串

**SCAS**：扫描字符串（与AL/RAX比较）

**STOS**：存储字符串（将AL/RAX的值存入目的地址）

这些指令通常与重复前缀（如REP）一起使用。

```assembly
; 配合REP前缀使用
MOVSB/MOVSW/MOVSD/MOVSQ ; 移动字节/字/双字/四字
CMPSB/CMPSW/CMPSD/CMPSQ ; 比较字符串
SCASB/SCASW/SCASD/SCASQ ; 扫描字符串（与AL/AX/EAX/RAX比较）
STOSB/STOSW/STOSD/STOSQ ; 存储字符串（AL/AX/EAX/RAX到[EDI]）
LODSB/LODSW/LODSD/LODSQ ; 加载字符串（[ESI]到AL/AX/EAX/RAX）

; 重复前缀
REP                  ; 重复直到ECX/RCX=0
REPE/REPZ            ; 相等/为零时重复
REPNE/REPNZ          ; 不相等/非零时重复
```

### 6. 系统指令

**SYSCALL**：调用系统调用（Linux）

**SYSENTER**：快速系统调用（Linux）

**INT**：中断（例如`int 0x80`是Linux传统的系统调用方式）

```assembly
; 特权指令
IN   dest, port      ; 从端口输入
OUT  port, src       ; 输出到端口
HLT                  ; 停机
CLI                  ; 清除中断标志（禁止中断）
STI                  ; 设置中断标志（允许中断）

; 系统调用（x64 Linux）
SYSCALL              ; 快速系统调用
SYSENTER             ; 系统进入
SYSEXIT              ; 系统退出

; 中断
INT  n               ; 软中断（n=中断号）
IRET/IRETD/IRETQ     ; 中断返回
```

### 7. SIMD指令（SSE/AVX）

```assembly
; 数据移动
MOVAPS/MOVAPD        ; 移动对齐的打包单/双精度浮点数
MOVUPS/MOVUPD        ; 移动未对齐的打包单/双精度浮点数

; 算术运算
ADDPS/ADDPD          ; 打包单/双精度浮点数加法
SUBPS/SUBPD          ; 减法
MULPS/MULPD          ; 乘法
DIVPS/DIVPD          ; 除法
SQRTPS/SQRTPD        ; 平方根

; 比较
CMPPS/CMPPD          ; 打包浮点数比较
MINPS/MINPD          ; 最小值
MAXPS/MAXPD          ; 最大值

; 转换
CVTSI2SS/CVTSI2SD    ; 整数转标量单/双精度浮点
CVTSS2SI/CVTSD2SI    ; 标量浮点转整数
```

## 三、指令示例

### 基本算术操作
```assembly
; 计算 (a + b) * c
mov eax, [a]        ; eax = a
add eax, [b]        ; eax = a + b
imul eax, [c]       ; eax = (a + b) * c
mov [result], eax   ; 存储结果
```

### 条件判断
```assembly
; if (x > y) { x = x - y; } else { x = y - x; }
mov eax, [x]
mov ebx, [y]
cmp eax, ebx        ; 比较x和y
jg greater          ; 如果x > y，跳转到greater
sub ebx, eax        ; y - x
mov [x], ebx
jmp done
greater:
sub eax, ebx        ; x - y
mov [x], eax
done:
```

### 循环
```assembly
; for (int i = 0; i < 10; i++) { sum += i; }
mov ecx, 10         ; 循环计数器
mov eax, 0          ; sum = 0
mov ebx, 0          ; i = 0
loop_start:
add eax, ebx        ; sum += i
inc ebx             ; i++
loop loop_start     ; ecx--，如果ecx≠0则跳转
```

### 字符串复制
```assembly
; 复制字符串
mov esi, source_str ; 源地址
mov edi, dest_str   ; 目的地址
mov ecx, length     ; 长度
cld                 ; 清除方向标志（向前复制）
rep movsb           ; 重复复制字节
```

## 四、寄存器使用约定

### Windows x64调用约定
```assembly
; 前4个整数/指针参数：RCX, RDX, R8, R9
; 前4个浮点参数：XMM0, XMM1, XMM2, XMM3
; 返回值：RAX（整数），XMM0（浮点）
; 调用者保存：RAX, RCX, RDX, R8-R11, XMM0-XMM5
; 被调用者保存：RBX, RBP, RDI, RSI, R12-R15, XMM6-XMM15
```

### Linux x64调用约定
```assembly
; 前6个整数/指针参数：RDI, RSI, RDX, RCX, R8, R9
; 前8个浮点参数：XMM0-XMM7
; 返回值：RAX（整数），XMM0（浮点）
; 调用者保存：R10, R11
; 被调用者保存：RBX, RBP, R12-R15
```

## 五、实用技巧

### 1. 清零寄存器的高效方法
```assembly
xor eax, eax    ; 比 mov eax, 0 更快（1字节 vs 5字节）
```

### 2. 测试寄存器是否为0
```assembly
test eax, eax   ; 设置ZF标志，不改变寄存器值
jz zero_case    ; 如果eax=0则跳转
```

### 3. 乘法的优化
```assembly
; 乘以2的幂次方
shl eax, 3      ; eax * 8，比 imul eax, 8 更快

; 乘以常数
lea eax, [eax*4 + eax]    ; eax * 5 (eax*4 + eax)
```

### 4. 条件移动（无分支）
```assembly
; 替代if-else分支
cmp eax, ebx
cmovg ecx, edx   ; 如果eax>ebx，则ecx=edx（无分支跳转）
```

## 六、其他知识

1. **指令后缀**：B（字节）、W（字）、D（双字）、Q（四字）
2. **操作数顺序**：Intel语法是`目标, 源`，AT&T语法是`源, 目标`
3. **内存寻址**：`[基址 + 索引*比例 + 位移]`
4. **对齐**：某些指令要求内存地址对齐（如MOVAPS要求16字节对齐）

记住，汇编语言依赖于具体的处理器架构（x86, ARM, MIPS等）。上述内容主要针对x86/x64架构。不同的汇编器（NASM, MASM, GAS）可能有不同的语法细节。

## 七、注意事项

1. 汇编指令的语法有Intel和AT&T两种，上述示例为Intel语法（常用于Windows和NASM）。AT&T语法（常用于GCC）中操作数顺序相反，并且寄存器前有%号，立即数前有$号。
2. 不同的汇编器（如NASM、MASM、GAS）可能有不同的语法规则。
3. 不同的操作系统有不同的系统调用约定（如Linux使用系统调用号，而Windows使用API）。
4. 在64位模式下，大多数指令的操作数默认是32位的，但使用64位寄存器时会使用64位操作。可以使用操作数大小前缀来改变默认大小。

以上只是x86-64汇编语言的一个概览。实际上，指令集非常庞大，还包括许多扩展指令（如MMX、SSE、AVX等）。学习汇编语言需要结合实践，从简单的程序开始，逐步深入。