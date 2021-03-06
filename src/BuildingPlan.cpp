//
//  BuildingPlan.cpp
//  Cityscape
//
//  Created by andrew morton on 6/8/15.
//
//

#include "BuildingPlan.h"
#include "CinderCGAL.h"
#include "CgalArrangement.h"
#include "CgalStraightSkeleton.h"

#include "cinder/Rand.h"
#include "cinder/Triangulate.h"

using namespace ci;
using namespace ci::geom;
using namespace std;


typedef std::map<std::pair<float, float>, vec3> OffsetMap;



BuildingPlan::RoofStyle BuildingPlan::randomRoof()
{
    return static_cast<RoofStyle>(ci::randInt(5));
}

ci::PolyLine2f BuildingPlan::triangle()
{
    return ci::PolyLine2f( {
        ci::vec2(10, -10), ci::vec2(10, 10), ci::vec2(-10, 10),
        ci::vec2(10, -10) // closure
    } );
}

ci::PolyLine2f BuildingPlan::square()
{
    return rectangle( 20, 20 );
}

ci::PolyLine2f BuildingPlan::rectangle( const uint16_t width, const uint16_t height )
{
    float w = width / 2.0;
    float h = height / 2.0;
    return ci::PolyLine2f( {
        ci::vec2(w, -h), ci::vec2(w, h),
        ci::vec2(-w, h), ci::vec2(-w, -h),
        ci::vec2(w, -h) // closure
    } );
}

ci::PolyLine2f BuildingPlan::lshape()
{
    return ci::PolyLine2f( {
        ci::vec2(15, 0), ci::vec2(15, 10), ci::vec2(-15, 10),
        ci::vec2(-15, -10), ci::vec2(-5, -10), ci::vec2(-5, 0),
        ci::vec2(15, 0) // closure
    } );
}

ci::PolyLine2f BuildingPlan::plus()
{
    return ci::PolyLine2f( {
        ci::vec2(15,-5), ci::vec2(15,5), ci::vec2(5,5),
        ci::vec2(5,15), ci::vec2(-5,15), ci::vec2(-5,5),
        ci::vec2(-15,5), ci::vec2(-15,-5), ci::vec2(-5,-5),
        ci::vec2(-5,-15), ci::vec2(5,-15), ci::vec2(5,-5),
        ci::vec2(15,-5) // closure
    } );
}

ci::PolyLine2f BuildingPlan::tee()
{
    return ci::PolyLine2f( {
        ci::vec2(5,10), ci::vec2(-5,10), ci::vec2(-5,0),
        ci::vec2(-15,0), ci::vec2(-15,-10), ci::vec2(15,-10),
        ci::vec2(15,0), ci::vec2(5,0),
        ci::vec2(5,10) // closure
    } );
}

ci::PolyLine2f BuildingPlan::randomOutline()
{
    switch (randInt(5)) {
        case 0:
            return triangle();
        case 1:
            return square();
        case 2:
            return lshape();
        case 3:
            return plus();
        case 4:
            return tee();
        default:
            return rectangle( 10 * randInt( 3 ), 10 * randInt( 4 ) );
    }
}

const ci::PolyLine2f BuildingPlan::outline(const ci::vec2 offset, const float rotation) const
{
    glm::mat3 matrix;
    matrix = rotate( translate( matrix, offset ), rotation );

    PolyLine2f ret = PolyLine2f();
    for( auto it = mOutline.begin(); it != mOutline.end(); ++it ) {
        vec3 transformed = matrix * vec3( *it, 1 );
        ret.push_back( vec2( transformed ) );
    }
    return ret;
}

BuildingPlanRef BuildingPlan::create( const ci::PolyLine2f &outline, const BuildingPlan::RoofStyle roof )
{
    return BuildingPlanRef( new BuildingPlan( outline, roof ) );
}

BuildingPlanRef BuildingPlan::createRandom()
{
    return BuildingPlanRef( new BuildingPlan( randomOutline(), randomRoof() ) );
}

// * * *

class WallMesh : public Source {
public:
    WallMesh( const PolyLine2f &outline, const OffsetMap &topOffsets, const float defaultTopHeight )
    {
        uint32_t base = 0;
        for ( auto i = outline.begin(); i != outline.end(); ++i ) {
            vec3 bottom = vec3( i->x, i->y, 0.0 );
            auto it = topOffsets.find( std::make_pair( i->x, i->y ) );
            vec3 topOffset = ( it == topOffsets.end() ) ? vec3( 0.0, 0.0, defaultTopHeight ) : it->second;

            mPositions.push_back( bottom );
            mPositions.push_back( bottom + topOffset );
        }
        uint16_t totalVerts = mPositions.size();

        uint32_t i;
        for ( i = base + 2; i < totalVerts; i += 2 ) {
            mIndices.push_back( i + 0 );
            mIndices.push_back( i + 1 );
            mIndices.push_back( i - 1 );
            mIndices.push_back( i + 0 );
            mIndices.push_back( i - 1 );
            mIndices.push_back( i - 2 );
        }
        mIndices.push_back( base + 0 );
        mIndices.push_back( base + 1 );
        mIndices.push_back( i - 1 );
        mIndices.push_back( base + 0 );
        mIndices.push_back( i - 1 );
        mIndices.push_back( i - 2 );
    };

    size_t    getNumVertices() const override { return mPositions.size(); }
    size_t    getNumIndices() const override { return mIndices.size(); }
    Primitive getPrimitive() const override { return Primitive::TRIANGLES; }
    uint8_t   getAttribDims( Attrib attr ) const override
    {
        switch( attr ) {
            case Attrib::POSITION: return 3;
            default: return 0;
        }
    }

    AttribSet getAvailableAttribs() const override { return { Attrib::POSITION }; }
    void    loadInto( Target *target, const AttribSet &requestedAttribs ) const override
    {
        target->copyAttrib( Attrib::POSITION, 3, 0, (const float*)mPositions.data(), mPositions.size() );
        target->copyIndices( Primitive::TRIANGLES, mIndices.data(), mIndices.size(), 4 );
    }
    WallMesh* clone() const override { return new WallMesh( *this ); }


protected:
    std::vector<vec3>       mPositions;
    std::vector<uint32_t>   mIndices;
};

// * * *

// Build walls
//
// We're building the walls using individual triangles (rather than strip) so we
// can use Cinder's triangulator to build the roof.
//
void buildWallsFromOutlineAndTopOffsets(const PolyLine2f &outline, const OffsetMap &topOffsets, const float defaultTopHeight, vector<vec3> &verts, vector<uint32_t> &indices)
{
    uint32_t base = verts.size();
    for ( auto i = outline.begin(); i != outline.end(); ++i ) {
        vec3 bottom = vec3( i->x, i->y, 0.0 );
        auto it = topOffsets.find( std::make_pair( i->x, i->y ) );
        vec3 topOffset = ( it == topOffsets.end() ) ? vec3( 0.0, 0.0, defaultTopHeight ) : it->second;

        verts.push_back( bottom );
        verts.push_back( bottom + topOffset );
    }
    uint16_t totalVerts = verts.size();

    uint32_t i = base + 2;
    for ( ; i < totalVerts; i += 2 ) {
        indices.push_back( i + 0 );
        indices.push_back( i + 1 );
        indices.push_back( i - 1 );
        indices.push_back( i + 0 );
        indices.push_back( i - 1 );
        indices.push_back( i - 2 );
    }
    indices.push_back( base + 0 );
    indices.push_back( base + 1 );
    indices.push_back( i - 1 );
    indices.push_back( base + 0 );
    indices.push_back( i - 1 );
    indices.push_back( i - 2 );
}

// Use cinder's triangulator to convert a polygon into a face of a roof.
// The offsets adjust the positions of the verticies, allowing a 2d outline to
// fill a 3d space
void buildRoofFaceFromOutlineAndOffsets( const PolyLine2f &outline, const OffsetMap &offsets, vector<vec3> &verts, vector<uint32_t> &indices )
{
    uint32_t index = verts.size();
    ci::Triangulator triangulator( outline );
    ci::TriMesh roofMesh = triangulator.calcMesh();

    const vec2 *positions = roofMesh.getPositions<2>();
    uint32_t numVerts = roofMesh.getNumVertices();
    for ( uint32_t i = 0; i < numVerts ; i++ ) {
        vec3 position( positions[i], 0 );
        auto it = offsets.find( std::make_pair( position.x, position.y ) );
        vec3 offset = it == offsets.end() ? vec3(0) : it->second;
        verts.push_back( offset + position );
    }

    std::vector<uint32_t> roofIndices = roofMesh.getIndices();
    for ( auto i = roofIndices.begin(); i != roofIndices.end(); ++i) {
        indices.push_back( index + *i );
    }
}

// Compute vertex height based off distance from incident edges
OffsetMap heightOfSkeleton( const SsPtr &skel )
{
    OffsetMap heightMap;
    for( auto vert = skel->vertices_begin(); vert != skel->vertices_end(); ++vert ) {
        if (vert->is_contour()) { continue; }

        InexactK::Point_2 p = vert->point();
        heightMap[ std::make_pair( p.x(), p.y() ) ] = vec3( 0, 0, vert->time() );
    }
    return heightMap;
}

// - triangulate roof faces and add to mesh
void buildRoofFromSkeletonAndOffsets( const SsPtr &skel, const OffsetMap &offsets, vector<vec3> &verts, vector<uint32_t> &indices )
{
    for( auto face = skel->faces_begin(); face != skel->faces_end(); ++face ) {
        PolyLine2f faceOutline;
        Ss::Halfedge_handle start = face->halfedge(),
        edge = start;
        do {
            faceOutline.push_back( vecFrom( edge->vertex()->point() ) );
            edge = edge->next();
        } while (edge != start);

        buildRoofFaceFromOutlineAndOffsets( faceOutline, offsets, verts, indices );
    }
}

void buildFlatRoof(const PolyLine2f &outline, vector<vec3> &verts, vector<uint32_t> &indices)
{
    OffsetMap empty;
    buildRoofFaceFromOutlineAndOffsets( outline, empty, verts, indices );
}

void buildHippedRoof(const PolyLine2f &outline, vector<vec3> &verts, vector<uint32_t> &indices)
{
    // - build straight skeleton
    SsPtr skel = CGAL::create_interior_straight_skeleton_2( polygonFrom<InexactK>( outline ), InexactK() );

    // - compute vertex height based off distance from incident edges
    OffsetMap offsetMap = heightOfSkeleton( skel );

    // - triangulate roof faces and add to mesh
    buildRoofFromSkeletonAndOffsets(skel, offsetMap, verts, indices);
}

void buildGabledRoof(const PolyLine2f &outline, vector<vec3> &verts, vector<uint32_t> &indices)
{
    // - build straight skeleton
    SsPtr skel = CGAL::create_interior_straight_skeleton_2( polygonFrom<InexactK>( outline ), InexactK() );

    // - compute vertex height based off distance from incident edges
    OffsetMap offsetMap = heightOfSkeleton( skel );

    // - find faces with 3 edges: 1 skeleton and 2 contour
    for( auto face = skel->faces_begin(); face != skel->faces_end(); ++face ) {
        // Move around the face until we get to an edge with a skeleton
        // (seems to be the second edge).
        Ss::Halfedge_handle skelEdge = face->halfedge();
        do {
            skelEdge = skelEdge->next();
        } while ( !skelEdge->vertex()->is_skeleton() );

        // Bail if we don't have two contour verts followed by the skeleton vert.
        Ss::Halfedge_handle contourA = skelEdge->next();
        Ss::Halfedge_handle contourB = contourA->next();
        if (!contourA->vertex()->is_contour()) continue;
        if (!contourB->vertex()->is_contour()) continue;
        if (contourB->next() != skelEdge) continue;

        // Find point where skeleton vector intersects contour edge
        vec2 A = vecFrom( contourA->vertex()->point() );
        vec2 B = vecFrom( contourB->vertex()->point() );
        vec2 C = vecFrom( skelEdge->vertex()->point() );
        vec2 adjustment =  ( ( B + A ) / vec2( 2.0 ) ) - C;

        // Adjust the position
        auto it = offsetMap.find( std::make_pair( C.x, C.y ) );
        if ( it != offsetMap.end() ) {
            it->second += vec3( adjustment, 0.0 );
        }
    }

    // - triangulate roof faces and add to mesh
    buildRoofFromSkeletonAndOffsets(skel, offsetMap, verts, indices);
}

// TODO:
// - decide on how to orient the slope of the roof. one option is to find
//   longest side of the outline and use that to define the roof plane. another
//   would be passing in an angle.
// - get a proper formula for determining height
void buildShedRoof(const PolyLine2f &outline, const float slope, vector<vec3> &verts, vector<uint32_t> &indices)
{
    // For now we find the left most point and have the roof slope up along the
    // x-axis from there.
    assert( outline.size() > 0 );
    float leftmost = outline.begin()->x;
    for (auto i = outline.begin() + 1; i != outline.end(); ++i) {
        if (i->x < leftmost) leftmost = i->x;
    }

    // - compute the height of vertexes base on position on the roof
    OffsetMap offsetMap;
    for (auto i = outline.begin(); i != outline.end(); ++i) {
        float z = slope * (i->x - leftmost);
        offsetMap[ std::make_pair( i->x, i->y ) ] = vec3( 0, 0, z );
    }

    // - triangulate roof faces and add to mesh
    buildRoofFaceFromOutlineAndOffsets( outline, offsetMap, verts, indices );
    buildWallsFromOutlineAndTopOffsets( outline, offsetMap, 0.0, verts, indices );
}

float sawtoothHeight( const float x, const float upWidth, const float height, const float downWidth )
{
    float p = fmod(x, upWidth + downWidth);
    if ( p < upWidth ) {
        return ( height / upWidth ) * p;
    } else {
        return ( -height / downWidth ) * ( p - upWidth ) + height;
    }
}

void buildSawtoothRoof(const PolyLine2f &outline, const float upWidth, const float height, const float downWidth, vector<vec3> &verts, vector<uint32_t> &indices)
{
    if (outline.size() == 0) return;

    Arrangement_2 arr;
    OffsetMap offsets;

    // Put the outline onto the arrangment.
    std::list<Segment_2> outlineSegments = contiguousSegmentsFrom( outline.getPoints() );
    insert_empty( arr, outlineSegments.begin(), outlineSegments.end() );

    // Create a list of segements to intersect with.
    std::list<Segment_2> intersect;
    intersect.insert( intersect.begin(), outlineSegments.begin(), outlineSegments.end() );

    // Then start walking across the outline looking for intersections.
    std::list<Segment_2> newEdges;
    std::list<Point_2> newPoints;
    u_int16_t step = 0;
    Rectf bounds = Rectf( outline.getPoints() );
    float x = bounds.x1;
    while ( x < bounds.x2 ) {
        intersect.push_back( Segment_2( Point_2( x, bounds.y2 ), Point_2( x, bounds.y1 ) ) );
        findIntersections( intersect, newEdges, newPoints );
        intersect.pop_back();

        x += (step % 2) ? downWidth : upWidth;
        ++step;
    };

    // Add all the new points and edges.
    Naive_pl pl(arr);
    for ( auto p = newPoints.begin(); p != newPoints.end(); ++p )
        insert_point( arr, *p, pl );
    if (newEdges.size())
        insert( arr, newEdges.begin(), newEdges.end() );

    // Compute a height for each vertex.
    for ( auto i = arr.vertices_begin(); i != arr.vertices_end(); ++i ) {
        vec2 v = vecFrom( i->point() );
        float h = sawtoothHeight( v.x - bounds.x1, upWidth, height, downWidth );
        offsets[ std::make_pair( v.x, v.y ) ] = vec3( 0, 0, h );
    }

    // Now turn the arrangment into a mesh.
    for ( auto i = arr.faces_begin(); i != arr.faces_end(); ++i ) {
        for ( auto j = i->holes_begin(); j != i->holes_end(); ++j ) {
            PolyLine2f faceOutline( polyLineFrom( *j ).reversed() );

            buildRoofFaceFromOutlineAndOffsets( faceOutline, offsets, verts, indices );
            buildWallsFromOutlineAndTopOffsets( faceOutline, offsets, 0.0, verts, indices );
        }
    }
}

class RoofMesh : public Source {
public:
    RoofMesh( const PolyLine2f &outline, BuildingPlan::RoofStyle roof )
    {
        switch ( roof ) {
            case BuildingPlan::FLAT_ROOF:
                buildFlatRoof( outline, mPositions, mIndices );
                break;
            case BuildingPlan::HIPPED_ROOF:
                buildHippedRoof( outline, mPositions, mIndices );
                break;
            case BuildingPlan::GABLED_ROOF:
                buildGabledRoof( outline, mPositions, mIndices );
                break;
            case BuildingPlan::SAWTOOTH_ROOF:
                buildSawtoothRoof( outline, 10, 3, 5, mPositions, mIndices );
                break;
            case BuildingPlan::SHED_ROOF:
                // Make slope configurable... might be good for other angled roofs.
                buildShedRoof( outline, 0.2, mPositions, mIndices );
                break;
            case BuildingPlan::GAMBREL_ROOF:
                // probably based off GABLED with an extra division of the faces to give it the barn look
                break;
        }
    };

    size_t    getNumVertices() const override { return mPositions.size(); }
    size_t    getNumIndices() const override { return mIndices.size(); }
    Primitive getPrimitive() const override { return Primitive::TRIANGLES; }
    uint8_t   getAttribDims( Attrib attr ) const override
    {
        switch( attr ) {
            case Attrib::POSITION: return 3;
            default: return 0;
        }
    }

    AttribSet getAvailableAttribs() const override { return { Attrib::POSITION }; }
    void    loadInto( Target *target, const AttribSet &requestedAttribs ) const override
    {
        target->copyAttrib( Attrib::POSITION, 3, 0, (const float*)mPositions.data(), mPositions.size() );
        target->copyIndices( Primitive::TRIANGLES, mIndices.data(), mIndices.size(), 4 );
    }
    RoofMesh* clone() const override { return new RoofMesh( *this ); }

protected:
    std::vector<vec3>       mPositions;
    std::vector<uint32_t>   mIndices;
};


void BuildingPlan::makeMesh()
{
    // Build the walls
    mWallMeshRef = gl::VboMesh::create( WallMesh( mOutline, {}, mFloorHeight ) );

    // Build roof
    mRoofMeshRef = gl::VboMesh::create( RoofMesh( mOutline, mRoof ) );
}