#include <Windows.h>

#include "bang_helpers.h"

namespace bang
{

// --- memory management --- //

HANDLE g_Heap = nullptr;

void __cdecl memcpy(void* dest, size_t dest_size, void* src, size_t num)
{
    for (size_t i = 0; i < num && i < dest_size; i++)
    {
        reinterpret_cast<char*>(dest)[i] = reinterpret_cast<char*>(src)[i];
    }
}

void __cdecl memset(void* ptr, int value, size_t num)
{
    for (size_t i = 0; i < num; i++)
    {
        reinterpret_cast<char*>(ptr)[i] = value;
    }
}

// --- InternetHandle --- //

InternetHandle::InternetHandle(HINTERNET handle) : m_handle(handle) {}

InternetHandle::~InternetHandle()
{
    Reset();
}

void InternetHandle::Reset(HINTERNET other)
{
    if (m_handle)
    {
        WinHttpCloseHandle(m_handle);
    }
    m_handle = other;
}

HINTERNET InternetHandle::Release()
{
    HINTERNET handle = m_handle;
    m_handle = nullptr;
    return handle;
}

// --- ObjectHandle --- //

ObjectHandle::ObjectHandle(HANDLE handle) : m_handle(handle) {}

ObjectHandle::~ObjectHandle()
{
    Reset();
}

void ObjectHandle::Reset(HANDLE other)
{
    if (m_handle)
    {
        CloseHandle(m_handle);
    }
    m_handle = other;
}

HANDLE ObjectHandle::Release()
{
    HANDLE handle = m_handle;
    m_handle = nullptr;
    return handle;
}

// --- MutexLock --- //

MutexLock::MutexLock(const HANDLE mutex, DWORD timeout) : m_mutex(mutex)
{
    if (WAIT_OBJECT_0 == WaitForSingleObject(m_mutex, timeout))
    {
       m_held = true;
    }
}

MutexLock::~MutexLock()
{
    if (m_mutex && m_held)
    {
        ReleaseMutex(m_mutex);
    }
}

}  // namespace bang
