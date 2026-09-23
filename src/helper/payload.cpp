#include "payload.hpp"

// Enumerate windows for the current process if targetHwnd is NULL
struct EnumProcessWindowsContext {
    DWORD processId;
    DWORD desiredAffinity;
    DWORD successCount;
};

static BOOL CALLBACK EnumProcessWindowsCallback(HWND hwnd, LPARAM lParam) {
    EnumProcessWindowsContext* ctx = reinterpret_cast<EnumProcessWindowsContext*>(lParam);
    if (!ctx) return FALSE;

    if (!hwnd || !IsWindow(hwnd)) return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == ctx->processId) {
        // Only touch visible windows
        if (!IsWindowVisible(hwnd)) {
            return TRUE;
        }

        // Skip message-only windows
        if (GetParent(hwnd) == HWND_MESSAGE) {
            return TRUE;
        }

        // Only apply to top-level or popup windows (skip internal child control windows)
        HWND parent = GetParent(hwnd);
        if (parent != NULL && parent != GetDesktopWindow()) {
            LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
            if (!(style & WS_POPUP)) {
                return TRUE;
            }
        }

        // Skip zero-sized / off-screen stub windows
        RECT rc = { 0 };
        if (GetWindowRect(hwnd, &rc)) {
            if ((rc.right - rc.left) <= 1 || (rc.bottom - rc.top) <= 1) {
                return TRUE;
            }
        }

        if (SetWindowDisplayAffinity(hwnd, ctx->desiredAffinity)) {
            ctx->successCount++;
        }
    }
    return TRUE;
}

extern "C" {

PAYLOAD_API BOOL SetAffinityDirect(HWND hwnd, DWORD affinity) {
    if (!hwnd || !IsWindow(hwnd)) {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    return SetWindowDisplayAffinity(hwnd, affinity);
}

PAYLOAD_API DWORD WINAPI ApplyAffinityRemote(LPVOID lpParam) {
    if (!lpParam) {
        return ERROR_INVALID_PARAMETER;
    }

    AffinityPayloadData* data = reinterpret_cast<AffinityPayloadData*>(lpParam);
    data->resultCode = 0;
    data->lastError = ERROR_SUCCESS;

    EnumProcessWindowsContext ctx;
    ctx.processId = GetCurrentProcessId();
    ctx.desiredAffinity = data->desiredAffinity;
    ctx.successCount = 0;

    // 1. If a specific target window was provided, set affinity on it first
    if (data->targetHwnd && IsWindow(data->targetHwnd)) {
        if (SetWindowDisplayAffinity(data->targetHwnd, data->desiredAffinity)) {
            ctx.successCount++;
        }
    }

    // 2. Also apply affinity to ALL windows and popups owned by this process
    // (Protects Discord voice overlays, context menus, popup chat windows, Chrome tabs/dialogs)
    EnumWindows(EnumProcessWindowsCallback, reinterpret_cast<LPARAM>(&ctx));

    if (ctx.successCount > 0) {
        data->resultCode = 1;
        data->lastError = ERROR_SUCCESS;
    } else {
        data->resultCode = 0;
        data->lastError = GetLastError();
    }

    return (data->resultCode == 1) ? ERROR_SUCCESS : data->lastError;
}

} // extern "C"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID /*lpvReserved*/) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
