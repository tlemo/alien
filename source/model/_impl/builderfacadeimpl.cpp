#include "global/numbergenerator.h"
#include "global/servicelocator.h"
#include "model/entities/cell.h"
#include "model/entities/cellcluster.h"
#include "model/entities/energyparticle.h"
#include "model/entities/token.h"
#include "model/entities/entityfactory.h"
#include "model/features/cellfunction.h"
#include "model/features/cellfunctioncomputer.h"
#include "model/features/energyguidance.h"
#include "model/features/cellfeaturefactory.h"
#include "model/context/simulationparameters.h"
#include "model/context/cellmap.h"
#include "model/context/energyparticlemap.h"
#include "model/context/spacemetric.h"
#include "model/context/contextfactory.h"
#include "model/context/mapcompartment.h"
#include "model/context/threadcontroller.h"
#include "model/context/simulationgrid.h"
#include "model/context/simulationcontext.h"
#include "model/context/simulationunit.h"
#include "model/context/simulationunitcontext.h"
#include "model/metadata/symboltable.h"
#include "model/modelsettings.h"

#include "builderfacadeimpl.h"

namespace {
	BuilderFacadeImpl factoryFacadeImpl;
}

BuilderFacadeImpl::BuilderFacadeImpl ()
{
    ServiceLocator::getInstance().registerService<BuilderFacade>(this);
}

SimulationContext* BuilderFacadeImpl::buildSimulationContext(int maxRunngingThreads, IntVector2D gridSize, SpaceMetric* metric, SymbolTable* symbolTable
	, SimulationParameters* parameters, QObject* parent) const
{
	ContextFactory* factory = ServiceLocator::getInstance().getService<ContextFactory>();
	SimulationContext* context = factory->buildSimulationContext(parent);

	auto threads = factory->buildSimulationThreads(context);
	threads->init(maxRunngingThreads);

	auto grid = factory->buildSimulationGrid(context);
	grid->init(gridSize, metric);

	parameters->setParent(context);
	symbolTable->setParent(context);
	context->init(metric, grid, threads, symbolTable, parameters);

	for (int x = 0; x < gridSize.x; ++x) {
		for (int y = 0; y < gridSize.y; ++y) {
			auto unit = buildSimulationUnit({ x,y }, context);
			grid->registerUnit({ x,y }, unit);
			threads->registerUnit(unit);
		}
	}

	for (int x = 0; x < gridSize.x; ++x) {
		for (int y = 0; y < gridSize.y; ++y) {
			auto unitContext = grid->getUnit({ x, y })->getContext();
			auto compartment = unitContext->getMapCompartment();
			auto getContextFromDelta = [&](IntVector2D const& delta) {
				return grid->getUnit({ (x + delta.x + gridSize.x) % gridSize.x, (y + delta.x + gridSize.y) % gridSize.y })->getContext();
			};
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::UpperLeft, getContextFromDelta({ -1, -1 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::Upper, getContextFromDelta({ 0, -1 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::UpperRight, getContextFromDelta({ +1, -1 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::Left, getContextFromDelta({ -1, 0 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::Right, getContextFromDelta({ +1, 0 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::LowerLeft, getContextFromDelta({ -1, +1 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::Lower, getContextFromDelta({ 0, +1 }));
			compartment->registerNeighborContext(MapCompartment::RelativeLocation::LowerRight, getContextFromDelta({ +1, +1 }));
		}
	}

	return context;
}

SimulationUnit * BuilderFacadeImpl::buildSimulationUnit(IntVector2D gridPos, SimulationContext* context) const
{
	ContextFactory* factory = ServiceLocator::getInstance().getService<ContextFactory>();
	auto grid = context->getSimulationGrid();
	auto threads = context->getSimulationThreads();
	auto unit = factory->buildSimulationUnit();		//unit has no parent due to an QObject::moveToThread call later
	auto unitContext = factory->buildSimulationUnitContext(unit);
	auto metric = context->getTopology()->clone(unit);
	auto compartment = factory->buildMapCompartment(unit);
	auto cellMap = factory->buildCellMap(unit);
	auto energyMap = factory->buildEnergyParticleMap(unit);
	auto symbolTable = context->getSymbolTable()->clone(unit);
	auto parameters = context->getSimulationParameters()->clone(unit);
	compartment->init(metric, grid->calcMapRect(gridPos));
	cellMap->init(metric, compartment);
	energyMap->init(metric, compartment);
	unitContext->init(metric, cellMap, energyMap, compartment, symbolTable, parameters);
	unit->init(unitContext);

	return unit;
}

SpaceMetric * BuilderFacadeImpl::buildSpaceMetric(IntVector2D universeSize, QObject* parent) const
{
	ContextFactory* factory = ServiceLocator::getInstance().getService<ContextFactory>();
	auto metric = factory->buildSpaceMetric(parent);
	metric->init(universeSize);
	return metric;
}

CellCluster* BuilderFacadeImpl::buildCellCluster (SimulationUnitContext* context) const
{
    EntityFactory* entityFactory = ServiceLocator::getInstance().getService<EntityFactory>();
    return entityFactory->buildCellCluster(context);
}

CellCluster* BuilderFacadeImpl::buildCellCluster (QList< Cell* > cells, qreal angle, QVector3D pos, qreal angularVel
    , QVector3D vel, SimulationUnitContext* context) const
{
    EntityFactory* entityFactory = ServiceLocator::getInstance().getService<EntityFactory>();
    return entityFactory->buildCellCluster(cells, angle, pos, angularVel, vel, context);
}


Cell* BuilderFacadeImpl::buildFeaturedCell (qreal energy, Enums::CellFunction::Type type, QByteArray data
    , SimulationUnitContext* context, int maxConnections, int tokenAccessNumber, QVector3D relPos) const
{
    EntityFactory* entityFactory = ServiceLocator::getInstance().getService<EntityFactory>();
    CellFeatureFactory* decoratorFactory = ServiceLocator::getInstance().getService<CellFeatureFactory>();
    Cell* cell = entityFactory->buildCell(energy, context, maxConnections, tokenAccessNumber, relPos);
    decoratorFactory->addCellFunction(cell, type, data, context);
    decoratorFactory->addEnergyGuidance(cell, context);
    return cell;
}

Cell* BuilderFacadeImpl::buildFeaturedCell (qreal energy, Enums::CellFunction::Type type, SimulationUnitContext* context
    , int maxConnections, int tokenAccessNumber, QVector3D relPos) const
{
    EntityFactory* entityFactory = ServiceLocator::getInstance().getService<EntityFactory>();
    CellFeatureFactory* decoratorFactory = ServiceLocator::getInstance().getService<CellFeatureFactory>();
    Cell* cell = entityFactory->buildCell(energy, context, maxConnections, tokenAccessNumber, relPos);
    decoratorFactory->addCellFunction(cell, type, context);
    decoratorFactory->addEnergyGuidance(cell, context);
    return cell;
}

Cell* BuilderFacadeImpl::buildFeaturedCellWithRandomData (qreal energy, SimulationUnitContext* context) const
{
	SimulationParameters* parameters = context->getSimulationParameters();
    int randomMaxConnections = NumberGenerator::getInstance().random(parameters->cellMaxBonds+1);
    int randomTokenAccessNumber = NumberGenerator::getInstance().random(parameters->cellMaxTokenBranchNumber);
    QByteArray randomData(256, 0);
	for (int i = 0; i < 256; ++i)
		randomData[i] = NumberGenerator::getInstance().random(256);
    Enums::CellFunction::Type randomCellFunction = static_cast<Enums::CellFunction::Type>(NumberGenerator::getInstance().random(Enums::CellFunction::_COUNTER));
    return buildFeaturedCell(energy, randomCellFunction, randomData, context, randomMaxConnections, randomTokenAccessNumber, QVector3D());
}

Token* BuilderFacadeImpl::buildToken(SimulationUnitContext* context, qreal energy) const
{
	EntityFactory* entityFactory = ServiceLocator::getInstance().getService<EntityFactory>();
	return entityFactory->buildToken(context, energy);
}

CellTO BuilderFacadeImpl::buildFeaturedCellTO (Cell* cell) const
{
    CellTO to;

    //copy cell properties
    CellCluster* cluster = cell->getCluster();
    to.numCells = cluster->getMass();
    to.clusterPos = cluster->getPosition();
    to.clusterVel = cluster->getVelocity();
    to.clusterAngle = cluster->getAngle();
    to.clusterAngVel = cluster->getAngularVel();
    to.cellPos = cell->calcPosition();
    to.cellEnergy = cell->getEnergy();
    to.cellNumCon = cell->getNumConnections();
    to.cellMaxCon = cell->getMaxConnections();
    to.cellAllowToken = !cell->isTokenBlocked();
    to.cellTokenAccessNum = cell->getBranchNumber();
    CellFunction* cellFunction = cell->getFeatures()->findObject<CellFunction>();
    to.cellFunctionType = cellFunction->getType();

    //copy computer data
    CellFunctionComputer* computer = cellFunction->findObject<CellFunctionComputer>();
    if( computer ) {
        QByteArray d = computer->getMemoryReference();
		to.computerMemory = d;
        to.computerCode = computer->decompileInstructionCode();
    }

    //copy token data
    for(int i = 0; i < cell->getNumToken(); ++i) {
        Token* token = cell->getToken(i);
        to.tokenEnergies << token->getEnergy();
        to.tokenData << token->getMemoryRef();
    }
    return to;
}

void BuilderFacadeImpl::changeFeaturesOfCell (Cell* cell, Enums::CellFunction::Type type, SimulationUnitContext* context) const
{
    cell->removeFeatures();
    CellFeatureFactory* decoratorFactory = ServiceLocator::getInstance().getService<CellFeatureFactory>();
    decoratorFactory->addCellFunction(cell, type, context);
    decoratorFactory->addEnergyGuidance(cell, context);
}

