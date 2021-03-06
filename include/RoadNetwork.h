//
//  RoadNetwork.h
//  Cityscape
//
//  Created by Andrew Morton on 5/31/15.
//
//

#ifndef __Cityscape__RoadNetwork__
#define __Cityscape__RoadNetwork__

#include "Options.h"
#include "Block.h"
#include "FlatShape.h"

#include <CGAL/Polygon_set_2.h>

class RoadNetwork {
public:
    RoadNetwork() {}

    void clear()
    {
        mPoints.clear();
    }

    // Insert multiple points and avoid a layout for each.
    void addPoints( const std::vector<ci::vec2> &points )
    {
        mPoints.insert( mPoints.end(), points.begin(), points.end() );
    }

    void addPoint( const ci::vec2 &pos )
    {
        mPoints.push_back( pos );
    }
    void layout( const Options &options );
    void draw( const Options &options );

private:
    void buildHighways( const Options &options, CGAL::Polygon_set_2<ExactK> &paved );
    void buildSideStreets( const Options &options, CGAL::Polygon_set_2<ExactK> &paved );
    void buildBlocks( const Options &options );

    std::vector<ci::vec2> mPoints;
    std::vector<Block> mBlocks;
    std::vector<FlatShape> mShapes;
};

#endif /* defined(__Cityscape__RoadNetwork__) */
