#pragma once

#include "Base/JsonParser.h"
#include "EngineInterface/GpuSettings.h"

#include "Definitions.h"

struct GlobalSettingsImpl;

class GlobalSettings
{
public:
    static constexpr const char* VideoMode = "settings.display.video mode";
    static constexpr const char* Fullscreen = "settings.display.fullscreen";
    static constexpr const char* WindowWidth = "settings.display.window width";
    static constexpr const char* WindowHeight = "settings.display.window height";

    static constexpr int DefaultWindowWidth = 1920;
    static constexpr int DefaultWindowHeight = 1080;

public:
    static GlobalSettings& getInstance();

    GlobalSettings(GlobalSettings const&) = delete;
    void operator=(GlobalSettings const&) = delete;

    GpuSettings getGpuSettings();
    void setGpuSettings(GpuSettings gpuSettings);

    bool getBoolState(std::string const& name, bool defaultValue);
    void setBoolState(std::string const& name, bool value);

    int getIntState(std::string const& name, int defaultValue);
    void setIntState(std::string const& name, int value);

private:
    GlobalSettings();
    ~GlobalSettings();

    void encodeDecodeGpuSettings(GpuSettings& gpuSettings, ParserTask task);

    GlobalSettingsImpl* _impl;
};