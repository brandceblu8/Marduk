cmake_minimum_required(VERSION 3.21)
project(DNSManagerDll LANGUAGES CXX)

# ─── Sources ───────────────────────────────────────────────────
set(SOURCES
    dllmain.cpp
    pch.cpp
    DNSManager.cpp
)

set(HEADERS
    framework.h
    pch.h
    DNSManager.h
)

# ─── Shared Library ───────────────────────────────────────────
add_library(DNSManagerDll SHARED ${SOURCES} ${HEADERS})

# ─── Precompiled Header ───────────────────────────────────────
target_precompiled_headers(DNSManagerDll PRIVATE pch.h)

# ─── Preprocessor Definitions ─────────────────────────────────
target_compile_definitions(DNSManagerDll PRIVATE
    DNSMANAGERDLL_EXPORTS
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

# ─── Include Directories ──────────────────────────────────────
target_include_directories(DNSManagerDll
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# ─── Compiler Options ─────────────────────────────────────────
target_compile_options(DNSManagerDll PRIVATE /utf-8 /W3 /permissive-)

# ─── Copy Acrylic files to output directory ───────────────────
set(ACRYLIC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/AcrylicFiles/AcrylicCache.dat
    ${CMAKE_CURRENT_SOURCE_DIR}/AcrylicFiles/AcrylicConfiguration.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/AcrylicFiles/AcrylicConsole.exe
    ${CMAKE_CURRENT_SOURCE_DIR}/AcrylicFiles/AcrylicHosts.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/AcrylicFiles/AcrylicService.exe
)

add_custom_command(TARGET DNSManagerDll POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:DNSManagerDll>/AcrylicFiles"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ACRYLIC_FILES}
        "$<TARGET_FILE_DIR:DNSManagerDll>/AcrylicFiles/"
    COMMENT "Copying Acrylic DNS proxy files to output directory"
)
