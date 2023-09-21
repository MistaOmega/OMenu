#cmake_minimum_required(VERSION 3.23)
#project(OMenu)
#set(CMAKE_CXX_STANDARD 17)
#find_library(D3DX9
#    NAMES d3dx9
#    HINTS "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86"
#)
#find_package(Vulkan REQUIRED)
#include_directories("C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include")
#
# add_compile_definitions(IMGUI_USER_CONFIG="my_imgui_config.h")
#
#add_library(OMenu SHARED
#        library.cpp
#        pch.h
#        hooks/dx9/hook_dx9.cpp
#        hooks/dx9/hook_dx9.h
#        utils/rendering_backend.cpp
#        utils/rendering_backend.h
#        utils/utils.cpp
#        utils/utils.h
#        console/console.cpp
#        console/console.h
#        hooks/detours.h
#        hooks/detours.cpp
#        gh_d3d9.cpp
#        dependencies/imgui/imconfig.h
#        dependencies/imgui/imgui.cpp
#        dependencies/imgui/imgui.h
#        dependencies/imgui/imgui_demo.cpp
#        dependencies/imgui/imgui_draw.cpp
#        dependencies/imgui/imgui_impl_dx9.cpp
#        dependencies/imgui/imgui_impl_dx9.h
#        dependencies/imgui/imgui_impl_dx10.cpp
#        dependencies/imgui/imgui_impl_dx10.h
#        dependencies/imgui/imgui_impl_dx11.cpp
#        dependencies/imgui/imgui_impl_dx11.h
#        dependencies/imgui/imgui_impl_dx12.cpp
#        dependencies/imgui/imgui_impl_dx12.h
#        dependencies/imgui/imgui_impl_opengl3.cpp
#        dependencies/imgui/imgui_impl_opengl3.h
#        dependencies/imgui/imgui_impl_opengl3_loader.h
#        dependencies/imgui/imgui_impl_vulkan.cpp
#        dependencies/imgui/imgui_impl_vulkan.h
#        dependencies/imgui/imgui_impl_win32.cpp
#        dependencies/imgui/imgui_impl_win32.h
#        dependencies/imgui/imgui_internal.h
#        dependencies/imgui/imgui_tables.cpp
#        dependencies/imgui/imgui_widgets.cpp
#        dependencies/imgui/imstb_rectpack.h
#        dependencies/imgui/imstb_textedit.h
#        dependencies/imgui/imstb_truetype.h
#        dependencies/minhook/buffer.c
#        dependencies/minhook/buffer.h
#        dependencies/minhook/hook.c
#        dependencies/minhook/MinHook.h
#        dependencies/minhook/trampoline.c
#        dependencies/minhook/trampoline.h
#        dependencies/minhook/hde/hde32.c
#        dependencies/minhook/hde/hde32.h
#        dependencies/minhook/hde/hde64.c
#        dependencies/minhook/hde/hde64.h
#        dependencies/minhook/hde/pstdint.h
#        dependencies/minhook/hde/table32.h
#        dependencies/minhook/hde/table64.h
#)
#
#
#
#target_link_libraries(OMenu ${D3DX9})
#target_link_libraries(OMenu Vulkan::Vulkan)
#set(LIBS d3d9 d3d10 dxgi d3d11 d3d12)
#target_link_libraries(OMenu ${LIBS})
#target_precompile_headers(OMenu PRIVATE
#        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/pch.h>")
#
#
#
#

cmake_minimum_required(VERSION 3.23)
project(OMenu)
set(CMAKE_CXX_STANDARD 17)

# Define the DirectX SDK paths
set(DIRECTX_SDK_DIR "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)")
set(DIRECTX_LIB_DIR "${DIRECTX_SDK_DIR}/Lib/x86")
set(DIRECTX_LIB_DIR "${DIRECTX_SDK_DIR}/Lib/x64")
set(DIRECTX_INCLUDE_DIR "${DIRECTX_SDK_DIR}/Include")


# Find and link to the DirectX libraries
find_library(D3DX9
        NAMES d3dx9
        HINTS "${DIRECTX_LIB_DIR}"
)

# Find the Vulkan package (assumes you have Vulkan installed)
find_package(Vulkan REQUIRED)

# Include the DirectX SDK headers
include_directories("${DIRECTX_INCLUDE_DIR}")

# Define dependencies path
set(DEPENDENCIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")

# List of source files from the main project
set(SOURCES
        library.cpp
        pch.h
        hooks/dx9/hook_dx9.cpp
        hooks/dx9/hook_dx9.h
        utils/rendering_framework.cpp
        utils/rendering_framework.h
        utils/utils.cpp
        utils/utils.h
        console/console.cpp
        console/console.h
        hooks/detours.h
        hooks/detours.cpp
        library.h
)

# ImGui source files
set(IMGUI_SOURCES
        ${DEPENDENCIES_DIR}/imgui/imconfig.h
        ${DEPENDENCIES_DIR}/imgui/imgui.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui.h
        ${DEPENDENCIES_DIR}/imgui/imgui_demo.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_draw.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx9.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx9.h
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx10.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx10.h
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx11.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx11.h
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx12.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx12.h
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_opengl3.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_opengl3.h
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_opengl3_loader.h
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_vulkan.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_vulkan.h
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_win32.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_win32.h
        ${DEPENDENCIES_DIR}/imgui/imgui_internal.h
        ${DEPENDENCIES_DIR}/imgui/imgui_tables.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_widgets.cpp
        ${DEPENDENCIES_DIR}/imgui/imstb_rectpack.h
        ${DEPENDENCIES_DIR}/imgui/imstb_textedit.h
        ${DEPENDENCIES_DIR}/imgui/imstb_truetype.h
)

# MinHook source files
set(MINHOOK_SOURCES
        ${DEPENDENCIES_DIR}/minhook/buffer.c
        ${DEPENDENCIES_DIR}/minhook/buffer.h
        ${DEPENDENCIES_DIR}/minhook/hook.c
        ${DEPENDENCIES_DIR}/minhook/MinHook.h
        ${DEPENDENCIES_DIR}/minhook/trampoline.c
        ${DEPENDENCIES_DIR}/minhook/trampoline.h
        ${DEPENDENCIES_DIR}/minhook/hde/hde32.c
        ${DEPENDENCIES_DIR}/minhook/hde/hde32.h
        ${DEPENDENCIES_DIR}/minhook/hde/hde64.c
        ${DEPENDENCIES_DIR}/minhook/hde/hde64.h
        ${DEPENDENCIES_DIR}/minhook/hde/pstdint.h
        ${DEPENDENCIES_DIR}/minhook/hde/table32.h
        ${DEPENDENCIES_DIR}/minhook/hde/table64.h
        hooks/dx12/hook_dx12.cpp
        hooks/dx12/hook_dx12.h
        menu/menu.cpp
        menu/menu.h
        utils/config.h
        console/consolecolor.cpp
        console/consolecolor.h
        hooks/opengl/hook_opengl.cpp
        hooks/opengl/hook_opengl.h
)

# Create the shared library
add_library(OMenu SHARED ${SOURCES} ${IMGUI_SOURCES} ${MINHOOK_SOURCES})

# Link libraries
set(LIBS d3d9 d3d10 dxgi d3d11 d3d12 dxguid)
target_link_libraries(OMenu ${D3DX9})
target_link_libraries(OMenu Vulkan::Vulkan)
target_link_libraries(OMenu ${LIBS})

ADD_COMPILE_DEFINITIONS(IMGUI_USER_CONFIG="my_imgui_config.h")

# Precompile headers
target_precompile_headers(OMenu PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/pch.h>"
)
