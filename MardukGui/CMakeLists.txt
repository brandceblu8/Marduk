cmake_minimum_required(VERSION 3.21)
project(MardukGui VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ─── Qt6 ──────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)

# ─── Sources ───────────────────────────────────────────────────
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

# ─── Executable ────────────────────────────────────────────────
qt_add_executable(MardukGui WIN32 ${SOURCES})

target_include_directories(MardukGui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ─── Link against project DLL targets + Qt6 ───────────────────
target_link_libraries(MardukGui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    ZfwInteractionDll
    DNSManagerDll
    NetworkDiagnosticDll
    WifiPortalDll
    shell32
    ole32
)

# ─── Compiler Options ─────────────────────────────────────────
target_compile_options(MardukGui PRIVATE /utf-8 /W3 /permissive-)
target_compile_definitions(MardukGui PRIVATE
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

# ─── Windows-specific: DPI-awareness ─────────────────────────
if(WIN32)
    set_target_properties(MardukGui PROPERTIES
        WIN32_EXECUTABLE TRUE
        VS_DPI_AWARE "PerMonitor"
    )
endif()
