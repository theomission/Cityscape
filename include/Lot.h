//
//  Lot.h
//  Cityscape
//
//  Created by andrew morton on 2/16/15.
//
//

#ifndef __Cityscape__Lot__
#define __Cityscape__Lot__

#include "FlatShape.h"
#include "Building.h"
#include "Options.h"

class Lot {
  public:
    Lot( const Lot &src ) : mShape(src.mShape), mId(src.mId), mColor(src.mColor), mBuilding(src.mBuilding), buildingPosition(src.buildingPosition) { };
    Lot( const unsigned int lid, const FlatShape &fs ) : mShape(fs), mId(lid) { };

    void layout();
    void draw( const Options &options );

    unsigned int mId;
    FlatShape mShape;
    ci::ColorA mColor;
    Building mBuilding;
    ci::Vec2f buildingPosition;
};

#endif /* defined(__Cityscape__Lot__) */
