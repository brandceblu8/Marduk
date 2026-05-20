# custom-x64-windows.cmake
#
# 自定义 vcpkg triplet —— 用于修复 onnxruntime 的
# "Schema error: Trying to register schema ... already registered" 问题。
#
#   vcpkg 的 onnxruntime port 依赖独立的 onnx port。onnxruntime 要求
#   onnx 必须以 -DONNX_DISABLE_STATIC_REGISTRATION=ON 构建，否则
#   onnx 与 onnxruntime 各注册一遍算子 schema，导致重复注册、
#   schema 注册表被污染，任何 .onnx 模型都会被判定为 invalid model。
#   vcpkg 的 onnx port 默认不开此选项，故需用本 triplet 注入。
#
# 这个文件 = 标准 x64-windows triplet + 针对 onnx port 的一段定制。

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# 关键定制：仅当构建 onnx 这个 port 时，注入禁用静态注册的选项。
if(PORT STREQUAL "onnx")
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS
        "-DONNX_DISABLE_STATIC_REGISTRATION=ON")
endif()
