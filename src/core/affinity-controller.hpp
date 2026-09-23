#pragma once

#include "window-info.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>

class AffinityController {
public:
    AffinityController();
    ~AffinityController();

    // Toggle exclusion on a specific window
    bool SetWindowExcluded(HWND hwnd, bool exclude);

    // Toggle exclusion on all windows of a process
    bool SetProcessExcluded(DWORD pid, bool exclude);

    // Check if an HWND is currently tracked as excluded
    bool IsTrackedExcluded(HWND hwnd);

    // Check if a process ID is currently tracked as excluded
    bool IsProcessExcluded(DWORD pid);

    // Restore all previously hidden windows to normal visibility (WDA_NONE)
    void RestoreAllWindows();

    // Get list of tracked excluded HWNDs
    std::vector<HWND> GetExcludedWindows();

    // Focus / Solo Mode: Hide all windows EXCEPT the target application
    bool EnterFocusMode(DWORD targetPid, const std::wstring& targetProcName, const std::vector<WindowInfo>& allWindows);
    bool ExitFocusMode();
    bool IsFocusModeActive() const;
    DWORD GetFocusedPid() const;
    std::wstring GetFocusedProcessName() const;

private:
    std::wstring GetHelperExecutablePath();
    bool ExecuteHelper(DWORD pid, HWND hwnd, DWORD affinity);

    std::unordered_set<HWND> excludedWindows;
    std::unordered_set<DWORD> excludedPids;
    bool isFocusMode = false;
    DWORD focusedPid = 0;
    std::wstring focusedProcessName;
    std::mutex stateMutex;
};
