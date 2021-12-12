#pragma once

#include <cstdint>
#include <atomic>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>

#include "EngineInterface/OverallStatistics.h"
#include "EngineInterface/Settings.h"
#include "EngineInterface/SelectionShallowData.h"
#include "EngineInterface/ShallowUpdateSelectionData.h"

#include "Definitions.cuh"
#include "DllExport.h"

class _CudaSimulation
{
public:
    ENGINEGPUKERNELS_EXPORT static void initCuda();

    ENGINEGPUKERNELS_EXPORT
    _CudaSimulation(uint64_t timestep, Settings const& settings, GpuSettings const& gpuSettings);
    ENGINEGPUKERNELS_EXPORT ~_CudaSimulation();

    ENGINEGPUKERNELS_EXPORT void* registerImageResource(GLuint image);

    ENGINEGPUKERNELS_EXPORT void calcCudaTimestep();

    ENGINEGPUKERNELS_EXPORT void drawVectorGraphics(
        float2 const& rectUpperLeft,
        float2 const& rectLowerRight,
        void* cudaResource,
        int2 const& imageSize,
        double zoom);
    ENGINEGPUKERNELS_EXPORT void
    getSimulationData(int2 const& rectUpperLeft, int2 const& rectLowerRight, DataAccessTO const& dataTO);
    ENGINEGPUKERNELS_EXPORT void
    getOverlayData(int2 const& rectUpperLeft, int2 const& rectLowerRight, DataAccessTO const& dataTO);
    ENGINEGPUKERNELS_EXPORT void setSimulationData(DataAccessTO const& dataTO);

    ENGINEGPUKERNELS_EXPORT void applyForce(ApplyForceData const& applyData);
    ENGINEGPUKERNELS_EXPORT void switchSelection(SwitchSelectionData const& switchData);
    ENGINEGPUKERNELS_EXPORT void setSelection(SetSelectionData const& selectionData);
    ENGINEGPUKERNELS_EXPORT SelectionShallowData getSelectionShallowData();
    ENGINEGPUKERNELS_EXPORT void shallowUpdateSelection(ShallowUpdateSelectionData const& shallowUpdateData);
    ENGINEGPUKERNELS_EXPORT void removeSelection();

    ENGINEGPUKERNELS_EXPORT void setGpuConstants(GpuSettings const& cudaConstants);
    ENGINEGPUKERNELS_EXPORT void setSimulationParameters(SimulationParameters const& parameters);
    ENGINEGPUKERNELS_EXPORT void setSimulationParametersSpots(SimulationParametersSpots const& spots);
    ENGINEGPUKERNELS_EXPORT void setFlowFieldSettings(FlowFieldSettings const& settings);

    struct ArraySizes
    {
        int cellArraySize;
        int particleArraySize;
        int tokenArraySize;
    };
    ENGINEGPUKERNELS_EXPORT ArraySizes getArraySizes() const;

    ENGINEGPUKERNELS_EXPORT OverallStatistics getMonitorData();
    ENGINEGPUKERNELS_EXPORT uint64_t getCurrentTimestep() const;
    ENGINEGPUKERNELS_EXPORT void setCurrentTimestep(uint64_t timestep);

    ENGINEGPUKERNELS_EXPORT void clear();

    ENGINEGPUKERNELS_EXPORT void resizeArraysIfNecessary(ArraySizes const& additionals);

private:
    void automaticResizeArrays();
    void resizeArrays(ArraySizes const& additionals);

    std::atomic<uint64_t> _currentTimestep;
    SimulationData* _cudaSimulationData;
    RenderingData* _cudaRenderingData;
    SimulationResult* _cudaSimulationResult;
    SelectionResult* _cudaSelectionResult;
    DataAccessTO* _cudaAccessTO;
    CudaMonitorData* _cudaMonitorData;
};
