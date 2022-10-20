#pragma once

#include <cuda_runtime.h>
#include <stdint.h>

#include "EngineInterface/Enums.h"

struct ParticleMetadataAccessTO
{
    unsigned char color;
};

struct ParticleAccessTO
{
	uint64_t id;
	float energy;
	float2 pos;
	float2 vel;
    ParticleMetadataAccessTO metadata;

	int selected;
};

struct CellMetadataAccessTO
{
    int nameLen;
    uint64_t nameStringIndex;

    int descriptionLen;
    uint64_t descriptionStringIndex;

    int sourceCodeLen;
    uint64_t sourceCodeStringIndex;
};

struct ConnectionAccessTO
{
    int cellIndex;
    float distance;
    float angleFromPrevious;
};

struct CellAccessTO
{
	uint64_t id;
    ConnectionAccessTO connections[MAX_CELL_BONDS];

    float2 pos;
    float2 vel;
	float energy;
    int color;
    int maxConnections;
	int numConnections;
	int executionOrderNumber;
    bool barrier;
    int age;

    bool underConstruction;
    bool inputBlocked;
    bool outputBlocked;
    Enums::CellFunction cellFunction;

	CellMetadataAccessTO metadata;

	int selected;
};

struct DataAccessTO
{
	int* numCells = nullptr;
	CellAccessTO* cells = nullptr;
	int* numParticles = nullptr;
	ParticleAccessTO* particles = nullptr;
    int* numStringBytes = nullptr;
    char* stringBytes = nullptr;

	bool operator==(DataAccessTO const& other) const
	{
		return numCells == other.numCells
			&& cells == other.cells
			&& numParticles == other.numParticles
			&& particles == other.particles
            && numStringBytes == other.numStringBytes
            && stringBytes == other.stringBytes;
	}
};

