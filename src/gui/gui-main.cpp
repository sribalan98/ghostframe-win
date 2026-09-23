#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "gui-theme.hpp"

#include "../core/window-manager.hpp"
#include "../core/affinity-controller.hpp"
#include "../core/config-manager.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// DirectX 11 globals
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Helper for UTF-16 to UTF-8 conversion
static std::string WStringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
}

// Helper for UTF-8 to UTF-16 conversion
static std::wstring Utf8ToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstrTo[0], sizeNeeded);
    return wstrTo;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Load Application Icon from embedded resources (ID 1) with disk fallback
    HICON hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    if (!hIcon) {
        hIcon = (HICON)LoadImageW(NULL, L"resources\\ghostframe.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }
    if (!hIcon) {
        hIcon = (HICON)LoadImageW(NULL, L"src\\icon\\ghostframe.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }

    // Register Window Class
    WNDCLASSEXW wc = {
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        hInstance ? hInstance : GetModuleHandle(NULL),
        hIcon,
        NULL,
        NULL,
        NULL,
        L"GhostFrame_WindowClass",
        hIcon
    };
    RegisterClassExW(&wc);

    int windowWidth = 1260;
    int windowHeight = 820;

    // Center window on screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    HWND hwnd = CreateWindowW(
        wc.lpszClassName,
        L"GhostFrame - Stream Capture Cloaker",
        WS_OVERLAPPEDWINDOW,
        posX,
        posY,
        windowWidth,
        windowHeight,
        NULL,
        NULL,
        wc.hInstance,
        NULL
    );

    // Apply icon directly to the window and taskbar
    if (hIcon) {
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

    // Enable Immersive Dark Mode for window title bar (Windows 10 / 11)
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &darkMode, sizeof(darkMode));

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigDebugHighlightIdConflicts = false;

    // Load Crisp High-DPI Windows Segoe UI Fonts
    ImFontConfig fontCfg;
    fontCfg.OversampleH = 3;
    fontCfg.OversampleV = 3;
    fontCfg.PixelSnapH = true;

    ImFont* fontRegular = nullptr;
    ImFont* fontTitle = nullptr;
    ImFont* fontBold = nullptr;
    ImFont* fontSmall = nullptr;

    const char* segoePath = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char* segoeBoldPath = "C:\\Windows\\Fonts\\segoeuib.ttf";
    const char* arialPath = "C:\\Windows\\Fonts\\arial.ttf";

    if (GetFileAttributesA(segoePath) != INVALID_FILE_ATTRIBUTES) {
        fontRegular = io.Fonts->AddFontFromFileTTF(segoePath, 17.5f, &fontCfg);
        fontSmall   = io.Fonts->AddFontFromFileTTF(segoePath, 13.5f, &fontCfg);
        if (GetFileAttributesA(segoeBoldPath) != INVALID_FILE_ATTRIBUTES) {
            fontTitle = io.Fonts->AddFontFromFileTTF(segoeBoldPath, 22.0f, &fontCfg);
            fontBold  = io.Fonts->AddFontFromFileTTF(segoeBoldPath, 17.5f, &fontCfg);
        }
    } else if (GetFileAttributesA(arialPath) != INVALID_FILE_ATTRIBUTES) {
        fontRegular = io.Fonts->AddFontFromFileTTF(arialPath, 17.5f, &fontCfg);
    }

    if (!fontRegular) {
        io.Fonts->AddFontDefault();
    }

    // Apply Spotify Dark Theme
    GuiTheme::ApplyDarkTheme();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Core Managers
    WindowManager winMgr;
    AffinityController affinityCtrl;
    ConfigManager configMgr;

    std::vector<WindowInfo> windowsList;
    char searchFilter[128] = "";
    char newRuleBuffer[128] = "";
    int currentTab = 0; // 0: Active Windows, 1: Focus Mode, 2: Auto-Hide Rules, 3: Streaming Guide
    int filterMode = 0; // 0: All, 1: Cloaked Only, 2: Streaming Only
    bool autoRefreshEnabled = true;
    float autoRefreshInterval = 2.0f; // seconds
    std::string statusNotice = "Ready - Live Hardware DWM Cloaking Active";

    auto lastRefreshTime = std::chrono::steady_clock::now();

    // Initial enumeration
    windowsList = winMgr.GetActiveWindows();

    // Real-time WinEventHook: Zero-leak dynamic window detection
    winMgr.StartEventHook([&](HWND newHwnd, DWORD pid, const std::wstring& procName) {
        if (WindowManager::IsSystemProcess(procName)) return; // Keep Windows Taskbar & Shell safe!

        if (affinityCtrl.IsFocusModeActive()) {
            if (pid != affinityCtrl.GetFocusedPid() && pid != GetCurrentProcessId()) {
                affinityCtrl.SetWindowExcluded(newHwnd, true);
            }
        } else if (affinityCtrl.IsProcessExcluded(pid) || configMgr.ShouldAutoExclude(procName)) {
            affinityCtrl.SetWindowExcluded(newHwnd, true);
        }
    });

    // Main loop
    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }
        if (done) break;

        // Auto-refresh timer check
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastRefreshTime;
        if (autoRefreshEnabled && elapsed.count() >= autoRefreshInterval) {
            windowsList = winMgr.GetActiveWindows();

            if (affinityCtrl.IsFocusModeActive()) {
                // Focus Mode active: Ensure every other user app window is cloaked
                DWORD targetPid = affinityCtrl.GetFocusedPid();
                DWORD myPid = GetCurrentProcessId();
                for (auto& win : windowsList) {
                    if (win.processId == targetPid || win.processId == myPid) continue;
                    if (WindowManager::IsSystemProcess(win.processName)) continue;

                    if (!win.isExcludedFromCapture && !affinityCtrl.IsProcessExcluded(win.processId)) {
                        if (affinityCtrl.SetWindowExcluded(win.hwnd, true)) {
                            win.isExcludedFromCapture = true;
                        }
                    }
                }
            } else {
                // Standard mode: Apply auto-hide rules
                for (auto& win : windowsList) {
                    if (!win.isExcludedFromCapture && configMgr.ShouldAutoExclude(win.processName)) {
                        if (affinityCtrl.SetWindowExcluded(win.hwnd, true)) {
                            win.isExcludedFromCapture = true;
                        }
                    }
                }
            }
            lastRefreshTime = currentTime;
        }

        // Handle resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Calculate statistics
        int cloakedCount = 0;
        for (const auto& w : windowsList) {
            if (w.isExcludedFromCapture) cloakedCount++;
        }
        int streamingCount = (int)windowsList.size() - cloakedCount;

        // Create main viewport covering the entire client area
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGuiWindowFlags rootFlags = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("RootWindow", nullptr, rootFlags);

        // =====================================================================
        // SPOTIFY-STYLE 2-COLUMN LAYOUT: (SIDEBAR + MAIN CONTENT)
        // =====================================================================
        float sidebarWidth = 230.0f;
        float contentWidth = ImGui::GetContentRegionAvail().x - sidebarWidth - 12.0f;
        float fullHeight = ImGui::GetContentRegionAvail().y - 32.0f; // Leave space for footer

        // ---------------------------------------------------------------------
        // 1. SPOTIFY LEFT SIDEBAR
        // ---------------------------------------------------------------------
        ImGui::PushStyleColor(ImGuiCol_ChildBg, GuiTheme::ColSpotifySidebar);
        ImGui::PushStyleColor(ImGuiCol_Border, GuiTheme::ColSpotifyBorder);
        ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, fullHeight), true, ImGuiWindowFlags_NoScrollbar);
        {
            // App Branding Header
            ImGui::SetCursorPos(ImVec2(16, 18));
            if (fontTitle) ImGui::PushFont(fontTitle);
            ImGui::PushStyleColor(ImGuiCol_Text, GuiTheme::ColSpotifyGreen);
            ImGui::Text("GHOSTFRAME");
            ImGui::PopStyleColor();
            if (fontTitle) ImGui::PopFont();

            ImGui::SameLine();
            ImGui::SetCursorPosY(18);
            ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColSpotifyCardHover);
            ImGui::PushStyleColor(ImGuiCol_Text, GuiTheme::ColSpotifySubtext);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 2.0f));
            ImGui::Button("v1.0");
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);

            ImGui::SetCursorPos(ImVec2(16, 48));
            ImGui::PushStyleColor(ImGuiCol_Text, GuiTheme::ColTextMuted);
            ImGui::Text("Stream Capture Cloaker");
            ImGui::PopStyleColor();

            ImGui::SetCursorPosY(76);
            ImGui::Separator();
            ImGui::Spacing();

            // Navigation Menu (Spotify Style Pill Buttons)
            auto RenderNavButton = [&](int tabIndex, const char* icon, const char* label, int badgeCount = -1) {
                bool isSelected = (currentTab == tabIndex);
                ImGui::PushID(tabIndex);

                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColSpotifyCardHover);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColSpotifyCardHover);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, GuiTheme::ColSpotifyCardHover);
                    ImGui::PushStyleColor(ImGuiCol_Text, GuiTheme::ColSpotifyGreen);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColSpotifyCard);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, GuiTheme::ColSpotifyCardHover);
                    ImGui::PushStyleColor(ImGuiCol_Text, GuiTheme::ColTextSubdued);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.08f, 0.5f));
                
                std::string btnText = std::string(icon) + "  " + label;
                if (ImGui::Button(btnText.c_str(), ImVec2(sidebarWidth - 28.0f, 40.0f))) {
                    currentTab = tabIndex;
                }

                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(4);

                // Optional trailing badge
                if (badgeCount >= 0) {
                    ImGui::SameLine(sidebarWidth - 62.0f);
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, isSelected ? GuiTheme::ColSpotifyGreen : GuiTheme::ColTextMuted);
                    ImGui::Text("(%d)", badgeCount);
                    ImGui::PopStyleColor();
                }

                ImGui::PopID();
                ImGui::Spacing();
            };

            ImGui::SetCursorPosY(92);
            RenderNavButton(0, "[#]", "Active Windows", (int)windowsList.size());
            RenderNavButton(1, "[*]", "Focus Mode", (affinityCtrl.IsFocusModeActive() ? 1 : 0));
            RenderNavButton(2, "[+]", "Auto-Hide Rules", (int)configMgr.GetAutoExcludeList().size());
            RenderNavButton(3, "[?]", "Streaming Guide");

            // Sidebar Bottom: GhostFrame Self-Hiding Card
            float bottomCardY = fullHeight - 145.0f;
            ImGui::SetCursorPos(ImVec2(12, bottomCardY));

            ImGui::PushStyleColor(ImGuiCol_ChildBg, GuiTheme::ColSpotifyCard);
            ImGui::PushStyleColor(ImGuiCol_Border, GuiTheme::ColSpotifyBorder);
            ImGui::BeginChild("SelfHideCard", ImVec2(sidebarWidth - 24.0f, 130.0f), true, ImGuiWindowFlags_NoScrollbar);
            {
                ImGui::SetCursorPos(ImVec2(10, 8));
                ImGui::TextColored(GuiTheme::ColTextLight, "App Stream Visibility");

                bool isSelfHidden = WindowManager::IsExcluded(hwnd);
                ImGui::SetCursorPos(ImVec2(10, 32));
                if (isSelfHidden) {
                    ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColRose);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColRoseHover);
                    if (ImGui::Button("Unhide This App", ImVec2(sidebarWidth - 44.0f, 32.0f))) {
                        affinityCtrl.SetWindowExcluded(hwnd, false);
                        windowsList = winMgr.GetActiveWindows();
                        statusNotice = "GhostFrame is now visible to screen capture.";
                    }
                    ImGui::PopStyleColor(2);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColSpotifyGreen);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColSpotifyGreenHover);
                    if (ImGui::Button("Hide This App", ImVec2(sidebarWidth - 44.0f, 32.0f))) {
                        affinityCtrl.SetWindowExcluded(hwnd, true);
                        windowsList = winMgr.GetActiveWindows();
                        statusNotice = "GhostFrame is now HIDDEN from screen capture!";
                    }
                    ImGui::PopStyleColor(2);
                }

                ImGui::SetCursorPos(ImVec2(10, 72));
                ImGui::PushStyleColor(ImGuiCol_Text, isSelfHidden ? GuiTheme::ColRoseText : GuiTheme::ColSpotifyGreen);
                ImGui::Text("%s", isSelfHidden ? "● Cloaked from Stream" : "● Visible in Stream");
                ImGui::PopStyleColor();

                ImGui::SetCursorPos(ImVec2(10, 92));
                ImGui::TextColored(GuiTheme::ColTextMuted, "DWM x64 Protected");
            }
            ImGui::EndChild();
            ImGui::PopStyleColor(2);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        // ---------------------------------------------------------------------
        // 2. MAIN CONTENT VIEWPORT (TAB 0, 1, 2, 3)
        // ---------------------------------------------------------------------
        ImGui::PushStyleColor(ImGuiCol_ChildBg, GuiTheme::ColSpotifyBlack);
        ImGui::BeginChild("MainViewport", ImVec2(contentWidth, fullHeight), false);
        {
            // =================================================================
            // TAB 0: ACTIVE WINDOWS TABLE
            // =================================================================
            if (currentTab == 0) {
                // Focus Mode Active Banner
                if (affinityCtrl.IsFocusModeActive()) {
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, GuiTheme::ColSpotifyGreenBg);
                    ImGui::PushStyleColor(ImGuiCol_Border, GuiTheme::ColSpotifyGreen);
                    ImGui::BeginChild("FocusBanner", ImVec2(0, 52), true, ImGuiWindowFlags_NoScrollbar);
                    {
                        ImGui::SetCursorPos(ImVec2(14, 14));
                        ImGui::TextColored(GuiTheme::ColSpotifyGreen, "★ FOCUS MODE ACTIVE:");
                        ImGui::SameLine();
                        ImGui::TextColored(GuiTheme::ColTextLight, "Only '%s' is streaming. All other windows are cloaked.", WStringToUtf8(affinityCtrl.GetFocusedProcessName()).c_str());

                        ImGui::SameLine(ImGui::GetWindowWidth() - 170.0f);
                        ImGui::SetCursorPosY(10);
                        ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColSpotifyGreen);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColSpotifyGreenHover);
                        if (ImGui::Button("Exit Focus Mode", ImVec2(150, 32))) {
                            affinityCtrl.ExitFocusMode();
                            windowsList = winMgr.GetActiveWindows();
                            statusNotice = "Exited Focus Mode. All windows restored.";
                        }
                        ImGui::PopStyleColor(2);
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                    ImGui::Spacing();
                }

                // Top Search and Filter Bar
                ImGui::PushStyleColor(ImGuiCol_ChildBg, GuiTheme::ColSpotifyCard);
                ImGui::PushStyleColor(ImGuiCol_Border, GuiTheme::ColSpotifyBorder);
                ImGui::BeginChild("TopBar", ImVec2(0, 54), true, ImGuiWindowFlags_NoScrollbar);
                {
                    ImGui::SetCursorPos(ImVec2(12, 10));

                    // Search input
                    ImGui::PushItemWidth(320);
                    ImGui::InputTextWithHint("##search", "Search by process or window title...", searchFilter, IM_ARRAYSIZE(searchFilter));
                    ImGui::PopItemWidth();

                    ImGui::SameLine(0, 18);
                    ImGui::SetCursorPosY(12);

                    // Filter chips: All / Cloaked / Streaming
                    if (ImGui::RadioButton("All", filterMode == 0)) filterMode = 0;
                    ImGui::SameLine(0, 14);
                    if (ImGui::RadioButton("Cloaked Only", filterMode == 1)) filterMode = 1;
                    ImGui::SameLine(0, 14);
                    if (ImGui::RadioButton("Streaming Only", filterMode == 2)) filterMode = 2;

                    ImGui::SameLine(0, 18);
                    ImGui::Checkbox("Auto-Refresh", &autoRefreshEnabled);

                    // Right action buttons (generous spacing, no collisions)
                    float rightActionsX = ImGui::GetWindowWidth() - 250.0f;
                    ImGui::SetCursorPos(ImVec2(rightActionsX, 10));

                    if (ImGui::Button("Restore All", ImVec2(115, 34))) {
                        affinityCtrl.RestoreAllWindows();
                        windowsList = winMgr.GetActiveWindows();
                        statusNotice = "All windows restored to normal visibility.";
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Restore all tracked windows so stream can see them");
                    }

                    ImGui::SameLine(0, 10);
                    if (ImGui::Button("Refresh (F5)", ImVec2(115, 34))) {
                        windowsList = winMgr.GetActiveWindows();
                        statusNotice = "Refreshed active windows list.";
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor(2);

                ImGui::Spacing();

                // Hero Stat Cards Bar
                {
                    float cardWidth = (contentWidth - 36.0f) / 4.0f;
                    GuiTheme::RenderStatCard("Total Windows", std::to_string(windowsList.size()).c_str(), GuiTheme::ColTextLight, cardWidth);
                    ImGui::SameLine(0, 12);
                    GuiTheme::RenderStatCard("Streaming", std::to_string(streamingCount).c_str(), GuiTheme::ColSpotifyGreen, cardWidth);
                    ImGui::SameLine(0, 12);
                    GuiTheme::RenderStatCard("Cloaked", std::to_string(cloakedCount).c_str(), (cloakedCount > 0 ? GuiTheme::ColRoseText : GuiTheme::ColTextMuted), cardWidth);
                    ImGui::SameLine(0, 12);
                    GuiTheme::RenderStatCard("Stream Mode", (affinityCtrl.IsFocusModeActive() ? "Focus / Solo" : "Standard"), (affinityCtrl.IsFocusModeActive() ? GuiTheme::ColSoloGoldText : GuiTheme::ColSpotifyGreen), cardWidth);
                }

                ImGui::Spacing();

                // Windows Table with generous collision-free columns
                static ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg |
                                                   ImGuiTableFlags_BordersInnerV |
                                                   ImGuiTableFlags_BordersOuter |
                                                   ImGuiTableFlags_ScrollY |
                                                   ImGuiTableFlags_Resizable;

                float tableHeight = ImGui::GetContentRegionAvail().y - 10.0f;
                if (ImGui::BeginTable("WindowsTable", 5, tableFlags, ImVec2(0, tableHeight))) {
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                    ImGui::TableSetupColumn("Application", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                    ImGui::TableSetupColumn("Window Title & Details", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Stream Actions", ImGuiTableColumnFlags_WidthFixed, 220.0f);
                    ImGui::TableSetupColumn("Auto-Rule", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                    ImGui::TableHeadersRow();

                    std::string filterStr = searchFilter;
                    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

                    for (size_t i = 0; i < windowsList.size(); ++i) {
                        auto& win = windowsList[i];
                        std::string procUtf8 = WStringToUtf8(win.processName);
                        std::string titleUtf8 = WStringToUtf8(win.windowTitle);

                        // Mode filter
                        if (filterMode == 1 && !win.isExcludedFromCapture) continue;
                        if (filterMode == 2 && win.isExcludedFromCapture) continue;

                        // Search filter
                        if (!filterStr.empty()) {
                            std::string lowerProc = procUtf8;
                            std::string lowerTitle = titleUtf8;
                            std::transform(lowerProc.begin(), lowerProc.end(), lowerProc.begin(), ::tolower);
                            std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);
                            if (lowerProc.find(filterStr) == std::string::npos &&
                                lowerTitle.find(filterStr) == std::string::npos) {
                                continue;
                            }
                        }

                        bool isSoloTarget = (affinityCtrl.IsFocusModeActive() && affinityCtrl.GetFocusedPid() == win.processId);

                        ImGui::TableNextRow(0, 42.0f);
                        ImGui::PushID((int)i);

                        // Col 0: Status Badge
                        ImGui::TableSetColumnIndex(0);
                        GuiTheme::RenderStatusBadge(win.isExcludedFromCapture, isSoloTarget, (int)i);

                        // Col 1: Application (Process Name + PID)
                        ImGui::TableSetColumnIndex(1);
                        bool isThisApp = (win.processId == GetCurrentProcessId());
                        if (isThisApp) {
                            ImGui::TextColored(GuiTheme::ColSpotifyGreen, "%s", procUtf8.c_str());
                            ImGui::SameLine();
                            ImGui::TextColored(GuiTheme::ColTextMuted, "[THIS APP]");
                        } else {
                            ImGui::TextColored(GuiTheme::ColTextLight, "%s", procUtf8.c_str());
                        }
                        if (fontSmall) ImGui::PushFont(fontSmall);
                        ImGui::TextColored(GuiTheme::ColTextMuted, "PID: %lu", win.processId);
                        if (fontSmall) ImGui::PopFont();

                        // Col 2: Window Title & Hover Details
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(titleUtf8.c_str());
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::TextColored(GuiTheme::ColSpotifyGreen, "Application Details:");
                            ImGui::Text("Window Title: %s", titleUtf8.c_str());
                            ImGui::Text("Process: %s", procUtf8.c_str());
                            ImGui::Text("PID: %lu", win.processId);
                            ImGui::Text("HWND Handle: 0x%p", win.hwnd);
                            ImGui::Text("Executable: %s", WStringToUtf8(win.executablePath).c_str());
                            ImGui::EndTooltip();
                        }
                        if (fontSmall) ImGui::PushFont(fontSmall);
                        ImGui::TextColored(GuiTheme::ColTextMuted, "HWND: 0x%p", win.hwnd);
                        if (fontSmall) ImGui::PopFont();

                        // Col 3: Stream Actions (Cloak/Show + Solo)
                        ImGui::TableSetColumnIndex(3);

                        // Button 1: Cloak / Show Toggle
                        if (win.isExcludedFromCapture) {
                            ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColSpotifyGreen);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColSpotifyGreenHover);
                            if (ImGui::Button("Show", ImVec2(95, 30))) {
                                if (affinityCtrl.SetWindowExcluded(win.hwnd, false)) {
                                    win.isExcludedFromCapture = false;
                                    statusNotice = "Restored " + procUtf8 + " to stream capture.";
                                }
                            }
                            ImGui::PopStyleColor(2);
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColRose);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColRoseHover);
                            if (ImGui::Button("Cloak", ImVec2(95, 30))) {
                                if (affinityCtrl.SetWindowExcluded(win.hwnd, true)) {
                                    win.isExcludedFromCapture = true;
                                    statusNotice = "Cloaked " + procUtf8 + " from screen capture!";
                                }
                            }
                            ImGui::PopStyleColor(2);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip(win.isExcludedFromCapture ? "Make this window visible on stream" : "Make this window invisible to stream/recording");
                        }

                        // Button 2: Solo / Focus Mode Button
                        ImGui::SameLine(0, 10);
                        if (isSoloTarget) {
                            ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColSoloGold);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColSoloGoldHover);
                            if (ImGui::Button("Exit Solo", ImVec2(95, 30))) {
                                affinityCtrl.ExitFocusMode();
                                windowsList = winMgr.GetActiveWindows();
                                statusNotice = "Exited Solo Mode. All windows restored.";
                            }
                            ImGui::PopStyleColor(2);
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColSpotifyCardHover);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColSpotifyGreen);
                            if (ImGui::Button("Solo", ImVec2(95, 30))) {
                                affinityCtrl.EnterFocusMode(win.processId, win.processName, windowsList);
                                windowsList = winMgr.GetActiveWindows();
                                statusNotice = "Solo Mode engaged: Only " + procUtf8 + " is streaming!";
                            }
                            ImGui::PopStyleColor(2);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Show ONLY this app on stream and automatically cloak all other windows!");
                        }

                        // Col 4: Auto-Hide Rule Checkbox
                        ImGui::TableSetColumnIndex(4);
                        bool isAuto = configMgr.ShouldAutoExclude(win.processName);
                        if (ImGui::Checkbox("Always Hide", &isAuto)) {
                            if (isAuto) {
                                configMgr.AddAutoExcludeProcess(win.processName);
                                statusNotice = "Added auto-hide rule for " + procUtf8;
                            } else {
                                configMgr.RemoveAutoExcludeProcess(win.processName);
                                statusNotice = "Removed auto-hide rule for " + procUtf8;
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Automatically cloak this application whenever it launches");
                        }

                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
            }

            // =================================================================
            // TAB 1: FOCUS / SOLO MODE OVERVIEW & LAUNCHER
            // =================================================================
            else if (currentTab == 1) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, GuiTheme::ColSpotifyCard);
                ImGui::PushStyleColor(ImGuiCol_Border, GuiTheme::ColSpotifyBorder);
                ImGui::BeginChild("FocusPageCard", ImVec2(0, 170), true, ImGuiWindowFlags_NoScrollbar);
                {
                    ImGui::SetCursorPos(ImVec2(18, 16));
                    ImGui::TextColored(GuiTheme::ColSpotifyGreen, "FOCUS MODE / SOLO STREAMING");

                    ImGui::SetCursorPos(ImVec2(18, 42));
                    ImGui::TextWrapped(
                        "Focus Mode is the inverse of hiding apps. Instead of manually picking windows to cloak,\n"
                        "you select ONE application that you want to share (e.g. Visual Studio, Game, or Chrome),\n"
                        "and GhostFrame automatically cloaks ALL other desktop windows in real-time."
                    );

                    ImGui::SetCursorPos(ImVec2(18, 110));
                    if (affinityCtrl.IsFocusModeActive()) {
                        ImGui::TextColored(GuiTheme::ColSpotifyGreen, "Current Status: ACTIVE (Focused on: %s, PID: %lu)",
                            WStringToUtf8(affinityCtrl.GetFocusedProcessName()).c_str(),
                            affinityCtrl.GetFocusedPid());

                        ImGui::SameLine(ImGui::GetWindowWidth() - 190.0f);
                        ImGui::SetCursorPosY(102);
                        if (ImGui::Button("Exit Focus Mode", ImVec2(170, 36))) {
                            affinityCtrl.ExitFocusMode();
                            windowsList = winMgr.GetActiveWindows();
                            statusNotice = "Exited Focus Mode.";
                        }
                    } else {
                        ImGui::TextColored(GuiTheme::ColTextMuted, "Current Status: Inactive (Normal streaming mode)");
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor(2);

                ImGui::Spacing();
                ImGui::TextColored(GuiTheme::ColTextLight, "Quick Launch Focus Mode (Click an app below to isolate it on stream):");
                ImGui::Spacing();

                // List of detected top apps to solo with 1 click
                std::vector<std::pair<std::wstring, DWORD>> uniqueApps;
                for (const auto& w : windowsList) {
                    if (w.processId == GetCurrentProcessId()) continue;
                    bool exists = false;
                    for (const auto& u : uniqueApps) {
                        if (u.second == w.processId) { exists = true; break; }
                    }
                    if (!exists) uniqueApps.push_back({ w.processName, w.processId });
                }

                if (ImGui::BeginTable("QuickSoloTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuter)) {
                    ImGui::TableSetupColumn("Process Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                    ImGui::TableHeadersRow();

                    for (size_t a = 0; a < uniqueApps.size(); ++a) {
                        ImGui::TableNextRow(0, 38.0f);
                        std::string pName = WStringToUtf8(uniqueApps[a].first);
                        DWORD pPid = uniqueApps[a].second;

                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextColored(GuiTheme::ColTextLight, "%s", pName.c_str());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(GuiTheme::ColTextMuted, "%lu", pPid);

                        ImGui::TableSetColumnIndex(2);
                        ImGui::PushID((int)a + 9000);
                        bool isThisAppSolo = (affinityCtrl.IsFocusModeActive() && affinityCtrl.GetFocusedPid() == pPid);
                        if (isThisAppSolo) {
                            ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColSoloGold);
                            if (ImGui::Button("Solo Active", ImVec2(120, 28))) {
                                affinityCtrl.ExitFocusMode();
                                windowsList = winMgr.GetActiveWindows();
                            }
                            ImGui::PopStyleColor();
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColSpotifyGreen);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColSpotifyGreenHover);
                            if (ImGui::Button("Solo This App", ImVec2(120, 28))) {
                                affinityCtrl.EnterFocusMode(pPid, uniqueApps[a].first, windowsList);
                                windowsList = winMgr.GetActiveWindows();
                            }
                            ImGui::PopStyleColor(2);
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
            }

            // =================================================================
            // TAB 2: AUTO-HIDE RULES
            // =================================================================
            else if (currentTab == 2) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, GuiTheme::ColSpotifyCard);
                ImGui::PushStyleColor(ImGuiCol_Border, GuiTheme::ColSpotifyBorder);
                ImGui::BeginChild("RulesHeaderCard", ImVec2(0, 116), true, ImGuiWindowFlags_NoScrollbar);
                {
                    ImGui::SetCursorPos(ImVec2(16, 12));
                    ImGui::TextColored(GuiTheme::ColSpotifyGreen, "ALWAYS HIDE RULES (Automated Streamer Cloak)");

                    ImGui::SetCursorPos(ImVec2(16, 36));
                    ImGui::TextColored(GuiTheme::ColTextSubdued, "Any application matching these names will be instantly cloaked the moment it opens.");

                    ImGui::SetCursorPos(ImVec2(16, 68));
                    ImGui::TextColored(GuiTheme::ColTextLight, "Quick Presets:");
                    ImGui::SameLine(0, 12);

                    auto AddPreset = [&](const wchar_t* proc) {
                        configMgr.AddAutoExcludeProcess(proc);
                        statusNotice = "Added preset rule: " + WStringToUtf8(proc);
                    };

                    if (ImGui::Button("+ Discord"))  AddPreset(L"discord.exe");
                    ImGui::SameLine();
                    if (ImGui::Button("+ WhatsApp")) AddPreset(L"whatsapp.exe");
                    ImGui::SameLine();
                    if (ImGui::Button("+ Telegram")) AddPreset(L"telegram.exe");
                    ImGui::SameLine();
                    if (ImGui::Button("+ Chrome"))   AddPreset(L"chrome.exe");
                    ImGui::SameLine();
                    if (ImGui::Button("+ Spotify"))  AddPreset(L"spotify.exe");
                    ImGui::SameLine();
                    if (ImGui::Button("+ Notepad"))  AddPreset(L"notepad.exe");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor(2);

                ImGui::Spacing();

                // Custom Rule Input
                ImGui::PushItemWidth(340);
                ImGui::InputTextWithHint("##newruleinput", "Enter process name (e.g. slack.exe, signal.exe)...", newRuleBuffer, IM_ARRAYSIZE(newRuleBuffer));
                ImGui::PopItemWidth();

                ImGui::SameLine(0, 12);
                ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColSpotifyGreen);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColSpotifyGreenHover);
                if (ImGui::Button("Add Custom Rule", ImVec2(150, 32)) && strlen(newRuleBuffer) > 0) {
                    configMgr.AddAutoExcludeProcess(Utf8ToWString(newRuleBuffer));
                    newRuleBuffer[0] = '\0';
                    statusNotice = "Custom rule added successfully.";
                }
                ImGui::PopStyleColor(2);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                auto rules = configMgr.GetAutoExcludeList();
                if (rules.empty()) {
                    ImGui::TextColored(GuiTheme::ColTextMuted, "No auto-hide rules configured yet. Click a quick preset above or type a process name.");
                } else {
                    if (ImGui::BeginTable("RulesListTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter)) {
                        ImGui::TableSetupColumn("No.", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableSetupColumn("Process Name", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Manage", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                        ImGui::TableHeadersRow();

                        for (size_t r = 0; r < rules.size(); ++r) {
                            std::string rUtf8 = WStringToUtf8(rules[r]);
                            ImGui::TableNextRow(0, 36.0f);

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%zu", r + 1);

                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextColored(GuiTheme::ColTextLight, "•  %s", rUtf8.c_str());

                            ImGui::TableSetColumnIndex(2);
                            ImGui::PushID((int)r + 5000);
                            ImGui::PushStyleColor(ImGuiCol_Button, GuiTheme::ColRose);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiTheme::ColRoseHover);
                            if (ImGui::Button("Remove", ImVec2(100, 26))) {
                                configMgr.RemoveAutoExcludeProcess(rules[r]);
                                statusNotice = "Removed rule for " + rUtf8;
                            }
                            ImGui::PopStyleColor(2);
                            ImGui::PopID();
                        }
                        ImGui::EndTable();
                    }
                }
            }

            // =================================================================
            // TAB 3: STREAMING GUIDE & IMMEDIATE VERIFICATION
            // =================================================================
            else if (currentTab == 3) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, GuiTheme::ColSpotifyCard);
                ImGui::PushStyleColor(ImGuiCol_Border, GuiTheme::ColSpotifyBorder);
                ImGui::BeginChild("Guide1", ImVec2(0, 160), true, ImGuiWindowFlags_NoScrollbar);
                {
                    ImGui::SetCursorPos(ImVec2(16, 14));
                    ImGui::TextColored(GuiTheme::ColSpotifyGreen, "1. HOW HARDWARE CLOAKING WORKS IN OBS & DISCORD");
                    ImGui::SetCursorPos(ImVec2(16, 38));
                    ImGui::TextWrapped(
                        "GhostFrame operates directly at the Windows Desktop Window Manager (DWM) hardware compositor layer.\n"
                        "When you cloak a window, Windows instructs the GPU compositor to exclude that window surface whenever\n"
                        "screen capture frames are assembled. Because this is an OS-level exclusion, OBS Studio Display Capture,\n"
                        "Discord Live Screen Share, Zoom, Teams, and screenshot tools CANNOT see the window.\n"
                        "Meanwhile, Windows continues rendering the window to your physical monitor so you can use it normally!"
                    );
                }
                ImGui::EndChild();

                ImGui::Spacing();

                ImGui::BeginChild("Guide2", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
                {
                    ImGui::SetCursorPos(ImVec2(16, 14));
                    ImGui::TextColored(GuiTheme::ColSpotifyGreen, "2. HOW TO TEST AND VERIFY RIGHT NOW");
                    ImGui::SetCursorPos(ImVec2(16, 38));
                    ImGui::BulletText("Click [ Cloak ] next to any running app or click [ Hide This App ] in the sidebar.");
                    ImGui::BulletText("Verify that the app is still visible and fully interactive on your physical monitor.");
                    ImGui::BulletText("Press Win + Shift + S to take a screenshot with Windows Snipping Tool, or start Discord screen share.");
                    ImGui::BulletText("Look at the captured frame: The cloaked window is completely invisible, and wallpaper shows through cleanly!");
                }
                ImGui::EndChild();

                ImGui::Spacing();

                ImGui::BeginChild("Guide3", ImVec2(0, 120), true, ImGuiWindowFlags_NoScrollbar);
                {
                    ImGui::SetCursorPos(ImVec2(16, 14));
                    ImGui::TextColored(GuiTheme::ColSpotifyGreen, "3. PRIVACY & SAFETY GUARANTEE");
                    ImGui::SetCursorPos(ImVec2(16, 38));
                    ImGui::BulletText("Clicking [ Restore All ] or closing GhostFrame safely restores all windows to normal visibility.");
                    ImGui::BulletText("Process-wide cloaking automatically covers all child popups, dialogs, and context menus.");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor(2);
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // =====================================================================
        // 3. STATUS FOOTER
        // =====================================================================
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 28);
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, GuiTheme::ColTextMuted);
        ImGui::Text("  Status: %s", statusNotice.c_str());

        ImGui::SameLine(ImGui::GetWindowWidth() - 380);
        ImGui::Text("Hardware Capture Exclusion: Active (x64)");
        ImGui::PopStyleColor();

        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color[4] = { 0.07f, 0.07f, 0.07f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present with vsync
        g_pSwapChain->Present(1, 0);
    }

    // Cleanup and restore all windows
    affinityCtrl.RestoreAllWindows();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// -----------------------------------------------------------------------------
// Direct3D 11 Helper Functions
// -----------------------------------------------------------------------------

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &featureLevel,
        &g_pd3dDeviceContext
    );

    if (res == DXGI_ERROR_UNSUPPORTED) {
        // Fallback to WARP software driver
        res = D3D11CreateDeviceAndSwapChain(
            NULL,
            D3D_DRIVER_TYPE_WARP,
            NULL,
            createDeviceFlags,
            featureLevelArray,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &g_pSwapChain,
            &g_pd3dDevice,
            &featureLevel,
            &g_pd3dDeviceContext
        );
    }

    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
