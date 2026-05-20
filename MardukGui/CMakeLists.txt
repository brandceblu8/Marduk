cmake_minimum_required(VERSION 3.21)
project(MardukGui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ─── vcpkg / Qt6 ───────────────────────────────────────────────
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../ZfwInteractionDll
    ${CMAKE_CURRENT_SOURCE_DIR}/../DNSManagerDll
    ${CMAKE_CURRENT_SOURCE_DIR}/../NetworkDiagnosticDll
    ${CMAKE_CURRENT_SOURCE_DIR}/../WifiPortalDll
)

# ─── Link against existing DLLs ───────────────────────────────
# In Release builds these .lib files come from the solution's output directory.
# Adjust the path to match your build layout.
set(MARDUK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Release" CACHE PATH "Path to DLL import libraries")

target_link_libraries(MardukGui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    # Existing DLL import libraries
    ${MARDUK_LIB_DIR}/ZfwInteractionDll.lib
    ${MARDUK_LIB_DIR}/DNSManagerDll.lib
    ${MARDUK_LIB_DIR}/NetworkDiagnosticDll.lib
    ${MARDUK_LIB_DIR}/WifiPortalDll.lib
)

# ─── Windows-specific: manifest for DPI-awareness ─────────────
if(WIN32)
    target_link_libraries(MardukGui PRIVATE shell32 ole32)
    set_target_properties(MardukGui PROPERTIES
        WIN32_EXECUTABLE TRUE
        VS_DPI_AWARE "PerMonitor"
    )
endif()
