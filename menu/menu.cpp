//
// Created by jackn on 19/09/2023.
//

#include "menu.h"
#include "../dependencies/imgui/imgui_impl_win32.h"
#include "../console/console.h"
#include "../hooks/detours.h"
#include "../utils/utils.h"

ImGuiStyle style;

namespace Menu {

    void CreateImGuiForWindow(HWND hWnd) {
        if (ImGui::GetCurrentContext())
            return;

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hWnd);

        ImGuiIO &io = ImGui::GetIO();
        io.IniFilename = io.LogFilename = nullptr;
        PRINT_CUSTOM_COLOR(11, "[IMGUI]", "Menu initialized successfully \n");
        PRINT_CUSTOM("[YourMenuName]", "ESP Loaded");
        CreateStyle();
    }

    void Render() {
        if (!showMenu)
            return;
        ImGui::SetNextWindowSize(ImVec2(520.000f, 280.000f), ImGuiCond_Once);
        ImGui::Begin("OMenu", nullptr, 58);

        ImGui::SetCursorPos(ImVec2(8.000f, 250.000f));
        if (ImGui::Button(("EXIT"), ImVec2(0.000f, 0.000f))) {
            Hooks::hooked = false;
            Utils::UnloadDLL();
        }
        ImGui::End();
    }

    void CreateStyle() {
        // RGBA
        style.Colors[ImGuiCol_Text] = ImColor(255, 248, 242, 255);  // ImVec4(0.988f, 0.976f, 0.953f, 1.00f)
        style.Colors[ImGuiCol_TextDisabled] = ImColor(128, 128, 128, 255);  // ImVec4(0.50f, 0.50f, 0.50f, 1.00f)
        style.Colors[ImGuiCol_WindowBg] = ImColor(44, 41, 41, 255);  // ImVec4(0.172f, 0.160f, 0.160f, 1.00f)
        style.Colors[ImGuiCol_ChildBg] = ImColor(44, 41, 41, 255);  // ImVec4(0.172f, 0.160f, 0.160f, 1.00f)
        style.Colors[ImGuiCol_PopupBg] = ImColor(44, 41, 41, 255);  // ImVec4(0.172f, 0.160f, 0.160f, 1.00f)
        style.Colors[ImGuiCol_Border] = ImColor(110, 110, 128, 128);  // ImVec4(0.43f, 0.43f, 0.50f, 0.50f)
        style.Colors[ImGuiCol_BorderShadow] = ImColor(0, 0, 0, 0);  // ImVec4(0.00f, 0.00f, 0.00f, 0.00f)
        style.Colors[ImGuiCol_FrameBg] = ImColor(64, 64, 64, 255);  // ImVec4(0.25f, 0.25f, 0.25f, 1.00f)
        style.Colors[ImGuiCol_FrameBgHovered] = ImColor(97, 97, 97, 255);  // ImVec4(0.38f, 0.38f, 0.38f, 1.00f)
        style.Colors[ImGuiCol_FrameBgActive] = ImColor(171, 171, 171, 99);  // ImVec4(0.67f, 0.67f, 0.67f, 0.39f)
        style.Colors[ImGuiCol_TitleBg] = ImColor(21, 21, 23, 255);  // ImVec4(0.08f, 0.08f, 0.09f, 1.00f)
        style.Colors[ImGuiCol_TitleBgActive] = ImColor(21, 21, 23, 255);  // ImVec4(0.08f, 0.08f, 0.09f, 1.00f)
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(0, 0, 0, 130);  // ImVec4(0.00f, 0.00f, 0.00f, 0.51f)
        style.Colors[ImGuiCol_MenuBarBg] = ImColor(36, 36, 36, 255);  // ImVec4(0.14f, 0.14f, 0.14f, 1.00f)
        style.Colors[ImGuiCol_ScrollbarBg] = ImColor(5, 5, 5, 135);  // ImVec4(0.02f, 0.02f, 0.02f, 0.53f)
        style.Colors[ImGuiCol_ScrollbarGrab] = ImColor(79, 79, 79, 255);  // ImVec4(0.31f, 0.31f, 0.31f, 1.00f)
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(105, 105, 105,
                                                              255);  // ImVec4(0.41f, 0.41f, 0.41f, 1.00f)
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(130, 130, 130, 255);  // ImVec4(0.51f, 0.51f, 0.51f, 1.00f)
        style.Colors[ImGuiCol_CheckMark] = ImColor(28, 163, 235, 255);  // ImVec4(0.11f, 0.64f, 0.92f, 1.00f)
        style.Colors[ImGuiCol_SliderGrab] = ImColor(28, 163, 235, 255);  // ImVec4(0.11f, 0.64f, 0.92f, 1.00f)
        style.Colors[ImGuiCol_SliderGrabActive] = ImColor(21, 128, 184, 255);  // ImVec4(0.08f, 0.50f, 0.72f, 1.00f)
        style.Colors[ImGuiCol_Button] = ImColor(64, 64, 64, 255);  // ImVec4(0.25f, 0.25f, 0.25f, 1.00f)
        style.Colors[ImGuiCol_ButtonHovered] = ImColor(97, 97, 97, 255);  // ImVec4(0.38f, 0.38f, 0.38f, 1.00f)
        style.Colors[ImGuiCol_ButtonActive] = ImColor(171, 171, 171, 99);  // ImVec4(0.67f, 0.67f, 0.67f, 0.39f)
        style.Colors[ImGuiCol_Header] = ImColor(56, 56, 56, 255);  // ImVec4(0.22f, 0.22f, 0.22f, 1.00f)
        style.Colors[ImGuiCol_HeaderHovered] = ImColor(64, 64, 64, 255);  // ImVec4(0.25f, 0.25f, 0.25f, 1.00f)
        style.Colors[ImGuiCol_HeaderActive] = ImColor(171, 171, 171, 99);  // ImVec4(0.67f, 0.67f, 0.67f, 0.39f)
        style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
        style.Colors[ImGuiCol_SeparatorHovered] = ImColor(105, 107, 112, 255);  // ImVec4(0.41f, 0.42f, 0.44f, 1.00f)
        style.Colors[ImGuiCol_SeparatorActive] = ImColor(66, 149, 250, 242);  // ImVec4(0.26f, 0.59f, 0.98f, 0.95f)
        style.Colors[ImGuiCol_ResizeGrip] = ImColor(0, 0, 0, 0);  // ImVec4(0.00f, 0.00f, 0.00f, 0.00f)
        style.Colors[ImGuiCol_ResizeGripHovered] = ImColor(74, 76, 79, 170);  // ImVec4(0.29f, 0.30f, 0.31f, 0.67f)
        style.Colors[ImGuiCol_ResizeGripActive] = ImColor(66, 149, 250, 242);  // ImVec4(0.26f, 0.59f, 0.98f, 0.95f)
        style.Colors[ImGuiCol_Tab] = ImColor(21, 21, 23, 213);  // ImVec4(0.08f, 0.08f, 0.09f, 0.83f)
        style.Colors[ImGuiCol_TabHovered] = ImColor(85, 87, 89, 213);  // ImVec4(0.33f, 0.34f, 0.36f, 0.83f)
        style.Colors[ImGuiCol_TabActive] = ImColor(59, 59, 61, 255);  // ImVec4(0.23f, 0.23f, 0.24f, 1.00f)
        style.Colors[ImGuiCol_TabUnfocused] = ImColor(21, 21, 23, 255);  // ImVec4(0.08f, 0.08f, 0.09f, 1.00f)
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImColor(33, 36, 38, 255);  // ImVec4(0.13f, 0.14f, 0.15f, 1.00f)
        style.Colors[ImGuiCol_PlotLines] = ImColor(156, 156, 156, 255);  // ImVec4(0.61f, 0.61f, 0.61f, 1.00f)
        style.Colors[ImGuiCol_PlotLinesHovered] = ImColor(255, 110, 89, 255);  // ImVec4(1.00f, 0.43f, 0.35f, 1.00f)
        style.Colors[ImGuiCol_PlotHistogram] = ImColor(230, 178, 0, 255);  // ImVec4(0.90f, 0.70f, 0.00f, 1.00f)
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImColor(255, 153, 0, 255);  // ImVec4(1.00f, 0.60f, 0.00f, 1.00f)
        style.Colors[ImGuiCol_TextSelectedBg] = ImColor(66, 149, 250, 89);  // ImVec4(0.26f, 0.59f, 0.98f, 0.35f)
        style.Colors[ImGuiCol_DragDropTarget] = ImColor(28, 163, 235, 255);  // ImVec4(0.11f, 0.64f, 0.92f, 1.00f)
        style.Colors[ImGuiCol_NavHighlight] = ImColor(66, 149, 250, 255);  // ImVec4(0.26f, 0.59f, 0.98f, 1.00f)
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImColor(255, 255, 255,
                                                               179);  // ImVec4(1.00f, 1.00f, 1.00f, 0.70f)
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImColor(204, 204, 204, 51);  // ImVec4(0.80f, 0.80f, 0.80f, 0.20f)
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImColor(204, 204, 204, 89);  // ImVec4(0.80f, 0.80f, 0.80f, 0.35f)

        style.GrabRounding = style.FrameRounding = 2.3f;
        ImGui::GetStyle() = style;
    }
}
