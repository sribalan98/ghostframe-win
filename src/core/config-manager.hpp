#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>

class ConfigManager {
public:
    ConfigManager();
    explicit ConfigManager(const std::wstring& configFilePath);

    bool Load();
    bool Save();

    void AddAutoExcludeProcess(const std::wstring& processName);
    void RemoveAutoExcludeProcess(const std::wstring& processName);
    bool ShouldAutoExclude(const std::wstring& processName);

    std::vector<std::wstring> GetAutoExcludeList();

private:
    std::wstring configPath;
    std::unordered_set<std::wstring> autoExcludeProcesses;
    std::mutex configMutex;

    static std::wstring ToLower(const std::wstring& str);
};
