#include <Windows.h>
#include <WinUser.h>

HINSTANCE g_instance = nullptr;
HWND g_window = nullptr;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    g_instance = hInstance;
    g_window = CreateWindowExW(WS_EX_NOACTIVATE, L"STATIC", nullptr, WS_OVERLAPPEDWINDOW, 0, 0,
        0, 0, HWND_MESSAGE, nullptr, g_instance, nullptr);

    AddClipboardFormatListener(g_window);

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
                OutputDebugStringW(L"clipboard updated\n");
                if (IsClipboardFormatAvailable(CF_TEXT))
                {
                    if (OpenClipboard(g_window))
                    {
                        HANDLE data = GetClipboardData(CF_TEXT);
                        if (data)
                        {
                            auto text = static_cast<char*>(GlobalLock(data));
                            if (text)
                            {
                                OutputDebugStringA(text);
                                GlobalUnlock(text);
                            }
                        }
                        CloseClipboard();
                    }
                }
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return ERROR_SUCCESS;
}