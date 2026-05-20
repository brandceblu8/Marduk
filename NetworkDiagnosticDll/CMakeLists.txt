cmake_minimum_required(VERSION 3.21)
project(NetworkDiagnosticDll LANGUAGES CXX)

# ─── Sources ───────────────────────────────────────────────────
set(SOURCES
    dllmain.cpp
    pch.cpp
    NetworkDiagnostic.cpp
    HtmlTemplateManager.cpp
)

set(HEADERS
    framework.h
    pch.h
    NetworkDiagnostic.h
    HtmlTemplateManager.h
    DiagnosticHTMLTemplates.h
    SimpleTemplateProcessor.h
    ThreadPool.h
)

# ─── Shared Library ───────────────────────────────────────────
add_library(NetworkDiagnosticDll SHARED ${SOURCES} ${HEADERS})

# ─── Precompiled Header ───────────────────────────────────────
target_precompiled_headers(NetworkDiagnosticDll PRIVATE pch.h)

# ─── Preprocessor Definitions ─────────────────────────────────
target_compile_definitions(NetworkDiagnosticDll PRIVATE
    NETWORKDIAGNOSTICDLL_EXPORTS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WINSOCK_DEPRECATED_NO_WARNINGS
    UNICODE
    _UNICODE
)

# ─── Include Directories ──────────────────────────────────────
target_include_directories(NetworkDiagnosticDll
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# ─── System Libraries (Winsock, IPHLPAPI for network diagnostics) ─
target_link_libraries(NetworkDiagnosticDll PRIVATE
    ws2_32
    iphlpapi
    ole32
    oleaut32
    wbemuuid
)

# ─── Compiler Options ─────────────────────────────────────────
target_compile_options(NetworkDiagnosticDll PRIVATE /utf-8 /W3 /permissive-)
