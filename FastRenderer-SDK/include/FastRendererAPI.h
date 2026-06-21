#pragma once
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include "GuiDefinition.h"

#ifdef FR_EXPORT
#define FRAPI __declspec(dllexport)
#else
#define FRAPI __declspec(dllimport)
#endif

class FRAPI IFastRendererAPI {
public:
    virtual ~IFastRendererAPI() = default;

    static IFastRendererAPI* getInstance();

    virtual bool registerGui(const std::string& pluginId,
        const std::string& guiId, const nlohmann::json& definition) = 0;

    virtual bool unregisterGui(const std::string& pluginId,
        const std::string& guiId) = 0;

    virtual bool registerKeybind(const std::string& pluginId,
        const std::string& bindId, const std::string& name, int vkCode) = 0;

    virtual bool unregisterKeybind(const std::string& pluginId,
        const std::string& bindId) = 0;

    virtual void publish(const std::string& channel,
        const std::string& data) = 0;

    virtual void subscribe(const std::string& channel,
        std::function<void(const std::string&)> callback) = 0;

    virtual void onGuiEvent(const std::string& guiId,
        std::function<void(const std::string& controlId,
            const std::string& eventType, const std::string& value)> callback) = 0;

    virtual bool isServerConnected() = 0;
};
