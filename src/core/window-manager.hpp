#pragma once

#include "window-info.hpp"
#include <vector>
#include <functional>
#include <memory>

class WindowManager {
public:
    using WindowCreatedCallback = std::function<void(HWND hwnd, DWORD processId, const std::wstring& processName)>;

    WindowManager();
    ~WindowManager();

    // Enumerate all visible, interactive top-level application windows
    std::vector<WindowInfo> GetActiveWindows();

    // Check current display affinity of a specific window
    static DWORD GetAffinity(HWND hwnd);

    // Check whether the window is currently excluded from capture
    static bool IsExcluded(HWND hwnd);

    // Check if an HWND is a candidate application window (not cloaked/tool/desktop)
    static bool IsCandidateWindow(HWND hwnd);

    // Check if a process is a Windows shell or system process that must not be cloaked
    static bool IsSystemProcess(const std::wstring& procName);

    // Check if a window class belongs to system shell (Taskbar, Desktop, etc.)
    static bool IsSystemWindowClass(HWND hwnd);

    // Retrieve process name from Process ID
    static std::wstring GetProcessName(DWORD processId, std::wstring* outFullPath = nullptr);

    // Start background event hook for detecting newly opened windows
    bool StartEventHook(WindowCreatedCallback callback);

    // Stop background event hook
    void StopEventHook();

private:
    HWINEVENTHOOK eventHookHandle;
    WindowCreatedCallback onWindowCreated;

    static void CALLBACK WinEventProc(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD idEventThread,
        DWORD dwmsEventTime
    );

    static WindowManager* s_instance;
};
