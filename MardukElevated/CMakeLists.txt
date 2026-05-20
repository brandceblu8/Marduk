# MardukElevated —— 提权代理子进程
# 由 MardukGui 在需要管理员权限时调用 ShellExecute("runas", ...) 拉起。

add_executable(MardukElevated main.cpp)

target_link_libraries(MardukElevated PRIVATE
    DNSManagerDll
    shell32
)

set_target_properties(MardukElevated PROPERTIES
    WIN32_EXECUTABLE FALSE       # Console
)

# 嵌入要求管理员权限的 manifest（UAC level=requireAdministrator）。
if(MSVC)
    set_target_properties(MardukElevated PROPERTIES
        LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
    )
endif()
