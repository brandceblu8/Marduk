cmake_minimum_required(VERSION 3.21)
project(MardukElevated VERSION 2.0.0 LANGUAGES CXX)

# ─── Sources ───────────────────────────────────────────────────
add_executable(MardukElevated main.cpp)

# ─── Link against DNS manager DLL target ──────────────────────
target_link_libraries(MardukElevated PRIVATE
    DNSManagerDll
    shell32
)

# ─── Compiler Options ─────────────────────────────────────────
target_compile_options(MardukElevated PRIVATE /utf-8 /W3 /permissive-)
target_compile_definitions(MardukElevated PRIVATE
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

# ─── Require admin via manifest ────────────────────────────────
if(MSVC)
    target_link_options(MardukElevated PRIVATE
        "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'"
    )
endif()
