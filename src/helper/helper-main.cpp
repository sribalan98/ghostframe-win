#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>
#include "payload.hpp"

// Utility to get base address of a loaded module in a remote process
static DWORD_PTR GetRemoteModuleBase(DWORD pid, const std::wstring& moduleName) {
    DWORD_PTR baseAddress = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W me;
        me.dwSize = sizeof(me);
        if (Module32FirstW(snapshot, &me)) {
            do {
                if (_wcsicmp(me.szModule, moduleName.c_str()) == 0) {
                    baseAddress = reinterpret_cast<DWORD_PTR>(me.modBaseAddr);
                    break;
                }
            } while (Module32NextW(snapshot, &me));
        }
        CloseHandle(snapshot);
    }
    return baseAddress;
}

// Get the directory containing the current executable
static std::wstring GetExecutableDir() {
    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring strPath(path);
    size_t pos = strPath.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        return strPath.substr(0, pos);
    }
    return L".";
}

// Perform injection and invoke ApplyAffinityRemote
static bool InjectAndSetAffinity(DWORD pid, HWND targetHwnd, DWORD affinity, DWORD& outError) {
    outError = ERROR_SUCCESS;

    // Enable SeDebugPrivilege if possible
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        TOKEN_PRIVILEGES tp;
        LUID luid;
        if (LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &luid)) {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
        }
        CloseHandle(hToken);
    }

    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
        FALSE,
        pid
    );

    if (!hProcess) {
        outError = GetLastError();
        std::wcerr << L"[Helper] OpenProcess failed for PID " << pid << L", Error: " << outError << std::endl;
        return false;
    }

    std::wstring payloadName = L"ghostframe-payload64.dll";
    std::wstring payloadPath = GetExecutableDir() + L"\\" + payloadName;

    if (GetFileAttributesW(payloadPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        payloadName = L"hidder-payload64.dll";
        payloadPath = GetExecutableDir() + L"\\" + payloadName;
    }

    if (GetFileAttributesW(payloadPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        outError = ERROR_FILE_NOT_FOUND;
        std::wcerr << L"[GhostFrame Helper] Payload DLL not found at: " << payloadPath << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // 1. Check if payload is already loaded in target process
    DWORD_PTR remoteModBase = GetRemoteModuleBase(pid, payloadName);

    if (remoteModBase == 0) {
        // Not loaded yet, inject it via LoadLibraryW
        size_t pathBytes = (payloadPath.length() + 1) * sizeof(wchar_t);
        LPVOID remotePathMem = VirtualAllocEx(hProcess, NULL, pathBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remotePathMem) {
            outError = GetLastError();
            CloseHandle(hProcess);
            return false;
        }

        if (!WriteProcessMemory(hProcess, remotePathMem, payloadPath.c_str(), pathBytes, NULL)) {
            outError = GetLastError();
            VirtualFreeEx(hProcess, remotePathMem, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        LPVOID pLoadLibraryW = reinterpret_cast<LPVOID>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoadLibraryW), remotePathMem, 0, NULL);
        if (!hThread) {
            outError = GetLastError();
            VirtualFreeEx(hProcess, remotePathMem, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        WaitForSingleObject(hThread, 5000);
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, remotePathMem, 0, MEM_RELEASE);

        // Retrieve module base after injection
        remoteModBase = GetRemoteModuleBase(pid, payloadName);
        if (remoteModBase == 0) {
            outError = ERROR_MOD_NOT_FOUND;
            std::wcerr << L"[Helper] Failed to locate injected module in target process." << std::endl;
            CloseHandle(hProcess);
            return false;
        }
    }

    // 2. Resolve offset of ApplyAffinityRemote from local module
    HMODULE localMod = LoadLibraryExW(payloadPath.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!localMod) {
        outError = GetLastError();
        CloseHandle(hProcess);
        return false;
    }

    FARPROC localFunc = GetProcAddress(localMod, "ApplyAffinityRemote");
    if (!localFunc) {
        outError = GetLastError();
        FreeLibrary(localMod);
        CloseHandle(hProcess);
        return false;
    }

    DWORD_PTR funcOffset = reinterpret_cast<DWORD_PTR>(localFunc) - reinterpret_cast<DWORD_PTR>(localMod);
    FreeLibrary(localMod);

    LPTHREAD_START_ROUTINE remoteFunc = reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteModBase + funcOffset);

    // 3. Allocate and write AffinityPayloadData into target process
    AffinityPayloadData payloadData;
    payloadData.targetHwnd = targetHwnd;
    payloadData.desiredAffinity = affinity;
    payloadData.resultCode = 0;
    payloadData.lastError = ERROR_SUCCESS;

    LPVOID remoteDataMem = VirtualAllocEx(hProcess, NULL, sizeof(payloadData), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteDataMem) {
        outError = GetLastError();
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, remoteDataMem, &payloadData, sizeof(payloadData), NULL)) {
        outError = GetLastError();
        VirtualFreeEx(hProcess, remoteDataMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 4. Execute remote function
    HANDLE hExecThread = CreateRemoteThread(hProcess, NULL, 0, remoteFunc, remoteDataMem, 0, NULL);
    if (!hExecThread) {
        outError = GetLastError();
        VirtualFreeEx(hProcess, remoteDataMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hExecThread, 5000);
    CloseHandle(hExecThread);

    // Read back results
    ReadProcessMemory(hProcess, remoteDataMem, &payloadData, sizeof(payloadData), NULL);
    VirtualFreeEx(hProcess, remoteDataMem, 0, MEM_RELEASE);

    // 5. Unload payload DLL from target process to release file locks and prevent UI runtime interference
    LPVOID pFreeLibrary = reinterpret_cast<LPVOID>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "FreeLibrary"));
    if (pFreeLibrary && remoteModBase) {
        HANDLE hFreeThread = CreateRemoteThread(hProcess, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pFreeLibrary), reinterpret_cast<LPVOID>(remoteModBase), 0, NULL);
        if (hFreeThread) {
            WaitForSingleObject(hFreeThread, 3000);
            CloseHandle(hFreeThread);
        }
    }

    CloseHandle(hProcess);

    outError = payloadData.lastError;
    return (payloadData.resultCode == 1);
}

int wmain(int argc, wchar_t* argv[]) {
    DWORD targetPid = 0;
    HWND targetHwnd = NULL;
    DWORD desiredAffinity = WDA_EXCLUDEFROMCAPTURE_FLAG; // Default: hide

    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"--pid" && i + 1 < argc) {
            targetPid = std::wcstoul(argv[++i], nullptr, 10);
        } else if (arg == L"--hwnd" && i + 1 < argc) {
            targetHwnd = reinterpret_cast<HWND>(std::wcstoull(argv[++i], nullptr, 16));
        } else if (arg == L"--affinity" && i + 1 < argc) {
            desiredAffinity = std::wcstoul(argv[++i], nullptr, 10);
        }
    }

    if (targetPid == 0 && targetHwnd != NULL) {
        GetWindowThreadProcessId(targetHwnd, &targetPid);
    }

    if (targetPid == 0) {
        std::wcout << L"Usage: hidder-helper64.exe --pid <PID> [--hwnd <0xHWND>] [--affinity <0|17>]" << std::endl;
        std::wcout << L"  --affinity 17 : WDA_EXCLUDEFROMCAPTURE (Hide window from capture)" << std::endl;
        std::wcout << L"  --affinity 0  : WDA_NONE (Restore capture visibility)" << std::endl;
        return 1;
    }

    DWORD err = 0;
    bool success = InjectAndSetAffinity(targetPid, targetHwnd, desiredAffinity, err);

    if (success) {
        std::wcout << L"[SUCCESS] Affinity " << desiredAffinity 
                   << L" applied to PID " << targetPid 
                   << L" HWND " << targetHwnd << std::endl;
        return 0;
    } else {
        std::wcerr << L"[FAILED] Could not apply affinity. LastError: " << err << std::endl;
        return (err != 0) ? err : 2;
    }
}
