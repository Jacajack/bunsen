#include "ui.hpp"
#include "../../log.hpp"
#include <imgui_icon_font_headers/IconsFontAwesome5.h>

/**
	A cherry ImGui theme to use before I come up with my own one
	\author https://github.com/r-lyeh
*/
static void imgui_cherry_theme()
{
    // cherry colors, 3 intensities
    #define HI(v)   ImVec4(0.502f, 0.075f, 0.256f, v)
    #define MED(v)  ImVec4(0.455f, 0.198f, 0.301f, v)
    #define LOW(v)  ImVec4(0.232f, 0.201f, 0.271f, v)
    // backgrounds (@todo: complete with BG_MED, BG_LOW)
    #define BG(v)   ImVec4(0.200f, 0.220f, 0.270f, v)
    // text
    #define TEXT(v) ImVec4(0.860f, 0.930f, 0.890f, v)

    auto &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text]                  = TEXT(0.78f);
    style.Colors[ImGuiCol_TextDisabled]          = TEXT(0.28f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]         = BG( 0.8f);
    style.Colors[ImGuiCol_PopupBg]               = BG( 0.9f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]               = BG( 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = MED( 0.78f);
    style.Colors[ImGuiCol_FrameBgActive]         = MED( 1.00f);
    style.Colors[ImGuiCol_TitleBg]               = LOW( 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = HI( 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = BG( 0.75f);
    style.Colors[ImGuiCol_MenuBarBg]             = BG( 0.47f);
    style.Colors[ImGuiCol_ScrollbarBg]           = BG( 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = MED( 0.78f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = MED( 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_ButtonHovered]         = MED( 0.86f);
    style.Colors[ImGuiCol_ButtonActive]          = MED( 1.00f);
    style.Colors[ImGuiCol_Header]                = MED( 0.76f);
    style.Colors[ImGuiCol_HeaderHovered]         = MED( 0.86f);
    style.Colors[ImGuiCol_HeaderActive]          = HI( 1.00f);
    // style.Colors[ImGuiCol_Column]                = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
    // style.Colors[ImGuiCol_ColumnHovered]         = MED( 0.78f);
    // style.Colors[ImGuiCol_ColumnActive]          = MED( 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = MED( 0.78f);
    style.Colors[ImGuiCol_ResizeGripActive]      = MED( 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = TEXT(0.63f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = MED( 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = TEXT(0.63f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = MED( 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = MED( 0.43f);
    // [...]
    style.Colors[ImGuiCol_ModalWindowDimBg]  = BG( 0.73f);

    // style.WindowPadding            = ImVec2(6, 4);
    // style.WindowRounding           = 0.0f;
    // style.FramePadding             = ImVec2(5, 2);
    // style.FrameRounding            = 3.0f;
    // style.ItemSpacing              = ImVec2(7, 1);
    // style.ItemInnerSpacing         = ImVec2(1, 1);
    // style.TouchExtraPadding        = ImVec2(0, 0);
    // style.IndentSpacing            = 6.0f;
    // style.ScrollbarSize            = 12.0f;
    // style.ScrollbarRounding        = 16.0f;
    // style.GrabMinSize              = 20.0f;
    // style.GrabRounding             = 2.0f;

    // style.WindowTitleAlign.x = 0.50f;

    style.Colors[ImGuiCol_Border] = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
    style.FrameBorderSize = 0.0f;
    style.WindowBorderSize = 1.0f;
}

void bu::ui::load_theme(float r, float g, float b)
{
    imgui_cherry_theme();
    auto &style = ImGui::GetStyle();

    ImVec4 col_active_hi(1, 1, 1, 0.7);
    ImVec4 col_active_med(1, 1, 1, 0.7);
    ImVec4 col_active_low(1, 1, 1, 0.7);

    float col_h, col_s, col_v;
    
    // col_h = 0.933;
    // col_s = 0.565;
    // col_v = 0.650; //455
    // void load_theme(float h = 0.74, float s = 0.258, float v = 0.450);

    ImGui::ColorConvertRGBtoHSV(r, g, b, col_h, col_s, col_v);
    ImGui::ColorConvertHSVtoRGB(col_h, col_s, col_v * 1.00, col_active_hi.x, col_active_hi.y, col_active_hi.z);
    ImGui::ColorConvertHSVtoRGB(col_h, col_s, col_v * 0.66, col_active_med.x, col_active_med.y, col_active_med.z);
    ImGui::ColorConvertHSVtoRGB(col_h, col_s, col_v * 0.33, col_active_low.x, col_active_low.y, col_active_low.z);

    // A bit of rounding
    style.WindowRounding = 4;
    style.FrameRounding = 4;
    style.ChildRounding = 4;
    style.PopupRounding = 0;
    style.ScrollbarRounding = 3;
    style.GrabRounding = 3;
    style.TabRounding = 4;


    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.119f, 0.119f, 0.119f, 1.000f);
    style.Colors[ImGuiCol_TitleBg]  = ImVec4(0.232f, 0.201f, 0.271f, 1.000f);
    style.Colors[ImGuiCol_Border]   = ImVec4(0.000f, 0.000f, 0.000f, 0.318f);

    style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.174f, 0.174f, 0.174f, 1.000f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.214f, 0.214f, 0.214f, 1.000f);
    style.Colors[ImGuiCol_FrameBgActive]  = ImVec4(0.323f, 0.323f, 0.323f, 1.000f);

    style.Colors[ImGuiCol_Button] = ImVec4(0.214f, 0.214f, 0.214f, 1.000f);
    style.Colors[ImGuiCol_ButtonHovered] = col_active_med;
    style.Colors[ImGuiCol_ButtonActive] = col_active_hi;

    style.Colors[ImGuiCol_Tab] = ImVec4(0.000f, 0.000f, 0.000f, 0.392f);
    style.Colors[ImGuiCol_TabActive] = col_active_med;
    style.Colors[ImGuiCol_TabHovered] = col_active_med;

    style.Colors[ImGuiCol_Header] = col_active_med;
    style.Colors[ImGuiCol_HeaderHovered] = col_active_med;
    style.Colors[ImGuiCol_HeaderActive] = col_active_hi;

    style.Colors[ImGuiCol_TitleBg] = col_active_med;
    style.Colors[ImGuiCol_TitleBgActive] = col_active_hi;
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
	auto font = io.Fonts->AddFontFromFileTTF("resources/fonts/Cousine-Regular.ttf", 15);
	io.Fonts->AddFontFromFileTTF("resources/fonts/fa-regular-400.ttf", fa_size, &config, &fa_ranges[0]);
	io.Fonts->AddFontFromFileTTF("resources/fonts/fa-solid-900.ttf", fa_size, &config, &fa_ranges[0]);
    
    // Rebuild atlas and set default font
    io.Fonts->Build();
    io.FontDefault = font;

    // LOG_DEBUG << "droid sans  " << droid_sans;
    // ImGui::PushFont(droid_sans);

    LOG_INFO << "Loaded fonts!";
}