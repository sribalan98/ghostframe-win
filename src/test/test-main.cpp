#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "../core/window-manager.hpp"
#include "../core/affinity-controller.hpp"
#include "../core/config-manager.hpp"

static void PrintHeader() {
    std::wcout << L"\n=======================================================================\n";
    std::wcout << L"       OBS WINDOW HIDER (hidder-win) - Standalone POC Verifier         \n";
    std::wcout << L"=======================================================================\n";
    std::wcout << L" This tool tests the Windows DWM capture exclusion API cross-process.  \n";
    std::wcout << L" When hidden, the window remains visible on your screen, but DISAPPEARS\n";
    std::wcout << L" from OBS Display Capture, Snipping Tool (Win+Shift+S), and recordings!\n";
    std::wcout << L"=======================================================================\n\n";
}

int wmain(int argc, wchar_t* argv[]) {
    (void)argc;
    (void)argv;
    // Enable Unicode console output
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    PrintHeader();

    WindowManager winMgr;
    AffinityController affinityCtrl;
    ConfigManager configMgr;

    while (true) {
        std::vector<WindowInfo> windows = winMgr.GetActiveWindows();

        std::wcout << std::left
                   << std::setw(5)  << L"#"
                   << std::setw(8)  << L"PID"
                   << std::setw(14) << L"STATUS"
                   << std::setw(22) << L"PROCESS"
                   << L"WINDOW TITLE" << std::endl;
        std::wcout << std::wstring(71, L'-') << std::endl;

        for (size_t i = 0; i < windows.size(); ++i) {
            const auto& win = windows[i];
            std::wstring statusStr = win.isExcludedFromCapture ? L"[HIDDEN]" : L"[CAPTURED]";

            std::wstring titleDisplay = win.windowTitle;
            if (titleDisplay.length() > 35) {
                titleDisplay = titleDisplay.substr(0, 32) + L"...";
            }

            std::wcout << std::left
                       << std::setw(5)  << (i + 1)
                       << std::setw(8)  << win.processId
                       << std::setw(14) << statusStr
                       << std::setw(22) << win.processName
                       << titleDisplay << std::endl;
        }

        std::wcout << L"\nCommands:\n";
        std::wcout << L"  [1 - " << windows.size() << L"] : Toggle Hide/Unhide for that window\n";
        std::wcout << L"  [R]     : Refresh window list\n";
        std::wcout << L"  [U]     : Unhide / restore ALL windows\n";
        std::wcout << L"  [Q]     : Quit\n";
        std::wcout << L"Enter choice: ";

        std::wstring input;
        std::getline(std::wcin, input);

        if (input.empty()) continue;

        if (input == L"Q" || input == L"q") {
            break;
        } else if (input == L"R" || input == L"r") {
            std::wcout << L"\nRefreshing...\n\n";
            continue;
        } else if (input == L"U" || input == L"u") {
            affinityCtrl.RestoreAllWindows();
            std::wcout << L"\n>> All tracked windows restored to normal capture visibility.\n\n";
            continue;
        }

        try {
            size_t idx = std::stoul(input);
            if (idx >= 1 && idx <= windows.size()) {
                const auto& target = windows[idx - 1];
                bool currentHidden = target.isExcludedFromCapture;
                bool newHiddenState = !currentHidden;

                std::wcout << L"\n>> " << (newHiddenState ? L"Hiding" : L"Restoring")
                           << L" \"" << target.windowTitle << L"\" (" << target.processName << L")...\n";

                bool ok = affinityCtrl.SetWindowExcluded(target.hwnd, newHiddenState);
                if (ok) {
                    if (newHiddenState) {
                        std::wcout << L">> [SUCCESS] Window is now HIDDEN from screen capture!\n";
                        std::wcout << L">> [TEST IT NOW] Press Win + Shift + S to take a screenshot,\n";
                        std::wcout << L"   or check OBS Display Capture / Discord. The window will NOT appear!\n\n";
                    } else {
                        std::wcout << L">> [SUCCESS] Window capture visibility restored.\n\n";
                    }
                } else {
                    std::wcout << L">> [FAILED] Could not toggle display affinity.\n";
                    std::wcout << L"   Note: Target app might be running as Administrator. If so, run this tool as Admin.\n\n";
                }
            } else {
                std::wcout << L">> Invalid number. Try again.\n\n";
            }
        } catch (...) {
            std::wcout << L">> Unrecognized input. Try again.\n\n";
        }
    }

    std::wcout << L"\nCleaning up and restoring windows before exit...\n";
    affinityCtrl.RestoreAllWindows();
    std::wcout << L"Exited successfully.\n";
    return 0;
}
