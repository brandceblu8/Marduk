cmake_minimum_required(VERSION 3.21)
project(Marduk VERSION 2.0.0 LANGUAGES CXX)

# ─── 全局设置 ──────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg 工具链（如果从命令行 cmake -DCMAKE_TOOLCHAIN_FILE=... 传入则优先用那个）
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# ─── 子项目 ───────────────────────────────────────────────────
# GUI 主程序
add_subdirectory(MardukGui)

# 提权代理子进程
add_subdirectory(MardukElevated)

# 注意：现有的 DLL 项目 (ZfwInteractionDll, DNSManagerDll, etc.)
# 仍通过 Visual Studio .sln 构建。
# 如果你后续想全部迁移到 CMake，可以在这里 add_subdirectory 它们。
