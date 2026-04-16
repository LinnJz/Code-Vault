# -----------------------------------------------------------------------------
# Compiler-Specific Configuration(General)
# -----------------------------------------------------------------------------
if(COMPILER_MSVC_LIKE)
    message(STATUS "Set MSVC compiler options")
    # MSVC Runtime Library Configuration
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    # Debug Information Format
    # if(POLICY CMP0141)
    #     cmake_policy(SET CMP0141 NEW)
    #     # Embedded /Z7; ProgramDatabase /Zi; EditAndContinue /ZI
    #     # If you set CMAKE_MSVC_DEBUG_INFORMATION_FORMAT and then
    #     # add the /Zi /ZI options in compile_options, it will be overridden.
    #     # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug>:EditAndContinue>,$<$<CONFIG:RelWithDebInfo>:ProgramDatabase>>")
    # endif()

    # 所有 MSVC 编译选项（完整列表）
    set(MSVC_COMPILE_GENERAL_OPTIONS
        /std:c++latest        # C++20 及以上，自动启用模块支持
        /utf-8                # 字符集 UTF-8
        /permissive-          # 严格标准一致性，禁用宽松行为
        /Zc:__cplusplus       # 正确设置 __cplusplus 宏
        /Zc:wchar_t           # wchar_t 作为内置类型
        /Zc:forScope          # 标准 for 循环作用域
        /Zc:inline            # 移除未使用的函数
        /Zc:preprocessor      # 标准预处理器行为
        /W4                   # 警告级别 4
        /WX-                  # 不将警告视为错误
        /MP                   # 多进程编译
        /Gd                   # __cdecl 调用约定
        /fp:precise           # 精确浮点模型
        /FC                   # 诊断信息中显示完整路径
        /openmp:experimental  # /openmp 2.0, /openmp:experimental 2.0 simd, /openmp:llvm 3.0 simd
        /sdl                  # 安全检查
        /EHsc                 # C++ 同步异常处理模型
        /errorReport:prompt   # 错误报告模式
        /diagnostics:column   # 诊断信息格式（列号）
        /nologo               # 隐藏版权信息
        /Gm-                  # 禁用最小重建
        /arch:AVX2            # 启用 AVX2 指令集
        /wd5103               # 禁用特定警告
        /bigobj               # 支持大量段的目标文件
        # /ifcOutput ${CMAKE_BINARY_DIR}/  # 模块接口输出目录（按需启用）
        # /await:strict        # 协程支持（VS 2026 及以后）
    )

    # 必须传播给使用者的选项（PUBLIC / INTERFACE）
    set(PUBLIC_MSVC_COMPILE_OPTIONS
        /std:c++latest
        /utf-8
        /permissive-
        /Zc:__cplusplus
        /Zc:wchar_t
        /Zc:forScope
        /Zc:preprocessor
        /Gd                       # 调用约定影响符号修饰
        /fp:precise               # 浮点行为影响内联计算
        /EHsc                     # 异常处理模型必须匹配
        /arch:AVX2                # 若头文件使用 AVX2 intrinsic
        /wd5103
    )
    if(COMPILER_CLANG_CL)
        list(REMOVE_ITEM MSVC_COMPILE_GENERAL_OPTIONS
            /std:c++latest
            /Zc:preprocessor
            /MP
            /Gm-
        )
        list(REMOVE_ITEM PUBLIC_MSVC_COMPILE_OPTIONS
            /std:c++latest
            /Zc:preprocessor
        )
    endif()

    # Derive private MSVC options by removing public ones from the full list
    set(PRIVATE_MSVC_COMPILE_OPTIONS ${MSVC_COMPILE_GENERAL_OPTIONS})
    list(REMOVE_ITEM PRIVATE_MSVC_COMPILE_OPTIONS ${PUBLIC_MSVC_COMPILE_OPTIONS})
    
    # Apply public MSVC options to the target
    target_compile_options(${PROJECT_NAME} PUBLIC
        "$<$<CXX_COMPILER_ID:MSVC,Clang>:${PUBLIC_MSVC_COMPILE_OPTIONS}>"
    )

    # Apply private MSVC options to the target
    target_compile_options(${PROJECT_NAME} PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC,Clang>:${PRIVATE_MSVC_COMPILE_OPTIONS}>"
    )

    # Compiler definitions (all private)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        _UNICODE
        UNICODE
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )
else()
    # GCC/Clang Configuration
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Found ccache program: ${CCACHE_PROGRAM}")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM})
    endif()

    # GCC/Clang General Compilation Options (equivalent to MSVC)
    set(GCC_CLANG_COMPILE_GENERAL_OPTIONS
        -std=c++${CMAKE_CXX_STANDARD}
        # Character set (equivalent to /utf-8)
        -finput-charset=UTF-8
        -fexec-charset=UTF-8
        
        # Strict standard compliance (equivalent to /permissive-)
        -pedantic
        
        # Correct __cplusplus macro (equivalent to /Zc:__cplusplus)
        # Handled by -std=c++20
        
        # Warning levels (equivalent to /W4)
        -Wall
        -Wextra
        -Wshadow
        -Wconversion
        -Wformat-security
        -Wdouble-promotion
        -Wpedantic
        
        # Treat warnings as errors (equivalent to /WX- but we enable it)
        -Werror
        
        # Floating point precision (equivalent to /fp:precise)
        -fno-fast-math
        
        # OpenMP support (equivalent to /openmp:experimental)
        -fopenmp
        
        # Security checks (equivalent to /sdl)
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
        
        # Exception handling (equivalent to /EHsc, default in GCC/Clang)
        -fexceptions
        
        # Diagnostic format (equivalent to /diagnostics:column)
        -fdiagnostics-color=always
        -fdiagnostics-show-option
        
        # AVX2 instruction set (equivalent to /arch:AVX2)
        -mavx2
        -mfma
    )

    # 必须传播给使用者的选项（PUBLIC / INTERFACE）
    set(PUBLIC_GCC_CLANG_COMPILE_OPTIONS
        -finput-charset=UTF-8    # 字符集
        -fexec-charset=UTF-8
        -pedantic                # 严格标准一致性
        -fno-fast-math           # 精确浮点模型
        -fexceptions             # 异常处理模型
        -mavx2                   # AVX2 指令集（若头文件使用 intrinsic）
        -mfma
    )

    # 仅内部使用的选项（PRIVATE）= 全部选项 - 公共选项
    set(PRIVATE_GCC_CLANG_COMPILE_OPTIONS ${GCC_CLANG_COMPILE_GENERAL_OPTIONS})
    list(REMOVE_ITEM PRIVATE_GCC_CLANG_COMPILE_OPTIONS ${PUBLIC_GCC_CLANG_COMPILE_OPTIONS})

    set(EXTRA_WARNING_SUPPRESSIONS
        # -Wno-unused-parameter
        # -Wno-unused-variable
        -Wno-unused-local-typedef
    )

    # 模块支持选项（若项目不使用模块接口，设为 PRIVATE；否则需考虑传播）
    # 这里假设项目不使用模块，故设为 PRIVATE
    # GCC/Clang Modules Support - disabled as project doesn't use modules
    if(COMPILER_GCC)
        message(STATUS "Set GCC compiler options")
        set(MODULE_SUPPORT_OPTIONS
            -fmodules-ts
        )
    elseif(COMPILER_CLANG)
        message(STATUS "Set Clang compiler options")
        set(MODULE_SUPPORT_OPTIONS
            -fmodules
            -fbuiltin-module-map
            -fimplicit-module-maps
            -fprebuilt-module-path=${CMAKE_CURRENT_BINARY_DIR}
        )
    endif()

    # Apply GCC/Clang general options (all private)
    target_compile_options(${PROJECT_NAME} PUBLIC
        "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${PUBLIC_GCC_CLANG_COMPILE_OPTIONS}>"
    )
    target_compile_options(${PROJECT_NAME} PRIVATE
        "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${PRIVATE_GCC_CLANG_COMPILE_OPTIONS}>"
    )

    # Additional warning suppressions (private)
    target_compile_options(${PROJECT_NAME} PRIVATE
        "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${PRIVATE_GCC_CLANG_COMPILE_OPTIONS}>"
        "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${EXTRA_WARNING_SUPPRESSIONS}>"
        "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${MODULE_SUPPORT_OPTIONS}>"
    )

    target_link_options(-fopenmp)
endif()

# Stack size linker options (private)
if(COMPILER_MSVC_LIKE)
    add_link_options("/STACK:4194304")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_link_options("-Wl,-z,stack-size=4194304")
endif()

# -----------------------------------------------------------------------------
# Build Type Configuration
# -----------------------------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Build Type: Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG _DEBUG)
    
    if(COMPILER_MSVC_LIKE)
        # MSVC Debug compilation options (private)
        target_compile_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Debug>>:/JMC>"    # Just My Code debugging
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Debug>>:/Zi>"     # Edit and Continue
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Debug>>:/Od>"     # Disable optimization
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Debug>>:/RTC1>"   # Runtime error checks
        )
        
        # MSVC Debug link options (private)
        target_link_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Debug>>:/DEBUG>"
        )
    else()
        # GCC/Clang Debug compilation options (private)
        target_compile_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-g>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-O0>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-fno-omit-frame-pointer>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-fno-inline>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-fno-optimize-sibling-calls>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-fno-eliminate-unused-debug-types>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-fno-common>"
            #"$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-fno-strict-aliasing>"
        )
        
        # GCC/Clang Debug link options (private)
        target_link_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-g>"
        )
    endif()
    
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Build Type: Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG _NDEBUG)
    
    if(COMPILER_MSVC_LIKE)
        # MSVC Release compilation options (private)
        target_compile_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Release>>:/O2>"    # Maximize speed
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Release>>:/Ob2>"   # Inline any suitable function
            "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/GL>"    # Whole program optimization
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Release>>:/GF>"
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Release>>:/GS>"    # Buffer security check
            "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Qpar>"  # Auto-parallelization
            "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Qpar-report:2>"
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Release>>:/Gy>"    # Function-level linking
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Release>>:/Oi>"    # Intrinsic functions
        )
        
        # MSVC Release link options (private)
        target_link_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Release>>:/LTCG>"  # Link-time code generation
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:Release>>:/DEBUG:NONE>"
        )
    else()
        # GCC/Clang Release compilation options (private)
        target_compile_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-O3>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-flto>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-fmerge-constants>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-finline-functions>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-funroll-loops>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-fomit-frame-pointer>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-ffunction-sections>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-fno-math-errno>"
        )
        
        # GCC/Clang Release link options (private)
        target_link_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-flto>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-s>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-Wl,--gc-sections>"
        )
    endif()
    
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(STATUS "Build Type: RelWithDebInfo")
    # No specific definitions added in original for RelWithDebInfo
    
    if(COMPILER_MSVC_LIKE)
        # MSVC RelWithDebInfo compilation options (private)
        target_compile_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:RelWithDebInfo>>:/O2>" 
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:RelWithDebInfo>>:/Ob1>"
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:RelWithDebInfo>>:/Zi>" 
            "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/GL>" 
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:RelWithDebInfo>>:/GF>"
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:RelWithDebInfo>>:/GS>" 
            "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/Qpar>"
            "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/Qpar-report:2>"
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:RelWithDebInfo>>:/Gy>" 
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:RelWithDebInfo>>:/Oi>" 
        )
        
        # MSVC RelWithDebInfo link options (private)
        target_link_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:RelWithDebInfo>>:/LTCG>"
            "$<$<AND:$<CXX_COMPILER_ID:MSVC,Clang>,$<CONFIG:RelWithDebInfo>>:/DEBUG:FULL>"
        )
    else()
        # GCC/Clang RelWithDebInfo compilation options (private)
        target_compile_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-O2>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-g>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-fno-omit-frame-pointer>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-flto>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-funroll-loops>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-fmerge-constants>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-finline-functions-called-once>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-ffunction-sections>"
        )
        
        # GCC/Clang RelWithDebInfo link options (private)
        target_link_options(${PROJECT_NAME} PRIVATE
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-flto>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-g>"
            "$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:RelWithDebInfo>>:-Wl,--gc-sections>"
        )
    endif()
    
else()
    message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
endif()

# -----------------------------------------------------------------------------
# Sanitizers Configuration
# -----------------------------------------------------------------------------
if(USE_SANITIZERS)
    if(COMPILER_CLANG OR COMPILER_GCC)
        target_compile_options(${PROJECT_NAME} PRIVATE
            "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fsanitize=address>"
            "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fsanitize=undefined>"
            "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fno-omit-frame-pointer>"
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fsanitize=address>"
            "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fsanitize=undefined>"
        )
        message(STATUS "Sanitizers enabled")
    else()
        message(WARNING "Sanitizers only supported for Clang/GCC")
    endif()
endif()


if (MINGW)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif ()
if (COMPILER_MSVC_LIKE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        DEBUG_POSTFIX "d"
        COMPILE_PDB_NAME ${PROJECT_NAME}
        COMPILE_PDB_NAME_DEBUG ${PROJECT_NAME}d
    )
endif()