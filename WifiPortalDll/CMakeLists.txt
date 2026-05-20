cmake_minimum_required(VERSION 3.21)
project(WifiPortalDll LANGUAGES CXX)

# ─── Sources ───────────────────────────────────────────────────
set(SOURCES
    dllmain.cpp
    pch.cpp
    WifiPortal.cpp
)

set(HEADERS
    framework.h
    pch.h
    WifiPortal.h
)

# ─── Shared Library ───────────────────────────────────────────
add_library(WifiPortalDll SHARED ${SOURCES} ${HEADERS})

# ─── Precompiled Header ───────────────────────────────────────
target_precompiled_headers(WifiPortalDll PRIVATE pch.h)

# ─── Preprocessor Definitions ─────────────────────────────────
target_compile_definitions(WifiPortalDll PRIVATE
    WIFIPORTALDLL_EXPORTS
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

# ─── vcpkg Dependencies ──────────────────────────────────────
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)

target_link_libraries(WifiPortalDll PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
    cryptopp::cryptopp
)

# ─── Include Directories ──────────────────────────────────────
target_include_directories(WifiPortalDll
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# ─── Compiler Options ─────────────────────────────────────────
target_compile_options(WifiPortalDll PRIVATE /utf-8 /W3 /permissive-)
