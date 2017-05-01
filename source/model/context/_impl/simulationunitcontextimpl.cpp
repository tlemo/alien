#include "model/context/cellmap.h"
#include "model/context/energyparticlemap.h"
#include "model/context/simulationparameters.h"
#include "model/entities/cellcluster.h"
#include "model/entities/energyparticle.h"
#include "model/metadata/symboltable.h"

#include "simulationunitcontextimpl.h"

SimulationUnitContextImpl::SimulationUnitContextImpl(QObject* parent)
	: SimulationUnitContext(parent)
{
}

SimulationUnitContextImpl::~SimulationUnitContextImpl ()
{
	deleteClustersAndEnergyParticles();
}

void SimulationUnitContextImpl::init(SpaceMetric* metric, CellMap* cellMap, EnergyParticleMap* energyMap, SymbolTable* symbolTable, SimulationParameters* parameters)
{
	if (_metric != metric) {
		delete _metric;
		_metric = metric;
	}
	if (_cellMap != cellMap) {
		delete _cellMap;
		_cellMap = cellMap;
	}
	if (_energyParticleMap != energyMap) {
		delete _energyParticleMap;
		_energyParticleMap = energyMap;
	}
	if (_symbolTable != symbolTable) {
		delete _symbolTable;
		_symbolTable = symbolTable;
	}
	if (_simulationParameters != parameters) {
		delete _simulationParameters;
		_simulationParameters = parameters;
	}

	_energyParticleMap->init(_metric);
	_cellMap->init(_metric);
	deleteClustersAndEnergyParticles();
}

void SimulationUnitContextImpl::lock()
{
	_mutex.lock();
}

void SimulationUnitContextImpl::unlock()
{
	_mutex.unlock();
}

SpaceMetric* SimulationUnitContextImpl::getTopology () const
{
    return _metric;
}

EnergyParticleMap* SimulationUnitContextImpl::getEnergyParticleMap () const
{
    return _energyParticleMap;
}

CellMap* SimulationUnitContextImpl::getCellMap () const
{
    return _cellMap;
}

SymbolTable* SimulationUnitContextImpl::getSymbolTable() const 
{
	return _symbolTable;
}

SimulationParameters* SimulationUnitContextImpl::getSimulationParameters() const
{
	return _simulationParameters;
}

QList<CellCluster*>& SimulationUnitContextImpl::getClustersRef ()
{
    return _clusters;
}

QList<EnergyParticle*>& SimulationUnitContextImpl::getEnergyParticlesRef ()
{
    return _energyParticles;
}

void SimulationUnitContextImpl::deleteClustersAndEnergyParticles()
{
	foreach(CellCluster* cluster, _clusters) {
		delete cluster;
	}
	_clusters.clear();
	foreach(EnergyParticle* particle, _energyParticles) {
		delete particle;
	}
	_energyParticles.clear();
}

