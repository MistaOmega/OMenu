#include "platform_x11.h"
#include "../../pch.h"
#include "../../console/console.h"
#include "../../menu/menu.h"
#include "../../utils/utils.h"

// XCB not Xlib, Xlib's default error handler kills the game on BadWindow and replacing it is process wide
#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <cstdlib>
#include <cstring>

namespace {
    xcb_connection_t *gConnection = nullptr;
    bool gConnectionTried = false;
    std::chrono::steady_clock::time_point gLastFrame;
    uint8_t gPrevKeys[32] = {};

    xcb_keycode_t gMinKeycode = 0;
    uint8_t gSymsPerCode = 0;
    std::vector<xcb_keysym_t> gKeyboardMap;

    struct KeyMapping {
        xcb_keysym_t sym;
        ImGuiKey key;
    };

    const KeyMapping kKeyMap[] = {
            {XK_Tab,       ImGuiKey_Tab},
            {XK_Left,      ImGuiKey_LeftArrow},
            {XK_Right,     ImGuiKey_RightArrow},
            {XK_Up,        ImGuiKey_UpArrow},
            {XK_Down,      ImGuiKey_DownArrow},
            {XK_Prior,     ImGuiKey_PageUp},
            {XK_Next,      ImGuiKey_PageDown},
            {XK_Home,      ImGuiKey_Home},
            {XK_Delete,    ImGuiKey_Delete},
            {XK_BackSpace, ImGuiKey_Backspace},
            {XK_Return,    ImGuiKey_Enter},
            {XK_Escape,    ImGuiKey_Escape},
            {XK_space,     ImGuiKey_Space},
            {XK_Control_L, ImGuiKey_LeftCtrl},
            {XK_Control_R, ImGuiKey_RightCtrl},
            {XK_Shift_L,   ImGuiKey_LeftShift},
            {XK_Shift_R,   ImGuiKey_RightShift},
            {XK_Alt_L,     ImGuiKey_LeftAlt},
            {XK_Alt_R,     ImGuiKey_RightAlt},
            {XK_a,         ImGuiKey_A},
            {XK_c,         ImGuiKey_C},
            {XK_v,         ImGuiKey_V},
            {XK_x,         ImGuiKey_X},
            {XK_y,         ImGuiKey_Y},
            {XK_z,         ImGuiKey_Z},
    };

    xcb_keysym_t KeysymAt(int code, int level) {
        size_t index = static_cast<size_t>(code - gMinKeycode) * gSymsPerCode + level;
        if (code < gMinKeycode || level >= gSymsPerCode || index >= gKeyboardMap.size())
            return 0;
        return gKeyboardMap[index];
    }

    xcb_keycode_t KeycodeFor(xcb_keysym_t sym) {
        for (int code = gMinKeycode; code < 256; ++code) {
            if (KeysymAt(code, 0) == sym)
                return static_cast<xcb_keycode_t>(code);
        }
        return 0;
    }

    bool IsDown(const uint8_t *keys, xcb_keycode_t code) {
        return code != 0 && (keys[code / 8] & (1 << (code % 8)));
    }

    bool Pressed(const uint8_t *keys, xcb_keycode_t code) {
        return IsDown(keys, code) && !IsDown(gPrevKeys, code);
    }

    void LoadKeyboardMap() {
        const xcb_setup_t *setup = xcb_get_setup(gConnection);
        gMinKeycode = setup->min_keycode;
        auto cookie = xcb_get_keyboard_mapping(gConnection, setup->min_keycode,
                                               setup->max_keycode - setup->min_keycode + 1);
        xcb_get_keyboard_mapping_reply_t *reply = xcb_get_keyboard_mapping_reply(gConnection, cookie, nullptr);
        if (!reply)
            return;
        gSymsPerCode = reply->keysyms_per_keycode;
        xcb_keysym_t *syms = xcb_get_keyboard_mapping_keysyms(reply);
        gKeyboardMap.assign(syms, syms + xcb_get_keyboard_mapping_keysyms_length(reply));
        std::free(reply);
    }

    bool EnsureConnection() {
        if (!gConnectionTried) {
            gConnectionTried = true;
            xcb_connection_t *connection = xcb_connect(nullptr, nullptr);
            if (xcb_connection_has_error(connection)) {
                xcb_disconnect(connection);
                PRINT_ERROR("Could not connect to an X server; menu input and hotkeys are disabled.");
                return false;
            }
            gConnection = connection;
            LoadKeyboardMap();
        }
        return gConnection != nullptr;
    }

    bool QueryPointer(xcb_window_t window, ImVec2 &pos, uint16_t &mask) {
        xcb_query_pointer_reply_t *reply =
                xcb_query_pointer_reply(gConnection, xcb_query_pointer(gConnection, window), nullptr);
        if (!reply)
            return false;
        pos = ImVec2(reply->win_x, reply->win_y);
        mask = reply->mask;
        std::free(reply);
        return true;
    }

    xcb_window_t FocusedWindow() {
        xcb_get_input_focus_reply_t *reply =
                xcb_get_input_focus_reply(gConnection, xcb_get_input_focus(gConnection), nullptr);
        if (!reply)
            return XCB_NONE;
        xcb_window_t focus = reply->focus;
        std::free(reply);
        return focus == XCB_INPUT_FOCUS_POINTER_ROOT ? XCB_NONE : focus;
    }

    void FeedMouse(ImGuiIO &io, xcb_window_t window) {
        ImVec2 pos;
        uint16_t mask = 0;
        // glXCreateWindow drawables aren't X windows, so this can fail
        bool found = window != XCB_NONE && QueryPointer(window, pos, mask);
        if (!found) {
            xcb_window_t focus = FocusedWindow();
            found = focus != XCB_NONE && QueryPointer(focus, pos, mask);
        }
        if (!found)
            return;

        io.AddMousePosEvent(pos.x, pos.y);
        io.AddMouseButtonEvent(0, mask & XCB_KEY_BUT_MASK_BUTTON_1);
        io.AddMouseButtonEvent(1, mask & XCB_KEY_BUT_MASK_BUTTON_3);
        io.AddMouseButtonEvent(2, mask & XCB_KEY_BUT_MASK_BUTTON_2);
    }

    void FeedKeyboard(ImGuiIO &io, const uint8_t *keys) {
        for (const KeyMapping &mapping: kKeyMap) {
            xcb_keycode_t code = KeycodeFor(mapping.sym);
            if (IsDown(keys, code) != IsDown(gPrevKeys, code))
                io.AddKeyEvent(mapping.key, IsDown(keys, code));
        }

        bool shift = IsDown(keys, KeycodeFor(XK_Shift_L)) || IsDown(keys, KeycodeFor(XK_Shift_R));
        bool ctrl = IsDown(keys, KeycodeFor(XK_Control_L)) || IsDown(keys, KeycodeFor(XK_Control_R));
        io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
        io.AddKeyEvent(ImGuiMod_Shift, shift);

        // polling means no key repeat, a held key types once. ASCII only
        if (ctrl)
            return;
        for (int code = gMinKeycode; code < 256; ++code) {
            if (!Pressed(keys, static_cast<xcb_keycode_t>(code)))
                continue;
            xcb_keysym_t sym = KeysymAt(code, 0);
            if (shift) {
                xcb_keysym_t shifted = KeysymAt(code, 1);
                if (shifted != 0)
                    sym = shifted;
                else if (sym >= XK_a && sym <= XK_z)
                    sym -= XK_a - XK_A;
            }
            if (sym >= 0x20 && sym <= 0x7e)
                io.AddInputCharacter(sym);
        }
    }
}

namespace LinuxPlatform {
    void NewFrame(int width, int height, unsigned long window) {
        ImGuiIO &io = ImGui::GetIO();
        io.BackendPlatformName = "omenu_x11_poll";
        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

        auto now = std::chrono::steady_clock::now();
        float delta = gLastFrame.time_since_epoch().count() == 0
                      ? 0.0f
                      : std::chrono::duration<float>(now - gLastFrame).count();
        io.DeltaTime = delta > 0.0f ? delta : 1.0f / 60.0f;
        gLastFrame = now;

        if (!EnsureConnection())
            return;

        xcb_query_keymap_reply_t *keymap =
                xcb_query_keymap_reply(gConnection, xcb_query_keymap(gConnection), nullptr);
        if (!keymap)
            return;
        uint8_t keys[32];
        std::memcpy(keys, keymap->keys, sizeof(keys));
        std::free(keymap);

        if (Pressed(keys, KeycodeFor(XK_Insert)))
            Menu::showMenu = !Menu::showMenu;
        if (Pressed(keys, KeycodeFor(XK_End)))
            Utils::UnloadDLL();

        if (Menu::showMenu) {
            FeedMouse(io, static_cast<xcb_window_t>(window));
            FeedKeyboard(io, keys);
        }

        std::memcpy(gPrevKeys, keys, sizeof(keys));
    }

    void Shutdown() {
        if (gConnection) {
            xcb_disconnect(gConnection);
            gConnection = nullptr;
        }
    }
}
