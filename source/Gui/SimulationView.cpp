#include "SimulationView.h"

#include <glad/glad.h>
#include "imgui.h"

#include "EngineImpl/SimulationController.h"

#include "Shader.h"
#include "SimulationScrollbar.h"
#include "Viewport.h"
#include "Resources.h"
#include "ModeWindow.h"
#include "StyleRepository.h"

namespace
{
    auto const MotionBlurStandard = 0.8f;
    auto const MotionBlurZooming = 0.5f;
    auto const ZoomFactorForOverlay = 16.0f;

    std::unordered_map<Enums::CellFunction::Type, std::string> cellFunctionToStringMap = {
        {Enums::CellFunction::COMPUTER, "Computer"},
        {Enums::CellFunction::PROPULSION, "Propulsion"},
        {Enums::CellFunction::SCANNER, "Scanner"},
        {Enums::CellFunction::WEAPON, "Weapon"},
        {Enums::CellFunction::CONSTRUCTOR, "Constructor"},
        {Enums::CellFunction::SENSOR, "Sensor"},
        {Enums::CellFunction::MUSCLE, "Muscle"},
    };
}

_SimulationView::_SimulationView(
    StyleRepository const& styleRepository,
    SimulationController const& simController,
    ModeWindow const& modeWindow,
    Viewport const& viewport)
    : _viewport(viewport)
    , _styleRepository(styleRepository)
{
    _modeWindow = modeWindow;

    _simController = simController;
    _shader = boost::make_shared<_Shader>(Const::SimulationVertexShader, Const::SimulationFragmentShader);

    _scrollbarX = boost::make_shared<_SimulationScrollbar>(
        "SimScrollbarX", _SimulationScrollbar ::Orientation::Horizontal, _simController, _viewport);
    _scrollbarY = boost::make_shared<_SimulationScrollbar>(
        "SimScrollbarY", _SimulationScrollbar::Orientation::Vertical, _simController, _viewport);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions        // texture coordinates
        1.0f,  1.0f,  0.0f, 1.0f, 1.0f,  // top right
        1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom left
        -1.0f, 1.0f,  0.0f, 0.0f, 1.0f   // top left
    };
    unsigned int indices[] = {
        0,
        1,
        3,  // first triangle
        1,
        2,
        3  // second triangle
    };
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
  
    // texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    resize(_viewport->getViewSize());

    _shader->use();
    _shader->setInt("texture1", 0);
    _shader->setInt("texture2", 1);
    _shader->setBool("glowEffect", true);
    _shader->setBool("motionEffect", true);
    _shader->setFloat("motionBlurFactor", MotionBlurStandard);
}

void _SimulationView::resize(IntVector2D const& size)
{
    if (_areTexturesInitialized) {
        glDeleteFramebuffers(1, &_fbo);
        glDeleteTextures(1, &_textureId);
        glDeleteTextures(1, &_textureFramebufferId);
        _areTexturesInitialized = true;
    }
    glGenTextures(1, &_textureId);
    glBindTexture(GL_TEXTURE_2D, _textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_SHORT, NULL);
    _simController->registerImageResource(_textureId);

    glGenTextures(1, &_textureFramebufferId);
    glBindTexture(GL_TEXTURE_2D, _textureFramebufferId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_SHORT, NULL);

    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);  
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _textureFramebufferId, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  

    _viewport->setViewSize(size);
}

void _SimulationView::leftMouseButtonPressed(IntVector2D const& viewPos)
{
    _shader->setFloat("motionBlurFactor", MotionBlurZooming);
}

void _SimulationView::leftMouseButtonHold(IntVector2D const& viewPos, IntVector2D const& prevViewPos)
{
    if (_modeWindow->getMode() == _ModeWindow::Mode::Navigation) {
        _viewport->zoom(viewPos, _viewport->getZoomSensitivity());
    }
}

void _SimulationView::leftMouseButtonReleased()
{
    _shader->setFloat("motionBlurFactor", MotionBlurStandard);
}

void _SimulationView::rightMouseButtonPressed()
{
    _shader->setFloat("motionBlurFactor", MotionBlurZooming);
}

void _SimulationView::rightMouseButtonHold(IntVector2D const& viewPos)
{
    if (_modeWindow->getMode() == _ModeWindow::Mode::Navigation) {
        _viewport->zoom(viewPos, 1.0f / _viewport->getZoomSensitivity());
    }
}

void _SimulationView::rightMouseButtonReleased()
{
    _shader->setFloat("motionBlurFactor", MotionBlurStandard);
}

void _SimulationView::middleMouseButtonPressed(IntVector2D const& viewPos)
{
    _worldPosForMovement = _viewport->mapViewToWorldPosition({toFloat(viewPos.x), toFloat(viewPos.y)});
}

void _SimulationView::middleMouseButtonHold(IntVector2D const& viewPos)
{
    _viewport->centerTo(*_worldPosForMovement, viewPos);
}

void _SimulationView::middleMouseButtonReleased()
{
    _worldPosForMovement = boost::none;
}

void _SimulationView::processEvents()
{
    if (!ImGui::GetIO().WantCaptureMouse) {
        auto mousePos = ImGui::GetMousePos();
        IntVector2D mousePosInt{toInt(mousePos.x), toInt(mousePos.y)};
        IntVector2D prevMousePosInt = _prevMousePosInt ? *_prevMousePosInt : mousePosInt;

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            leftMouseButtonPressed(mousePosInt);
        }
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            leftMouseButtonHold(mousePosInt, prevMousePosInt);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            leftMouseButtonReleased();
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            rightMouseButtonPressed();
        }
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            rightMouseButtonHold(mousePosInt);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            rightMouseButtonReleased();
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
            middleMouseButtonPressed(mousePosInt);
        }
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            middleMouseButtonHold(mousePosInt);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
            middleMouseButtonReleased();
        }
        _prevMousePosInt = mousePosInt;
    }
}

void _SimulationView::processContent()
{
    processEvents();

    updateImageFromSimulation();

    _shader->use();

    GLint currentFbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFbo);

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    _shader->setInt("phase", 0);
    glBindVertexArray(_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _textureId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _textureFramebufferId);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, currentFbo);
    _shader->setInt("phase", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _textureFramebufferId);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void _SimulationView::processControls()
{
    auto worldRect = _viewport->getVisibleWorldRect();
    auto visibleWorldSize = worldRect.bottomRight - worldRect.topLeft;
    auto worldSize = _simController->getWorldSize();

    ImGuiStyle& style = ImGui::GetStyle();
    float childHeight = 1 + style.ScrollbarSize + style.WindowPadding.y * 2.0f;
    float childWidth = 1 + style.ScrollbarSize + style.WindowPadding.x * 2.0f;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    _scrollbarX->process({{viewport->Pos.x, viewport->Size.y - 17}, {viewport->Size.x - 1 - 17, 1}});
    _scrollbarY->process({{viewport->Size.x - 17, viewport->Pos.y + 22}, {1, viewport->Size.y - 1 - 17}});
}

void _SimulationView::updateImageFromSimulation()
{

    auto worldRect = _viewport->getVisibleWorldRect();
    auto viewSize = _viewport->getViewSize();
    auto zoomFactor = _viewport->getZoomFactor();

    if (zoomFactor < ZoomFactorForOverlay) {
        _simController->drawVectorGraphics(
            worldRect.topLeft, worldRect.bottomRight, {viewSize.x, viewSize.y}, zoomFactor);
        _overlay = boost::none;

    } else {
        auto overlay = _simController->drawVectorGraphicsAndReturnOverlay(
            worldRect.topLeft, worldRect.bottomRight, {viewSize.x, viewSize.y}, zoomFactor);
        if (overlay) {
            _overlay = overlay;
        }
    }

    if(_overlay) {
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        for (auto const& overlayElement : _overlay->elements) {
            auto fontSize = std::min(30.0f, _viewport->getZoomFactor()) / 2;
            auto viewPos = _viewport->mapWorldToViewPosition({overlayElement.pos.x, overlayElement.pos.y + 0.4f});
            auto text = cellFunctionToStringMap.at(overlayElement.cellType);
            draw_list->AddText(
                _styleRepository->getMediumFont(),
                fontSize,
                {viewPos.x - 2*fontSize, viewPos.y},
                Const::CellFunctionOverlayShadowColor,
                text.c_str());
            draw_list->AddText(
                _styleRepository->getMediumFont(),
                fontSize,
                {viewPos.x - 2 * fontSize + 1, viewPos.y + 1},
                Const::CellFunctionOverlayColor,
                text.c_str());
        }
    }
}

