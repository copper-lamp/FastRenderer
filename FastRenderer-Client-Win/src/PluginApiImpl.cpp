#include <FastRendererAPI.h>
#include <gui/GuiService.h>
#include <gui/JsonGuiRenderer.h>
#include <input/KeybindService.h>
#include <nlohmann/json.hpp>

class FastRendererApiImpl : public IFastRendererAPI {
public:
    static FastRendererApiImpl& getInstanceImpl() {
        static FastRendererApiImpl instance;
        return instance;
    }

    bool registerGui(const std::string& pluginId,
        const std::string& guiId, const nlohmann::json& definition) override
    {
        GuiService::registerGui(pluginId, guiId, definition.dump());
        return true;
    }

    bool unregisterGui(const std::string& pluginId,
        const std::string& guiId) override
    {
        GuiService::unregisterGui(pluginId, guiId);
        return true;
    }

    bool registerKeybind(const std::string& pluginId,
        const std::string& bindId, const std::string& name, int vkCode) override
    {
        return KeybindService::registerKeybind(pluginId, bindId, name, vkCode, nullptr);
    }

    bool unregisterKeybind(const std::string& pluginId,
        const std::string& bindId) override
    {
        return KeybindService::unregisterKeybind(pluginId, bindId);
    }

    void publish(const std::string& channel,
        const std::string& data) override
    {
        // Local pub/sub - will be extended for network in Phase 1.5
    }

    void subscribe(const std::string& channel,
        std::function<void(const std::string&)> callback) override
    {
        // Local pub/sub - will be extended for network in Phase 1.5
    }

    void onGuiEvent(const std::string& guiId,
        std::function<void(const std::string& controlId,
            const std::string& eventType, const std::string& value)> callback) override
    {
        JsonGuiRenderer::setEventCallback([guiId, callback](const std::string& eventName) {
            if (callback) {
                callback("", eventName, "");
            }
        });
    }

    bool isServerConnected() override {
        return false; // Network layer not yet active
    }
};

IFastRendererAPI* IFastRendererAPI::getInstance() {
    return &FastRendererApiImpl::getInstanceImpl();
}
