// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#ifdef __ANDROID__
#include "../../android/erbium/compat/win32_compat.h"
#else
#include "framework.h"
#endif
#include "../SDK/Includes.h"
using namespace SDK;
#include "Erbium/Public/Utils.h"
#include <numeric>
#include <algorithm>
#ifndef __ANDROID__
#include "libcurl/curl.h"
#else
// Android port: libcurl disabled (webhook status only) — no-op stubs.
extern "C" {
typedef void CURL;
typedef void curl_slist;
typedef int CURLcode;
enum { CURLE_OK = 0 };
enum {
    CURLOPT_URL = 1, CURLOPT_HTTPHEADER = 2, CURLOPT_POSTFIELDS = 3,
    CURLOPT_WRITEDATA = 4, CURLOPT_WRITEFUNCTION = 5, CURLOPT_FOLLOWLOCATION = 6,
    CURLOPT_VERBOSE = 7
};
inline CURL* curl_easy_init() { return nullptr; }
inline CURLcode curl_easy_setopt(CURL*, int, ...) { return 0; }
inline CURLcode curl_easy_perform(CURL*) { return 0; }
inline void curl_easy_cleanup(CURL*) {}
inline curl_slist* curl_slist_append(curl_slist* l, const char*) { return l; }
inline void curl_slist_free_all(curl_slist*) {}
inline const char* curl_easy_strerror(CURLcode) { return "curl-disabled"; }
inline CURLcode curl_global_init(long) { return 0; }
}
#endif

inline UEAllocatedString iso8601()
{
    time_t now;
    time(&now);
    char buf[sizeof "2011-10-08T07:07:09Z"];
    tm* t = new tm();
    gmtime_s(t, &now);
    strftime(buf, sizeof buf, "%FT%TZ", t);
    return UEAllocatedString(buf);
}

#endif // PCH_H
