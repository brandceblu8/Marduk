cmake_minimum_required(VERSION 3.21)
project(ZfwInteractionDll LANGUAGES CXX)

# ─── Sources ───────────────────────────────────────────────────
set(SOURCES
    dllmain.cpp
    pch.cpp
    ZfwManager.cpp
)

set(HEADERS
    framework.h
    pch.h
    ZfwManager.h
    GumboWrapper.h
    resource.h
)

# ─── Shared Library ───────────────────────────────────────────
add_library(ZfwInteractionDll SHARED ${SOURCES} ${HEADERS})

# ─── Precompiled Header ───────────────────────────────────────
target_precompiled_headers(ZfwInteractionDll PRIVATE pch.h)

# ─── Preprocessor Definitions ─────────────────────────────────
target_compile_definitions(ZfwInteractionDll PRIVATE
    ZFWINTERACTIONDLL_EXPORTS
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

# ─── vcpkg Dependencies ──────────────────────────────────────
find_package(cpr CONFIG REQUIRED)
find_package(unofficial-gumbo CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(onnxruntime CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)

target_link_libraries(ZfwInteractionDll PRIVATE
    cpr::cpr
    unofficial::gumbo::gumbo
    nlohmann_json::nlohmann_json
    onnxruntime::onnxruntime
    ${OpenCV_LIBS}
    cryptopp::cryptopp
)

# ─── Include Directories ──────────────────────────────────────
target_include_directories(ZfwInteractionDll
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${OpenCV_INCLUDE_DIRS}
)

# ─── Compiler Options ─────────────────────────────────────────
target_compile_options(ZfwInteractionDll PRIVATE /utf-8 /W3 /permissive-)
