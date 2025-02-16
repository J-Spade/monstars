#ifndef _JAM_MUTEX_H_
#define _JAM_MUTEX_H_

#include <Windows.h>


namespace monstars
{

class MutexLock
{
public:
    MutexLock(const HANDLE mutex, DWORD timeout = INFINITE);
    ~MutexLock();

    operator bool() const { return m_held; };

private:
    const HANDLE m_mutex;
    bool m_held = false;
};


}  // namespace monstars


#endif  // _JAM_MUTEX_H_