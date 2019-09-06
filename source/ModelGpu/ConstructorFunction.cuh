#pragma once
#include "HashMap.cuh"
#include "Math.cuh"
#include "ModelBasic/ElementaryTypes.h"
#include "QuantityConverter.cuh"
#include "SimulationData.cuh"

class ConstructorFunction
{
public:
    __inline__ __device__ void init_blockCall(Token* token, SimulationData* data);
    __inline__ __device__ void processing();

private:
    struct ClusterComponent
    {
        enum Type
        {
            Constructor = 0,
            ConstructionSite = 1
        };
    };
    struct Angles
    {
        float constructor;
        float constructionSite;
    };
    struct AngularMasses
    {
        float constructor;
        float constructionSite;
    };
    struct RotationMatrices
    {
        float constructor[2][2];
        float constructionSite[2][2];
    };
    struct EnergyForNewEntities
    {
        bool energyAvailable;
        float cell;
        float token;
    };

    __inline__ __device__ void checkMaxRadius(bool& result);
    __inline__ __device__ bool checkDistance(float distance);
    __inline__ __device__ Cell* getConstructionSite();

    __inline__ __device__ void continueConstruction(Cell* firstCellOfConstructionSite);
    __inline__ __device__ void startNewConstruction();

    __inline__ __device__ void
    continueConstructionWithRotationOnly(Cell* constructionCell, Angles const& anglesToRotate, float desiredAngle);

    __inline__ __device__ void continueConstructionWithRotationAndCreation(
        Cell* constructionCell,
        Angles const& anglesToRotate,
        float desiredAngle);

    __inline__ __device__ void tagConstructionSite(Cell* baseCell, Cell* firstCellOfConstructionSite);

    __inline__ __device__ void calcMaxAngles(Cell* constructionCell, Angles& result);
    __inline__ __device__ AngularMasses calcAngularMasses(Cluster* cluster, Cell* constructionCell);
    __inline__ __device__ RotationMatrices calcRotationMatrices(Angles const& angles);

    __inline__ __device__ float calcFreeAngle(Cell* cell);
    __inline__ __device__ Angles
    calcAnglesToRotate(AngularMasses const& angularMasses, float desiredAngleBetweenConstructurAndConstructionSite);
    __inline__ __device__ bool restrictAngles(Angles& angles, Angles const& minAngles);

    __inline__ __device__ float calcAngularMassAfterTransformationAndAddingCell(
        Cluster* cluster,
        float2 const& relPosOfNewCell,
        float2 const& centerOfRotation,
        RotationMatrices const& rotationMatrices,
        float2 const& displacementOfConstructionSite);
    __inline__ __device__ float calcAngularMassAfterAddingCell(Cluster* cluster, float2 const& relPosOfNewCell);
    __inline__ __device__ EnergyForNewEntities adaptEnergies(Token* token, float energyLoss);


    __inline__ __device__ void transformClusterComponents(
        Cluster* cluster,
        float2 const& centerOfRotation,
        RotationMatrices const& rotationMatrices,
        float2 const& displacementForConstructionSite);
    __inline__ __device__ void ConstructorFunction::adaptRelPositions(Cluster* cluster);
    __inline__ __device__ void completeCellAbsPosAndVel(Cluster* cluster);

    __inline__ __device__ float2 getTransformedCellRelPos(
        Cell* cell,
        float2 const& centerOfRotation,
        RotationMatrices const& matrices,
        float2 const& displacementForConstructionSite);

    struct CellAndNewAbsPos
    {
        Cell* cell;
        float2 newAbsPos;
    };
    __inline__ __device__ bool isObstaclePresent_onlyRotation(
        bool ignoreOwnCluster,
        Cluster* cluster,
        float2 const& centerOfRotation,
        RotationMatrices const& rotationMatrices,
        Map<Cell> const& map,
        HashMap<int2, CellAndNewAbsPos>& tempMap);
    __inline__ __device__ bool isObstaclePresent_rotationAndCreation(
        bool ignoreOwnCluster,
        Cluster* cluster,
        float2 const& relPosOfNewCell,
        float2 const& centerOfRotation,
        RotationMatrices const& rotationMatrices,
        float2 const& displacementOfConstructionSite,
        Map<Cell> const& map,
        HashMap<int2, CellAndNewAbsPos>& tempMap);
    __inline__ __device__ bool isObstaclePresent_firstCreation(
        bool ignoreOwnCluster,
        Cluster* cluster,
        float2 const& relPosOfNewCell,
        Map<Cell> const& map,
        HashMap<int2, CellAndNewAbsPos>& tempMap);
    __inline__ __device__ bool isObstaclePresent_helper(
        bool ignoreOwnCluster,
        Cluster* cluster,
        Cell* cell,
        float2 const& absPos,
        Map<Cell> const& map,
        HashMap<int2, CellAndNewAbsPos>& tempMap);

    __inline__ __device__ Cell* constructNewCell(
        Token* token,
        Cluster* cluster,
        float2 const& relPosOfNewCell,
        float const energyOfNewCell,
        EntityFactory& factory);
    __inline__ __device__ Token* constructNewToken(
        Token* token,
        Cell* cellOfNewToken,
        Cell* sourceCellOfNewToken,
        float energyOfNewToken,
        EntityFactory& factory,
        bool duplicate);

    __inline__ __device__ void addCellToCluster(Cell* newCell, Cluster* cluster, Cell** newCellPointers);
    __inline__ __device__ void addTokenToCluster(Token* token, Cluster* cluster, Token** newTokenPointers);

    __inline__ __device__ void separateConstructionWhenFinished(Token* token, Cell* newCell);

    __inline__ __device__ void
    connectNewCell(Cell* newCell, Cell* cellOfConstructionSite, Token* token, Cluster* cluster, SimulationData* data);
    __inline__ __device__ void removeConnection(Cell* cell1, Cell* cell2);
    enum class AdaptMaxConnections
    {
        No,
        Yes
    };
    __inline__ __device__ AdaptMaxConnections isAdaptMaxConnections(Token* token);
    __inline__ __device__ bool isConnectable(Cell* cell, AdaptMaxConnections adaptMaxConnections);
    __inline__ __device__ void establishConnection(Cell* cell1, Cell* cell2, AdaptMaxConnections adaptMaxConnections);

private:
    SimulationData* _data;
    Token* _token;
    Cluster* _cluster;
    PartitionData _cellBlock;
};

/************************************************************************/
/* Implementation                                                       */
/************************************************************************/

__inline__ __device__ void ConstructorFunction::processing()
{
    auto const command = _token->memory[Enums::Constr::IN] % Enums::ConstrIn::_COUNTER;
    if (Enums::ConstrIn::DO_NOTHING == command) {
        _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::SUCCESS;
        __syncthreads();
        return;
    }

    __shared__ bool isRadiusTooLarge;
    checkMaxRadius(isRadiusTooLarge);
    __syncthreads();

    if (!isRadiusTooLarge) {
        _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_MAX_RADIUS;
        __syncthreads();
        return;
    }

    //TODO: short energy check for optimization

    __shared__ Cell* firstCellOfConstructionSite;
    if (0 == threadIdx.x) {
        firstCellOfConstructionSite = getConstructionSite();
    }
    __syncthreads();

    if (firstCellOfConstructionSite) {
        auto const distance = QuantityConverter::convertDataToDistance(_token->memory[Enums::Constr::IN_DIST]);
        if (!checkDistance(distance)) {
            _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_DIST;
            __syncthreads();
            return;
        }
        __syncthreads();
        continueConstruction(firstCellOfConstructionSite);
    } else {
        startNewConstruction();
    }
    __syncthreads();
}

__inline__ __device__ void ConstructorFunction::init_blockCall(Token* token, SimulationData* data)
{
    _data = data;
    _cluster = token->cell->cluster;
    _token = token;
    _cellBlock = calcPartition(_cluster->numCellPointers, threadIdx.x, blockDim.x);
}

__inline__ __device__ void ConstructorFunction::checkMaxRadius(bool& result)
{
    __shared__ float maxRadius;
    if (0 == threadIdx.x) {
        result = true;
        maxRadius = _data->cellMap.getMaxRadius();
    }
    __syncthreads();
    for (int cellIndex = _cellBlock.startIndex; cellIndex <= _cellBlock.endIndex; ++cellIndex) {
        auto const& cell = _cluster->cellPointers[cellIndex];
        if (Math::length(cell->relPos) >= maxRadius - FP_PRECISION) {
            result = false;
            return;
        }
    }
}

__inline__ __device__ bool ConstructorFunction::checkDistance(float distance)
{
    return cudaSimulationParameters.cellMinDistance < distance && distance < cudaSimulationParameters.cellMaxDistance;
}

__inline__ __device__ Cell* ConstructorFunction::getConstructionSite()
{
    Cell* result = nullptr;
    auto const& cell = _token->cell;
    for (int i = 0; i < cell->numConnections; ++i) {
        auto const& connectingCell = cell->connections[i];
        if (connectingCell->tokenBlocked) {
            result = connectingCell;
        }
    }
    return result;
}

__inline__ __device__ void ConstructorFunction::continueConstruction(Cell* firstCellOfConstructionSite)
{
    auto const& cell = _token->cell;
    tagConstructionSite(cell, firstCellOfConstructionSite);
    __syncthreads();

    if (ClusterComponent::ConstructionSite == cell->tag) {
        _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_CONNECTION;
        __syncthreads();
        return;
    }

    __shared__ Angles maxAngles;
    calcMaxAngles(firstCellOfConstructionSite, maxAngles);
    __syncthreads();
    if (0 == threadIdx.x) {
        auto const angularMasses = calcAngularMasses(_cluster, firstCellOfConstructionSite);
        auto const desiredAngleBetweenConstructurAndConstructionSite =
            QuantityConverter::convertDataToAngle(_token->memory[Enums::Constr::INOUT_ANGLE]);

        auto anglesToRotate = calcAnglesToRotate(angularMasses, desiredAngleBetweenConstructurAndConstructionSite);
        auto const isAngleRestricted = restrictAngles(anglesToRotate, maxAngles);

        if (isAngleRestricted) {

            //angle discretization correction
            anglesToRotate.constructor =
                QuantityConverter::convertDataToAngle(QuantityConverter::convertAngleToData(anglesToRotate.constructor));
            anglesToRotate.constructionSite = QuantityConverter::convertDataToAngle(
                QuantityConverter::convertAngleToData(anglesToRotate.constructionSite));

            continueConstructionWithRotationOnly(
                firstCellOfConstructionSite,
                anglesToRotate,
                desiredAngleBetweenConstructurAndConstructionSite);
        }
        else {
            continueConstructionWithRotationAndCreation(
                firstCellOfConstructionSite,
                anglesToRotate,
                desiredAngleBetweenConstructurAndConstructionSite);
        }
    }
    __syncthreads();
}

__inline__ __device__ void ConstructorFunction::startNewConstruction()
{
    if (0 == threadIdx.x) {
        auto const& cell = _token->cell;
        auto const& cluster = cell->cluster;

        auto const adaptMaxConnections = isAdaptMaxConnections(_token);

        if (!isConnectable(cell, adaptMaxConnections)) {
            _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_CONNECTION;
            return;
        }

        auto const freeAngle = calcFreeAngle(cell);
        auto const newCellAngle =
            QuantityConverter::convertDataToAngle(_token->memory[Enums::Constr::INOUT_ANGLE]) + freeAngle;

        auto const option = _token->memory[Enums::Constr::IN_OPTION] % Enums::ConstrInOption::_COUNTER;
        bool const separation = Enums::ConstrInOption::FINISH_WITH_SEP == option
            || Enums::ConstrInOption::FINISH_WITH_SEP_RED == option
            || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option;

        auto const distance = QuantityConverter::convertDataToDistance(_token->memory[Enums::Constr::IN_DIST]);
        auto const relPosOfNewCellDelta =
            Math::unitVectorOfAngle(newCellAngle) * cudaSimulationParameters.cellFunctionConstructorOffspringCellDistance;
        auto const relPosOfNewCell = separation
            ? cell->relPos + relPosOfNewCellDelta + Math::unitVectorOfAngle(newCellAngle) * distance
            : cell->relPos + relPosOfNewCellDelta;

        auto const command = _token->memory[Enums::Constr::IN] % Enums::ConstrIn::_COUNTER;
        auto const newCellPointers = _data->entities.cellPointers.getNewSubarray(cluster->numCellPointers * 2);
        if (Enums::ConstrIn::SAFE == command || Enums::ConstrIn::UNSAFE == command) {
            auto ignoreOwnCluster = (Enums::ConstrIn::UNSAFE == command);
            HashMap<int2, CellAndNewAbsPos> tempCellMap(cluster->numCellPointers * 2, _data->arrays);
            if (isObstaclePresent_firstCreation(ignoreOwnCluster, cluster, relPosOfNewCell, _data->cellMap, tempCellMap)) {
                _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_OBSTACLE;
                return;
            }
        }

        auto const kineticEnergyBeforeRotation =
            Physics::kineticEnergy(cluster->numCellPointers, cluster->vel, cluster->angularMass, cluster->angularVel);
        auto const angularMassAfterRotation = calcAngularMassAfterAddingCell(cluster, relPosOfNewCell);
        auto const angularVelAfterRotation =
            Physics::angularVelocity(cluster->angularMass, angularMassAfterRotation, cluster->angularVel);
        auto const kineticEnergyAfterRotation = Physics::kineticEnergy(
            cluster->numCellPointers, cluster->vel, angularMassAfterRotation, angularVelAfterRotation);
        auto const kineticEnergyDiff = kineticEnergyAfterRotation - kineticEnergyBeforeRotation;

        auto const energyForNewEntities = adaptEnergies(_token, kineticEnergyDiff);
        if (!energyForNewEntities.energyAvailable) {
            _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_NO_ENERGY;
            return;
        }

        EntityFactory factory;
        factory.init(_data);

        auto const newCell = constructNewCell(_token, cluster, relPosOfNewCell, energyForNewEntities.cell, factory);
        addCellToCluster(newCell, cluster, newCellPointers);
        establishConnection(newCell, cell, adaptMaxConnections);
        adaptRelPositions(cluster);
        completeCellAbsPosAndVel(cluster);
        cluster->angularVel = angularVelAfterRotation;
        cluster->angularMass = angularMassAfterRotation;

        separateConstructionWhenFinished(_token, newCell);

        bool const createEmptyToken = Enums::ConstrInOption::CREATE_EMPTY_TOKEN == option
            || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option;
        bool const createDuplicateToken = Enums::ConstrInOption::CREATE_DUP_TOKEN == option;
        if (createEmptyToken || createDuplicateToken) {
            auto const newToken =
                constructNewToken(_token, newCell, cell, energyForNewEntities.token, factory, createDuplicateToken);
            auto const newTokenPointers = _data->entities.tokenPointers.getNewSubarray(cluster->numTokenPointers + 1);
            addTokenToCluster(newToken, cluster, newTokenPointers);
        }

        _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::SUCCESS;
        _token->memory[Enums::Constr::INOUT_ANGLE] = 0;
    }
}

__inline__ __device__ void ConstructorFunction::continueConstructionWithRotationOnly(
    Cell* firstCellOfConstructionSite,
    Angles const& anglesToRotate,
    float desiredAngle)
{
    auto const& cluster = firstCellOfConstructionSite->cluster;
    auto const kineticEnergyBeforeRotation =
        Physics::kineticEnergy(cluster->numCellPointers, cluster->vel, cluster->angularMass, cluster->angularVel);
    auto const rotationMatrices = calcRotationMatrices(anglesToRotate);

    auto const angularMassAfterRotation = calcAngularMassAfterTransformationAndAddingCell(
        cluster, {0, 0}, firstCellOfConstructionSite->relPos, rotationMatrices, {0, 0});
    auto const angularVelAfterRotation =
        Physics::angularVelocity(cluster->angularMass, angularMassAfterRotation, cluster->angularVel);
    auto const kineticEnergyAfterRotation = Physics::kineticEnergy(
        cluster->numCellPointers, cluster->vel, angularMassAfterRotation, angularVelAfterRotation);

    auto const kineticEnergyDiff = kineticEnergyAfterRotation - kineticEnergyBeforeRotation;

    if (_token->energy <= cudaSimulationParameters.cellFunctionConstructorOffspringCellEnergy
            + cudaSimulationParameters.tokenMinEnergy + kineticEnergyDiff) {
        _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_NO_ENERGY;
        return;
    }

    auto const command = _token->memory[Enums::Constr::IN] % Enums::ConstrIn::_COUNTER;
    if (Enums::ConstrIn::SAFE == command || Enums::ConstrIn::UNSAFE == command) {
        auto const ignoreOwnCluster = (Enums::ConstrIn::UNSAFE == command);
        HashMap<int2, CellAndNewAbsPos> tempCellMap(cluster->numCellPointers * 2, _data->arrays);
        if (isObstaclePresent_onlyRotation(
                ignoreOwnCluster,
                cluster,
                firstCellOfConstructionSite->relPos,
                rotationMatrices,
                _data->cellMap,
                tempCellMap)) {
            _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_OBSTACLE;
            return;
        }
    }

    transformClusterComponents(cluster, firstCellOfConstructionSite->relPos, rotationMatrices, {0, 0});
    adaptRelPositions(cluster);
    completeCellAbsPosAndVel(cluster);
    cluster->angularVel = angularVelAfterRotation;
    cluster->angularMass = angularMassAfterRotation;

    _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::SUCCESS_ROT;
    _token->memory[Enums::Constr::INOUT_ANGLE] = QuantityConverter::convertAngleToData(
        desiredAngle - (anglesToRotate.constructionSite - anglesToRotate.constructor));
}

__inline__ __device__ void ConstructorFunction::continueConstructionWithRotationAndCreation(
    Cell* firstCellOfConstructionSite,
    Angles const& anglesToRotate,
    float desiredAngle)
{
    auto const& cell = _token->cell;

    auto const adaptMaxConnections = isAdaptMaxConnections(_token);
    if (1 == _token->memory[Enums::Constr::IN_CELL_MAX_CONNECTIONS]) {
        _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_CONNECTION;
        return;
    }

    auto const distance = QuantityConverter::convertDataToDistance(_token->memory[Enums::Constr::IN_DIST]);

    auto relPosOfNewCell = firstCellOfConstructionSite->relPos;
    auto const centerOfRotation = firstCellOfConstructionSite->relPos;
    auto const rotationMatrices = calcRotationMatrices(anglesToRotate);

    auto const cellRelPos_transformed = getTransformedCellRelPos(cell, centerOfRotation, rotationMatrices, {0, 0});

    auto displacementForConstructionSite =
        Math::normalized(firstCellOfConstructionSite->relPos - cellRelPos_transformed) * distance;
    auto const option = _token->memory[Enums::Constr::IN_OPTION] % Enums::ConstrInOption::_COUNTER;
    if (Enums::ConstrInOption::FINISH_WITH_SEP == option || Enums::ConstrInOption::FINISH_WITH_SEP_RED == option
        || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option) {

        relPosOfNewCell = relPosOfNewCell + displacementForConstructionSite;
        displacementForConstructionSite = displacementForConstructionSite * 2;
    }

    auto const& cluster = firstCellOfConstructionSite->cluster;
    auto const command = _token->memory[Enums::Constr::IN] % Enums::ConstrIn::_COUNTER;
    if (Enums::ConstrIn::SAFE == command || Enums::ConstrIn::UNSAFE == command) {
        auto const ignoreOwnCluster = (Enums::ConstrIn::UNSAFE == command);
        HashMap<int2, CellAndNewAbsPos> tempCellMap(cluster->numCellPointers * 2, _data->arrays);
        if (isObstaclePresent_rotationAndCreation(
                ignoreOwnCluster,
                cluster,
                relPosOfNewCell,
                centerOfRotation,
                rotationMatrices,
                displacementForConstructionSite,
                _data->cellMap,
                tempCellMap)) {
            _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_OBSTACLE;
            return;
        }
    }

    auto const kineticEnergyBeforeRotation =
        Physics::kineticEnergy(cluster->numCellPointers, cluster->vel, cluster->angularMass, cluster->angularVel);

    auto const angularMassAfterRotation = calcAngularMassAfterTransformationAndAddingCell(
        cluster, relPosOfNewCell, centerOfRotation, rotationMatrices, displacementForConstructionSite);
    auto const angularVelAfterRotation =
        Physics::angularVelocity(cluster->angularMass, angularMassAfterRotation, cluster->angularVel);
    auto const kineticEnergyAfterRotation = Physics::kineticEnergy(
        cluster->numCellPointers, cluster->vel, angularMassAfterRotation, angularVelAfterRotation);

    auto const kineticEnergyDiff = kineticEnergyAfterRotation - kineticEnergyBeforeRotation;

    auto const energyForNewEntities = adaptEnergies(_token, kineticEnergyDiff);
    if (!energyForNewEntities.energyAvailable) {
        _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::ERROR_NO_ENERGY;
        return;
    }

    transformClusterComponents(cluster, centerOfRotation, rotationMatrices, displacementForConstructionSite);

    EntityFactory factory;
    factory.init(_data);
    auto const newCell = constructNewCell(_token, cluster, relPosOfNewCell, energyForNewEntities.cell, factory);
    auto const newCellPointers = _data->entities.cellPointers.getNewSubarray(cluster->numCellPointers + 1);
    addCellToCluster(newCell, cluster, newCellPointers);
    connectNewCell(newCell, firstCellOfConstructionSite, _token, cluster, _data);
    adaptRelPositions(cluster);
    completeCellAbsPosAndVel(cluster);
    cluster->angularVel = angularVelAfterRotation;
    cluster->angularMass = angularMassAfterRotation;

    firstCellOfConstructionSite->tokenBlocked = false;  //disable token blocking on construction side
    separateConstructionWhenFinished(_token, newCell);

    bool const createEmptyToken = Enums::ConstrInOption::CREATE_EMPTY_TOKEN == option
        || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option;
    bool const createDuplicateToken = Enums::ConstrInOption::CREATE_DUP_TOKEN == option;

    if (createEmptyToken || createDuplicateToken) {
        auto const newToken =
            constructNewToken(_token, newCell, cell, energyForNewEntities.token, factory, createDuplicateToken);
        auto const newTokenPointers = _data->entities.tokenPointers.getNewSubarray(cluster->numTokenPointers + 1);
        addTokenToCluster(newToken, cluster, newTokenPointers);
    }

    _token->memory[Enums::Constr::OUT] = Enums::ConstrOut::SUCCESS;
    _token->memory[Enums::Constr::INOUT_ANGLE] = 0;
}

__inline__ __device__ auto ConstructorFunction::adaptEnergies(Token* token, float energyLoss) -> EnergyForNewEntities
{
    EnergyForNewEntities result;
    result.energyAvailable = true;
    result.token = 0.0f;
    result.cell = cudaSimulationParameters.cellFunctionConstructorOffspringCellEnergy;

    auto const& cell = token->cell;
    auto const option = token->memory[Enums::Constr::IN_OPTION] % Enums::ConstrInOption::_COUNTER;

    if (Enums::ConstrInOption::CREATE_EMPTY_TOKEN == option || Enums::ConstrInOption::CREATE_DUP_TOKEN == option
        || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option) {
        result.token = cudaSimulationParameters.cellFunctionConstructorOffspringTokenEnergy;
    }

    if (token->energy <= cudaSimulationParameters.cellFunctionConstructorOffspringCellEnergy + result.token + energyLoss
            + cudaSimulationParameters.tokenMinEnergy) {
        result.energyAvailable = false;
        return result;
    }

    token->energy -= cudaSimulationParameters.cellFunctionConstructorOffspringCellEnergy + result.token + energyLoss;
    if (Enums::ConstrInOption::CREATE_EMPTY_TOKEN == option || Enums::ConstrInOption::CREATE_DUP_TOKEN == option
        || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option) {
        auto const averageEnergy = (cell->energy + result.cell) / 2;
        cell->energy = averageEnergy;
        result.cell = averageEnergy;
    }

    return result;
}

__inline__ __device__ void ConstructorFunction::tagConstructionSite(Cell* baseCell, Cell* firstCellOfConstructionSite)
{
    for (int cellIndex = _cellBlock.startIndex; cellIndex <= _cellBlock.endIndex; ++cellIndex) {
        auto& cell = _cluster->cellPointers[cellIndex];
        cell->tag = ClusterComponent::Constructor;
    }
    __syncthreads();

    if (0 == threadIdx.x) {
        firstCellOfConstructionSite->tag = ClusterComponent::ConstructionSite;
    }

    __shared__ bool changes;
    do {
        changes = false;
        __syncthreads();

        for (int cellIndex = _cellBlock.startIndex; cellIndex <= _cellBlock.endIndex; ++cellIndex) {
            auto& cell = _cluster->cellPointers[cellIndex];
            for (int i = 0; i < cell->numConnections; ++i) {
                auto& otherCell = cell->connections[i];
                if (otherCell->tag > cell->tag) {
                    if (cell == firstCellOfConstructionSite && otherCell == baseCell
                        || cell == baseCell && otherCell == firstCellOfConstructionSite) {
                        continue;
                    }
                    cell->tag = otherCell->tag;
                    changes = true;
                }
            }
        }
        __syncthreads();
    } while (changes);
}

__inline__ __device__ void ConstructorFunction::calcMaxAngles(Cell* constructionCell, Angles& result)
{
    if (0 == threadIdx.x) {
        result = { 360.0f, 360.0f };
    }
    __syncthreads();
    for (int cellIndex = _cellBlock.startIndex; cellIndex <= _cellBlock.endIndex; ++cellIndex) {
        auto const& cell = _cluster->cellPointers[cellIndex];
        auto r = Math::length(cell->relPos - constructionCell->relPos);
        if (cudaSimulationParameters.cellMaxDistance < 2 * r) {
            auto a = abs(2.0 * asinf(cudaSimulationParameters.cellMaxDistance / (2.0 * r)) * RAD_TO_DEG);
            if (ClusterComponent::Constructor == cell->tag) {
                result.constructor = min(result.constructor, a);
            }
            if (ClusterComponent::ConstructionSite == cell->tag) {
                result.constructionSite = min(result.constructionSite, a);
            }
        }
    }
}

__inline__ __device__ auto ConstructorFunction::calcAngularMasses(Cluster* cluster, Cell* constructionCell)
    -> AngularMasses
{
    AngularMasses result{0, 0};
    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        if (ClusterComponent::Constructor == cell->tag) {
            result.constructor = result.constructor + Math::lengthSquared(cell->relPos - constructionCell->relPos);
        }
        if (ClusterComponent::ConstructionSite == cell->tag) {
            result.constructionSite =
                result.constructionSite + Math::lengthSquared(cell->relPos - constructionCell->relPos);
        }
    }
    return result;
}

__inline__ __device__ auto ConstructorFunction::calcRotationMatrices(Angles const& angles) -> RotationMatrices
{
    RotationMatrices result;
    Math::rotationMatrix(angles.constructionSite, result.constructionSite);
    Math::rotationMatrix(angles.constructor, result.constructor);
    return result;
}

__inline__ __device__ float ConstructorFunction::calcFreeAngle(Cell* cell)
{
    auto const numConnections = cell->numConnections;
    float angles[MAX_CELL_BONDS];
    for (int i = 0; i < numConnections; ++i) {
        auto const displacement = cell->connections[i]->relPos - cell->relPos;
        auto const angleToAdd = Math::angleOfVector(displacement);
        auto indexToAdd = 0;
        for (; indexToAdd < i; ++indexToAdd) {
            if (angles[indexToAdd] > angleToAdd) {
                break;
            }
        }
        for (int j = indexToAdd; j < numConnections - 1; ++j) {
            angles[j + 1] = angles[j];
        }
        angles[indexToAdd] = angleToAdd;
    }

    auto largestAnglesDiff = 0.0f;
    auto result = 0.0f;
    for (int i = 0; i < numConnections; ++i) {
        auto angleDiff = angles[(i + 1) % numConnections] - angles[i];
        if (angleDiff <= 0.0f) {
            angleDiff += 360.0f;
        }
        if (angleDiff > 360.0f) {
            angleDiff -= 360.0f;
        }
        if (angleDiff > largestAnglesDiff) {
            largestAnglesDiff = angleDiff;
            result = angles[i] + angleDiff / 2;
        }
    }

    return result;
}

__inline__ __device__ auto ConstructorFunction::calcAnglesToRotate(
    AngularMasses const& angularMasses,
    float desiredAngleBetweenConstructurAndConstructionSite) -> Angles
{
    Angles result;
    auto const sumAngularMasses = angularMasses.constructor + angularMasses.constructionSite;
    result.constructionSite =
        angularMasses.constructor * desiredAngleBetweenConstructurAndConstructionSite / sumAngularMasses;
    result.constructor =
        -angularMasses.constructionSite * desiredAngleBetweenConstructurAndConstructionSite / sumAngularMasses;

    return result;
}

__inline__ __device__ bool ConstructorFunction::restrictAngles(Angles& angles, Angles const& maxAngles)
{
    auto result = false;
    if (abs(angles.constructionSite) > maxAngles.constructionSite) {
        result = true;
        if (angles.constructionSite >= 0.0) {
            angles.constructionSite = abs(maxAngles.constructionSite);
        }
        if (angles.constructionSite < 0.0) {
            angles.constructionSite = -abs(maxAngles.constructionSite);
        }
    }
    if (abs(angles.constructor) > maxAngles.constructor) {
        result = true;
        if (angles.constructor >= 0.0) {
            angles.constructor = abs(maxAngles.constructor);
        }
        if (angles.constructor < 0.0) {
            angles.constructor = -abs(maxAngles.constructor);
        }
    }
    return result;
}

__inline__ __device__ float ConstructorFunction::calcAngularMassAfterTransformationAndAddingCell(
    Cluster* cluster,
    float2 const& relPosOfNewCell,
    float2 const& centerOfRotation,
    RotationMatrices const& rotationMatrices,
    float2 const& displacementOfConstructionSite)
{

    auto center = relPosOfNewCell;
    for (auto cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        auto cellRelPosTransformed =
            getTransformedCellRelPos(cell, centerOfRotation, rotationMatrices, displacementOfConstructionSite);
        center = center + cellRelPosTransformed;
    }
    center = center / cluster->numCellPointers;

    auto result = Math::lengthSquared(relPosOfNewCell - center);
    for (auto cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        auto cellRelPosTransformed =
            getTransformedCellRelPos(cell, centerOfRotation, rotationMatrices, displacementOfConstructionSite);
        result += Math::lengthSquared(cellRelPosTransformed - center);
    }
    return result;
}

__inline__ __device__ float ConstructorFunction::calcAngularMassAfterAddingCell(
    Cluster* cluster,
    float2 const& relPosOfNewCell)
{
    auto center = relPosOfNewCell;
    for (auto cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        center = center + cell->relPos;
    }
    center = center / cluster->numCellPointers;

    auto result = Math::lengthSquared(relPosOfNewCell - center);
    for (auto cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        result += Math::lengthSquared(cell->relPos - center);
    }
    return result;
}

__inline__ __device__ void ConstructorFunction::transformClusterComponents(
    Cluster* cluster,
    float2 const& centerOfRotation,
    RotationMatrices const& rotationMatrices,
    float2 const& displacementForConstructionSite)
{
    float rotMatrix[2][2];
    Math::rotationMatrix(cluster->angle, rotMatrix);
    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        cell->relPos =
            getTransformedCellRelPos(cell, centerOfRotation, rotationMatrices, displacementForConstructionSite);
        cell->absPos = Math::applyMatrix(cell->relPos, rotMatrix) + cluster->pos;
    }
}

__inline__ __device__ void ConstructorFunction::adaptRelPositions(Cluster* cluster)
{
    float2 newCenter{0, 0};
    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        newCenter = newCenter + cell->relPos;
    }
    newCenter = newCenter / cluster->numCellPointers;

    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        cell->relPos = cell->relPos - newCenter;
    }
}

__inline__ __device__ void ConstructorFunction::completeCellAbsPosAndVel(Cluster* cluster)
{
    Math::Matrix rotationMatrix;
    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto& cell = cluster->cellPointers[cellIndex];
        Math::rotationMatrix(cluster->angle, rotationMatrix);
        cell->absPos = Math::applyMatrix(cell->relPos, rotationMatrix) + cluster->pos;
    }

    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto& cell = cluster->cellPointers[cellIndex];
        auto r = cell->absPos - cluster->pos;
        cell->vel = Physics::tangentialVelocity(r, cluster->vel, cluster->angularVel);
    }
}

__inline__ __device__ float2 ConstructorFunction::getTransformedCellRelPos(
    Cell* cell,
    float2 const& centerOfRotation,
    RotationMatrices const& matrices,
    float2 const& displacementForConstructionSite)
{
    if (ClusterComponent::Constructor == cell->tag) {
        return Math::applyMatrix(cell->relPos - centerOfRotation, matrices.constructor) + centerOfRotation;
    }
    if (ClusterComponent::ConstructionSite == cell->tag) {
        return Math::applyMatrix(cell->relPos - centerOfRotation, matrices.constructionSite) + centerOfRotation
            + displacementForConstructionSite;
    }
    return cell->relPos;
}

__inline__ __device__ bool ConstructorFunction::isObstaclePresent_onlyRotation(
    bool ignoreOwnCluster,
    Cluster* cluster,
    float2 const& centerOfRotation,
    RotationMatrices const& rotationMatrices,
    Map<Cell> const& map,
    HashMap<int2, CellAndNewAbsPos>& tempMap)
{
    float2 newCenter{0, 0};

    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        auto relPos = getTransformedCellRelPos(cell, centerOfRotation, rotationMatrices, {0, 0});
        newCenter = newCenter + relPos;
    }
    newCenter = newCenter / cluster->numCellPointers;

    Math::Matrix clusterMatrix;
    Math::rotationMatrix(cluster->angle, clusterMatrix);
    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        auto relPos = getTransformedCellRelPos(cell, centerOfRotation, rotationMatrices, {0, 0});
        relPos = relPos - newCenter;
        auto const absPos = cluster->pos + Math::applyMatrix(relPos, clusterMatrix);
        if (isObstaclePresent_helper(ignoreOwnCluster, cluster, cell, absPos, map, tempMap)) {
            return true;
        }
    }
    return false;
}

__inline__ __device__ bool ConstructorFunction::isObstaclePresent_rotationAndCreation(
    bool ignoreOwnCluster,
    Cluster* cluster,
    float2 const& relPosOfNewCell,
    float2 const& centerOfRotation,
    RotationMatrices const& rotationMatrices,
    float2 const& displacementOfConstructionSite,
    Map<Cell> const& map,
    HashMap<int2, CellAndNewAbsPos>& tempMap)
{
    float2 newCenter = relPosOfNewCell;

    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        auto relPos =
            getTransformedCellRelPos(cell, centerOfRotation, rotationMatrices, displacementOfConstructionSite);
        newCenter = newCenter + relPos;
    }
    newCenter = newCenter / (cluster->numCellPointers + 1);

    Math::Matrix clusterMatrix;
    Math::rotationMatrix(cluster->angle, clusterMatrix);
    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        auto relPos =
            getTransformedCellRelPos(cell, centerOfRotation, rotationMatrices, displacementOfConstructionSite);
        relPos = relPos - newCenter;
        auto const absPos = cluster->pos + Math::applyMatrix(relPos, clusterMatrix);
        if (isObstaclePresent_helper(ignoreOwnCluster, cluster, cell, absPos, map, tempMap)) {
            return true;
        }
    }
    return false;
}

__inline__ __device__ bool ConstructorFunction::isObstaclePresent_firstCreation(
    bool ignoreOwnCluster,
    Cluster* cluster,
    float2 const& relPosOfNewCell,
    Map<Cell> const& map,
    HashMap<int2, CellAndNewAbsPos>& tempMap)
{
    auto newCenter = relPosOfNewCell;
    for (auto cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        newCenter = newCenter + cell->relPos;
    }
    newCenter = newCenter / (cluster->numCellPointers + 1);

    Math::Matrix clusterMatrix;
    Math::rotationMatrix(cluster->angle, clusterMatrix);
    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        auto const relPos = cell->relPos - newCenter;
        auto const absPos = cluster->pos + Math::applyMatrix(relPos, clusterMatrix);
        if (isObstaclePresent_helper(ignoreOwnCluster, cluster, cell, absPos, map, tempMap)) {
            return true;
        }
    }

    //check obstacle for cell to be constructed
    auto const absPosForNewCell = cluster->pos + Math::applyMatrix(relPosOfNewCell - newCenter, clusterMatrix);
    if (isObstaclePresent_helper(ignoreOwnCluster, cluster, nullptr, absPosForNewCell, map, tempMap)) {
        return true;
    }

    return false;
}

__inline__ __device__ bool ConstructorFunction::isObstaclePresent_helper(
    bool ignoreOwnCluster,
    Cluster* cluster,
    Cell* cell,
    float2 const& absPos,
    Map<Cell> const& map,
    HashMap<int2, CellAndNewAbsPos>& tempMap)
{
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            float2 const lookupPos = {absPos.x + dx, absPos.y + dy};
            if (auto const otherCell = map.get(lookupPos)) {
                if (cluster != otherCell->cluster) {
                    if (map.mapDistance(otherCell->absPos, absPos) < cudaSimulationParameters.cellMinDistance) {
                        return true;
                    }

                    //check also connected cells
                    for (int i = 0; i < otherCell->numConnections; ++i) {
                        auto const connectedOtherCell = otherCell->connections[i];
                        if (map.mapDistance(connectedOtherCell->absPos, absPos)
                            < cudaSimulationParameters.cellMinDistance) {
                            return true;
                        }
                    }
                }
            }
            if (!ignoreOwnCluster) {
                auto const lookupPosInt = toInt2(lookupPos);
                if (tempMap.contains(lookupPosInt)) {
                    auto const otherCellAndNewPos = tempMap[lookupPosInt];

                    if (cell != otherCellAndNewPos.cell) {
                        if (map.mapDistance(otherCellAndNewPos.newAbsPos, absPos)
                            < cudaSimulationParameters.cellMinDistance) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    if (!ignoreOwnCluster && cell) {
        tempMap.insertOrAssign(toInt2(absPos), CellAndNewAbsPos{cell, absPos});
    }
    return false;
}

__inline__ __device__ Cell* ConstructorFunction::constructNewCell(
    Token* token,
    Cluster* cluster,
    float2 const& relPosOfNewCell,
    float const energyOfNewCell,
    EntityFactory& factory)
{
    auto result = factory.createCell(cluster);
    result->energy = energyOfNewCell;
    result->relPos = relPosOfNewCell;
    float rotMatrix[2][2];
    Math::rotationMatrix(cluster->angle, rotMatrix);
    result->absPos = Math::applyMatrix(result->relPos, rotMatrix) + cluster->pos;
    result->maxConnections = token->memory[Enums::Constr::IN_CELL_MAX_CONNECTIONS];
    result->numConnections = 0;
    result->branchNumber =
        token->memory[Enums::Constr::IN_CELL_BRANCH_NO] % cudaSimulationParameters.cellMaxTokenBranchNumber;
    result->tokenBlocked = true;
    result->cellFunctionType = token->memory[Enums::Constr::IN_CELL_FUNCTION];
    result->numStaticBytes = token->memory[Enums::Constr::IN_CELL_FUNCTION_DATA];
    for (int i = 0; i < result->numStaticBytes; ++i) {
        result->staticData[i] = token->memory[Enums::Constr::IN_CELL_FUNCTION_DATA + i + 1];
    }
    int offset = result->numStaticBytes + 1;
    result->numMutableBytes = token->memory[Enums::Constr::IN_CELL_FUNCTION_DATA + offset];
    for (int i = 0; i < result->numMutableBytes; ++i) {
        result->mutableData[i] = token->memory[Enums::Constr::IN_CELL_FUNCTION_DATA + offset + i + 1];
    }
    return result;
}

__inline__ __device__ Token* ConstructorFunction::constructNewToken(
    Token* token,
    Cell* cellOfNewToken,
    Cell* sourceCellOfNewToken,
    float energyOfNewToken,
    EntityFactory& factory,
    bool duplicate)
{
    auto result = factory.createToken(cellOfNewToken);
    result->sourceCell = sourceCellOfNewToken;
    result->energy = energyOfNewToken;
    if (duplicate) {
        for (int i = 1; i < MAX_TOKEN_MEM_SIZE; ++i) {  //do not copy branchnumber (at address 0)
            result->memory[i] = token->memory[i];
        }
    } else {
        for (int i = 1; i < MAX_TOKEN_MEM_SIZE; ++i) {  //do not copy branchnumber (at address 0)
            result->memory[i] = 0;
        }
    }
    return result;
}

__inline__ __device__ void
ConstructorFunction::addCellToCluster(Cell* newCell, Cluster* cluster, Cell** newCellPointers)
{
    for (int i = 0; i < cluster->numCellPointers; ++i) {
        newCellPointers[i] = cluster->cellPointers[i];
    }
    newCellPointers[cluster->numCellPointers] = newCell;
    cluster->cellPointers = newCellPointers;
    ++cluster->numCellPointers;
}

__inline__ __device__ void
ConstructorFunction::addTokenToCluster(Token* token, Cluster* cluster, Token** newTokenPointers)
{
    for (int i = 0; i < cluster->numTokenPointers; ++i) {
        newTokenPointers[i] = cluster->tokenPointers[i];
    }
    newTokenPointers[cluster->numTokenPointers] = token;
    cluster->tokenPointers = newTokenPointers;
    ++cluster->numTokenPointers;
}

__inline__ __device__ void ConstructorFunction::separateConstructionWhenFinished(Token* token, Cell* newCell)
{
    auto const& cell = token->cell;
    auto const& cluster = cell->cluster;
    auto const option = token->memory[Enums::Constr::IN_OPTION] % Enums::ConstrInOption::_COUNTER;

    if (Enums::ConstrInOption::FINISH_NO_SEP == option || Enums::ConstrInOption::FINISH_WITH_SEP == option
        || Enums::ConstrInOption::FINISH_WITH_SEP_RED == option
        || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option) {
        newCell->tokenBlocked = false;
    }

    if (Enums::ConstrInOption::FINISH_WITH_SEP == option || Enums::ConstrInOption::FINISH_WITH_SEP_RED == option
        || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option) {
        atomicExch(&cluster->decompositionRequired, 1);
        removeConnection(newCell, cell);
    }
    if (Enums::ConstrInOption::FINISH_WITH_SEP_RED == option
        || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option) {
        --newCell->maxConnections;
        --cell->maxConnections;
    }
}

__inline__ __device__ void ConstructorFunction::connectNewCell(
    Cell* newCell,
    Cell* cellOfConstructionSite,
    Token* token,
    Cluster* cluster,
    SimulationData* data)
{
    Cell* cellOfConstructor = token->cell;

    auto const adaptMaxConnections = isAdaptMaxConnections(token);

    removeConnection(cellOfConstructionSite, cellOfConstructor);
    establishConnection(newCell, cellOfConstructionSite, adaptMaxConnections);
    establishConnection(newCell, cellOfConstructor, adaptMaxConnections);

    if (newCell->numConnections >= cudaSimulationParameters.cellMaxBonds) {
        return;
    }

    for (int cellIndex = 0; cellIndex < cluster->numCellPointers; ++cellIndex) {
        auto const& cell = cluster->cellPointers[cellIndex];
        if (ClusterComponent::ConstructionSite != cell->tag) {
            continue;
        }
        if (cell->numConnections >= cudaSimulationParameters.cellMaxBonds) {
            continue;
        }
        if (cell == cellOfConstructionSite) {
            continue;
        }
        if (_data->cellMap.mapDistance(cell->absPos, newCell->absPos) >= cudaSimulationParameters.cellMaxDistance) {
            continue;
        }
        if (!isConnectable(cell, adaptMaxConnections) || !isConnectable(newCell, adaptMaxConnections)) {
            continue;
        }
        establishConnection(newCell, cell, adaptMaxConnections);
    }
}

__inline__ __device__ void ConstructorFunction::removeConnection(Cell* cell1, Cell* cell2)
{
    auto remove = [](Cell* cell, Cell* connectionToRemove) {
        bool connectionFound = false;
        for (int i = 0; i < cell->numConnections; ++i) {
            auto& connectingCell = cell->connections[i];
            if (!connectionFound) {
                if (connectingCell == connectionToRemove) {
                    connectionFound = true;
                }
            } else {
                cell->connections[i - 1] = connectingCell;
            }
        }
        --cell->numConnections;
    };

    remove(cell1, cell2);
    remove(cell2, cell1);
}

__inline__ __device__ auto ConstructorFunction::isAdaptMaxConnections(Token* token) -> AdaptMaxConnections
{
    return 0 == token->memory[Enums::Constr::IN_CELL_MAX_CONNECTIONS] ? AdaptMaxConnections::Yes
                                                                      : AdaptMaxConnections::No;
}

__inline__ __device__ bool ConstructorFunction::isConnectable(Cell* cell, AdaptMaxConnections adaptMaxConnections)
{
    if (AdaptMaxConnections::Yes == adaptMaxConnections) {
        if (cell->numConnections == cudaSimulationParameters.cellMaxBonds) {
            return false;
        }
    }
    if (AdaptMaxConnections::No == adaptMaxConnections) {
        if (cell->numConnections == cell->maxConnections) {
            return false;
        }
    }
    return true;
}

__inline__ __device__ void
ConstructorFunction::establishConnection(Cell* cell1, Cell* cell2, AdaptMaxConnections adaptMaxConnections)
{
    cell1->connections[cell1->numConnections++] = cell2;
    cell2->connections[cell2->numConnections++] = cell1;

    if (adaptMaxConnections == AdaptMaxConnections::Yes) {
        cell1->maxConnections = cell1->numConnections;
        cell2->maxConnections = cell2->numConnections;
    }
}
