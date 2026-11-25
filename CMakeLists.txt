cmake_minimum_required(VERSION 3.23)
project(OMenu C CXX)
set(CMAKE_CXX_STANDARD 17)

set(DEPENDENCIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")

set(COMMON_SOURCES
        pch.h
        hooks/detours.h
        utils/utils.h
        utils/config.h
        console/console.cpp
        console/console.h
        console/consolecolor.cpp
        console/consolecolor.h
        menu/menu.cpp
        menu/menu.h
)

set(IMGUI_SOURCES
        ${DEPENDENCIES_DIR}/imgui/imconfig.h
        ${DEPENDENCIES_DIR}/imgui/imgui.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui.h
        ${DEPENDENCIES_DIR}/imgui/imgui_demo.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_draw.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_internal.h
        ${DEPENDENCIES_DIR}/imgui/imgui_tables.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_widgets.cpp
        ${DEPENDENCIES_DIR}/imgui/imstb_rectpack.h
        ${DEPENDENCIES_DIR}/imgui/imstb_textedit.h
        ${DEPENDENCIES_DIR}/imgui/imstb_truetype.h
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_opengl3.cpp
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_opengl3.h
        ${DEPENDENCIES_DIR}/imgui/imgui_impl_opengl3_loader.h
)

if (WIN32)
    # Windows DLL, MSVC or MinGW, MinGW covers Proton games too
    set(PLATFORM_SOURCES
            library.cpp
            library.h
            hooks/detours.cpp
            hooks/dx9/hook_dx9.cpp
            hooks/dx9/hook_dx9.h
            hooks/dx10/hook_dx10.cpp
            hooks/dx10/hook_dx10.h
            hooks/dx11/hook_dx11.cpp
            hooks/dx11/hook_dx11.h
            hooks/dx12/hook_dx12.cpp
            hooks/dx12/hook_dx12.h
            hooks/opengl/hook_opengl.cpp
            hooks/opengl/hook_opengl.h
            utils/rendering_framework.cpp
            utils/rendering_framework.h
            utils/utils.cpp
            ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx9.cpp
            ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx9.h
            ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx10.cpp
            ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx10.h
            ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx11.cpp
            ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx11.h
            ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx12.cpp
            ${DEPENDENCIES_DIR}/imgui/imgui_impl_dx12.h
            ${DEPENDENCIES_DIR}/imgui/imgui_impl_win32.cpp
            ${DEPENDENCIES_DIR}/imgui/imgui_impl_win32.h
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
    )
elseif (UNIX AND NOT APPLE)
    # Linux .so for native GL games, loaded with LD_PRELOAD
    set(PLATFORM_SOURCES
            hooks/linux/hook_gl_linux.cpp
            hooks/linux/platform_x11.cpp
            hooks/linux/platform_x11.h
            utils/utils_linux.cpp
    )
else ()
    message(FATAL_ERROR "OMenu supports Windows and Linux only")
endif ()

add_library(OMenu SHARED ${COMMON_SOURCES} ${IMGUI_SOURCES} ${PLATFORM_SOURCES})
target_compile_definitions(OMenu PRIVATE IMGUI_USER_CONFIG="my_imgui_config.h")
target_precompile_headers(OMenu PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/pch.h>"
)

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(DIRECTX_SDK_LIB_DIR "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x64")
    else ()
        set(DIRECTX_SDK_LIB_DIR "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86")
    endif ()

    # D3DX9 is optional, only bother linking if the legacy SDK is installed.
    find_library(D3DX9 NAMES d3dx9 HINTS "${DIRECTX_SDK_LIB_DIR}")
    if (D3DX9)
        message(STATUS "D3DX9 found, linking it")
        target_include_directories(OMenu PRIVATE "${DIRECTX_SDK_LIB_DIR}/../../Include")
        target_link_libraries(OMenu PRIVATE ${D3DX9})
    else ()
        message(STATUS "D3DX9 not found, skipping (install the legacy DirectX SDK if you add D3DX-based features like texture loading)")
    endif ()

    # Optional, Vulkan isn't impled yet.
    find_package(Vulkan QUIET)
    if (Vulkan_FOUND)
        message(STATUS "Vulkan SDK found, building imgui_impl_vulkan")
        target_sources(OMenu PRIVATE
                ${DEPENDENCIES_DIR}/imgui/imgui_impl_vulkan.cpp
                ${DEPENDENCIES_DIR}/imgui/imgui_impl_vulkan.h
        )
        target_link_libraries(OMenu PRIVATE Vulkan::Vulkan)
    else ()
        message(STATUS "Vulkan SDK not found, skipping imgui_impl_vulkan (install the Vulkan SDK to enable it)")
    endif ()

    # ImGui's DX backends link d3dcompiler with #pragma comment, MinGW ignores that
    target_link_libraries(OMenu PRIVATE d3d9 d3d10 dxgi d3d11 d3d12 dxguid d3dcompiler dwmapi)

    if (MINGW)
        # game folders and Proton prefixes don't ship the MinGW runtime, so link it statically
        set_target_properties(OMenu PROPERTIES PREFIX "")
        target_link_options(OMenu PRIVATE -static -static-libgcc -static-libstdc++)
    endif ()
else ()
    find_package(X11 REQUIRED)
    if (NOT X11_xcb_FOUND)
        message(FATAL_ERROR "libxcb development files are required")
    endif ()

    # hide everything so our ImGui can't clash with a game's own copy, the hooks export themselves
    set_target_properties(OMenu PROPERTIES
            C_VISIBILITY_PRESET hidden
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
    )
    target_include_directories(OMenu PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(OMenu PRIVATE X11::xcb ${CMAKE_DL_LIBS})
    target_link_options(OMenu PRIVATE -Wl,--no-undefined)
endif ()
