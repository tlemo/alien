#include "MainWindow.h"

#include <iostream>

#include <glad/glad.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#include "ImFileDialog.h"
#include "implot.h"
#include "IconFontCppHeaders/IconsFontAwesome5.h"

#include "EngineInterface/Serializer.h"
#include "EngineInterface/ChangeDescriptions.h"
#include "EngineImpl/SimulationController.h"

#include "ModeWindow.h"
#include "SimulationView.h"
#include "StyleRepository.h"
#include "TemporalControlWindow.h"
#include "SpatialControlWindow.h"
#include "SimulationParametersWindow.h"
#include "StatisticsWindow.h"
#include "GpuSettingsWindow.h"
#include "Viewport.h"
#include "NewSimulationDialog.h"
#include "StartupWindow.h"
#include "FlowFieldWindow.h"
#include "AlienImGui.h"

namespace
{
    void glfwErrorCallback(int error, const char* description)
    {
        std::cerr << "Glfw Error " << error << ": " << description << std::endl;
    }

    _SimulationView* simulationViewPtr;
    void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        if (width > 0 && height > 0) {
            simulationViewPtr->resize({width, height});
            glViewport(0, 0, width, height);
        }
    }
}

GLFWwindow* _MainWindow::init(SimulationController const& simController)
{
    _simController = simController;
    
    auto glfwData = initGlfw();
    if (!glfwData.window) {
        return nullptr;
    }
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    _styleRepository = boost::make_shared<_StyleRepository>();

//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    //ImGui::StyleColorsDark();
//    ImGui::StyleColorsLight();

    // Setup Platform/Renderer back-ends
    ImGui_ImplGlfw_InitForOpenGL(glfwData.window, true);
    ImGui_ImplOpenGL3_Init(glfwData.glsl_version);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }

    _modeWindow = boost::make_shared<_ModeWindow>();
    auto worldSize = simController->getWorldSize();
    _viewport = boost::make_shared<_Viewport>();
    _viewport->setCenterInWorldPos({toFloat(worldSize.x) / 2, toFloat(worldSize.y) / 2});
    _viewport->setZoomFactor(4.0f);
    _viewport->setViewSize(IntVector2D{glfwData.mode->width, glfwData.mode->height});

    _simulationView = boost::make_shared<_SimulationView>(simController, _modeWindow, _viewport);
    simulationViewPtr = _simulationView.get();
    _statisticsWindow = boost::make_shared<_StatisticsWindow>(_simController);
    _temporalControlWindow = boost::make_shared<_TemporalControlWindow>(simController, _styleRepository, _statisticsWindow);
    _spatialControlWindow = boost::make_shared<_SpatialControlWindow>(simController, _viewport, _styleRepository);
    _simulationParametersWindow = boost::make_shared<_SimulationParametersWindow>(_styleRepository, _simController);
    _gpuSettingsWindow = boost::make_shared<_GpuSettingsWindow>(_styleRepository, _simController);
    _newSimulationDialog = boost::make_shared<_NewSimulationDialog>(_simController, _viewport, _statisticsWindow, _styleRepository);
    _startupWindow = boost::make_shared<_StartupWindow>(
        _simController, _viewport, _temporalControlWindow, _spatialControlWindow, _statisticsWindow);
    _flowFieldWindow = boost::make_shared<_FlowFieldWindow>(_simController);

    ifd::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h, char fmt) -> void* {
        GLuint tex;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        return (void*)tex;
    };
    ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
        GLuint texID = (GLuint)((uintptr_t)tex);
        glDeleteTextures(1, &texID);
    };
    return glfwData.window;
}

void _MainWindow::mainLoop(GLFWwindow* window)
{
    bool show_demo_window = true;
    while (!glfwWindowShouldClose(window) && !_onClose)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

/*
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        {}
*/

        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, Const::SliderBarWidth);
        if (_startupWindow->getState() == _StartupWindow::State::LoadingControls
            || _startupWindow->getState() == _StartupWindow::State::Finished) {
            processMenubar();
            processDialogs();
            processWindows();
            _simulationView->processControls();
        }
        if (_startupWindow->getState() != _StartupWindow::State::Finished) {
            _startupWindow->process();
        }
        ImGui::PopStyleVar();

        // render content
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        if (_startupWindow->getState() != _StartupWindow::State::Unintialized) {
            _simulationView->processContent();
        } else {
            glClearColor(0, 0, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        if (_startupWindow->getState() == _StartupWindow::State::Unintialized) {
            _startupWindow->activate();
        }
    }
}

void _MainWindow::shutdown(GLFWwindow* window)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    _simulationView.reset();
}

auto _MainWindow::initGlfw() -> GlfwData
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        return {nullptr, nullptr, nullptr};
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(primaryMonitor);
    auto screenWidth = mode->width;
    auto screenHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "alien", primaryMonitor, NULL);
    if (window == NULL) {
        return {nullptr, nullptr, nullptr};
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);  // Enable vsync

    return {window, mode, glsl_version};
}

void _MainWindow::processMenubar()
{
    if (ImGui::BeginMainMenuBar()) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.f, 0.8f, 0.8f));
        if (ImGui::Button(ICON_FA_POWER_OFF)) {
            _showExitDialog = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        if (AlienImGui::BeginMenuButton(" " ICON_FA_GLOBE "  Simulation ", _simulationMenuToggled, "Simulation")) {
            if (ImGui::MenuItem("New", "CTRL+N")) { 
                _newSimulationDialog->show();
                _simulationMenuToggled = false;
            }
            if (ImGui::MenuItem("Open", "CTRL+O")) {
                onOpenSimulation();
                _simulationMenuToggled = false;
            }
            if (ImGui::MenuItem("Save", "CTRL+S")) {
                onSaveSimulation();
                _simulationMenuToggled = false;
            }
            ImGui::Separator();
            ImGui::BeginDisabled(_simController->isSimulationRunning());
            if (ImGui::MenuItem("Run", "CTRL+R")) {
                onRunSimulation();
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(!_simController->isSimulationRunning());
            if (ImGui::MenuItem("Pause", "CTRL+P")) {
                onPauseSimulation();
            }
            ImGui::EndDisabled();
            AlienImGui::EndMenuButton();
        }

        if (AlienImGui::BeginMenuButton(" " ICON_FA_WINDOW_RESTORE "  Window ", _windowMenuToggled, "Window")) {
            if (ImGui::MenuItem("Temporal control", "", _temporalControlWindow->isOn())) {
                _temporalControlWindow->setOn(!_temporalControlWindow->isOn());
            }
            if (ImGui::MenuItem("Spatial control", "", _spatialControlWindow->isOn())) {
                _spatialControlWindow->setOn(!_spatialControlWindow->isOn());
            }
            if (ImGui::MenuItem("Statistics", "", _statisticsWindow->isOn())) {
                _statisticsWindow->setOn(!_statisticsWindow->isOn());
            }
            AlienImGui::EndMenuButton();
        }

        if (AlienImGui::BeginMenuButton(" " ICON_FA_COG "  Settings ", _settingsMenuToggled, "Settings")) {
            if (ImGui::MenuItem("GPU settings", "", _gpuSettingsWindow->isOn())) {
                _gpuSettingsWindow->setOn(!_gpuSettingsWindow->isOn());
            }
            if (ImGui::MenuItem("Simulation parameters", "", _simulationParametersWindow->isOn())) {
                _simulationParametersWindow->setOn(!_simulationParametersWindow->isOn());
            }
            if (ImGui::MenuItem("Flow field", "", _flowFieldWindow->isOn())) {
                _flowFieldWindow->setOn(!_flowFieldWindow->isOn());
            }
            AlienImGui::EndMenuButton();
        }

        if (AlienImGui::BeginMenuButton(" " ICON_FA_TOOLS "  Tools ", _toolsMenuToggled, "Tools")) {
            AlienImGui::EndMenuButton();
        }
        if (AlienImGui::BeginMenuButton(" " ICON_FA_LIFE_RING"  Help ", _helpMenuToggled, "Help")) {
            AlienImGui::EndMenuButton();
        }
        ImGui::EndMainMenuBar();
    }

    //menu hotkeys
    auto io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(GLFW_KEY_O)) {
        onOpenSimulation();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(GLFW_KEY_S)) {
        onSaveSimulation();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(GLFW_KEY_R)) {
        onRunSimulation();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(GLFW_KEY_P)) {
        onPauseSimulation();
    }
}

void _MainWindow::processDialogs()
{
    if (ifd::FileDialog::Instance().IsDone("SimulationOpenDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            const std::vector<std::filesystem::path>& res = ifd::FileDialog::Instance().GetResults();
            auto firstFilename = res.front();

            _simController->closeSimulation();

            reset();

            Serializer serializer = boost::make_shared<_Serializer>();
            SerializedSimulation serializedData;
            serializer->loadSimulationDataFromFile2(firstFilename.string(), serializedData);
            auto deserializedData = serializer->deserializeSimulation2(serializedData);

            _simController->newSimulation(
                deserializedData.timestep, deserializedData.settings, deserializedData.symbolMap);
            _simController->updateData(deserializedData.content);
            _viewport->setCenterInWorldPos(
                {toFloat(deserializedData.settings.generalSettings.worldSizeX) / 2,
                 toFloat(deserializedData.settings.generalSettings.worldSizeY) / 2});
            _viewport->setZoomFactor(4.0f);
        }
        ifd::FileDialog::Instance().Close();
    }

    if (ifd::FileDialog::Instance().IsDone("SimulationSaveDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            const std::vector<std::filesystem::path>& res = ifd::FileDialog::Instance().GetResults();
            auto firstFilename = res.front();

            DeserializedSimulation sim;
            sim.timestep = static_cast<uint32_t>(_simController->getCurrentTimestep());
            sim.settings = _simController->getSettings();
            sim.symbolMap = _simController->getSymbolMap();
            sim.content = _simController->getSimulationData({0, 0}, _simController->getWorldSize());

            Serializer serializer = boost::make_shared<_Serializer>();
            auto serializedSim = serializer->serializeSimulation2(sim);
            serializer->saveSimulationDataToFile2(firstFilename.string(), serializedSim);

        }
        ifd::FileDialog::Instance().Close();
    }
    _newSimulationDialog->process();
    processExitDialog();
}

void _MainWindow::processWindows()
{
    _temporalControlWindow->process();
    _spatialControlWindow->process();
    _modeWindow->process();
    _statisticsWindow->process();
    _simulationParametersWindow->process();
    _gpuSettingsWindow->process();
    _flowFieldWindow->process();
}

void _MainWindow::onOpenSimulation()
{
    ifd::FileDialog::Instance().Open(
        "SimulationOpenDialog", "Open simulation", "Simulation file (*.sim){.sim},.*", false);
}

void _MainWindow::onSaveSimulation()
{
    ifd::FileDialog::Instance().Save("SimulationSaveDialog", "Save simulation", "Simulation file (*.sim){.sim},.*");
}

void _MainWindow::onRunSimulation()
{
    _simController->runSimulation();
}

void _MainWindow::onPauseSimulation()
{
    _simController->pauseSimulation();
}

void _MainWindow::processExitDialog()
{
     if (_showExitDialog) {
        auto name = "Exit";
        ImGui::OpenPopup(name);

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(name, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Do you really want to terminate the program?");

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Spacing();

            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
                _onClose = true;
            }
            ImGui::SetItemDefaultFocus();

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                _showExitDialog = false;
            }

            ImGui::EndPopup();
        }
    }
}

void _MainWindow::reset()
{
    _statisticsWindow->reset();
}
