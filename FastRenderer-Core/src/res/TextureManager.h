#pragma once
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <cstdio>
#include <thread>
#include <chrono>
#include <cstdint>

// stb_image for decoding PNG/JPG
#include <stb/stb_image.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "winhttp.lib")
#endif

#include <imgui.h>

// ─── Simple HTTP download using Windows API (WinHTTP)
// avoids dependency on cpr at compile time for the Core module
// ═══════════════════════════════════════════════════════════

namespace TextureManager {

struct TextureEntry {
    ImTextureID textureId = (ImTextureID)0;
    int width = 0;
    int height = 0;
    bool loading = false;
    bool loaded = false;
    bool failed = false;
    std::string url;
    std::chrono::steady_clock::time_point lastAccess;
};

// ─── Internal state ───
inline std::map<std::string, TextureEntry> g_textures;
inline std::mutex g_mutex;
inline ID3D11Device* g_device = nullptr;
inline ID3D11DeviceContext* g_context = nullptr;
inline bool g_initialized = false;

// ─── Logging ───
inline void texLog(const char* msg) {
    FILE* f = nullptr;
    fopen_s(&f, "FastRenderer_Tex.txt", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

// ─── Initialize with D3D11 device (called after DX11 init) ───
inline void init(ID3D11Device* device, ID3D11DeviceContext* context) {
    g_device = device;
    g_context = context;
    g_initialized = true;
    texLog("TextureManager initialized");
}

// ─── Download file from URL using WinHTTP ───
inline bool downloadUrl(const std::string& url, std::vector<uint8_t>& outData) {
    outData.clear();
    
    HINTERNET hSession = WinHttpOpen(L"FastRenderer/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return false;

    // Parse URL
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
    if (wcsncmp(hostName, L"https", 5) == 0 || urlComp.nScheme == INTERNET_SCHEME_HTTPS)
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

    // Read response
    DWORD bytesRead = 0;
    uint8_t buf[8192];
    while (WinHttpReadData(hRequest, buf, sizeof(buf), &bytesRead) && bytesRead > 0) {
        outData.insert(outData.end(), buf, buf + bytesRead);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    texLog(("Downloaded " + std::to_string(outData.size()) + " bytes from " + url).c_str());
    return !outData.empty();
}

// ─── Create D3D11 texture from RGBA pixel data ───
inline ImTextureID createD3DTexture(const uint8_t* rgba, int width, int height) {
    if (!g_device || !rgba || width <= 0 || height <= 0) return (ImTextureID)0;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = rgba;
    initData.SysMemPitch = width * 4;

    ID3D11Texture2D* texture = nullptr;
    if (FAILED(g_device->CreateTexture2D(&desc, &initData, &texture))) {
        texLog("CreateTexture2D failed");
        return (ImTextureID)0;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    if (FAILED(g_device->CreateShaderResourceView(texture, &srvDesc, &srv))) {
        texture->Release();
        texLog("CreateShaderResourceView failed");
        return (ImTextureID)0;
    }
    texture->Release();

    texLog(("Created texture: " + std::to_string(width) + "x" + std::to_string(height)).c_str());
    return (ImTextureID)srv;
}

// ─── Load image bytes → decode → create D3D texture ───
inline ImTextureID loadTextureFromMemory(const std::vector<uint8_t>& data, int& outW, int& outH) {
    if (data.empty()) return (ImTextureID)0;

    int w = 0, h = 0, channels = 0;
    stbi_uc* image = stbi_load_from_memory(data.data(), (int)data.size(), &w, &h, &channels, 4);
    if (!image) {
        texLog("stbi_load_from_memory failed");
        return (ImTextureID)0;
    }

    ImTextureID texId = createD3DTexture(image, w, h);
    outW = w;
    outH = h;
    stbi_image_free(image);
    return texId;
}

// ─── Public API ───

// Load a texture from URL (synchronous download + decode + GPU upload)
inline bool loadFromUrl(const std::string& url) {
    if (!g_initialized || url.empty()) return false;

    std::lock_guard<std::mutex> lock(g_mutex);

    // Check cache
    auto it = g_textures.find(url);
    if (it != g_textures.end() && it->second.loaded) {
        it->second.lastAccess = std::chrono::steady_clock::now();
        return true;
    }

    TextureEntry& entry = g_textures[url];
    if (entry.loading) return false; // Already being loaded
    entry.loading = true;
    entry.url = url;

    texLog(("Loading texture: " + url).c_str());

    // Download
    std::vector<uint8_t> rawData;
    if (!downloadUrl(url, rawData)) {
        entry.failed = true;
        entry.loading = false;
        texLog(("Download failed: " + url).c_str());
        return false;
    }

    // Decode and create GPU texture
    entry.textureId = loadTextureFromMemory(rawData, entry.width, entry.height);
    entry.loaded = (entry.textureId != (ImTextureID)0);
    entry.failed = !entry.loaded;
    entry.loading = false;
    entry.lastAccess = std::chrono::steady_clock::now();

    std::string logStr = (entry.loaded ? "OK: " : "FAIL: ") + url;
    texLog(logStr.c_str());
    return entry.loaded;
}

// Asynchronous load (starts download in background thread)
inline void loadFromUrlAsync(const std::string& url) {
    if (!g_initialized || url.empty()) return;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_textures.find(url);
        if (it != g_textures.end() && (it->second.loaded || it->second.loading)) return;
        g_textures[url].loading = true;
        g_textures[url].url = url;
    }
    std::thread([url]() { loadFromUrl(url); }).detach();
}

// Load texture from local file path
inline bool loadFromFile(const std::string& filePath) {
    if (!g_initialized || filePath.empty()) return false;

    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_textures.find(filePath);
    if (it != g_textures.end() && it->second.loaded) {
        it->second.lastAccess = std::chrono::steady_clock::now();
        return true;
    }

    TextureEntry& entry = g_textures[filePath];
    if (entry.loading) return false;
    entry.loading = true;
    entry.url = filePath;

    texLog(("Loading texture from file: " + filePath).c_str());

    // Read file
    FILE* f = nullptr;
    fopen_s(&f, filePath.c_str(), "rb");
    if (!f) { entry.failed = true; entry.loading = false; texLog("File open failed"); return false; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> rawData(sz);
    fread(rawData.data(), 1, sz, f);
    fclose(f);

    // Decode and create GPU texture
    entry.textureId = loadTextureFromMemory(rawData, entry.width, entry.height);
    entry.loaded = (entry.textureId != (ImTextureID)0);
    entry.failed = !entry.loaded;
    entry.loading = false;
    entry.lastAccess = std::chrono::steady_clock::now();

    std::string logStr = entry.loaded ? "OK: " : "FAIL: ";
    texLog((logStr + filePath).c_str());
    return entry.loaded;
}

// Get texture info for rendering
inline ImTextureID get(const std::string& id, int* outW = nullptr, int* outH = nullptr) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_textures.find(id);
    if (it != g_textures.end() && it->second.loaded) {
        it->second.lastAccess = std::chrono::steady_clock::now();
        if (outW) *outW = it->second.width;
        if (outH) *outH = it->second.height;
        return it->second.textureId;
    }
    return (ImTextureID)0;
}

inline bool isLoading(const std::string& id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_textures.find(id);
    return it != g_textures.end() && it->second.loading;
}

inline bool hasFailed(const std::string& id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_textures.find(id);
    return it != g_textures.end() && it->second.failed;
}

// Pre-register a texture from memory (for programmatic textures)
inline void registerTexture(const std::string& id, ImTextureID tex, int w, int h) {
    std::lock_guard<std::mutex> lock(g_mutex);
    TextureEntry& entry = g_textures[id];
    if (entry.loaded && entry.textureId != (ImTextureID)0) {
        // Release old texture? For now, just overwrite
    }
    entry.textureId = tex;
    entry.width = w;
    entry.height = h;
    entry.loaded = true;
    entry.url = id;
}

// Cleanup unused textures (call periodically)
inline void cleanup(int maxAgeSeconds = 300) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto now = std::chrono::steady_clock::now();
    for (auto it = g_textures.begin(); it != g_textures.end(); ) {
        if (it->second.loaded && !it->second.url.empty()) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second.lastAccess).count();
            if (age > maxAgeSeconds) {
                if (it->second.textureId) {
                    ((ID3D11ShaderResourceView*)it->second.textureId)->Release();
                }
                it = g_textures.erase(it);
                continue;
            }
        }
        ++it;
    }
}

// Release all textures
inline void shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& [id, entry] : g_textures) {
        if (entry.textureId != (ImTextureID)0) {
            ((ID3D11ShaderResourceView*)entry.textureId)->Release();
        }
    }
    g_textures.clear();
    g_device = (ID3D11Device*)0;
    g_context = (ID3D11DeviceContext*)0;
    g_initialized = false;
    texLog("TextureManager shutdown");
}

} // namespace TextureManager
