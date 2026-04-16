# CMakePresets.json剖析

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

