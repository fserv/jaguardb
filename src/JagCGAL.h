#ifndef _jag_cgal_h_
#define _jag_cgal_h_

#include <abax.h>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/algorithms/perimeter.hpp>
#include <boost/geometry/algorithms/is_convex.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/polygon/voronoi.hpp>
#include <boost/polygon/voronoi_geometry_type.hpp>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/boost/graph/graph_traits_Surface_mesh.h>
#include <CGAL/boost/graph/Euler_operations.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh_simplification/edge_collapse.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Count_ratio_stop_predicate.h>
#include <CGAL/Memory_sizer.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/intersections.h>

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Cartesian_d.h>
#include <CGAL/Min_sphere_of_spheres_d.h>
#include <CGAL/Min_circle_2.h>
#include <CGAL/Min_circle_2_traits_2.h>


#include <JagGeom.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel CGALKernel;
typedef CGALKernel::Point_2 CGALPoint2D;
typedef CGALKernel::Point_3 CGALPoint3D;
typedef CGAL::Surface_mesh<CGALKernel::Point_3> CGALSurfaceMesh;
typedef CGALKernel::Segment_2 CGALSegment2D;

typedef CGAL::Cartesian_d<double>  CGALCartDouble;
typedef CGALCartDouble::Point_d  CGALCartPoint;
typedef CGAL::Min_sphere_of_spheres_d_traits_d<CGALCartDouble,double,3> CGAL3DTraits;
typedef CGAL::Min_sphere_of_spheres_d<CGAL3DTraits> CGAL_Min_sphere;
typedef CGAL3DTraits::Sphere         CGALSphere;

typedef CGAL::Min_sphere_of_spheres_d_traits_d<CGALCartDouble,double,2> CGAL2DTraits;
typedef CGAL::Min_sphere_of_spheres_d<CGAL2DTraits> CGAL_Min_circle;
typedef CGAL2DTraits::Sphere         CGALCircle;

typedef boost::geometry::model::d2::point_xy<double> BoostPoint2D;
typedef boost::geometry::model::linestring<BoostPoint2D> BoostLineString2D;
typedef boost::geometry::model::polygon<BoostPoint2D,false> BoostPolygon2D; // counter-clock-wise
typedef boost::geometry::model::polygon<BoostPoint2D,true> BoostPolygon2DCW; // clock-wise
typedef boost::geometry::ring_type<BoostPolygon2D>::type BoostRing2D; // counter-clock-wise
typedef boost::geometry::ring_type<BoostPolygon2DCW>::type BoostRing2DCW; // clock-wise

typedef boost::geometry::model::multi_polygon<BoostPolygon2D> BoostMultiPolygon2D; 
typedef boost::geometry::model::multi_polygon<BoostPolygon2DCW> BoostMultiPolygon2DCW; 
typedef boost::geometry::model::multi_linestring<BoostLineString2D> BoostMultiLineString2D;

typedef boost::geometry::strategy::buffer::distance_symmetric<double> JagDistanceSymmetric;
typedef boost::geometry::strategy::buffer::distance_asymmetric<double> JagDistanceASymmetric;
typedef boost::geometry::strategy::buffer::side_straight JagSideStraight;
typedef boost::geometry::strategy::buffer::join_round JagJoinRound;
typedef boost::geometry::strategy::buffer::join_miter JagJoinMiter;
typedef boost::geometry::strategy::buffer::end_round JagEndRound;
typedef boost::geometry::strategy::buffer::end_flat JagEndFlat;
typedef boost::geometry::strategy::buffer::point_circle JagPointCircle;
typedef boost::geometry::strategy::buffer::point_square JagPointSquare;

using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;
namespace boost {
  namespace polygon {
    struct PointLong2D 
    {
        long long x;
        long long y;
        PointLong2D(long long ix, long long iy) : x(ix), y(iy) {} 
        PointLong2D() {} 
    };
    
    template <> struct geometry_concept<PointLong2D> { typedef point_concept type; };
    template <> struct point_traits<PointLong2D> 
    {
      	typedef long long coordinate_type;
    	static inline coordinate_type get(const PointLong2D &point, orientation_2d orient) 
    	{
    	    return (orient == HORIZONTAL) ? point.x : point.y; 
    	}
    };
} }
typedef boost::polygon::PointLong2D BoostPointLong2D;

class JagVoronoiPoint2D
{
  public:
     JagVoronoiPoint2D() { side = 0; }
     JagVoronoiPoint2D( double inx, double iny, int sd=0 ): x(inx), y(iny), side(sd) {}
	 bool operator == ( const JagVoronoiPoint2D &o) const {
	 	if ( jagEQ(x, o.x) && jagEQ(y,o.y) ) { return true; } else { return false; }
	 }
	 bool operator != ( const JagVoronoiPoint2D &o) const {
	 	if ( ! (jagEQ(x, o.x) && jagEQ(y,o.y) ) ) { return true; } else { return false; }
	 }
     double x, y;
	 int  side;
};


class JagStrategy
{
  public:
  	JagStrategy( const Jstr &nm, double a1, double a2 );
  	~JagStrategy();
	void  *ptr;
  	Jstr name;
	short  t;
  protected:
	double f1, f2;
};


class JagCGAL
{
  public:
    static void getConvexHull2DStr( const JagLineString &line, const Jstr &hdr, const Jstr &bbox, Jstr &value );
    static void getConvexHull2D( const JagLineString &line, JagLineString &hull );

    static void getConvexHull3DStr( const JagLineString &line, const Jstr &hdr, const Jstr &bbox, Jstr &value );
    static void getConvexHull3D( const JagLineString &line, JagLineString &hull );

    static bool getBufferLineString2DStr( const JagLineString &line, int srid, const Jstr &arg, Jstr &value );
    static bool getBufferMultiPoint2DStr( const JagLineString &line, int srid, const Jstr &arg, Jstr &value );
    static bool getBufferPolygon2DStr( const JagPolygon &pgon, int srid, const Jstr &arg, Jstr &value );
    static bool getBufferMultiLineString2DStr( const JagPolygon &pgon, int srid, const Jstr &arg, Jstr &value );
    static bool getBufferMultiPolygon2DStr( const JagVector<JagPolygon> &pgvec, int srid, const Jstr &arg, Jstr &value );

    static bool getIsSimpleLineString2DStr( const JagLineString &line );
    static bool getIsSimpleMultiLineString2DStr( const JagPolygon &pgon );
    static bool getIsSimplePolygon2DStr( const JagPolygon &pgon );
    static bool getIsSimpleMultiPolygon2DStr( const JagVector<JagPolygon> &pgvec );
    static bool isPolygonConvex( const JagPolygon &pgon );

    static bool getIsValidLineString2DStr( const JagLineString &line );
    static bool getIsValidMultiLineString2DStr( const JagPolygon &pgon );
    static bool getIsValidPolygon2DStr( const JagPolygon &pgon );
    static bool getIsValidMultiPolygon2DStr( const JagVector<JagPolygon> &pgvec );
    static bool getIsValidMultiPoint2DStr( const JagLineString &line );

    static bool getIsRingLineString2DStr( const JagLineString &line );

	template <class TGeo>
    static bool getBuffer2D( const TGeo &obj, const Jstr &arg, JagVector<JagPolygon> &pgvec );

	static bool get2DStrFromMultiPolygon( const JagVector<JagPolygon> &pgvec, int srid, Jstr &value );
    static void getRingStr( const JagLineString &line, const Jstr &hdr, const Jstr &bbox, bool is3D, Jstr &value );
    static void getOuterRingsStr( const JagVector<JagPolygon> &pgvec, const Jstr &hdr, const Jstr &bbox, 
									bool is3D, Jstr &value );
    static void getInnerRingsStr( const JagVector<JagPolygon> &pgvec, const Jstr &hdr, const Jstr &bbox, 
									bool is3D, Jstr &value );
    static void getPolygonNStr( const JagVector<JagPolygon> &pgvec, const Jstr &hdr, const Jstr &bbox, 
							   bool is3D, int N, Jstr &value );
    static void getUniqueStr( const JagStrSplit &sp, const Jstr &hdr, const Jstr &bbox, Jstr &value );
	static int unionOfTwoPolygons( const JagStrSplit &sp1, const JagStrSplit &sp2, Jstr &wkt );
	static int unionOfPolygonAndMultiPolygons( const JagStrSplit &sp1, const JagStrSplit &sp2, Jstr &wkt );
	static bool hasIntersection( const JagLine2D &line1, const JagLine2D &line2, JagVector<JagPoint2D> &res ); 
	static bool hasIntersection( const JagLine3D &line1, const JagLine3D &line2, JagVector<JagPoint3D> &res ); 
	static void split2DSPToVector( const JagStrSplit &sp, JagVector<JagPoint2D> &vec1 );
	static void split3DSPToVector( const JagStrSplit &sp, JagVector<JagPoint3D> &vec1 );
	static int  getTwoPolygonIntersection( const JagPolygon &pgon1, const JagPolygon &pgon2, JagVector<JagPolygon> &vec );
	static bool convertPolygonJ2B( const JagPolygon &pgon1, BoostPolygon2D &bgon );

	template <class GEOM1, class GEOM2, class RESULT> 
	static void getTwoGeomDifference( const Jstr &wkt1, const Jstr &wkt2, Jstr &reswkt );
	template <class GEOM1, class GEOM2, class RESULT> 
	static void getTwoGeomSymDifference( const Jstr &wkt1, const Jstr &wkt2, Jstr &reswkt );
	static void getVoronoiPolygons2D( int srid, const JagStrSplit &sp, double tolerance, 
									  double xmin, double ymin, double xmax, double ymax, const Jstr &retType, Jstr &vor );
	static void getVoronoiMultiPolygons3D( const JagStrSplit &sp, double tolerance, 
									  double xmin, double ymin, double zmin, double xmax, double ymax, double zmax, Jstr &vor );
	static void getDelaunayTriangles2D( int srid, const JagStrSplit &sp, double tolerance, Jstr &mpg );
	static bool getMinBoundCircle( int srid, const JagVector<JagPoint2D> &vec, Jstr &res );
	static bool getMinBoundSphere( int srid, const JagVector<JagPoint3D> &vec, Jstr &res );
	static int  pointRelateLine( double px, double py, double x1, double y1, double x2, double y2);
	static int  lineRelateLine( double x1, double y1, double x2, double y2, double mx1, double my1, double mx2, double my2 );
    static void getConcaveHull2DStr( const JagLineString3D &lstr, const Jstr &hdr, const Jstr &bbox, Jstr &value );


  protected:
  	static bool createStrategies( JagStrategy *sptr[], const Jstr &arg );
  	static void destroyStrategies( JagStrategy *sptr[] );
	static int getIntersectionPointWithBox( double vx, double vy,
	                    				    const BoostPointLong2D &p1, const BoostPointLong2D &p2,
						                    double xmin, double ymin, double xmax, double ymax, 
											double &bx, double &by );
	static int getIntersectionPointOfTwoLines( double x1, double y1, double x2, double y2, double vx, double vy, JagPoint2D &jp );
	static void fillInCorners(const JagBox2D &bbox, const JagVector<JagVoronoiPoint2D> &vec1, JagVector<JagVoronoiPoint2D> &vec2);

};

class JagIntersectionVisitor2D
{
  public:
    typedef void result_type;
    JagIntersectionVisitor2D( JagVector<JagPoint2D> &res ) : _res( res ) { }

    void operator()(const CGALKernel::Point_2& p) const
    {
		_res.append(JagPoint2D(p.x(), p.y()) );
    }

    void operator()(const CGALKernel::Segment_2& seg) const
    {
		CGALPoint2D s = seg.source();
		CGALPoint2D t = seg.target();
		_res.append(JagPoint2D(s.x(), s.y() ));
		_res.append(JagPoint2D(t.x(), t.y() ));
    }

	JagVector<JagPoint2D> &_res;
};


class JagIntersectionVisitor3D
{
  public:
    typedef void result_type;
    JagIntersectionVisitor3D( JagVector<JagPoint3D> &res ) : _res( res ) { }

    void operator()(const CGALKernel::Point_3& p) const
    {
		_res.append(JagPoint3D(p.x(), p.y(), p.z() ) );
    }

    void operator()(const CGALKernel::Segment_3& seg) const
    {
		CGALPoint3D s = seg.source();
		CGALPoint3D t = seg.target();
		_res.append(JagPoint3D(s.x(), s.y(), s.z() ));
		_res.append(JagPoint3D(t.x(), t.y(), t.z() ));
    }

	JagVector<JagPoint3D> &_res;
};


template <class GEOM1, class GEOM2, class RESULT>
void JagCGAL::getTwoGeomDifference( const Jstr &wkt1, const Jstr &wkt2, Jstr &reswkt )
{
	RESULT result;
	GEOM1 geom1;
	boost::geometry::read_wkt( wkt1.c_str(), geom1 );
	GEOM1 geom2;
	boost::geometry::read_wkt( wkt2.c_str(), geom2 );
	boost::geometry::difference( geom1, geom2, result );
	std::stringstream ifs;
	ifs << boost::geometry::wkt(result);
	reswkt = ifs.str().c_str();
}

template <class GEOM1, class GEOM2, class RESULT>
void JagCGAL::getTwoGeomSymDifference( const Jstr &wkt1, const Jstr &wkt2, Jstr &reswkt )
{
	RESULT result;
	GEOM1 geom1;
	boost::geometry::read_wkt( wkt1.c_str(), geom1 );
	GEOM1 geom2;
	boost::geometry::read_wkt( wkt2.c_str(), geom2 );
	boost::geometry::sym_difference( geom1, geom2, result );
	std::stringstream ifs;
	ifs << boost::geometry::wkt(result);
	reswkt = ifs.str().c_str();
}

#endif
