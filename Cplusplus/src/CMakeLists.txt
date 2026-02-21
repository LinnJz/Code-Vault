# file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.cxx" "*.cc")
# file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "*.h" "*.hpp" "*.hxx")

add_executable(${PROJECT_NAME} 
    main.cpp 
 "radix_sort/msvc_ppl.cpp")

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE OFF    # 在 Windows 系统中，运行时不启动控制台窗口，默认OFF
    LINK_WHAT_YOU_USE ON    # 告诉编译器不要自动剔除没有引用符号的链接库，默认OFF
)
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    setup_target_path(${PROJECT_NAME})
endif()

find_package(TBB COMPONENTS tbb tbbmalloc tbbmalloc_proxy REQUIRED HINTS ${VCPKG_CMAKE_SHARED_PATH})
find_package(benchmark REQUIRED COMPONENTS benchmark benchmark_main HINTS ${VCPKG_CMAKE_SHARED_PATH})
find_package(frozen REQUIRED HINTS ${VCPKG_CMAKE_SHARED_PATH})
find_package(magic_enum REQUIRED HINTS ${VCPKG_CMAKE_SHARED_PATH})
target_include_directories(${PROJECT_NAME} PRIVATE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    TBB::tbb
    TBB::tbbmalloc
    TBB::tbbmalloc_proxy
    benchmark::benchmark
    benchmark::benchmark_main
)

# 主程序需要的DLL列表
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MAIN_DLL_LIST
        "tbb12_debug.dll"
        "benchmark*.dll"
    )
    copy_dlls_to_target(${PROJECT_NAME} "${MAIN_DLL_LIST}" ${VCPKG_DEBUG_BIN_PATH})
else()
    set(MAIN_DLL_LIST
        "tbb12.dll"
        "benchmark*.dll"
    )
    copy_dlls_to_target(${PROJECT_NAME} "${MAIN_DLL_LIST}" ${VCPKG_BIN_PATH})
endif()

