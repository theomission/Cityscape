//
//  Building.h
//  Cityscape
//
//  Created by andrew morton on 2/16/15.
//
//

#ifndef __Cityscape__Building__
#define __Cityscape__Building__

//#include <boost/flyweight.hpp>
//#include <boost/flyweight/key_value.hpp>
//using namespace ::boost::flyweights;

#include "BuildingPlan.h"
#include "Options.h"

class Building;
typedef std::shared_ptr<Building> BuildingRef;


class Building {
  public:

    static BuildingRef create( const BuildingPlan &plan, uint32_t floors = 1,
        ci::vec2 position = ci::vec2(0, 0), float rotation = 0 )
    {
        return BuildingRef( new Building( plan, floors, position, rotation ) );
    }

    Building( const BuildingPlan &plan, uint32_t floors = 1,
        ci::vec2 position = ci::vec2(0, 0), float rotation = 0 )
        : mPlan(plan), mFloors(floors), mPosition(position), mRotation(rotation)
    {};
    Building( const Building &s )
        : mPlan(s.mPlan), mFloors(s.mFloors), mPosition(s.mPosition), mRotation(s.mRotation)
    {};

    void layout( const Options &options );
    void draw( const Options &options ) const;

    const BuildingPlan plan() { return mPlan; };
    const ci::PolyLine2f outline() const
    {
        return mPlan.outline(mPosition, mRotation);
    }

private:
    BuildingPlan mPlan;
    uint32_t mFloors;
    ci::vec2 mPosition;
    float mRotation; // radians
};

#endif /* defined(__Cityscape__Building__) */
