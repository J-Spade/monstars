#include <Windows.h>
#include <WinUser.h>

HINSTANCE g_instance = NULL;
HWND g_window = NULL;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    g_instance = hInstance;
    g_window = CreateWindowExW(WS_EX_NOACTIVATE, L"STATIC", nullptr, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, nullptr, g_instance, nullptr);

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
            DispatchMessage(&msg);
        }
    }

    return ERROR_SUCCESS;
}