#pragma once

#include "cuda_runtime_api.h"
#include "sm_60_atomic_functions.h"

#include "SimulationData.cuh"
#include "Cell.cuh"
#include "Token.cuh"

__global__ extern void preparePointerArraysForCleanup(SimulationData data);
__global__ extern void prepareArraysForCleanup(SimulationData data);

template<typename Entity>
__global__ void cleanupPointerArray(Array<Entity> entityArray, Array<Entity> newEntityArray)
{
    auto partition =
        calcPartition(entityArray.getNumEntries(), threadIdx.x + blockIdx.x * blockDim.x, blockDim.x * gridDim.x);

    __shared__ int numEntities;
    if (0 == threadIdx.x) {
        numEntities = 0;
    }
    __syncthreads();

    for (int index = partition.startIndex; index <= partition.endIndex; ++index) {
        if (entityArray.at(index) != nullptr) {
            atomicAdd_block(&numEntities, 1);
        }
    }
    __syncthreads();

    __shared__ Entity* newEntities;
    if (0 == threadIdx.x) {
        if (numEntities > 0) {
            newEntities = newEntityArray.getNewSubarray(numEntities);
        }
        numEntities = 0;
    }
    __syncthreads();

    for (int index = partition.startIndex; index <= partition.endIndex; ++index) {
        auto const& entity = entityArray.at(index);
        if (entity != nullptr) {
            int newIndex = atomicAdd(&numEntities, 1);
            newEntities[newIndex] = entity;
        }
    }
    __syncthreads();
}

__global__ extern void cleanupParticles(Array<Particle*> particlePointers, Array<Particle> particles);
__global__ extern void cleanupCellsStep1(Array<Cell*> cellPointers, Array<Cell> cells);
__global__ extern void cleanupCellsStep2(Array<Token*> tokenPointers, Array<Cell> cells);
__global__ extern void cleanupTokens(Array<Token*> tokenPointers, Array<Token> newToken);
__global__ extern void cleanupCellMap(SimulationData data);
__global__ extern void cleanupParticleMap(SimulationData data);
__global__ extern void swapPointerArrays(SimulationData data);
__global__ extern void swapArrays(SimulationData data);
__global__ extern void checkIfCleanupIsNecessary(SimulationData data, bool* result);

//#TODO remove
__global__ void cleanupAfterDataManipulationKernel(SimulationData data);