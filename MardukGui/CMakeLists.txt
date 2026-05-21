# MardukGui —— Qt6 图形界面前端

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)

set(SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    pages/LoginPage.cpp
    pages/LoginPage.h
    pages/DnsManagerPage.cpp
    pages/DnsManagerPage.h
    pages/DiagnosticsPage.cpp
    pages/DiagnosticsPage.h
    pages/WifiPortalPage.cpp
    pages/WifiPortalPage.h
    utils/ElevationHelper.cpp
    utils/ElevationHelper.h
    utils/AsyncWorker.h
    resources/resources.qrc
)

qt_add_executable(MardukGui WIN32 ${SOURCES})

target_include_directories(MardukGui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MardukGui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    # 仓库内的 4 个 DLL —— 直接依赖 CMake target，include 路径与
    # import lib 自动通过 PUBLIC 接口传播。
    DNSManagerDll
    NetworkDiagnosticDll
    WifiPortalDll
    ZfwInteractionDll
    # Windows 系统库
    shell32
    ole32
)

set_target_properties(MardukGui PROPERTIES
    WIN32_EXECUTABLE TRUE       # /SUBSYSTEM:WINDOWS
    VS_DPI_AWARE "PerMonitor"
)

# ────────────────────────────────────────────────────────────────
# Qt 运行时部署
#
# 没有它，启动 MardukGui.exe 会弹出
#   "This application failed to start because no Qt platform plugin
#    could be initialized."
# 因为 Qt 在运行时按 <exe>/platforms/qwindows.dll 这种相对路径定位
# 平台插件，单纯 link Qt6::Widgets 不会把这些目录拷过来。
#
# 查找策略：从 Qt6::Core 这个 imported target 的实际 DLL 路径反推
# bin/ 目录，然后在附近找 windeployqt —— 这比依赖
# VCPKG_INSTALLED_DIR 等可能为空的变量更可靠。
# ────────────────────────────────────────────────────────────────
if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    get_target_property(_qt6_core_location Qt6::Core LOCATION)
    get_filename_component(_qt6_bin_dir "${_qt6_core_location}" DIRECTORY)

    find_program(MARDUK_WINDEPLOYQT_EXE
        NAMES windeployqt6 windeployqt
        HINTS
            "${_qt6_bin_dir}"
            "${_qt6_bin_dir}/../tools/Qt6/bin"
            "${_qt6_bin_dir}/../../tools/Qt6/bin"
        NO_DEFAULT_PATH
    )
    # 兜底：不限路径再搜一次（PATH、标准 Qt 安装等）
    if(NOT MARDUK_WINDEPLOYQT_EXE)
        find_program(MARDUK_WINDEPLOYQT_EXE NAMES windeployqt6 windeployqt)
    endif()

    if(MARDUK_WINDEPLOYQT_EXE)
        message(STATUS "  windeployqt : ${MARDUK_WINDEPLOYQT_EXE}")

        add_custom_command(TARGET MardukGui POST_BUILD
            COMMAND "${MARDUK_WINDEPLOYQT_EXE}"
                $<IF:$<CONFIG:Debug>,--debug,--release>
                --no-translations
                --no-compiler-runtime
                --no-system-d3d-compiler
                --no-opengl-sw
                --no-quick-import
                "$<TARGET_FILE:MardukGui>"
            COMMENT "Running windeployqt to deploy Qt plugins next to MardukGui.exe"
            VERBATIM
        )
    else()
        message(WARNING
            "windeployqt NOT found — MardukGui.exe will crash on startup.\n"
            "Workaround: manually copy platforms/qwindows.dll next to the exe,\n"
            "or set -DMARDUK_WINDEPLOYQT_EXE=<path>.")
    endif()
endif()
