#include "FlowFieldWindow.h"

#include "imgui.h"

#include "Base/Definitions.h"
#include "EngineImpl/SimulationController.h"
#include "StyleRepository.h"

_FlowFieldWindow::_FlowFieldWindow(SimulationController const& simController)
    : _simController(simController)
    , _activeTabs{0}
{
}

void _FlowFieldWindow::process()
{
    if (!_on) {
        return;
    }
    auto flowFieldSettings = _simController->getFlowFieldSettings();
    auto origFlowFieldSettings = flowFieldSettings;

    ImGui::SetNextWindowBgAlpha(Const::WindowAlpha);
    ImGui::Begin("Flow field", &_on, ImGuiWindowFlags_None);

    ImGui::Checkbox("Activate", &flowFieldSettings.active);
    ImGui::SameLine();
    
    const char* flowTypes[] = {"Radial flow"};
    int currentFlowTypes = 0;
    ImGui::BeginDisabled(!flowFieldSettings.active);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::Combo("##", &currentFlowTypes, flowTypes, IM_ARRAYSIZE(flowTypes));
    ImGui::PopItemWidth();

    if (ImGui::BeginTabBar(
            "##Flow",
            ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyResizeDown)) {

        if (_activeTabs.size() < 2) {
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
                int i = 0;
                for (; i < _activeTabs.size(); ++i) {
                    _activeTabs[i] = i;
                }
                _activeTabs.emplace_back(i);
            }
        }

        std::vector<int> activeTabs;
        for (auto const& tab : _activeTabs) {
            RadialFlowCenterData& radialFlowData = flowFieldSettings.radialFlowCenters[tab];
            bool open = true;
            char name[16];
            bool* openPtr = _activeTabs.size() == 1 ? NULL : &open;
            snprintf(name, IM_ARRAYSIZE(name), "Center %01d", tab + 1);
            if (ImGui::BeginTabItem(name, openPtr, ImGuiTabItemFlags_None)) {
                if (ImGui::BeginTable("##", 2, ImGuiTableFlags_SizingStretchProp)) {

                    //pos x
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputFloat("##posX", &radialFlowData.posX, 0.1f, 1.0f, "%.1f");
                    ImGui::PopItemWidth();

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Position X");

                    //pos y
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputFloat("##posY", &radialFlowData.posY, 0.1f, 1.0f, "%.1f");
                    ImGui::PopItemWidth();

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Position Y");

                    //radius
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputFloat("##radius", &radialFlowData.radius, 0.1f, 1.0f, "%.1f");
                    ImGui::PopItemWidth();

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Radius");

                    //strength
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::SliderFloat(
                        "##strength", &radialFlowData.strength, 0.0, 0.1, "%.3f", ImGuiSliderFlags_Logarithmic);

/*
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputFloat("##strength", &radialFlowData.strength, 0.1f, 1.0f, "%.1f");
                    ImGui::PopItemWidth();
*/

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Strength");

                    //orientation
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    const char* orientations[] = {"Clockwise", "Counter clockwise"};
                    int currentOrientations = radialFlowData.orientation == Orientation::Clockwise ? 0 : 1;
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::Combo("##", &currentOrientations, orientations, IM_ARRAYSIZE(orientations));
                    radialFlowData.orientation =
                        currentOrientations == 0 ? Orientation::Clockwise : Orientation::CounterClockwise;
                    ImGui::PopItemWidth();

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Orientation");

                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            if (open) {
                activeTabs.emplace_back(toInt(activeTabs.size()));
            }
        }
        _activeTabs = activeTabs;
        flowFieldSettings.numCenters = _activeTabs.size();

        ImGui::EndTabBar();
    }
    ImGui::EndDisabled();
    ImGui::End();

    if (flowFieldSettings != origFlowFieldSettings) {
        _simController->setFlowFieldSettings_async(flowFieldSettings);
    }
}

bool _FlowFieldWindow::isOn() const
{
    return _on;
}

void _FlowFieldWindow::setOn(bool value)
{
    _on = value;
}