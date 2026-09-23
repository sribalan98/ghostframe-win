#pragma once

#include <windows.h>
#include <string>

struct WindowInfo {
    HWND hwnd;
    DWORD processId;
    std::wstring processName;
    std::wstring executablePath;
    std::wstring windowTitle;
    DWORD currentAffinity;
    bool isExcludedFromCapture;

    WindowInfo()
        : hwnd(NULL),
          processId(0),
          currentAffinity(0),
          isExcludedFromCapture(false) {}
};
