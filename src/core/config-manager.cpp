#include "config-manager.hpp"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <algorithm>

ConfigManager::ConfigManager() {
    wchar_t appDataPath[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring dir = std::wstring(appDataPath) + L"\\GhostFrame";
        CreateDirectoryW(dir.c_str(), NULL);
        configPath = dir + L"\\rules.txt";

        // Check if legacy rules exist and copy over if new file doesn't exist
        std::wstring legacyPath = std::wstring(appDataPath) + L"\\obs-studio\\plugin_config\\obs-hidder-win\\rules.txt";
        if (GetFileAttributesW(configPath.c_str()) == INVALID_FILE_ATTRIBUTES &&
            GetFileAttributesW(legacyPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            CopyFileW(legacyPath.c_str(), configPath.c_str(), TRUE);
        }
    } else {
        configPath = L"ghostframe-rules.txt";
    }
    Load();
}

ConfigManager::ConfigManager(const std::wstring& path) : configPath(path) {
    Load();
}

std::wstring ConfigManager::ToLower(const std::wstring& str) {
    std::wstring lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    return lower;
}

bool ConfigManager::Load() {
    std::lock_guard<std::mutex> lock(configMutex);
    autoExcludeProcesses.clear();

    std::wifstream infile(configPath);
    if (!infile.is_open()) {
        return false;
    }

    std::wstring line;
    while (std::getline(infile, line)) {
        // Strip whitespace and carriage return
        line.erase(std::remove(line.begin(), line.end(), L'\r'), line.end());
        line.erase(std::remove(line.begin(), line.end(), L'\n'), line.end());
        if (!line.empty() && line[0] != L'#') {
            autoExcludeProcesses.insert(ToLower(line));
        }
    }
    return true;
}

bool ConfigManager::Save() {
    std::lock_guard<std::mutex> lock(configMutex);
    std::wofstream outfile(configPath);
    if (!outfile.is_open()) {
        return false;
    }

    outfile << L"# OBS Window Hider Auto-Exclude Rules\n";
    outfile << L"# Add process names (e.g. discord.exe, telegram.exe, whatsapp.exe)\n";
    for (const auto& proc : autoExcludeProcesses) {
        outfile << proc << L"\n";
    }
    return true;
}

void ConfigManager::AddAutoExcludeProcess(const std::wstring& processName) {
    if (processName.empty()) return;
    {
        std::lock_guard<std::mutex> lock(configMutex);
        autoExcludeProcesses.insert(ToLower(processName));
    }
    Save();
}

void ConfigManager::RemoveAutoExcludeProcess(const std::wstring& processName) {
    if (processName.empty()) return;
    {
        std::lock_guard<std::mutex> lock(configMutex);
        autoExcludeProcesses.erase(ToLower(processName));
    }
    Save();
}

bool ConfigManager::ShouldAutoExclude(const std::wstring& processName) {
    if (processName.empty()) return false;
    std::lock_guard<std::mutex> lock(configMutex);
    return (autoExcludeProcesses.find(ToLower(processName)) != autoExcludeProcesses.end());
}

std::vector<std::wstring> ConfigManager::GetAutoExcludeList() {
    std::lock_guard<std::mutex> lock(configMutex);
    return std::vector<std::wstring>(autoExcludeProcesses.begin(), autoExcludeProcesses.end());
}
