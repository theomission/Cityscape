//
//  FlatShape.h
//  Cityscape
//
//  Created by andrew morton on 5/20/15.
//
//

#ifndef __Cityscape__FlatShape__
#define __Cityscape__FlatShape__

#include "cinder/TriMesh.h"
#include "cinder/Triangulate.h"
#include "CinderCGAL.h"

class FlatShape {
  public:
    typedef std::vector<ci::PolyLine2f> PolyLine2fs;

    FlatShape( const FlatShape &s )
        : mOutline(s.mOutline), mHoles(s.mHoles), mMesh(s.mMesh)
    {}
    FlatShape( const ci::PolyLine2f &outline, const PolyLine2fs &holes = {} )
        : mOutline(outline), mHoles(holes)
    {
        mMesh = makeMesh();
    };
    FlatShape( const CGAL::Polygon_with_holes_2<ExactK> &pwh )
    {
        mOutline = polyLineFrom<ExactK>( pwh.outer_boundary() );

        mHoles.reserve( pwh.number_of_holes() );
        for ( auto hit = pwh.holes_begin(); hit != pwh.holes_end(); ++hit ) {
            mHoles.push_back( polyLineFrom<ExactK>( *hit ) );
        }

        mMesh = makeMesh();
    };

    const ci::PolyLine2f outline() { return mOutline; }
    const PolyLine2fs holes() { return mHoles; }
    const ci::TriMesh mesh() { return mMesh; }

    const ci::vec2 centroid();

    template<class K>
    inline const CGAL::Polygon_2<K> polygon()
    {
        return polygonFrom<K>( mOutline );
    }

    template<class K>
    inline const CGAL::Polygon_with_holes_2<K> polygon_with_holes()
    {
        CGAL::Polygon_with_holes_2<K> poly( polygon<K>() );
        for ( auto it = mHoles.begin(); it != mHoles.end(); ++it ) {
            poly.add_hole( polygonFrom<K>( *it ) );
        }
        return poly;
    }
    
  private:

    const ci::TriMesh makeMesh();

    ci::PolyLine2f mOutline;
    PolyLine2fs mHoles;
    ci::TriMesh mMesh;
};

#endif /* defined(__Cityscape__FlatShape__) */
