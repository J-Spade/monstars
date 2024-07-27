#ifndef _BANG_HTTP_H_
#define _BANG_HTTP_H_

#include "bang_helpers.h"

namespace bang
{

struct Creds
{
    wchar_t Domain[MAX_PATH];
    wchar_t User[MAX_PATH];
    wchar_t Password[MAX_PATH];
};

class HttpClient
{
public:
    HttpClient(const wchar_t* hostname, const wchar_t* endpoint, const wchar_t* authtoken);
    
    bool SendCreds(const wchar_t* domain, const wchar_t* username, const wchar_t* password);

    bool Forbidden() const { return m_forbidden; };

    operator bool() const { return m_connected; };

private:
    const wchar_t* m_hostname = nullptr;
    const wchar_t* m_endpoint = nullptr;
    const wchar_t* m_authtoken = nullptr;

    bool m_connected = false;
    bool m_forbidden = false;

    InternetHandle m_session;
    InternetHandle m_connection;
};

}  // namespace bang

#endif  //_BANG_HTTP_H_