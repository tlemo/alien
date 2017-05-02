#ifndef MAPCOMPARTMENT_H
#define MAPCOMPARTMENT_H

#include "model/definitions.h"

class MapCompartment
	: public QObject
{
	Q_OBJECT
public:
	MapCompartment(QObject* parent) : QObject(parent) {}
	virtual ~MapCompartment() {}

	virtual void init(SpaceMetric* metric, IntRect mapRect) = 0;

	enum class RelativeLocation {
		UpperLeft, Upper, UpperRight, 
		Left, Right, 
		LowerLeft, Lower, LowerRight,
	};
	virtual void registerNeighborContext(RelativeLocation location, UnitContext* context) = 0;
	virtual UnitContext* getNeighborContext(RelativeLocation location) const = 0;
	virtual UnitContext* getNeighborContext(IntVector2D pos) const = 0;

private:
};

#endif // MAPCOMPARTMENT_H
