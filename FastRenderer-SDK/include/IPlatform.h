#pragma once
#include <string>
#include <functional>
#include <filesystem>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

class IPlatform {
public:
    virtual ~IPlatform() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual void initImGui() = 0;
    virtual void newFrame() = 0;
    virtual void renderDrawData() = 0;
    virtual void shutdownImGui() = 0;
    virtual float getFrameTime() = 0;
    virtual Vec2 getDisplaySize() = 0;

    virtual bool isKeyDown(int vk) = 0;
    virtual Vec2 getMousePos() = 0;
    virtual void setCursorVisible(bool visible) = 0;
    virtual void setInputBlocked(bool blocked) = 0;

    virtual void sendPacket(std::string_view channel, std::string_view data) = 0;
    virtual void onPacket(std::string_view channel,
        std::function<void(std::string_view)> callback) = 0;

    virtual std::filesystem::path getDataDir() = 0;
    virtual std::filesystem::path getGuiDefsDir() = 0;
    virtual std::filesystem::path getConfigDir() = 0;

    virtual bool loadFont(const std::string& name, const std::string& path,
        float size) = 0;
};
