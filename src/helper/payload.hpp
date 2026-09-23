#pragma once

#include <windows.h>

#define WDA_EXCLUDEFROMCAPTURE_FLAG 0x00000011

// Structure passed to the remote function inside target process
#pragma pack(push, 8)
struct AffinityPayloadData {
    HWND targetHwnd;
    DWORD desiredAffinity;
    DWORD resultCode; // Output: 1 for success, 0 for failure
    DWORD lastError;  // Output: GetLastError() if failed
};
#pragma pack(pop)

#ifdef HIDDER_PAYLOAD_EXPORTS
#define PAYLOAD_API __declspec(dllexport)
#else
#define PAYLOAD_API __declspec(dllimport)
#endif

extern "C" {
    // Exported function that can be called directly or via CreateRemoteThread
    PAYLOAD_API DWORD WINAPI ApplyAffinityRemote(LPVOID lpParam);
    
    // Direct C-callable export
    PAYLOAD_API BOOL SetAffinityDirect(HWND hwnd, DWORD affinity);
}
