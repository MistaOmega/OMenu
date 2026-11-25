// Linux backend, loaded with LD_PRELOAD. no MinHook on Linux so we export the swap functions ourselves
// and win the symbol lookup, games that grab them at runtime go through dlsym or GetProcAddress so we hook those too

#include "../../pch.h"
#include "../../console/console.h"
#include "../../menu/menu.h"
#include "../detours.h"
#include "platform_x11.h"

#include <dlfcn.h>
#include <cstring>
#include <mutex>
#include <GL/glx.h>
#include <EGL/egl.h>

#define OMENU_EXPORT extern "C" __attribute__((visibility("default")))

#if defined(__has_attribute) && __has_attribute(musttail)
#define OMENU_MUSTTAIL __attribute__((musttail))
#else
#warning "no musttail here, needs GCC 15 or Clang 13. build with optimisation so dlsym still tail calls"
#define OMENU_MUSTTAIL
#endif

namespace {
    constexpr const char *kLibGL = "libGL.so.1";
    constexpr const char *kLibEGL = "libEGL.so.1";

    using DlsymFn = void *(*)(void *, const char *) noexcept;

    DlsymFn RealDlsym() {
        static DlsymFn fn = [] {
            // our dlsym hides the real one, dlvsym doesn't so grab it from there
            for (const char *version: {"GLIBC_2.34", "GLIBC_2.2.5", "GLIBC_2.17", "GLIBC_2.0"}) {
                if (void *p = dlvsym(RTLD_NEXT, "dlsym", version))
                    return reinterpret_cast<DlsymFn>(p);
            }
            std::fputs("[OMENU] [ERROR] Could not resolve the real dlsym\n", stderr);
            std::abort();
        }();
        return fn;
    }

    void *LookupInLibrary(const char *name, const char *library) {
        void *symbol = RealDlsym()(RTLD_NEXT, name);
        if (!symbol) {
            // RTLD_NEXT can't see libs the game opened with RTLD_LOCAL
            void *handle = dlopen(library, RTLD_LAZY | RTLD_NOLOAD);
            if (!handle)
                handle = dlopen(library, RTLD_LAZY);
            if (handle)
                symbol = RealDlsym()(handle, name);
        }
        return symbol;
    }

    template<typename Fn>
    Fn Real(const char *name, const char *library) {
        void *symbol = LookupInLibrary(name, library);
        if (!symbol && std::strncmp(name, "egl", 3) == 0) {
            // libglvnd's libEGL doesn't export extensions like eglSwapBuffersWithDamageKHR
            static auto getProc = reinterpret_cast<void *(*)(const char *)>(
                    LookupInLibrary("eglGetProcAddress", kLibEGL));
            if (getProc)
                symbol = getProc(name);
        }
        return reinterpret_cast<Fn>(symbol);
    }

    std::mutex gOverlayMutex;
    void *gRendererContext = nullptr;
    bool gStarted = false;
    bool gStopped = false;

    void StopOverlay(void *currentContext) {
        // GL objects can only be deleted while their context is current
        if (gRendererContext && currentContext != gRendererContext)
            return;
        if (ImGui::GetCurrentContext()) {
            if (gRendererContext)
                ImGui_ImplOpenGL3_Shutdown();
            ImGui::DestroyContext();
        }
        LinuxPlatform::Shutdown();
        gRendererContext = nullptr;
        gStopped = true;
        PRINT("Overlay shut down.");
    }

    void RenderOverlay(void *glContext, int width, int height, unsigned long window) {
        std::lock_guard<std::mutex> lock(gOverlayMutex);
        if (gStopped || !glContext || width <= 0 || height <= 0)
            return;
        if (!Hooks::Hooked) {
            StopOverlay(glContext);
            return;
        }

        if (!gStarted) {
            gStarted = true;
            DebugConsole::Alloc();
            Menu::CreateContext();
        }
        if (!gRendererContext) {
            if (!ImGui_ImplOpenGL3_Init(nullptr)) {
                PRINT_ERROR("ImGui OpenGL3 renderer failed to initialise; overlay disabled.");
                gStopped = true;
                return;
            }
            gRendererContext = glContext;
            PRINT_COLOR(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "Overlay attached to GL context %p", glContext);
        }
        // ImGui's GL objects live in the first context, so we only draw there
        if (glContext != gRendererContext)
            return;

        ImGui::GetIO().MouseDrawCursor = Menu::showMenu;
        LinuxPlatform::NewFrame(width, height, window);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        Menu::Render();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void RenderOverlayGLX(Display *display, GLXDrawable drawable) {
        static auto getCurrentContext = Real<GLXContext (*)()>("glXGetCurrentContext", kLibGL);
        static auto queryDrawable =
                Real<void (*)(Display *, GLXDrawable, int, unsigned int *)>("glXQueryDrawable", kLibGL);
        if (!getCurrentContext || !queryDrawable)
            return;

        unsigned int width = 0, height = 0;
        queryDrawable(display, drawable, GLX_WIDTH, &width);
        queryDrawable(display, drawable, GLX_HEIGHT, &height);
        RenderOverlay(getCurrentContext(), static_cast<int>(width), static_cast<int>(height), drawable);
    }

    void RenderOverlayEGL(EGLDisplay display, EGLSurface surface) {
        static auto getCurrentContext = Real<EGLContext (*)()>("eglGetCurrentContext", kLibEGL);
        static auto querySurface =
                Real<EGLBoolean (*)(EGLDisplay, EGLSurface, EGLint, EGLint *)>("eglQuerySurface", kLibEGL);
        static auto queryContext =
                Real<EGLBoolean (*)(EGLDisplay, EGLContext, EGLint, EGLint *)>("eglQueryContext", kLibEGL);
        if (!getCurrentContext || !querySurface || !queryContext)
            return;

        EGLContext context = getCurrentContext();
        EGLint clientType = 0;
        queryContext(display, context, EGL_CONTEXT_CLIENT_TYPE, &clientType);
        if (clientType != EGL_OPENGL_API) {
            // renderer is built for desktop GL, GLES needs IMGUI_IMPL_OPENGL_ES3 at compile time
            static bool warned = false;
            if (!warned) {
                warned = true;
                PRINT_ERROR("EGL context is OpenGL ES, which the overlay does not support.");
            }
            return;
        }

        EGLint width = 0, height = 0;
        querySurface(display, surface, EGL_WIDTH, &width);
        querySurface(display, surface, EGL_HEIGHT, &height);
        RenderOverlay(context, width, height, 0);
    }

    void *HookFor(const char *name);
}

OMENU_EXPORT void glXSwapBuffers(Display *display, GLXDrawable drawable) {
    static auto real = Real<void (*)(Display *, GLXDrawable)>("glXSwapBuffers", kLibGL);
    RenderOverlayGLX(display, drawable);
    real(display, drawable);
}

OMENU_EXPORT __GLXextFuncPtr glXGetProcAddressARB(const GLubyte *name) {
    static auto real = Real<__GLXextFuncPtr (*)(const GLubyte *)>("glXGetProcAddressARB", kLibGL);
    if (void *hook = HookFor(reinterpret_cast<const char *>(name)))
        return reinterpret_cast<__GLXextFuncPtr>(hook);
    return real(name);
}

OMENU_EXPORT __GLXextFuncPtr glXGetProcAddress(const GLubyte *name) {
    return glXGetProcAddressARB(name);
}

OMENU_EXPORT EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    static auto real = Real<EGLBoolean (*)(EGLDisplay, EGLSurface)>("eglSwapBuffers", kLibEGL);
    RenderOverlayEGL(display, surface);
    return real(display, surface);
}

OMENU_EXPORT EGLBoolean eglSwapBuffersWithDamageKHR(EGLDisplay display, EGLSurface surface,
                                                     const EGLint *rects, EGLint count) {
    static auto real = Real<EGLBoolean (*)(EGLDisplay, EGLSurface, const EGLint *, EGLint)>(
            "eglSwapBuffersWithDamageKHR", kLibEGL);
    RenderOverlayEGL(display, surface);
    return real(display, surface, rects, count);
}

OMENU_EXPORT EGLBoolean eglSwapBuffersWithDamageEXT(EGLDisplay display, EGLSurface surface,
                                                     const EGLint *rects, EGLint count) {
    static auto real = Real<EGLBoolean (*)(EGLDisplay, EGLSurface, const EGLint *, EGLint)>(
            "eglSwapBuffersWithDamageEXT", kLibEGL);
    RenderOverlayEGL(display, surface);
    return real(display, surface, rects, count);
}

OMENU_EXPORT __eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *name) {
    static auto real = Real<__eglMustCastToProperFunctionPointerType (*)(const char *)>("eglGetProcAddress", kLibEGL);
    if (void *hook = HookFor(name))
        return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(hook);
    return real(name);
}

OMENU_EXPORT void *dlsym(void *handle, const char *name) noexcept {
    // callers here come from other spots like MangoHud, we can't share, it'll loop
    if (handle != RTLD_NEXT) {
        if (void *hook = HookFor(name)) {
            // only return our hook if the lib actually has the symbol, keeps feature checks honest
            if (RealDlsym()(handle, name))
                return hook;
        }
    }
    // has to be a jump not a call, glibc resolves RTLD_DEFAULT and RTLD_NEXT from the return address.
    // if that points here RTLD_LOCAL libs like NVIDIA's EGL platforms break
    OMENU_MUSTTAIL return RealDlsym()(handle, name);
}

namespace {
    void *HookFor(const char *name) {
        struct Entry {
            const char *name;
            void *hook;
        };
        static const Entry kHooks[] = {
                {"glXSwapBuffers",              reinterpret_cast<void *>(&glXSwapBuffers)},
                {"glXGetProcAddress",           reinterpret_cast<void *>(&glXGetProcAddress)},
                {"glXGetProcAddressARB",        reinterpret_cast<void *>(&glXGetProcAddressARB)},
                {"eglSwapBuffers",              reinterpret_cast<void *>(&eglSwapBuffers)},
                {"eglSwapBuffersWithDamageKHR", reinterpret_cast<void *>(&eglSwapBuffersWithDamageKHR)},
                {"eglSwapBuffersWithDamageEXT", reinterpret_cast<void *>(&eglSwapBuffersWithDamageEXT)},
                {"eglGetProcAddress",           reinterpret_cast<void *>(&eglGetProcAddress)},
        };
        if (!name)
            return nullptr;
        for (const Entry &entry: kHooks) {
            if (std::strcmp(entry.name, name) == 0)
                return entry.hook;
        }
        return nullptr;
    }
}
