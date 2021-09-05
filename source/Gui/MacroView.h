#pragma once

#include "Base/Definitions.h"
#include "EngineImpl/Definitions.h"
#include "Definitions.h"

class MacroView
{
public:
    void init(SimulationController* simController, IntVector2D const& viewportSize);
    void resize(IntVector2D const& viewportSize);

    void render();

private:
    unsigned int _vao, _vbo, _ebo;
    unsigned int _fbo;
    Shader* _shader = nullptr;
    void* _cudaResource = nullptr;

    bool _areTexturesInitialized = false;
    unsigned int _textureId = 0;
    unsigned int _textureFramebufferId = 0;

    SimulationController* _simController = nullptr;
    IntVector2D _viewportSize;
};
