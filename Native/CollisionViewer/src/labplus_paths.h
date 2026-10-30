#pragma once
#include <windows.h>
#include <filesystem>

static std::filesystem::path labplusDataDirectory() {
    wchar_t path[32768]{};
    DWORD length=GetEnvironmentVariableW(L"LABPLUS_DATA_DIRECTORY",path,32768);
    if(length&&length<32768) return std::filesystem::path(path);
    length=GetEnvironmentVariableW(L"LOCALAPPDATA",path,32768);
    return length&&length<32768?std::filesystem::path(path)/L"labplus":std::filesystem::path{};
}
