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
# Qt 运行时部署 —— windeployqt
#
# 没有它，启动 MardukGui.exe 会弹出
#   "This application failed to start because no Qt platform plugin
#    could be initialized."
# 因为 Qt 在运行时按 <exe>/platforms/qwindows.dll 这种相对路径定位
# 平台插件，单纯 link Qt6::Widgets 不会把这些目录拷过来。vcpkg 的
# applocal 步骤对 Qt 插件覆盖也不稳定（特别是当我们改了
# CMAKE_RUNTIME_OUTPUT_DIRECTORY 之后），所以这里直接调用 Qt 自己的
# windeployqt.exe ——qttools 这个 vcpkg port 顺手装的。
# ────────────────────────────────────────────────────────────────
if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    # vcpkg 把 windeployqt 装到 tools/Qt6/bin/ 下，find_program 也
    # 能在那里找到；同时兼容标准 Qt 安装的 bin/ 目录。
    find_program(MARDUK_WINDEPLOYQT_EXE
        NAMES windeployqt6 windeployqt
        HINTS
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/Qt6/bin"
            "${Qt6_DIR}/../../../tools/Qt6/bin"
            "${Qt6Core_DIR}/../../../tools/Qt6/bin"
            "${Qt6Core_DIR}/../../../../bin"
        DOC "windeployqt executable used to stage Qt runtime + plugins"
    )

    if(MARDUK_WINDEPLOYQT_EXE)
        message(STATUS "  windeployqt          : ${MARDUK_WINDEPLOYQT_EXE}")

        # windeployqt 在 vcpkg 部署时偶尔会被 environment.cmake 拦截。
        # 直接把它放到目标 .exe 旁边运行，让它自己去 walk 依赖。
        # 默认行为：把 Qt 核心 DLL 和 platforms/、styles/、imageformats/
        # 等插件子目录都拷到 exe 同级——正是 Qt 运行时按相对路径要找的位置。
        add_custom_command(TARGET MardukGui POST_BUILD
            COMMAND "${MARDUK_WINDEPLOYQT_EXE}"
                $<IF:$<CONFIG:Debug>,--debug,--release>
                --no-translations          # 不需要 .qm 翻译文件
                --no-compiler-runtime      # VC++ 运行库由系统/Redist 提供
                --no-system-d3d-compiler   # 不复制 d3dcompiler_47.dll
                --no-opengl-sw             # 不复制软件 OpenGL 备用渲染器
                --no-quick-import          # 没用 QtQuick / QML
                "$<TARGET_FILE:MardukGui>"
            COMMENT "Deploying Qt runtime DLLs + platform plugins next to MardukGui.exe"
            VERBATIM
        )
    else()
        message(WARNING
            "windeployqt was NOT found. MardukGui.exe will start with the error\n"
            "  'no Qt platform plugin could be initialized'\n"
            "because platforms/qwindows.dll won't be next to it. Either install\n"
            "the qttools vcpkg port (it's in vcpkg.json — re-run cmake configure)\n"
            "or set -DMARDUK_WINDEPLOYQT_EXE=<path-to-windeployqt.exe>.")
    endif()
endif()
