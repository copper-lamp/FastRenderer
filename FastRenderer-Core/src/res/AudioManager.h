#pragma once
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <cstdio>
#include <thread>
#include <chrono>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "winhttp.lib")

// ═══════════════════════════════════════════════════════════
//  AudioManager — 音频远程加载 + 本地播放
//  支持：WAV 文件（通过 PlaySound）
//  远程：下载到临时目录缓存后播放
// ═══════════════════════════════════════════════════════════

namespace AudioManager {

inline std::string g_cacheDir;
inline std::mutex g_mutex;

inline void audLog(const char* msg) {
    FILE* f = nullptr;
    fopen_s(&f, "FastRenderer_Audio.txt", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

// ─── Initialize with cache directory ───
inline void init(const std::string& cacheDir) {
    g_cacheDir = cacheDir;
    std::filesystem::create_directories(g_cacheDir);
    audLog(("AudioManager initialized, cache=" + g_cacheDir).c_str());
}

// ─── Download file from URL using WinHTTP ───
inline bool downloadFile(const std::string& url, const std::string& outPath) {
    HINTERNET hSession = WinHttpOpen(L"FastRenderer/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return false;

    URL_COMPONENTSW urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256] = {}, urlPath[2048] = {};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 2048;

    int urlLen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, NULL, 0);
    std::wstring wurl(urlLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, &wurl[0], urlLen);

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp)) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    DWORD flags = WINHTTP_FLAG_REFRESH;
    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS)
        flags |= WINHTTP_FLAG_SECURE;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath, NULL,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        NULL, &statusCode, &statusSize, NULL);
    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    // Read response to temp buffer
    std::vector<uint8_t> data;
    DWORD bytesRead = 0;
    uint8_t buf[8192];
    while (WinHttpReadData(hRequest, buf, sizeof(buf), &bytesRead) && bytesRead > 0) {
        data.insert(data.end(), buf, buf + bytesRead);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (data.empty()) return false;

    // Write to file
    FILE* f = nullptr;
    fopen_s(&f, outPath.c_str(), "wb");
    if (!f) return false;
    fwrite(data.data(), 1, data.size(), f);
    fclose(f);

    audLog(("Downloaded audio: " + std::to_string(data.size()) + " bytes -> " + outPath).c_str());
    return true;
}

// ─── Get cache file path for URL ───
inline std::string cachePathForUrl(const std::string& url) {
    // Simple hash-based filename
    size_t h = std::hash<std::string>{}(url);
    return g_cacheDir + "/audio_" + std::to_string(h) + ".wav";
}

// ─── Check if a URL is cached locally ───
inline bool isCached(const std::string& url) {
    std::string path = cachePathForUrl(url);
    return std::filesystem::exists(path);
}

// ─── Ensure audio file is available (download if needed) ───
inline bool ensureCached(const std::string& url) {
    if (url.empty()) return false;

    // Local file path
    if (url.find("://") == std::string::npos) {
        return std::filesystem::exists(url);
    }

    std::string cachePath = cachePathForUrl(url);
    if (std::filesystem::exists(cachePath)) return true;

    audLog(("Caching audio: " + url).c_str());
    return downloadFile(url, cachePath);
}

// ─── Play WAV file ───
inline bool play(const std::string& pathOrUrl, bool async = true) {
    std::string playPath = pathOrUrl;

    // If it's a URL, ensure it's cached
    if (pathOrUrl.find("://") != std::string::npos) {
        std::string cachePath = cachePathForUrl(pathOrUrl);
        if (!std::filesystem::exists(cachePath)) {
            if (!downloadFile(pathOrUrl, cachePath)) {
                audLog(("Play failed (download): " + pathOrUrl).c_str());
                return false;
            }
        }
        playPath = cachePath;
    }

    // Convert to wide string for PlaySound
    int len = MultiByteToWideChar(CP_UTF8, 0, playPath.c_str(), -1, NULL, 0);
    std::wstring wpath(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, playPath.c_str(), -1, &wpath[0], len);

    DWORD flags = SND_FILENAME | (async ? SND_ASYNC : SND_SYNC) | SND_NODEFAULT;
    bool result = PlaySoundW(wpath.c_str(), NULL, flags) != 0;

    std::string audStr = (result ? "Played: " : "Play FAILED: ") + playPath;
    audLog(audStr.c_str());
    return result;
}

// ─── Play a beep (programmatic tone) ───
inline void beep(int frequency = 440, int durationMs = 100) {
    Beep(frequency, durationMs);
}

// ─── Stop current playback ───
inline void stop() {
    PlaySoundW(NULL, NULL, 0);
}

// ─── Cleanup ───
inline void shutdown() {
    stop();
    audLog("AudioManager shutdown");
}

} // namespace AudioManager
