#include "window-manager.hpp"
#include <dwmapi.h>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

#ifndef DWMWA_CLOAKED
#define DWMWA_CLOAKED 14
#endif

WindowManager* WindowManager::s_instance = nullptr;

WindowManager::WindowManager() : eventHookHandle(NULL) {
    s_instance = this;
}

WindowManager::~WindowManager() {
    StopEventHook();
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

DWORD WindowManager::GetAffinity(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return 0;
    DWORD affinity = 0;
    if (GetWindowDisplayAffinity(hwnd, &affinity)) {
        return affinity;
    }
    return 0;
}

bool WindowManager::IsExcluded(HWND hwnd) {
    DWORD aff = GetAffinity(hwnd);
    return (aff == WDA_EXCLUDEFROMCAPTURE);
}

std::wstring WindowManager::GetProcessName(DWORD processId, std::wstring* outFullPath) {
    if (processId == 0) return L"";

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!hProc) return L"";

    wchar_t fullPath[MAX_PATH] = { 0 };
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(hProc, 0, fullPath, &size)) {
        CloseHandle(hProc);
        if (outFullPath) {
            *outFullPath = fullPath;
        }
        std::wstring str(fullPath);
        size_t lastSlash = str.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            return str.substr(lastSlash + 1);
        }
        return str;
    }

    CloseHandle(hProc);
    return L"";
}

bool WindowManager::IsSystemProcess(const std::wstring& procName) {
    if (procName.empty()) return true;

    std::wstring lower = procName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    if (lower == L"explorer.exe" ||
        lower == L"dwm.exe" ||
        lower == L"applicationframehost.exe" ||
        lower == L"shellexperiencehost.exe" ||
        lower == L"startmenuexperiencehost.exe" ||
        lower == L"searchhost.exe" ||
        lower == L"searchapp.exe" ||
        lower == L"textinputhost.exe" ||
        lower == L"systemsettings.exe" ||
        lower == L"systemsettingsadminflows.exe" ||
        lower == L"lockapp.exe" ||
        lower == L"screenclippinghost.exe" ||
        lower == L"snippingtool.exe" ||
        lower == L"taskmgr.exe" ||
        lower == L"csrss.exe" ||
        lower == L"services.exe" ||
        lower == L"lsass.exe" ||
        lower == L"smss.exe" ||
        lower == L"wininit.exe" ||
        lower == L"winlogon.exe" ||
        lower == L"fontdrvhost.exe" ||
        lower == L"ctfmon.exe" ||
        lower == L"conhost.exe" ||
        lower == L"audiodg.exe" ||
        lower == L"widgetboard.exe" ||
        lower == L"widgetservice.exe" ||
        lower == L"shellhost.exe" ||
        lower == L"sihost.exe" ||
        lower == L"crossdeviceresume.exe" ||
        lower == L"useroobebroker.exe" ||
        lower == L"securityhealthsystray.exe" ||
        lower == L"securityhealthservice.exe" ||
        lower == L"spoolsv.exe") {
        return true;
    }
    return false;
}

bool WindowManager::IsSystemWindowClass(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return true;

    wchar_t clsBuf[256] = { 0 };
    if (GetClassNameW(hwnd, clsBuf, 256) == 0) return false;

    std::wstring cls(clsBuf);
    if (cls == L"Shell_TrayWnd" ||
        cls == L"Shell_SecondaryTrayWnd" ||
        cls == L"Progman" ||
        cls == L"WorkerW" ||
        cls == L"Windows.UI.Core.CoreWindow" ||
        cls == L"DV2ControlHost" ||
        cls == L"Button" ||
        cls == L"TopLevelWindowForOverflowXamlIsland" ||
        cls == L"XamlExplorerHostIslandWindow" ||
        cls == L"DummyDWMListenerWindow") {
        return true;
    }
    return false;
}

bool WindowManager::IsCandidateWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return false;
    if (!IsWindowVisible(hwnd)) return false;

    // Check window styles
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if ((exStyle & WS_EX_TOOLWINDOW) && !(exStyle & WS_EX_APPWINDOW)) {
        return false;
    }

    if (exStyle & WS_EX_TRANSPARENT) {
        return false;
    }

    // Check window cloaking (UWP apps suspended, or virtual desktop hidden)
    int cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
        if (cloaked != 0 && cloaked != DWM_CLOAKED_INHERITED) {
            return false;
        }
    }

    // Ignore zero-size windows
    RECT rc;
    if (GetWindowRect(hwnd, &rc)) {
        if ((rc.right - rc.left <= 40) || (rc.bottom - rc.top <= 40)) {
            return false;
        }
    }

    // Check system window classes (Taskbar, Desktop, etc.)
    if (IsSystemWindowClass(hwnd)) {
        return false;
    }

    // Check PID & system processes (explorer, applicationframehost, etc.)
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0 || pid == 4) return false;

    std::wstring procName = GetProcessName(pid);
    if (IsSystemProcess(procName)) {
        return false;
    }

    // Get window title
    wchar_t title[512] = { 0 };
    int len = GetWindowTextW(hwnd, title, 512);
    if (len == 0) return false;

    std::wstring strTitle(title);
    if (strTitle == L"Program Manager" ||
        strTitle == L"Settings" ||
        strTitle == L"Windows Input Experience" ||
        strTitle == L"Desktop Window Manager" ||
        strTitle == L"Windows Shell Experience Host") {
        return false;
    }

    return true;
}

struct EnumWindowsData {
    std::vector<WindowInfo> windows;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);
    if (!data) return FALSE;

    if (!WindowManager::IsCandidateWindow(hwnd)) {
        return TRUE;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return TRUE;

    // Allow all candidate windows including Window Hider itself so users can hide this app too

    wchar_t titleBuf[512] = { 0 };
    GetWindowTextW(hwnd, titleBuf, 512);

    std::wstring fullPath;
    std::wstring procName = WindowManager::GetProcessName(pid, &fullPath);

    DWORD affinity = WindowManager::GetAffinity(hwnd);

    WindowInfo info;
    info.hwnd = hwnd;
    info.processId = pid;
    info.processName = procName;
    info.executablePath = fullPath;
    info.windowTitle = titleBuf;
    info.currentAffinity = affinity;
    info.isExcludedFromCapture = (affinity == WDA_EXCLUDEFROMCAPTURE);

    data->windows.push_back(info);
    return TRUE;
}

std::vector<WindowInfo> WindowManager::GetActiveWindows() {
    EnumWindowsData data;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.windows;
}

void CALLBACK WindowManager::WinEventProc(
    HWINEVENTHOOK /*hWinEventHook*/,
    DWORD /*event*/,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD /*idEventThread*/,
    DWORD /*dwmsEventTime*/
) {
    if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF) return;
    if (!hwnd || !s_instance || !s_instance->onWindowCreated) return;

    if (IsCandidateWindow(hwnd)) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != 0) {
            std::wstring procName = GetProcessName(pid);
            s_instance->onWindowCreated(hwnd, pid, procName);
        }
    }
}

bool WindowManager::StartEventHook(WindowCreatedCallback callback) {
    StopEventHook();
    onWindowCreated = callback;

    eventHookHandle = SetWinEventHook(
        EVENT_OBJECT_CREATE,
        EVENT_OBJECT_SHOW,
        NULL,
        WinEventProc,
        0,
        0,
        WINEVENT_OUTOFCONTEXT
    );

    return (eventHookHandle != NULL);
}

void WindowManager::StopEventHook() {
    if (eventHookHandle) {
        UnhookWinEvent(eventHookHandle);
        eventHookHandle = NULL;
    }
}
