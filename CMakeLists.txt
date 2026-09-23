cmake_minimum_required(VERSION 3.20)
project(ghostframe VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Unicode on Windows
add_compile_definitions(UNICODE _UNICODE)

# MSVC specific flags
if(MSVC)
    add_compile_options(/W4 /EHsc /utf-8)
endif()

# Set output directories so helper, payload, GUI, and test tool are in the same folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(BUILD_TEST_CLI "Build standalone interactive test utility" ON)
option(BUILD_OBS_PLUGIN "Build OBS Studio plugin module" ON)
option(BUILD_IMGUI_GUI "Build Dear ImGui GUI Application" ON)

# ==============================================================================
# 1. Target: ghostframe-payload64 (DLL injected into target apps)
# ==============================================================================
add_library(ghostframe-payload64 SHARED
    src/helper/payload.hpp
    src/helper/payload.cpp
)
target_compile_definitions(ghostframe-payload64 PRIVATE HIDDER_PAYLOAD_EXPORTS)
target_link_libraries(ghostframe-payload64 PRIVATE user32 kernel32)

# ==============================================================================
# 2. Target: ghostframe-helper64 (Process injector executable)
# ==============================================================================
add_executable(ghostframe-helper64
    src/helper/helper-main.cpp
    src/helper/payload.hpp
)
target_link_libraries(ghostframe-helper64 PRIVATE user32 kernel32 advapi32)
add_dependencies(ghostframe-helper64 ghostframe-payload64)

# ==============================================================================
# 3. Target: ghostframe-test (Standalone interactive test verification tool)
# ==============================================================================
if(BUILD_TEST_CLI)
    add_executable(ghostframe-test
        src/test/test-main.cpp
        src/core/window-info.hpp
        src/core/window-manager.hpp
        src/core/window-manager.cpp
        src/core/affinity-controller.hpp
        src/core/affinity-controller.cpp
        src/core/config-manager.hpp
        src/core/config-manager.cpp
    )
    target_link_libraries(ghostframe-test PRIVATE user32 dwmapi shell32 kernel32)
    add_dependencies(ghostframe-test ghostframe-helper64 ghostframe-payload64)
endif()

# ==============================================================================
# 4. Target: ghostframe-gui (Professional Dear ImGui + DirectX 11 Application)
# ==============================================================================
if(BUILD_IMGUI_GUI)
    set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
    if(EXISTS "${IMGUI_DIR}/imgui.h")
        message(STATUS "Found Dear ImGui at ${IMGUI_DIR}. Configuring 'ghostframe-gui'...")

        set(IMGUI_SOURCES
            ${IMGUI_DIR}/imgui.cpp
            ${IMGUI_DIR}/imgui_draw.cpp
            ${IMGUI_DIR}/imgui_tables.cpp
            ${IMGUI_DIR}/imgui_widgets.cpp
            ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
            ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
        )

        add_executable(ghostframe-gui WIN32
            src/gui/gui-main.cpp
            src/gui/gui-theme.hpp
            src/gui/ghostframe.rc
            src/core/window-info.hpp
            src/core/window-manager.hpp
            src/core/window-manager.cpp
            src/core/affinity-controller.hpp
            src/core/affinity-controller.cpp
            src/core/config-manager.hpp
            src/core/config-manager.cpp
            ${IMGUI_SOURCES}
        )

        target_include_directories(ghostframe-gui PRIVATE
            src/
            resources/
            src/icon/
            ${CMAKE_CURRENT_SOURCE_DIR}/resources
            ${CMAKE_CURRENT_SOURCE_DIR}/src/icon
            ${IMGUI_DIR}
            ${IMGUI_DIR}/backends
        )

        target_link_libraries(ghostframe-gui PRIVATE
            d3d11
            dxgi
            d3dcompiler
            dwmapi
            user32
            gdi32
            shell32
            kernel32
        )

        add_dependencies(ghostframe-gui ghostframe-helper64 ghostframe-payload64)
    else()
        message(STATUS "Dear ImGui not found in third_party/imgui yet. It will be downloaded automatically by build.ps1.")
    endif()
endif()

# ==============================================================================
# 5. Target: ghostframe (OBS Studio Plugin Module)
# ==============================================================================
if(BUILD_OBS_PLUGIN)
    # Search for Qt6 (or Qt5 fallback)
    find_package(Qt6 COMPONENTS Widgets Core Gui QUIET)
    if(NOT Qt6_FOUND)
        find_package(Qt5 COMPONENTS Widgets Core Gui QUIET)
    endif()

    # Search for OBS Studio SDK
    set(OBS_STUDIO_DIR "C:/Program Files/obs-studio" CACHE PATH "Path to OBS Studio installation or SDK")
    
    find_path(LIBOBS_INCLUDE_DIR
        NAMES obs.h
        PATHS
            ${OBS_STUDIO_DIR}/include
            ${OBS_STUDIO_DIR}/../obs-studio/libobs
            "C:/obs-studio/libobs"
            ENV LIBOBS_DIR
    )

    find_library(LIBOBS_LIB
        NAMES obs libobs
        PATHS
            ${OBS_STUDIO_DIR}/bin/64bit
            ${OBS_STUDIO_DIR}/lib
            "C:/obs-studio/build/libobs"
            ENV LIBOBS_DIR
    )

    find_path(OBS_FRONTEND_INCLUDE_DIR
        NAMES obs-frontend-api.h
        PATHS
            ${OBS_STUDIO_DIR}/include
            ${OBS_STUDIO_DIR}/../obs-studio/UI/obs-frontend-api
            ENV LIBOBS_DIR
    )

    find_library(OBS_FRONTEND_LIB
        NAMES obs-frontend-api
        PATHS
            ${OBS_STUDIO_DIR}/bin/64bit
            ${OBS_STUDIO_DIR}/lib
            ENV LIBOBS_DIR
    )

    if((Qt6_FOUND OR Qt5_FOUND) AND LIBOBS_INCLUDE_DIR AND LIBOBS_LIB)
        message(STATUS "Found Qt and libobs. Building OBS Studio plugin 'ghostframe'...")

        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        set(CMAKE_AUTOUIC ON)

        add_library(ghostframe MODULE
            src/plugin-main.cpp
            src/ui/hider-dock.hpp
            src/ui/hider-dock.cpp
            src/core/window-info.hpp
            src/core/window-manager.hpp
            src/core/window-manager.cpp
            src/core/affinity-controller.hpp
            src/core/affinity-controller.cpp
            src/core/config-manager.hpp
            src/core/config-manager.cpp
        )

        target_include_directories(ghostframe PRIVATE
            src/
            ${LIBOBS_INCLUDE_DIR}
            ${OBS_FRONTEND_INCLUDE_DIR}
        )

        if(Qt6_FOUND)
            target_link_libraries(ghostframe PRIVATE
                Qt6::Widgets Qt6::Core Qt6::Gui
            )
        else()
            target_link_libraries(ghostframe PRIVATE
                Qt5::Widgets Qt5::Core Qt5::Gui
            )
        endif()

        target_link_libraries(ghostframe PRIVATE
            ${LIBOBS_LIB}
            ${OBS_FRONTEND_LIB}
            user32 dwmapi shell32 kernel32
        )

        add_dependencies(ghostframe ghostframe-helper64 ghostframe-payload64)
    else()
        message(STATUS "Qt or libobs not found. The OBS plugin 'ghostframe' will be skipped, but 'ghostframe-gui', 'ghostframe-test', 'ghostframe-helper64', and 'ghostframe-payload64' will be built.")
    endif()
endif()
