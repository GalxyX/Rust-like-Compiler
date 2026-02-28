# Rust-like-Compiler (类 Rust 语言编译器)

Rust-like-Compiler 是一个基于 C++ 开发的简单类 Rust 语言编译器。它能够将类 Rust 语言的源代码（支持变量声明、函数定义、控制流等）编译为 MIPS 汇编代码。本项目适合用于学习编译原理的核心概念，可以深入掌握编译前端和后端各个阶段的基本实现原理。

本项目实现了编译器的完整流程，包括词法分析、语法分析（LR1）、语义分析、中间代码生成（四元式）以及目标代码生成（MIPS）等。此外，项目支持以 JSON 格式输出编译过程中的中间数据（如 Token 序列、语法分析表、四元式等），便于前端可视化展示。

![demo](https://github.com/user-attachments/assets/208d0809-98fd-4ef1-bcb1-341093352dc5)

## ✨ 功能特性 (Features)

本编译器按照标准编译流程划分为多个核心模块，各模块功能如下：

### 1. 词法分析 (Lexical Analysis)

词法分析器负责将源代码字符流转换为 Token 流，支持自动过滤注释。

- **关键字 (Keywords)**:
  - 数据类型: `i8`, `u8`, `i16`, `u16`, `i32`, `u32`, `i64`, `u64`, `f32`, `f64`, `bool`, `char` 等。
  - 流控制: `if`, `else`, `while`, `for`, `in`, `loop`, `break`, `continue`, `return`。
  - 声明与修饰: `fn`, `let`, `mut`, `const`, `static`。
  - 字面量: `true`, `false`。
- **标识符 (Identifiers)**: 支持标准变量名与函数名命名规则。
- **字面量 (Literals)**: 整数 (`i32`), 字符 (`char`), 字符串 (`string`)。
- **运算符 (Operators)**:
  - 算术运算: `+`, `-`, `*`, `/`, `%` 以及赋值变体 `+=`, `-=`, `*=`, `/=`, `%=`。
  - 关系运算: `==`, `!=`, `>`, `<`, `>=`, `<=`。
  - 逻辑与位运算: `&&`, `||`, `!`, `&`, `|`, `^`, `<<`, `>>`。
  - 特殊符号: 赋值 `=`, 箭头 `->`, 范围 `..`, 模式匹配 `=>` 等。
- **界符 (Delimiters)**: `(`, `)`, `{`, `}`, `[`, `]`, `;`, `:`, `,`。
- **注释处理**: 支持单行注释 `//` 与多行注释 `/* ... */`，在词法分析阶段自动过滤。

### 2. 语法分析 (Syntax Analysis)

基于 LR(1) 分析法，支持通过文件配置所有的产生式规则进行解析。

本类 Rust 语言编译器使用的产生式结构包括:

- **程序结构**: 支持变量与函数定义的混合序列。
- **函数定义**: 支持形参列表、返回值类型声明及函数体代码块。
- **变量声明**: 支持类型显式标注 (`let x: i32 = 10;`) 与自动推导 (`let x = 10;`)，以及可变性修饰符 `mut`。
- **语句类型**: 赋值语句、表达式语句、返回语句、块语句等。
- **表达式**: 支持算术、比较、函数调用等表达式的嵌套与优先级处理。
- **控制流结构**:
  - 分支: `if ... else if ... else`。
  - 循环: `while`, `for ... in` (支持 `start..end` 范围迭代), `loop`。
  - 跳转: `break`, `continue`。

### 3. 语义分析 (Semantic Analysis)

在语法分析的同时构建抽象语法树 (AST) 并进行语义检查。

- **符号表管理**: 维护变量、常量的作用域、类型、内存地址及引用属性。
- **类型检查**: 确保赋值、运算两端的类型一致性（如 `i32` 不可直接与 `f32` 运算）。
- **引用与借用**: 支持 Rust 特有的引用语义，包括不可变引用 `&` 和可变引用 `& mut`。
- **语义错误检测**:
  - 变量未声明使用。
  - 变量重复声明。
  - 函数参数数量与类型匹配。
  - 赋值给不可变变量 (`let` 非 `mut`) 。
  - ……

### 4. 中间代码生成 (Intermediate Code Generation)

将通过语义分析的代码转化为线性中间表示（四元式）。

- **四元式格式**: `(op, arg1, arg2, result)`。
- **支持指令**:
  - 算术/逻辑运算: `+`, `-`, `*`, `/`, `>`, `<`, `==` 等。
  - 数据传输: 赋值 `=`, 取址 `&`, 解引用 `*`。
  - 控制流: 无条件跳转 `j`, 条件跳转 `j>`, `j<`, `j==` 等。
  - 函数操作: 参数传递 `param`, 函数调用 `call`, 函数返回 `ret`。
- **优化预备**: 生成基本块 (Basic Block) 与控制流图 (Flow Graph)，计算变量的活跃性信息 (Liveness Analysis) 与待用信息 (Next-Use Information)。

### 5. 目标代码生成 (Target Code Generation)

将中间代码翻译为 MIPS 汇编指令，可直接运行。

- **指令映射**: 将四元式操作直译为 MIPS 指令（如 `add`, `sub`, `lw`, `sw`, `beq`, `jal`, `jr` 等）。
- **寄存器分配**: 依据活跃性与待用信息进行寄存器分配。
- **内存管理**: 处理函数调用栈帧，管理局部变量与临时变量的内存偏移。
- **输出**: 生成 `.asm` 或 `.txt` 格式的 MIPS 汇编代码文件。

## 🛠️ 构建与环境 (Build & Environment)

### 环境要求

- **操作系统**: Windows (推荐) / Linux
- **编译器**: 支持 C++17 或更高版本的编译器 (如 MSVC, GCC, Clang)
- **IDE**: Visual Studio 2019/2022 (本项目提供 `.sln` 解决方案文件)

### 依赖库

- [nlohmann/json](https://github.com/nlohmann/json): 用于生成 JSON 格式的中间数据（仅启用 BACKEND 宏时需要）。

### 构建步骤

1.  使用 Visual Studio 打开 `.sln` 解决方案文件。
2.  选择构建配置（`Debug` 或 `Release`）。
3.  点击 **生成解决方案 (Build Solution)**。

> **注意**: 如果你需要启用 JSON 输出功能（用于可视化后端模式），请确保在 main.cpp 中取消注释 `#define BACKEND`。

## 🚀 运行与使用 (Usage)

编译生成 `Rust-like-Compiler.exe` 后，默认情况下编译器会读取 test.rs 作为源代码输入，可以将你的测试代码写入该文件。

### 命令行模式 (Backend Mode Enabled)

如果启用了 `BACKEND` 宏，可以通过命令行参数指定编译阶段：

```bash
# 执行仅词法分析，输出 rust/result/LexicalAnalyzer.json
./Rust-like-Compiler.exe lexical

# 执行语法分析，输出 Action/Goto 表及解析过程
./Rust-like-Compiler.exe grammar

# 执行完整编译，生成四元式和语义检查
./Rust-like-Compiler.exe parse

# (默认) 生成目标代码到 rust/result/objectCode.txt
./Rust-like-Compiler.exe target
```

### 输出文件

编译器的输出文件通常位于 `rust/` 或 `rust/result/` 目录下：

- `intermediateCodes.txt`: 中间代码（四元式）
- `symTable.txt`: 符号表信息
- `objectCode.txt`: 生成的 MIPS 汇编代码
- `*.json`: 各阶段的详细分析数据（需开启后端模式）

## 🎨 图形化界面 (GUI)

本项目提供一个基于 Electron 的前端可视化界面，用于展示编译结果（包括 Token 列表、Action/Goto 表、实时分析过程与生成代码等）。

### 快速启动

1. 在 `main.cpp` 中启用 `BACKEND` 宏，并编译运行后端项目。
2. 将生成的 `Rust-like-Compiler.exe` 放置在前端 `gui/` 目录下。
3. 将生成的 `parser.galp` 放置在 `gui/rust/` 目录下。
4. 进入 `gui` 文件夹：
   ```bash
   cd gui
   ```
5. 安装前端依赖：
   ```bash
   npm install
   ```
6. 启动 Electron 前端界面：
   ```bash
   npm run dev
   ```

### 打包

如需打包前端为独立的 exe 应用，可以根据 `package.json` 中的构建脚本运行如下命令：

```bash
npm run build
```

生成的可执行文件将位于 `gui/dist/` 目录下，可直接运行。

## 📂 项目结构 (Project Structure)

```text
/
├── Rust-like-Compiler/     # 编译器核心代码
│   ├── main.cpp            # 程序入口，控制编译流程
│   ├── LexicalAnalyzer.*   # 词法分析器 (Scanner)
│   ├── Parser.*            # 语法分析器 (LR1 Parser)
│   ├── SemanticAnalyzer.*  # 语义分析器
│   ├── ObjectCodeGenerator.* # 目标代码生成器 (MIPS)
│   ├── json.hpp            # JSON 库头文件
│   └── rust/               # 资源目录
│       ├── test.rs         # 测试用例源代码
│       ├── grammar.txt     # 语法规则定义
│       ├── parser.galp     # 语法分析表二进制缓存（生成）
│       └── result/         # 编译结果输出目录
├── gui/                    # 前端可视化界面
│   ├── main.js             # Electron 主进程入口
│   ├── index.html/style.css # UI 渲染层与样式定义
│   ├── package.json        # 前端依赖与脚本配置
│   └── rust/               # 工作目录
│       ├── test.rs         # 前端界面输入源代码
│       ├── grammar.txt     # 语法规则定义（与后端一致）
│       └── result/         # 编译结果输出目录
├── README.md               # 项目说明文档
├── LICENSE.txt             # 开源许可证
└── .sln                    # Visual Studio 解决方案文件
```

## 📝 语言示例文法 (Example Grammar)

以下是一段支持的类 Rust 代码示例：

```rust
// 阶乘函数
fn factorial(mut n: i32) -> i32 {
    if n <= 1 { return 1; }
    return n * factorial(n - 1);
}

fn main() -> i32 {
    let mut x: i32 = 10;
    let result: i32 = factorial(5);

    // while 循环
    let mut i: i32 = 0;
    while i < 5 {
        x = x + 1;
        i = i + 1;
    }

    return 0;
}
```

## 📄 License

本项目基于 [MIT License](LICENSE.txt) 开源，允许自由使用、修改和分发，但请保留版权声明。
