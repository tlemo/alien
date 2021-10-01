#include "DescriptionHelper.h"

#include "Base/NumberGenerator.h"
#include "SpaceCalculator.h"

void DescriptionHelper::duplicate(DataDescription& data, IntVector2D const& origSize, IntVector2D const& size)
{
    DataDescription result;

    for (int incX = 0; incX < size.x; incX += origSize.x) {
        for (int incY = 0; incY < size.y; incY += origSize.y) {
            if (data.clusters) {
                for (auto cluster : *data.clusters) {
                    auto origPos = cluster.getClusterPosFromCells();
                    RealVector2D clusterPos = {origPos.x + incX, origPos.y + incY};
                    if (clusterPos.x < size.x && clusterPos.y < size.y) {
                        if (cluster.cells) {
                            for (auto& cell : *cluster.cells) {
                                auto origPos = *cell.pos;
                                cell.pos = RealVector2D{origPos.x + incX, origPos.y + incY};
                            }
                        }
                        makeValid(cluster);
                        result.addCluster(cluster);
                    }
                }
            }
            if (data.particles) {
                for (auto particle : *data.particles) {
                    auto origPos = *particle.pos;
                    particle.pos = RealVector2D{origPos.x + incX, origPos.y + incY};
                    if (particle.pos->x < size.x && particle.pos->y < size.y) {
                        particle.setId(NumberGenerator::getInstance().getId());
                        result.addParticle(particle);
                    }
                }
            }
        }
    }
    data = result;
}

void DescriptionHelper::correctConnections(DataDescription& data, IntVector2D const& worldSize)
{
    SpaceCalculator spaceCalculator(worldSize);
    auto threshold = std::min(worldSize.x, worldSize.y) /3;
    if (data.clusters) {
        std::unordered_map<uint64_t, CellDescription&> cellById;
        for (auto& cluster : *data.clusters) {
            for (auto& cell : *cluster.cells) {
                cellById.emplace(cell.id, cell);
            }
        }
        for (auto& cluster : *data.clusters) {
            for (auto& cell: *cluster.cells) {
                std::list<ConnectionDescription> newConnections;
                float angleToAdd = 0;
                for (auto connection : *cell.connections) {
                    auto& connectingCell = cellById.at(connection.cellId);
                    if (spaceCalculator.distance(*cell.pos, *connectingCell.pos) > threshold) {
                        angleToAdd += connection.angleFromPrevious;
                    } else {
                        connection.angleFromPrevious += angleToAdd;
                        angleToAdd = 0;
                        newConnections.emplace_back(connection);
                    }
                }
                cell.connections = newConnections;
            }
        }
    }
}

void DescriptionHelper::makeValid(ClusterDescription& cluster)
{
    auto& numberGen = NumberGenerator::getInstance();
    cluster.id = numberGen.getId();
    if (cluster.cells) {
        unordered_map<uint64_t, uint64_t> newByOldIds;
        for (auto& cell : *cluster.cells) {
            uint64_t newId = numberGen.getId();
            newByOldIds.insert_or_assign(cell.id, newId);
            cell.id = newId;
        }

        for (auto& cell : *cluster.cells) {
            if (cell.connections) {
                for (auto& connection : *cell.connections) {
                    connection.cellId = newByOldIds.at(connection.cellId);
                }
            }
        }
    }
}
