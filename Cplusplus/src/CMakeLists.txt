cmake_minimum_required(VERSION 3.28)

project(main
    VERSION 1.0.0
    DESCRIPTION ""
    HOMEPAGE_URL ""
    LANGUAGES CXX
)

FILE(GLOB Src CONFIGURE_DEPENDS *.h *.cpp *.hpp)

source_group(Src FILES ${Src})

set(PROJECT_SOURCES
    ${Src}
     ${SrcF}
)
add_executable(
    ${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

include(Dependencies.cmake)

target_include_directories(${PROJECT_NAME} PRIVATE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../stdex>
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
    LINN_PROPERTY_ENABLE_SHORT_MACROS
    LINN_SINGLETON_ENABLE_SHORT_MACROS
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE ON
    WIN32_EXECUTABLE OFF          # ON = 关闭控制台窗口（Windows GUI应用）
    LINK_WHAT_YOU_USE ON
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    VS_DEBUGGER_COMMAND "$<TARGET_FILE:${PROJECT_NAME}>"
)

# Vcpkg 的 folly 包在其 folly-targets.cmake:64 中硬编码了 INTERFACE_COMPILE_OPTIONS "/std:c++17"，
# 当 main 链接 Folly::folly 时，该 flag 被追加到编译选项末尾。MSVC 以最后一个 /std: 为准，
# 所以 /std:c++17 覆盖了前面的 /std:c++latest，使编译器实际处于 C++17 模式——std::println 需要 C++23，因此找不到。

# Fix：在 src/CMakeLists.txt 末尾添加了一个 __cxx_std_override 接口库，最后链接，
# 确保 /std:c++latest 作为最后一个 /std: flag 生效：
if(MSVC)
    add_library(__cxx_std_override INTERFACE)
    target_compile_options(__cxx_std_override INTERFACE "/std:c++latest")
    target_link_libraries(${PROJECT_NAME} PRIVATE __cxx_std_override)
endif()
