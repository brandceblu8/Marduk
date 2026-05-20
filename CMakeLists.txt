cmake_minimum_required(VERSION 3.21)

# ───────────────────────────────────────────────────────────────────
# vcpkg toolchain: 必须在 project() 之前注入
# 命令行 -DCMAKE_TOOLCHAIN_FILE=... 或 CMakePresets 优先；
# 否则回退到仓库内子模块 vcpkg/。
# ───────────────────────────────────────────────────────────────────
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "vcpkg toolchain file")
    endif()
endif()

# 自定义 triplet —— 修复 onnxruntime 在 vcpkg 默认配置下的 schema 重复注册。
# 命令行/preset 没指定时回退到仓库内的 custom-triplets。
if(NOT DEFINED VCPKG_OVERLAY_TRIPLETS)
    set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}/custom-triplets"
        CACHE STRING "vcpkg overlay triplets")
endif()
if(NOT DEFINED VCPKG_TARGET_TRIPLET AND WIN32)
    set(VCPKG_TARGET_TRIPLET "custom-x64-windows"
        CACHE STRING "vcpkg target triplet")
endif()

project(Marduk
    VERSION 2.0.0
    DESCRIPTION "Xidian campus network diagnostic tool"
    LANGUAGES CXX
)

# RC 仅在 Windows 上启用（ZfwInteractionDll.rc 需要）
if(WIN32)
    enable_language(RC)
endif()

# ───────────────────────────────────────────────────────────────────
# 全局编译/链接选项
# ───────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# DLL 导出宏 (DNSMANAGERDLL_EXPORTS 等) 的自动定义依赖目标名 ==
# 工程名 + "_EXPORTS"，不要改目标名。
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

# 仅 Windows 平台
if(NOT WIN32)
    message(FATAL_ERROR "Marduk currently only supports Windows.")
endif()

# 全局宏：与原 .vcxproj/Directory.Build.props 保持一致
add_compile_definitions(
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WINSOCK_DEPRECATED_NO_WARNINGS
)

# MSVC 编译选项
if(MSVC)
    add_compile_options(
        /utf-8           # 源文件 UTF-8（原 Directory.Build.props 全局选项）
        /permissive-     # ConformanceMode=true
        /W3              # WarningLevel=Level3
        /MP              # 并行编译，加速整体构建
        /sdl             # SDLCheck=true
        /Zc:__cplusplus  # 让 __cplusplus 反映真实 C++ 版本，避免一些三方库误判
    )
    add_compile_options(
        $<$<CONFIG:Release>:/Gy>  # FunctionLevelLinking
        $<$<CONFIG:Release>:/Oi>  # IntrinsicFunctions
        $<$<CONFIG:Release>:/GL>  # WholeProgramOptimization
    )
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>
        /DEBUG  # 始终生成 .pdb（GenerateDebugInformation=true）
    )
endif()

# ───────────────────────────────────────────────────────────────────
# 输出目录：所有 .exe/.dll/.lib 集中到 build/bin/<Config>/
# Multi-config 生成器（VS）会自动为每种 Config 维护独立子目录。
# 这样所有可执行文件天然能找到同目录下的 DLL，省去 PATH/拷贝烦恼。
# ───────────────────────────────────────────────────────────────────
set(MARDUK_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MARDUK_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MARDUK_OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MARDUK_OUTPUT_DIR}")

# 多配置生成器下，CMAKE_*_OUTPUT_DIRECTORY_<CONFIG> 也指向同一根
foreach(_cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${_cfg}" _cfg_up)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg_up} "${MARDUK_OUTPUT_DIR}/${_cfg}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_cfg_up} "${MARDUK_OUTPUT_DIR}/${_cfg}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_cfg_up} "${MARDUK_OUTPUT_DIR}/${_cfg}")
endforeach()

# vcpkg 在 Windows 下默认开启 applocal 部署，所有依赖 DLL 会自动拷到
# 可执行文件旁边，无需手动复制 vcpkg_installed\<triplet>\bin\*.dll。
# 但 NDK/CI 安全起见：仍保留 CI 中的兜底拷贝步骤。
set(VCPKG_APPLOCAL_DEPS ON CACHE BOOL "" FORCE)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON CACHE BOOL "" FORCE)

# ───────────────────────────────────────────────────────────────────
# 子项目
# ───────────────────────────────────────────────────────────────────

# 库（DLL）—— 顺序无关，但靠前列出方便阅读
add_subdirectory(DNSManagerDll)
add_subdirectory(NetworkDiagnosticDll)
add_subdirectory(WifiPortalDll)
add_subdirectory(ZfwInteractionDll)

# 可执行
add_subdirectory(Marduk)
add_subdirectory(TestApp)
add_subdirectory(MardukElevated)

# GUI（Qt）—— 可选构建：默认 ON，找不到 Qt 时关掉即可
option(MARDUK_BUILD_GUI "Build the Qt-based MardukGui front-end" ON)
if(MARDUK_BUILD_GUI)
    add_subdirectory(MardukGui)
endif()

# ───────────────────────────────────────────────────────────────────
# 摘要
# ───────────────────────────────────────────────────────────────────
message(STATUS "----- Marduk build configuration -----")
message(STATUS "  CMake version       : ${CMAKE_VERSION}")
message(STATUS "  Generator           : ${CMAKE_GENERATOR}")
message(STATUS "  C++ standard        : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Toolchain           : ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "  vcpkg triplet       : ${VCPKG_TARGET_TRIPLET}")
message(STATUS "  vcpkg overlays      : ${VCPKG_OVERLAY_TRIPLETS}")
message(STATUS "  Output directory    : ${MARDUK_OUTPUT_DIR}")
message(STATUS "  Build GUI           : ${MARDUK_BUILD_GUI}")
message(STATUS "--------------------------------------")
