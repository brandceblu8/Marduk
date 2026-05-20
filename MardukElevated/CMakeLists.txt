cmake_minimum_required(VERSION 3.21)
project(MardukElevated VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Sources ───────────────────────────────────────────────────
add_executable(MardukElevated main.cpp)

target_include_directories(MardukElevated PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../DNSManagerDll
)

# ─── Link existing DLL ────────────────────────────────────────
set(MARDUK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Release" CACHE PATH "Path to DLL import libraries")

target_link_libraries(MardukElevated PRIVATE
    ${MARDUK_LIB_DIR}/DNSManagerDll.lib
    shell32
)

# ─── Require admin via manifest ────────────────────────────────
# 这个 exe 必须以管理员身份运行（嵌入 manifest）
if(MSVC)
    set_target_properties(MardukElevated PROPERTIES
        LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
    )
endif()
