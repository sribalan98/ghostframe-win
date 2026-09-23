#include "affinity-controller.hpp"
#include "window-manager.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

#ifndef WDA_NONE
#define WDA_NONE 0x00000000
#endif

// External helper to get current module handle (DLL or EXE)
extern "C" IMAGE_DOS_HEADER __ImageBase;

AffinityController::AffinityController() {}

AffinityController::~AffinityController() {
    RestoreAllWindows();
}

std::wstring AffinityController::GetHelperExecutablePath() {
    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
    std::wstring dir(path);
    size_t pos = dir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        dir = dir.substr(0, pos);
    }

    // Candidate locations for GhostFrame helper
    std::vector<std::wstring> candidates = {
        dir + L"\\ghostframe-helper64.exe",
        dir + L"\\bin\\ghostframe-helper64.exe",
        L".\\ghostframe-helper64.exe",
        L".\\bin\\ghostframe-helper64.exe",
        dir + L"\\hidder-helper64.exe",
        dir + L"\\bin\\hidder-helper64.exe"
    };

    for (const auto& candidate : candidates) {
        if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return candidate;
        }
    }

    return dir + L"\\ghostframe-helper64.exe";
}

bool AffinityController::ExecuteHelper(DWORD pid, HWND hwnd, DWORD affinity) {
    std::wstring helperPath = GetHelperExecutablePath();

    if (GetFileAttributesW(helperPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"[AffinityController] Helper not found at: " << helperPath << std::endl;
        return false;
    }

    std::wstringstream cmdStream;
    cmdStream << L"\"" << helperPath << L"\" --pid " << pid;
    if (hwnd) {
        cmdStream << L" --hwnd 0x" << std::hex << reinterpret_cast<uintptr_t>(hwnd);
    }
    cmdStream << L" --affinity " << std::dec << affinity;

    std::wstring cmdLine = cmdStream.str();

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back(L'\0');

    BOOL created = CreateProcessW(
        NULL,
        cmdBuffer.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!created) {
        std::wcerr << L"[AffinityController] Failed to launch helper, error: " << GetLastError() << std::endl;
        return false;
    }

    // Wait for helper to complete
    WaitForSingleObject(pi.hProcess, 10000);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (exitCode == 0);
}

bool AffinityController::SetWindowExcluded(HWND hwnd, bool exclude) {
    if (!hwnd || !IsWindow(hwnd)) return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return false;

    DWORD targetAffinity = exclude ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
    bool success = false;

    // If target window belongs to current process, apply directly without helper
    if (pid == GetCurrentProcessId()) {
        success = (SetWindowDisplayAffinity(hwnd, targetAffinity) != FALSE);
    } else {
        success = ExecuteHelper(pid, hwnd, targetAffinity);
    }

    if (success) {
        std::lock_guard<std::mutex> lock(stateMutex);
        if (exclude) {
            excludedWindows.insert(hwnd);
            excludedPids.insert(pid);
        } else {
            excludedWindows.erase(hwnd);
            // Only erase PID if no other windows from this PID are currently excluded
            bool hasOtherWindows = false;
            for (HWND w : excludedWindows) {
                DWORD wpid = 0;
                GetWindowThreadProcessId(w, &wpid);
                if (wpid == pid) {
                    hasOtherWindows = true;
                    break;
                }
            }
            if (!hasOtherWindows) {
                excludedPids.erase(pid);
            }
        }
    }

    return success;
}

bool AffinityController::SetProcessExcluded(DWORD pid, bool exclude) {
    if (pid == 0) return false;

    DWORD targetAffinity = exclude ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
    bool success = ExecuteHelper(pid, NULL, targetAffinity);
    if (success) {
        std::lock_guard<std::mutex> lock(stateMutex);
        if (exclude) {
            excludedPids.insert(pid);
        } else {
            excludedPids.erase(pid);
        }
    }
    return success;
}

bool AffinityController::IsTrackedExcluded(HWND hwnd) {
    std::lock_guard<std::mutex> lock(stateMutex);
    return (excludedWindows.find(hwnd) != excludedWindows.end());
}

bool AffinityController::IsProcessExcluded(DWORD pid) {
    std::lock_guard<std::mutex> lock(stateMutex);
    return (excludedPids.find(pid) != excludedPids.end());
}

void AffinityController::RestoreAllWindows() {
    std::lock_guard<std::mutex> lock(stateMutex);

    DWORD myPid = GetCurrentProcessId();

    // 1. Restore self window locally if tracked
    for (HWND hwnd : excludedWindows) {
        if (IsWindow(hwnd)) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid == myPid) {
                SetWindowDisplayAffinity(hwnd, WDA_NONE);
            }
        }
    }

    // 2. Restore remote processes (one helper call per process, NOT per window!)
    for (DWORD pid : excludedPids) {
        if (pid != 0 && pid != myPid) {
            ExecuteHelper(pid, NULL, WDA_NONE);
        }
    }

    excludedWindows.clear();
    excludedPids.clear();
    isFocusMode = false;
    focusedPid = 0;
    focusedProcessName.clear();
}

std::vector<HWND> AffinityController::GetExcludedWindows() {
    std::lock_guard<std::mutex> lock(stateMutex);
    return std::vector<HWND>(excludedWindows.begin(), excludedWindows.end());
}

bool AffinityController::EnterFocusMode(DWORD targetPid, const std::wstring& targetProcName, const std::vector<WindowInfo>& allWindows) {
    if (targetPid == 0) return false;

    // Reset any previous state first
    RestoreAllWindows();

    std::lock_guard<std::mutex> lock(stateMutex);
    isFocusMode = true;
    focusedPid = targetPid;
    focusedProcessName = targetProcName;

    DWORD myPid = GetCurrentProcessId();

    // Collect UNIQUE target PIDs to cloak (1 call per process, never duplicate)
    std::unordered_set<DWORD> pidsToExclude;
    for (const auto& win : allWindows) {
        if (win.processId == targetPid) continue; // Keep target app visible!
        if (win.processId == myPid) continue;     // Leave GhostFrame alone
        if (win.processId == 0 || win.processId == 4) continue;
        if (WindowManager::IsSystemProcess(win.processName)) continue; // Never cloak explorer / taskbar / system!

        pidsToExclude.insert(win.processId);
        excludedWindows.insert(win.hwnd);
    }

    for (DWORD pid : pidsToExclude) {
        if (ExecuteHelper(pid, NULL, WDA_EXCLUDEFROMCAPTURE)) {
            excludedPids.insert(pid);
        }
    }

    return true;
}

bool AffinityController::ExitFocusMode() {
    RestoreAllWindows();
    return true;
}

bool AffinityController::IsFocusModeActive() const {
    return isFocusMode;
}

DWORD AffinityController::GetFocusedPid() const {
    return focusedPid;
}

std::wstring AffinityController::GetFocusedProcessName() const {
    return focusedProcessName;
}
