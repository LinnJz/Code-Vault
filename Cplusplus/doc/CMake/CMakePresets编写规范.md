# CMakePresets.json剖析

https://zhuanlan.zhihu.com/p/17517393367

https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html#macro-expansion

https://learn.microsoft.com/zh-cn/cpp/build/cmake-presets-json-reference?view=msvc-170#visual-studio-settings-vendor-map

https://learn.microsoft.com/zh-cn/cpp/build/cmake-presets-vs?view=msvc-170

## configurePresets数组对象说明

### 字段表格说明

以下是 CMakePresets.json 中 `configurePresets` 数组内每个预设对象所支持的完整字段及其说明。

| 字段              | 类型            | 必填   | 说明                                                         |
| :---------------- | :-------------- | :----- | :----------------------------------------------------------- |
| `name`            | string          | **是** | 预设的**唯一机器友好名称**，用于在 `cmake --preset` 命令中引用。 |
| `hidden`          | boolean         | 否     | 若为 `true`，此预设将**不可见且不可直接使用**，通常用作其他预设的“基类”。 |
| `inherits`        | string / array  | 否     | 要**继承的预设名称**，支持字符串或字符串数组，数组靠前的预设具有更高优先级。 |
| `displayName`     | string          | 否     | 供 UI 显示用的预设**友好名称**。                             |
| `description`     | string          | 否     | 对预设的**详细描述**，用于帮助理解其用途。                   |
| `generator`       | string          | 条件性 | 指定构建系统**生成器**（如 "Ninja", "Visual Studio 17 2022"）。隐藏预设或已通过继承链指定的情况下可省略。 |
| `architecture`    | string / object | 否     | 设置目标平台架构（如 "Win32", "x64", "ARM64"）。用于 VS 等生成器，可使用 `value` 和 `strategy` 字段进行高级配置。 |
| `toolset`         | string / object | 否     | 设置工具集（如 "v143", "ClangCL"），用法与 `architecture` 类似。 |
| `toolchainFile`   | string          | 否     | 指定工具链文件路径（CMake 3.21+）。支持宏展开，优先级高于 `CMAKE_TOOLCHAIN_FILE` 变量。 |
| `binaryDir`       | string          | 条件性 | 构建输出目录。支持宏展开，隐藏预设或已通过继承链指定的情况下可省略。 |
| `installDir`      | string          | 否     | 安装目录，对应 `CMAKE_INSTALL_PREFIX` 变量。支持宏展开（CMake 3.21+）。 |
| `cmakeExecutable` | string          | 否     | CMake 可执行文件路径。**仅供 IDE 使用**，CMake 命令行会忽略此字段。 |
| `cacheVariables`  | object          | 否     | 设置 CMake 缓存变量。支持 `null`、布尔值、字符串或带 `type` 和 `value` 的对象，设为 `null` 可清除继承的变量。 |
| `environment`     | object          | 否     | 设置构建时环境变量。支持宏展开，设为 `null` 可清除继承的变量。 |
| `condition`       | object          | 否     | 条件对象（CMake 3.21+），用于预设的平台或环境过滤（如 `{ "type": "equals", "lhs": "${hostSystemName}", "rhs": "Windows" }`）。 |
| `vendor`          | object          | 否     | 存储**供应商特定的元数据**。CMake 不解析内容，键通常为供应商的域名（如 `"example.com/ExampleIDE/1.0"`）。 |
| `warnings`        | object          | 否     | 控制 CMake 警告输出，包含 `dev`、`deprecated`、`uninitialized`、`unusedCli`、`systemVars` 等子字段。 |
| `errors`          | object          | 否     | 将警告视为错误，包含 `dev`、`deprecated` 子字段。            |
| `debug`           | object          | 否     | 启用调试输出，包含 `output`、`tryCompile`、`find` 等子字段。 |
| `trace`           | object          | 否     | 控制详细调用跟踪（CMake 3.21+），包含 `mode`、`format`、`source`、`redirect` 等子字段。 |
| `graphviz`        | string          | 否     | 指定 Graphviz 输入文件路径，用于生成依赖关系图。支持宏展开（CMake 4.0+）。 |

### 补充说明

*   **版本要求**：`condition`、`toolchainFile`、`installDir`、`warnings` 等字段在 CMake 3.21 或更高版本中才被支持。
*   **继承规则**：使用 `inherits` 字段可避免配置重复。除 `name`、`hidden`、`inherits`、`description` 和 `displayName` 外，所有字段默认都会被继承，子预设可覆盖父预设的值。
*   **条件判断**：`condition` 字段允许你创建特定于平台的预设。例如，可以定义一个仅在 Windows 系统上生效的预设，或者根据某个 CMake 变量的值来决定是否启用预设。

## CMakePresets.json完整示例说明

```json
{
  "version": 9, // VS2022 最多支持到9
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 28,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "msvc-base",
      "hidden": true,
      "displayName": "Base Configuration",
      "description": "Base preset for MSVC x64 builds using Ninja",
      "generator": "Ninja",
      "architecture": {
        "value": "x64",
        "strategy": "external" // 提供set和external两种策略，external灵活，set强制规定
      },
      "toolset": {
        "value": "v143", // VS2022工具集编号v143，如果时set则强制，如果是external没有会找其他的工具集，如VS2019 V142
        "strategy": "external"
      },
      "binaryDir": "${sourceDir}/out/build/${presetName}",
      "installDir": "${sourceDir}/out/install/${presetName}",
      // "cmakeExecutable": "cmake", // 不建议开启，由VS自动获取
      "cacheVariables": {
        "CMAKE_C_COMPILER": "cl.exe",
        "CMAKE_CXX_COMPILER": "cl.exe",
          // 以下两项可选，因为可以在CMakeLists.txt设置
        "CMAKE_CONFIGURATION_TYPES": "Debug;RelWithDebInfo;Release",
        "CMAKE_MSVC_RUNTIME_LIBRARY": "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
      },
      // 不建议开启，由VS自动获取
      /*"environment": {
        "PATH": "${sourceDir}/tools;$env{PATH}"
      },*/
      "condition": {
        "type": "equals",
        "lhs": "${hostSystemName}",
        "rhs": "Windows"
      },
      "vendor": {
        "example.com/IDE/1.0": {
          "intelliSenseMode": "msvc-x64",
          "someCustomFlag": true
        }
      },
       // version 9 不支持 trace graphviz，不建议开启warnings、errors、debug容易产生大量cmake生成错误
       /*
      "warnings": {
        "dev": true,
        "deprecated": true,
        "uninitialized": true,
        "unusedCli": false,
        "systemVars": true
      },
      "errors": {
        "dev": true,
        "deprecated": false
      },
      "debug": {
        "output": true,
        "tryCompile": true,
        "find": true
      },
      
      "trace": {
        "mode": "on",
        "format": "human",
        "source": "${sourceDir}/trace.log",
        "redirect": "${sourceDir}/trace_output.txt"
      },
      "graphviz": "${sourceDir}/graphviz/deps.dot"
      */
    },
    {
      "name": "msvc_x64_debug",
      "inherits": "msvc-base",
      "displayName": "X64 Debug MSVC",
      "description": "Debug build with full symbols",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        // CMAKE_CXX_FLAGS_DEBUG 可以由CMakeLists.txt设置
        "CMAKE_CXX_FLAGS_DEBUG": "/Od /Zi /MDd"
      },
      "environment": {
        "BUILD_MODE": "debug"
      }
    },
    {
      "name": "msvc_x64_relwithdebinfo",
      "inherits": "msvc-base",
      "displayName": "X64 RelWithDebInfo MSVC",
      "description": "RelWithDebInfo build with full symbols",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        // CMAKE_CXX_FLAGS_RELWITHDEBINFO 可以由CMakeLists.txt设置
        "CMAKE_CXX_FLAGS_RELWITHDEBINFO": "/O2 /Zi /MD"
      },
      "environment": {
        "BUILD_MODE": "relwithdebinfo"
      }
    },
    {
      "name": "msvc_x64_release",
      "inherits": "msvc-base",
      "displayName": "X64 Release MSVC",
      "description": "Release build with full symbols",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        // CMAKE_CXX_FLAGS_RELEASE 可以由CMakeLists.txt设置
        "CMAKE_CXX_FLAGS_RELEASE": "/O2 /MD"
      },
      "environment": {
        "BUILD_MODE": "release"
      }
    }
  ],
  // buildPresets、testPresets是可选项可以不设置
  "buildPresets": [
    {
      "name": "debug-build",
      "configurePreset": "debug",
      "displayName": "Build Debug"
    },
    {
      "name": "relwithdebinfo-build",
      "configurePreset": "relwithdebinfo",
      "displayName": "Build RelWithDebInfo"
    },
    {
      "name": "release-build",
      "configurePreset": "release",
      "displayName": "Build Release"
    }
  ],
  "testPresets": [
    {
      "name": "debug-test",
      "configurePreset": "debug",
      "displayName": "Test Debug"
    }
  ]
}
```

# 官方文档翻译

## 简介

<div class="versionadded">
<p><span class="versionmodified added">添加于版本 3.19。</span></p>
</div>

CMake 用户经常遇到的一个问题是：如何与他人分享配置项目的常用设置。这可能是为了支持 CI 构建，或者为了方便经常使用相同构建方式的用户。CMake 支持两个主要文件 `CMakePresets.json` 和 `CMakeUserPresets.json`，允许用户指定常见的配置选项并与他人共享。CMake 还支持通过 `include` 字段包含的文件。

`CMakePresets.json` 和 `CMakeUserPresets.json` 位于项目的根目录。它们具有完全相同的格式，并且都是可选的（但如果指定了 [`--preset`](cmake.1.html#cmdoption-cmake-preset) 参数，则至少必须存在一个）。`CMakePresets.json` 用于指定项目范围的构建细节，而 `CMakeUserPresets.json` 供开发者指定他们自己的本地构建细节。

`CMakePresets.json` 可以签入版本控制系统，而 `CMakeUserPresets.json` **不应**签入。例如，如果项目使用 Git，则可以跟踪 `CMakePresets.json`，而 `CMakeUserPresets.json` 应添加到 `.gitignore` 中。

## 格式

这些文件是一个 JSON 文档，根节点为一个对象：

```json
{
  "version": 10,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 23,
    "patch": 0
  },
  "$comment": "An example CMakePresets.json file",
  "include": [
    "otherThings.json",
    "moreThings.json"
  ],
  "configurePresets": [
    {
      "$comment": [
        "This is a comment row.",
        "This is another comment,",
        "just because we can do it"
      ],
      "name": "default",
      "displayName": "Default Config",
      "description": "Default build using Ninja generator",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/default",
      "cacheVariables": {
        "FIRST_CACHE_VARIABLE": {
          "type": "BOOL",
          "value": "OFF"
        },
        "SECOND_CACHE_VARIABLE": "ON"
      },
      "environment": {
        "MY_ENVIRONMENT_VARIABLE": "Test",
        "PATH": "$env{HOME}/ninja/bin:$penv{PATH}"
      },
      "vendor": {
        "example.com/ExampleIDE/1.0": {
          "autoFormat": true
        }
      }
    },
    {
      "name": "ninja-multi",
      "inherits": "default",
      "displayName": "Ninja Multi-Config",
      "description": "Default build using Ninja Multi-Config generator",
      "generator": "Ninja Multi-Config"
    },
    {
      "name": "windows-only",
      "inherits": "default",
      "displayName": "Windows-only configuration",
      "description": "This build is only available on Windows",
      "condition": {
        "type": "equals",
        "lhs": "${hostSystemName}",
        "rhs": "Windows"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "default",
      "configurePreset": "default"
    }
  ],
  "testPresets": [
    {
      "name": "default",
      "configurePreset": "default",
      "output": {"outputOnFailure": true},
      "execution": {"noTestsAction": "error", "stopOnFailure": true}
    }
  ],
  "packagePresets": [
    {
      "name": "default",
      "configurePreset": "default",
      "generators": [
        "TGZ"
      ]
    }
  ],
  "workflowPresets": [
    {
      "name": "default",
      "steps": [
        {
          "type": "configure",
          "name": "default"
        },
        {
          "type": "build",
          "name": "default"
        },
        {
          "type": "test",
          "name": "default"
        },
        {
          "type": "package",
          "name": "default"
        }
      ]
    }
  ],
  "vendor": {
    "example.com/ExampleIDE/1.0": {
      "autoFormat": false
    }
  }
}
```

指定版本 `10` 或以上的预设文件可以包含注释，使用键 `$comment` 在 JSON 对象的任何层级提供文档。

根对象识别以下字段：

- **`$schema`**  
  可选的字符串，提供描述此 JSON 文档结构的 JSON 模式的 URI。该字段用于支持 JSON 模式的编辑器中的验证和自动补全。它不影响文档本身的行为。如果未指定此字段，JSON 文档仍然有效，但使用 JSON 模式进行验证和自动补全的工具可能无法正常工作。

- **`version`**  
  必需的整数，表示 JSON 模式的版本。参见 [版本](#versions) 了解支持的版本以及添加它们的相应 CMake 版本。

- **`cmakeMinimumRequired`**  
  可选的对象，表示构建此项目所需的最小 CMake 版本。此对象包含以下字段：
  - **`major`**：可选的整数，表示主版本号。
  - **`minor`**：可选的整数，表示次版本号。
  - **`patch`**：可选的整数，表示补丁版本号。

- **`include`**  
  可选的字符串数组，表示要包含的文件。如果文件名不是绝对路径，则视为相对于当前文件。在指定版本 `4` 或以上的预设文件中允许。参见 [包含文件](#includes) 了解对包含文件的约束。

- **`vendor`**  
  可选的映射，包含供应商特定信息。CMake 不解释此字段的内容，但如果存在则验证其为映射。不过，键应为供应商特定的域名后跟 `/` 分隔的路径。例如，示例 IDE 1.0 可以使用 `example.com/ExampleIDE/1.0`。每个字段的值可以是供应商想要的任何内容，但通常是一个映射。

- **`configurePresets`**  
  可选的 [配置预设](#configure-preset) 对象数组。在指定版本 `1` 或以上的预设文件中允许。

- **`buildPresets`**  
  可选的 [构建预设](#build-preset) 对象数组。在指定版本 `2` 或以上的预设文件中允许。

- **`testPresets`**  
  可选的 [测试预设](#test-preset) 对象数组。在指定版本 `2` 或以上的预设文件中允许。

- **`packagePresets`**  
  可选的 [打包预设](#package-preset) 对象数组。在指定版本 `6` 或以上的预设文件中允许。

- **`workflowPresets`**  
  可选的 [工作流预设](#workflow-preset) 对象数组。在指定版本 `6` 或以上的预设文件中允许。

### 包含文件

`CMakePresets.json` 和 `CMakeUserPresets.json` 可以在文件版本 `4` 及更高版本中使用 `include` 字段包含其他文件。这些文件所包含的文件也可以再包含其他文件。如果 `CMakePresets.json` 和 `CMakeUserPresets.json` 同时存在，则 `CMakeUserPresets.json` 隐式包含 `CMakePresets.json`，即使没有 `include` 字段，在所有版本的格式中都是如此。

如果预设文件包含从另一个文件中的预设继承的预设，则该文件必须直接或间接地包含另一个文件。文件之间不允许包含循环。如果 `a.json` 包含 `b.json`，则 `b.json` 不能包含 `a.json`。但是，一个文件可以被同一个文件或不同文件多次包含。

从 `CMakePresets.json` 直接或间接包含的文件应由项目保证提供。`CMakeUserPresets.json` 可以包含来自任何位置的文件。

从版本 `7` 开始，`include` 字段支持[宏展开](#macro-expansion)，但仅支持 `$penv{}` 宏展开。从版本 `9` 开始，其他宏展开也可用，但 `$env{}` 和预设特定宏（即从预设定义内部的字段派生的宏，如 `presetName`）除外。

### 配置预设

`configurePresets` 数组的每个条目是一个 JSON 对象，可以包含以下字段：

- **`name`**  
  必需的字符串，表示预设的机器友好名称。该标识符用于 [`cmake --preset`](cmake.1.html#cmdoption-cmake-preset) 选项。在同一个目录下，`CMakePresets.json` 和 `CMakeUserPresets.json` 的并集中不能有两个配置预设同名。但是，配置预设可以与构建、测试、打包或工作流预设同名。

- **`hidden`**  
  可选的布尔值，指定预设是否应隐藏。如果预设隐藏，则不能用于 `--preset` 参数，不会出现在 [CMake GUI](cmake-gui.1.html) 中，并且即使通过继承也不需要有有效的 `generator` 或 `binaryDir`。`hidden` 预设旨在作为其他预设通过 `inherits` 字段继承的基础。

- **`inherits`**  
  可选的字符串数组，表示要继承的预设名称。该字段也可以是字符串，等效于包含一个字符串的数组。  
  预设默认会继承 `inherits` 预设中的所有字段（除了 `name`、`hidden`、`inherits`、`description` 和 `displayName`），但可以根据需要覆盖它们。如果多个 `inherits` 预设对同一字段提供冲突的值，则 `inherits` 数组中较早的预设优先。  
  预设只能继承定义在同一文件或其直接/间接包含的文件中的另一个预设。`CMakePresets.json` 中的预设不能继承 `CMakeUserPresets.json` 中的预设。

- **`condition`**  
  可选的 [Condition](#condition) 对象。在指定版本 `3` 或以上的预设文件中允许。

- **`vendor`**  
  可选的映射，包含供应商特定信息。CMake 不解释此字段的内容，但如果存在则验证其为映射。不过，它应遵循与根级 `vendor` 字段相同的约定。如果供应商使用自己的每个预设的 `vendor` 字段，则应在适当时以合理的方式实现继承。

- **`displayName`**  
  可选的字符串，预设的人类友好名称。

- **`description`**  
  可选的字符串，预设的人类友好描述。

- **`generator`**  
  可选的字符串，表示为预设使用的[生成器](cmake-generators.7.html)。如果未指定 `generator`，则必须从 `inherits` 预设继承（除非此预设是 `hidden`）。在版本 `3` 或以上，可以省略此字段以回退到常规的生成器发现过程。  
  注意，对于 [Visual Studio 生成器](cmake-generators.7.html#visual-studio-generators)，与命令行 [`-G`](cmake.1.html#cmdoption-cmake-G) 参数不同，不能在生成器名称中包含平台名称。应改用 `architecture` 字段。

- **`architecture`、`toolset`**  
  可选的字段，分别表示支持这些字段的生成器的平台和工具集。  
  参见 [`cmake -A`](cmake.1.html#cmdoption-cmake-A) 选项了解 `architecture` 的可能值，以及 [`cmake -T`](cmake.1.html#cmdoption-cmake-T) 了解 `toolset`。  
  每个字段可以是字符串或具有以下字段的对象：
  - **`value`**：可选的字符串，表示值。
  - **`strategy`**：可选的字符串，告诉 CMake 如何处理 `architecture` 或 `toolset` 字段。有效值为：
    - `"set"`：设置相应的值。对于不支持相应字段的生成器，这将导致错误。
    - `"external"`：即使生成器支持也不设置该值。例如，如果预设使用 Ninja 生成器，而 IDE 知道如何从 `architecture` 和 `toolset` 字段设置 Visual C++ 环境，则此设置很有用。在这种情况下，CMake 将忽略该字段，但 IDE 可以使用它们来调用 CMake 之前设置环境。  
    如果没有给出 `strategy` 字段，或者字段使用字符串形式而不是对象形式，则行为与 `"set"` 相同。

- **`toolchainFile`**  
  可选的字符串，表示工具链文件的路径。该字段支持[宏展开](#macro-expansion)。如果指定了相对路径，则首先相对于构建目录计算，如果未找到，则相对于源目录计算。此字段优先于任何 [`CMAKE_TOOLCHAIN_FILE`](../variable/CMAKE_TOOLCHAIN_FILE.html) 值。在指定版本 `3` 或以上的预设文件中允许。

- **`graphviz`**  
  可选的字符串，表示 graphviz 输入文件的路径，该文件将包含项目中所有库和可执行文件的依赖关系。有关更多详细信息，请参见 [`cmake --graphviz`](cmake.1.html#cmdoption-cmake-graphviz) 的文档。  
  该字段支持宏展开。如果指定了相对路径，则相对于当前工作目录计算。在指定版本 `10` 或以上的预设文件中允许。

- **`binaryDir`**  
  可选的字符串，表示输出二进制目录的路径。该字段支持宏展开。如果指定了相对路径，则相对于源目录计算。如果未指定 `binaryDir`，则必须从 `inherits` 预设继承（除非此预设是 `hidden`）。在版本 `3` 或以上，可以省略此字段。

- **`installDir`**  
  可选的字符串，表示安装目录的路径，将用作 [`CMAKE_INSTALL_PREFIX`](../variable/CMAKE_INSTALL_PREFIX.html) 变量。该字段支持宏展开。如果指定了相对路径，则相对于源目录计算。在指定版本 `3` 或以上的预设文件中允许。

- **`cmakeExecutable`**  
  可选的字符串，表示为该预设使用的 CMake 可执行文件的路径。此字段保留供 IDE 使用，CMake 本身不使用。使用此字段的 IDE 应展开其中的任何宏。

- **`cacheVariables`**  
  可选的缓存变量映射。键是变量名（不能为空字符串），值可以是 `null`、布尔值（等效于值为 `"TRUE"` 或 `"FALSE"` 且类型为 `BOOL`）、表示变量值的字符串（支持宏展开），或者具有以下字段的对象：
  - **`type`**：可选的字符串，表示变量的类型。
  - **`value`**：必需的字符串或布尔值，表示变量的值。布尔值等效于 `"TRUE"` 或 `"FALSE"`。该字段支持宏展开。  
  缓存变量通过 `inherits` 字段继承，预设的变量将是其自己的 `cacheVariables` 与其所有父级的 `cacheVariables` 的并集。如果此并集中的多个预设定义了同一变量，则应用 `inherits` 的标准规则。将变量设置为 `null` 会导致不设置该变量，即使从另一个预设继承了值。

- **`environment`**  
  可选的环境变量映射。键是变量名（不能为空字符串），值要么是 `null`，要么是表示变量值的字符串。每个变量都会被设置，无论进程环境是否已给它一个值。  
  该字段支持宏展开，并且此映射中的环境变量可以相互引用，可以按任何顺序列出，只要此类引用不会导致循环（例如，如果 `ENV_1` 是 `$env{ENV_2}`，则 `ENV_2` 不能是 `$env{ENV_1}`）。`$penv{NAME}` 允许通过仅访问父环境中的值来对现有环境变量进行前置或附加。  
  环境变量通过 `inherits` 字段继承，预设的环境将是其自己的 `environment` 与其所有父级的 `environment` 的并集。如果此并集中的多个预设定义了同一变量，则应用 `inherits` 的标准规则。将变量设置为 `null` 会导致不设置该变量，即使从另一个预设继承了值。

- **`warnings`**  
  可选的指定要启用的警告的对象。该对象可以包含以下字段：
  - **`dev`**：可选的布尔值。等效于在命令行传递 [`-Wdev`](cmake.1.html#cmdoption-cmake-Wdev) 或 [`-Wno-dev`](cmake.1.html#cmdoption-cmake-Wno-dev)。如果 `errors.dev` 设置为 `true`，则不能设置为 `false`。
  - **`deprecated`**：可选的布尔值。等效于传递 [`-Wdeprecated`](cmake.1.html#cmdoption-cmake-Wdeprecated) 或 [`-Wno-deprecated`](cmake.1.html#cmdoption-cmake-Wno-deprecated)。如果 `errors.deprecated` 设置为 `true`，则不能设置为 `false`。
  - **`uninitialized`**：可选的布尔值。设置为 `true` 等效于传递 [`--warn-uninitialized`](cmake.1.html#cmdoption-cmake-warn-uninitialized)。
  - **`unusedCli`**：可选的布尔值。设置为 `false` 等效于传递 [`--no-warn-unused-cli`](cmake.1.html#cmdoption-cmake-no-warn-unused-cli)。
  - **`systemVars`**：可选的布尔值。设置为 `true` 等效于传递 [`--check-system-vars`](cmake.1.html#cmdoption-cmake-check-system-vars)。

- **`errors`**  
  可选的指定要启用的错误的对象。该对象可以包含以下字段：
  - **`dev`**：可选的布尔值。等效于传递 [`-Werror=dev`](cmake.1.html#cmdoption-cmake-Werror) 或 [`-Wno-error=dev`](cmake.1.html#cmdoption-cmake-Werror)。如果 `warnings.dev` 设置为 `false`，则不能设置为 `true`。
  - **`deprecated`**：可选的布尔值。等效于传递 [`-Werror=deprecated`](cmake.1.html#cmdoption-cmake-Werror) 或 [`-Wno-error=deprecated`](cmake.1.html#cmdoption-cmake-Werror)。如果 `warnings.deprecated` 设置为 `false`，则不能设置为 `true`。

- **`debug`**  
  可选的指定调试选项的对象。该对象可以包含以下字段：
  - **`output`**：可选的布尔值。设置为 `true` 等效于传递 [`--debug-output`](cmake.1.html#cmdoption-cmake-debug-output)。
  - **`tryCompile`**：可选的布尔值。设置为 `true` 等效于传递 [`--debug-trycompile`](cmake.1.html#cmdoption-cmake-debug-trycompile)。
  - **`find`**：可选的布尔值。设置为 `true` 等效于传递 [`--debug-find`](cmake.1.html#cmdoption-cmake-debug-find)。

- **`trace`**  
  可选的指定跟踪选项的对象。在指定版本 `7` 的预设文件中允许。该对象可以包含以下字段：
  - **`mode`**：可选的字符串，指定跟踪模式。有效值为：
    - `on`：打印所有调用及其位置的跟踪。等效于传递 [`--trace`](cmake.1.html#cmdoption-cmake-trace)。
    - `off`：不打印调用跟踪。
    - `expand`：打印展开变量的调用跟踪。等效于传递 [`--trace-expand`](cmake.1.html#cmdoption-cmake-trace-expand)。
  - **`format`**：可选的字符串，指定跟踪输出的格式。有效值为：
    - `human`：以人类可读格式打印每行跟踪。这是默认格式。等效于传递 [`--trace-format=human`](cmake.1.html#cmdoption-cmake-trace-format)。
    - `json-v1`：将每行打印为单独的 JSON 文档。等效于传递 [`--trace-format=json-v1`](cmake.1.html#cmdoption-cmake-trace-format)。
  - **`source`**：可选的字符串数组，表示要跟踪的源文件路径。该字段也可以是字符串，等效于包含一个字符串的数组。等效于传递 [`--trace-source`](cmake.1.html#cmdoption-cmake-trace-source)。
  - **`redirect`**：可选的字符串，指定跟踪输出文件的路径。等效于传递 [`--trace-redirect`](cmake.1.html#cmdoption-cmake-trace-redirect)。

### 测试预设

每个 `testPresets` 数组的条目是一个 JSON 对象，可以包含以下字段：

- **`name`**  
  必需的字符串，表示预设的机器友好名称。该标识符用于 [`ctest --preset`](ctest.1.html#cmdoption-ctest-preset) 选项。  
  在同一个目录下，`CMakePresets.json` 和 `CMakeUserPresets.json` 的并集中不能有两个测试预设同名。但是，测试预设可以与配置、构建、打包或工作流预设同名。

- **`hidden`**  
  可选的布尔值，指定预设是否应隐藏。如果预设隐藏，则不能用于 `--preset` 参数，并且即使通过继承也不需要有有效的 `configurePreset`。`hidden` 预设旨在作为其他预设通过 `inherits` 字段继承的基础。

- **`inherits`**  
  可选的字符串数组，表示要继承的预设名称。该字段也可以是字符串，等效于包含一个字符串的数组。  
  预设默认会继承 `inherits` 预设中的所有字段（除了 `name`、`hidden`、`inherits`、`description` 和 `displayName`），但可以根据需要覆盖它们。如果多个 `inherits` 预设对同一字段提供冲突的值，则 `inherits` 数组中较早的预设优先。  
  预设只能继承定义在同一文件或其直接/间接包含的文件中的另一个预设。`CMakePresets.json` 中的预设不能继承 `CMakeUserPresets.json` 中的预设。

- **`condition`**  
  可选的 [Condition](#condition) 对象。在指定版本 `3` 或以上的预设文件中允许。

- **`vendor`**  
  可选的映射，包含供应商特定信息。CMake 不解释此字段的内容，但如果存在则验证其为映射。不过，它应遵循与根级 `vendor` 字段相同的约定。如果供应商使用自己的每个预设的 `vendor` 字段，则应在适当时以合理的方式实现继承。

- **`displayName`**  
  可选的字符串，预设的人类友好名称。

- **`description`**  
  可选的字符串，预设的人类友好描述。

- **`environment`**  
  可选的环境变量映射。键是变量名（不能为空字符串），值要么是 `null`，要么是表示变量值的字符串。每个变量都会被设置，无论进程环境是否已给它一个值。  
  该字段支持 [宏展开](#macro-expansion)，并且此映射中的环境变量可以相互引用，可以按任何顺序列出，只要此类引用不会导致循环（例如，如果 `ENV_1` 是 `$env{ENV_2}`，则 `ENV_2` 不能是 `$env{ENV_1}`）。`$penv{NAME}` 允许通过仅访问父环境中的值来对现有环境变量进行前置或附加。  
  环境变量通过 `inherits` 字段继承，预设的环境将是其自己的 `environment` 与其所有父级的 `environment` 的并集。如果此并集中的多个预设定义了同一变量，则应用 `inherits` 的标准规则。将变量设置为 `null` 会导致不设置该变量，即使从另一个预设继承了值。

- **`configurePreset`**  
  可选的字符串，指定与此测试预设关联的配置预设的名称。如果未指定 `configurePreset`，则必须从继承预设继承（除非此预设是隐藏的）。构建目录从配置预设推断，因此测试将在与配置和构建相同的 `binaryDir` 中运行。

- **`inheritConfigureEnvironment`**  
  可选的布尔值，默认为 true。如果为 true，则来自关联配置预设的环境变量在所有继承的测试预设环境之后、但在此测试预设中显式指定的环境变量之前被继承。

- **`configuration`**  
  可选的字符串。等效于在命令行传递 [`--build-config`](ctest.1.html#cmdoption-ctest-C)。

- **`overwriteConfigurationFile`**  
  可选的配置选项数组，用于覆盖 CTest 配置文件中指定的选项。等效于为数组中的每个值传递 [`--overwrite`](ctest.1.html#cmdoption-ctest-overwrite)。数组值支持宏展开。

- **`output`**  
  可选的指定输出选项的对象。该对象可以包含以下字段：

  - **`shortProgress`**  
    可选的布尔值。如果为 true，等效于在命令行传递 [`--progress`](ctest.1.html#cmdoption-ctest-progress)。

  - **`verbosity`**  
    可选的字符串，指定详细级别。必须是以下之一：
    - `default`：等效于命令行不传递任何详细标志。
    - `verbose`：等效于传递 [`--verbose`](ctest.1.html#cmdoption-ctest-V)。
    - `extra`：等效于传递 [`--extra-verbose`](ctest.1.html#cmdoption-ctest-VV)。

  - **`debug`**  
    可选的布尔值。如果为 true，等效于传递 [`--debug`](ctest.1.html#cmdoption-ctest-debug)。

  - **`outputOnFailure`**  
    可选的布尔值。如果为 true，等效于传递 [`--output-on-failure`](ctest.1.html#cmdoption-ctest-output-on-failure)。

  - **`quiet`**  
    可选的布尔值。如果为 true，等效于传递 [`--quiet`](ctest.1.html#cmdoption-ctest-Q)。

  - **`outputLogFile`**  
    可选的字符串，指定日志文件的路径。等效于传递 [`--output-log`](ctest.1.html#cmdoption-ctest-O)。该字段支持宏展开。

- **`outputJUnitFile`**  
  可选的字符串，指定 JUnit 文件的路径。等效于传递 [`--output-junit`](ctest.1.html#cmdoption-ctest-output-junit)。该字段支持宏展开。在指定版本 `6` 或以上的预设文件中允许。

- **`labelSummary`**  
  可选的布尔值。如果为 false，等效于传递 [`--no-label-summary`](ctest.1.html#cmdoption-ctest-no-label-summary)。

- **`subprojectSummary`**  
  可选的布尔值。如果为 false，等效于传递 [`--no-subproject-summary`](ctest.1.html#cmdoption-ctest-no-subproject-summary)。

- **`maxPassedTestOutputSize`**  
  可选的整数，指定通过的测试的最大输出字节数。等效于传递 [`--test-output-size-passed`](ctest.1.html#cmdoption-ctest-test-output-size-passed)。

- **`maxFailedTestOutputSize`**  
  可选的整数，指定失败的测试的最大输出字节数。等效于传递 [`--test-output-size-failed`](ctest.1.html#cmdoption-ctest-test-output-size-failed)。

- **`testOutputTruncation`**  
  可选的字符串，指定测试输出截断模式。等效于传递 [`--test-output-truncation`](ctest.1.html#cmdoption-ctest-test-output-truncation)。在指定版本 `5` 或以上的预设文件中允许。

- **`maxTestNameWidth`**  
  可选的整数，指定输出测试名称的最大宽度。等效于传递 [`--max-width`](ctest.1.html#cmdoption-ctest-max-width)。

- **`filter`**  
  可选的指定如何筛选要运行的测试的对象。该对象可以包含以下字段：

  - **`include`**  
    可选的指定包含哪些测试的对象。该对象可以包含以下字段：
    - **`name`**  
      可选的字符串，指定测试名称的正则表达式。等效于传递 [`--tests-regex`](ctest.1.html#cmdoption-ctest-R)。该字段支持宏展开。CMake 正则表达式语法在 [`string(REGEX)`](../command/string.html#regex-specification) 中描述。
    - **`label`**  
      可选的字符串，指定测试标签的正则表达式。等效于传递 [`--label-regex`](ctest.1.html#cmdoption-ctest-L)。该字段支持宏展开。
    - **`useUnion`**  
      可选的布尔值。等效于传递 [`--union`](ctest.1.html#cmdoption-ctest-U)。
    - **`index`**  
      可选的按测试索引包含测试的对象。该对象可以包含以下字段。也可以是一个可选的字符串，指定具有 [`--tests-information`](ctest.1.html#cmdoption-ctest-I) 命令行语法的文件。如果指定为字符串，该字段支持宏展开。
      - `start`：可选的整数，指定开始测试的测试索引。
      - `end`：可选的整数，指定停止测试的测试索引。
      - `stride`：可选的整数，指定增量。
      - `specificTests`：可选的整数数组，指定要运行的具体测试索引。

  - **`exclude`**  
    可选的指定排除哪些测试的对象。该对象可以包含以下字段：
    - **`name`**  
      可选的字符串，指定测试名称的正则表达式。等效于传递 [`--exclude-regex`](ctest.1.html#cmdoption-ctest-E)。该字段支持宏展开。
    - **`label`**  
      可选的字符串，指定测试标签的正则表达式。等效于传递 [`--label-exclude`](ctest.1.html#cmdoption-ctest-LE)。该字段支持宏展开。
    - **`fixtures`**  
      可选的指定排除哪些装置（fixture）以添加测试的对象。该对象可以包含以下字段：
      - `any`：可选的字符串，指定排除任何测试的文本装置的正则表达式。等效于 [`--fixture-exclude-any`](ctest.1.html#cmdoption-ctest-FA)。该字段支持宏展开。
      - `setup`：可选的字符串，指定排除设置测试的文本装置的正则表达式。等效于 [`--fixture-exclude-setup`](ctest.1.html#cmdoption-ctest-FS)。该字段支持宏展开。
      - `cleanup`：可选的字符串，指定排除清理测试的文本装置的正则表达式。等效于 [`--fixture-exclude-cleanup`](ctest.1.html#cmdoption-ctest-FC)。该字段支持宏展开。

- **`execution`**  
  可选的指定测试执行选项的对象。该对象可以包含以下字段：

  - **`stopOnFailure`**  
    可选的布尔值。如果为 true，等效于传递 [`--stop-on-failure`](ctest.1.html#cmdoption-ctest-stop-on-failure)。

  - **`enableFailover`**  
    可选的布尔值。如果为 true，等效于传递 [`-F`](ctest.1.html#cmdoption-ctest-F)。

- **`jobs`**  
  可选的整数。等效于传递 [`--parallel`](ctest.1.html#cmdoption-ctest-j)。如果值为 `0`，等效于无限制的并行。  
  在指定版本 `11` 或以上的预设文件中，此字段也可以是字符串，此时必须为空字符串，等效于传递省略了 `<jobs>` 的 `--parallel`。  
  > **版本变更**：从 4.3 版开始，此字段不接受负整数值，无论预设文件中的版本如何。

- **`resourceSpecFile`**  
  可选的字符串。等效于传递 [`--resource-spec-file`](ctest.1.html#cmdoption-ctest-resource-spec-file)。该字段支持宏展开。

- **`testLoad`**  
  可选的整数。等效于传递 [`--test-load`](ctest.1.html#cmdoption-ctest-test-load)。

- **`showOnly`**  
  可选的字符串。等效于传递 [`--show-only`](ctest.1.html#cmdoption-ctest-N)。字符串必须是以下值之一：`human`、`json-v1`。

- **`repeat`**  
  可选的指定如何重复测试的对象。等效于传递 [`--repeat`](ctest.1.html#cmdoption-ctest-repeat)。该对象必须包含以下字段：
  - **`mode`**：必需的字符串，必须是 `until-fail`、`until-pass` 或 `after-timeout` 之一。
  - **`count`**：必需的整数。

- **`interactiveDebugging`**  
  可选的布尔值。如果为 true，等效于传递 [`--interactive-debug-mode 1`](ctest.1.html#cmdoption-ctest-interactive-debug-mode)；如果为 false，等效于传递 `--interactive-debug-mode 0`。

- **`scheduleRandom`**  
  可选的布尔值。如果为 true，等效于传递 [`--schedule-random`](ctest.1.html#cmdoption-ctest-schedule-random)。

- **`timeout`**  
  可选的整数。等效于传递 [`--timeout`](ctest.1.html#cmdoption-ctest-timeout)。

- **`noTestsAction`**  
  可选的字符串，指定如果没有找到测试时的行为。必须是以下值之一：
  - `default`：等效于命令行不传递任何值。
  - `error`：等效于传递 [`--no-tests=error`](ctest.1.html#cmdoption-ctest-no-tests)。
  - `ignore`：等效于传递 [`--no-tests=ignore`](ctest.1.html#cmdoption-ctest-no-tests)。

---

### 打包预设

打包预设可在模式版本 `6` 或以上使用。`packagePresets` 数组的每个条目是一个 JSON 对象，可以包含以下字段：

- **`name`**  
  必需的字符串，表示预设的机器友好名称。该标识符用于 [`cpack --preset`](cpack.1.html#cmdoption-cpack-preset)。在同一个目录下，`CMakePresets.json` 和 `CMakeUserPresets.json` 的并集中不能有两个打包预设同名。但是，打包预设可以与配置、构建、测试或工作流预设同名。

- **`hidden`**  
  可选的布尔值，指定预设是否应隐藏。如果隐藏，则不能用于 `--preset` 参数，并且即使通过继承也不需要有有效的 `configurePreset`。`hidden` 预设旨在作为其他预设继承的基础。

- **`inherits`**  
  可选的字符串数组，表示要继承的预设名称。也可以是字符串。预设默认会继承 `inherits` 预设中的所有字段（除了 `name`、`hidden`、`inherits`、`description`、`displayName`），但可以覆盖。多个继承预设冲突时，数组中较早的优先。预设只能继承同一文件或其包含的文件中的预设。`CMakePresets.json` 中的预设不能继承 `CMakeUserPresets.json` 中的预设。

- **`condition`**  
  可选的 [Condition](#condition) 对象。

- **`vendor`**  
  可选的供应商特定信息映射。CMake 不解释其内容，但如果存在则验证为映射。应遵循根级 `vendor` 的约定。

- **`displayName`**  
  可选的人类友好名称。

- **`description`**  
  可选的人类友好描述。

- **`environment`**  
  可选的环境变量映射。键不能为空字符串，值可以是 `null` 或字符串。支持宏展开，变量可以相互引用但不能循环。`$penv{NAME}` 允许前置/追加父环境的值。环境变量通过继承合并，设置 `null` 可取消继承的值。

- **`configurePreset`**  
  可选的字符串，指定关联的配置预设名称。如果未指定，必须从继承预设继承（除非隐藏）。打包将在与配置相同的 `binaryDir` 中运行。

- **`inheritConfigureEnvironment`**  
  可选的布尔值，默认为 true。若为 true，则关联配置预设的环境变量在继承的打包预设环境之后、显式指定的环境变量之前被继承。

- **`generators`**  
  可选的字符串数组，表示 CPack 使用的生成器。

- **`configurations`**  
  可选的字符串数组，表示 CPack 要打包的构建配置。

- **`variables`**  
  可选的变量映射，传递给 CPack，等效于 `-D` 参数。每个键是变量名，值是分配给该变量的字符串。

- **`configFile`**  
  可选的字符串，表示 CPack 使用的配置文件。

- **`output`**  
  可选的指定输出选项的对象。有效键：
  - **`debug`**：可选的布尔值，是否打印调试信息。`true` 等效于 `--debug`。
  - **`verbose`**：可选的布尔值，是否详细打印。`true` 等效于 `--verbose`。

- **`packageName`**  
  可选的字符串，表示包名称。  
  > **注意**：由于实现问题，此字段不影响最终包文件的名称。包的其他方面可能使用该值，导致不一致。未来的 CMake 版本可能解决此问题，在此之前建议不要使用此字段。

- **`packageVersion`**  
  可选的字符串，表示包版本。  
  > **注意**：同上，建议不要使用。

- **`packageDirectory`**  
  可选的字符串，表示放置包的目录。

- **`vendorName`**  
  可选的字符串，表示供应商名称。

---

### 工作流预设

工作流预设可在模式版本 `6` 或以上使用。`workflowPresets` 数组的每个条目是一个 JSON 对象，可以包含以下字段：

- **`name`**  
  必需的字符串，表示预设的机器友好名称。该标识符用于 [`cmake --workflow --preset`](cmake.1.html#cmdoption-cmake-workflow-preset)。在同一个目录下，`CMakePresets.json` 和 `CMakeUserPresets.json` 的并集中不能有两个工作流预设同名。但是，工作流预设可以与配置、构建、测试或打包预设同名。

- **`vendor`**  
  可选的供应商特定信息映射。CMake 不解释其内容，但如果存在则验证为映射。应遵循根级 `vendor` 的约定。

- **`displayName`**  
  可选的人类友好名称。

- **`description`**  
  可选的人类友好描述。

- **`steps`**  
  必需的数组，描述工作流的步骤。第一步必须是配置预设，所有后续步骤必须是非配置预设，且其 `configurePreset` 字段与起始配置预设匹配。每个对象可以包含以下字段：
  - **`type`**：必需的字符串。第一步必须是 `configure`。后续步骤必须是 `build`、`test` 或 `package`。
  - **`name`**：必需的字符串，表示要作为此工作流步骤运行的配置、构建、测试或打包预设的名称。

---

### 条件

预设的 `condition` 字段（在指定版本 `3` 或以上的预设文件中允许）用于确定预设是否启用。例如，可用于在非 Windows 平台上禁用预设。  
`condition` 可以是布尔值、`null` 或对象。如果是布尔值，表示预设启用或禁用。如果是 `null`，预设启用，但 `null` 条件不会被子预设继承。子条件（例如在 `not`、`anyOf` 或 `allOf` 条件中）不能是 `null`。如果是对象，则具有以下字段：

- **`type`**  
  必需的字符串，取以下值之一：

  - **`"const"`**  
    表示条件为常量。等效于使用布尔值代替对象。额外字段：
    - **`value`**：必需的布尔值，提供条件求值的常量值。

  - **`"equals"`** / **`"notEquals"`**  
    表示比较两个字符串是否相等（或不相等）。额外字段：
    - **`lhs`**：要比较的第一个字符串。支持宏展开。
    - **`rhs`**：要比较的第二个字符串。支持宏展开。

  - **`"inList"`** / **`"notInList"`**  
    表示在字符串列表中搜索字符串。额外字段：
    - **`string`**：要搜索的字符串。支持宏展开。
    - **`list`**：要搜索的字符串数组。支持宏展开，使用短路求值。

  - **`"matches"`** / **`"notMatches"`**  
    表示在字符串中搜索正则表达式。额外字段：
    - **`string`**：要搜索的字符串。支持宏展开。
    - **`regex`**：要搜索的正则表达式。支持宏展开。

  - **`"anyOf"`** / **`"allOf"`**  
    表示零个或多个嵌套条件的聚合。额外字段：
    - **`conditions`**：必需的条件对象数组。这些条件使用短路求值。

  - **`"not"`**  
    表示另一个条件的反转。额外字段：
    - **`condition`**：必需的条件对象。

---

### 宏展开

如上所述，某些字段支持宏展开。宏的形式为 `$<macro-namespace>{<macro-name>}`。所有宏都在使用预设的上下文中求值，即使宏位于从另一个预设继承的字段中。例如，如果 `Base` 预设将变量 `PRESET_NAME` 设置为 `${presetName}`，而 `Derived` 预设继承自 `Base`，则 `PRESET_NAME` 将被设置为 `Derived`。  
宏名末尾没有右大括号是错误的（例如 `${sourceDir` 无效）。美元符号后跟除左大括号（带可能命名空间）之外的任何内容均解释为字面美元符号。

识别的宏包括：

- **`${sourceDir}`**  
  项目源目录的路径（与 [`CMAKE_SOURCE_DIR`](../variable/CMAKE_SOURCE_DIR.html) 相同）。

- **`${sourceParentDir}`**  
  项目源目录的父目录路径。

- **`${sourceDirName}`**  
  `${sourceDir}` 的最后一个文件名组件。例如，如果 `${sourceDir}` 是 `/path/to/source`，则此宏为 `source`。

- **`${presetName}`**  
  预设的 `name` 字段中指定的名称。  
  这是一个预设特定的宏。

- **`${generator}`**  
  预设的 `generator` 字段中指定的生成器。对于构建和测试预设，将求值为 `configurePreset` 指定的生成器。  
  这是一个预设特定的宏。

- **`${hostSystemName}`**  
  主机操作系统的名称。包含与 [`CMAKE_HOST_SYSTEM_NAME`](../variable/CMAKE_HOST_SYSTEM_NAME.html) 相同的值。在指定版本 `3` 或以上的预设文件中允许。

- **`${fileDir}`**  
  包含该宏的预设文件所在目录的路径。在指定版本 `4` 或以上的预设文件中允许。

- **`${dollar}`**  
  字面美元符号（`$`）。

- **`${pathListSep}`**  
  用于分隔路径列表的本机字符，例如 `:` 或 `;`。  
  例如，将 `PATH` 设置为 `/path/to/ninja/bin${pathListSep}$env{PATH}`，`${pathListSep}` 将扩展为底层操作系统用于 `PATH` 连接的字符。在指定版本 `5` 或以上的预设文件中允许。

- **`$env{<variable-name>}`**  
  名为 `<variable-name>` 的环境变量。变量名不能为空字符串。如果该变量在 `environment` 字段中定义，则使用该值而不是父环境的值。如果环境变量未定义，则求值为空字符串。  
  注意：Windows 环境变量名不区分大小写，但预设中的变量名仍然区分大小写。为获得最佳结果，请保持环境变量名的大小写一致。

- **`$penv{<variable-name>}`**  
  类似于 `$env{<variable-name>}`，但值仅来自父环境，绝不来自 `environment` 字段。这允许对现有环境变量进行前置或追加。例如，将 `PATH` 设置为 `/path/to/ninja/bin:$penv{PATH}` 会将 `/path/to/ninja/bin` 前置到 `PATH` 环境变量。这是因为 `$env{<variable-name>}` 不允许循环引用。

- **`$vendor{<macro-name>}`**  
  供供应商插入其自己宏的扩展点。CMake 无法使用包含 `$vendor{<macro-name>}` 宏的预设，实际上会忽略此类预设。但是，它仍然可以使用同一文件中的其他预设。  
  CMake 不尝试解释 `$vendor{<macro-name>}` 宏。然而，为避免名称冲突，IDE 供应商应使用非常短（最好 ≤4 个字符）的供应商标识符前缀后跟 `.` 再跟宏名来为 `<macro-name>` 添加前缀。例如，示例 IDE 可以有 `$vendor{xide.ideInstallDir}`。

---

## 版本

`CMakePresets.json` 和 `CMakeUserPresets.json` 的 JSON 模式遵循版本方案，新版本被添加并允许在较新版本的 CMake 中使用。

下面列出了支持的版本、添加该版本的 CMake 版本以及新功能和更改的摘要。

- **`1`**  
  *添加于版本 3.19。*  
  初始版本支持 [配置预设](#configure-preset) 和 [宏展开](#macro-expansion)。

- **`2`**  
  *添加于版本 3.20。*  
  - 添加了 [构建预设](#build-preset)。  
  - 添加了 [测试预设](#test-preset)。

- **`3`**  
  *添加于版本 3.21。*  
  - 为 [配置](#configure-preset)、[构建](#build-preset) 和 [测试预设](#test-preset) 添加了 [Condition](#condition) 对象。  
  - 对配置预设的更改：
    - 添加了 `installDir` 字段。
    - 添加了 `toolchainFile` 字段。
    - `binaryDir` 字段现在可选。
    - `generator` 字段现在可选。
  - 对宏展开的更改：
    - 添加了 `${hostSystemName}` 宏。

- **`4`**  
  *添加于版本 3.23。*  
  - 添加了 [Includes](#includes) 以支持在 `CMakePresets.json` 和 `CMakeUserPresets.json` 中包含其他 JSON 文件。  
  - 对构建预设的更改：
    - 添加了 `resolvePackageReferences` 字段。
  - 对宏展开的更改：
    - 添加了 `${fileDir}` 宏。

- **`5`**  
  *添加于版本 3.24。*  
  - 对测试预设的更改：
    - 在 `output` 对象中添加了 `testOutputTruncation` 字段。
  - 对宏展开的更改：
    - 添加了 `${pathListSep}` 宏。

- **`6`**  
  *添加于版本 3.25。*  
  - 添加了 [打包预设](#package-preset)。  
  - 添加了 [工作流预设](#workflow-preset)。  
  - 对测试预设的更改：
    - 在 `output` 对象中添加了 `outputJUnitFile` 字段。

- **`7`**  
  *添加于版本 3.27。*  
  - 对配置预设的更改：
    - 添加了 `trace` 字段。
  - 对 Includes 的更改：
    - `include` 字段现在支持 `$penv{}` 宏展开。

- **`8`**  
  *添加于版本 3.28。*  
  - 根对象中添加了 `$schema` 字段。

- **`9`**  
  *添加于版本 3.30。*  
  - 对 Includes 的更改：
    - `include` 字段现在支持其他类型的宏展开。

- **`10`**  
  *添加于版本 3.31。*  
  - 添加了可选的 `$comment` 字段以支持在 `CMakePresets.json` 和 `CMakeUserPresets.json` 中添加文档。  
  - 对配置预设的更改：
    - 添加了 `graphviz` 字段。

- **`11`**  
  *添加于版本 4.3。*  
  - 对测试预设的更改：
    - `jobs` 字段现在接受空字符串，表示省略 `<jobs>` 的 `--parallel`。

---

## 模式

[此文件](https://cmake.org/cmake/help/latest/_downloads/3e2d73bff478d88a7de0de736ba5e361/schema.json) 提供了 `CMakePresets.json` 格式的机器可读 JSON 模式。

[./schema.json](./schema.json)
