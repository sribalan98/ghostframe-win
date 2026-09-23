#pragma once

#include "imgui.h"

namespace GuiTheme {

// =============================================================================
// Spotify-Inspired Dark Color Palette
// =============================================================================
inline const ImVec4 ColSpotifyBlack         = ImVec4(0.07f, 0.07f, 0.07f, 1.00f); // #121212 (Main canvas background)
inline const ImVec4 ColSpotifySidebar       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f); // #000000 (Pure black sidebar)
inline const ImVec4 ColSpotifyCard          = ImVec4(0.09f, 0.09f, 0.09f, 1.00f); // #181818 (Elevated cards & rows)
inline const ImVec4 ColSpotifyCardHover     = ImVec4(0.16f, 0.16f, 0.16f, 1.00f); // #282828 (Hover state)
inline const ImVec4 ColSpotifyInput         = ImVec4(0.14f, 0.14f, 0.14f, 1.00f); // #242424 (Search & text fields)
inline const ImVec4 ColSpotifyBorder        = ImVec4(0.20f, 0.20f, 0.20f, 0.65f); // #333333 (Subtle borders)
inline const ImVec4 ColSpotifyBorderFocus   = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);

// Spotify Signature Green
inline const ImVec4 ColSpotifyGreen         = ImVec4(0.11f, 0.73f, 0.33f, 1.00f); // #1DB954
inline const ImVec4 ColSpotifyGreenHover    = ImVec4(0.12f, 0.84f, 0.38f, 1.00f); // #1ED760
inline const ImVec4 ColSpotifyGreenActive   = ImVec4(0.09f, 0.61f, 0.27f, 1.00f); // #169C46
inline const ImVec4 ColSpotifyGreenBg       = ImVec4(0.11f, 0.73f, 0.33f, 0.16f); // Soft green badge background

// Cloaked Alert Rose Red
inline const ImVec4 ColRose                 = ImVec4(0.95f, 0.25f, 0.37f, 1.00f); // #F43F5E
inline const ImVec4 ColRoseHover            = ImVec4(1.00f, 0.35f, 0.45f, 1.00f);
inline const ImVec4 ColRoseBg               = ImVec4(0.95f, 0.25f, 0.37f, 0.16f);
inline const ImVec4 ColRoseText             = ImVec4(0.99f, 0.64f, 0.69f, 1.00f);

// Solo / Focus Mode Gold Highlight
inline const ImVec4 ColSoloGold             = ImVec4(0.98f, 0.75f, 0.18f, 1.00f); // #FBBF24
inline const ImVec4 ColSoloGoldHover        = ImVec4(1.00f, 0.82f, 0.28f, 1.00f);
inline const ImVec4 ColSoloGoldBg           = ImVec4(0.98f, 0.75f, 0.18f, 0.18f);
inline const ImVec4 ColSoloGoldText         = ImVec4(1.00f, 0.88f, 0.55f, 1.00f);

// High-Contrast Clean Typography Colors
inline const ImVec4 ColTextLight            = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // #FFFFFF
inline const ImVec4 ColTextSubdued          = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // #B3B3B3
inline const ImVec4 ColSpotifySubtext       = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // #B3B3B3
inline const ImVec4 ColTextMuted            = ImVec4(0.48f, 0.48f, 0.48f, 1.00f); // #7A7A7A

// Compatibility Aliases
inline const ImVec4 ColObsidian             = ColSpotifyBlack;
inline const ImVec4 ColPanel                = ColSpotifyCard;
inline const ImVec4 ColSurface              = ColSpotifyCardHover;
inline const ImVec4 ColBorderSubtle         = ColSpotifyBorder;
inline const ImVec4 ColIndigoHover          = ColSpotifyGreen;
inline const ImVec4 ColEmerald              = ColSpotifyGreen;
inline const ImVec4 ColEmeraldBg            = ColSpotifyGreenBg;
inline const ImVec4 ColEmeraldText          = ColSpotifyGreen;
inline const ImVec4 ColCyanText             = ColTextLight;

// =============================================================================
// Theme Initialization
// =============================================================================
inline void ApplyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Window & Padding styling - Spotify Spacious & Clean Layout
    style.WindowPadding     = ImVec2(16.0f, 16.0f);
    style.FramePadding      = ImVec2(14.0f, 8.0f);
    style.ItemSpacing       = ImVec2(10.0f, 10.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.CellPadding       = ImVec2(14.0f, 12.0f); // Generous padding to prevent collisions
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 10.0f;
    style.GrabMinSize       = 10.0f;

    // Spotify Rounded Geometry (Pill aesthetic)
    style.WindowRounding    = 10.0f;
    style.ChildRounding     = 8.0f;
    style.FrameRounding     = 18.0f; // Signature pill-rounding
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding      = 10.0f;
    style.TabRounding       = 6.0f;

    // Clean Border Styling
    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;

    colors[ImGuiCol_Text]                  = ColTextLight;
    colors[ImGuiCol_TextDisabled]          = ColTextMuted;
    colors[ImGuiCol_WindowBg]              = ColSpotifyBlack;
    colors[ImGuiCol_ChildBg]               = ColSpotifyCard;
    colors[ImGuiCol_PopupBg]               = ColSpotifyCard;
    colors[ImGuiCol_Border]                = ColSpotifyBorder;
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ColSpotifyInput;
    colors[ImGuiCol_FrameBgHovered]        = ColSpotifyCardHover;
    colors[ImGuiCol_FrameBgActive]         = ColSpotifyCardHover;
    colors[ImGuiCol_TitleBg]               = ColSpotifyBlack;
    colors[ImGuiCol_TitleBgActive]         = ColSpotifyBlack;
    colors[ImGuiCol_TitleBgCollapsed]      = ColSpotifyBlack;
    colors[ImGuiCol_MenuBarBg]             = ColSpotifyCard;
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ColSpotifyBorder;
    colors[ImGuiCol_ScrollbarGrabHovered]  = ColTextMuted;
    colors[ImGuiCol_ScrollbarGrabActive]   = ColTextLight;
    colors[ImGuiCol_CheckMark]             = ColSpotifyGreen;
    colors[ImGuiCol_SliderGrab]            = ColSpotifyGreen;
    colors[ImGuiCol_SliderGrabActive]      = ColSpotifyGreenActive;
    colors[ImGuiCol_Button]                = ColSpotifyCardHover;
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_Header]                = ColSpotifyCardHover;
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ColSpotifyGreenBg;
    colors[ImGuiCol_Separator]             = ColSpotifyBorder;
    colors[ImGuiCol_SeparatorHovered]      = ColSpotifyBorderFocus;
    colors[ImGuiCol_SeparatorActive]       = ColSpotifyGreen;
    colors[ImGuiCol_ResizeGrip]            = ColSpotifyBorder;
    colors[ImGuiCol_ResizeGripHovered]     = ColSpotifyGreen;
    colors[ImGuiCol_ResizeGripActive]      = ColSpotifyGreenActive;
    colors[ImGuiCol_Tab]                   = ColSpotifyCard;
    colors[ImGuiCol_TabHovered]            = ColSpotifyCardHover;
    colors[ImGuiCol_TabActive]             = ColSpotifyGreenBg;
    colors[ImGuiCol_TabUnfocused]          = ColSpotifyCard;
    colors[ImGuiCol_TabUnfocusedActive]    = ColSpotifyCardHover;
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ColSpotifyBorder;
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.18f, 0.18f, 0.18f, 0.60f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.11f, 0.73f, 0.33f, 0.30f);
    colors[ImGuiCol_NavHighlight]          = ColSpotifyGreen;
}

// Render a sleek Stat Card
inline void RenderStatCard(const char* label, const char* value, const ImVec4& accentColor, float width = 150.0f) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ColSpotifyCard);
    ImGui::PushStyleColor(ImGuiCol_Border, ColSpotifyBorder);
    ImGui::BeginChild(label, ImVec2(width, 56), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::SetCursorPos(ImVec2(12, 8));
        ImGui::PushStyleColor(ImGuiCol_Text, ColTextSubdued);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(12, 28));
        ImGui::PushStyleColor(ImGuiCol_Text, accentColor);
        ImGui::TextUnformatted(value);
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
}

// Render pill-shaped status badge with support for Solo Target
inline void RenderStatusBadge(bool isExcluded, bool isSoloTarget = false, int id = 0) {
    ImGui::PushID(id);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 3.0f));

    if (isSoloTarget) {
        ImGui::PushStyleColor(ImGuiCol_Button, ColSoloGoldBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ColSoloGoldBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ColSoloGoldBg);
        ImGui::PushStyleColor(ImGuiCol_Text, ColSoloGoldText);
        ImGui::Button("★ SOLO TARGET##status_badge");
        ImGui::PopStyleColor(4);
    } else if (isExcluded) {
        ImGui::PushStyleColor(ImGuiCol_Button, ColRoseBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ColRoseBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ColRoseBg);
        ImGui::PushStyleColor(ImGuiCol_Text, ColRoseText);
        ImGui::Button("● CLOAKED##status_badge");
        ImGui::PopStyleColor(4);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ColSpotifyGreenBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ColSpotifyGreenBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ColSpotifyGreenBg);
        ImGui::PushStyleColor(ImGuiCol_Text, ColSpotifyGreen);
        ImGui::Button("● STREAMING##status_badge");
        ImGui::PopStyleColor(4);
    }

    ImGui::PopStyleVar(2);
    ImGui::PopID();
}

} // namespace GuiTheme
