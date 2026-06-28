#pragma once
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>

// ═══════════════════════════════════════════════════════════════
//  FileLog — Reliable file-based logging.
//
//  On modern Android (API 30+), /sdcard is NOT writable by most
//  apps. We write to /data/data/com.mojang.minecraftpe/ which is
//  always writable from within MCPE's process.
//
//  View the log via:
//    adb shell cat /data/data/com.mojang.minecraftpe/FastRenderer_Log.txt
//  Or copy with root:
//    adb shell su -c cat /data/data/com.mojang.minecraftpe/FastRenderer_Log.txt
// ═══════════════════════════════════════════════════════════════

namespace {
    inline void FileLog(const char* tag, const char* fmt, ...) {
        // Try MCPE's data directory (always writable from within the process)
        FILE* f = std::fopen("/data/data/com.mojang.minecraftpe/FastRenderer_Log.txt", "a");
        if (!f) {
            // Last resort: a path that works on rooted devices
            f = std::fopen("/data/local/tmp/FastRenderer_Log.txt", "a");
            if (!f) return;
        }

        // Timestamp
        time_t now = time(nullptr);
        struct tm tmv_buf = {};
        struct tm* tmv = localtime_r(&now, &tmv_buf);
        char ts[32] = {};
        if (tmv) {
            strftime(ts, sizeof(ts), "%H:%M:%S", tmv);
        }

        std::fprintf(f, "[%s] [%s] ", ts, tag);

        va_list args;
        va_start(args, fmt);
        std::vfprintf(f, fmt, args);
        va_end(args);

        std::fprintf(f, "\n");
        std::fflush(f);
        std::fclose(f);
    }
}
