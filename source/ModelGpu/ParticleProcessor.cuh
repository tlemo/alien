#pragma once

#include "device_functions.h"
#include "sm_60_atomic_functions.h"

#include "CudaAccessTOs.cuh"
#include "Base.cuh"
#include "Physics.cuh"
#include "Map.cuh"

class ParticleProcessor
{
public:
	__inline__ __device__ void init_gridCall(SimulationData& data);

    __inline__ __device__ void processingMovement_gridCall();
    __inline__ __device__ void updateMap_gridCall();
    __inline__ __device__ void processingCollision_gridCall();
    __inline__ __device__ void processingTransformation_gridCall();
	__inline__ __device__ void processingDataCopy_gridCall();

private:

	SimulationData* _data;

    PartitionData _particleBlock;
};


/************************************************************************/
/* Implementation                                                       */
/************************************************************************/
__inline__ __device__ void ParticleProcessor::init_gridCall(SimulationData & data)
{
    _data = &data;

    _particleBlock = calcPartition(
        data.entities.particlePointers.getNumEntries(), threadIdx.x + blockIdx.x * blockDim.x, blockDim.x * gridDim.x);
}

__inline__ __device__ void ParticleProcessor::processingMovement_gridCall()
{
    for (int particleIndex = _particleBlock.startIndex; particleIndex <= _particleBlock.endIndex; ++particleIndex) {
        Particle* particle = _data->entities.particlePointers.at(particleIndex);
        particle->absPos = particle->absPos + particle->vel;
        _data->particleMap.mapPosCorrection(particle->absPos);
    }
}

__inline__ __device__ void ParticleProcessor::updateMap_gridCall()
{
    auto const particleBlock = calcPartition(_data->entities.particlePointers.getNumEntries(), blockIdx.x, gridDim.x);

    Particle** particlePointers = &_data->entities.particlePointers.at(particleBlock.startIndex);
    _data->particleMap.set_blockCall(particleBlock.numElements(), particlePointers);
}

__inline__ __device__ void ParticleProcessor::processingCollision_gridCall()
{
    for (int particleIndex = _particleBlock.startIndex; particleIndex <= _particleBlock.endIndex; ++particleIndex) {
        Particle* particle = _data->entities.particlePointers.at(particleIndex);
        Particle* otherParticle = _data->particleMap.get(particle->absPos);
        if (otherParticle && otherParticle != particle) {
            if (1 == particle->alive && 1 == otherParticle->alive) {

                DoubleLock lock;
                lock.init(&particle->locked, &otherParticle->locked);
                lock.tryLock();
                if (!lock.isLocked()) {
                    continue;
                }

                float factor1 = particle->energy / (particle->energy + otherParticle->energy);
                float factor2 = 1.0f - factor1;
                particle->vel = particle->vel * factor1 + otherParticle->vel * factor2;
                particle->energy += otherParticle->energy;
                otherParticle->energy = 0;
                atomicExch(&otherParticle->alive, 0);

                lock.releaseLock();
            }
        }
    }
}

__inline__ __device__ void ParticleProcessor::processingTransformation_gridCall()
{
    for (int particleIndex = _particleBlock.startIndex; particleIndex <= _particleBlock.endIndex; ++particleIndex) {
        if (_data->numberGen.random() < cudaSimulationParameters.cellTransformationProb) {
            Particle* particle = _data->entities.particlePointers.getEntireArray()[particleIndex];
            auto innerEnergy = particle->energy - Physics::linearKineticEnergy(1.0f, particle->vel);
            if (innerEnergy >= cudaSimulationParameters.cellMinEnergy) {
                EntityFactory factory;
                factory.init(_data);
                factory.createClusterWithRandomCell(innerEnergy, particle->absPos, particle->vel);
                atomicExch(&particle->alive, 0);
            }
        }
    }
}

__inline__ __device__ void ParticleProcessor::processingDataCopy_gridCall()
{
	for (int particleIndex = _particleBlock.startIndex; particleIndex <= _particleBlock.endIndex; ++particleIndex) {
		auto& particle = _data->entities.particlePointers.at(particleIndex);
		if (0 == particle->alive) {
            particle = nullptr;
            continue;
		}
        if (auto cell = _data->cellMap.get(particle->absPos)) {
			if (1 == cell->alive) {
                cell->changeEnergy(particle->energy);
                particle = nullptr;
			}
		}
	}
}
