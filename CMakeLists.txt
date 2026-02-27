cmake_minimum_required(VERSION 3.15)
project(ScrollNice VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
set(SOURCES
    src/main.cpp
    src/core/Config.cpp
    src/core/Zone.cpp
    src/core/ScrollEngine.cpp
    src/core/StateMachine.cpp
    src/platform/win/WinMouseHook.cpp
    src/platform/win/WinInputInjector.cpp
    src/platform/win/WinOverlay.cpp
    src/platform/win/WinTray.cpp
    src/platform/win/WinHotkeys.cpp
    src/platform/win/WinSettings.cpp
)

# Win32 GUI application (no console window)
add_executable(ScrollNice WIN32 ${SOURCES})

target_include_directories(ScrollNice PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor
)

target_link_libraries(ScrollNice PRIVATE
    user32
    gdi32
    shell32
    winmm
    comctl32
    dwmapi
    advapi32
)

# Definitions
target_compile_definitions(ScrollNice PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# MSVC specific
if(MSVC)
    target_compile_options(ScrollNice PRIVATE /W4 /permissive-)
    set_property(TARGET ScrollNice PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
