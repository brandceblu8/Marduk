cmake_minimum_required(VERSION 3.21)
project(TestApp LANGUAGES CXX)

# ─── Sources ───────────────────────────────────────────────────
set(SOURCES
    TestApp.cpp
    dnsTestApp.cpp
    zfwTestApp.cpp
)

# ─── Console Executable ───────────────────────────────────────
add_executable(TestApp ${SOURCES})

# ─── Preprocessor Definitions ─────────────────────────────────
target_compile_definitions(TestApp PRIVATE
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

# ─── Link against project DLLs ────────────────────────────────
target_link_libraries(TestApp PRIVATE
    ZfwInteractionDll
    DNSManagerDll
)

# ─── System Libraries ─────────────────────────────────────────
target_link_libraries(TestApp PRIVATE
    shell32
)

# ─── Compiler Options ─────────────────────────────────────────
target_compile_options(TestApp PRIVATE /utf-8 /W3 /permissive-)

# ─── Copy ONNX model to output directory ──────────────────────
add_custom_command(TARGET TestApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/crnn_model.onnx"
        "$<TARGET_FILE_DIR:TestApp>/crnn_model.onnx"
    COMMENT "Copying crnn_model.onnx to TestApp output"
)
