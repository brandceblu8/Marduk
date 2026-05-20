# Marduk —— 命令行主程序（wmain，Console 子系统）

find_package(tomlplusplus CONFIG REQUIRED)

add_executable(Marduk Marduk.cpp)

target_link_libraries(Marduk PRIVATE
    # 仓库内的 4 个 DLL —— 直接用目标名，CMake 自动处理 import lib 与
    # include 路径（PUBLIC include dirs 已传播）。
    DNSManagerDll
    NetworkDiagnosticDll
    WifiPortalDll
    ZfwInteractionDll
    # 第三方
    tomlplusplus::tomlplusplus
    # Windows 系统库
    wininet
    shell32
)

# wmain + Console 子系统：CMake 默认 add_executable 即 Console；
# MSVC 链接器在没有 main 而有 wmain 时自动选择 wmainCRTStartup。
set_target_properties(Marduk PROPERTIES
    WIN32_EXECUTABLE FALSE   # Console
)

# 部署文件：crnn_model.onnx 紧贴 exe；diagno_config.toml 进 exe/config/
add_custom_command(TARGET Marduk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/crnn_model.onnx"
        "$<TARGET_FILE_DIR:Marduk>/crnn_model.onnx"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Marduk>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/diagno_config.toml"
        "$<TARGET_FILE_DIR:Marduk>/config/diagno_config.toml"
    COMMENT "Deploying Marduk model + config"
    VERBATIM
)
