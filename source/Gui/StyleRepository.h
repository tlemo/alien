#pragma once

#include "Definitions.h"

#include <cstdint>

#include "imgui.h"

namespace Const
{
    int64_t const SimulationSliderColor_Base = 0xff4c4c4c;
    int64_t const SimulationSliderColor_Active = 0xff6c6c6c;
    int64_t const TextDecentColor = 0xff909090;
    int64_t const TextInfoColor = 0xff30b0b0;

    ImColor const MenuButtonColor = ImColor::HSV(0.6f, 0.3f, 0.35f);
    ImColor const MenuButtonHoveredColor = ImColor::HSV(0.6f, 1.0f, 1.0f);
    ImColor const MenuButtonActiveColor = ImColor::HSV(0.6f, 0.6f, 0.6f);

    ImColor const ShutdownButtonColor = ImColor::HSV(0.f, 0.6f, 0.6f);
    ImColor const ShutdownButtonHoveredColor = ImColor::HSV(0.f, 1.0f, 1.0f);
    ImColor const ShutdownButtonActiveColor = ImColor::HSV(0.f, 1.0f, 1.0f);
    ImColor const LogMessageColor = ImColor::HSV(0.3f, 1.0f, 1.0f);

    ImColor const HeadlineColor = ImColor::HSV(0.4f, 0.4f, 0.8f);

    ImColor const SelectionAreaFillColor = ImColor::HSV(0.33f, 0.0f, 0.0f, 0.6f);
    ImColor const SelectionAreaBorderColor = ImColor::HSV(0.33f, 0.0f, 1.0f, 1.0f);

    ImColor const CellFunctionOverlayColor = ImColor::HSV(0.33f, 0.0f, 1.0f, 0.5f);
    ImColor const CellFunctionOverlayShadowColor = ImColor::HSV(0.33f, 0.0f, 0.0f, 0.7f);

    float const WindowAlpha = 0.8f;
    float const SliderBarWidth = 30.0f;
}

class _StyleRepository
{
public:
    _StyleRepository();

    ImFont* getMediumFont() const;
    ImFont* getLargeFont() const;
    ImFont* getMonospaceFont() const;

private:
    ImFont* _mediumFont = nullptr;
    ImFont* _largeFont = nullptr;
    ImFont* _monospaceFont = nullptr;
};