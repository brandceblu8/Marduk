cmake_minimum_required(VERSION 3.21)
project(MardukCli LANGUAGES CXX)

# ─── Sources ───────────────────────────────────────────────────
set(SOURCES
    Marduk.cpp
)

# ─── Console Executable ───────────────────────────────────────
add_executable(MardukCli ${SOURCES})

# ─── Preprocessor Definitions ─────────────────────────────────
target_compile_definitions(MardukCli PRIVATE
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

# ─── vcpkg Dependencies ──────────────────────────────────────
find_package(tomlplusplus CONFIG REQUIRED)

# ─── Link against project DLLs + system libs ──────────────────
target_link_libraries(MardukCli PRIVATE
    ZfwInteractionDll
    DNSManagerDll
    NetworkDiagnosticDll
    WifiPortalDll
    tomlplusplus::tomlplusplus
    Wininet
    shell32
    ole32
)

# ─── Compiler Options ─────────────────────────────────────────
target_compile_options(MardukCli PRIVATE /utf-8 /W3 /permissive-)

# ─── Set output name to "Marduk" (not "MardukCli") ───────────
set_target_properties(MardukCli PROPERTIES
    OUTPUT_NAME "Marduk"
)

# ─── Copy runtime files to output directory ───────────────────
# ONNX model
add_custom_command(TARGET MardukCli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/crnn_model.onnx"
        "$<TARGET_FILE_DIR:MardukCli>/crnn_model.onnx"
    COMMENT "Copying crnn_model.onnx to output"
)

# Diagnostics config
add_custom_command(TARGET MardukCli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:MardukCli>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/diagno_config.toml"
        "$<TARGET_FILE_DIR:MardukCli>/config/diagno_config.toml"
    COMMENT "Copying diagnostic config to output"
)
