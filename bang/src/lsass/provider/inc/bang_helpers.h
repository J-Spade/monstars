#ifndef _BANG_HELPERS_H_
#define _BANG_HELPERS_H_

#include <Windows.h>
#include <winhttp.h>

// --- debug macros --- //

#ifdef _DEBUG

#define DEBUG_PRINTW(fmt, ...)              \
do {                                        \
    wchar_t printbuf[MAX_PATH];             \
    wsprintfW(printbuf, fmt, __VA_ARGS__);  \
    OutputDebugStringW(printbuf);           \
} while (0);

#else

#define DEBUG_PRINTW(...)

#endif

namespace bang
{

// --- memory management --- //

extern HANDLE g_Heap;

void __cdecl memcpy(void* dest, size_t dest_size, void* src, size_t num);

void __cdecl memset(void* ptr, int value, size_t num);

// --- RAII handle types --- //

struct InternetHandle
{
public:
    InternetHandle(HINTERNET handle = nullptr);
    ~InternetHandle();

    void Reset(HINTERNET other = nullptr);
    HINTERNET Release();

    operator HINTERNET() const { return m_handle; };
    operator bool() const { return m_handle != nullptr; };
    void operator= (HINTERNET h) { Reset(h); };

private:
    HINTERNET m_handle;
};

struct ObjectHandle
{
public:
    ObjectHandle(HANDLE handle = nullptr);
    ~ObjectHandle();

    void Reset(HANDLE other = nullptr);
    HANDLE Release();

    operator HANDLE() const { return m_handle; };
    operator bool() const { return m_handle != nullptr; };
    void operator= (HANDLE h) { Reset(h); };

private:
    HANDLE m_handle;
};

struct MutexLock
{
public:
    MutexLock(const HANDLE mutex, DWORD timeout = INFINITE);
    ~MutexLock();

    operator bool() const { return m_held; };

private:
    const HANDLE m_mutex;
    bool m_held = false;
};

}


#endif  // _BANG_HELPERS_H_ 