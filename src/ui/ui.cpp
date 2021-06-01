#include "ui.hpp"
#include "log.hpp"
#include <imgui_icon_font_headers/IconsFontAwesome5.h>

void bu::ui::load_theme(float r, float g, float b)
{
    auto &style = ImGui::GetStyle();

    ImVec4 col_active_hi(1, 1, 1, 0.7);
    ImVec4 col_active_med(1, 1, 1, 0.7);
    ImVec4 col_active_low(1, 1, 1, 0.7);

    float col_h, col_s, col_v;
    
    ImGui::ColorConvertRGBtoHSV(r, g, b, col_h, col_s, col_v);
    ImGui::ColorConvertHSVtoRGB(col_h, col_s, col_v * 1.00, col_active_hi.x, col_active_hi.y, col_active_hi.z);
    ImGui::ColorConvertHSVtoRGB(col_h, col_s, col_v * 0.66, col_active_med.x, col_active_med.y, col_active_med.z);
    ImGui::ColorConvertHSVtoRGB(col_h, col_s, col_v * 0.33, col_active_low.x, col_active_low.y, col_active_low.z);

    style.WindowTitleAlign = ImVec2(0.5, 0.5);

    // A bit of rounding
    style.WindowRounding = 3;
    style.FrameRounding = 4;
    style.ChildRounding = 3;
    style.PopupRounding = 0;
    style.ScrollbarRounding = 3;
    style.GrabRounding = 3;
    style.TabRounding = 4;

    // Windows
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.149f, 0.149f, 0.149f, 1.000f);
    style.Colors[ImGuiCol_Border]   = ImVec4(0.000f, 0.000f, 0.000f, 0.318f);

    // Titles
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.085f, 0.085f, 0.085f, 1.000f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.060f, 0.060f, 0.060f, 1.000f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = style.Colors[ImGuiCol_TitleBg];


    // Separators and resizing
    style.Colors[ImGuiCol_Separator] = ImVec4(0.2f, 0.2f, 0.2f, 1.000f);
    style.Colors[ImGuiCol_SeparatorHovered] = col_active_med;
    style.Colors[ImGuiCol_SeparatorActive] = col_active_hi;
    style.Colors[ImGuiCol_ResizeGrip] = col_active_med;
    style.Colors[ImGuiCol_ResizeGripHovered] = col_active_med;
    style.Colors[ImGuiCol_ResizeGripActive] = col_active_hi;

    // Headers
    style.Colors[ImGuiCol_Header] = col_active_med;
    style.Colors[ImGuiCol_HeaderHovered] = col_active_med;
    style.Colors[ImGuiCol_HeaderActive] = col_active_hi;

    // Frames
    style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.095f, 0.095f, 0.095f, 1.000f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.109f, 0.109f, 0.109f, 1.000f);
    style.Colors[ImGuiCol_FrameBgActive]  = ImVec4(0.244f, 0.244f, 0.244f, 1.000f);

    // Selected text
    style.Colors[ImGuiCol_TextSelectedBg] = col_active_hi;

    // Buttons
    style.Colors[ImGuiCol_Button] = ImVec4(0.214f, 0.214f, 0.214f, 1.000f);
    style.Colors[ImGuiCol_ButtonHovered] = col_active_med;
    style.Colors[ImGuiCol_ButtonActive] = col_active_hi;

    // Tabs
    style.Colors[ImGuiCol_Tab] = style.Colors[ImGuiCol_FrameBg];
    style.Colors[ImGuiCol_TabActive] = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_TabUnfocused] = style.Colors[ImGuiCol_FrameBg];
    style.Colors[ImGuiCol_TabUnfocusedActive] = style.Colors[ImGuiCol_WindowBg];
    style.Colors[ImGuiCol_TabHovered] = col_active_med;

    // Checkmarks
    style.Colors[ImGuiCol_CheckMark] = col_active_hi;

    // Sliders
    style.Colors[ImGuiCol_SliderGrab] = style.Colors[ImGuiCol_ScrollbarGrab];
    style.Colors[ImGuiCol_SliderGrabActive] = style.Colors[ImGuiCol_ScrollbarGrabActive];
}

void bu::ui::load_extra_fonts(ImGuiIO &io)
{
    io.Fonts->AddFontDefault();

    static const float fa_size = 15.f;
    static const ImWchar fa_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

    // Load FontAwesome
    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = fa_size;

    // Load DroidSans
	// auto font = io.Fonts->AddFontFromFileTTF("resources/fonts/Cousine-Regular.ttf", 15);
	auto font = io.Fonts->AddFontFromFileTTF("resources/fonts/DejaVuSans.ttf", 15);
	io.Fonts->AddFontFromFileTTF("resources/fonts/fa-regular-400.ttf", fa_size, &config, &fa_ranges[0]);
	io.Fonts->AddFontFromFileTTF("resources/fonts/fa-solid-900.ttf", fa_size, &config, &fa_ranges[0]);
    
    // Rebuild atlas and set default font
    io.Fonts->Build();
    io.FontDefault = font;

    // LOG_DEBUG << "droid sans  " << droid_sans;
    // ImGui::PushFont(droid_sans);

    LOG_INFO << "Loaded fonts!";
}