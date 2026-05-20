cmake_minimum_required(VERSION 3.21)
project(Marduk VERSION 2.0.0 LANGUAGES CXX)

# ══════════════════════════════════════════════════════════════════
# 全局设置
# ══════════════════════════════════════════════════════════════════
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 统一输出目录 — 所有 exe/dll 都放在同一目录方便运行
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ══════════════════════════════════════════════════════════════════
# DLL 子项目（先构建，因为 exe 依赖它们）
# ══════════════════════════════════════════════════════════════════
add_subdirectory(ZfwInteractionDll)
add_subdirectory(DNSManagerDll)
add_subdirectory(NetworkDiagnosticDll)
add_subdirectory(WifiPortalDll)

# ══════════════════════════════════════════════════════════════════
# EXE 子项目
# ══════════════════════════════════════════════════════════════════
# CLI 主程序
add_subdirectory(Marduk)

# 测试程序
add_subdirectory(TestApp)

# 提权代理子进程
add_subdirectory(MardukElevated)

# Qt6 GUI 主程序（可选：如果未安装 Qt6 则跳过）
find_package(Qt6 QUIET COMPONENTS Core)
if(Qt6_FOUND)
    add_subdirectory(MardukGui)
    message(STATUS "Qt6 found — MardukGui will be built")
else()
    message(STATUS "Qt6 not found — MardukGui will be skipped (install qtbase via vcpkg)")
endif()
