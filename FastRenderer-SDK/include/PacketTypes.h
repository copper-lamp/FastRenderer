#pragma once
// ═══════════════════════════════════════════════════════════
//  DEPRECATED — LeviLamina custom packet type definitions.
//  Replaced by TCP Bridge JSON messages (see FR-TCP桥接通信协议.md).
//  Kept for reference only; no longer used in network layer.
// ═══════════════════════════════════════════════════════════
#include <cstdint>
#include <string>
#include <string_view>

namespace FastPackets {

inline uint64_t hashId(std::string_view str) {
    uint64_t h = 14695981039346656037ULL;
    for (auto c : str) {
        h ^= static_cast<uint8_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

enum class Id : uint64_t {
    GuiRegistration = hashId("fastrenderer:gui_reg"),
    GuiUnregistration = hashId("fastrenderer:gui_unreg"),
    KeybindRegistration = hashId("fastrenderer:keybind_reg"),
    KeybindUnregistration = hashId("fastrenderer:keybind_unreg"),
    GuiEvent = hashId("fastrenderer:gui_event"),
    DataExchange = hashId("fastrenderer:data_exchange"),
};

struct GuiRegistrationPayload {
    std::string pluginId;
    std::string guiId;
    std::string jsonContent;
    int version = 1;
};

struct GuiUnregistrationPayload {
    std::string pluginId;
    std::string guiId;
};

struct KeybindRegistrationPayload {
    std::string pluginId;
    std::string bindId;
    std::string bindName;
    int vkCode = 0;
};

struct GuiEventPayload {
    std::string guiId;
    std::string controlId;
    std::string eventType;
    std::string value;
};

struct DataExchangePayload {
    std::string channel;
    std::string data;
};

}
