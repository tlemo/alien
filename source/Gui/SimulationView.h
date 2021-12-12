#pragma once

#include "Base/Definitions.h"
#include "EngineInterface/OverlayDescriptions.h"
#include "EngineImpl/Definitions.h"
#include "Definitions.h"

class _SimulationView
{
public:
    _SimulationView(
        StyleRepository const& styleRepository,
        SimulationController const& simController,
        ModeWindow const& modeWindow,
        Viewport const& viewport);
    void resize(IntVector2D const& viewportSize);

    void processContent();
    void processControls();

private:
    void processEvents();

    void leftMouseButtonPressed(IntVector2D const& viewPos);
    void leftMouseButtonHold(IntVector2D const& viewPos, IntVector2D const& prevViewPos);
    void leftMouseButtonReleased();

    void rightMouseButtonPressed();
    void rightMouseButtonHold(IntVector2D const& viewPos);
    void rightMouseButtonReleased();

    void middleMouseButtonPressed(IntVector2D const& viewPos);
    void middleMouseButtonHold(IntVector2D const& viewPos);
    void middleMouseButtonReleased();

    void updateImageFromSimulation();

    //widgets
    SimulationScrollbar _scrollbarX;
    SimulationScrollbar _scrollbarY;

    //overlay
    boost::optional<OverlayDescription> _overlay;
    
    //shader data
    unsigned int _vao, _vbo, _ebo;
    unsigned int _fbo;
    Shader _shader;

    bool _areTexturesInitialized = false;
    unsigned int _textureId = 0;
    unsigned int _textureFramebufferId = 0;

    //navigation
    boost::optional<RealVector2D> _worldPosForMovement;
    boost::optional<IntVector2D> _prevMousePosInt;

    Viewport _viewport;
    ModeWindow _modeWindow;
    StyleRepository _styleRepository;
    SimulationController _simController;
};
