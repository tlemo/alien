#pragma once

#include "EngineInterface/SimulationParameters.h"

#include "Base.cuh"
#include "ConstantMemory.cuh"

class ParameterCalculator
{
public:
    __inline__ __device__ static float get(float SimulationParameters::*parameter, float2 const& pos);
};

/************************************************************************/
/* Implementation                                                       */
/************************************************************************/
__inline__ __device__ float ParameterCalculator::get(float SimulationParameters::*parameter, float2 const& pos)
{
    auto const r = Math::length(pos - float2{7000, 250});
    if (r < 7000.0f) {
        auto const factor = r / 7000.0f;
        return cudaSimulationParameters.*parameter * factor + cudaSimulationParametersSpot.*parameter * (1.0f - factor);
    }
    return cudaSimulationParameters.*parameter;
}
