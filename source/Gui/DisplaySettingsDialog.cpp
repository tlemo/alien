#include "DisplaySettingsDialog.h"

#include <sstream>

#include <GLFW/glfw3.h>
#include "imgui.h"

#include "Base/LoggingService.h"
#include "Base/ServiceLocator.h"

#include "AlienImGui.h"
#include "GlobalSettings.h"


namespace
{
    auto const ItemTextWidth = 130.0f;
}

_DisplaySettingsDialog::_DisplaySettingsDialog(GLFWwindow* window)
    : _window(window)
{
    auto primaryMonitor = glfwGetPrimaryMonitor();
    _videoModes = glfwGetVideoModes(primaryMonitor, &_videoModesCount);
    _desktopVideoMode = glfwGetVideoMode(primaryMonitor);

    auto optimalVideoMode = getOptimalVideoModeIndex();

    auto& settings = GlobalSettings::getInstance();
    _videoModeSelection = settings.getIntState(GlobalSettings::VideoMode, optimalVideoMode);
    _fullscreen = settings.getBoolState(GlobalSettings::Fullscreen, false);

    auto videoModeStrings = createVideoModeStrings();
    auto loggingService = ServiceLocator::getInstance().getService<LoggingService>();
    loggingService->logMessage(
        Priority::Important, "desktop video settings is " + createVideoModeString(*_desktopVideoMode));

    if (_fullscreen) {
        _origVideoModeSelection = _videoModeSelection;
        if (_videoModeSelection > 0) {
            loggingService->logMessage(Priority::Important, "switching to  " + videoModeStrings[_videoModeSelection]);
            auto const& mode = _videoModes[_videoModeSelection - 1];
            glfwSetWindowMonitor(_window, primaryMonitor, 0, 0, mode.width, mode.height, mode.refreshRate);
        }
    } else {
        /*
        glfwSetWindowMonitor(_window, nullptr, 0, 0, _width, _height, GLFW_DONT_CARE);
        glfwSetWindowSize(_window, _width, _height);
        */
    }
}

_DisplaySettingsDialog::~_DisplaySettingsDialog()
{
    auto& settings = GlobalSettings::getInstance();
    settings.setIntState(GlobalSettings::VideoMode, _videoModeSelection);
    settings.setBoolState(GlobalSettings::Fullscreen, _fullscreen);
}

void _DisplaySettingsDialog::process()
{
    if (!_show) {
        return;
    }

    ImGui::OpenPopup("Display settings");
    if (ImGui::BeginPopupModal("Display settings", NULL, ImGuiWindowFlags_None)) {

        auto prevVideoModeSelection = _videoModeSelection;
        AlienImGui::Combo(
            AlienImGui::ComboParameters()
                .name("Resolution")
                .textWidth(ItemTextWidth)
                .defaultValue(_origVideoModeSelection)
                .values(createVideoModeStrings()),
            _videoModeSelection);

        if (prevVideoModeSelection != _videoModeSelection) {
            onSetVideoMode();
        }

        ImGui::Checkbox("Fullscreen", &_fullscreen);

        AlienImGui::Separator();

        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
            _show = false;
            _origVideoModeSelection = _videoModeSelection;
        }
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            _show = false;

            if (_origVideoModeSelection != _videoModeSelection) {
                _videoModeSelection = _origVideoModeSelection;
                onSetVideoMode();
            }
        }

        ImGui::EndPopup();
    }
}

void _DisplaySettingsDialog::show()
{
    _show = true;
}

void _DisplaySettingsDialog::onSetVideoMode()
{
    auto loggingService = ServiceLocator::getInstance().getService<LoggingService>();
    auto videoModeStrings = createVideoModeStrings();
    loggingService->logMessage(Priority::Important, "switching video settings to " + videoModeStrings[_videoModeSelection]);

    GLFWmonitor* primaryMonitor = _fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    if (_videoModeSelection == 0) {
        glfwSetWindowMonitor(
            _window,
            primaryMonitor,
            0,
            0,
            _desktopVideoMode->width,
            _desktopVideoMode->height,
            _desktopVideoMode->refreshRate);
    } else {
        auto mode = _videoModes[_videoModeSelection - 1];
        glfwSetWindowMonitor(_window, primaryMonitor, 0, 0, mode.width, mode.height, mode.refreshRate);
    }
}

int _DisplaySettingsDialog::getOptimalVideoModeIndex() const
{
    int result = 0;
    int refreshRate = 0;
    for (int i = 0; i < _videoModesCount; ++i) {
        if (_videoModes[i].width == 1920 && _videoModes[i].height == 1080 && _videoModes[i].refreshRate > refreshRate) {
            result = i + 1;
            refreshRate = _videoModes[i].refreshRate;
        }
    }
    return result;
}

std::string _DisplaySettingsDialog::createVideoModeString(GLFWvidmode const& videoMode) const
{
    std::stringstream ss;
    ss << videoMode.width << " x " << videoMode.height << " @ " << videoMode.refreshRate << "Hz";
    return ss.str();
}

std::vector<std::string> _DisplaySettingsDialog::createVideoModeStrings() const
{
    std::vector<std::string> result;
    result.emplace_back("Desktop");
    for (int i = 0; i < _videoModesCount; ++i) {
        result.emplace_back(createVideoModeString(_videoModes[i]));
    }

    return result;
}
