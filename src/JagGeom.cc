/*
/*
 * Copyright JaguarDB
 *
 * This file is part of JaguarDB.
 *
 * JaguarDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JaguarDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JaguarDB (LICENSE.txt). If not, see <http://www.gnu.org/licenses/>.
 */
#include <JagGlobalDef.h>
#include <math.h>
#include <JagGeom.h>
#include <JagArray.h>
#include "JagSortLinePoints.h"
#include "JagParser.h"

const double JagGeo::NUM_SAMPLE = 10;


JagLineSeg2D JagLineSeg2D::NULLVALUE = JagLineSeg2D(JAG_LONG_MIN,JAG_LONG_MIN,JAG_LONG_MIN,JAG_LONG_MIN);
JagLineSeg2DPair JagLineSeg2DPair::NULLVALUE = JagLineSeg2DPair( JagLineSeg2D::NULLVALUE );
JagLineSeg3D JagLineSeg3D::NULLVALUE = JagLineSeg3D(JAG_LONG_MIN,JAG_LONG_MIN,JAG_LONG_MIN,JAG_LONG_MIN,JAG_LONG_MIN,JAG_LONG_MIN);
JagLineSeg3DPair JagLineSeg3DPair::NULLVALUE = JagLineSeg3DPair( JagLineSeg3D::NULLVALUE );




///////////////////////////// Within methods ////////////////////////////////////////
bool JagGeo::pointInTriangle( double px, double py, double x1, double y1,
                              double x2, double y2, double x3, double y3,
                              bool strict, bool boundcheck )
{
	JagPoint2D point(px,py);
	JagPoint2D p1(x1,y1);
	JagPoint2D p2(x2,y2);
	JagPoint2D p3(x3,y3);
	return pointWithinTriangle( point, p1, p2, p3, strict, boundcheck );
}

// strict true: point strictly inside (no boundary) in triangle.
//        false: point can be on boundary
// boundcheck:  do bounding box check or not
bool JagGeo::pointWithinTriangle( const JagPoint2D &point,
                              const JagPoint2D &point1, const JagPoint2D &point2,
                              const JagPoint2D &point3, bool strict, bool boundcheck )
{
	double f;
	if ( boundcheck ) {
		f = ( point1.x < point2.x ) ?  point1.x : point2.x;
		f = (  f < point3.x ) ?  f : point3.x;
		if ( point.x < ( f - 0.01 ) ) { return false; }

		f = ( point1.y < point2.y ) ?  point1.y : point2.y;
		f = ( f < point3.y ) ?  f : point3.y;
		if ( point.y < ( f - 0.01 ) ) { return false; }


		f = ( point1.x > point2.x ) ?  point1.x : point2.x;
		f = (  f > point3.x ) ?  f : point3.x;
		if ( point.x > ( f + 0.01 ) ) { return false; }

		f = ( point1.y > point2.y ) ?  point1.y : point2.y;
		f = ( f > point3.y ) ?  f : point3.y;
		if ( point.y > ( f + 0.01 ) ) { return false; }
	}

	// simple check
	bool b1, b2, b3;
	b1=b2=b3=false;
	f = doSign( point, point1, point2 ); 
	if ( f <= 0.0 ) b1 = true;

	f = doSign( point, point2, point3 ); 
	if ( f <= 0.0 ) b2 = true;

	f = doSign( point, point3, point1 ); 
	if ( f <= 0.0 ) b3 = true;

	if ( b1 == b2 && b2 == b3 ) {
		return true;
	}

	if ( strict ) {
		return false;
	}

	// b1==b2==b3
 	if (distSquarePointToSeg( point, point1, point2 ) <= JAG_ZERO) {
		return true;
	}

 	if (distSquarePointToSeg( point, point2, point3 ) <= JAG_ZERO) {
		return true;
	}

 	if (distSquarePointToSeg( point, point3, point1 ) <= JAG_ZERO) {
		return true;
	}

	return false;
}

bool JagGeo::doPointWithin( const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = sp1[JAG_SP_START+0].tof();
	double py0 = sp1[JAG_SP_START+1].tof();

	if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		return pointWithinPoint( px0, py0, x0, y0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		return pointWithinLine( px0, py0, x1, y1, x2, y2, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_MULTIPOINT ) {
        dn("s4592820 pointWithinLineString ...");
		return pointWithinLineString( px0, py0, mk2, sp2, strict );
	} else if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
    	JagPoint2D point( sp1[JAG_SP_START+0].c_str(), sp1[JAG_SP_START+1].c_str() );
    	JagPoint2D p1( sp2[JAG_SP_START+0].c_str(), sp2[JAG_SP_START+1].c_str() );
    	JagPoint2D p2( sp2[JAG_SP_START+2].c_str(), sp2[JAG_SP_START+3].c_str() );
    	JagPoint2D p3( sp2[JAG_SP_START+4].c_str(), sp2[JAG_SP_START+5].c_str() );
		return pointWithinTriangle( point, p1, p2, p3, strict, true );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		double lon = meterToLon( srid2, r, x0, y0);
		double lat = meterToLat( srid2, r, x0, y0);
		return pointWithinRectangle( px0, py0, x0, y0, lon, lat, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0] ); 
		double y0 = jagatof( sp2[JAG_SP_START+1] ); 
		double w = jagatof( sp2[JAG_SP_START+2] ); 
		double h = jagatof( sp2[JAG_SP_START+3] ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double lon = meterToLon( srid2, w, x0, y0);
		double lat = meterToLat( srid2, h, x0, y0);
		return pointWithinRectangle( px0, py0, x0, y0, lon, lat, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = jagatof( sp2[JAG_SP_START+0] ); 
		double y = jagatof( sp2[JAG_SP_START+1] ); 
		double r = jagatof( sp2[JAG_SP_START+2] );
		double lon = meterToLon( srid2, r, x, y);
		double lat = meterToLat( srid2, r, x, y);
		// return pointWithinCircle( px0, py0, x, y, r, strict );
		d("s222203 x=%f y=%f  r=%f srid2=%d lon=%f  lat=%f\n", x, y, r, srid2, lon, lat );
		return pointWithinEllipse( px0, py0, x, y, lon, lat, 0.0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0] ); 
		double y0 = jagatof( sp2[JAG_SP_START+1] ); 
		double w = jagatof( sp2[JAG_SP_START+2] );
		double h = jagatof( sp2[JAG_SP_START+3] );
		double nx = safeget(sp2, JAG_SP_START+4);
		//double ny = safeget(sp2, JAG_SP_START+5);
		double lon = meterToLon( srid2, w, x0, y0);
		double lat = meterToLat( srid2, h, x0, y0);
		return pointWithinEllipse( px0, py0, x0, y0, lon, lat, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return pointWithinPolygon( px0, py0, mk2, sp2, strict );
	}
	return false;
}

bool JagGeo::doPoint3DWithin( const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	//d("s4409 doPoint3DWithin colType2=[%s]\n", colType2.c_str() );

	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		return point3DWithinPoint3D( px0, py0, pz0, x1, y1, z1, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINE3D ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		return point3DWithinLine3D( px0, py0, pz0, x1, y1, z1, x2, y2, z2, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return point3DWithinLineString3D( px0, py0, pz0, mk2, sp2, strict );
	} else if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return point3DWithinBox( px0, py0, pz0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double c = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return point3DWithinBox( px0, py0, pz0, x0, y0, z0, a,b,c, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return point3DWithinSphere( px0,py0,pz0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() );
		double b = jagatof( sp2[JAG_SP_START+4].c_str() );
		double c = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return point3DWithinEllipsoid( px0, py0, pz0, x0, y0, z0, a,b,c, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return point3DWithinCone( px0,py0,pz0, x, y, z, r, h, nx, ny, strict );
	}
	return false;
}

double JagGeo::doCircleArea( int srid1, const JagStrSplit &sp1 )
{
	//double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	//double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double r = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	return r * r * JAG_PI;
}

double JagGeo::doCirclePerimeter( int srid1, const JagStrSplit &sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	return 2.0 * r * JAG_PI;
}

bool JagGeo::doCircleWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	pr0 = meterToLon( srid2, pr0, px0, py0);

	if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() );
		r = meterToLon( srid2, r, x, y);
		return circleWithinCircle(px0,py0,pr0, x,y,r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() );
		r = meterToLon( srid2, r, x, y);
		double nx = safeget(sp2, JAG_SP_START+3);
		return circleWithinSquare(px0,py0,pr0, x,y,r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+2].c_str() );
		double h = jagatof( sp2[JAG_SP_START+3].c_str() );
		w = meterToLon( srid2, w, x0, y0);
		h = meterToLat( srid2, h, x0, y0);
		double nx = safeget(sp2, JAG_SP_START+4);
		return circleWithinRectangle( px0, py0, pr0, x0, y0, w, h, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		return circleWithinTriangle(px0, py0, pr0, x1, y1, x2, y2, x3, y3, strict, true );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		a = meterToLon( srid2, a, x0, y0);
		b = meterToLat( srid2, b, x0, y0);
		double nx = safeget(sp2, JAG_SP_START+4);
		return circleWithinEllipse(px0, py0, pr0, x0, y0, a, b, nx, strict, true );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return circleWithinPolygon(px0, py0, pr0, mk2, sp2, strict );
	}
	return false;
}

// circle surface with x y z and orientation
bool JagGeo::doCircle3DWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 

	double nx0 = 0.0;
	double ny0 = 0.0;
	if ( sp1.length() >= 5 ) { nx0 = jagatof( sp1[JAG_SP_START+4].c_str() ); }
	if ( sp1.length() >= 6 ) { ny0 = jagatof( sp1[JAG_SP_START+5].c_str() ); }

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return circle3DWithinCube( px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, r, nx, ny, strict );
	} else if (  colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double c = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return circle3DWithinBox( px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, a,b,c, nx, ny, strict );
	} else if (  colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return circle3DWithinSphere( px0, py0, pz0, pr0, nx0, ny0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return circle3DWithinEllipsoid( px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, w,d,h,  nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return circle3DWithinCone( px0,py0,pz0,pr0,nx0,ny0, x, y, z, r, h, nx, ny, strict );
	}

	return false;
}

bool JagGeo::doSphereWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return sphereWithinCube( px0, py0, pz0, pr0, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 

		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return sphereWithinBox( px0, py0, pz0, pr0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return sphereWithinSphere( px0, py0, pz0, pr0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return sphereWithinEllipsoid( px0, py0, pz0, pr0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return sphereWithinCone( px0, py0, pz0, pr0,    x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

// 2D
bool JagGeo::doSquareWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	//d("s3033 doSquareWithin colType2=[%s] \n", colType2.c_str() );
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	pr0 = meterToLon( srid2, pr0, px0, py0 );
	double nx0 = safeget(sp1, JAG_SP_START+3);

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return rectangleWithinTriangle( px0, py0, pr0,pr0, nx0, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleWithinSquare( px0, py0, pr0,pr0, nx0, x0, y0, a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleWithinRectangle( px0, py0, pr0,pr0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		r = meterToLon( srid2, r, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleWithinCircle( px0, py0, pr0,pr0, nx0, x0, y0, r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleWithinEllipse( px0, py0, pr0,pr0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return rectangleWithinPolygon( px0, py0, pr0,pr0,nx0, mk2, sp2, strict );
	}
	return false;
}

bool JagGeo::doSquare3DWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);
	double ny0 = safeget(sp1, JAG_SP_START+5);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		// return square3DWithinCube( px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, r, nx, ny, strict );
		return rectangle3DWithinCube( px0, py0, pz0, pr0,pr0, nx0, ny0, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		// return square3DWithinBox( px0, py0, pz0, pr0,  nx0, ny0,x0, y0, z0, w,d,h, nx, ny, strict );
		return rectangle3DWithinBox( px0, py0, pz0, pr0,pr0, nx0, ny0,x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		// return square3DWithinSphere( px0, py0, pz0, pr0, nx0, ny0, x, y, z, r, strict );
		return rectangle3DWithinSphere( px0, py0, pz0, pr0,pr0, nx0, ny0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		// return square3DWithinEllipsoid( px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
		return rectangle3DWithinEllipsoid( px0, py0, pz0, pr0,pr0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		// return square3DWithinCone( px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
		return rectangle3DWithinCone( px0, py0, pz0, pr0,pr0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}


bool JagGeo::doCubeWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);
	double ny0 = safeget(sp1, JAG_SP_START+5);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return boxWithinCube( px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxWithinBox( px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return boxWithinSphere( px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxWithinEllipsoid( px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return boxWithinCone( px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, r, h, nx,ny, strict );
	}
	return false;
}

// 2D
bool JagGeo::doRectangleWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);

	a0 = meterToLon( srid2, a0, px0, py0 );
	b0 = meterToLat( srid2, b0, px0, py0 );

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return rectangleWithinTriangle( px0, py0, a0, b0, nx0, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleWithinSquare( px0, py0, a0, b0, nx0, x0, y0, a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleWithinRectangle( px0, py0, a0, b0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		r = meterToLon( srid2, r, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleWithinCircle( px0, py0, a0, b0, nx0, x0, y0, r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleWithinEllipse( px0, py0, a0, b0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return rectangleWithinPolygon( px0,py0,a0,b0,nx0, mk2, sp2, strict );
	}
	return false;
}

// 3D rectiangle
bool JagGeo::doRectangle3DWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return rectangle3DWithinCube( px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return rectangle3DWithinBox( px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return rectangle3DWithinSphere( px0, py0, pz0, a0, b0, nx0, ny0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return rectangle3DWithinEllipsoid( px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return rectangle3DWithinCone( px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

bool JagGeo::doBoxWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+6);
	double ny0 = safeget(sp1, JAG_SP_START+7);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return boxWithinCube( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxWithinBox( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return boxWithinSphere( px0, py0, pz0, a0, b0, c0, nx0, ny0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxWithinEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return boxWithinCone( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}


// 3D
bool JagGeo::doCylinderWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 

	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return cylinderWithinCube( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return cylinderWithinBox( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return cylinderWithinSphere( px0, py0, pz0, pr0, c0,  nx0, ny0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return cylinderWithinEllipsoid( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return cylinderWithinCone( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

bool JagGeo::doConeWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return coneWithinCube( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return coneWithinBox( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return coneWithinSphere( px0, py0, pz0, pr0, c0,  nx0, ny0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return coneWithinEllipsoid( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return coneWithinCone( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

// 2D
bool JagGeo::doEllipseWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);

	a0 = meterToLon( srid2, a0, px0, py0 );
	b0 = meterToLat( srid2, b0, px0, py0 );

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return ellipseWithinTriangle( px0, py0, a0, b0, nx0, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		a = meterToLon( srid2, a, x0, y0 );
		double b = meterToLat( srid2, a, x0, y0 );
		//return ellipseWithinSquare( px0, py0, a0, b0, nx0, x0, y0, a, nx, strict );
		return ellipseWithinRectangle( px0, py0, a0, b0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return ellipseWithinRectangle( px0, py0, a0, b0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		r = meterToLon( srid2, r, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return ellipseWithinCircle( px0, py0, a0, b0, nx0, x0, y0, r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		return ellipseWithinEllipse( px0, py0, a0, b0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return ellipseWithinPolygon( px0, py0, a0, b0, nx0, mk2, sp2, strict );
	}
	return false;
}

// 3D ellipsoid
bool JagGeo::doEllipsoidWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+6);
	double ny0 = safeget(sp1, JAG_SP_START+7);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return ellipsoidWithinCube( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return ellipsoidWithinBox( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return ellipsoidWithinSphere( px0, py0, pz0, a0, b0, c0, nx0, ny0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return ellipsoidWithinEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return ellipsoidWithinCone( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

// 2D triangle within
bool JagGeo::doTriangleWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double x30 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double y30 = jagatof( sp1[JAG_SP_START+5].c_str() );

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return triangleWithinTriangle( x10, y10, x20, y20, x30, y30, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		a = meterToLon( srid2, a, x0, y0 );
		double b = meterToLat( srid2, a, x0, y0 );
		// return triangleWithinSquare( x10, y10, x20, y20, x30, y30, x0, y0, a, nx, strict );
		return triangleWithinRectangle( x10, y10, x20, y20, x30, y30, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return triangleWithinRectangle( x10, y10, x20, y20, x30, y30, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		r = meterToLon( srid2, r, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return triangleWithinCircle( x10, y10, x20, y20, x30, y30, x0, y0, r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return triangleWithinEllipse( x10, y10, x20, y20, x30, y30, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return triangleWithinPolygon( x10, y10, x20, y20, x30, y30, mk2, sp2, strict );
	}
	return false;
}

// 3D  triangle
bool JagGeo::doTriangle3DWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double z10 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double z20 = jagatof( sp1[JAG_SP_START+5].c_str() );
	double x30 = jagatof( sp1[JAG_SP_START+6].c_str() );
	double y30 = jagatof( sp1[JAG_SP_START+7].c_str() );
	double z30 = jagatof( sp1[JAG_SP_START+8].c_str() );

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return triangle3DWithinCube( x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return triangle3DWithinBox( x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return triangle3DWithinSphere( x10,y10,z10,x20,y20,z20,x30,y30,z30, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return triangle3DWithinEllipsoid( x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return triangle3DWithinCone( x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

// 2D line
bool JagGeo::doLineWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+3].c_str() );

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return lineWithinTriangle( x10, y10, x20, y20, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return lineWithinSquare( x10, y10, x20, y20, x0, y0, a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return lineWithinLineString( x10, y10, x20, y20, mk2, sp2, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineWithinRectangle( x10, y10, x20, y20, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		r = meterToLon( srid2, r, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return lineWithinCircle( x10, y10, x20, y20, x0, y0, r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineWithinEllipse( x10, y10, x20, y20, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return lineWithinPolygon( x10, y10, x20, y20, mk2, sp2, strict );
	}
	return false;
}

bool JagGeo::doLine3DWithin( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double z10 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double z20 = jagatof( sp1[JAG_SP_START+5].c_str() );

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return line3DWithinCube( x10,y10,z10,x20,y20,z20, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return line3DWithinLineString3D( x10,y10,z10,x20,y20,z20, mk2, sp2, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return line3DWithinBox( x10,y10,z10,x20,y20,z20, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return line3DWithinSphere( x10,y10,z10,x20,y20,z20, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return line3DWithinEllipsoid( x10,y10,z10,x20,y20,z20, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return line3DWithinCone( x10,y10,z10,x20,y20,z20, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

// 2D linestring
bool JagGeo::doLineStringWithin( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
								 const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict )
{
	// like point within
	//d("s6683 doLineStringWithin colType2=[%s]\n", colType2.c_str() );
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return lineStringWithinTriangle( mk1, sp1, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return lineStringWithinLineString( mk1, sp1, mk2, sp2, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		//d("s0881 lineStringWithinSquare ...\n" );
		return lineStringWithinSquare( mk1, sp1, x0, y0, a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineStringWithinRectangle( mk1, sp1, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		r = meterToLon( srid2, r, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return lineStringWithinCircle( mk1, sp1, x0, y0, r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineStringWithinEllipse( mk1, sp1, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return lineStringWithinPolygon( mk1, sp1, mk2, sp2, strict );
	}
	return false;
}

bool JagGeo::doLineString3DWithin( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict )
{
	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return lineString3DWithinCube( mk1, sp1, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return lineString3DWithinLineString3D( mk1, sp1, mk2, sp2, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return lineString3DWithinBox( mk1, sp1, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return lineString3DWithinSphere( mk1, sp1, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return lineString3DWithinEllipsoid( mk1, sp1, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return lineString3DWithinCone( mk1, sp1, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

bool JagGeo::doPolygonWithin( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
								 const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict )
{
	/***
	//sp1.print();
	//sp1.printStr();

	//sp2.print();
	//sp2.printStr();
	***/

	/***
	//sp1.print();
	i=0 [OJAG=0=test.pol2.po2=PL]
	i=1 [0.0:0.0:500.0:600.0] // bbox
	i=2 [0.0:0.0]
	i=3 [20.0:0.0]
	i=4 [8.0:9.0]
	i=5 [0.0:0.0]
	i=6 [|]
	i=7 [1.0:2.0]
	i=8 [2.0:3.0]
	i=9 [1.0:2.0]
	***/

	// like point within
	//d("s6683 doPolygonWithin colType2=[%s]\n", colType2.c_str() );
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return polygonWithinTriangle( mk1, sp1, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		d("s6040 JAG_C_COL_TYPE_SQUARE sp2 print():\n");
		//sp2.print();
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return polygonWithinSquare( mk1, sp1, x0, y0, a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return polygonWithinRectangle( mk1, sp1, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		r = meterToLon( srid2, r, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return polygonWithinCircle( mk1, sp1, x0, y0, r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return polygonWithinEllipse( mk1, sp1, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return polygonWithinPolygon( mk1, sp1, mk2, sp2 );
	}
	return false;
}

double JagGeo::doMultiPolygonArea( const Jstr &mk1, int srid1, const JagStrSplit &sp1 )
{
	// d("s7739 doMultiPolygonArea sp1.print():\n" );
	//sp1.print();

	int start = JAG_SP_START;
    double dx, dy;
    const char *str;
    char *p;
	double ar, area = 0.0;
	int ring = 0;

	if ( 0 == srid1 ) {
    	JagVector<std::pair<double,double>> vec;
    	for ( int i=start; i < sp1.length(); ++i ) {
    		if ( sp1[i] == "!" || sp1[i] == "|" ) { 
    			ar = computePolygonArea( vec );
				if ( 0 == ring ) {
    				area += ar;
				} else {
    				area -= ar;
				}
				vec.clean();
				if ( sp1[i] == "!" ) { ring = 0;}
				else { ++ring; }
			} else {
        		str = sp1[i].c_str();
        		if ( strchrnum( str, ':') < 1 ) continue;
        		get2double(str, p, ':', dx, dy );
    			vec.append(std::make_pair(dx, dy));
    		}
		}
    
    	if ( vec.size() > 0 ) {
    		ar = computePolygonArea( vec );
			if ( 0 == ring ) {
    			area += ar;
			} else {
    			area -= ar;
			}
    	}
	} else if ( JAG_GEO_WGS84 == srid1 ) {
		const Geodesic& geod = Geodesic::WGS84();
		PolygonArea poly(geod);
		double perim;
		int numPoints = 0;
    	for ( int i=start; i < sp1.length(); ++i ) {
    		if ( sp1[i] == "!" || sp1[i] == "|" ) { 
				poly.Compute( false, true, perim, ar );
				if ( 0 == ring ) {
    				area += ar;
				} else {
    				area -= ar;  // hole
				}
    			poly.Clear(); 
				numPoints = 0;
				if ( sp1[i] == "!" ) { ring = 0;}
				else { ++ring; }
			} else {
        		str = sp1[i].c_str();
        		if ( strchrnum( str, ':') < 1 ) continue;
        		get2double(str, p, ':', dx, dy );
				poly.AddPoint( dx, dy );
				++numPoints;
    		}
		}
    
    	if ( numPoints > 0 ) {
			poly.Compute( false, true, perim, ar );
			if ( 0 == ring ) {
    			area += ar;
			} else {
    			area -= ar;
			}
    	}
	}

	return area;
}

double JagGeo::doMultiPolygonPerimeter( const Jstr &mk1, int srid1, const JagStrSplit &sp1 )
{
	//d("s7739 doMultiPolygonArea sp1.print():\n" );
	//sp1.print();

	int start = JAG_SP_START;
    double dx, dy;
    const char *str;
    char *p;
	double perim = 0.0;
   	JagVector<std::pair<double,double>> vec;
   	for ( int i=start; i < sp1.length(); ++i ) {
    		if ( sp1[i] == "!" || sp1[i] == "|" ) { 
    			perim += computePolygonPerimeter( vec, srid1 );
				vec.clean();
			} else {
        		str = sp1[i].c_str();
        		if ( strchrnum( str, ':') < 1 ) continue;
        		get2double(str, p, ':', dx, dy );
    			vec.append(std::make_pair(dx, dy));
    		}
	}
    
   	if ( vec.size() > 0 ) {
   		perim += computePolygonPerimeter( vec, srid1 );
   	}

	return perim;
}

double JagGeo::doMultiPolygon3DPerimeter( const Jstr &mk1, int srid1, const JagStrSplit &sp1 )
{
	//d("s7739 doMultiPolygon3DPerimeter sp1.print():\n" );
	//sp1.print();

	int start = JAG_SP_START;
    double dx, dy, dz;
    const char *str;
    char *p;
	double perim = 0.0;
   	JagVector<JagPoint3D> vec;
   	for ( int i=start; i < sp1.length(); ++i ) {
    		if ( sp1[i] == "!" || sp1[i] == "|" ) { 
    			perim += computePolygon3DPerimeter( vec, srid1 );
				vec.clean();
			} else {
        		str = sp1[i].c_str();
        		if ( strchrnum( str, ':') < 2 ) continue;
        		get3double(str, p, ':', dx, dy, dz );
    			vec.append(JagPoint3D(dx, dy, dz));
    		}
	}
    
   	if ( vec.size() > 0 ) {
   		perim += computePolygon3DPerimeter( vec, srid1 );
   	}

	return perim;
}

bool JagGeo::doMultiPolygonWithin( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
								 const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict )
{
	// like point within
	d("s6683 domultiPolygonWithin colType2=[%s]\n", colType2.c_str() );
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return multiPolygonWithinTriangle( mk1, sp1, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		d("s6040 JAG_C_COL_TYPE_SQUARE sp2 print():\n");
		//sp2.print();
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		a = meterToLon( srid2, a, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return multiPolygonWithinSquare( mk1, sp1, x0, y0, a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return multiPolygonWithinRectangle( mk1, sp1, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		r = meterToLon( srid2, r, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+3);
		return multiPolygonWithinCircle( mk1, sp1, x0, y0, r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		a = meterToLon( srid2, a, x0, y0 );
		b = meterToLat( srid2, b, x0, y0 );
		double nx = safeget(sp2, JAG_SP_START+4);
		return multiPolygonWithinEllipse( mk1, sp1, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return multiPolygonWithinPolygon( mk1, sp1, mk2, sp2 );
	}
	return false;
}


bool JagGeo::doPolygon3DWithin( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict )
{
	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return polygon3DWithinCube( mk1, sp1, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return polygon3DWithinBox( mk1, sp1, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return polygon3DWithinSphere( mk1, sp1, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return polygon3DWithinEllipsoid( mk1, sp1, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return polygon3DWithinCone( mk1, sp1, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

bool JagGeo::doMultiPolygon3DWithin( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict )
{
	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return multiPolygon3DWithinCube( mk1, sp1, x0, y0, z0, r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return multiPolygon3DWithinBox( mk1, sp1, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return multiPolygon3DWithinSphere( mk1, sp1, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return multiPolygon3DWithinEllipsoid( mk1, sp1, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return multiPolygon3DWithinCone( mk1, sp1, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

////////////////////////////////////// 2D circle /////////////////////////
bool JagGeo::circleWithinTriangle( double px, double py, double pr, double x1, double y1, double x2, double y2, 
								double x3, double y3, bool strict, bool bound )
{
		bool rc;
		rc = pointInTriangle( px, py, x1, y1, x2, y2, x3, y3, strict, bound );
		if ( ! rc ) return false;

		pr = pr*pr;

		double dist = (y2-y1)*px - (x2-x1)*py + x2*y1-y2*x1; 
		dist = dist * dist; 
		double ps = (y2-y1)*(y2-y1) + (x2-x1)*(x2-x1);
		if ( strict ) {
			if ( dist <  ps * pr ) return false;
		} else {
			if ( jagLE(dist, ps * pr ) ) return false;
		}

		dist = (y3-y1)*px - (x3-x1)*py + x3*y1-y3*x1; 
		dist = dist * dist; 
		ps = (y3-y1)*(y3-y1) + (x3-x1)*(x3-x1);
		if ( strict ) {
			if ( dist <  ps * pr ) return false;
		} else {
			if ( jagLE(dist, ps * pr ) ) return false;
		}

		dist = (y3-y2)*px - (x3-x2)*py + x3*y2-y3*x2; 
		dist = dist * dist; 
		ps = (y3-y2)*(y3-y2) + (x3-x2)*(x3-x2);
		if ( strict ) {
			if ( dist <  ps * pr ) return false;
		} else {
			if ( jagLE(dist, ps * pr ) ) return false;
		}
		return true;
}


bool JagGeo::circleWithinEllipse( double px0, double py0, double pr, 
							  double x, double y, double w, double h, double nx, 
							  bool strict, bool bound )
{
	if (  bound2DDisjoint( px0, py0, pr,pr, x, y, w,h ) ) { return false; }

	double px, py;
	transform2DCoordGlobal2Local( x, y, px0, py0, nx, px, py );
	if ( bound ) {
		if ( px-pr < x-w || px+pr > x+w || py-pr < y-h || py+pr > y+h ) return false;
	}
	JagVector<JagPoint2D> vec;
	samplesOn2DCircle( px0, py0, pr, 2*NUM_SAMPLE, vec );
	for ( int i=0; i < vec.size(); ++i ) {
		if ( ! pointWithinEllipse( vec[i].x, vec[i].y, x, y, w, h, nx, strict ) ) {
			return false;
		}
	}
	return true;
}

bool JagGeo::circleWithinPolygon( double px0, double py0, double pr, 
							const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	double bbx, bby, rx, ry;
	getPolygonBound( mk2, sp2, bbx, bby, rx, ry );
	if ( bound2DDisjoint( px0, py0, pr,pr, bbx, bby, rx, ry ) ) { return false; }

	JagPolygon pgon;
	int rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) return false;

	JagVector<JagPoint2D> vec;
	samplesOn2DCircle( px0, py0, pr, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
		if ( ! pointWithinPolygon( vec[i].x, vec[i].y,  pgon ) ) {
			return false;
		}
	}
	return true;
}


////////////////////////////////////////// 2D point /////////////////////////////////////////
bool JagGeo::pointWithinPoint( double px, double py, double x1, double y1, bool strict )
{
	if ( strict ) return false;
	if ( jagEQ(px,x1) && jagEQ(py,y1) ) {
		return true;
	}
	return false;
}

bool JagGeo::pointWithinLine( double px, double py, double x1, double y1, double x2, double y2, bool strict )
{
	// check if falls in both ends
	if ( (jagIsZero(px-x1) && jagIsZero(py-y1)) || ( jagIsZero(px-x2) && jagIsZero(py-y2) ) ) {
		if ( strict ) return false;
		else return true;
	}

	// slope of (px,py) and (x1,xy) is same as (x1,y1) and (x2,y2)
	double xmin, ymin, endx, endy;
	if ( x1 < x2 ) {
		xmin = x1; ymin = y1; endx = x2; endy = y2;
	} else {
		xmin = x2; ymin = y2; endx = x1; endy = y1;
	}

	if ( jagEQ(xmin, endx) && jagEQ(xmin, px) ) {
		if ( ymin < py && py < endy ) {
			return true;
		} else {
			return false;
		}
	}

	if ( jagEQ(xmin, endx) && ! jagEQ(xmin, px) ) {
		return false;
	}
	if ( ! jagEQ(xmin, endx) && jagEQ(xmin, px) ) {
		return false;
	}

	if ( jagEQ( (endy-ymin)/(endx-xmin), (py-ymin)/(px-xmin) ) ) {
		return true;
	}

	return false;
}

bool JagGeo::pointWithinLineString( double x, double y, 
									const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	int start = JAG_SP_START;
    double dx1, dy1, dx2, dy2;
    const char *str;
    char *p;

    //sp2.print();

	dn("s6790 pointWithinLineString start=%d sp2.len=%d  x=%f y=%f strict=%d", start, sp2.length(), x, y, strict );
	for ( int i=start; i < sp2.length()-1; ++i ) {
		//d("s6658 sp1[%d]=[%s]\n", i, sp1[i].c_str() );
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx1, dy1 );
		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx2, dy2 );

        dn("s390834003 dx1=%f dy1=%f     dx2=%f dy2=%f", dx1, dy1, dx2, dy2 );
		if ( pointWithinLine( x,y, dx1,dy1,dx2,dy2, strict) ) { 
            return true; 
       }
	}
	return false;
}

bool JagGeo::pointWithinRectangle( double px0, double py0, double x0, double y0, 
									double w, double h, double nx, bool strict )
{
	if (  bound2DDisjoint( px0, py0, 0.0,0.0, x0, y0, w,h ) ) { return false; }
    double px, py;
	transform2DCoordGlobal2Local( x0, y0, px0, py0, nx, px, py );

	if ( strict ) {
		if ( px > -w && px < w && py >-h && py < h ) { return true; } 
	} else {
		if ( jagGE(px, -w) && jagLE(px, w) && jagGE(py, -h) && jagLE(py, h ) ) { return true; } 
	}
	return false;
}

bool JagGeo::pointWithinCircle( double px, double py, double x0, double y0, double r, bool strict )
{
	if ( px < x0-r || px > x0+r || py < y0-r || py > y0+r ) return false;

	if ( strict ) {
		if ( ( (px-x0)*(px-x0) + (py-y0)*(py-y0) ) < r*r  ) return true; 
	} else {
		if ( jagLE( (px-x0)*(px-x0)+(py-y0)*(py-y0), r*r ) ) return true;
	}
	return false;
}

bool JagGeo::pointWithinEllipse( double px0, double py0, double x0, double y0, 
									double w, double h, double nx, bool strict )
{
	d("s2330 pointWithinEllipse px0=%f py0=%f   x0=%f  y0=%f  w=%f  h=%f  nx=%f strict=%d\n", px0, py0, x0, y0, w,h, nx, strict ); 
	if ( jagIsZero(w) || jagIsZero(h) ) {
		d("s22202 w or h is zero\n");
		return false;
	}

	if (  bound2DDisjoint( px0, py0, 0.0,0.0, x0, y0, w,h ) ) { 
		d("s24202 bound2disjoin \n");
		return false; 
	}

    double px, py;
	transform2DCoordGlobal2Local( x0, y0, px0, py0, nx, px, py );

	if ( px < -w || px > w || py < -h || py > h ) {
		d("s220299 3 false\n");
		return false;
	}
	double f = px*px/(w*w) + py*py/(h*h);
	if ( strict ) {
		if ( f < 1.0 ) return true; 
	} else {
		if ( jagLE(f, 1.0) ) return true; 
	}

	d("s30339 false\n");
	return false;
}

bool JagGeo::pointWithinPolygon( double x, double y, 
								const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
    //const char *str;
    //char *p;
	JagPolygon pgon;
	int rc;
	rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) {
		//d("s8112 rc=%d false\n", rc );
		return false;
	}

   	if ( ! pointWithinPolygon( x, y, pgon.linestr[0] ) ) {
		//d("s8113 outer polygon false\n" );
		return false;
	}

	for ( int i=1; i < pgon.size(); ++i ) {
		// pgon.linestr[i] is JagLineString3D
		if ( pointWithinPolygon(x, y, pgon.linestr[i] ) ) {
			//d("s8114 i=%d witin hole false\n", i );
			return false;
		}
	}

	return true;
}

bool JagGeo::point3DWithinLineString3D( double x, double y, double z, 
									const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	int start = JAG_SP_START;
    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p;
	//d("s6790 start=%d len=%d  square: x0=%f y0=%f r=%f\n", start, sp1.length(), x0,y0,r );
	for ( int i=start; i < sp2.length()-1; ++i ) {
		//d("s6658 sp1[%d]=[%s]\n", i, sp1[i].c_str() );
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx1, dy1, dz1 );
		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx2, dy2, dz2 );
		if ( point3DWithinLine3D( x,y,z, dx1,dy1,dz1,dx2,dy2,dz2, strict) ) { return true; }
	}
	return false;
}

///////////////////////// 3D point ////////////////////////////////////////////////
bool JagGeo::point3DWithinBox( double px0, double py0, double pz0,
								 double x0, double y0, double z0, 
								 double w, double d, double h, double nx, double ny, bool strict )
{
	if (  bound3DDisjoint( px0, py0, pz0, 0.0,0.0,0.0, x0, y0, z0, w,d,h ) ) { return false; }
	double px, py, pz;
	transform3DCoordGlobal2Local( x0,y0,z0, px0,py0,pz0, nx, ny, px, py, pz );

	if ( strict ) {
		if ( px > -w && px < w && py >-d && py < d && pz>-h && pz<h ) { return true; } 
	} else {
		if ( jagGE(px, -w) && jagLE(px, w) 
			 && jagGE(py, -d) && jagLE(py, d )
			 && jagGE(pz, -h) && jagLE(pz, h ) ) { return true; } 
	}
	return false;
}

bool JagGeo::point3DWithinSphere( double px, double py, double pz, 
								  double x, double y, double z, double r, bool strict )
{
	if ( px < x-r || px > x+r || py < y-r || py > y+r || pz<z-r || pz>z+r ) return false;

	if ( strict ) {
		if ( ( (px-x)*(px-x) + (py-y)*(py-y) + (pz-z)*(pz-z) ) < r*r  ) return true; 
	} else {
		if ( jagLE( (px-x)*(px-x)+(py-y)*(py-y)+(pz-z)*(pz-z), r*r ) ) return true;
	}
	return false;
}

bool JagGeo::point3DWithinEllipsoid( double px0, double py0, double pz0,
								 double x0, double y0, double z0, 
								 double a, double b, double c, double nx, double ny, bool strict )
{
	if ( jagIsZero(a) || jagIsZero(b) || jagIsZero(c) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, 0.0,0.0,0.0, x0, y0, z0, a,b,c ) ) { return false; }

	double px, py, pz;
	transform3DCoordGlobal2Local( x0,y0,z0, px0,py0,pz0, nx, ny, px, py, pz );
	if ( ! point3DWithinNormalEllipsoid( px,py,pz, a,b,c, strict ) ) { return false; }
	return true;
}


/////////////////////////////////////// 2D circle ////////////////////////////////////////
bool JagGeo::circleWithinCircle( double px, double py, double pr, double x, double y, double r, bool strict )
{
	if ( px+pr < x-r || px-pr > x+r || py+pr < y-r || py-pr > y+r ) return false;

	double dist2  = (px-x)*(px-x) + (py-y)*(py-y);
	if ( strict ) {
		// strictly inside
		if ( dist2 < fabs(pr-r)*fabs(pr-r) ) return true;
	} else {
		if ( jagLE( dist2,  fabs(pr-r)*fabs(pr-r) ) ) return true;
	}
	return false;
}

bool JagGeo::circleWithinSquare( double px0, double py0, double pr, double x0, double y0, double r, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	if (  bound2DDisjoint( px0, py0, pr,pr, x0, y0, r,r ) ) { return false; }
    double px, py;
	transform2DCoordGlobal2Local( x0,y0, px0,py0, nx, px, py );

	if ( strict ) {
		if ( px+pr < r && py+pr < r && px-pr > -r && py-pr > -r ) return true;
	} else {
		if ( jagLE(px+pr, r) && jagLE(py+pr, r) && jagGE(px-pr, -r) && jagGE(py-pr, -r) ) return true;
	}
	return false;
}

bool JagGeo::circleWithinRectangle( double px0, double py0, double pr, double x0, double y0, 
									double w, double h, double nx,  bool strict )
{
	if ( ! validDirection(nx) ) return false;
    double px, py;
	transform2DCoordGlobal2Local( x0,y0, px0,py0, nx, px, py );

	if ( strict ) {
		if ( px+pr < w && py+pr < h && px-pr > -w && py-pr > -h ) return true;
	} else {
		if ( jagLE(px+pr, w) && jagLE(py+pr, h) && jagGE(px-pr, -w) && jagGE(py-pr, -h) ) return true;
	}
	return false;
}


//////////////////////////////////////// 3D circle ///////////////////////////////////////
bool JagGeo::circle3DWithinCube( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
							     double x, double y, double z,  double r, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,pr0, x, y, z, r,r,r ) ) { return false; }

	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( px0, py0, pz0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	double sq_x, sq_y, sq_z, px, py, pz;
	for ( int i=0; i < vec.size(); ++i ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[i].x, vec[i].y, vec[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, px, py, pz );
		// x y z is center of second cube
		// px py pz are within second cube sys
		if ( ! locIn3DCenterBox( px,py,pz, r,r,r, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::circle3DWithinBox( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
							    double x, double y, double z,  double w, double d, double h, 
								double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,pr0, x, y, z, w,d,h ) ) { return false; }

	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( px0, py0, pz0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	double sq_x, sq_y, sq_z, px, py, pz;
	for ( int i=0; i < vec.size(); ++i ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[i].x, vec[i].y, vec[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordLocal2Global( px0, py0, pz0, vec[i].x, vec[i].y, vec[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, px, py, pz );
		if ( ! locIn3DCenterBox( px,py,pz, w,d,h, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::circle3DWithinSphere( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
								   double x, double y, double z, double r, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,pr0, x, y, z, r,r,r ) ) { return false; }
	/***
	if ( px0-pr0 < x-r || px0+pr0 > x+r || py0-pr0 < y-r || py0+pr0 > y+r || pz0-pr0<z-r || pz0+pr0>z+r ) return false;

	if ( strict ) {
		if ( ( (px0-x)*(px0-x) + (py0-y)*(py0-y) + (pz0-z)*(pz0-z) ) < (r-pr0)*(r-pr0)  ) return true; 
	} else {
		if ( jagLE( (px0-x)*(px0-x)+(py0-y)*(py0-y)+(pz0-z)*(pz0-z), (r-pr0)*(r-pr0) ) ) return true;
	}
	return false;
	***/
	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( px0, py0, pz0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	double sq_x, sq_y, sq_z, locx, locy, locz, d2;
	for ( int i=0; i < vec.size(); ++i ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[i].x, vec[i].y, vec[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		locx = sq_x - x; locy = sq_y - y; locz = sq_z - z;
		d2 = locx*locx + locy*locy + locz*locz;
		if ( strict ) {
			if ( jagGE(d2, r*r) ) return false;
		} else {
			if ( d2 > r*r ) return false;
		}
	}
	return true;
}


bool JagGeo::circle3DWithinEllipsoid( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
									  double x, double y, double z, double w, double d, double h,
									  double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,pr0, x, y, z, w,d,h ) ) { return false; }

	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( px0, py0, pz0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	double sq_x, sq_y, sq_z, locx, locy, locz ;
	for ( int i=0; i < vec.size(); ++i ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[i].x, vec[i].y, vec[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, locx, locy, locz );
		if ( ! point3DWithinNormalEllipsoid( locx, locy, locz, w,d,h, strict ) ) { return false; }
	}
	return true;
}


////////////////////////////////////// 3D sphere ////////////////////////////////////////
bool JagGeo::sphereWithinCube(  double px0, double py0, double pz0, double pr0,
						        double x0, double y0, double z0, double r, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound2DDisjoint( px0, py0, pr0,pr0, x0, y0, r,r ) ) { return false; }

    double px, py, pz, x, y, z;
    rotate3DCoordGlobal2Local( px0, py0, pz0, nx, ny, px, py, pz );
    rotate3DCoordGlobal2Local( x0, y0, z0, nx, ny, x, y, z );

	if ( strict ) {
		if ( px-pr0 > x-r && px+pr0 < x+r && py-pr0 >y-r && py+pr0 < y+r && pz-pr0 > z-r && pz+pr0 < z+r ) { return true; } 
	} else {
		if ( jagGE(px-pr0, x-r) && jagLE(px+pr0, x+r) 
		     && jagGE(py-pr0, y-r) && jagLE(py+pr0, y+r )
			 && jagGE(pz-pr0, z-r) && jagLE(pz+pr0, z+r )) { return true; } 
	}
	return false;
}


bool JagGeo::sphereWithinBox(  double px0, double py0, double pz0, double r,
						        double x0, double y0, double z0, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx) ) return false;

    double px, py, pz, x, y, z;
    rotate3DCoordGlobal2Local( px0, py0, pz0, nx, ny, px, py, pz );
    rotate3DCoordGlobal2Local( x0, y0, z0, nx, ny, x, y, z );

	if ( strict ) {
		if ( px-r > x-w && px+r < x+w && py-r >y-d && py+r < y+d && pz-r>z-h && pz+r<z+h ) { return true; } 
	} else {
		if ( jagGE(px-r, x-w) && jagLE(px+r, x+w) 
			 && jagGE(py-r, y-d) && jagLE(py+r, y+d )
			 && jagGE(pz-r, z-h) && jagLE(pz+r, z+h ) ) { return true; } 
	}
	return false;
}

bool JagGeo::sphereWithinSphere(  double px, double py, double pz, double pr,
                                double x, double y, double z, double r, bool strict )
{
	if ( px-pr < x-r || px+pr > x+r || py-pr < y-r || py+pr > y+r || pz-pr<z-r || pz+pr>z+r ) return false;

	if ( strict ) {
		if ( ( (px-x)*(px-x) + (py-y)*(py-y) + (pz-z)*(pz-z) ) < (r-pr)*(r-pr)  ) return true; 
	} else {
		if ( jagLE( (px-x)*(px-x)+(py-y)*(py-y)+(pz-z)*(pz-z), (r-pr)*(r-pr) ) ) return true;
	}
	return false;
}

bool JagGeo::sphereWithinEllipsoid(  double px0, double py0, double pz0, double pr,
						        	double x0, double y0, double z0, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;

	if ( jagIsZero(w) || jagIsZero(d) || jagIsZero(h) ) return false;

    double px, py, pz, x, y, z;
    rotate3DCoordGlobal2Local( px0, py0, pz0, nx, ny, px, py, pz );
    rotate3DCoordGlobal2Local( x0, y0, z0, nx, ny, x, y, z );

	if ( px-pr < x-w || px+pr > x+w || py-pr < y-d || py+pr > y+d || pz-pr<z-h || pz+pr>z+h ) return false;
	w -= pr; d -= pr; h -= pr;

	double f = (px-x)*(px-x)/(w*w) + (py-y)*(py-y)/(d*d) + (pz-z)*(pz-z)/(h*h);
	if ( strict ) {
		if ( f < 1.0 ) return true; 
	} else {
		if ( jagLE(f, 1.0) ) return true; 
	}
	return false;
}

//////////////////////////// 2D rectangle //////////////////////////////////////////////////
bool JagGeo::rectangleWithinTriangle( double px0, double py0, double a0, double b0, double nx0,
									double x1, double y1, double x2, double y2, double x3, double y3,
									bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	double X2, Y2, R2x, R2y;
	triangleRegion( x1, y1, x2, y2, x3, y3, X2, Y2, R2x, R2y );
	if (  bound2DDisjoint( px0, py0, a0,b0, X2, Y2, R2x, R2y ) ) { return false; }

	double sq_x1, sq_y1, sq_x2, sq_y2, sq_x3, sq_y3,  sq_x4, sq_y4;
	transform2DCoordLocal2Global( px0, py0, -a0, -b0, nx0, sq_x1, sq_y1 );
	transform2DCoordLocal2Global( px0, py0, -a0, b0, nx0, sq_x2, sq_y2 );
	transform2DCoordLocal2Global( px0, py0, a0, b0, nx0, sq_x3, sq_y3 );
	transform2DCoordLocal2Global( px0, py0, a0, -b0, nx0, sq_x4, sq_y4 );

   	JagPoint2D p1( x1, y1 );
   	JagPoint2D p2( x2, y2 );
   	JagPoint2D p3( x3, y3 );

   	JagPoint2D psq( sq_x1, sq_y1 );
	bool rc = pointWithinTriangle( psq, p1, p2, p3, strict, true );
	if ( ! rc ) return false;

	psq.x = sq_x2; psq.y = sq_y2;
	rc = pointWithinTriangle( psq, p1, p2, p3, strict, true );
	if ( ! rc ) return false;

	psq.x = sq_x3; psq.y = sq_y3;
	rc = pointWithinTriangle( psq, p1, p2, p3, strict, true );
	if ( ! rc ) return false;

	psq.x = sq_x4; psq.y = sq_y4;
	rc = pointWithinTriangle( psq, p1, p2, p3, strict, true );
	if ( ! rc ) return false;

	return true;
}

bool JagGeo::rectangleWithinSquare( double px0, double py0, double a0, double b0, double nx0,
							 	double x0, double y0, double r, double nx, bool strict )

{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if (  bound2DDisjoint( px0, py0, a0,b0, x0, y0, r,r ) ) { return false; }

	double sq_x[4], sq_y[4];
	transform2DCoordLocal2Global( px0, py0, -a0, -b0, nx0, sq_x[0], sq_y[0] );
	transform2DCoordLocal2Global( px0, py0, -a0, b0, nx0, sq_x[1], sq_y[1] );
	transform2DCoordLocal2Global( px0, py0, a0, b0, nx0, sq_x[2], sq_y[2] );
	transform2DCoordLocal2Global( px0, py0, a0, -b0, nx0, sq_x[3], sq_y[3] );
	double loc_x, loc_y;
	for ( int i=0; i < 4; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, sq_x[i], sq_y[i], nx, loc_x, loc_y );
		if ( ! locIn2DCenterBox( loc_x, loc_y, r, r, strict ) ) { return false; }
	}

	return true;
}

bool JagGeo::rectangleWithinRectangle( double px0, double py0, double a0, double b0, double nx0,
                                	double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if (  bound2DDisjoint( px0, py0, a0,b0, x0, y0, a,b ) ) { return false; }
	double sq_x[4], sq_y[4];
	transform2DCoordLocal2Global( px0, py0, -a0, -b0, nx0, sq_x[0], sq_y[0] );
	transform2DCoordLocal2Global( px0, py0, -a0, b0, nx0, sq_x[1], sq_y[1] );
	transform2DCoordLocal2Global( px0, py0, a0, b0, nx0, sq_x[2], sq_y[2] );
	transform2DCoordLocal2Global( px0, py0, a0, -b0, nx0, sq_x[3], sq_y[3] );

	double loc_x, loc_y;
	for ( int i=0; i < 4; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, sq_x[i], sq_y[i], nx, loc_x, loc_y );
		if ( ! locIn2DCenterBox( loc_x, loc_y, a, b, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::rectangleWithinCircle( double px0, double py0, double a0, double b0, double nx0,
                                	double x0, double y0, double r, double nx, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if (  bound2DDisjoint( px0, py0, a0,b0, x0, y0, r,r ) ) { return false; }
	double sq_x[4], sq_y[4];
	transform2DCoordLocal2Global( px0, py0, -a0, -b0, nx0, sq_x[0], sq_y[0] );
	transform2DCoordLocal2Global( px0, py0, -a0, b0, nx0, sq_x[1], sq_y[1] );
	transform2DCoordLocal2Global( px0, py0, a0, b0, nx0, sq_x[2], sq_y[2] );
	transform2DCoordLocal2Global( px0, py0, a0, -b0, nx0, sq_x[3], sq_y[3] );
	double r2, loc_x, loc_y;
	r2 = r*r;
	for ( int i=0; i < 4; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, sq_x[i], sq_y[i], nx, loc_x, loc_y );
		if ( strict ) {
			if ( jagGE( jagsq2(loc_x) + jagsq2(loc_y), r2 ) ) return false;
		} else {
			if ( jagsq2(loc_x) + jagsq2(loc_y) > r2 ) return false;
		}
	}
	return true;
}


bool JagGeo::rectangleWithinEllipse( double px0, double py0, double a0, double b0, double nx0,
                                	double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if (  bound2DDisjoint( px0, py0, a0,b0, x0, y0, a,b ) ) { return false; }
	if ( jagIsZero(a) || jagIsZero(b) ) return false;
	double sq_x[4], sq_y[4];
	transform2DCoordLocal2Global( px0, py0, -a0, -b0, nx0, sq_x[0], sq_y[0] );
	transform2DCoordLocal2Global( px0, py0, -a0, b0, nx0, sq_x[1], sq_y[1] );
	transform2DCoordLocal2Global( px0, py0, a0, b0, nx0, sq_x[2], sq_y[2] );
	transform2DCoordLocal2Global( px0, py0, a0, -b0, nx0, sq_x[3], sq_y[3] );
	double f, loc_x, loc_y;
	for ( int i=0; i < 4; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, sq_x[i], sq_y[i], nx, loc_x, loc_y );
		f = jagsq2(loc_x)/(a*a) + jagsq2(loc_y)/(b*b);
		if ( strict ) {
			if ( jagGE(f, 1.0) ) return false;
		} else {
			if ( f > 1.0 ) return false; 
		}
	}
	return true;
}

bool JagGeo::rectangleWithinPolygon( double px0, double py0, double a0, double b0, double nx0,
				const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( jagIsZero(a0) || jagIsZero(b0) ) return false;
	double bbx, bby, rx, ry;
	getPolygonBound( mk2, sp2, bbx, bby, rx, ry );
	if (  bound2DDisjoint( px0, py0, a0,b0, bbx, bby, rx, ry ) ) { return false; }
	double sq_x[4], sq_y[4];
	transform2DCoordLocal2Global( px0, py0, -a0, -b0, nx0, sq_x[0], sq_y[0] );
	transform2DCoordLocal2Global( px0, py0, -a0, b0, nx0, sq_x[1], sq_y[1] );
	transform2DCoordLocal2Global( px0, py0, a0, b0, nx0, sq_x[2], sq_y[2] );
	transform2DCoordLocal2Global( px0, py0, a0, -b0, nx0, sq_x[3], sq_y[3] );
	JagPolygon pgon;
	int rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) return false;
	for ( int i=0; i < 4; ++i ) {
		if ( ! pointWithinPolygon( sq_x[i], sq_y[i], pgon ) ) {
			return false;
		}
	}
	return true;
}


//////////////////////////// 2D line  //////////////////////////////////////////////////
bool JagGeo::lineWithinTriangle( double x10, double y10, double x20, double y20, 
			                         double x1, double y1, double x2, double y2, double x3, double y3, bool strict )

{
   	JagPoint2D p10( x10, y10 );
   	JagPoint2D p20( x20, y20 );

   	JagPoint2D p1( x1, y1 );
   	JagPoint2D p2( x2, y2 );
   	JagPoint2D p3( x3, y3 );

	bool rc = pointWithinTriangle( p10, p1, p2, p3, strict, true );
	if ( ! rc ) return false;

	rc = pointWithinTriangle( p20, p1, p2, p3, strict, true );
	if ( ! rc ) return false;

	return true;
}

bool JagGeo::lineWithinLineString( double x10, double y10, double x20, double y20,
 								   const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	// 2 points are some two neighbor points in sp2
	int start = JAG_SP_START;
    double dx1, dy1, dx2, dy2;
    const char *str;
    char *p;
	//d("s6390 start=%d len=%d  square: x0=%f y0=%f r=%f\n", start, sp1.length(), x0,y0,r );
	for ( int i=start; i < sp2.length()-1; ++i ) {
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx1, dy1 );
		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx2, dy2 );

		if ( jagEQ(x10, dx1) && jagEQ(y10, dy1) && jagEQ(x20, dx2) && jagEQ(y20, dy2) ) {
			return true;
		}
		if ( jagEQ(x20, dx1) && jagEQ(y20, dy1) && jagEQ(x10, dx2) && jagEQ(y10, dy2) ) {
			return true;
		}
	}
	return false;
}


bool JagGeo::lineWithinSquare( double x10, double y10, double x20, double y20, 
                           double x0, double y0, double r, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	double sq_x[2], sq_y[2];
	sq_x[0] = x10; sq_y[0] = y10;
	sq_x[1] = x20; sq_y[1] = y20;
	double loc_x, loc_y;
	for ( int i=0; i < 2; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, sq_x[i], sq_y[i], nx, loc_x, loc_y );
		if ( ! locIn2DCenterBox( loc_x, loc_y, r,r, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::lineWithinRectangle( double x10, double y10, double x20, double y20, 
                                         double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	double sq_x[2], sq_y[2];
	sq_x[0] = x10; sq_y[0] = y10;
	sq_x[1] = x20; sq_y[1] = y20;
	double loc_x, loc_y;
	for ( int i=0; i < 2; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, sq_x[i], sq_y[i], nx, loc_x, loc_y );
		if ( ! locIn2DCenterBox( loc_x, loc_y, a,b, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::lineWithinCircle( double x10, double y10, double x20, double y20, 
								   double x0, double y0, double r, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;
	double sq_x[2], sq_y[2];
	sq_x[0] = x10; sq_y[0] = y10;
	sq_x[1] = x20; sq_y[1] = y20;
	double loc_x, loc_y;
	for ( int i=0; i < 2; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, sq_x[i], sq_y[i], nx, loc_x, loc_y );
		if ( ! pointWithinCircle( loc_x, loc_y, 0.0, 0.0, r, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::lineWithinEllipse( double x10, double y10, double x20, double y20, 
									double x0, double y0, double a, double b, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;
	if ( jagIsZero(a) || jagIsZero(b) ) return false;

	double tri_x[2], tri_y[2];
	tri_x[0] = x10; tri_y[0] = y10;
	tri_x[1] = x20; tri_y[1] = y20;
	double loc_x, loc_y;
	for ( int i=0; i < 2; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, tri_x[i], tri_y[i], nx, loc_x, loc_y );
		if ( ! pointWithinNormalEllipse( loc_x, loc_y, a, b, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::lineWithinPolygon( double x10, double y10, double x20, double y20,
								const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	double bbx, bby, rx, ry;
	getPolygonBound( mk2, sp2, bbx, bby, rx, ry );
	if ( bound2DLineBoxDisjoint( x10, y10, x20, y20, bbx, bby, rx, ry ) ) {
		return false;
	}

    //const char *str;
    //char *p;
	//JagLineString3D linestr;
	JagPolygon pgon;
	int rc;
	/***
	if ( mk2 == JAG_OJAG ) {
		rc = JagParser::addPolygonData( pgon, sp2, false );
	} else {
		// form linesrting3d  from pdata
		p = secondTokenStart( sp2.c_str() );
		rc = JagParser::addPolygonData( pgon, p, false, false );
	}
	***/
	rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) return false;

   	if ( ! pointWithinPolygon( x10, y10, pgon.linestr[0] ) ) { return false; }
   	if ( ! pointWithinPolygon( x20, y20, pgon.linestr[0] ) ) { return false; }
	for ( int i=1; i < pgon.size(); ++i ) {
		if ( pointWithinPolygon(x10, y10, pgon.linestr[i] ) ) {
			return false;
		}
		if ( pointWithinPolygon(x20, y20, pgon.linestr[i] ) ) {
			return false;
		}
	}

	return true;
}


//////////////////////////// 2D linestring  //////////////////////////////////////////////////
bool JagGeo::lineStringWithinTriangle(  const Jstr &mk1, const JagStrSplit &sp1,
			                         double x1, double y1, double x2, double y2, double x3, double y3, bool strict )

{
	double trix, triy, rx, ry;
	triangleRegion(x1,y1, x2,y2, x3,y3, trix, triy, rx, ry );
	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  trix, triy, rx, ry ) ) {
			return false;
		}
	}

    JagPoint2D p1( x1, y1 );
    JagPoint2D p2( x2, y2 );
    JagPoint2D p3( x3, y3 );
	bool rc;
	double dx, dy;
	const char *str;
	char *p;
	for ( int i=start; i < sp1.length(); ++i ) {
    	// line: x10, y10 --> x20, y20 
		str = sp1[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx, dy );
       	JagPoint2D pt( dx, dy );
    	rc = pointWithinTriangle( pt, p1, p2, p3, strict, true );
    	if ( ! rc ) return false;
	}

	return true;
}

bool JagGeo::lineStringWithinLineString(  const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	int start1 = JAG_SP_START;
	double bbx1, bby1, brx1, bry1;
	if ( mk1 == JAG_OJAG ) {
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx1, bby1, brx1, bry1 );
	}

	int start2 = JAG_SP_START;
	double bbx2, bby2, brx2, bry2;
	if ( mk2 == JAG_OJAG ) {
		boundingBoxRegion(sp2[JAG_SP_START+0], bbx2, bby2, brx2, bry2 );
	}

	if (  mk1 == JAG_OJAG && mk2 == JAG_OJAG ) {
		if ( bound2DDisjoint( bbx1, bby1, brx1, bry1,  bbx2, bby2, brx2, bry2 ) ) {
			return false;
		}
	}

	// assume sp1 has fewer lines than sp2
	if ( strict ) {
		if ( sp1.length() - start1 >= sp2.length() - start2 ) return false;
	} else {
		if ( sp1.length() - start1 > sp2.length() - start2 ) return false;
	}

	int rc = KMPPointsWithin( sp1, start1, sp2, start2 );
	if ( rc < 0 ) return false;
	return true;
}

bool JagGeo::sequenceSame(  const Jstr &colType, const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2 )
{
	int start1 = JAG_SP_START;
	if ( sp1.length() != sp2.length() ) {
		return false;
	}

	int dim = getDimension( colType ); 
	double x1, y1, z1;
	double x2, y2, z2;
	const char *str; char *p;
	int n;
	for ( int i=start1; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "|" ) continue;
		str = sp1[i].c_str();
		n = strchrnum(str, ':');
		if ( 3 == dim ) { 
			if ( n < 2 ) continue;
			get3double(str, p, ':', x1, y1, z1 );

			str = sp2[i].c_str();
			n = strchrnum(str, ':');
			if ( n < 2 ) continue;
			get3double(str, p, ':', x2, y2, z2 );
			
			if ( !  ( jagEQ( x1, x2 ) && jagEQ( y1, y2 ) && jagEQ( z1, z2 ) ) ) return false;
		} else {
			if ( n < 1 ) continue;
			get2double(str, p, ':', x1, y1 );

			str = sp2[i].c_str();
			n = strchrnum(str, ':');
			if ( n < 1 ) continue;
			get2double(str, p, ':', x2, y2 );
			
			if ( !  ( jagEQ( x1, x2 ) && jagEQ( y1, y2 ) ) ) return false;
		}
	}

	return true;
}

bool JagGeo::lineStringWithinSquare( const Jstr &mk1, const JagStrSplit &sp1,
                           			 double x0, double y0, double r, double nx, bool strict )
{
	d("s6724 lineStringWithinSquare nx=%f ...\n", nx );
	if ( ! validDirection(nx) ) return false;
	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, r, r ) ) {
			d("s6770 false\n" );
			return false;
		}
	}

	double loc_x, loc_y;
    double dx, dy;
    const char *str;
    char *p;
	d("s6590 start=%d len=%d  square: x0=%f y0=%f r=%f\n", start, sp1.length(), x0,y0,r );
	for ( int i=start; i < sp1.length(); ++i ) {
		//d("s6658 sp1[%d]=[%s]\n", i, sp1[i].c_str() );
		str = sp1[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx, dy );
    	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
		d("s6783 dx=%f dy=%f loc_x=%f loc_y=%f r=%f\n", dx, dy, loc_x, loc_y, r );
		if ( ! locIn2DCenterBox( loc_x, loc_y, r,r, strict ) ) { 
			d("s8630 locIn2DCenterBox false, return false\n" );
			return false; 
		}
	}
	return true;
}

bool JagGeo::lineStringWithinRectangle( const Jstr &mk1, const JagStrSplit &sp1,
                                        double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, a, b ) ) {
			return false;
		}
	}
	
	double loc_x, loc_y;
    double dx, dy;
    const char *str;
    char *p;
	for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );
    	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
		if ( ! locIn2DCenterBox( loc_x, loc_y, a,b, strict ) ) { return false; }
	}
	return true;

}

bool JagGeo::lineStringWithinCircle( const Jstr &mk1, const JagStrSplit &sp1,
								     double x0, double y0, double r, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, r, r ) ) {
			return false;
		}
	}
	
	double loc_x, loc_y;
	double dx, dy;
	const char *str;
	char *p;
	for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );
    	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
		if ( ! pointWithinCircle( loc_x, loc_y, 0.0, 0.0, r, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::lineStringWithinEllipse( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double a, double b, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;
	if ( jagIsZero(a) || jagIsZero(b) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, a, b ) ) {
			return false;
		}
	}
	
	double loc_x, loc_y;
    double dx, dy;
    const char *str;
    char *p;
	for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );
    	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
		if ( ! pointWithinNormalEllipse( loc_x, loc_y, a, b, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::lineStringWithinPolygon( const Jstr &mk1, const JagStrSplit &sp1,
									  const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx1, bby1, brx1, bry1;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx1, bby1, brx1, bry1 );

		double bbx2, bby2, rx2, ry2;
		getPolygonBound( mk2, sp2, bbx2, bby2, rx2, ry2 );

		if ( bound2DDisjoint( bbx1, bby1, brx1, bry1,  bbx2, bby2, rx2, ry2 ) ) {
			return false;
		}
	}

	JagPolygon pgon;
	int rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) return false;

    double dx, dy;
    const char *str;
    char *p;
	for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );
		if ( ! pointWithinPolygon( dx, dy, pgon ) ) {
			return false;
		}
	}
	return true;
}


//////////////////////////// 2D polygon  //////////////////////////////////////////////////
bool JagGeo::polygonWithinTriangle(  const Jstr &mk1, const JagStrSplit &sp1,
			                         double x1, double y1, double x2, double y2, double x3, double y3, bool strict )

{
	double trix, triy, rx, ry;
	triangleRegion(x1,y1, x2,y2, x3,y3, trix, triy, rx, ry );
	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  trix, triy, rx, ry ) ) {
			return false;
		}
	}

    JagPoint2D p1( x1, y1 );
    JagPoint2D p2( x2, y2 );
    JagPoint2D p3( x3, y3 );
	bool rc;
	double dx, dy;
	const char *str;
	char *p;
	for ( int i=start; i < sp1.length(); ++i ) {
    	// line: x10, y10 --> x20, y20 
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
		str = sp1[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx, dy );
       	JagPoint2D pt( dx, dy );
    	rc = pointWithinTriangle( pt, p1, p2, p3, strict, true );
    	if ( ! rc ) return false;
	}

	return true;
}

bool JagGeo::polygonWithinSquare( const Jstr &mk1, const JagStrSplit &sp1,
                           			 double x0, double y0, double r, double nx, bool strict )
{
	d("s6724 polygonWithinSquare nx=%f ...\n", nx );
	if ( ! validDirection(nx) ) return false;
	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, r, r ) ) {
			d("s6770 false\n" );
			return false;
		}
	}
	//d("s8120 polygonWithinSquare sp1:\n" );
	//sp1.print();

	double loc_x, loc_y;
    double dx, dy;
    const char *str;
    char *p;
	d("s6790 start=%d len=%d  square: x0=%f y0=%f r=%f\n", start, sp1.length(), x0,y0,r );
	for ( int i=start; i < sp1.length(); ++i ) {
		//d("s6658 sp1[%d]=[%s]\n", i, sp1[i].c_str() );
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
		str = sp1[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx, dy );
    	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
		d("s6783 dx=%f dy=%f loc_x=%f loc_y=%f r=%f\n", dx, dy, loc_x, loc_y, r );
		if ( ! locIn2DCenterBox( loc_x, loc_y, r,r, strict ) ) { 
			d("s8630 locIn2DCenterBox false, return false\n" );
			return false; 
		}
	}
	return true;
}

bool JagGeo::polygonWithinRectangle( const Jstr &mk1, const JagStrSplit &sp1,
                                        double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, a, b ) ) {
			return false;
		}
	}
	
	double loc_x, loc_y;
    double dx, dy;
    const char *str;
    char *p;
	for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );
    	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
		if ( ! locIn2DCenterBox( loc_x, loc_y, a,b, strict ) ) { return false; }
	}
	return true;

}

bool JagGeo::polygonWithinCircle( const Jstr &mk1, const JagStrSplit &sp1,
								     double x0, double y0, double r, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, r, r ) ) {
			return false;
		}
	}
	
	double loc_x, loc_y;
	double dx, dy;
	const char *str;
	char *p;
	for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );
    	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
		if ( ! pointWithinCircle( loc_x, loc_y, 0.0, 0.0, r, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::polygonWithinEllipse( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double a, double b, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;
	if ( jagIsZero(a) || jagIsZero(b) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, a, b ) ) {
			return false;
		}
	}
	
	double loc_x, loc_y;
    double dx, dy;
    const char *str;
    char *p;
	for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );
    	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
		if ( ! pointWithinNormalEllipse( loc_x, loc_y, a, b, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::polygonWithinPolygon( const Jstr &mk1, const JagStrSplit &sp1, 
								   const Jstr &mk2, const JagStrSplit &sp2 )
{
	double bbx1, bby1, brx1, bry1;
	double bbx2, bby2, brx2, bry2;
	double xmin, ymin, xmax, ymax;
   	const char *str;
   	double dx, dy;
	char *p;

	xmin = ymin = JAG_LONG_MAX; 
    xmax = ymax = JAG_LONG_MIN;

	int rc;
	getPolygonBound( mk1, sp1, bbx1, bby1, brx1, bry1 );
	if ( mk2 == JAG_OJAG ) {
		boundingBoxRegion(sp2[JAG_SP_START+0], bbx2, bby2, brx2, bry2 );
	} else {
		//pdata = secondTokenStart( sp2.c_str() );
		//if ( ! pdata ) return false;
		getPolygonBound( mk2, sp2, bbx2, bby2, brx2, bry2 );
	}

	if ( bound2DDisjoint( bbx1, bby1, brx1, bry1,  bbx2, bby2, brx2, bry2 ) ) {
		d("s7581 bound2DDisjoint two polygon not within\n" );
		return false;
	}


    	JagPolygon pgon2;  // each polygon can have multiple linestrings
    	rc = JagParser::addPolygonData( pgon2, sp2, true );
    	//d("s3388 addPolygonData pgon: print():\n" );
    	//pgon.print();
    
    	if ( rc <  0 ) { return false; }
    	if ( pgon2.size() < 1 ) { return false; }
    	for ( int i=JAG_SP_START; i < sp1.length(); ++i ) {
    		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
    		str = sp1[i].c_str();
    		if ( strchrnum( str, ':') < 1 ) continue;
    		get2double(str, p, ':', dx, dy );
    		if ( ! pointWithinPolygon( dx, dy, pgon2.linestr[0] ) ) {
    			return false;
    		}
    	}
    	return true;
}


//////////////////////////// 2D multipolygon  //////////////////////////////////////////////////
bool JagGeo::multiPolygonWithinTriangle(  const Jstr &mk1, const JagStrSplit &sp1,
			                         double x1, double y1, double x2, double y2, double x3, double y3, bool strict )

{
	double trix, triy, rx, ry;
	triangleRegion(x1,y1, x2,y2, x3,y3, trix, triy, rx, ry );
	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  trix, triy, rx, ry ) ) {
			return false;
		}
	}

    JagPoint2D p1( x1, y1 );
    JagPoint2D p2( x2, y2 );
    JagPoint2D p3( x3, y3 );
	bool rc;
	double dx, dy;
	const char *str;
	char *p;
	bool skip = false;
	for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" ) {  skip = true; }
		else if ( sp1[i] == "!" ) {  skip = false; }
		else {
			if ( skip ) continue;
    		str = sp1[i].c_str();
    		if ( strchrnum( str, ':') < 1 ) continue;
    		get2double(str, p, ':', dx, dy );
           	JagPoint2D pt( dx, dy );
        	rc = pointWithinTriangle( pt, p1, p2, p3, strict, true );
        	if ( ! rc ) return false;
    	}
    }
   	return true;
}
    
bool JagGeo::multiPolygonWithinSquare( const Jstr &mk1, const JagStrSplit &sp1,
                               			 double x0, double y0, double r, double nx, bool strict )
{
   	d("s6724 multiPolygonWithinSquare nx=%f ...\n", nx );
   	if ( ! validDirection(nx) ) return false;
	int start = JAG_SP_START;
   	if ( mk1 == JAG_OJAG ) {
   		double bbx, bby, brx, bry;
   		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, r, r ) ) {
			d("s6770 false\n" );
			return false;
		}
	}

	double loc_x, loc_y;
    double dx, dy;
    const char *str;
    char *p;
	bool skip = false;
	d("s6790 start=%d len=%d  square: x0=%f y0=%f r=%f\n", start, sp1.length(), x0,y0,r );
	for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" ) {  skip = true; }
		else if ( sp1[i] == "!" ) {  skip = false; }
		else {
			if ( skip ) continue;
    		str = sp1[i].c_str();
    		if ( strchrnum( str, ':') < 1  ) continue;
    		get2double(str, p, ':', dx, dy );
        	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
    		d("s6783 dx=%f dy=%f loc_x=%f loc_y=%f r=%f\n", dx, dy, loc_x, loc_y, r );
    		if ( ! locIn2DCenterBox( loc_x, loc_y, r,r, strict ) ) { 
    			d("s8630 locIn2DCenterBox false, return false\n" );
    			return false; 
    		}
		}
	}
	return true;
}

bool JagGeo::multiPolygonWithinRectangle( const Jstr &mk1, const JagStrSplit &sp1,
                                        double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, a, b ) ) {
			return false;
		}
	}
	
	double loc_x, loc_y;
    double dx, dy;
    const char *str;
    char *p;
	bool skip = false;
	for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" ) {  skip = true; }
		else if ( sp1[i] == "!" ) {  skip = false; }
		else {
			if ( skip ) continue;
        	str = sp1[i].c_str();
        	if ( strchrnum( str, ':') < 1 ) continue;
        	get2double(str, p, ':', dx, dy );
    		transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
			if ( ! locIn2DCenterBox( loc_x, loc_y, a,b, strict ) ) { return false; }
		}
	}
	return true;
}

bool JagGeo::multiPolygonWithinCircle( const Jstr &mk1, const JagStrSplit &sp1,
								     double x0, double y0, double r, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, r, r ) ) {
			return false;
		}
	}
	
	double loc_x, loc_y;
	double dx, dy;
	const char *str;
	char *p;
	bool skip = false;
	for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" ) {  skip = true; }
		else if ( sp1[i] == "!" ) {  skip = false; }
		else {
			if ( skip ) continue;
            str = sp1[i].c_str();
            if ( strchrnum( str, ':') < 1 ) continue;
            get2double(str, p, ':', dx, dy );
        	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
    		if ( ! pointWithinCircle( loc_x, loc_y, 0.0, 0.0, r, strict ) ) { return false; }
		}
	}
	return true;
}


bool JagGeo::multiPolygonWithinEllipse( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double a, double b, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;
	if ( jagIsZero(a) || jagIsZero(b) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, a, b ) ) {
			return false;
		}
	}
	
	double loc_x, loc_y;
    double dx, dy;
    const char *str;
    char *p;
	bool skip = false;
	for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" ) {  skip = true; }
		else if ( sp1[i] == "!" ) {  skip = false; }
		else {
			if ( skip ) continue;
            str = sp1[i].c_str();
            if ( strchrnum( str, ':') < 1 ) continue;
            get2double(str, p, ':', dx, dy );
        	transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, loc_x, loc_y );
    		if ( ! pointWithinNormalEllipse( loc_x, loc_y, a, b, strict ) ) { return false; }
		}
	}
	return true;
}

bool JagGeo::multiPolygonWithinPolygon( const Jstr &mk1, const JagStrSplit &sp1, 
								   const Jstr &mk2, const JagStrSplit &sp2 )
{
	double bbx1, bby1, brx1, bry1;
	double bbx2, bby2, brx2, bry2;
   	const char *str;
   	double dx, dy;
	char *p;

	int rc;
	getPolygonBound( mk1, sp1, bbx1, bby1, brx1, bry1 );
	getPolygonBound( mk2, sp2, bbx2, bby2, brx2, bry2 );

	if ( bound2DDisjoint( bbx1, bby1, brx1, bry1,  bbx2, bby2, brx2, bry2 ) ) {
		d("s7581 bound2DDisjoint two polygon not within\n" );
		return false;
	}

    	JagPolygon pgon2;  // each polygon can have multiple linestrings
    	rc = JagParser::addPolygonData( pgon2, sp2, true );
    	//d("s3388 addPolygonData pgon: print():\n" );
    	//pgon.print();
    
    	if ( rc <  0 ) { return false; }
    	if ( pgon2.size() < 1 ) { return false; }
    
    	// sp1 array:  "bbox  x:y x:y  ... | x:y  x:y ...| ..." sp1: start=1 skip '|' and '!'
    	// sp2 cstr:  ( ( x y, x y, ...), ( ... ), (...) )
    	// pgon has sp2 data parsed
    	// check first polygon only for now
		bool skip = false;
    	for ( int i=JAG_SP_START; i < sp1.length(); ++i ) {
			if ( sp1[i] == "|" ) {  skip = true; }
			else if ( sp1[i] == "!" ) {  skip = false; }
			else {
				if ( skip ) continue;
    			str = sp1[i].c_str();
    			if ( strchrnum( str, ':') < 1 ) continue;
    			get2double(str, p, ':', dx, dy );
    			if ( ! pointWithinPolygon( dx, dy, pgon2.linestr[0] ) ) { return false; }
			}
    	}
    	return true;
}


//////////////////////////// 2D linestring intersect  //////////////////////////////////////////////////
bool JagGeo::lineStringIntersectLineString( const Jstr &mk1, const JagStrSplit &sp1,
			                                const Jstr &mk2, const JagStrSplit &sp2,
											bool doRes, JagVector<JagPoint2D> &retVec )
{
	// sweepline algo
	int start1 = JAG_SP_START;
	int start2 = JAG_SP_START;

	double dx1, dy1, dx2, dy2, t;
	const char *str;
	char *p; int i;
	int totlen = sp1.length()-start1 + sp2.length() - start2;
	JagSortPoint2D *points = new JagSortPoint2D[2*totlen];
	int j = 0;
	int rc;
	bool found = false;
	for ( i=start1; i < sp1.length()-1; ++i ) {
		str = sp1[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx1, dy1 );

		str = sp1[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx2, dy2 );

		if ( jagEQ(dx1, dx2)) {
			dx2 += 0.000001f;
		}

		if ( dx1 > dx2 ) {
			// swap
			t = dx1; dx1 = dx2; dx2 = t; 
			t = dy1; dy1 = dy2; dy2 = t; 
		}

		points[j].x1 = dx1; points[j].y1 = dy1;
		points[j].x2 = dx2; points[j].y2 = dy2;
		points[j].end = JAG_LEFT;
		points[j].color = JAG_RED;
		++j;

		points[j].x1 = dx1; points[j].y1 = dy1;
		points[j].x2 = dx2; points[j].y2 = dy2;
		points[j].end = JAG_RIGHT;
		points[j].color = JAG_RED;
		++j;
	}

	for ( i=start2; i < sp2.length()-1; ++i ) {
		str = sp2[i].c_str();

		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx1, dy1 );

		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx2, dy2 );

		if ( jagEQ(dx1, dx2) )  {
			dx2 += 0.000001f;
		}

		if ( dx1 > dx2 ) {
			// swap
			t = dx1; dx1 = dx2; dx2 = t; 
			t = dy1; dy1 = dy2; dy2 = t; 
		}

		points[j].x1 = dx1; points[j].y1 = dy1;
		points[j].x2 = dx2; points[j].y2 = dy2;
		points[j].end = JAG_LEFT;
		points[j].color = JAG_BLUE;
		++j;

		points[j].x1 = dx1; points[j].y1 = dy1;
		points[j].x2 = dx2; points[j].y2 = dy2;
		points[j].end = JAG_RIGHT;
		points[j].color = JAG_BLUE;
		++j;

	}

	int len = j;
	rc = inlineQuickSort<JagSortPoint2D>( points, len );
	if ( rc ) {
		//d("s7732 sortIntersectLinePoints rc=%d retur true intersect\n", rc );
		// return true;
	}

	JagArray<JagLineSeg2DPair> *jarr = new JagArray<JagLineSeg2DPair>();
	const JagLineSeg2DPair *above;
	const JagLineSeg2DPair *below;
	JagLineSeg2DPair seg; seg.value = '1';
	for ( int i=0; i < len; ++i ) {
		seg.key.x1 =  points[i].x1; seg.key.y1 =  points[i].y1;
		seg.key.x2 =  points[i].x2; seg.key.y2 =  points[i].y2;
		seg.color = points[i].color;
		/**
		// d("s0088 seg print:\n" )); seg.println(;
		**/

		if ( JAG_LEFT == points[i].end ) {
			jarr->insert( seg );
			above = jarr->getSucc( seg );
			below = jarr->getPred( seg );

			#if 0
			d("s1290 JAG_LEFT: \n" );
			if ( above ) {
				d("s7781 above print: \n" );
				above->println();
			}
			if ( below ) {
				d("s7681 below print: \n" );
				below->println();
			}

			if ( above && *above == JagLineSeg2DPair::NULLVALUE ) {
				d("s6201 above is NULLVALUE abort\n" );
				//abort();
				above = NULL;
			}

			if ( below && *below == JagLineSeg2DPair::NULLVALUE ) {
				d("s2098 below is NULLVALUE abort\n" );
				//abort();
				below = NULL;
			}
			#endif

			if ( above && below ) {
				if ( above->color != below->color 
				     && lineIntersectLine( above->key.x1,above->key.y1,above->key.x2,above->key.y2, 
									    below->key.x1,below->key.y1,below->key.x2,below->key.y2) ) {
					//d("s7640 left above below intersect\n" );
					if ( doRes ) {
						found = true;
						appendLine2DLine2DIntersection(above->key.x1,above->key.y1, above->key.x2,above->key.y2,
										below->key.x1,below->key.y1,below->key.x2,below->key.y2, retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			} else if ( above ) {
				if ( above->color != seg.color 
					 && lineIntersectLine( above->key.x1,above->key.y1,above->key.x2,above->key.y2, 
									    seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2) ) {
					/***
					d("s7641 left above seg intersect\n" );
					// above->println(); seg.println();
					***/
					if ( doRes ) {
						found = true;
						appendLine2DLine2DIntersection(above->key.x1,above->key.y1, above->key.x2,above->key.y2,
										seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2, retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			} else if ( below ) {
				if ( below->color != seg.color 
					 && lineIntersectLine( below->key.x1,below->key.y1,below->key.x2,below->key.y2, 
									    seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2) ) {
					//d("s7641 left below seg intersect\n" );
					if ( doRes ) {
						appendLine2DLine2DIntersection( below->key.x1,below->key.y1,below->key.x2,below->key.y2,
										seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2, retVec );
					} else {
						found = true;
						delete [] points;
						delete jarr;
						return true;
					}
				}
			}
		} else {
			// right end
			above = jarr->getSucc( seg );
			below = jarr->getPred( seg );

			#if 0
			d("s1290 JAG_RIGHT: \n" );
			if ( above ) {
				d("s7781 above print: \n" );
				above->println();
			}
			if ( below ) {
				d("s7681 below print: \n" );
				below->println();
			}

			if ( above && *above == JagLineSeg2DPair::NULLVALUE ) {
				d("s7201 above is NULLVALUE abort\n" );
				//abort();
				above = NULL;
			}

			if ( below && *below == JagLineSeg2DPair::NULLVALUE ) {
				/***
				d("72098 below is NULLVALUE abort\n" );
				jarr->printKey();
				// seg.println();
				//abort();
				***/
				below = NULL;
			}
			#endif

			if ( above && below ) {
				if ( above->color != below->color 
					 && lineIntersectLine( above->key.x1,above->key.y1,above->key.x2,above->key.y2, 
									    below->key.x1,below->key.y1,below->key.x2,below->key.y2) ) {
					//d("s7740 rightend above below intersect\n" );
					if ( doRes ) {
						found = true;
						appendLine2DLine2DIntersection( above->key.x1,above->key.y1,above->key.x2,above->key.y2,
										below->key.x1,below->key.y1,below->key.x2,below->key.y2, retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			} else if ( above ) {
				if ( above->color != seg.color 
					 && lineIntersectLine( above->key.x1,above->key.y1,above->key.x2,above->key.y2, 
									    seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2) ) {
					//d("s7740 rightend above seg intersect\n" );
					if ( doRes ) {
						found = true;
						appendLine2DLine2DIntersection( above->key.x1,above->key.y1,above->key.x2,above->key.y2,
										seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2, retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			} else if ( below ) {
				if ( below->color != seg.color 
					 && lineIntersectLine( below->key.x1,below->key.y1,below->key.x2,below->key.y2, 
									    seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2) ) {
					//d("s7740 rightend below seg intersect\n" );
					if ( doRes ) {
						found = true;
						appendLine2DLine2DIntersection( below->key.x1,below->key.y1,below->key.x2,below->key.y2,
										seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2, retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			}
			jarr->remove( seg );
		}
	}

	delete [] points;
	delete jarr;

	d("s7740 intersect found=%d\n", found );
	return found;
}


bool JagGeo::lineStringIntersectTriangle( const Jstr &mk1, const JagStrSplit &sp1,
			                         double x1, double y1, double x2, double y2, double x3, double y3, bool strict )

{
	double trix, triy, rx, ry;
	triangleRegion(x1,y1, x2,y2, x3,y3, trix, triy, rx, ry );
	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  trix, triy, rx, ry ) ) {
			return false;
		}
	}

	double dx1, dy1, dx2, dy2;
	const char *str;
	char *p; int i;
	for ( i=start; i < sp1.length()-1; ++i ) {
    	// line: x10, y10 --> x20, y20 
		str = sp1[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx1, dy1 );
		if ( pointInTriangle( dx1, dy1, x1,y1, x2,y2, x3,y3, strict, false) ) return true;

		str = sp1[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx2, dy2 );

		if ( lineIntersectLine(dx1,dy1,dx2,dy2, x1,y1,x2,y2 ) ) return true;
		if ( lineIntersectLine(dx1,dy1,dx2,dy2, x2,y2,x3,y3 ) ) return true;
		if ( lineIntersectLine(dx1,dy1,dx2,dy2, x3,y3,x1,y1 ) ) return true;
	}

	str = sp1[i].c_str();
	if ( strchrnum( str, ':') >= 1 ) {
		get2double(str, p, ':', dx1, dy1 );
		if ( pointInTriangle( dx1, dy1, x1,y1, x2,y2, x3,y3, strict, false) ) return true;
	}

	return false;
}

bool JagGeo::lineStringIntersectRectangle( const Jstr &mk1, const JagStrSplit &sp1,
                                         double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, a, b ) ) {
			return false;
		}
	}

	JagLine2D line[4];
	edgesOfRectangle( a, b, line );
	//double gy1, gx2,gy2;
	for ( int i=0; i < 4; ++i ) {
		line[i].transform(x0,y0,nx);
	}
	
    double dx1, dy1, dx2, dy2;
    const char *str;
    char *p; int i;
	for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx1, dy1 );
		if ( pointWithinRectangle( dx1, dy1, x0,y0,a,b,nx, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx2, dy2 );

		for ( int j=0;j<4;++j) {
			if ( lineIntersectLine( dx1,dy1,dx2,dy2, line[j].x1,line[j].y1,line[j].x2,line[j].y2 ) ) return true;
		}
	}

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 1 ) {
    	get2double(str, p, ':', dx1, dy1 );
		if ( pointWithinRectangle( dx1, dy1, x0,y0,a,b,nx, strict ) ) return true;
	}

	return false;
}

bool JagGeo::lineStringIntersectEllipse( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	if ( jagIsZero(a) || jagIsZero(b) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, a, b ) ) {
			return false;
		}
	}

    double dx1, dy1, dx2, dy2;
    const char *str;
    char *p; int i;
	for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx1, dy1 );
		if ( pointWithinEllipse( dx1, dy1, x0,y0,a,b,nx, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx2, dy2 );
		if ( lineIntersectEllipse( dx1,dy1,dx2,dy2, x0, y0, a, b, nx, strict ) ) return true;
	}

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 1 ) {
    	get2double(str, p, ':', dx1, dy1 );
		if ( pointWithinEllipse( dx1, dy1, x0,y0,a,b,nx, strict ) ) return true;
	}

	return false;
}


//////////////////////////// 2D polygon intersect  //////////////////////////////////////////////////
bool JagGeo::polygonIntersectLineString( const Jstr &mk1, const JagStrSplit &sp1,
			                                const Jstr &mk2, const JagStrSplit &sp2 )
{
	// sweepline algo
	int start1 = JAG_SP_START;
	int start2 = JAG_SP_START;
	//d("s8019 polygonIntersectLineString sp1: sp2:\n" );
	//sp1.print();
	//sp2.print();

	double dx1, dy1, dx2, dy2, t;
	const char *str;
	char *p; int i;
	int totlen = sp1.length()-start1 + sp2.length() - start2;
	JagSortPoint2D *points = new JagSortPoint2D[2*totlen];
	int j = 0;
	int rc;
	for ( i=start1; i < sp1.length()-1; ++i ) {
		str = sp1[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;  // skip |
		get2double(str, p, ':', dx1, dy1 );

		str = sp1[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;  // skip |
		get2double(str, p, ':', dx2, dy2 );

		if ( jagEQ(dx1, dx2)) {
			dx2 += 0.000001f;
		}

		if ( dx1 > dx2 ) {
			// swap
			t = dx1; dx1 = dx2; dx2 = t; 
			t = dy1; dy1 = dy2; dy2 = t; 
		}

		points[j].x1 = dx1; points[j].y1 = dy1;
		points[j].x2 = dx2; points[j].y2 = dy2;
		points[j].end = JAG_LEFT;
		points[j].color = JAG_RED;
		++j;

		points[j].x1 = dx1; points[j].y1 = dy1;
		points[j].x2 = dx2; points[j].y2 = dy2;
		points[j].end = JAG_RIGHT;
		points[j].color = JAG_RED;
		++j;
	}

	for ( i=start2; i < sp2.length()-1; ++i ) {
		str = sp2[i].c_str();

		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx1, dy1 );

		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx2, dy2 );

		if ( jagEQ(dx1, dx2) )  {
			dx2 += 0.000001f;
		}

		if ( dx1 > dx2 ) {
			// swap
			t = dx1; dx1 = dx2; dx2 = t; 
			t = dy1; dy1 = dy2; dy2 = t; 
		}

		points[j].x1 = dx1; points[j].y1 = dy1;
		points[j].x2 = dx2; points[j].y2 = dy2;
		points[j].end = JAG_LEFT;
		points[j].color = JAG_BLUE;
		++j;

		points[j].x1 = dx1; points[j].y1 = dy1;
		points[j].x2 = dx2; points[j].y2 = dy2;
		points[j].end = JAG_RIGHT;
		points[j].color = JAG_BLUE;
		++j;

	}

	int len = j;
	rc = inlineQuickSort<JagSortPoint2D>( points, len );
	if ( rc ) {
		//d("s7732 sortIntersectLinePoints rc=%d retur true intersect\n", rc );
		// return true;
	}

	JagArray<JagLineSeg2DPair> *jarr = new JagArray<JagLineSeg2DPair>();
	const JagLineSeg2DPair *above;
	const JagLineSeg2DPair *below;
	JagLineSeg2DPair seg; seg.value = '1';
	for ( int i=0; i < len; ++i ) {
		seg.key.x1 =  points[i].x1; seg.key.y1 =  points[i].y1;
		seg.key.x2 =  points[i].x2; seg.key.y2 =  points[i].y2;
		seg.color = points[i].color;
		/**
		//d("s0088 seg print:\n" )); seg.println(;
		**/

		if ( JAG_LEFT == points[i].end ) {
			jarr->insert( seg );
			above = jarr->getSucc( seg );
			below = jarr->getPred( seg );

			#if 0
			d("s1290 JAG_LEFT: \n" );
			if ( above ) {
				d("s7781 above print: \n" );
				above->println();
			}
			if ( below ) {
				d("s7681 below print: \n" );
				below->println();
			}

			if ( above && *above == JagLineSeg2DPair::NULLVALUE ) {
				d("s6201 above is NULLVALUE abort\n" );
				//abort();
				above = NULL;
			}

			if ( below && *below == JagLineSeg2DPair::NULLVALUE ) {
				d("s2098 below is NULLVALUE abort\n" );
				//abort();
				below = NULL;
			}
			#endif

			if ( above && below ) {
				if ( above->color != below->color 
				     && lineIntersectLine( above->key.x1,above->key.y1,above->key.x2,above->key.y2, 
									    below->key.x1,below->key.y1,below->key.x2,below->key.y2) ) {
					//d("s7640 left above below intersect\n" );
					return true;
				}
			} else if ( above ) {
				if ( above->color != seg.color 
					 && lineIntersectLine( above->key.x1,above->key.y1,above->key.x2,above->key.y2, 
									    seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2) ) {
					/***
					d("s7641 left above seg intersect\n" );
					//above->println();
					//seg.println();
					***/
					return true;
				}
			} else if ( below ) {
				if ( below->color != seg.color 
					 && lineIntersectLine( below->key.x1,below->key.y1,below->key.x2,below->key.y2, 
									    seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2) ) {
					//d("s7641 left below seg intersect\n" );
					return true;
				}
			}
		} else {
			// right end
			above = jarr->getSucc( seg );
			below = jarr->getPred( seg );

			#if 0
			d("s1290 JAG_RIGHT: \n" );
			if ( above ) {
				d("s7781 above print: \n" );
				above->println();
			}
			if ( below ) {
				d("s7681 below print: \n" );
				below->println();
			}

			if ( above && *above == JagLineSeg2DPair::NULLVALUE ) {
				d("s7201 above is NULLVALUE abort\n" );
				//abort();
				above = NULL;
			}

			if ( below && *below == JagLineSeg2DPair::NULLVALUE ) {
				/***
				d("72098 below is NULLVALUE abort\n" );
				jarr->printKey();
				//seg.println();
				//abort();
				***/
				below = NULL;
			}
			#endif

			if ( above && below ) {
				if ( above->color != below->color 
					 && lineIntersectLine( above->key.x1,above->key.y1,above->key.x2,above->key.y2, 
									    below->key.x1,below->key.y1,below->key.x2,below->key.y2) ) {
					//d("s7740 rightend above below intersect\n" );
					return true;
				}
			} else if ( above ) {
				if ( above->color != seg.color 
					 && lineIntersectLine( above->key.x1,above->key.y1,above->key.x2,above->key.y2, 
									    seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2) ) {
					//d("s7740 rightend above seg intersect\n" );
					return true;
				}
			} else if ( below ) {
				if ( below->color != seg.color 
					 && lineIntersectLine( below->key.x1,below->key.y1,below->key.x2,below->key.y2, 
									    seg.key.x1,seg.key.y1,seg.key.x2,seg.key.y2) ) {
					//d("s7740 rightend below seg intersect\n" );
					return true;
				}
			}
			jarr->remove( seg );
		}
	}

	delete [] points;
	delete jarr;

	//d("s7740 no intersect\n");
	return false;
}

bool JagGeo::polygonIntersectTriangle( const Jstr &mk1, const JagStrSplit &sp1,
			                         double x1, double y1, double x2, double y2, double x3, double y3, bool strict )
{
	double trix, triy, rx, ry;
	triangleRegion(x1,y1, x2,y2, x3,y3, trix, triy, rx, ry );
	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  trix, triy, rx, ry ) ) {
			return false;
		}
	}

	double dx1, dy1, dx2, dy2;
	const char *str;
	char *p; int i;
	for ( i=start; i < sp1.length()-2; ++i ) {
    	// line: x10, y10 --> x20, y20 
		str = sp1[i].c_str();
		if ( *str == '|' || *str == '!' ) break;  // test outer polygon only
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx1, dy1 );
		if ( pointInTriangle( dx1, dy1, x1,y1, x2,y2, x3,y3, strict, false) ) return true;

		str = sp1[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx2, dy2 );

		if ( lineIntersectLine(dx1,dy1,dx2,dy2, x1,y1,x2,y2 ) ) return true;
		if ( lineIntersectLine(dx1,dy1,dx2,dy2, x2,y2,x3,y3 ) ) return true;
		if ( lineIntersectLine(dx1,dy1,dx2,dy2, x3,y3,x1,y1 ) ) return true;
	}

	str = sp1[i].c_str();
	if ( strchrnum( str, ':') >= 1 ) {
		get2double(str, p, ':', dx1, dy1 );
		if ( pointInTriangle( dx1, dy1, x1,y1, x2,y2, x3,y3, strict, false) ) return true;
	}

	return false;
}


bool JagGeo::polygonIntersectLine( const Jstr &mk1, const JagStrSplit &sp1, 
								   double x1, double y1, double x2, double y2 )
{
	double trix, triy, rx, ry;
	trix = (x1+x2)/2.0;
	triy = (y1+y2)/2.0;
	rx = fabs( trix - jagmin(x1,x2) );
	ry = fabs( triy - jagmin(y1,y2) );
	//d("s1209 x1=%.2f y1=%.2f  x2=%.2f y2=%.2f\n", x1, y1, x2, y2 );
	//d("s2288 line x1 trix=%.2f triy=%.2f rx=%.2f ry=%.2f\n", trix, triy, rx, ry );
	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		//d("s3062 sp1[]0]=[%s] bbx=%.2f bby=%.2f brx=%.2f bry=%.2f\n", sp1[JAG_SP_START+0].c_str(), bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  trix, triy, rx, ry ) ) {
			//d("s3383 bound2DDisjoint sp1[JAG_SP_START+0]=[%s]\n", sp1[JAG_SP_START+0].c_str() );
			return false;
		}
	}

	JagPolygon pgon;
	int rc = JagParser::addPolygonData( pgon, sp1, false );
	if ( rc < 0 ) {
		//d("s3647 addPolygonData rc=%d < 0  false\n", rc );
		return false;
	}

	if ( pointWithinPolygon( x1, y1, pgon ) ) { 
		//d("s3283 x1, y1 in pgon\n" );
		return true; 
	}

	if ( pointWithinPolygon( x2, y2, pgon ) ) { 
		//d("s3283 x2, y2 in pgon\n" );
		return true; 
	}

	double dx1, dy1, dx2, dy2;
	const char *str;
	char *p; int i;
	for ( i=start; i < sp1.length()-2; ++i ) {
    	// line: x10, y10 --> x20, y20 
		str = sp1[i].c_str();
		if ( *str == '|' || *str == '!' ) break;  // test outer polygon only
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx1, dy1 );

		str = sp1[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx2, dy2 );

		if ( lineIntersectLine(dx1,dy1,dx2,dy2, x1,y1,x2,y2 ) ) {
			//d("s9737 lineIntersectLine true\n" );
			return true;
		}
	}

	return false;
}

bool JagGeo::polygonIntersectRectangle( const Jstr &mk1, const JagStrSplit &sp1,
                                         double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, a, b ) ) {
			return false;
		}
	}

	//d("s8612 polygonIntersectRectangle sp1:\n" );
	//sp1.print();

	JagLine2D line[4];
	edgesOfRectangle( a, b, line );
	//double gx1,gy1, gx2,gy2;
	for ( int i=0; i < 4; ++i ) {
		line[i].transform(x0,y0,nx);
	}
	
    double dx1, dy1, dx2, dy2;
    const char *str;
    char *p; int i;
	for ( i=start; i < sp1.length()-2; ++i ) {
        str = sp1[i].c_str();
		if ( *str == '|' || *str == '!' ) break;
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx1, dy1 );
		if ( pointWithinRectangle( dx1, dy1, x0,y0,a,b,nx, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx2, dy2 );

		for ( int j=0;j<4;++j) {
			if ( lineIntersectLine( dx1,dy1,dx2,dy2, line[j].x1,line[j].y1,line[j].x2,line[j].y2 ) ) return true;
		}
	}

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 1 ) {
    	get2double(str, p, ':', dx1, dy1 );
		if ( pointWithinRectangle( dx1, dy1, x0,y0,a,b,nx, strict ) ) return true;
	}

	return false;
}

bool JagGeo::polygonIntersectEllipse( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	if ( jagIsZero(a) || jagIsZero(b) ) return false;

	int start = JAG_SP_START;
	if ( mk1 == JAG_OJAG ) {
		double bbx, bby, brx, bry;
		boundingBoxRegion(sp1[JAG_SP_START+0], bbx, bby, brx, bry );
		if ( bound2DDisjoint( bbx, bby, brx, bry,  x0, y0, a, b ) ) {
			return false;
		}
	}

    double dx1, dy1, dx2, dy2;
    const char *str;
    char *p; int i;
	for ( i=start; i < sp1.length()-2; ++i ) {
        str = sp1[i].c_str();
		if ( *str == '|' || *str == '!' ) break;
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx1, dy1 );
		if ( pointWithinEllipse( dx1, dy1, x0,y0,a,b,nx, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx2, dy2 );
		if ( lineIntersectEllipse( dx1,dy1,dx2,dy2, x0, y0, a, b, nx, strict ) ) return true;
	}

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 1 ) {
    	get2double(str, p, ':', dx1, dy1 );
		if ( pointWithinEllipse( dx1, dy1, x0,y0,a,b,nx, strict ) ) return true;
	}

	return false;
}


//////////////////////////// 2D triangle //////////////////////////////////////////////////
bool JagGeo::triangleWithinTriangle( double x10, double y10, double x20, double y20, double x30, double y30,
			                         double x1, double y1, double x2, double y2, double x3, double y3, bool strict )

{
   	JagPoint2D p10( x10, y10 );
   	JagPoint2D p20( x20, y20 );
   	JagPoint2D p30( x30, y30 );

   	JagPoint2D p1( x1, y1 );
   	JagPoint2D p2( x2, y2 );
   	JagPoint2D p3( x3, y3 );

	bool rc = pointWithinTriangle( p10, p1, p2, p3, strict, true );
	if ( ! rc ) return false;

	rc = pointWithinTriangle( p20, p1, p2, p3, strict, true );
	if ( ! rc ) return false;

	rc = pointWithinTriangle( p30, p1, p2, p3, strict, true );
	if ( ! rc ) return false;

	return true;
}

/***
bool JagGeo::triangleWithinSquare( double x10, double y10, double x20, double y20, double x30, double y30,
                           double x0, double y0, double r, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	if ( bound2DTriangleDisjoint( x10, y10, x20, y20, x30, y30, x0, y0, r,r ) ) return false;

	double sq_x[3], sq_y[3];
	sq_x[0] = x10; sq_y[0] = y10;
	sq_x[1] = x20; sq_y[1] = y20;
	sq_x[2] = x30; sq_y[2] = y30;
	double loc_x, loc_y;
	for ( int i=0; i < 3; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, sq_x[i], sq_y[i], nx, loc_x, loc_y );
		if ( ! locIn2DCenterBox( loc_x, loc_y, r,r, strict ) ) { return false; }
	}
	return true;
}
***/

bool JagGeo::triangleWithinRectangle( double x10, double y10, double x20, double y20, double x30, double y30,
                                         double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	if ( bound2DTriangleDisjoint( x10, y10, x20, y20, x30, y30, x0, y0, a,b ) ) return false;
	double sq_x[3], sq_y[3];
	sq_x[0] = x10; sq_y[0] = y10;
	sq_x[1] = x20; sq_y[1] = y20;
	sq_x[2] = x30; sq_y[2] = y30;
	double loc_x, loc_y;
	for ( int i=0; i < 3; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, sq_x[i], sq_y[i], nx, loc_x, loc_y );
		if ( ! locIn2DCenterBox( loc_x, loc_y, a,b, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::triangleWithinCircle( double x10, double y10, double x20, double y20, double x30, double y30,
								   double x0, double y0, double r, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;
	if ( bound2DTriangleDisjoint( x10, y10, x20, y20, x30, y30, x0, y0, r,r ) ) return false;
	double sq_x[3], sq_y[3];
	sq_x[0] = x10; sq_y[0] = y10;
	sq_x[1] = x20; sq_y[1] = y20;
	sq_x[2] = x30; sq_y[2] = y30;
	double loc_x, loc_y;
	for ( int i=0; i < 3; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, sq_x[i], sq_y[i], nx, loc_x, loc_y );
		if ( ! pointWithinCircle( loc_x, loc_y, 0.0, 0.0, r, strict ) ) { return false; }

	}
	return true;
}


bool JagGeo::triangleWithinEllipse( double x10, double y10, double x20, double y20, double x30, double y30,
									double x0, double y0, double a, double b, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;
	if ( jagIsZero(a) || jagIsZero(b) ) return false;
	if ( bound2DTriangleDisjoint( x10, y10, x20, y20, x30, y30, x0, y0, a,b ) ) return false;

	double tri_x[3], tri_y[3];
	tri_x[0] = x10; tri_y[0] = y10;
	tri_x[1] = x20; tri_y[1] = y20;
	tri_x[2] = x30; tri_y[2] = y30;
	double loc_x, loc_y;
	for ( int i=0; i < 3; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, tri_x[i], tri_y[i], nx, loc_x, loc_y );
		if ( ! pointWithinNormalEllipse( loc_x, loc_y, a, b, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::triangleWithinPolygon( double x10, double y10, double x20, double y20, double x30, double y30,
									const Jstr &mk2, const JagStrSplit &sp2, bool strict )

{
	double bbx, bby, rx, ry;
	getPolygonBound( mk2, sp2, bbx, bby, rx, ry );
	if ( bound2DTriangleDisjoint( x10, y10, x20, y20, x30, y30, bbx, bby, rx, ry ) ) {
		return false;
	}

	JagPolygon pgon;
	int rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) return false;

	double tri_x[3], tri_y[3];
	tri_x[0] = x10; tri_y[0] = y10;
	tri_x[1] = x20; tri_y[1] = y20;
	tri_x[2] = x30; tri_y[2] = y30;
	for ( int i=0; i < 3; ++i ) {
		if ( ! pointWithinPolygon( tri_x[i], tri_y[i], pgon ) ) {
			return false;
		}
	}
	return true;
}


///////////////////////// 2D ellipse
bool JagGeo::ellipseWithinTriangle( double px0, double py0, double a0, double b0, double nx0,
									double x1, double y1, double x2, double y2, double x3, double y3,
									bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( bound2DTriangleDisjoint( x1, y1, x2, y2, x3, y3, px0, py0, a0,b0 ) ) return false;

   	JagPoint2D p1( x1, y1 );
   	JagPoint2D p2( x2, y2 );
   	JagPoint2D p3( x3, y3 );
	JagVector<JagPoint2D> vec;
	JagPoint2D psq;
	bool rc;
	samplesOn2DEllipse( px0, py0, a0, b0, nx0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
   		psq.x = vec[i].x;
   		psq.y = vec[i].y;
		rc = pointWithinTriangle( psq, p1, p2, p3, strict, true );
		if ( ! rc ) return false;
	}
	return true;
}

bool JagGeo::ellipseWithinSquare( double px0, double py0, double a0, double b0, double nx0,
							 	double x0, double y0, double r, double nx, bool strict )

{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if (  bound2DDisjoint( px0, py0, a0,b0, x0, y0, r,r ) ) { return false; }

	JagVector<JagPoint2D> vec;
	double loc_x, loc_y;
	samplesOn2DEllipse( px0, py0, a0, b0, nx0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, vec[i].x, vec[i].y, nx, loc_x, loc_y );
		if ( ! locIn2DCenterBox( loc_x, loc_y, r, r, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::ellipseWithinRectangle( double px0, double py0, double a0, double b0, double nx0,
                                	double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if (  bound2DDisjoint( px0, py0, a0,b0, x0, y0, a,b ) ) { return false; }
	JagVector<JagPoint2D> vec;
	double loc_x, loc_y;
	samplesOn2DEllipse( px0, py0, a0, b0, nx0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, vec[i].x, vec[i].y, nx, loc_x, loc_y );
		if ( ! locIn2DCenterBox( loc_x, loc_y, a, b, strict ) ) { return false; }
	}
	return true;

}

bool JagGeo::ellipseWithinCircle( double px0, double py0, double a0, double b0, double nx0,
                                	double x0, double y0, double r, double nx, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if (  bound2DDisjoint( px0, py0, a0,b0, x0, y0, r,r ) ) { return false; }
	JagVector<JagPoint2D> vec;
	double loc_x, loc_y;
	samplesOn2DEllipse( px0, py0, a0, b0, nx0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, vec[i].x, vec[i].y, nx, loc_x, loc_y );
		if ( ! pointWithinCircle( loc_x, loc_y, 0.0, 0.0, r, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::ellipseWithinEllipse( double px0, double py0, double a0, double b0, double nx0,
                                	double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if (  bound2DDisjoint( px0, py0, a0,b0, x0, y0, a,a ) ) { return false; }
	JagVector<JagPoint2D> vec;
	double loc_x, loc_y;
	samplesOn2DEllipse( px0, py0, a0, b0, nx0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, vec[i].x, vec[i].y, nx, loc_x, loc_y );
		if ( ! pointWithinNormalEllipse( loc_x, loc_y, a, b, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::ellipseWithinPolygon( double px0, double py0, double a0, double b0, double nx0,
                                	const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	double bbx, bby, rx, ry;
	getPolygonBound( mk2, sp2, bbx, bby, rx, ry );
	if (  bound2DDisjoint( px0, py0, a0,b0, bbx, bby, rx, ry ) ) { return false; }

	JagPolygon pgon;
	int rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) return false;
	JagVector<JagPoint2D> vec;
	samplesOn2DEllipse( px0, py0, a0, b0, nx0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
		if ( ! pointWithinPolygon( vec[i].x, vec[i].y, pgon ) ) { return false; }
	}
	return true;
}

///////////////////////////////////// rectangle 3D /////////////////////////////////
bool JagGeo::rectangle3DWithinCube(  double px0, double py0, double pz0, double a0, double b0, double nx0, double ny0,
						        double x, double y, double z, double r, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x, y, z, r,r,r ) ) { return false; }

	JagPoint3D point[4];
	point[0].x = -a0; point[0].y = -b0; point[0].z = 0.0;
	point[1].x = -a0; point[1].y = b0; point[1].z = 0.0;
	point[2].x = a0; point[2].y = b0; point[2].z = 0.0;
	point[3].x = a0; point[3].y = -b0; point[3].z = 0.0;
	double sq_x, sq_y, sq_z, loc_x, loc_y, loc_z;
	for ( int i=0; i < 4; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordLocal2Global( px0, py0, pz0, point[i].x, point[i].y, point[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, r, r, r, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::rectangle3DWithinBox(  double px0, double py0, double pz0, double a0, double b0,
								double nx0, double ny0,
						        double x, double y, double z, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x, y, z, w,d,h ) ) { return false; }
	JagPoint3D point[4];
	point[0].x = -a0; point[0].y = -b0; point[0].z = 0.0;
	point[1].x = -a0; point[1].y = b0; point[1].z = 0.0;
	point[2].x = a0; point[2].y = b0; point[2].z = 0.0;
	point[3].x = a0; point[3].y = -b0; point[3].z = 0.0;
	double sq_x, sq_y, sq_z, loc_x, loc_y, loc_z;
	for ( int i=0; i < 4; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordLocal2Global( px0, py0, pz0, point[i].x, point[i].y, point[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, w, d, h, strict ) ) { return false; }
	}

	return true;
}

bool JagGeo::rectangle3DWithinSphere(  double px0, double py0, double pz0, double a0, double b0,
									   double nx0, double ny0,
                                	   double x, double y, double z, double r, bool strict )
{
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x, y, z, r,r,r ) ) { return false; }
	double sq_x[4], sq_y[4], sq_z[4];
	transform3DCoordLocal2Global( px0, py0, pz0, -a0, -b0, 0.0, nx0, ny0, sq_x[0], sq_y[0], sq_z[0] );
	transform3DCoordLocal2Global( px0, py0, pz0, -a0, b0, 0.0,  nx0, ny0, sq_x[1], sq_y[1], sq_z[1] );
	transform3DCoordLocal2Global( px0, py0, pz0, a0, b0, 0.0,   nx0, ny0, sq_x[2], sq_y[2], sq_z[2] );
	transform3DCoordLocal2Global( px0, py0, pz0, a0, -b0, 0.0,  nx0, ny0, sq_x[3], sq_y[3], sq_z[3] );
	for ( int i=0; i <4; ++i ) {
		if ( ! point3DWithinSphere( sq_x[i], sq_y[i], sq_z[i], x, y, z, r, strict ) ) {
			return false;
		}
	}
	return true;
}

bool JagGeo::rectangle3DWithinEllipsoid(  double px0, double py0, double pz0, double a0, double b0,
									double nx0, double ny0,
						        	double x, double y, double z, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x, y, z, w,d,h ) ) { return false; }
	JagPoint3D point[4];
	point[0].x = -a0; point[0].y = -b0; point[0].z = 0.0;
	point[1].x = -a0; point[1].y = b0; point[1].z = 0.0;
	point[2].x = a0; point[2].y = b0; point[2].z = 0.0;
	point[3].x = a0; point[3].y = -b0; point[3].z = 0.0;
	double sq_x, sq_y, sq_z, loc_x, loc_y, loc_z;
	for ( int i=0; i < 4; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordLocal2Global( px0, py0, pz0, point[i].x, point[i].y, point[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! point3DWithinNormalEllipsoid( loc_x, loc_y, loc_z, w,d,h, strict ) ) {
			return false;
		}
	}
	return true;
}

bool JagGeo::rectangle3DWithinCone(  double px0, double py0, double pz0, double a0, double b0,
									double nx0, double ny0,
						        	double x, double y, double z, 
							    	double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x, y, z, r,r,h ) ) { return false; }

	JagPoint3D point[4];
	point[0].x = -a0; point[0].y = -b0; point[0].z = 0.0;
	point[1].x = -a0; point[1].y = b0; point[1].z = 0.0;
	point[2].x = a0; point[2].y = b0; point[2].z = 0.0;
	point[3].x = a0; point[3].y = -b0; point[3].z = 0.0;
	double sq_x, sq_y, sq_z, loc_x, loc_y, loc_z;
	for ( int i=0; i < 4; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordLocal2Global( px0, py0, pz0, point[i].x, point[i].y, point[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! point3DWithinCone(  sq_x, sq_y, sq_z, x,y,z, r,h, nx, ny, strict ) ) {
			return false;
		}
	}
	return true;
}


///////////////////////////////////// line 3D /////////////////////////////////
bool JagGeo::line3DWithinLineString3D( double x10, double y10, double z10, double x20, double y20, double z20,
 								   const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	// 2 points are some two neighbor points in sp2
	int start = JAG_SP_START;
    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p;
	//d("s6790 start=%d len=%d  square: x0=%f y0=%f r=%f\n", start, sp1.length(), x0,y0,r );
	for ( int i=start; i < sp2.length()-1; ++i ) {
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx1, dy1, dz1 );
		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx2, dy2, dz2 );

		if ( jagEQ(x10, dx1) && jagEQ(y10, dy1)  && jagEQ(z10, dz1)
			 && jagEQ(x20, dx2) && jagEQ(y20, dy2) && jagEQ( z20, dz2 ) ) {
			return true;
		}
		if ( jagEQ(x20, dx1) && jagEQ(y20, dy1) && jagEQ(z20, dz1)
		     && jagEQ(x10, dx2) && jagEQ(y10, dy2) && jagEQ(z10, dz2 ) ) {
			return true;
		}
	}
	return false;
}
 bool JagGeo::line3DWithinCube(  double x10, double y10, double z10, 
 									double x20, double y20, double z20, 
 								    double x0, double y0, double z0, double r, double nx, double ny, bool strict )

{
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x0, y0, z0, r,r,r ) ) { return false; }
	double tri_x[2], tri_y[2], tri_z[2];
	tri_x[0] = x10; tri_y[0] = y10; tri_z[0] = z10;
	tri_x[1] = x20; tri_y[1] = y20; tri_z[1] = z20;
	double loc_x, loc_y, loc_z;
	for ( int i=0; i < 2; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordGlobal2Local( x0, y0, z0, tri_x[i], tri_y[i], tri_z[i], nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, r, r, r, strict ) ) { return false; }
	}

	return true;
}


bool JagGeo::line3DWithinBox(  double x10, double y10, double z10, 
								   double x20, double y20, double z20, 
									double x0, double y0, double z0,
									double w, double d, double h, 
									double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x0, y0, z0, w,d,h ) ) { return false; }
	double tri_x[2], tri_y[2], tri_z[2];
	tri_x[0] = x10; tri_y[0] = y10; tri_z[0] = z10;
	tri_x[1] = x20; tri_y[1] = y20; tri_z[1] = z20;
	double loc_x, loc_y, loc_z;
	for ( int i=0; i < 2; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordGlobal2Local( x0, y0, z0, tri_x[i], tri_y[i], tri_z[i], nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, w, d, h, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::line3DWithinSphere(  double x10, double y10, double z10, double x20, double y20, double z20, 
 									  double x, double y, double z, double r, bool strict )
{
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x, y, z, r,r,r ) ) { return false; }
	double tri_x[2], tri_y[2], tri_z[2];
	tri_x[0] = x10; tri_y[0] = y10; tri_z[0] = z10;
	tri_x[1] = x20; tri_y[1] = y20; tri_z[1] = z20;
	for ( int i=0; i < 2; ++i ) {
		if ( ! point3DWithinSphere( tri_x[i], tri_y[i], tri_z[i], x, y, z, r, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::line3DWithinEllipsoid(  double x10, double y10, double z10, double x20, double y20, double z20,
								 double x0, double y0, double z0,
								double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x0, y0, z0, w,d,h ) ) { return false; }
	double tri_x[2], tri_y[2], tri_z[2];
	tri_x[0] = x10; tri_y[0] = y10; tri_z[0] = z10;
	tri_x[1] = x20; tri_y[1] = y20; tri_z[1] = z20;
	double loc_x, loc_y, loc_z;
	for ( int i=0; i < 2; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordGlobal2Local( x0, y0, z0, tri_x[i], tri_y[i], tri_z[i], nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! point3DWithinNormalEllipsoid( loc_x, loc_y, loc_z, w,d,h, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::line3DWithinCone(  double x10, double y10, double z10, double x20, double y20, double z20,
								 double x0, double y0, double z0,
								double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x0, y0, z0, r,r,h ) ) { return false; }

	double tri_x[2], tri_y[2], tri_z[2];
	tri_x[0] = x10; tri_y[0] = y10; tri_z[0] = z10;
	tri_x[1] = x20; tri_y[1] = y20; tri_z[1] = z20;
	for ( int i=0; i < 2; ++i ) {
		if ( ! point3DWithinCone( tri_x[i], tri_y[i], tri_z[i], x0,y0,z0, r, h, nx, ny, strict ) ) {
			return false;
		}
	}
	return true;
}


///////////////////////////////////// linestring 3D /////////////////////////////////
bool JagGeo::lineString3DWithinLineString3D(  const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2, bool strict )

{
	int start1 = JAG_SP_START;
	double bbx1, bby1, bbz1, brx1, bry1, brz1;
	if ( mk1 == JAG_OJAG ) {
		boundingBox3DRegion(sp1[JAG_SP_START+0], bbx1, bby1, bbz1, brx1, bry1, brz1 );
	}

	int start2 = JAG_SP_START;
	double bbx2, bby2, bbz2, brx2, bry2, brz2;
	if ( mk2 == JAG_OJAG ) {
		boundingBox3DRegion(sp2[JAG_SP_START+0], bbx2, bby2, bbz2, brx2, bry2, brz2 );
	}

	if ( bound3DDisjoint( bbx1, bby1, bbz1, brx1, bry1, brz1, bbx2, bby2, bbz2, brx2, bry2, brz2 ) ) {
		return false;
	}

	// assume sp1 has fewer lines than sp2
	if ( strict ) {
		if ( sp1.length() - start1 >= sp2.length() - start2 ) return false;
	} else {
		if ( sp1.length() - start1 > sp2.length() - start2 ) return false;
	}

	int rc = KMPPointsWithin( sp1, start1, sp2, start2 );
	if ( rc < 0 ) return false;
	return true;
}

bool JagGeo::lineString3DWithinCube( const Jstr &mk1, const JagStrSplit &sp1,
 								    double x0, double y0, double z0, double r, double nx, double ny, bool strict )

{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, r ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double loc_x, loc_y, loc_z;
    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
        transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
        if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z,  r,r,r, strict ) ) { return false; }
    }
	return true;
}


bool JagGeo::lineString3DWithinBox( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0,
									double w, double d, double h, 
									double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  w, d, h ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double loc_x, loc_y, loc_z;
    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
        transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
        if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z,  w,d,h, strict ) ) { return false; }
    }
	return true;

}

bool JagGeo::lineString3DWithinSphere(  const Jstr &mk1, const JagStrSplit &sp1,
 									  double x0, double y0, double z0, double r, bool strict )
{
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, r ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
		if ( ! point3DWithinSphere( dx, dy, dz, x0, y0, z0, r, strict ) ) { return false; }
    }
	return true;
}


bool JagGeo::lineString3DWithinEllipsoid(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  w, d, h ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double dx, dy, dz;
    double loc_x, loc_y, loc_z;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
		transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
		if ( ! point3DWithinNormalEllipsoid( loc_x, loc_y, loc_z, w,d,h, strict ) ) { return false; }
    }
	return true;
}

bool JagGeo::lineString3DWithinCone(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, h ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
		if ( ! point3DWithinCone( dx, dy, dz, x0,y0,z0, r, h, nx, ny, strict ) ) { return false; }
    }
	return true;
}


///////////////////////////////////// polygon 3D /////////////////////////////////

bool JagGeo::polygon3DWithinCube( const Jstr &mk1, const JagStrSplit &sp1,
 								    double x0, double y0, double z0, double r, double nx, double ny, bool strict )

{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, r ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

	//d("s1029 polygon3DWithinCube() sp1:\n");
	//sp1.print();

    double loc_x, loc_y, loc_z;
    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
        transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
        if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z,  r,r,r, strict ) ) { return false; }
    }
	return true;
}

void JagGeo::polygon3DWithinCubePoints(JagVector<JagSimplePoint3D> &vec,  const JagStrSplit &sp1,
 								    double x0, double y0, double z0, double r, double nx, double ny, bool strict )

{
	if ( ! validDirection(nx, ny) ) return;
	int start = JAG_SP_START;

    double loc_x, loc_y, loc_z;
    double dx, dy, dz;
    const char *str;
    char *p;

    for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
        transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );

        if ( locIn3DCenterBox( loc_x, loc_y, loc_z,  r,r,r, strict ) ) { 
            vec.append( JagSimplePoint3D(  dx, dy, dz  ) );
        }
    }
}


bool JagGeo::polygon3DWithinBox( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0,
									double w, double d, double h, 
									double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  w, d, h ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double loc_x, loc_y, loc_z;
    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
        transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
        if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z,  w,d,h, strict ) ) { return false; }
    }
	return true;

}

void JagGeo::polygon3DWithinBoxPoints( JagVector<JagSimplePoint3D> &vec, const JagStrSplit &sp1,
									double x0, double y0, double z0,
									double w, double d, double h, 
									double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return;
	int start = JAG_SP_START;

    double loc_x, loc_y, loc_z;
    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
        transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
        if ( locIn3DCenterBox( loc_x, loc_y, loc_z,  w,d,h, strict ) ) { 
            vec.append( JagSimplePoint3D(  dx, dy, dz  ) );
        }
    }

}

bool JagGeo::polygon3DWithinSphere(  const Jstr &mk1, const JagStrSplit &sp1,
 									  double x0, double y0, double z0, double r, bool strict )
{
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, r ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
		if ( ! point3DWithinSphere( dx, dy, dz, x0, y0, z0, r, strict ) ) { return false; }
    }
	return true;
}

void JagGeo::polygon3DWithinSpherePoints( JagVector<JagSimplePoint3D> &vec, const JagStrSplit &sp1,
 									  double x0, double y0, double z0, double r, bool strict )
{
	int start = JAG_SP_START;

    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
		if ( point3DWithinSphere( dx, dy, dz, x0, y0, z0, r, strict ) ) { 
            vec.append( JagSimplePoint3D(dx, dy, dz) );
        }
    }
}


bool JagGeo::polygon3DWithinEllipsoid(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  w, d, h ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double dx, dy, dz;
    double loc_x, loc_y, loc_z;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
		transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
		if ( ! point3DWithinNormalEllipsoid( loc_x, loc_y, loc_z, w,d,h, strict ) ) { return false; }
    }
	return true;
}

void JagGeo::polygon3DWithinEllipsoidPoints( JagVector<JagSimplePoint3D> &vec, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return;
	int start = JAG_SP_START;

    double dx, dy, dz;
    double loc_x, loc_y, loc_z;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
		transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
		if ( point3DWithinNormalEllipsoid( loc_x, loc_y, loc_z, w,d,h, strict ) ) { 
            vec.append( JagSimplePoint3D(dx, dy, dz) );
        }
    }
}

bool JagGeo::polygon3DWithinCone(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, h ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
		if ( ! point3DWithinCone( dx, dy, dz, x0,y0,z0, r, h, nx, ny, strict ) ) { return false; }
    }
	return true;
}

void JagGeo::polygon3DWithinConePoints( JagVector<JagSimplePoint3D> &vec, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return;
	int start = JAG_SP_START;

    double dx, dy, dz;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
		if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx, dy, dz );
		if ( point3DWithinCone( dx, dy, dz, x0,y0,z0, r, h, nx, ny, strict ) ) { 
            vec.append( JagSimplePoint3D(dx, dy, dz) );
        }
    }
}


///////////////////////// multiPolygon3DWithin /////////////////////////////////////////////////
bool JagGeo::multiPolygon3DWithinCube( const Jstr &mk1, const JagStrSplit &sp1,
 								    double x0, double y0, double z0, double r, double nx, double ny, bool strict )

{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, r ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double loc_x, loc_y, loc_z;
    double dx, dy, dz;
    const char *str;
    char *p;
	bool skip = false;
    for ( int i=start; i < sp1.length(); ++i ) {
        if ( sp1[i] == "|" ) {  skip = true; }
        else if ( sp1[i] == "!" ) {  skip = false; }
        else {
            if ( skip ) continue;
        	str = sp1[i].c_str();
        	if ( strchrnum( str, ':') < 2 ) continue;
        	get3double(str, p, ':', dx, dy, dz );
        	transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
        	if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z,  r,r,r, strict ) ) { return false; }
		}
    }
	return true;
}


bool JagGeo::multiPolygon3DWithinBox( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0,
									double w, double d, double h, 
									double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  w, d, h ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double loc_x, loc_y, loc_z;
    double dx, dy, dz;
    const char *str;
    char *p;
	bool skip = false;
    for ( int i=start; i < sp1.length(); ++i ) {
        if ( sp1[i] == "|" ) {  skip = true; }
        else if ( sp1[i] == "!" ) {  skip = false; }
        else {
            if ( skip ) continue;
        	str = sp1[i].c_str();
        	if ( strchrnum( str, ':') < 2 ) continue;
        	get3double(str, p, ':', dx, dy, dz );
        	transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
        	if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z,  w,d,h, strict ) ) { return false; }
		}
    }
	return true;

}

bool JagGeo::multiPolygon3DWithinSphere(  const Jstr &mk1, const JagStrSplit &sp1,
 									  double x0, double y0, double z0, double r, bool strict )
{
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, r ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double dx, dy, dz;
    const char *str;
    char *p;
	bool skip = false;
    for ( int i=start; i < sp1.length(); ++i ) {
		// if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        if ( sp1[i] == "|" ) {  skip = true; }
        else if ( sp1[i] == "!" ) {  skip = false; }
        else {
            if ( skip ) continue;
        	str = sp1[i].c_str();
        	if ( strchrnum( str, ':') < 2 ) continue;
        	get3double(str, p, ':', dx, dy, dz );
			if ( ! point3DWithinSphere( dx, dy, dz, x0, y0, z0, r, strict ) ) { return false; }
		}
    }
	return true;
}


bool JagGeo::multiPolygon3DWithinEllipsoid(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  w, d, h ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double dx, dy, dz;
    double loc_x, loc_y, loc_z;
    const char *str;
    char *p;
	bool skip = false;
    for ( int i=start; i < sp1.length(); ++i ) {
		// if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        if ( sp1[i] == "|" ) {  skip = true; }
        else if ( sp1[i] == "!" ) {  skip = false; }
        else {
            if ( skip ) continue;
        	str = sp1[i].c_str();
        	if ( strchrnum( str, ':') < 2 ) continue;
        	get3double(str, p, ':', dx, dy, dz );
			transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, loc_x, loc_y, loc_z );
			if ( ! point3DWithinNormalEllipsoid( loc_x, loc_y, loc_z, w,d,h, strict ) ) { return false; }
		}
    }
	return true;
}

bool JagGeo::multiPolygon3DWithinCone(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, h ) ) {
            //d("s6770 false\n" );
            return false;
        }
    }

    double dx, dy, dz;
    const char *str;
    char *p;
	bool skip = false;
    for ( int i=start; i < sp1.length(); ++i ) {
		//if ( sp1[i] == "|" || sp1[i] == "!" ) break;
        if ( sp1[i] == "|" ) {  skip = true; }
        else if ( sp1[i] == "!" ) {  skip = false; }
        else {
            if ( skip ) continue;
        	str = sp1[i].c_str();
        	if ( strchrnum( str, ':') < 2 ) continue;
        	get3double(str, p, ':', dx, dy, dz );
			if ( ! point3DWithinCone( dx, dy, dz, x0,y0,z0, r, h, nx, ny, strict ) ) { return false; }
		}
    }
	return true;
}


///////////////////////////////////// triangle 3D /////////////////////////////////
 bool JagGeo::triangle3DWithinCube(  double x10, double y10, double z10, 
 									double x20, double y20, double z20, 
									double x30, double y30, double z30,
 								    double x0, double y0, double z0, double r, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x0, y0, z0, r,r,r )
		&& bound3DLineBoxDisjoint( x20, y20, z20, x30, y30, z30, x0, y0, z0, r,r,r )
		&& bound3DLineBoxDisjoint( x30, y30, z30, x10, y10, z10, x0, y0, z0, r,r,r ) )
	{
		return false;
	}

	double tri_x[3], tri_y[3], tri_z[3];
	tri_x[0] = x10; tri_y[0] = y10; tri_z[0] = z10;
	tri_x[1] = x20; tri_y[1] = y20; tri_z[1] = z20;
	tri_x[2] = x30; tri_y[2] = y30; tri_z[2] = z30;
	double loc_x, loc_y, loc_z;
	for ( int i=0; i < 3; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordGlobal2Local( x0, y0, z0, tri_x[i], tri_y[i], tri_z[i], nx, ny, loc_x, loc_y, loc_z );
		if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, r, r, r, strict ) ) { return false; }
	}

	return true;
}

bool JagGeo::triangle3DWithinBox(  double x10, double y10, double z10, 
								   double x20, double y20, double z20, 
									double x30, double y30, double z30,
									double x0, double y0, double z0,
									double w, double d, double h, 
									double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x0, y0, z0, w,d,h )
		&& bound3DLineBoxDisjoint( x20, y20, z20, x30, y30, z30, x0, y0, z0, w,d,h )
		&& bound3DLineBoxDisjoint( x30, y30, z30, x10, y10, z10, x0, y0, z0, w,d,h ) )
	{
		return false;
	}

	double tri_x[3], tri_y[3], tri_z[3];
	tri_x[0] = x10; tri_y[0] = y10; tri_z[0] = z10;
	tri_x[1] = x20; tri_y[1] = y20; tri_z[1] = z20;
	tri_x[2] = x30; tri_y[2] = y30; tri_z[2] = z30;
	double loc_x, loc_y, loc_z;
	for ( int i=0; i < 3; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordGlobal2Local( x0, y0, z0, tri_x[i], tri_y[i], tri_z[i], nx, ny, loc_x, loc_y, loc_z );
		if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, w, d, h, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::triangle3DWithinSphere(  double x10, double y10, double z10, double x20, double y20, double z20, 
										double x30, double y30, double z30,
 									  double x, double y, double z, double r, bool strict )
{
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x, y, z, r,r,r )
		&& bound3DLineBoxDisjoint( x20, y20, z20, x30, y30, z30, x, y, z, r,r,r )
		&& bound3DLineBoxDisjoint( x30, y30, z30, x10, y10, z10, x, y, z, r,r,r ) )
	{
		return false;
	}

	double tri_x[3], tri_y[3], tri_z[3];
	tri_x[0] = x10; tri_y[0] = y10; tri_z[0] = z10;
	tri_x[1] = x20; tri_y[1] = y20; tri_z[1] = z20;
	tri_x[2] = x30; tri_y[2] = y30; tri_z[2] = z30;
	for ( int i=0; i < 3; ++i ) {
		if ( ! point3DWithinSphere( tri_x[i], tri_y[i], tri_z[i], x, y, z, r, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::triangle3DWithinEllipsoid(  double x10, double y10, double z10, double x20, double y20, double z20,
										double x30, double y30, double z30,
								 double x0, double y0, double z0,
								double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x0, y0, z0, w,d,h )
		&& bound3DLineBoxDisjoint( x20, y20, z20, x30, y30, z30, x0, y0, z0, w,d,h )
		&& bound3DLineBoxDisjoint( x30, y30, z30, x10, y10, z10, x0, y0, z0, w,d,h ) )
	{
		return false;
	}

	double tri_x[3], tri_y[3], tri_z[3];
	tri_x[0] = x10; tri_y[0] = y10; tri_z[0] = z10;
	tri_x[1] = x20; tri_y[1] = y20; tri_z[1] = z20;
	tri_x[2] = x30; tri_y[2] = y30; tri_z[2] = z30;
	double loc_x, loc_y, loc_z;
	for ( int i=0; i < 3; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordGlobal2Local( x0, y0, z0, tri_x[i], tri_y[i], tri_z[i], nx, ny, loc_x, loc_y, loc_z );
		if ( ! point3DWithinNormalEllipsoid( loc_x, loc_y, loc_z, w,d,h, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::triangle3DWithinCone(  double x10, double y10, double z10, double x20, double y20, double z20,
										double x30, double y30, double z30,
								 double x0, double y0, double z0,
								double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	//if (  bound3DDisjoint( px0, py0, pz0, a0,b0,c0, x, y, z, w,d,h ) ) { return false; }
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x0, y0, z0, r,r,h )
		&& bound3DLineBoxDisjoint( x20, y20, z20, x30, y30, z30, x0, y0, z0, r,r,h )
		&& bound3DLineBoxDisjoint( x30, y30, z30, x10, y10, z10, x0, y0, z0, r,r,h ) )
	{
		return false;
	}

	double tri_x[3], tri_y[3], tri_z[3];
	tri_x[0] = x10; tri_y[0] = y10; tri_z[0] = z10;
	tri_x[1] = x20; tri_y[1] = y20; tri_z[1] = z20;
	tri_x[2] = x30; tri_y[2] = y30; tri_z[2] = z30;
	for ( int i=0; i < 3; ++i ) {
		if ( ! point3DWithinCone( tri_x[i], tri_y[i], tri_z[i], x0,y0,z0, r, h, nx, ny, strict ) ) {
			return false;
		}
	}
	return true;
}

/////////////////////////////////////////////// 3D box within
bool JagGeo::boxWithinCube(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0,
						        double x, double y, double z, double r, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x, y, z, r,r,r ) ) { return false; }

	JagPoint3D point[8];
	point[0].x = -a0; point[0].y = -b0; point[0].z = -c0;
	point[1].x = -a0; point[1].y = b0; point[1].z = -c0;
	point[2].x = a0; point[2].y = b0; point[2].z = -c0;
	point[3].x = a0; point[3].y = -b0; point[3].z = -c0;
	point[4].x = -a0; point[4].y = -b0; point[4].z = c0;
	point[5].x = -a0; point[5].y = b0; point[5].z = c0;
	point[6].x = a0; point[6].y = b0; point[6].z = c0;
	point[7].x = a0; point[7].y = -b0; point[7].z = c0;

	double sq_x, sq_y, sq_z, loc_x, loc_y, loc_z;
	for ( int i=0; i < 8; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordLocal2Global( px0, py0, pz0, point[i].x, point[i].y, point[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, r, r, r, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::boxWithinBox(  double px0, double py0, double pz0, double a0, double b0, double c0,
								double nx0, double ny0,
						        double x, double y, double z, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,c0, x, y, z, w,d,h ) ) { return false; }

	JagPoint3D point[8];
	point[0].x = -a0; point[0].y = -b0; point[0].z = -c0;
	point[1].x = -a0; point[1].y = b0; point[1].z = -c0;
	point[2].x = a0; point[2].y = b0; point[2].z = -c0;
	point[3].x = a0; point[3].y = -b0; point[3].z = -c0;
	point[4].x = -a0; point[4].y = -b0; point[4].z = c0;
	point[5].x = -a0; point[5].y = b0; point[5].z = c0;
	point[6].x = a0; point[6].y = b0; point[6].z = c0;
	point[7].x = a0; point[7].y = -b0; point[7].z = c0;

	double sq_x, sq_y, sq_z, loc_x, loc_y, loc_z;
	for ( int i=0; i < 8; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordLocal2Global( px0, py0, pz0, point[i].x, point[i].y, point[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, w, d, h, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::boxWithinSphere(  double px0, double py0, double pz0, double a0, double b0, double c0,
							   double nx0, double ny0,
                           	   double x, double y, double z, double r, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,c0, x, y, z, r,r,r ) ) { return false; }
	//if ( ! validDirection(nx, ny) ) return false;

	//if ( ! point3DWithinSphere( sq_x8, sq_y8, sq_z8, x0, y0, z0, r, strict ) ) { return false; }
	// return true;
	JagPoint3D point[8];
	point[0].x = -a0; point[0].y = -b0; point[0].z = -c0;
	point[1].x = -a0; point[1].y = b0; point[1].z = -c0;
	point[2].x = a0; point[2].y = b0; point[2].z = -c0;
	point[3].x = a0; point[3].y = -b0; point[3].z = -c0;
	point[4].x = -a0; point[4].y = -b0; point[4].z = c0;
	point[5].x = -a0; point[5].y = b0; point[5].z = c0;
	point[6].x = a0; point[6].y = b0; point[6].z = c0;
	point[7].x = a0; point[7].y = -b0; point[7].z = c0;

	double sq_x, sq_y, sq_z;
	for ( int i=0; i < 8; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordLocal2Global( px0, py0, pz0, point[i].x, point[i].y, point[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! point3DWithinSphere( sq_x, sq_y, sq_z, x, y, z, r, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::boxWithinEllipsoid(  double px0, double py0, double pz0, double a0, double b0, double c0,
								  double nx0, double ny0,
						        	double x0, double y0, double z0, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,c0, x0, y0, z0, w,d,h ) ) { return false; }

	JagPoint3D point[8];
	point[0].x = -a0; point[0].y = -b0; point[0].z = -c0;
	point[1].x = -a0; point[1].y = b0; point[1].z = -c0;
	point[2].x = a0; point[2].y = b0; point[2].z = -c0;
	point[3].x = a0; point[3].y = -b0; point[3].z = -c0;
	point[4].x = -a0; point[4].y = -b0; point[4].z = c0;
	point[5].x = -a0; point[5].y = b0; point[5].z = c0;
	point[6].x = a0; point[6].y = b0; point[6].z = c0;
	point[7].x = a0; point[7].y = -b0; point[7].z = c0;
	double sq_x, sq_y, sq_z, loc_x, loc_y, loc_z;
	for ( int i=0; i < 8; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordLocal2Global( px0, py0, pz0, point[i].x, point[i].y, point[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x0, y0, z0, sq_x, sq_y, sq_z, nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if ( ! point3DWithinNormalEllipsoid( loc_x, loc_y, loc_z, w,d,h, strict ) ) { return false; }
	}
	return true;

}

bool JagGeo::boxWithinCone(  double px0, double py0, double pz0, double a0, double b0, double c0,
								  double nx0, double ny0,
						        	double x0, double y0, double z0, 
							    	double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x0, y0, z0, r,r,h ) ) { return false; }
	JagPoint3D point[8];
	point[0].x = -a0; point[0].y = -b0; point[0].z = -c0;
	point[1].x = -a0; point[1].y = b0; point[1].z = -c0;
	point[2].x = a0; point[2].y = b0; point[2].z = -c0;
	point[3].x = a0; point[3].y = -b0; point[3].z = -c0;
	point[4].x = -a0; point[4].y = -b0; point[4].z = c0;
	point[5].x = -a0; point[5].y = b0; point[5].z = c0;
	point[6].x = a0; point[6].y = b0; point[6].z = c0;
	point[7].x = a0; point[7].y = -b0; point[7].z = c0;
	double sq_x, sq_y, sq_z;
	for ( int i=0; i < 8; ++i ) {
		transform3DCoordLocal2Global( px0, py0, pz0, point[i].x, point[i].y, point[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		if ( ! point3DWithinCone( sq_x, sq_y, sq_z, x0,y0,z0, r,h, nx,ny, strict ) ) { return false; }
	}
	return true;

}

// ellipsoid within
bool JagGeo::ellipsoidWithinCube(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0,
						        double x0, double y0, double z0, double r, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,c0, x0, y0, z0, r,r,r ) ) { return false; }

	double loc_x, loc_y, loc_z;
	JagVector<JagPoint3D> vec;
	samplesOnEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
    	transform3DCoordGlobal2Local( x0, y0, z0, vec[i].x, vec[i].y, vec[i].z, nx, ny, loc_x, loc_y, loc_z );
		if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, r, r, r, strict ) ) { return false; }
	}
	return true;
}


bool JagGeo::ellipsoidWithinBox(  double px0, double py0, double pz0, double a0, double b0, double c0,
								double nx0, double ny0,
						        double x0, double y0, double z0, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x0, y0, z0, w,d,h ) ) { return false; }
	JagVector<JagPoint3D> vec;
	double loc_x, loc_y, loc_z;
	samplesOnEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
    	transform3DCoordGlobal2Local( x0, y0, z0, vec[i].x, vec[i].y, vec[i].z, nx, ny, loc_x, loc_y, loc_z );
		if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, w, d, h, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::ellipsoidWithinSphere(  double px0, double py0, double pz0, double a0, double b0, double c0,
							   double nx0, double ny0,
                           	   double x0, double y0, double z0, double r, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,c0, x0, y0, z0, r,r,r ) ) { return false; }

	JagVector<JagPoint3D> vec;
	samplesOnEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
		if ( ! point3DWithinSphere( vec[i].x, vec[i].y, vec[i].z, x0,y0,z0, r, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::ellipsoidWithinEllipsoid(  double px0, double py0, double pz0, double a0, double b0, double c0,
								  double nx0, double ny0,
						        	double x0, double y0, double z0, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x0, y0, z0, w,d,h ) ) { return false; }
	JagVector<JagPoint3D> vec;
	double loc_x, loc_y, loc_z;
	samplesOnEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
    	transform3DCoordGlobal2Local( x0, y0, z0, vec[i].x, vec[i].y, vec[i].z, nx, ny, loc_x, loc_y, loc_z );
		if ( ! point3DWithinNormalEllipsoid( loc_x, loc_y, loc_z, w,d,h, strict ) ) { return false; }
	}
	return true;
}

bool JagGeo::ellipsoidWithinCone(  double px0, double py0, double pz0, double a0, double b0, double c0,
								  double nx0, double ny0,
						        	double x0, double y0, double z0, 
							    	double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x0, y0, z0, r,r,h ) ) { return false; }
	JagVector<JagPoint3D> vec;
	//double loc_x, loc_y, loc_z;
	samplesOnEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
		if ( ! point3DWithinCone( vec[i].x, vec[i].y, vec[i].z, x0,y0,z0, r, h, nx, ny, strict ) ) {
			return false;
		}
	}
	return true;
}




///////////////////////////////////// 3D cylinder /////////////////////////////////
bool JagGeo::cylinderWithinCube(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        double x, double y, double z, double r, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,c0, x, y, z, r,r,r ) ) { return false; }

	#if 1
	JagPoint3D center[2];
	// upper and lower local circles of the cynliner
	center[0].x = 0.0; center[0].y = 0.0; center[0].z = c0; 
	center[1].x = 0.0; center[1].y = 0.0; center[1].z = -c0; 

	double circlenx, circleny, circlenz; // direction of nx0 ny0 in nx ny
	transform3DDirection( nx0, ny0, nx, ny, circlenx, circleny );
	circlenz = sqrt( 1.0 - circlenx*circlenx - circleny*circleny );
	double outx, outy, outz, loc_x, loc_y, loc_z, cirx, ciry, cirz, cirnx, cirny, cirnz, a, b;
	double x1, y1, x2, y2, x3, y3, x4, y4;
	for ( int i=0; i < 2; ++i ) {
		// conver from local to global
		transform3DCoordLocal2Global( px0, py0, pz0, center[i].x, center[i].y, center[i].z, nx0, ny0, outx, outy, outz );
		transform3DCoordGlobal2Local( x, y, z, outx, outy, outz, nx, ny, loc_x, loc_y, loc_z );
		// now we have 3D circle  at (loc_x, loc_y, loc_z) and direction (circlenx, circleny)

		project3DCircleToXYPlane( loc_x, loc_y, loc_z, pr0, circlenx, circleny, cirx, ciry, a, b, cirnx );
		// we have an ellipse on xy-plane in normal cube space   (cirx, ciry) (a,b) cirnx
		boundingBoxOfRotatedEllipse( cirx, ciry, a, b, cirnx, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, r, r, strict ) ) { return false; }

		project3DCircleToYZPlane( loc_x, loc_y, loc_z, pr0, circleny, circlenz, ciry, cirz, a, b, cirny );
		// we have an ellipse on xy-plane in normal cube space   (ciry, cirz) (a,b) cirny
		boundingBoxOfRotatedEllipse( ciry, cirz, a, b, cirny, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, r, r, strict ) ) { return false; }

		project3DCircleToZXPlane( loc_x, loc_y, loc_z, pr0, circlenz, circlenx, cirz, cirx, a, b, cirnz );
		// we have an ellipse on xy-plane in normal cube space   (cirz, cirx) (a,b) cirnz
		boundingBoxOfRotatedEllipse( cirz, cirx, a, b, cirnz, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, r, r, strict ) ) { return false; }

    }
	return true;

	#else

	JagPoint3D center[2];
	center[0].x = 0.0; center[0].y = 0.0; center[0].z = -c0;
	center[1].x = 0.0; center[1].y = 0.0; center[1].z = c0;
	double gx,gy,gz, locx, locy, locz;
	for ( int i=0; i < 2; ++i ) {
		JagVector<JagPoint3D> vec;
		samplesOn3DCircle( center[i].x, center[i].y, center[i].z, pr0, nx0, ny0, NUM_SAMPLE, vec );
		for ( int j=0; j < vec.size(); ++j ) {
			transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
			transform3DCoordGlobal2Local( x,y,z, gx,gy,gz, nx, ny, locx, locy, locz );
			if ( ! locIn3DCenterBox( locx, locy, locz, r,r,r, strict ) ) return false;
		}
	}
	return true;

	#endif
}


bool JagGeo::cylinderWithinBox(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        double x, double y, double z, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,c0, x, y, z, w,d,h ) ) { return false; }

	#if 1
	JagPoint3D center[2];
	// upper and lower local circles of the cynliner
	center[0].x = 0.0; center[0].y = 0.0; center[0].z = c0; 
	center[1].x = 0.0; center[1].y = 0.0; center[1].z = -c0; 

	double circlenx, circleny, circlenz; // direction of nx0 ny0 in nx ny
	transform3DDirection( nx0, ny0, nx, ny, circlenx, circleny );
	circlenz = sqrt( 1.0 - circlenx*circlenx - circleny*circleny );
	double outx, outy, outz, loc_x, loc_y, loc_z, cirx, ciry, cirz, cirnx, cirny, cirnz, a, b;
	double x1, y1, x2, y2, x3, y3, x4, y4;
	for ( int i=0; i < 2; ++i ) {
		// conver from local to global
		transform3DCoordLocal2Global( px0, py0, pz0, center[i].x, center[i].y, center[i].z, nx0, ny0, outx, outy, outz );
		transform3DCoordGlobal2Local( x, y, z, outx, outy, outz, nx, ny, loc_x, loc_y, loc_z );
		// now we have 3D circle  at (loc_x, loc_y, loc_z) and direction (circlenx, circleny)

		project3DCircleToXYPlane( loc_x, loc_y, loc_z, pr0, circlenx, circleny, cirx, ciry, a, b, cirnx );
		// we have an ellipse on xy-plane in normal cube space   (cirx, ciry) (a,b) cirnx
		boundingBoxOfRotatedEllipse( cirx, ciry, a, b, cirnx, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, w, d, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, w, d, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, w, d, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, w, d, strict ) ) { return false; }

		project3DCircleToYZPlane( loc_x, loc_y, loc_z, pr0, circleny, circlenz, ciry, cirz, a, b, cirny );
		// we have an ellipse on xy-plane in normal cube space   (ciry, cirz) (a,b) cirny
		boundingBoxOfRotatedEllipse( ciry, cirz, a, b, cirny, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, d, h, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, d, h, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, d, h, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, d, h, strict ) ) { return false; }

		project3DCircleToZXPlane( loc_x, loc_y, loc_z, pr0, circlenz, circlenx, cirz, cirx, a, b, cirnz );
		// we have an ellipse on xy-plane in normal cube space   (cirz, cirx) (a,b) cirnz
		boundingBoxOfRotatedEllipse( cirz, cirx, a, b, cirnz, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, h, w, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, h, w, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, h, w, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, h, w, strict ) ) { return false; }

    }

	return true;

	#else

	JagPoint3D center[2];
	center[0].x = 0.0; center[0].y = 0.0; center[0].z = -c0;
	center[1].x = 0.0; center[1].y = 0.0; center[1].z = c0;
	double gx,gy,gz;
	for ( int i=0; i < 2; ++i ) {
		JagVector<JagPoint3D> vec;
		samplesOn3DCircle( center[i].x, center[i].y, center[i].z, pr0, nx0, ny0, NUM_SAMPLE, vec );
		for ( int j=0; j < vec.size(); ++j ) {
			transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
			gx -= x; gy -= y; gz -= z;
			if ( ! locIn3DCenterBox( gx,gy,gz, w,d,h, strict ) ) return false;
		}
	}
	return true;

	#endif
}

bool JagGeo::cylinderWithinSphere(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
                                double x, double y, double z, double r, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,c0, x, y, z, r,r,r ) ) { return false; }

	JagPoint3D center[2];
	center[0].x = 0.0; center[0].y = 0.0; center[0].z = -c0;
	center[1].x = 0.0; center[1].y = 0.0; center[1].z = c0;
	double gx,gy,gz;
	for ( int i=0; i < 2; ++i ) {
		JagVector<JagPoint3D> vec;
		samplesOn3DCircle( center[i].x, center[i].y, center[i].z, pr0, nx0, ny0, NUM_SAMPLE, vec );
		for ( int j=0; j < vec.size(); ++j ) {
			transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
			if ( ! point3DWithinSphere( gx,gy,gz, x, y, z, r, strict ) ) return false;
		}
	}
	return true;
}

bool JagGeo::cylinderWithinEllipsoid(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        	double x, double y, double z, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,c0,c0, x, y, z, w,d,h ) ) { return false; }

	JagPoint3D center[2];
	center[0].x = 0.0; center[0].y = 0.0; center[0].z = -c0;
	center[1].x = 0.0; center[1].y = 0.0; center[1].z = c0;
	double gx,gy,gz;
	for ( int i=0; i < 2; ++i ) {
		JagVector<JagPoint3D> vec;
		samplesOn3DCircle( center[i].x, center[i].y, center[i].z, pr0, nx0, ny0, NUM_SAMPLE, vec );
		for ( int j=0; j < vec.size(); ++j ) {
			transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
			if ( ! point3DWithinEllipsoid( gx,gy,gz, x, y, z, w,d,h, nx, ny, strict ) ) return false;
		}
	}
	return true;
}

bool JagGeo::cylinderWithinCone(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        	double x0, double y0, double z0, 
							    	double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,c0, x0, y0, z0, r,r,h ) ) { return false; }

	JagPoint3D center[2];
	center[0].x = 0.0; center[0].y = 0.0; center[0].z = -c0;
	center[1].x = 0.0; center[1].y = 0.0; center[1].z = c0;
	double gx,gy,gz;
	for ( int i=0; i < 2; ++i ) {
		JagVector<JagPoint3D> vec;
		samplesOn3DCircle( center[i].x, center[i].y, center[i].z, pr0, nx0, ny0, NUM_SAMPLE, vec );
		for ( int j=0; j < vec.size(); ++j ) {
			transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
			if ( ! point3DWithinCone( gx,gy,gz, x0, y0, z0, r, h, nx, ny, strict ) ) return false;
		}
	}
	return true;
}


// faster than coneWithinCube()
bool JagGeo::coneWithinCube_test(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        double x, double y, double z, double r, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;

	#if 0
	JagPoint3D center[2];
	// upper and lower local circles of the cynliner
	center[0].x = 0.0; center[0].y = 0.0; center[0].z = c0; 
	center[1].x = 0.0; center[1].y = 0.0; center[1].z = -c0; 

	double circlenx, circleny, circlenz; // direction of nx0 ny0 in nx ny
	transform3DDirection( nx0, ny0, nx, ny, circlenx, circleny );
	circlenz = sqrt( 1.0 - circlenx*circlenx - circleny*circleny );
	double outx, outy, outz, loc_x, loc_y, loc_z, cirx, ciry, cirz, cirnx, cirny, cirnz, a, b;
	double x1, y1, x2, y2, x3, y3, x4, y4;
	// for does only 1 point
	for ( int i=0; i < 1; ++i ) {
		// conver from local to global
		transform3DCoordLocal2Global( px0, py0, pz0, center[i].x, center[i].y, center[i].z, nx0, ny0, outx, outy, outz );
		transform3DCoordGlobal2Local( x, y, z, outx, outy, outz, nx, ny, loc_x, loc_y, loc_z );
		// now we have 3D circle  at (loc_x, loc_y, loc_z) and direction (circlenx, circleny)

		project3DCircleToXYPlane( loc_x, loc_y, loc_z, pr0, circlenx, circleny, cirx, ciry, a, b, cirnx );
		// we have an ellipse on xy-plane in normal cube space   (cirx, ciry) (a,b) cirnx
		boundingBoxOfRotatedEllipse( cirx, ciry, a, b, cirnx, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, r, r, strict ) ) { return false; }

		project3DCircleToYZPlane( loc_x, loc_y, loc_z, pr0, circleny, circlenz, ciry, cirz, a, b, cirny );
		// we have an ellipse on xy-plane in normal cube space   (ciry, cirz) (a,b) cirny
		boundingBoxOfRotatedEllipse( ciry, cirz, a, b, cirny, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, r, r, strict ) ) { return false; }

		project3DCircleToZXPlane( loc_x, loc_y, loc_z, pr0, circlenz, circlenx, cirz, cirx, a, b, cirnz );
		// we have an ellipse on xy-plane in normal cube space   (cirz, cirx) (a,b) cirnz
		boundingBoxOfRotatedEllipse( cirz, cirx, a, b, cirnz, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, r, r, strict ) ) { return false; }

    }

	// the tip
	transform3DCoordLocal2Global( px0, py0, pz0, center[1].x, center[1].y, center[1].z, nx0, ny0, outx, outy, outz );
	transform3DCoordGlobal2Local( x, y, z, outx, outy, outz, nx, ny, loc_x, loc_y, loc_z );
	if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, r, r, r, strict ) ) { return false; }

	return true;
	#else

	double gx,gy,gz, locx, locy, locz;
	// base points
	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( 0.0, 0.0, -c0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int j=0; j < vec.size(); ++j ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
		transform3DCoordGlobal2Local( x,y,z, gx, gy, gz, nx, ny, locx, locy, locz );
		if ( ! locIn3DCenterBox( locx, locy, locz, r, r, r, strict ) ) { return false; }
	}
	// tip
	transform3DCoordLocal2Global( px0, py0, pz0, 0.0, 0.0, c0, nx0, ny0, gx, gy, gz );
	transform3DCoordGlobal2Local( x,y,z, gx, gy, gz, nx, ny, locx, locy, locz );
	if ( ! locIn3DCenterBox( locx, locy, locz, r, r, r, strict ) ) { return false; }
	return true;
	#endif

}

///////////////////////////////////// 3D cone /////////////////////////////////
bool JagGeo::coneWithinCube(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        double x, double y, double z, double r, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,c0, x, y, z, r,r,r ) ) { return false; }

	#if 1
	JagPoint3D center[2];
	// upper and lower local circles of the cynliner
	center[0].x = 0.0; center[0].y = 0.0; center[0].z = c0; 
	center[1].x = 0.0; center[1].y = 0.0; center[1].z = -c0; 

	double circlenx, circleny, circlenz; // direction of nx0 ny0 in nx ny
	transform3DDirection( nx0, ny0, nx, ny, circlenx, circleny );
	circlenz = sqrt( 1.0 - circlenx*circlenx - circleny*circleny );
	double outx, outy, outz, loc_x, loc_y, loc_z, cirx, ciry, cirz, cirnx, cirny, cirnz, a, b;
	double x1, y1, x2, y2, x3, y3, x4, y4;
	// for does only 1 point
	for ( int i=0; i < 1; ++i ) {
		// conver from local to global
		transform3DCoordLocal2Global( px0, py0, pz0, center[i].x, center[i].y, center[i].z, nx0, ny0, outx, outy, outz );
		transform3DCoordGlobal2Local( x, y, z, outx, outy, outz, nx, ny, loc_x, loc_y, loc_z );
		// now we have 3D circle  at (loc_x, loc_y, loc_z) and direction (circlenx, circleny)

		project3DCircleToXYPlane( loc_x, loc_y, loc_z, pr0, circlenx, circleny, cirx, ciry, a, b, cirnx );
		// we have an ellipse on xy-plane in normal cube space   (cirx, ciry) (a,b) cirnx
		boundingBoxOfRotatedEllipse( cirx, ciry, a, b, cirnx, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, r, r, strict ) ) { return false; }

		project3DCircleToYZPlane( loc_x, loc_y, loc_z, pr0, circleny, circlenz, ciry, cirz, a, b, cirny );
		// we have an ellipse on xy-plane in normal cube space   (ciry, cirz) (a,b) cirny
		boundingBoxOfRotatedEllipse( ciry, cirz, a, b, cirny, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, r, r, strict ) ) { return false; }

		project3DCircleToZXPlane( loc_x, loc_y, loc_z, pr0, circlenz, circlenx, cirz, cirx, a, b, cirnz );
		// we have an ellipse on xy-plane in normal cube space   (cirz, cirx) (a,b) cirnz
		boundingBoxOfRotatedEllipse( cirz, cirx, a, b, cirnz, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, r, r, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, r, r, strict ) ) { return false; }

    }

	// the tip
	transform3DCoordLocal2Global( px0, py0, pz0, center[1].x, center[1].y, center[1].z, nx0, ny0, outx, outy, outz );
	transform3DCoordGlobal2Local( x, y, z, outx, outy, outz, nx, ny, loc_x, loc_y, loc_z );
	if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, r, r, r, strict ) ) { return false; }
	return true;

	#else

	double gx,gy,gz, locx, locy, locz;
	// base points
	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( 0.0, 0.0, -c0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int j=0; j < vec.size(); ++j ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
		transform3DCoordGlobal2Local( x,y,z, gx, gy, gz, nx, ny, locx, locy, locz );
		if ( ! locIn3DCenterBox( locx, locy, locz, r, r, r, strict ) ) { return false; }
	}
	// tip
	transform3DCoordLocal2Global( px0, py0, pz0, 0.0, 0.0, c0, nx0, ny0, gx, gy, gz );
	transform3DCoordGlobal2Local( x,y,z, gx, gy, gz, nx, ny, locx, locy, locz );
	if ( ! locIn3DCenterBox( locx, locy, locz, r, r, r, strict ) ) { return false; }
	return true;
	#endif

}


bool JagGeo::coneWithinBox(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        double x, double y, double z, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,c0, x, y, z, w,d,h ) ) { return false; }

	#if 1
	JagPoint3D center[2];
	// upper and lower local circles of the cynliner
	center[0].x = 0.0; center[0].y = 0.0; center[0].z = c0; 
	center[1].x = 0.0; center[1].y = 0.0; center[1].z = -c0; 

	double circlenx, circleny, circlenz; // direction of nx0 ny0 in nx ny
	transform3DDirection( nx0, ny0, nx, ny, circlenx, circleny );
	circlenz = sqrt( 1.0 - circlenx*circlenx - circleny*circleny );
	double outx, outy, outz, loc_x, loc_y, loc_z, cirx, ciry, cirz, cirnx, cirny, cirnz, a, b;
	double x1, y1, x2, y2, x3, y3, x4, y4;
	for ( int i=0; i < 1; ++i ) {
		// conver from local to global
		transform3DCoordLocal2Global( px0, py0, pz0, center[i].x, center[i].y, center[i].z, nx0, ny0, outx, outy, outz );
		transform3DCoordGlobal2Local( x, y, z, outx, outy, outz, nx, ny, loc_x, loc_y, loc_z );
		// now we have 3D circle  at (loc_x, loc_y, loc_z) and direction (circlenx, circleny)

		project3DCircleToXYPlane( loc_x, loc_y, loc_z, pr0, circlenx, circleny, cirx, ciry, a, b, cirnx );
		// we have an ellipse on xy-plane in normal cube space   (cirx, ciry) (a,b) cirnx
		boundingBoxOfRotatedEllipse( cirx, ciry, a, b, cirnx, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, w, d, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, w, d, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, w, d, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, w, d, strict ) ) { return false; }

		project3DCircleToYZPlane( loc_x, loc_y, loc_z, pr0, circleny, circlenz, ciry, cirz, a, b, cirny );
		// we have an ellipse on xy-plane in normal cube space   (ciry, cirz) (a,b) cirny
		boundingBoxOfRotatedEllipse( ciry, cirz, a, b, cirny, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, d, h, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, d, h, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, d, h, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, d, h, strict ) ) { return false; }

		project3DCircleToZXPlane( loc_x, loc_y, loc_z, pr0, circlenz, circlenx, cirz, cirx, a, b, cirnz );
		// we have an ellipse on xy-plane in normal cube space   (cirz, cirx) (a,b) cirnz
		boundingBoxOfRotatedEllipse( cirz, cirx, a, b, cirnz, x1, y1, x2, y2, x3, y3, x4, y4 );
		if ( ! locIn2DCenterBox( x1, y1, h, w, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x2, y2, h, w, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x3, y3, h, w, strict ) ) { return false; }
		if ( ! locIn2DCenterBox( x4, y4, h, w, strict ) ) { return false; }
    }

	// the tip
	transform3DCoordLocal2Global( px0, py0, pz0, center[1].x, center[1].y, center[1].z, nx0, ny0, outx, outy, outz );
	transform3DCoordGlobal2Local( x, y, z, outx, outy, outz, nx, ny, loc_x, loc_y, loc_z );
	if ( ! locIn3DCenterBox( loc_x, loc_y, loc_z, w, d, h, strict ) ) { return false; }
	return true;

	#else

	double gx,gy,gz, locx, locy, locz;
	// base points
	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( 0.0, 0.0, -c0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int j=0; j < vec.size(); ++j ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
		transform3DCoordGlobal2Local( x,y,z, gx, gy, gz, nx, ny, locx, locy, locz );
		if ( ! locIn3DCenterBox( locx, locy, locz, w, d, h, strict ) ) { return false; }
	}
	// tip
	transform3DCoordLocal2Global( px0, py0, pz0, 0.0, 0.0, c0, nx0, ny0, gx, gy, gz );
	transform3DCoordGlobal2Local( x,y,z, gx, gy, gz, nx, ny, locx, locy, locz );
	if ( ! locIn3DCenterBox( locx, locy, locz, w, d, h, strict ) ) { return false; }
	return true;
	#endif

}

bool JagGeo::coneWithinSphere(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
                                double x, double y, double z, double r, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,c0, x, y, z, r,r,r ) ) { return false; }

	double gx,gy,gz;
	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( 0.0, 0.0, -c0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int j=0; j < vec.size(); ++j ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
		if ( ! point3DWithinSphere( gx,gy,gz, x,y,z,r,strict ) ) { return false;}
	}
	// tip
	transform3DCoordLocal2Global( px0, py0, pz0, 0.0, 0.0, c0, nx0, ny0, gx, gy, gz );
	if ( ! point3DWithinSphere( gx,gy,gz, x,y,z,r,strict ) ) { return false;}
	return true;
}

bool JagGeo::coneWithinEllipsoid(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        	double x, double y, double z, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,c0,c0, x, y, z, w,d,h ) ) { return false; }

	double gx,gy,gz;
	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( 0.0, 0.0, -c0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int j=0; j < vec.size(); ++j ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
		if ( ! point3DWithinEllipsoid( gx,gy,gz, x,y,z,w,d,h,nx,ny,strict ) ) { return false;}
	}
	// tip
	transform3DCoordLocal2Global( px0, py0, pz0, 0.0, 0.0, c0, nx0, ny0, gx, gy, gz );
	if ( ! point3DWithinEllipsoid( gx,gy,gz, x,y,z,w,d,h,nx,ny,strict ) ) { return false;}
	return true;
}


bool JagGeo::coneWithinCone(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        	double x, double y, double z, 
							    	double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,c0, x, y, z, r,r,h ) ) { return false; }

	double gx,gy,gz;
	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( 0.0, 0.0, -c0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int j=0; j < vec.size(); ++j ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[j].x, vec[j].y, vec[j].z, nx0, ny0, gx, gy, gz );
		if ( ! point3DWithinCone( gx,gy,gz, x,y,z,r,h,nx,ny,strict ) ) { return false;}
	}
	// tip
	transform3DCoordLocal2Global( px0, py0, pz0, 0.0, 0.0, c0, nx0, ny0, gx, gy, gz );
	if ( ! point3DWithinCone( gx,gy,gz, x,y,z,r,h,nx,ny,strict ) ) { return false;}
	return true;
}


bool JagGeo::circle3DWithinCone( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
                                 double x0, double y0, double z0,
                                 double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,pr0, x0, y0, z0, r,r,h ) ) { return false; }

	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( px0, py0, pz0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	double sq_x, sq_y, sq_z;
	for ( int i=0; i < vec.size(); ++i ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[i].x, vec[i].y, vec[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		if ( ! point3DWithinCone( sq_x, sq_y, sq_z, x0, y0, z0, r, h, nx, ny, strict ) ) return false;
	}
	return true;
}

bool JagGeo::sphereWithinCone(  double px0, double py0, double pz0, double pr0,
						        	double x0, double y0, double z0, 
							    	double r, double h, double nx, double ny, bool strict )
{
	//if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,pr0, x0, y0, z0, r,r,h ) ) { return false; }

	if ( jagIsZero(r) && jagIsZero(h) ) return false;
    double locx, locy, locz;
    rotate3DCoordGlobal2Local( px0, py0, pz0, nx, ny, locx, locy, locz );
	double d = (h-locz)*r/sqrt(r*r+h*h );
	if ( strict ) {
		if ( pr0 < d ) { return true; }
	} else {
		if ( jagLE(pr0, d) ) return true;
	}
	return false;
}

bool JagGeo::point3DWithinPoint3D( double px, double py, double pz,  double x1, double y1, double z1, bool strict )
{
	if ( strict ) return false;
	if ( jagEQ(px,x1) && jagEQ(py,y1) && jagEQ(pz,z1) ) {
		return true;
	}
	return false;

}

bool JagGeo::point3DWithinLine3D( double px, double py, double pz, double x1, double y1, double z1, 
								  double x2, double y2, double z2, bool strict )
{
	if ( ! pointWithinLine(px,py, x1,y1,x2,y2,strict ) ) return false;
	if ( ! pointWithinLine(py,pz, y1,z1,y2,z2,strict ) ) return false;
	if ( ! pointWithinLine(pz,px, z1,x1,z2,x2,strict ) ) return false;
	return true;
}

bool JagGeo::pointWithinNormalEllipse( double px, double py, double w, double h, bool strict )
{
	if ( jagIsZero(w) || jagIsZero(h) ) return false;

	if ( px < -w || px > w || py < -h || py > h ) return false;

	double f = px*px/(w*w) + py*py/(h*h);
	if ( strict ) {
		if ( f < 1.0 ) return true; 
	} else {
		if ( jagLE(f, 1.0) ) return true; 
	}
	return false;
}

bool JagGeo::point3DWithinNormalEllipsoid( double px, double py, double pz,
									double w, double d, double h, bool strict )
{
	if ( jagIsZero(w) || jagIsZero(d) || jagIsZero(h) ) return false;
	if ( px < -w || px > w || py < d || py > d || pz < h || pz > h) return false;

	double f = px*px/(w*w) + py*py/(d*d) + pz*pz/(h*h);
	if ( strict ) {
		if ( f < 1.0 ) return true; 
	} else {
		if ( jagLE(f, 1.0) ) return true; 
	}
	return false;

}

bool JagGeo::point3DWithinNormalCone( double px, double py, double pz, 
										 double r, double h, bool strict )
{
	//d("s2203 point3DWithinNormalCone px=%f py=%f pz=%f r=%f h=%f\n", px,py,pz,r,h );
	if ( strict ) {
		// canot be on boundary
		if ( jagLE( pz, -h) || jagGE( pz, h ) ) return false;
	} else {
		if ( pz < -h || pz > h ) return false;
	}

	double rz = ( 1.0 - pz/h ) * r;
	if ( rz < 0.0 ) return false;

	double dist2 = px*px + py*py;
	if ( strict ) {
		if (  dist2 < rz*rz ) return true;
	} else {
		if ( jagLE(dist2, rz*rz) ) return true;
	}

	//d("s2550 dist2=%f rz*rz=%f\n", dist2,  rz*rz );
	return false;
}

bool JagGeo::point3DWithinNormalCylinder( double px, double py, double pz, 
										 double r, double h, bool strict )
{
	if ( strict ) {
		// canot be on boundary
		if ( jagLE( pz, -h) || jagGE( pz, h ) ) return false;
	} else {
		if ( pz < -h || pz > h ) return false;
	}

	double dist2 = px*px + py*py;
	if ( strict ) {
		if (  dist2 < r*r ) return true;
	} else {
		if ( jagLE(dist2, r*r) ) return true;
	}

	return false;
}


bool JagGeo::point3DWithinCone( double px, double py, double pz, 
								double x0, double y0, double z0,
								 double r, double h,  double nx, double ny, bool strict )
{
	double locx, locy, locz;
	transform3DCoordGlobal2Local( x0,y0,z0, px,py,pz, nx, ny, locx, locy, locz );
	/***
	d("s2220 point3DWithinCone px=%f py=%f pz=%f locx=%f locy=%f locz=%f\n",
				px,py,pz, locx, locy, locz );
	***/
	return point3DWithinNormalCone( locx, locy, locz, r, h, strict );
}

bool JagGeo::point3DWithinCylinder( double px, double py, double pz, 
								double x0, double y0, double z0,
								 double r, double h,  double nx, double ny, bool strict )
{
	double locx, locy, locz;
	transform3DCoordGlobal2Local( x0,y0,z0, px,py,pz, nx, ny, locx, locy, locz );
	return point3DWithinNormalCylinder( locx, locy, locz, r, h, strict );
}



//////////////////////////// end within methods ////////////////////////////////////////////////


/////////////////////////////// start intersect methods //////////////////////////////////////////////
bool JagGeo::doPointIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		return pointWithinPoint( px0, py0, x0, y0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		return pointWithinLine( px0, py0, x1, y1, x2, y2, strict );
	} else if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
    	JagPoint2D point( sp1[JAG_SP_START+0].c_str(), sp1[JAG_SP_START+1].c_str() );
    	JagPoint2D p1( sp2[JAG_SP_START+0].c_str(), sp2[JAG_SP_START+1].c_str() );
    	JagPoint2D p2( sp2[JAG_SP_START+2].c_str(), sp2[JAG_SP_START+3].c_str() );
    	JagPoint2D p3( sp2[JAG_SP_START+4].c_str(), sp2[JAG_SP_START+5].c_str() );
		return pointWithinTriangle( point, p1, p2, p3, strict, true );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		//return pointWithinSquare( px0, py0, x0, y0, r, nx, strict );
		return pointWithinRectangle( px0, py0, x0, y0, r,r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
			double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double w = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double h = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			double nx = safeget(sp2, JAG_SP_START+4);
			return pointWithinRectangle( px0, py0, x0, y0, w,h, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
			double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double r = jagatof( sp2[JAG_SP_START+2].c_str() );
			return pointWithinCircle( px0, py0, x, y, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
			double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double w = jagatof( sp2[JAG_SP_START+2].c_str() );
			double h = jagatof( sp2[JAG_SP_START+3].c_str() );
			double nx = safeget(sp2, JAG_SP_START+4);
			//double ny = safeget(sp2, JAG_SP_START+5);
			return pointWithinEllipse( px0, py0, x0, y0, w, h, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
			return pointWithinPolygon( px0, py0, mk2, sp2, false );
	}
	return false;
}

bool JagGeo::doPoint3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		return point3DWithinPoint3D( px0, py0, pz0, x1, y1, z1, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINE3D ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		return point3DWithinLine3D( px0, py0, pz0, x1, y1, z1, x2, y2, z2, strict );
	} else if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return point3DWithinBox( px0, py0, pz0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double c = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return point3DWithinBox( px0, py0, pz0, x0, y0, z0, a,b,c, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return point3DWithinSphere( px0,py0,pz0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() );
		double b = jagatof( sp2[JAG_SP_START+4].c_str() );
		double c = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return point3DWithinEllipsoid( px0, py0, pz0, x0, y0, z0, a,b,c, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return point3DWithinCone( px0,py0,pz0, x, y, z, r, h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CYLINDER ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return point3DWithinCylinder( px0,py0,pz0, x, y, z, r, h, nx, ny, strict );
	}
	return false;
}

bool JagGeo::doCircleIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 

	if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() );
		return circleIntersectCircle(px0,py0,pr0, x,y,r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() );
		double nx = safeget(sp2, JAG_SP_START+3);
		return circleIntersectRectangle(px0,py0,pr0, x,y,r,r,nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+2].c_str() );
		double h = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return circleIntersectRectangle( px0, py0, pr0, x0, y0, w, h, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		return circleIntersectTriangle(px0, py0, pr0, x1, y1, x2, y2, x3, y3, strict, true );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return circleIntersectEllipse(px0, py0, pr0, x0, y0, a, b, nx, strict, true );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return circleIntersectPolygon(px0, py0, pr0, mk2, sp2, strict );
	}
	return false;
}

double JagGeo::doCircle3DArea( int srid1, const JagStrSplit &sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return r * r * JAG_PI;
}
double JagGeo::doCircle3DPerimeter( int srid1, const JagStrSplit &sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return 2.0 * r * JAG_PI;
}

// circle surface with x y z and orientation
bool JagGeo::doCircle3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 

	double nx0 = 0.0;
	double ny0 = 0.0;
	if ( sp1.length() >= 5 ) { nx0 = jagatof( sp1[JAG_SP_START+4].c_str() ); }
	if ( sp1.length() >= 6 ) { ny0 = jagatof( sp1[JAG_SP_START+5].c_str() ); }

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return circle3DIntersectBox( px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if (  colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double c = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return circle3DIntersectBox( px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, a,b,c, nx, ny, strict );
	} else if (  colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return circle3DIntersectSphere( px0, py0, pz0, pr0, nx0, ny0, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return circle3DIntersectEllipsoid( px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, w,d,h,  nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return circle3DIntersectCone( px0,py0,pz0,pr0,nx0,ny0, x, y, z, r, h, nx, ny, strict );
	}

	return false;
}

bool JagGeo::doSphereIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return ellipsoidIntersectBox( px0, py0, pz0, pr0,pr0,pr0,0.0,0.0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 

		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return ellipsoidIntersectBox( px0, py0, pz0, pr0,pr0,pr0,0.0,0.0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return ellipsoidIntersectEllipsoid( px0, py0, pz0, pr0,pr0,pr0,0.0,0.0, x, y, z, r,r,r, 0.0,0.0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return ellipsoidIntersectEllipsoid( px0, py0, pz0, pr0,pr0,pr0,0.0,0.0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return ellipsoidIntersectCone( px0, py0, pz0, pr0,pr0,pr0,0.0,0.0,  x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

double JagGeo::doSquareArea( int srid1, const JagStrSplit &sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	return (a*a*4.0);
}

double JagGeo::doSquarePerimeter( int srid1, const JagStrSplit &sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	return (a*8.0);
}

double JagGeo::doSquare3DArea( int srid1, const JagStrSplit &sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return (a*a*4.0);
}

double JagGeo::doSquare3DPerimeter( int srid1, const JagStrSplit &sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return (a*8.0);
}
// 2D
bool JagGeo::doSquareIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	//d("s3033 doSquareIntersect colType2=[%s] \n", colType2.c_str() );
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+3);

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return rectangleIntersectTriangle( px0, py0, pr0, pr0, nx0, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleIntersectRectangle( px0, py0, pr0,pr0, nx0, x0, y0, a,a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleIntersectRectangle( px0, py0, pr0,pr0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleIntersectEllipse( px0, py0, pr0,pr0, nx0, x0, y0, r,r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleIntersectEllipse( px0, py0, pr0,pr0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return rectangleIntersectPolygon( px0, py0, pr0,pr0, nx0, mk2, sp2 );
	} else {
		throw 2341;
	}
	return false;
}

bool JagGeo::doSquare3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);
	double ny0 = safeget(sp1, JAG_SP_START+5);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		// return square3DIntersectCube( px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, r, nx, ny, strict );
		return rectangle3DIntersectBox( px0, py0, pz0, pr0,pr0, nx0, ny0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return rectangle3DIntersectBox( px0, py0, pz0, pr0,pr0,  nx0, ny0,x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return rectangle3DIntersectEllipsoid( px0, py0, pz0, pr0,pr0, nx0, ny0, x, y, z, r,r,r, 0.0,0.0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return rectangle3DIntersectEllipsoid( px0, py0, pz0, pr0,pr0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return rectangle3DIntersectCone( px0, py0, pz0, pr0,pr0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}


bool JagGeo::doCubeIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);
	double ny0 = safeget(sp1, JAG_SP_START+5);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return boxIntersectBox( px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxIntersectBox( px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return boxIntersectEllipsoid( px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x, y, z, r,r,r, 0.0,0.0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxIntersectEllipsoid( px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return boxIntersectCone( px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, r, h, nx,ny, strict );
	}
	return false;
}

// 2D
bool JagGeo::doRectangleIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return rectangleIntersectTriangle( px0, py0, a0, b0, nx0, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleIntersectRectangle( px0, py0, a0, b0, nx0, x0, y0, a,a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleIntersectRectangle( px0, py0, a0, b0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleIntersectEllipse( px0, py0, a0, b0, nx0, x0, y0, r,r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleIntersectEllipse( px0, py0, a0, b0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		return rectangleIntersectPolygon( px0, py0, a0, b0, nx0, mk2, sp2 );
	}
	return false;
}

// 3D rectiangle
bool JagGeo::doRectangle3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 

	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return rectangle3DIntersectBox( px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return rectangle3DIntersectBox( px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return rectangle3DIntersectEllipsoid( px0, py0, pz0, a0, b0, nx0, ny0, x, y, z, r,r,r,0.0,0.0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return rectangle3DIntersectEllipsoid( px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return rectangle3DIntersectCone( px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

bool JagGeo::doBoxIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+6);
	double ny0 = safeget(sp1, JAG_SP_START+7);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return boxIntersectBox( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxIntersectBox( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return boxIntersectEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, x, y, z, r,r,r,0.0,0.0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxIntersectEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return boxIntersectCone( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}


// 3D
bool JagGeo::doCylinderIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	// not supported for now
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 

	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return cylinderIntersectBox( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return cylinderIntersectBox( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return cylinderIntersectEllipsoid( px0, py0, pz0, pr0, c0,  nx0, ny0, x, y, z, r,r,r,0.0,0.0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return cylinderIntersectEllipsoid( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return cylinderIntersectCone( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

bool JagGeo::doConeIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return coneIntersectBox( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return coneIntersectBox( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return coneIntersectEllipsoid( px0, py0, pz0, pr0, c0,  nx0, ny0, x, y, z, r,r,r, 0.0,0.0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return coneIntersectEllipsoid( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return coneIntersectCone( px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	} else {
		throw 2910;
	}
	return false;
}

// 2D
bool JagGeo::doEllipseIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return ellipseIntersectTriangle( px0, py0, a0, b0, nx0, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return ellipseIntersectRectangle( px0, py0, a0, b0, nx0, x0, y0, a,a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return ellipseIntersectRectangle( px0, py0, a0, b0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return ellipseIntersectEllipse( px0, py0, a0, b0, nx0, x0, y0, r,r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return ellipseIntersectEllipse( px0, py0, a0, b0, nx0, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return ellipseIntersectPolygon( px0, py0, a0, b0, nx0, mk2, sp2 );
	}
	return false;
}

// 3D ellipsoid
bool JagGeo::doEllipsoidIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+6);
	double ny0 = safeget(sp1, JAG_SP_START+7);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return ellipsoidIntersectBox( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return ellipsoidIntersectBox( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return ellipsoidIntersectEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, x, y, z, r,r,r, 0.0,0.0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return ellipsoidIntersectEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return ellipsoidIntersectCone( px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

// 2D triangle within
bool JagGeo::doTriangleIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double x30 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double y30 = jagatof( sp1[JAG_SP_START+5].c_str() );

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return triangleIntersectTriangle( x10, y10, x20, y20, x30, y30, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		return triangleIntersectLine( x10, y10, x20, y20, x30, y30, x1, y1, x2, y2 );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return triangleIntersectRectangle( x10, y10, x20, y20, x30, y30, x0, y0, a,a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return triangleIntersectRectangle( x10, y10, x20, y20, x30, y30, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return triangleIntersectEllipse( x10, y10, x20, y20, x30, y30, x0, y0, r,r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return triangleIntersectEllipse( x10, y10, x20, y20, x30, y30, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return triangleIntersectPolygon( x10, y10, x20, y20, x30, y30, mk2, sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return triangleIntersectLineString( x10, y10, x20, y20, x30, y30, mk2, sp2 );
	}
	return false;
}

// 3D  triangle
bool JagGeo::doTriangle3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double z10 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double z20 = jagatof( sp1[JAG_SP_START+5].c_str() );
	double x30 = jagatof( sp1[JAG_SP_START+6].c_str() );
	double y30 = jagatof( sp1[JAG_SP_START+7].c_str() );
	double z30 = jagatof( sp1[JAG_SP_START+8].c_str() );

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return triangle3DIntersectBox( x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return triangle3DIntersectBox( x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return triangle3DIntersectEllipsoid( x10,y10,z10,x20,y20,z20,x30,y30,z30, x, y, z, r,r,r,0.0,0.0, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return triangle3DIntersectEllipsoid( x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return triangle3DIntersectCone( x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, r,h, nx,ny, strict );
	}
	return false;
}

// 2D line
bool JagGeo::doLineIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+3].c_str() );

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return lineIntersectTriangle( x10, y10, x20, y20, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return lineIntersectLineString( x10, y10, x20, y20, mk2, sp2, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return lineIntersectRectangle( x10, y10, x20, y20, x0, y0, a,a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineIntersectRectangle( x10, y10, x20, y20, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return lineIntersectEllipse( x10, y10, x20, y20, x0, y0, r,r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineIntersectEllipse( x10, y10, x20, y20, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return lineIntersectPolygon( x10, y10, x20, y20, mk2, sp2 );
	}
	return false;
}

bool JagGeo::doLine3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2, bool strict )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double z10 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double z20 = jagatof( sp1[JAG_SP_START+5].c_str() );

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return line3DIntersectBox( x10,y10,z10,x20,y20,z20, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return line3DIntersectBox( x10,y10,z10,x20,y20,z20, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return line3DIntersectSphere( x10,y10,z10,x20,y20,z20, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return line3DIntersectEllipsoid( x10,y10,z10,x20,y20,z20, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return line3DIntersectCone( x10,y10,z10,x20,y20,z20, x0, y0, z0, r,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CYLINDER ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return line3DIntersectCylinder( x10,y10,z10,x20,y20,z20, x0, y0, z0, r,r,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double x3 = jagatof( sp2[JAG_SP_START+6].c_str() ); 
		double y3 = jagatof( sp2[JAG_SP_START+7].c_str() ); 
		double z3 = jagatof( sp2[JAG_SP_START+8].c_str() ); 
		JagLine3D line(x10, y10, z10, x20, y20, z20 );
		JagPoint3D p1(x1,y1,z1);
		JagPoint3D p2(x2,y2,z2);
		JagPoint3D p3(x3,y3,z3);
		JagPoint3D atPoint;
		return line3DIntersectTriangle3D( line, p1, p2, p3, atPoint ); 
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		JagLine3D line(x10, y10, z10, x20, y20, z20 );
    	double px0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    	double py0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    	double pz0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    	double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
    	double h = jagatof( sp2[JAG_SP_START+4].c_str() ); 
    	double nx = safeget(sp2, JAG_SP_START+5);
    	double ny = safeget(sp2, JAG_SP_START+6);
		return line3DIntersectRectangle3D( line, px0, py0, pz0, w, h, nx, ny );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE3D ) {
		JagLine3D line(x10, y10, z10, x20, y20, z20 );
    	double px0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    	double py0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    	double pz0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    	double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
    	double nx = safeget(sp2, JAG_SP_START+4);
    	double ny = safeget(sp2, JAG_SP_START+5);
		return line3DIntersectRectangle3D( line, px0, py0, pz0, w, w, nx, ny );
	}
	return false;
}


//////////////// point
// strict true: point strictly inside (no boundary) in triangle.
//        false: point can be on boundary
// boundcheck:  do bounding box check or not

////////////////////////////////////// 2D circle /////////////////////////
bool JagGeo::circleIntersectTriangle( double centrex, double centrey, double radius, double v1x, double v1y, double v2x, double v2y, 
								double v3x, double v3y, bool strict, bool bound )
{
	if ( bound ) {
		if ( bound2DTriangleDisjoint( v1x,v1y,v2x,v2y,v3x,v3y,centrex,centrey,radius,radius ) ) return false;
	}

	// see http://www.phatcode.net/articles.php?id=459
    // TEST 1: Vertex within circle
    double cx = centrex - v1x;
    double cy = centrey - v1y;
    double radiusSqr = radius*radius;
    double c1sqr = cx*cx + cy*cy - radiusSqr;
    
    if ( jagLE(c1sqr, 0.0) ) {
      	return true;
	}
    
    cx = centrex - v2x;
    cy = centrey - v2y;
    double c2sqr = cx*cx + cy*cy - radiusSqr;
    if ( jagLE(c2sqr, 0.0) ) {
      	return true;
	}
    
    cx = centrex - v3x;
    cy = centrey - v3y;
    double c3sqr = cx*cx + cy*cy - radiusSqr;
    if ( jagLE(c3sqr, 0.0) ) {
      	return true;
	}
    
    // TEST 2: Circle centre within triangle
	if ( pointInTriangle( centrex, centrey, v1x, v1y, v2x, v2y, v3x, v3y, false, true ) ) {
		return true;
	}

    /***
    e1x = v2x - v1x
    e1y = v2y - v1y
    
    e2x = v3x - v2x
    e2y = v3y - v2y
    
    e3x = v1x - v3x
    e3y = v1y - v3y
    ***/
    
    // TEST 3: Circle intersects edge
	double len, k, ex, ey;
    cx = centrex - v1x;
    cy = centrex - v1y;
    ex = v2x - v1x;
    ex = v2y - v1y;
    k = cx*ex + cy*ey;
    
    if ( k > 0.0 ) {
      len = ex*ex + ey*ey;
      if ( k < len ) {
        if ( jagLE(c1sqr*len, k*k ) ) {
          return true;
		}
      }
    }
    
    // Second edge
    cx = centrex - v2x;
    cy = centrex - v2y;
    ex = v3x - v2x;
    ex = v3y - v2y;
    k = cx*ex + cy*ey;
    if ( k > 0.0 ) {
      len = ex*ex + ey*ey;
      if ( k < len ) {
        if ( jagLE(c2sqr*len, k*k ) ) {
          return true;
		}
      }
    }
    
    
    // Third edge
    cx = centrex - v2x;
    cy = centrex - v2y;
    ex = v1x - v3x;
    ex = v1y - v3y;
    k = cx*ex + cy*ey;
    if ( k > 0.0 ) {
      len = ex*ex + ey*ey;
      if ( k < len ) {
        if ( jagLE(c3sqr*len, k*k ) ) {
          return true;
		}
      }
    }

    // We're done, no intersection
    return false;
}


bool JagGeo::circleIntersectEllipse( double px0, double py0, double pr, 
							  double x, double y, double w, double h, double nx, 
							  bool strict, bool bound )
{
	if ( bound ) {
		if ( bound2DDisjoint( px0,py0,pr,pr,  x,y,w,h ) ) return false;
	}

	double px, py;
	transform2DCoordGlobal2Local( x, y, px0, py0, nx, px, py );

	JagVector<JagPoint2D> vec;
	samplesOn2DCircle( px0, py0, pr, 2*NUM_SAMPLE, vec );
	for ( int i=0; i < vec.size(); ++i ) {
		if ( pointWithinEllipse( vec[i].x, vec[i].y, x, y, w, h, nx, strict ) ) {
			return true;
		}
	}

	JagVector<JagPoint2D> vec2;
	samplesOn2DEllipse( x, y, w, h, nx, 2*NUM_SAMPLE, vec2 );
	for ( int i=0; i < vec2.size(); ++i ) {
		if ( pointWithinCircle( vec2[i].x, vec2[i].y, px0,py0,pr, false ) ) {
			return true;
		}
	}

	return false;
}


bool JagGeo::circleIntersectPolygon( double px0, double py0, double pr, 
							  const Jstr &mk2, const JagStrSplit &sp2,
							  bool strict )
{
	double bbx, bby, rx, ry;
	getPolygonBound( mk2, sp2, bbx, bby, rx, ry );
	if ( bound2DDisjoint( px0,py0,pr,pr,  bbx, bby, rx, ry ) ) return false;

	JagPolygon pgon;
	int rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) return false;

	JagVector<JagPoint2D> vec;
	samplesOn2DCircle( px0, py0, pr, 2*NUM_SAMPLE, vec );
	for ( int i=0; i < vec.size(); ++i ) {
		if ( pointWithinPolygon( vec[i].x, vec[i].y, pgon ) ) {
			return true;
		}
	}

	int jj;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &linestr = pgon.linestr[i];
		for ( int j=0; j < linestr.size(); ++j ) {
			jj = ( j + 1) % linestr.size();
			if ( lineIntersectEllipse( linestr.point[j].x, linestr.point[j].y, 
									   linestr.point[jj].x, linestr.point[jj].y,
									   px0,py0,pr,pr,0.0 , strict ) ) {
				return true;
			}
		}
	}

	return false;
}


/////////////////////////////////////// 2D circle ////////////////////////////////////////
bool JagGeo::circleIntersectCircle( double px, double py, double pr, double x, double y, double r, bool strict )
{
	if ( px+pr < x-r || px-pr > x+r || py+pr < y-r || py-pr > y+r ) return false;
	double dist2  = (px-x)*(px-x) + (py-y)*(py-y);
	if ( jagLE( dist2,  fabs(pr+r)*fabs(pr+r) ) ) return true;
	return false;
}

bool JagGeo::circleIntersectRectangle( double px0, double py0, double pr, double x0, double y0, 
									double w, double h, double nx,  bool strict )
{
	if ( ! validDirection(nx) ) return false;
	if ( bound2DDisjoint( px0, py0, pr, pr, x0, y0, w, h ) ) return false;

	// circle center inside square
	if ( pointWithinRectangle( px0, py0, x0, y0, w, h, nx, false ) ) return true;

	// convert px0 py0 to rectangle local coords
	double locx, locy;
	transform2DCoordGlobal2Local( x0, y0, px0, py0, nx, locx, locy );
	JagLine2D line[4];
	edgesOfRectangle( w, h, line );
	int rel;
	for ( int i=0; i < 4; ++i ) {
		rel = relationLineCircle( line[i].x1, line[i].y1, line[i].x2, line[i].y2, locx, locy, pr );
		if ( rel > 0 ) { return true; }
	}
	return false;
}


//////////////////////////////////////// 3D circle ///////////////////////////////////////
bool JagGeo::circle3DIntersectBox( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
							    double x, double y, double z,  double w, double d, double h, 
								double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,pr0, x, y, z, w,d,h ) ) { return false; }

	if (  point3DWithinBox( px0, py0, pz0,   x,y,z, w,d,h,nx,ny, false ) ) {
		return true;
	}

	int num = 12;
	JagLine3D line[num];
	edgesOfBox( w, d, h, line );
	transform3DLinesLocal2Global( x, y, z, nx, ny, num, line );  // to x-y-z
	for ( int i=0; i<num; ++i ) {
		if ( line3DIntersectEllipse3D( line[i], px0, py0, pz0, pr0,pr0, nx0, ny0 )  ) {
			return true;
		}
	}
	return false;
}

bool JagGeo::circle3DIntersectSphere( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
								   double x, double y, double z, double r, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,pr0, x, y, z, r,r,r ) ) { return false; }

	double d2 = (px0-x)*(px0-x) + (py0-y)*(py0-y) + (pz0-z)*(pz0-z);
	if ( jagLE( d2, (pr0+r)*(pr0+r) ) ) return true;
	return false;
}


bool JagGeo::circle3DIntersectEllipsoid( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
									  double x, double y, double z, double w, double d, double h,
									  double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if (  bound3DDisjoint( px0, py0, pz0, pr0,pr0,pr0, x, y, z, w,d,h ) ) { return false; }

	JagVector<JagPoint3D> vec;
	samplesOn3DCircle( px0, py0, pz0, pr0, nx0, ny0, NUM_SAMPLE, vec );
	double sq_x, sq_y, sq_z, locx, locy, locz ;
	for ( int i=0; i < vec.size(); ++i ) {
		transform3DCoordLocal2Global( px0, py0, pz0, vec[i].x, vec[i].y, vec[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, locx, locy, locz );
		if ( point3DWithinNormalEllipsoid( locx, locy, locz, w,d,h, strict ) ) { return true; }
	}

	return false;
}



//////////////////////////// 2D rectangle //////////////////////////////////////////////////
bool JagGeo::rectangleIntersectTriangle( double px0, double py0, double a0, double b0, double nx0,
									double x1, double y1, double x2, double y2, double x3, double y3,
									bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( bound2DTriangleDisjoint(x1,y1,x2,y2,x3,y3, px0,py0,a0,b0 ) ) return false;

	int num = 4;
	JagLine2D line[num];
	edgesOfRectangle( a0, b0, line );
	transform2DLinesLocal2Global( px0, py0, nx0, num, line );
	for ( int i=0; i < num; ++i ) {
		if ( lineIntersectLine( line[i].x1, line[i].y1, line[i].x2, line[i].y2, x1, y1, x2, y2 ) ) return true;
		if ( lineIntersectLine( line[i].x1, line[i].y1, line[i].x2, line[i].y2, x2, y2, x3, y3 ) ) return true;
		if ( lineIntersectLine( line[i].x1, line[i].y1, line[i].x2, line[i].y2, x3, y3, x1, y1 ) ) return true;
	}
	return false;
}

bool JagGeo::rectangleIntersectRectangle( double px0, double py0, double a0, double b0, double nx0,
                                	double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if (  bound2DDisjoint( px0, py0, a0,b0, x0, y0, a, b ) ) { return false; }

	int num = 4;
	JagLine2D line1[num];
	edgesOfRectangle( a0, b0, line1 );
	transform2DLinesLocal2Global( px0, py0, nx0, num, line1 );

	JagLine2D line2[num];
	edgesOfRectangle( a, b, line2 );
	transform2DLinesLocal2Global( x0, y0, nx, num, line2 );
	for ( int i=0; i < num; ++i ) {
		for ( int j=0; j < num; ++j ) {
			if ( lineIntersectLine( line1[i].x1, line1[i].y1, line1[i].x2, line1[i].y2,
							        line1[j].x1, line1[j].y1, line1[j].x2, line1[j].y2 ) ) {
				return true;
			}
		}
	}
	return false;

}

bool JagGeo::rectangleIntersectEllipse( double px0, double py0, double a0, double b0, double nx0,
                                	double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if ( jagIsZero(a) || jagIsZero(b) ) return false;
	if (  bound2DDisjoint( px0, py0, a0,b0, x0, y0, a, b ) ) { return false; }

	if ( pointWithinRectangle( x0, y0, px0, py0, a0,b0, nx0, false ) ) { return true; }
	if ( pointWithinEllipse( px0, py0, x0, y0, a,b, nx, false ) ) { return true; }

	int num = 4;
	JagLine2D line[num];
	edgesOfRectangle( a0, b0, line );
	transform2DLinesLocal2Global( px0, py0, nx0, num, line );
	int rel;
	for (int i=0; i<num; ++i ) {
		rel = relationLineEllipse( line[i].x1, line[i].y1, line[i].x2, line[i].y2, x0, y0, a, b, nx );
		if ( rel > 0) { return true; }
	}
	return false;
}


bool JagGeo::rectangleIntersectPolygon( double px0, double py0, double a0, double b0, double nx0,
							  const Jstr &mk2, const JagStrSplit &sp2 )
{
	double bbx, bby, rx, ry;
	getPolygonBound( mk2, sp2, bbx, bby, rx, ry );
	if ( bound2DDisjoint( px0,py0,a0,b0, bbx, bby, rx, ry ) ) return false;

	JagPolygon pgon;
	int rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) return false;

	
	JagPoint2D corn[4];
	cornersOfRectangle( a0, b0, corn );
	transform2DEdgesLocal2Global( px0, py0, nx0, 4, corn );
	for ( int i=0; i < 4; ++i ) {
		if ( pointWithinPolygon( corn[i].x, corn[i].y, pgon ) ) {
			return true;
		}
	}

	int jj;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &linestr = pgon.linestr[i];
		for ( int j=0; j < linestr.size(); ++j ) {
			jj = ( j + 1) % linestr.size();
			if ( lineIntersectRectangle( linestr.point[j].x, linestr.point[j].y, 
									   linestr.point[jj].x, linestr.point[jj].y,
									   px0,py0,a0,b0,nx0, true ) ) {
				return true;
			}
		}
	}

	return false;
}


//////////////////////////// 2D line  //////////////////////////////////////////////////
bool JagGeo::lineIntersectTriangle( double x10, double y10, double x20, double y20, 
			                         double x1, double y1, double x2, double y2, double x3, double y3, bool strict )

{
	if ( jagmax(x10, x20) < jagMin(x1, x2, x3) || jagmin(x10, x20) > jagMax(x1,x2,x3) ) {
		d("s3228 false\n" );
		return false;
	}
	if ( jagmax(y10, y20) < jagMin(y1, y2, y3) || jagmin(y10, y20) > jagMax(y1,y2,y3) ) {
		d("s3223 false\n" );
		return false;
	}

	if ( lineIntersectLine(x10,y10,x20,y20, x1, y1, x2, y2 ) ) return true;
	if ( lineIntersectLine(x10,y10,x20,y20, x2, y2, x3, y3 ) ) return true;
	if ( lineIntersectLine(x10,y10,x20,y20, x3, y3, x1, y1 ) ) return true;
	return false;
}

bool JagGeo::lineIntersectLineString( double x10, double y10, double x20, double y20,
							          const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	// 2 points are some two neighbor points in sp2
	int start = JAG_SP_START;
    double dx1, dy1, dx2, dy2;
    const char *str;
    char *p;
	//d("s6790 start=%d len=%d  square: x0=%f y0=%f r=%f\n", start, sp1.length(), x0,y0,r );
	for ( int i=start; i < sp2.length()-1; ++i ) {
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx1, dy1 );
		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx2, dy2 );
		if ( lineIntersectLine( x10,y10, x20,y20, dx1,dy1,dx2,dy2 ) ) return true;
	}
	return false;
}



bool JagGeo::lineIntersectRectangle( double x10, double y10, double x20, double y20, 
                                         double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	if ( pointWithinRectangle( x10, y10, x0, y0,a,b,nx,false) ) return true;
	if ( pointWithinRectangle( x20, y20, x0, y0,a,b,nx,false) ) return true;

	JagLine2D line[4];
	edgesOfRectangle( a, b, line );
	double gx1,gy1, gx2,gy2;
	for ( int i=0; i < 4; ++i ) {
		transform2DCoordLocal2Global( x0, y0, line[i].x1, line[i].y1, nx, gx1,gy1 );
		transform2DCoordLocal2Global( x0, y0, line[i].x2, line[i].y2, nx, gx2,gy2 );
		if ( lineIntersectLine( x10,y10,x20,y20, gx1,gy1,gx2,gy2 ) ) return true;
	}
	return false;
}

bool JagGeo::lineIntersectEllipse( double x10, double y10, double x20, double y20, 
									double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	if ( jagIsZero(a) || jagIsZero(b) ) return false;

	double tri_x[2], tri_y[2];
	tri_x[0] = x10; tri_y[0] = y10;
	tri_x[1] = x20; tri_y[1] = y20;
	double loc_x, loc_y;
	for ( int i=0; i < 2; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, tri_x[i], tri_y[i], nx, loc_x, loc_y );
		if ( pointWithinNormalEllipse( loc_x, loc_y, a, b, strict ) ) { return true; }
	}

	int rel = relationLineEllipse( x10, y10, x20, y20, x0, y0, a, b, nx );
	if ( rel > 0 ) return true;
	return false;
}

bool JagGeo::lineIntersectPolygon( double x10, double y10, double x20, double y20, 
										const Jstr &mk2, const JagStrSplit &sp2 ) 
{
	double X1, Y1, R1x, R1y;
	X1 = ( x10+x20)/2.0; R1x = fabs(x10-x20)/2.0;
	Y1 = ( y10+y20)/2.0; R1y = fabs(y10-y20)/2.0;
	double bbx, bby, rx, ry;
	getPolygonBound( mk2, sp2, bbx, bby, rx, ry );
	if ( bound2DDisjoint(  X1, Y1, R1x, R1y,  bbx, bby, rx, ry ) ) return false;

	JagPolygon pgon;
	int rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) return false;

	if ( pointWithinPolygon( x10, y10, pgon ) ) { return true; }
	if ( pointWithinPolygon( x20, y20, pgon ) ) { return true; }

	int jj;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &linestr = pgon.linestr[i];
		for ( int j=0; j < linestr.size(); ++j ) {
			jj = ( j + 1) % linestr.size();
			if ( lineIntersectLine( linestr.point[j].x, linestr.point[j].y, 
								    linestr.point[jj].x, linestr.point[jj].y,
								    x10,y10, x20,y20 ) ) {
				return true;
			}
		}
	}

	return false;

}


//////////////////////////// 2D triangle //////////////////////////////////////////////////
bool JagGeo::triangleIntersectTriangle( double x10, double y10, double x20, double y20, double x30, double y30,
			                         double x1, double y1, double x2, double y2, double x3, double y3, bool strict )

{
	double X1, Y1, R1x, R1y;
	double X2, Y2, R2x, R2y;
	triangleRegion( x10, y10, x20, y20, x30, y30, X1, Y1, R1x, R1y );
	triangleRegion( x1, y1, x2, y2, x3, y3, X2, Y2, R2x, R2y );
	if ( bound2DDisjoint(  X1, Y1, R1x, R1y, X2, Y2, R2x, R2y ) ) return false;

	JagLine2D line1[3];
	edgesOfTriangle( x10, y10, x20, y20, x30, y30, line1 );
	JagLine2D line2[3];
	edgesOfTriangle( x1, y1, x2, y2, x3, y3, line2 );
	for ( int i=0; i <3; ++i ) {
		for ( int j=0; j <3; ++j ) {
			if ( lineIntersectLine( line1[i], line2[j] ) ) {
				return true;
			}
		}
	}
	return false;
}

bool JagGeo::triangleIntersectLine( double x10, double y10, double x20, double y20, double x30, double y30,
			                         double x1, double y1, double x2, double y2 )

{
	double X1, Y1, R1x, R1y;
	//double X2, Y2, R2x, R2y;
	triangleRegion( x10, y10, x20, y20, x30, y30, X1, Y1, R1x, R1y );

	double trix, triy, rx, ry;
	trix = (x1+x2)/2.0;
	triy = (y1+y2)/2.0;
	rx = fabs( trix - jagmin(x1,x2) );
	ry = fabs( triy - jagmin(y1,y2) );
	if ( bound2DDisjoint(  X1, Y1, R1x, R1y, trix, triy, rx, ry ) ) return false;

	JagLine2D line1[3];
	edgesOfTriangle( x10, y10, x20, y20, x30, y30, line1 );
	JagLine2D line2;
	line2.x1 = x1; line2.y1 = y1;
	line2.x2 = x2; line2.y2 = y2;
	for ( int i=0; i <3; ++i ) {
		if ( lineIntersectLine( line1[i], line2 ) ) {
			return true;
		}
	}
	return false;
}

bool JagGeo::triangleIntersectRectangle( double x10, double y10, double x20, double y20, double x30, double y30,
                                         double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx) ) return false;
	return rectangleIntersectTriangle( x0,y0,a,b,nx,  x10,y10, x20,y20, x30, y30, false );
}

bool JagGeo::triangleIntersectEllipse( double x10, double y10, double x20, double y20, double x30, double y30,
									double x0, double y0, double a, double b, double nx, bool strict )

{
	if ( ! validDirection(nx) ) return false;
	if ( jagIsZero(a) || jagIsZero(b) ) return false;

	double X1, Y1, R1x, R1y;
	triangleRegion( x10, y10, x20, y20, x30, y30, X1, Y1, R1x, R1y );
	if ( bound2DDisjoint(  X1, Y1, R1x, R1y, x0,y0,a,b ) ) return false;

	JagLine2D line[3];
	edgesOfTriangle( x10, y10, x20, y20, x30, y30, line);
	double loc_x, loc_y;
	for ( int i=0; i < 3; ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, line[i].x1, line[i].y1, nx, loc_x, loc_y );
		if ( pointWithinNormalEllipse( loc_x, loc_y, a, b, strict ) ) { return true; }
	}
	int rel;
	for ( int i=0; i<3; ++i ) {
		rel = relationLineEllipse( line[i].x1, line[i].y1, line[i].x2, line[i].y2, x0, y0, a, b, nx );
		if ( rel > 0 ) return true;
	}
	return false;
}

bool JagGeo::triangleIntersectPolygon( double x10, double y10, double x20, double y20, double x30, double y30,
										const Jstr &mk2, const JagStrSplit &sp2 ) 
{
	double X1, Y1, R1x, R1y;
	triangleRegion( x10, y10, x20, y20, x30, y30, X1, Y1, R1x, R1y );
	double bbx, bby, rx, ry;
	getPolygonBound( mk2, sp2, bbx, bby, rx, ry );
	if ( bound2DDisjoint(  X1, Y1, R1x, R1y,  bbx, bby, rx, ry ) ) {
		//d("s8383 bound2DDisjoint\n" );
		return false;
	}

	JagPolygon pgon;
	int rc;
	//sp2.print();
	rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) {
		//d("s6338 triangleIntersectPolygon addPolygonData rc=%d false\n", rc );
		return false;
	}

	if ( pointWithinPolygon( x10, y10, pgon ) ) { return true; }
	if ( pointWithinPolygon( x20, y20, pgon ) ) { return true; }
	if ( pointWithinPolygon( x30, y30, pgon ) ) { return true; }

	int jj;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &linestr = pgon.linestr[i];
		for ( int j=0; j < linestr.size(); ++j ) {
			jj = ( j + 1) % linestr.size();
			if ( lineIntersectTriangle( linestr.point[j].x, linestr.point[j].y, 
									   linestr.point[jj].x, linestr.point[jj].y,
									   x10,y10, x20,y20, x30,y30, false ) ) {
				return true;
			}
		}
	}

	//d("s8371 false\n" );
	return false;

}

bool JagGeo::triangleIntersectLineString( double x10, double y10, double x20, double y20, double x30, double y30,
										const Jstr &mk2, const JagStrSplit &sp2 ) 
{
	double X1, Y1, R1x, R1y;
	triangleRegion( x10, y10, x20, y20, x30, y30, X1, Y1, R1x, R1y );
	#if 0
	d("s3420 triangleIntersectLineString x10=%.2f y10=%.2f x20=%.2f y20=%.2f x30=%.2f y30=%.2f\n",
		  x10, y10, x20, y20, x30, y30 );
	  #endif

	double bbx, bby, rx, ry;
	getLineStringBound( mk2, sp2, bbx, bby, rx, ry );

	if ( bound2DDisjoint(  X1, Y1, R1x, R1y,  bbx, bby, rx, ry ) ) {
		d("X1=%.2f Y1=%.2f R1x=%.2 R1y=%.2f\n",  X1, Y1, R1x, R1y );
		d(" bbx=%.2f, bby=%.2f, rx=%.2f, ry=%.2f\n",  bbx, bby, rx, ry );
		d("s7918 triangleIntersectLineString bound2DDisjoint return false\n" );
		return false;
	}

	JagLineString linestr;
	JagParser::addLineStringData( linestr, sp2 );

	for ( int j=0; j < linestr.size()-1; ++j ) {
		/**
		d("s2287 lp1x=%s lp1y=%s  lp2x=%s lp2y=%s\n", linestr.point[j].x, linestr.point[j].y,
		       linestr.point[j+1].x, linestr.point[j+1].y );
	   ***/
		if ( lineIntersectTriangle( jagatof(linestr.point[j].x), jagatof(linestr.point[j].y), 
								   jagatof(linestr.point[j+1].x), jagatof(linestr.point[j+1].y),
								   x10,y10, x20,y20, x30,y30, false ) ) {
			//d("s8632 j=%d line intersect triangle\n", j );
			return true;
		}
	}

	return false;

}

///////////////////////// 2D ellipse
bool JagGeo::ellipseIntersectTriangle( double px0, double py0, double a0, double b0, double nx0,
									double x1, double y1, double x2, double y2, double x3, double y3, bool strict )
{
	return triangleIntersectEllipse( x1, y1, x2, y2, x3, y3, px0, py0, a0, b0, nx0, false );
}

bool JagGeo::ellipseIntersectRectangle( double px0, double py0, double a0, double b0, double nx0,
                                	double x0, double y0, double a, double b, double nx, bool strict )
{
	return rectangleIntersectEllipse( x0, y0, a, b, nx, px0,py0, a0, b0, nx0, false );
}

bool JagGeo::ellipseIntersectEllipse( double px0, double py0, double a0, double b0, double nx0,
                                	double x0, double y0, double a, double b, double nx, bool strict )
{
	if ( ! validDirection(nx0) ) return false;
	if ( ! validDirection(nx) ) return false;
	if ( bound2DDisjoint( px0, py0, a0, b0, x0, y0, a, a ) ) return false;

	if ( pointWithinEllipse( px0, py0, x0, y0, a, b, nx, false ) ) {
		return true;
	}
	if ( pointWithinEllipse( x0, y0, px0, py0, a0, b0, nx0, false ) ) {
		return true;
	}

	JagVector<JagPoint2D> vec;
	double loc_x, loc_y;
	samplesOn2DEllipse( px0, py0, a0, b0, nx0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
    	transform2DCoordGlobal2Local( x0, y0, vec[i].x, vec[i].y, nx, loc_x, loc_y );
		if ( pointWithinNormalEllipse( loc_x, loc_y, a, b, strict ) ) { return true; }
	}
	return false;
}

bool JagGeo::ellipseIntersectPolygon( double px0, double py0, double a0, double b0, double nx0,
									  const Jstr &mk2, const JagStrSplit &sp2 )
{
	if ( ! validDirection(nx0) ) return false;
	double bbx, bby, rx, ry;
	getPolygonBound( mk2, sp2, bbx, bby, rx, ry );
	if ( bound2DDisjoint( px0,py0,a0,b0,  bbx, bby, rx, ry ) ) return false;

	JagPolygon pgon;
	int rc = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc < 0 ) return false;

	JagVector<JagPoint2D> vec;
	samplesOn2DEllipse( px0, py0, a0, b0, nx0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
		if ( pointWithinPolygon( vec[i].x, vec[i].y, pgon  ) ) { return true; }
	}
	return false;
}

///////////////////////////////////// rectangle 3D /////////////////////////////////
bool JagGeo::rectangle3DIntersectBox(  double px0, double py0, double pz0, double a0, double b0,
								double nx0, double ny0,
						        double x, double y, double z, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DDisjoint( px0, py0, pz0, a0,b0, jagmax(a0,b0), x, y, z, w, d, h ) ) return false;

	JagLine3D line1[4];
	edgesOfRectangle3D( a0, b0, line1 );
	transform3DLinesLocal2Global( px0, py0, pz0, nx0, ny0, 4, line1 );

	JagLine3D line2[12];
	edgesOfBox( w, d, h, line2 );
	transform3DLinesLocal2Global( x, y, z, nx, ny, 12, line2 );

	for ( int i=0;i<4;++i) {
		for ( int j=0;j<12;++j) {
			if ( line3DIntersectLine3D( line1[i], line2[j] )  ) {
				return true;
			}
		}
	}
	return false;
}

bool JagGeo::rectangle3DIntersectEllipsoid(  double px0, double py0, double pz0, double a0, double b0,
									double nx0, double ny0,
						        	double x, double y, double z, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DDisjoint( px0, py0, pz0, a0,b0, jagmax(a0,b0), x, y, z, w, d, h ) ) return false;

	if ( point3DWithinEllipsoid( px0, py0, pz0, x, y, z, w, d, h, nx, ny, false ) ) {
		return true;
	}

	// rect 4 corners in ellipsoid
	JagPoint3D corn[4];
	cornersOfRectangle3D( a0, b0, corn);
	transform3DEdgesLocal2Global( px0, py0, pz0, nx0, ny0, 4, corn );
	for ( int i=0; i<4; ++i ) {
		if ( point3DWithinEllipsoid(corn[i].x, corn[i].y, corn[i].z, x, y, z, w, d, h, nx, ny, false ) ) {
			return true;
		}
	}

	// on ellipsoid
	bool rc = triangle3DIntersectEllipsoid(  corn[0].x, corn[0].y, corn[0].z,
									corn[1].x,  corn[1].y,  corn[1].z,
									 corn[2].x, corn[2].y, corn[2].z,
									 x,y,z, w,d,h, nx, ny, false );
	if ( rc ) return true;
	return false;
}

bool JagGeo::rectangle3DIntersectCone(  double px0, double py0, double pz0, double a0, double b0,
									double nx0, double ny0,
						        	double x, double y, double z, 
							    	double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DDisjoint( px0, py0, pz0, a0,b0,b0, x, y, z, r, r, h ) ) return false;

	JagPoint3D point[4];
	point[0].x = -a0; point[0].y = -b0; point[0].z = 0.0;
	point[1].x = -a0; point[1].y = b0; point[1].z = 0.0;
	point[2].x = a0; point[2].y = b0; point[2].z = 0.0;
	point[3].x = a0; point[3].y = -b0; point[3].z = 0.0;
	double sq_x, sq_y, sq_z;
	for ( int i=0; i < 4; ++i ) {
		// sq_x, sq_y, sq_z are coords in x-y-z of each cube corner node
		transform3DCoordLocal2Global( px0, py0, pz0, point[i].x, point[i].y, point[i].z, nx0, ny0, sq_x, sq_y, sq_z );
		// transform3DCoordGlobal2Local( x, y, z, sq_x, sq_y, sq_z, nx, ny, loc_x, loc_y, loc_z );
		// loc_x, loc_y, loc_z are within second cube sys
		if (  point3DWithinCone(  sq_x, sq_y, sq_z, x,y,z, r,h, nx, ny, strict ) ) {
			return true;
		}
	}

	bool rc =  triangle3DIntersectCone(  
							point[0].x, point[0].y, point[0].z,
							point[1].x, point[1].y, point[1].z,
							point[2].x, point[2].y, point[2].z,
							x,y,z, r,h, nx,ny, false );
	return rc;
}


///////////////////////////////////// line 3D /////////////////////////////////
bool JagGeo::line3DIntersectLineString3D( double x10, double y10, double z10, double x20, double y20, double z20,
							          const Jstr &mk2, const JagStrSplit &sp2, bool strict )
{
	// 2 points are some two neighbor points in sp2
	int start = JAG_SP_START;
    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p;
	//d("s6790 start=%d len=%d  square: x0=%f y0=%f r=%f\n", start, sp1.length(), x0,y0,r );
	for ( int i=start; i < sp2.length()-1; ++i ) {
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx1, dy1, dz1 );
		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx2, dy2, dz2 );
		if ( line3DIntersectLine3D( x10,y10,z10, x20,y20,z20, dx1,dy1,dz1,dx2,dy2,dz2 ) ) return true;
	}
	return false;
}
bool JagGeo::line3DIntersectBox( const JagLine3D &line, double x0, double y0, double z0,
				double w, double d, double h, double nx, double ny, bool strict )
{
	return line3DIntersectBox( line.x1, line.y1, line.z1, line.x2, line.y2, line.z2,
										x0,y0,z0,w,d,h, nx, ny, false );
}


bool JagGeo::line3DIntersectBox(  double x10, double y10, double z10, 
								   double x20, double y20, double z20, 
									double x0, double y0, double z0,
									double w, double d, double h, 
									double nx, double ny, bool strict )
{
	if ( bound3DLineBoxDisjoint(  x10, y10, z10, x20, y20, z20, x0, y0, z0, w, d, h ) ) return false;
	if ( point3DWithinBox( x10, y10, z10, x0, y0, z0, w, d, h, nx, ny, false ) ) return true;
	if ( point3DWithinBox( x20, y20, z20, x0, y0, z0, w, d, h, nx, ny, false ) ) return true;

	JagLine3D line0;
	line0.x1 = x10; line0.y1 = y10; line0.z1 = z10;
	line0.x2 = x20; line0.y2 = y20; line0.z2 = z20;

	JagRectangle3D rect[6];
	surfacesOfBox( w,d,h, rect); // local rectangles
	for ( int i=0; i < 6; ++i ) {
		rect[i].transform( x0, y0, z0, nx, ny ); // global 
		if ( line3DIntersectRectangle3D( line0, rect[i] ) ) return true;
	}
	return false;
}

bool JagGeo::line3DIntersectSphere(  double x10, double y10, double z10, double x20, double y20, double z20, 
 									  double x, double y, double z, double r, bool strict )
{
	if ( bound3DLineBoxDisjoint( x10, y10, z10, x20, y20, z20, x, y, z, r, r, r ) ) return false;
	if ( point3DWithinSphere( x10,y10,z10, x, y, z, r, false ) ) return true;
	if ( point3DWithinSphere( x20,y20,z20, x, y, z, r, false ) ) return true;

	int rel = relationLine3DSphere( x10,y10,z10,x20,y20,z20, x,y,z, r);
	if ( rel > 0 ) return true;
	return false;
}


bool JagGeo::line3DIntersectEllipsoid(  double x10, double y10, double z10, double x20, double y20, double z20,
								 double x0, double y0, double z0,
								double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DLineBoxDisjoint(  x10, y10, z10, x20, y20, z20, x0, y0, z0, w, d, h ) ) return false;
	if ( point3DWithinEllipsoid( x10,y10,z10, x0, y0, z0, w,d,h, nx,ny, false ) ) return true;
	if ( point3DWithinEllipsoid( x20,y20,z20, x0, y0, z0, w,d,h, nx,ny, false ) ) return true;
	int rel = relationLine3DEllipsoid( x10, y10, z10, x20, y20, z20,
                                       x0, y0, z0, w, d, h, nx, ny );
	if ( rel > 0 ) return true;
	return false;
}

bool JagGeo::line3DIntersectCone(  double x10, double y10, double z10, double x20, double y20, double z20,
								 double x0, double y0, double z0,
								double r, double h, double nx, double ny, bool strict )
{
	if ( bound3DLineBoxDisjoint(  x10, y10, z10, x20, y20, z20, x0, y0, z0, r, r, h ) ) return false;
	if ( point3DWithinCone( x10,y10,z10, x0, y0, z0, r,h, nx,ny, false ) ) return true;
	if ( point3DWithinCone( x20,y20,z20, x0, y0, z0, r,h, nx,ny, false ) ) return true;
	int rel = relationLine3DCone( x10, y10, z10, x20, y20, z20, x0, y0, z0, r, h, nx, ny );
	if ( rel > 0 ) return true;
	return false;
}

bool JagGeo::line3DIntersectCylinder(  double x10, double y10, double z10, double x20, double y20, double z20,
								 double x0, double y0, double z0,
								double a, double b, double c, double nx, double ny, bool strict )
{
	if ( bound3DLineBoxDisjoint(  x10, y10, z10, x20, y20, z20, x0, y0, z0, a, b, c ) ) return false;
	if ( point3DWithinCylinder( x10,y10,z10, x0, y0, z0, a,c, nx,ny, false ) ) return true;
	if ( point3DWithinCylinder( x20,y20,z20, x0, y0, z0, a,c, nx,ny, false ) ) return true;
	int rel = relationLine3DCylinder( x10, y10, z10, x20, y20, z20, x0, y0, z0, a,b,c, nx, ny );
	if ( rel > 0 ) return true;
	return false;
}


// lineString3D intersect
bool JagGeo::lineString3DIntersectLineString3D( const Jstr &mk1, const JagStrSplit &sp1,
			                                const Jstr &mk2, const JagStrSplit &sp2,
											bool doRes, JagVector<JagPoint3D> &retVec )
{
	// sweepline algo
	int start1 = JAG_SP_START;
	int start2 = JAG_SP_START;

	double dx1, dy1, dz1, dx2, dy2, dz2, t;
	const char *str;
	char *p; int i;
	int totlen = sp1.length()-start1 + sp2.length() - start2;
	JagSortPoint3D *points = new JagSortPoint3D[2*totlen];
	int j = 0;
	int rc;
	bool found = false;
	for ( i=start1; i < sp1.length()-1; ++i ) {
		str = sp1[i].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx1, dy1, dz1 );

		str = sp1[i+1].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx2, dy2, dz2 );

		if ( jagEQ(dx1, dx2)) {
			dx2 += 0.000001f;
		}

		if ( dx1 > dx2 ) {
			// swap
			t = dx1; dx1 = dx2; dx2 = t; 
			t = dy1; dy1 = dy2; dy2 = t; 
		}

		points[j].x1 = dx1; points[j].y1 = dy1; points[j].z1 = dz1;
		points[j].x2 = dx2; points[j].y2 = dy2; points[j].z2 = dz2;
		points[j].end = JAG_LEFT;
		points[j].color = JAG_RED;
		++j;

		points[j].x1 = dx1; points[j].y1 = dy1; points[j].z1 = dz1;
		points[j].x2 = dx2; points[j].y2 = dy2; points[j].z2 = dz2;
		points[j].end = JAG_RIGHT;
		points[j].color = JAG_RED;
		++j;
	}

	for ( i=start2; i < sp2.length()-1; ++i ) {
		str = sp2[i].c_str();

		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx1, dy1, dz1 );

		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx2, dy2, dz2 );

		if ( jagEQ(dx1, dx2) )  {
			dx2 += 0.000001f;
		}

		if ( dx1 > dx2 ) {
			// swap
			t = dx1; dx1 = dx2; dx2 = t; 
			t = dy1; dy1 = dy2; dy2 = t; 
		}

		points[j].x1 = dx1; points[j].y1 = dy1; points[j].z1 = dz1;
		points[j].x2 = dx2; points[j].y2 = dy2; points[j].z2 = dz2;
		points[j].end = JAG_LEFT;
		points[j].color = JAG_BLUE;
		++j;

		points[j].x1 = dx1; points[j].y1 = dy1; points[j].z1 = dz1;
		points[j].x2 = dx2; points[j].y2 = dy2; points[j].z2 = dz2;
		points[j].end = JAG_RIGHT;
		points[j].color = JAG_BLUE;
		++j;

	}

	int len = j;
	rc = inlineQuickSort<JagSortPoint3D>( points, len );
	if ( rc ) {
		//d("s7732 sortIntersectLinePoints rc=%d retur true intersect\n", rc );
		// return true;
	}

	JagArray<JagLineSeg3DPair> *jarr = new JagArray<JagLineSeg3DPair>();
	const JagLineSeg3DPair *above;
	const JagLineSeg3DPair *below;
	JagLineSeg3DPair seg; seg.value = '1';
	for ( int i=0; i < len; ++i ) {
		seg.key.x1 =  points[i].x1; seg.key.y1 =  points[i].y1; seg.key.z1 =  points[i].z1;
		seg.key.x2 =  points[i].x2; seg.key.y2 =  points[i].y2; seg.key.z2 =  points[i].z2;
		seg.color = points[i].color;
		//d("s0088 seg print:\n" );
		//seg.print();

		if ( JAG_LEFT == points[i].end ) {
			jarr->insert( seg );
			above = jarr->getSucc( seg );
			below = jarr->getPred( seg );
			/***
			if ( above ) {
				d("s7781 above print: \n" );
				above->print();
			}
			if ( below ) {
				d("s7681 below print: \n" );
				below->print();
			}
			***/

			if ( above && *above == JagLineSeg3DPair::NULLVALUE ) {
				d("s6251 above is NULLVALUE abort\n" );
				//abort();
				above = NULL;
			}

			if ( below && *below == JagLineSeg3DPair::NULLVALUE ) {
				d("s2598 below is NULLVALUE abort\n" );
				//abort();
				below = NULL;
			}

			if ( above && below ) {
				if ( above->color != below->color 
				     && line3DIntersectLine3D( above->key.x1,above->key.y1,above->key.z1, 
											   above->key.x2,above->key.y2, above->key.z2,
									           below->key.x1,below->key.y1,below->key.z1,
											   below->key.x2,below->key.y2, below->key.z2 ) ) {
					d("s7440 left above below intersect\n" );
					if ( doRes ) {
						found = true;
						appendLine3DLine3DIntersection( above->key.x1,above->key.y1,above->key.z1, 
											above->key.x2,above->key.y2, above->key.z2,
											below->key.x1,below->key.y1,below->key.z1,
											 below->key.x2,below->key.y2, below->key.z2, retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			} else if ( above ) {
				if ( above->color != seg.color 
				     && line3DIntersectLine3D( above->key.x1,above->key.y1,above->key.z1,
					 						   above->key.x2,above->key.y2,above->key.z2,
									    	   seg.key.x1,seg.key.y1,seg.key.z1,
											   seg.key.x2,seg.key.y2,seg.key.z2 ) ) {
					d("s7341 left above seg intersect\n" );
					if ( doRes ) {
						found = true;
						appendLine3DLine3DIntersection( above->key.x1,above->key.y1,above->key.z1, 
											above->key.x2,above->key.y2, above->key.z2,
											seg.key.x1,seg.key.y1,seg.key.z1,
											seg.key.x2,seg.key.y2,seg.key.z2, retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			} else if ( below ) {
				if ( below->color != seg.color 
				    && line3DIntersectLine3D( below->key.x1,below->key.y1,below->key.z1,
											  below->key.x2,below->key.y2,below->key.z2,
									    	  seg.key.x1,seg.key.y1,seg.key.z1,
											  seg.key.x2,seg.key.y2,seg.key.z2 ) ) {
					d("s7611 left below seg intersect\n" );
					if ( doRes ) {
						found = true;
						appendLine3DLine3DIntersection( below->key.x1,below->key.y1,below->key.z1,
											below->key.x2,below->key.y2,below->key.z2,
											seg.key.x1,seg.key.y1,seg.key.z1,
											seg.key.x2,seg.key.y2,seg.key.z2, retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			}
		} else {
			// right end
			above = jarr->getSucc( seg );
			below = jarr->getPred( seg );
			if ( above && *above == JagLineSeg3DPair::NULLVALUE ) {
				d("s7211 above is NULLVALUE abort\n" );
				//abort();
				above = NULL;
			}

			if ( below && *below == JagLineSeg3DPair::NULLVALUE ) {
				#if 0
				d("7258 below is NULLVALUE abort\n" );
				jarr->printKey();
				//seg.println();
				//abort();
				#endif
				below = NULL;
			}

			if ( above && below ) {
				if ( above->color != below->color 
					 && line3DIntersectLine3D( above->key.x1,above->key.y1,above->key.z1,
					 						   above->key.x2,above->key.y2,above->key.z2,
									    	   below->key.x1,below->key.y1,below->key.z1,
											   below->key.x2,below->key.y2,below->key.z2 ) ) {
					d("s7043 rightend above below intersect\n" );
					if ( doRes ) {
						found = true;
						appendLine3DLine3DIntersection( above->key.x1,above->key.y1,above->key.z1,
											 above->key.x2,above->key.y2,above->key.z2,
											 below->key.x1,below->key.y1,below->key.z1,
											 below->key.x2,below->key.y2,below->key.z2,
											 retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			} else if ( above ) {
				if ( above->color != seg.color 
					 && line3DIntersectLine3D( above->key.x1,above->key.y1,above->key.z1,
					 						   above->key.x2,above->key.y2,above->key.z2,
									    	   seg.key.x1,seg.key.y1,seg.key.z1,
											   seg.key.x2,seg.key.y2,seg.key.z2 ) ) {
					d("s7710 rightend above seg intersect\n" );
					if ( doRes ) {
						found = true;
						appendLine3DLine3DIntersection( above->key.x1,above->key.y1,above->key.z1,
											 above->key.x2,above->key.y2,above->key.z2,
											 seg.key.x1,seg.key.y1,seg.key.z1,
											 seg.key.x2,seg.key.y2,seg.key.z2,
											 retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			} else if ( below ) {
				if ( below->color != seg.color 
					 && line3DIntersectLine3D( below->key.x1,below->key.y1,below->key.z1,
					 						   below->key.x2,below->key.y2,below->key.z2,
									    	   seg.key.x1,seg.key.y1,seg.key.z1,
											   seg.key.x2,seg.key.y2,seg.key.z2) ) {
					d("s4740 rightend below seg intersect\n" );
					if ( doRes ) {
						found = true;
						appendLine3DLine3DIntersection( below->key.x1,below->key.y1,below->key.z1,
											 below->key.x2,below->key.y2,below->key.z2,
											 seg.key.x1,seg.key.y1,seg.key.z1,
											 seg.key.x2,seg.key.y2,seg.key.z2,
											 retVec );
					} else {
						delete [] points;
						delete jarr;
						return true;
					}
				}
			}
			jarr->remove( seg );
		}
	}

	delete [] points;
	delete jarr;

	d("s7810 no intersect\n");
	return found;
}

bool JagGeo::lineString3DIntersectBox(  const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0,
									double w, double dd, double h, 
									double nx, double ny, bool strict )
{
	// d("s6752 lineString3DIntersectBox ..\n" );
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
		/***
		d("s6753 sp1: bbx=%f bby=%f bbz=%f    brx=%f bry=%f brz=%f\n",
				bbx, bby, bbz, brx, bry, brz );
		d("s6753 x0=%f y0=%f z0=%f w=%f d=%f h=%f\n", x0, y0, z0,  w, d, h );
		***/

        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  w, dd, h ) ) {
			// d("s6750 bound3DDisjoint return false\n" );
            return false;
        }
    }

	JagRectangle3D rect[6];
	surfacesOfBox( w,dd,h, rect); // local rectangles
	for ( int i=0; i < 6; ++i ) {
		rect[i].transform( x0, y0, z0, nx, ny ); // global 
	}

    double dx1, dy1, dz1, dx2, dy2, dz2;
	JagLine3D line3d;
    const char *str;
    char *p; int i;
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinBox( dx1, dy1, dz1,  x0, y0, z0, w,dd,h,nx,ny, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		line3d.x1=dx1; line3d.y1=dy1; line3d.z1=dz1;
		line3d.x2=dx2; line3d.y2=dy2; line3d.z2=dz2;
		for ( int j=0; j<6; ++j ) {
			if ( line3DIntersectRectangle3D( line3d, rect[i] ) ) return true;
		}
    }

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 2 ) {
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinBox( dx1, dy1, dz1,  x0, y0, z0, w,dd,h, nx,ny, strict ) ) return true;
	}

	d("s6750 bound3DDisjoint return false\n" );
	return false;
}

bool JagGeo::lineString3DIntersectSphere(  const Jstr &mk1, const JagStrSplit &sp1,
 									  double x0, double y0, double z0, double r, bool strict )
{
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, r ) ) {
            return false;
        }
    }

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinSphere( dx1, dy1, dz1,  x0, y0, z0, r, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		if ( line3DIntersectSphere(  dx1,dy1,dz1,dx2,dy2,dz2, x0, y0, z0, r, strict ) ) return true;
    }

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 2 ) {
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinSphere( dx1, dy1, dz1,  x0, y0, z0, r, strict ) ) return true;
	}

	return false;
}


bool JagGeo::lineString3DIntersectEllipsoid(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double w, double d, double h, double nx, double ny, bool strict )
{
	/****
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DLineBoxDisjoint(  x10, y10, z10, x20, y20, z20, x0, y0, z0, w, d, h ) ) return false;
	if ( point3DWithinEllipsoid( x10,y10,z10, x0, y0, z0, w,d,h, nx,ny, false ) ) return true;
	if ( point3DWithinEllipsoid( x20,y20,z20, x0, y0, z0, w,d,h, nx,ny, false ) ) return true;
	int rel = relationLine3DEllipsoid( x10, y10, z10, x20, y20, z20,
                                       x0, y0, z0, w, d, h, nx, ny );
	if ( rel > 0 ) return true;
	return false;
	*****/
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  w, d, h ) ) {
            return false;
        }
    }

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinEllipsoid( dx1, dy1, dz1,  x0, y0, z0, w,d,h,nx,ny, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		if ( line3DIntersectEllipsoid(  dx1,dy1,dz1,dx2,dy2,dz2, x0, y0, z0, w,d,h,nx,ny, strict ) ) return true;
    }

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 2 ) {
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinEllipsoid( dx1, dy1, dz1,  x0, y0, z0, w,d,h,nx,ny, strict ) ) return true;
	}

	return false;
}

bool JagGeo::lineString3DIntersectCone(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double r, double h, double nx, double ny, bool strict )
{
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, h ) ) {
            return false;
        }
    }

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
    for ( int i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinCone( dx1, dy1, dz1,  x0, y0, z0, r,h,nx,ny, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		if ( line3DIntersectCone(  dx1,dy1,dz1,dx2,dy2,dz2, x0, y0, z0, r,h, nx,ny, strict ) ) return true;
    }

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 2 ) {
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinCone( dx1, dy1, dz1,  x0, y0, z0, r,h,nx,ny, strict ) ) return true;
	}
	return false;
}

bool JagGeo::lineString3DIntersectCylinder(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double a, double b, double c, double nx, double ny, bool strict )
{
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  a, b, c ) ) {
            return false;
        }
    }

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinCylinder( dx1, dy1, dz1,  x0, y0, z0, a,c,nx,ny, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		if ( line3DIntersectCylinder(  dx1,dy1,dz1,dx2,dy2,dz2, x0, y0, z0, a,b,c, nx,ny, strict ) ) return true;
    }

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 2 ) {
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinCylinder( dx1, dy1, dz1,  x0, y0, z0, a,c,nx,ny, strict ) ) return true;
	}
	return false;
}

bool JagGeo::lineString3DIntersectTriangle3D(  const Jstr &mk1, const JagStrSplit &sp1,
											   const Jstr &mk2, const JagStrSplit &sp2 )
{
	if ( sp2.length() < 9 ) return false;
	double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
	double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
	double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
	double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
	double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
	double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
	double x3 = jagatof( sp2[JAG_SP_START+6].c_str() ); 
	double y3 = jagatof( sp2[JAG_SP_START+7].c_str() ); 
	double z3 = jagatof( sp2[JAG_SP_START+8].c_str() ); 

	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
		double x0, y0, z0, Rx, Ry, Rz;
		triangle3DRegion( x1, y1, z1, x2, y2, z2, x3, y3, z3, x0, y0, z0, Rx, Ry, Rz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  Rx, Ry, Rz ) ) {
            return false;
        }
    }

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
	JagLine3D line3d;
	JagPoint3D atPoint;
	JagPoint3D p1(x1,y1,z1);
	JagPoint3D p2(x2,y2,z2);
	JagPoint3D p3(x3,y3,z3);
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );

		//if ( point3DWithinCylinder( dx1, dy1, dz1,  x0, y0, z0, a,c,nx,ny, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		//if ( line3DIntersectCylinder(  dx1,dy1,dz1,dx2,dy2,dz2, x0, y0, z0, a,b,c, nx,ny, strict ) ) return true;
		line3d.x1 = dx1; line3d.y1 = dy1; line3d.z1 = dz1;
		line3d.x2 = dx2; line3d.y2 = dy2; line3d.z2 = dz2;
		if ( line3DIntersectTriangle3D( line3d, p1, p2, p3, atPoint ) ) { return true; }
    }

	return false;
}

bool JagGeo::lineString3DIntersectSquare3D(  const Jstr &mk1, const JagStrSplit &sp1,
											   const Jstr &mk2, const JagStrSplit &sp2 )
{
	if ( sp2.length() < 4 ) return false;
	double px0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp2, JAG_SP_START+4);
	double ny0 = safeget(sp2, JAG_SP_START+5);

	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  px0, py0, pz0,  pr0, pr0, pr0 ) ) {
            return false;
        }
    }

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
	JagLine3D line3d;
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );

        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );

		line3d.x1 = dx1; line3d.y1 = dy1; line3d.z1 = dz1;
		line3d.x2 = dx2; line3d.y2 = dy2; line3d.z2 = dz2;
		if ( line3DIntersectRectangle3D( line3d, px0, py0, pz0, pr0, pr0, nx0, ny0 ) ) {
			return true;
		}
    }

	return false;
}

bool JagGeo::lineString3DIntersectRectangle3D(  const Jstr &mk1, const JagStrSplit &sp1,
											   const Jstr &mk2, const JagStrSplit &sp2 )
{
	if ( sp2.length() < 4 ) return false;
	double px0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp2, JAG_SP_START+4);
	double ny0 = safeget(sp2, JAG_SP_START+5);

	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  px0, py0, pz0,  a0, a0, b0 ) ) {
            return false;
        }
    }

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
	JagLine3D line3d;
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );

        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );

		line3d.x1 = dx1; line3d.y1 = dy1; line3d.z1 = dz1;
		line3d.x2 = dx2; line3d.y2 = dy2; line3d.z2 = dz2;
		if ( line3DIntersectRectangle3D( line3d, px0, py0, pz0, a0, b0, nx0, ny0 ) ) {
			return true;
		}
    }

	return false;
}


// Polygon3D intersect
bool JagGeo::polygon3DIntersectLineString3D( const Jstr &mk1, const JagStrSplit &sp1,
			                                const Jstr &mk2, const JagStrSplit &sp2 )
{
	// sweepline algo
	int start1 = JAG_SP_START;
	int start2 = JAG_SP_START;
	//d("s8122 polygon3DIntersectLineString3D sp1: sp2:\n" );
	//sp1.print();
	//sp2.print();

	double dx1, dy1, dz1, dx2, dy2, dz2, t;
	const char *str;
	char *p; int i;
	int rc;
	int totlen = sp1.length()-start1 + sp2.length() - start2;
	JagSortPoint3D *points = new JagSortPoint3D[2*totlen];

	int j = 0;
	for ( i=start1; i < sp1.length()-1; ++i ) {
		str = sp1[i].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx1, dy1, dz1 );

		str = sp1[i+1].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx2, dy2, dz2 );

		if ( jagEQ(dx1, dx2)) {
			dx2 += 0.000001f;
		}

		if ( dx1 > dx2 ) {
			// swap
			t = dx1; dx1 = dx2; dx2 = t; 
			t = dy1; dy1 = dy2; dy2 = t; 
		}

		points[j].x1 = dx1; points[j].y1 = dy1; points[j].z1 = dz1;
		points[j].x2 = dx2; points[j].y2 = dy2; points[j].z2 = dz2;
		points[j].end = JAG_LEFT;
		points[j].color = JAG_RED;
		++j;

		points[j].x1 = dx1; points[j].y1 = dy1; points[j].z1 = dz1;
		points[j].x2 = dx2; points[j].y2 = dy2; points[j].z2 = dz2;
		points[j].end = JAG_RIGHT;
		points[j].color = JAG_RED;
		++j;
	}

	/******
	JagPolygon pgon;
	double t;
	rc = addPolygonData( pgon, sp1, false ); // all polygons
	if ( rc < 0 ) return false;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &linestr = pgon.linestr[i];
		for ( int j=0; j < linestr.size(); ++j ) {
			dx1 = linestr[j].x1; dy1 = linestr[j].y1;
			dx2 = linestr[j].x2; dy2 = linestr[j].y2;
			if ( jagEQ(dx1, dx2)) { dx2 += 0.000001f; }
			if ( dx1 > dx2 ) {
				t = dx1; dx1 = dx2; dx2 = t; 
				t = dy1; dy1 = dy2; dy2 = t; 
			}
		}
	}
	*******/


	for ( i=start2; i < sp2.length()-1; ++i ) {
		str = sp2[i].c_str();

		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx1, dy1, dz1 );

		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx2, dy2, dz2 );

		if ( jagEQ(dx1, dx2) )  {
			dx2 += 0.000001f;
		}

		if ( dx1 > dx2 ) {
			// swap
			t = dx1; dx1 = dx2; dx2 = t; 
			t = dy1; dy1 = dy2; dy2 = t; 
		}

		points[j].x1 = dx1; points[j].y1 = dy1; points[j].z1 = dz1;
		points[j].x2 = dx2; points[j].y2 = dy2; points[j].z2 = dz2;
		points[j].end = JAG_LEFT;
		points[j].color = JAG_BLUE;
		++j;

		points[j].x1 = dx1; points[j].y1 = dy1; points[j].z1 = dz1;
		points[j].x2 = dx2; points[j].y2 = dy2; points[j].z2 = dz2;
		points[j].end = JAG_RIGHT;
		points[j].color = JAG_BLUE;
		++j;

	}

	int len = j;
	rc = inlineQuickSort<JagSortPoint3D>( points, len );
	if ( rc ) {
		//d("s7732 sortIntersectLinePoints rc=%d retur true intersect\n", rc );
		// return true;
	}

	JagArray<JagLineSeg3DPair> *jarr = new JagArray<JagLineSeg3DPair>();
	const JagLineSeg3DPair *above;
	const JagLineSeg3DPair *below;
	JagLineSeg3DPair seg; seg.value = '1';
	for ( int i=0; i < len; ++i ) {
		seg.key.x1 =  points[i].x1; seg.key.y1 =  points[i].y1; seg.key.z1 =  points[i].z1;
		seg.key.x2 =  points[i].x2; seg.key.y2 =  points[i].y2; seg.key.z2 =  points[i].z2;
		seg.color = points[i].color;
		//d("s0088 seg print:\n" );
		//seg.print();

		if ( JAG_LEFT == points[i].end ) {
			jarr->insert( seg );
			above = jarr->getSucc( seg );
			below = jarr->getPred( seg );
			/***
			if ( above ) {
				d("s7781 above print: \n" );
				above->print();
			}
			if ( below ) {
				d("s7681 below print: \n" );
				below->print();
			}
			***/

			if ( above && *above == JagLineSeg3DPair::NULLVALUE ) {
				d("s6251 above is NULLVALUE abort\n" );
				//abort();
				above = NULL;
			}

			if ( below && *below == JagLineSeg3DPair::NULLVALUE ) {
				d("s2598 below is NULLVALUE abort\n" );
				//abort();
				below = NULL;
			}

			if ( above && below ) {
				if ( above->color != below->color 
				     && line3DIntersectLine3D( above->key.x1,above->key.y1,above->key.z1, 
											   above->key.x2,above->key.y2, above->key.z2,
									           below->key.x1,below->key.y1,below->key.z1,
											   below->key.x2,below->key.y2, below->key.z2 ) ) {
					d("s7440 left above below intersect\n" );
					return true;
				}
			} else if ( above ) {
				if ( above->color != seg.color 
				     && line3DIntersectLine3D( above->key.x1,above->key.y1,above->key.z1,
					 						   above->key.x2,above->key.y2,above->key.z2,
									    	   seg.key.x1,seg.key.y1,seg.key.z1,
											   seg.key.x2,seg.key.y2,seg.key.z2 ) ) {
					d("s7341 left above seg intersect\n" );
					return true;
				}
			} else if ( below ) {
				if ( below->color != seg.color 
				    && line3DIntersectLine3D( below->key.x1,below->key.y1,below->key.z1,
											  below->key.x2,below->key.y2,below->key.z2,
									    	  seg.key.x1,seg.key.y1,seg.key.z1,
											  seg.key.x2,seg.key.y2,seg.key.z2 ) ) {
					d("s7611 left below seg intersect\n" );
					return true;
				}
			}
		} else {
			// right end
			above = jarr->getSucc( seg );
			below = jarr->getPred( seg );
			if ( above && *above == JagLineSeg3DPair::NULLVALUE ) {
				d("s7211 above is NULLVALUE abort\n" );
				//abort();
				above = NULL;
			}

			if ( below && *below == JagLineSeg3DPair::NULLVALUE ) {
				#if 0
				d("7258 below is NULLVALUE abort\n" );
				jarr->printKey();
				//seg.println();
				//abort();
				#endif
				below = NULL;
			}

			if ( above && below ) {
				if ( above->color != below->color 
					 && line3DIntersectLine3D( above->key.x1,above->key.y1,above->key.z1,
					 						   above->key.x2,above->key.y2,above->key.z2,
									    	   below->key.x1,below->key.y1,below->key.z1,
											   below->key.x2,below->key.y2,below->key.z2 ) ) {
					d("s7043 rightend above below intersect\n" );
					return true;
				}
			} else if ( above ) {
				if ( above->color != seg.color 
					 && line3DIntersectLine3D( above->key.x1,above->key.y1,above->key.z1,
					 						   above->key.x2,above->key.y2,above->key.z2,
									    	   seg.key.x1,seg.key.y1,seg.key.z1,
											   seg.key.x2,seg.key.y2,seg.key.z2 ) ) {
					d("s7710 rightend above seg intersect\n" );
					return true;
				}
			} else if ( below ) {
				if ( below->color != seg.color 
					 && line3DIntersectLine3D( below->key.x1,below->key.y1,below->key.z1,
					 						   below->key.x2,below->key.y2,below->key.z2,
									    	   seg.key.x1,seg.key.y1,seg.key.z1,
											   seg.key.x2,seg.key.y2,seg.key.z2) ) {
					d("s4740 rightend below seg intersect\n" );
					return true;
				}
			}
			jarr->remove( seg );
		}
	}

	delete [] points;
	delete jarr;

	d("s7810 no intersect\n");
	return false;
}

bool JagGeo::polygon3DIntersectBox(  const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0,
									double w, double dd, double h, 
									double nx, double ny, bool strict )
{
	// d("s6752 polygon3DIntersectBox ..\n" );
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
		/***
		d("s6753 sp1: bbx=%f bby=%f bbz=%f    brx=%f bry=%f brz=%f\n",
				bbx, bby, bbz, brx, bry, brz );
		d("s6753 x0=%f y0=%f z0=%f w=%f d=%f h=%f\n", x0, y0, z0,  w, d, h );
		***/

        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  w, dd, h ) ) {
			d("s6750 bound3DDisjoint return false\n" );
            return false;
        }
    }

	JagRectangle3D rect[6];
	surfacesOfBox( w,dd,h, rect); // local rectangles
	for ( int i=0; i < 6; ++i ) {
		rect[i].transform( x0, y0, z0, nx, ny ); // global 
	}

    double dx1, dy1, dz1, dx2, dy2, dz2;
	JagLine3D line3d;
    const char *str;
    char *p; int i;
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinBox( dx1, dy1, dz1,  x0, y0, z0, w,dd,h,nx,ny, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		line3d.x1=dx1; line3d.y1=dy1; line3d.z1=dz1;
		line3d.x2=dx2; line3d.y2=dy2; line3d.z2=dz2;
		for ( int j=0; j<6; ++j ) {
			if ( line3DIntersectRectangle3D( line3d, rect[i] ) ) return true;
		}
    }

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 2 ) {
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinBox( dx1, dy1, dz1,  x0, y0, z0, w,dd,h, nx,ny, strict ) ) return true;
	}

	d("s6750 bound3DDisjoint return false\n" );
	return false;
}

bool JagGeo::polygon3DIntersectSphere(  const Jstr &mk1, const JagStrSplit &sp1,
 									  double x0, double y0, double z0, double r, bool strict )
{
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, r ) ) {
            return false;
        }
    }

	//d("s9381 polygon3DIntersectSphere sp1:\n" );
	//sp1.print();

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinSphere( dx1, dy1, dz1,  x0, y0, z0, r, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		if ( line3DIntersectSphere(  dx1,dy1,dz1,dx2,dy2,dz2, x0, y0, z0, r, strict ) ) return true;
    }

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 2 ) {
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinSphere( dx1, dy1, dz1,  x0, y0, z0, r, strict ) ) return true;
	}

	return false;
}


bool JagGeo::polygon3DIntersectEllipsoid(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double w, double d, double h, double nx, double ny, bool strict )
{
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  w, d, h ) ) {
            return false;
        }
    }

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinEllipsoid( dx1, dy1, dz1,  x0, y0, z0, w,d,h,nx,ny, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		if ( line3DIntersectEllipsoid(  dx1,dy1,dz1,dx2,dy2,dz2, x0, y0, z0, w,d,h,nx,ny, strict ) ) return true;
    }

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 2 ) {
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinEllipsoid( dx1, dy1, dz1,  x0, y0, z0, w,d,h,nx,ny, strict ) ) return true;
	}

	return false;
}

bool JagGeo::polygon3DIntersectCone(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double r, double h, double nx, double ny, bool strict )
{
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  r, r, h ) ) {
            return false;
        }
    }

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
    for ( int i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinCone( dx1, dy1, dz1,  x0, y0, z0, r,h,nx,ny, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		if ( line3DIntersectCone(  dx1,dy1,dz1,dx2,dy2,dz2, x0, y0, z0, r,h, nx,ny, strict ) ) return true;
    }

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 2 ) {
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinCone( dx1, dy1, dz1,  x0, y0, z0, r,h,nx,ny, strict ) ) return true;
	}
	return false;
}

bool JagGeo::polygon3DIntersectCylinder(  const Jstr &mk1, const JagStrSplit &sp1,
								 double x0, double y0, double z0,
								double a, double b, double c, double nx, double ny, bool strict )
{
	int start = JAG_SP_START;
    if ( mk1 == JAG_OJAG ) {
        double bbx, bby, bbz, brx, bry, brz;
        boundingBox3DRegion(sp1[JAG_SP_START+0], bbx, bby, bbz, brx, bry, brz );
        if ( bound3DDisjoint( bbx, bby, bbz, brx, bry, brz,  x0, y0, z0,  a, b, c ) ) {
            return false;
        }
    }

    double dx1, dy1, dz1, dx2, dy2, dz2;
    const char *str;
    char *p; int i;
    for ( i=start; i < sp1.length()-1; ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinCylinder( dx1, dy1, dz1,  x0, y0, z0, a,c,nx,ny, strict ) ) return true;
        str = sp1[i+1].c_str();
        if ( strchrnum( str, ':') < 2 ) continue;
        get3double(str, p, ':', dx2, dy2, dz2 );
		if ( line3DIntersectCylinder(  dx1,dy1,dz1,dx2,dy2,dz2, x0, y0, z0, a,b,c, nx,ny, strict ) ) return true;
    }

    str = sp1[i].c_str();
    if ( strchrnum( str, ':') >= 2 ) {
        get3double(str, p, ':', dx1, dy1, dz1 );
		if ( point3DWithinCylinder( dx1, dy1, dz1,  x0, y0, z0, a,c,nx,ny, strict ) ) return true;
	}
	return false;
}


// 2D linestring
bool JagGeo::doLineStringIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
								    const Jstr &mk2, const Jstr &colType2, 
								    int srid2, const JagStrSplit &sp2, bool strict )
{
	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return lineStringIntersectTriangle( mk1, sp1, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		JagVector<JagPoint2D> vec;
		return lineStringIntersectLineString( mk1, sp1, mk2, sp2, false, vec );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return lineStringIntersectRectangle( mk1, sp1, x0, y0, a,a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineStringIntersectRectangle( mk1, sp1, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return lineStringIntersectEllipse( mk1, sp1, x0, y0, r,r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineStringIntersectEllipse( mk1, sp1, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return polygonIntersectLineString( mk2, sp2, mk1, sp1 );
	}
	return false;
}

bool JagGeo::doLineString3DIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
									  const Jstr &mk2, const Jstr &colType2, 
									  int srid2, const JagStrSplit &sp2, bool strict )
{
	d("s8761 doLineString3DIntersect colType2=[%s]\n", colType2.c_str() );
	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		d("s0872 lineString3DIntersectBox ..\n" );
		return lineString3DIntersectBox( mk1, sp1, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		JagVector<JagPoint3D> vec;
		return lineString3DIntersectLineString3D( mk1, sp1, mk2, sp2, false, vec );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return lineString3DIntersectBox( mk1, sp1, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return lineString3DIntersectSphere( mk1, sp1, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return lineString3DIntersectEllipsoid( mk1, sp1, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return lineString3DIntersectCone( mk1, sp1, x0, y0, z0, r,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CYLINDER ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return lineString3DIntersectCylinder( mk1, sp1, x0, y0, z0, r,r,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		return lineString3DIntersectTriangle3D( mk1, sp1, mk2, sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE3D ) {
		return lineString3DIntersectSquare3D( mk1, sp1, mk2, sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		return lineString3DIntersectRectangle3D( mk1, sp1, mk2, sp2 );
	}
	return false;
}

double JagGeo::doPolygonArea( const Jstr &mk1, int srid1, const JagStrSplit &sp1 )
{
	int start = JAG_SP_START;
	double dx, dy;
	const char *str;
	char *p;
	double ar, area = 0.0;
	int ring = 0; // n-th ring in polygon
	if ( 0 == srid1 ) {
    	JagVector<std::pair<double,double>> vec;
    	for ( int i=start; i < sp1.length(); ++i ) {
    		if ( sp1[i] == "!" ) break; // should not happen
    		if ( sp1[i] == "|" ) {
    			ar = computePolygonArea( vec );
				if ( 0 == ring ) {
    				area = ar;
				} else {
    				area -= ar;
				}
				vec.clean();
				++ring;
			}
    		str = sp1[i].c_str();
    		if ( strchrnum( str, ':') < 1 ) continue;
    		get2double(str, p, ':', dx, dy );
    		vec.append( std::make_pair(dx,dy) );
    	}

		if ( vec.size() > 0 ) {
    		ar = computePolygonArea( vec );
			if ( 0 == ring ) {
    			area = ar;
			} else {
    			area -= ar;
			}
		}
	} else if ( JAG_GEO_WGS84 == srid1 ) {
		const Geodesic& geod = Geodesic::WGS84();
		PolygonArea poly(geod);
		int numPoints = 0;
		double perim;
    	for ( int i=start; i < sp1.length(); ++i ) {
    		if ( sp1[i] == "!" ) break;
    		if ( sp1[i] == "|" ) {
				poly.Compute( false, true, perim, ar );
				if ( 0 == ring ) {
    				area = ar;
				} else {
    				area -= ar;
				}
				poly.Clear();
				numPoints = 0;
			} 
    		str = sp1[i].c_str();
    		if ( strchrnum( str, ':') < 1 ) continue;
    		get2double(str, p, ':', dx, dy );
			poly.AddPoint( dx, dy );
			++numPoints;
    	}

		if ( numPoints > 0 ) {
			poly.Compute( false, true, perim, ar );
			if ( 0 == ring ) {
    			area = ar;
			} else {
    			area -= ar;
			}
		}
	} 
	return area;
}

double JagGeo::doPolygonPerimeter( const Jstr &mk1, int srid1, const JagStrSplit &sp1 )
{
	int start = JAG_SP_START;
	double dx, dy;
	const char *str;
	char *p;
	double ar, perim = 0.0;
	int ring = 0; // n-th ring in polygon

   	JagVector<std::pair<double,double>> vec;
   	for ( int i=start; i < sp1.length(); ++i ) {
    		if ( sp1[i] == "!" ) break; // should not happen
    		if ( sp1[i] == "|" ) {
    			ar = computePolygonPerimeter( vec, srid1 );
				if ( 0 == ring ) {
    				perim = ar;
				} else {
    				perim += ar;
				}
				vec.clean();
				++ring;
			}
    		str = sp1[i].c_str();
    		if ( strchrnum( str, ':') < 1 ) continue;
    		get2double(str, p, ':', dx, dy );
    		vec.append( std::make_pair(dx,dy) );
   	}

	if ( vec.size() > 0 ) {
    		ar = computePolygonPerimeter( vec, srid1 );
			if ( 0 == ring ) {
    			perim = ar;
			} else {
    			perim += ar;
			}
	}

	return perim;
}

double JagGeo::doPolygon3DPerimeter( const Jstr &mk1, int srid1, const JagStrSplit &sp1 )
{
	int start = JAG_SP_START;
	double dx, dy, dz;
	const char *str;
	char *p;
	double ar, perim = 0.0;
	int ring = 0; // n-th ring in polygon

   	JagVector<JagPoint3D> vec;
   	for ( int i=start; i < sp1.length(); ++i ) {
    		if ( sp1[i] == "!" ) break; // should not happen
    		if ( sp1[i] == "|" ) {
    			ar = computePolygon3DPerimeter( vec, srid1 );
				if ( 0 == ring ) {
    				perim = ar;
				} else {
    				perim += ar;
				}
				vec.clean();
				++ring;
			}
    		str = sp1[i].c_str();
    		if ( strchrnum( str, ':') < 2 ) continue;
    		get3double(str, p, ':', dx, dy, dz );
    		vec.append( JagPoint3D(dx,dy,dz) );
   	}

	if ( vec.size() > 0 ) {
    		ar = computePolygon3DPerimeter( vec, srid1 );
			if ( 0 == ring ) {
    			perim = ar;
			} else {
    			perim += ar;
			}
	}

	return perim;
}

double JagGeo::computePolygonArea( const JagVector<std::pair<double,double>> &vec )
{
	int n = vec.size();
	double x[n+2];
	double y[n+2];
	for ( int i=0; i < n; ++i ) {
		x[i+1] = vec[i].first;
		y[i+1] = vec[i].second;
	}

    // e.g. n=3:
	// y0 y1 y2 y3 y4
	//    22 33 72 
	y[0] = y[n];
	y[n+1] = y[1];
	double area = 0.0;
	for ( int i = 1; i <= n; ++i ) {
		area += fabs( x[i] * ( y[i+1] - y[i-1]) )/2.0;
	}
	return area;
}

double JagGeo::computePolygonPerimeter( const JagVector<std::pair<double,double>> &vec, int srid )
{
	double perim = 0.0;
	for ( int i = 0; i < vec.size() -1 ; ++i ) {
		perim += distance(  vec[i].first, vec[i].second, vec[i+1].first, vec[i+1].second, srid );
	}
	return perim;
}

double JagGeo::computePolygon3DPerimeter( const JagVector<JagPoint3D> &vec, int srid )
{
	double perim = 0.0;
	for ( int i = 0; i < vec.size() -1 ; ++i ) {
		perim += distance(  vec[i].x, vec[i].y, vec[i].z, vec[i+1].x, vec[i+1].y, vec[i+1].z, srid );
	}
	return perim;
}


bool JagGeo::doPolygonIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
								    const Jstr &mk2, const Jstr &colType2, 
								    int srid2, const JagStrSplit &sp2, bool strict )
{
	//d("s2268 doPolygonIntersect ...\n" );

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return polygonIntersectTriangle( mk1, sp1, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		return polygonIntersectLine( mk1, sp1, x1, y1, x2, y2 );
	} else if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		//d("s2938 pointWithinPolygon x=%.2f y=%.2f\n", x, y );
		//sp1.print();
		return pointWithinPolygon( x, y, mk1, sp1, false );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return polygonIntersectLineString( mk1, sp1, mk2, sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return polygonIntersectRectangle( mk1, sp1, x0, y0, a,a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return polygonIntersectRectangle( mk1, sp1, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return polygonIntersectEllipse( mk1, sp1, x0, y0, r,r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return polygonIntersectEllipse( mk1, sp1, x0, y0, a, b, nx, strict );
	}
	return false;
}

bool JagGeo::doPolygon3DIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
									  const Jstr &mk2, const Jstr &colType2, 
									  int srid2, const JagStrSplit &sp2, bool strict )
{
	d("s8761 dopolygon3DIntersect colType2=[%s]\n", colType2.c_str() );
	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		d("s0872 polygon3DIntersectBox ..\n" );
		return polygon3DIntersectBox( mk1, sp1, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return polygon3DIntersectLineString3D( mk1, sp1, mk2, sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return polygon3DIntersectBox( mk1, sp1, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return polygon3DIntersectSphere( mk1, sp1, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return polygon3DIntersectEllipsoid( mk1, sp1, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return polygon3DIntersectCone( mk1, sp1, x0, y0, z0, r,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CYLINDER ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return polygon3DIntersectCylinder( mk1, sp1, x0, y0, z0, r,r,h, nx,ny, strict );
	}
	return false;
}

bool JagGeo::doMultiPolygonIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
								    const Jstr &mk2, const Jstr &colType2, 
								    int srid2, const JagStrSplit &sp2, bool strict )
{
	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return polygonIntersectTriangle( mk1, sp1, x1, y1, x2, y2, x3, y3, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return polygonIntersectLineString( mk1, sp1, mk2, sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return polygonIntersectRectangle( mk1, sp1, x0, y0, a,a, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return polygonIntersectRectangle( mk1, sp1, x0, y0, a, b, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return polygonIntersectEllipse( mk1, sp1, x0, y0, r,r, nx, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return polygonIntersectEllipse( mk1, sp1, x0, y0, a, b, nx, strict );
	}
	return false;
}

bool JagGeo::doMultiPolygon3DIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
									  const Jstr &mk2, const Jstr &colType2, 
									  int srid2, const JagStrSplit &sp2, bool strict )
{
	d("s8761 domultiPolygon3DIntersect colType2=[%s]\n", colType2.c_str() );

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		d("s0872 multiPolygon3DIntersectBox ..\n" );
		return polygon3DIntersectBox( mk1, sp1, x0, y0, z0, r,r,r, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return polygon3DIntersectLineString3D( mk1, sp1, mk2, sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return polygon3DIntersectBox( mk1, sp1, x0, y0, z0, w,d,h, nx, ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return polygon3DIntersectSphere( mk1, sp1, x, y, z, r, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return polygon3DIntersectEllipsoid( mk1, sp1, x0, y0, z0, w,d,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return polygon3DIntersectCone( mk1, sp1, x0, y0, z0, r,h, nx,ny, strict );
	} else if ( colType2 == JAG_C_COL_TYPE_CYLINDER ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return polygon3DIntersectCylinder( mk1, sp1, x0, y0, z0, r,r,h, nx,ny, strict );
	}
	return false;
}

///////////////////////////////////// triangle 3D /////////////////////////////////
bool JagGeo::triangle3DIntersectBox(  double x10, double y10, double z10, 
								   double x20, double y20, double z20, 
									double x30, double y30, double z30,
									double x0, double y0, double z0,
									double w, double d, double h, 
									double nx, double ny, bool strict )
{
	if ( ! validDirection(nx, ny) ) return false;
	double mxd = jagMax(w,d,h);
	if ( jagMax(x10, x20, x30) < x0-mxd ) return false;
	if ( jagMax(y10, y20, y30) < y0-mxd ) return false;
	if ( jagMax(z10, z20, z30) < z0-mxd ) return false;
	if ( jagMin(x10, x20, x30) > x0+mxd ) return false;
	if ( jagMin(y10, y20, y30) > y0+mxd ) return false;
	if ( jagMin(z10, z20, z30) > z0+mxd ) return false;

	if ( triangle3DWithinBox( x10,y10,z10, x20,y20,z20,  x30,y30,z30, x0,y0,z0, w,d,h, nx,ny, false ) ) {
		return true;
	}

	JagLine3D line[3];
	edgesOfTriangle3D( x10,y10,z10,x20,y20,z20, x30,y30,z30, line );
	for ( int i=0; i <3; ++i ) {
		if ( line3DIntersectBox( line[i], x0, y0, z0, w,d,h, nx, ny, false ) ) {
			return true;
		}
	}

	// box inside triangle
	// triangle as base plane; check z- coords of all box corners points. if all same side, then disjoint
	JagPoint3D corn[8];
	double nx0, ny0;
	triangle3DNormal(  x10,y10,z10, x20,y20,z20,  x30,y30,z30, nx0, ny0 );
	int up=0, down=0;
	for ( int i=0; i <8; ++i ) {
		corn[i].transform( x10, y10, z10, nx0, ny0 );
		if ( jagGE(corn[i].z, 0.0 ) ) ++up;
		else ++down;
	}
	if ( 0==up || 0==down ) return false;
	return true;
}


bool JagGeo::triangle3DIntersectEllipsoid(  double x10, double y10, double z10, double x20, double y20, double z20,
										double x30, double y30, double z30,
								 double x0, double y0, double z0,
								double w, double d, double h, double nx, double ny, bool strict )
{
	double mxd = jagMax(w,d,h);
	if ( jagMax(x10, x20, x30) < x0-mxd ) return false;
	if ( jagMax(y10, y20, y30) < y0-mxd ) return false;
	if ( jagMax(z10, z20, z30) < z0-mxd ) return false;
	if ( jagMin(x10, x20, x30) > x0+mxd ) return false;
	if ( jagMin(y10, y20, y30) > y0+mxd ) return false;
	if ( jagMin(z10, z20, z30) > z0+mxd ) return false;

	if ( triangle3DWithinEllipsoid( x10,y10,z10, x20,y20,z20,  x30,y30,z30, x0,y0,z0, w,d,h, nx,ny, false ) ) {
		return true;
	}

	// each edge of tri intersect ellipsoid
	JagLine3D line[3];
	edgesOfTriangle3D( x10,y10,z10,x20,y20,z20, x30,y30,z30, line );
	for ( int i=0; i <3; ++i ) {
		if ( line3DIntersectEllipsoid( line[i].x1, line[i].y1, line[i].z1, line[i].x2, line[i].y2, line[i].z2, 
										x0, y0, z0, w,d,h, nx, ny, false ) ) {
			return true;
		}
	}

	// ellipsoid inside triangle: surface sect
	JagPoint3D point[3];
	pointsOfTriangle3D(  x10,y10,z10,x20,y20,z20, x30,y30,z30, point );
	for ( int i=0; i <3; ++i ) {
		point[i].transform(x0,y0,z0, nx,ny  );
		// ellipsoid as normal
	}
	double A, B, C, D;
	triangle3DABCD( point[0].x, point[0].y, point[0].z,
					point[1].x, point[1].y, point[1].z,
					point[2].x, point[2].y, point[2].z, A, B, C, D );
	if ( planeIntersectNormalEllipsoid( A, B, C, D, w,d,h ) ) {
		return true;
	}

	return false;
}


bool JagGeo::triangle3DIntersectCone(  double x10, double y10, double z10, double x20, double y20, double z20,
										double x30, double y30, double z30,
								 double x0, double y0, double z0,
								double r, double h, double nx, double ny, bool strict )
{
	double mxd = jagmax(r,h);
	if ( jagMax(x10, x20, x30) < x0-mxd ) return false;
	if ( jagMax(y10, y20, y30) < y0-mxd ) return false;
	if ( jagMax(z10, z20, z30) < z0-mxd ) return false;
	if ( jagMin(x10, x20, x30) > x0+mxd ) return false;
	if ( jagMin(y10, y20, y30) > y0+mxd ) return false;
	if ( jagMin(z10, z20, z30) > z0+mxd ) return false;

	if ( triangle3DWithinCone( x10,y10,z10, x20,y20,z20,  x30,y30,z30, x0,y0,z0, r,h, nx,ny, false ) ) {
		return true;
	}

	// each edge of tri intersect ellipsoid
	JagLine3D line[3];
	edgesOfTriangle3D( x10,y10,z10,x20,y20,z20, x30,y30,z30, line );
	for ( int i=0; i <3; ++i ) {
		if ( line3DIntersectCone( line[i].x1, line[i].y1, line[i].z1, line[i].x2, line[i].y2, line[i].z2, 
										x0, y0, z0, r,h, nx, ny, false ) ) {
			return true;
		}
	}

	JagPoint3D point[3];
	pointsOfTriangle3D(  x10,y10,z10,x20,y20,z20, x30,y30,z30, point );
	for ( int i=0; i <3; ++i ) {
		point[i].transform(x0,y0,z0, nx,ny  );
		// ellipsoid as normal
	}
	double A, B, C, D;
	triangle3DABCD( point[0].x, point[0].y, point[0].z, point[1].x, point[1].y, point[1].z,
					point[2].x, point[2].y, point[2].z, A, B, C, D );
	if ( planeIntersectNormalCone( A, B, C, D, r,  h ) ) {
		return true;
	}

	return false;
}

/////////////////////////////////////////////// 3D box intersection
bool JagGeo::boxIntersectBox(  double px0, double py0, double pz0, double a0, double b0, double c0,
								double nx0, double ny0,
						        double x, double y, double z, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DDisjoint( px0,py0,pz0, a0,b0,c0,  x,y,z, w,d,h ) ) return false;
	if ( point3DWithinBox( px0,py0,pz0, x, y, z, w,d,h, nx, ny, false ) ) { return true; }
	if ( point3DWithinBox( x,y,z, px0,py0,pz0, a0,b0,c0, nx0, ny0, false ) ) { return true; }

	// 8 corners mutual check
	JagPoint3D corn1[8];
	cornersOfBox( a0,b0,c0, corn1 );
	for ( int i=0; i < 8; ++i ) {
		corn1[i].transform(px0,py0,pz0, nx0, ny0 );
	}

	JagPoint3D corn2[8];
	cornersOfBox( x,y,z, corn2 );
	for ( int i=0; i < 8; ++i ) {
		corn2[i].transform(x,y,z, nx, ny);
	}

	for ( int i=0; i < 8; ++i ) {
		if ( point3DWithinBox( corn1[i].x, corn1[i].y, corn1[i].z, x, y, z, w,d,h, nx, ny, false ) ) {
			return true;
		}
	}

	for ( int i=0; i < 8; ++i ) {
		if ( point3DWithinBox( corn2[i].x, corn2[i].y, corn2[i].z, px0,py0,pz0, a0,b0,c0, nx0, ny0, false ) ) {
			return true;
		}
	}

	// corners of first box on both sides of box2?
	int up=0, down=0;
	for ( int i=0; i < 8; ++i ) {
		corn1[i].transform(x,y,z, nx, ny );
		if ( jagEQ(corn1[i].z, 0.0) ) return true;
		if ( corn1[i].z > 0.0 ) ++up;
		else ++down;
	}
	if ( up *down != 0) return true;
	return false;
}

bool JagGeo::boxIntersectEllipsoid(  double px0, double py0, double pz0, double a0, double b0, double c0,
								  double nx0, double ny0,
						        	double x, double y, double z, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;

	if ( bound3DDisjoint( px0,py0,pz0, a0,b0,c0,  x,y,z, w,d,h ) ) return false;
	if ( point3DWithinEllipsoid( px0,py0,pz0, x, y, z, w,d,h, nx, ny, false ) ) { return true; }
	if ( point3DWithinBox( x,y,z, px0,py0,pz0, a0,b0,c0, nx0, ny0, false ) ) { return true; }

	// 8 corners of box inside ellipsoid
	JagPoint3D corn1[8];
	cornersOfBox( a0,b0,c0, corn1 );
	for ( int i=0; i < 8; ++i ) {
		corn1[i].transform(px0,py0,pz0, nx0, ny0 );
		if ( point3DWithinEllipsoid( corn1[i].x,  corn1[i].y, corn1[i].z, x, y, z, w,d,h, nx, ny, false ) ) { 
			return true; 
		}
	}

	// 12 edges intersect ellipsoid
	int num = 12;
	JagLine3D line[num];
	edgesOfBox( w, d, h, line );
	transform3DLinesLocal2Global( px0, py0, pz0, nx0, ny0, num, line );  // to x-y-z
	for ( int i=0; i<num; ++i ) {
		if ( line3DIntersectEllipse3D( line[i], x,y,z, w,h, nx, ny )  ) {
			return true;
		}
	}

	// 6 planes intersect ellipsoid
	JagTriangle3D tri[6];
	triangleSurfacesOfBox(w, d, h, tri );
   	double A, B, C, D;
	for ( int i=0; i < 6; ++i ) {
    	tri[i].transform(x,y,z, nx,ny  );
    	triangle3DABCD( tri[i].x1, tri[i].y1, tri[i].z1,
						tri[i].x2, tri[i].y2, tri[i].z2,
						tri[i].x3, tri[i].y3, tri[i].z3,
    					A, B, C, D );
    	if ( planeIntersectNormalEllipsoid( A, B, C, D, w,d,h ) ) {
    		return true;
    	}
	}

	// corners of first box on both sides of box2?
	int up=0, down=0;
	for ( int i=0; i < 8; ++i ) {
		corn1[i].transform(x,y,z, nx, ny );
		if ( jagEQ( corn1[i].z, 0.0 ) ) return true;
		if ( corn1[i].z > 0.0 ) ++up;
		else ++down;
	}
	if ( up *down != 0) return true;
	return false;
}

bool JagGeo::boxIntersectCone(  double px0, double py0, double pz0, double a0, double b0, double c0,
								  double nx0, double ny0,
						        	double x0, double y0, double z0, 
							    	double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;

	if ( bound3DDisjoint( px0,py0,pz0, a0,b0,c0,  x0,y0,z0, r,r,h ) ) return false;
	if ( point3DWithinCone( px0,py0,pz0, x0, y0, z0, r,h, nx, ny, false ) ) { return true; }
	if ( point3DWithinBox( x0,y0,z0, px0,py0,pz0, a0,b0,c0, nx0, ny0, false ) ) { return true; }

	//if ( boxWithinCone( px0,py0,pz0, a0,b0,c0, nx0, ny0, x0, y0, z0, r,h, nx, ny, false ) ) { return true; }
	//if ( coneWithinBox(  x0,y0,z0, r, h, nx, ny, px0,py0,pz0, a0,b0,c0, nx0, ny0, false ) ) return true;


	// triangle of the box
	JagTriangle3D tri[6];
	triangleSurfacesOfBox(a0, b0, c0, tri );
   	double A, B, C, D;
	for ( int i=0; i < 6; ++i ) {
    	tri[i].transform(x0,y0,z0, nx,ny  );
    	triangle3DABCD( tri[i].x1, tri[i].y1, tri[i].z1,
						tri[i].x2, tri[i].y2, tri[i].z2,
						tri[i].x3, tri[i].y3, tri[i].z3,
    					A, B, C, D );
    	if ( planeIntersectNormalCone( A, B, C, D, r,h ) ) {
    		return true;
    	}
	}
	return false;
}

bool JagGeo::ellipsoidIntersectBox(  double px0, double py0, double pz0, double a0, double b0, double c0,
								double nx0, double ny0,
						        double x0, double y0, double z0, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	return boxIntersectEllipsoid( x0, y0, z0, w,d,h, nx, ny, px0,py0,pz0, a0,b0,c0, nx0, ny0, false );
}

bool JagGeo::ellipsoidIntersectEllipsoid(  double px0, double py0, double pz0, double a0, double b0, double c0,
								  double nx0, double ny0,
						        	double x0, double y0, double z0, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DDisjoint( px0,py0,pz0, a0,b0,c0,  x0,y0,z0, w,d,h ) ) return false;
	if ( point3DWithinEllipsoid( px0,py0,pz0, x0, y0, z0, w,d,h, nx, ny, false ) ) { return true; }
	if ( point3DWithinEllipsoid( x0,y0,z0, px0, py0, pz0, a0,b0,c0, nx0, ny0, false ) ) { return true; }
	//if ( ellipsoidWithinEllipsoid( px0,py0,pz0, a0,b0,c0,nx0,ny0,  x0,y0,z0, w,d,h, nx,ny, false ) ) return true;
	//if ( ellipsoidWithinEllipsoid( x0,y0,z0, w,d,h, nx,ny,  px0,py0,pz0, a0,b0,c0,nx0,ny0,  false ) ) return true;


	JagVector<JagPoint3D> vec;
	double loc_x, loc_y, loc_z;
	samplesOnEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
    	transform3DCoordGlobal2Local( x0, y0, z0, vec[i].x, vec[i].y, vec[i].z, nx, ny, loc_x, loc_y, loc_z );
		if ( point3DWithinNormalEllipsoid( loc_x, loc_y, loc_z, w,d,h, strict ) ) { return true; }
	}
	return false;
}

bool JagGeo::ellipsoidIntersectCone(  double px0, double py0, double pz0, double a0, double b0, double c0,
								  double nx0, double ny0,
						        	double x0, double y0, double z0, 
							    	double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;

	if ( bound3DDisjoint( px0,py0,pz0, a0,b0,c0,  x0,y0,z0, r,r,h ) ) return false;

	if ( point3DWithinCone( px0,py0,pz0, x0, y0, z0, r,h, nx, ny, false ) ) { return true; }
	if ( point3DWithinEllipsoid( x0,y0,z0, px0, py0, pz0, a0,b0,c0, nx0, ny0, false ) ) { return true; }

	//if ( ellipsoidWithinCone( px0,py0,pz0, a0,b0,c0,nx0,ny0,  x0,y0,z0, r,h, nx,ny, false ) ) return true;
	//if ( coneWithinEllipsoid( x0,y0,z0, r,h, nx,ny,  px0,py0,pz0, a0,b0,c0,nx0,ny0,  false ) ) return true;


	JagVector<JagPoint3D> vec;
	//double loc_x, loc_y, loc_z;
	samplesOnEllipsoid( px0, py0, pz0, a0, b0, c0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int i=0; i <vec.size(); ++i ) {
		if ( point3DWithinCone( vec[i].x, vec[i].y, vec[i].z, x0,y0,z0, r, h, nx, ny, strict ) ) {
			return true;
		}
	}

	// sample lines in cone
	JagVector<JagLine3D> vec2;
	sampleLinesOnCone( x0, y0, z0, r, h, nx, ny, NUM_SAMPLE, vec2 ); 
	for ( int i=0; i < vec2.size(); ++i ) {
		if ( line3DIntersectEllipsoid( vec2[i].x1, vec2[i].y1, vec2[i].z1, vec2[i].x2, vec2[i].y2, vec2[i].z2,
										px0, py0, pz0, a0, b0, c0, nx0, ny0, false ) ) return true;
	}

	return false;
}


///////////////////////////////////// 3D cylinder /////////////////////////////////
bool JagGeo::cylinderIntersectBox(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        double x, double y, double z, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;

	if ( bound3DDisjoint( px0,py0,pz0, pr0,pr0,c0,  x,y,z, w,d,h ) ) return false;

	if ( point3DWithinBox( px0,py0,pz0,   x, y, z, w,d,h, nx, ny, false ) ) { return true; }
	if ( point3DWithinCylinder( x,y,z, px0, py0, pz0, pr0,c0, nx0, ny0, false ) ) { return true; }

	//if ( cylinderIntersectBox( px0,py0,pz0, a0,b0,c0,nx0,ny0,  x,y,z, w,d,h, nx,ny, false ) ) return true;
	//if ( boxWithinCylinder( x,y,z, w,d,h, nx,ny,  px0,py0,pz0, pr0,pr0,c0,nx0,ny0,  false ) ) return true;

	// bx corners in cylinder
	// 8 corners check
	JagPoint3D corn[8];
	cornersOfBox( w,d,h, corn );
	for ( int i=0; i < 8; ++i ) {
		corn[i].transform(px0,py0,pz0, nx0, ny0 );
	}
	for ( int i=0; i < 8; ++i ) {
		if ( point3DWithinCylinder( corn[i].x, corn[i].y, corn[i].z, px0, py0, pz0, pr0,c0, nx0, ny0, false ) ) {
			return true;
		}
	}

	// samples lines of cylinder intersect box 6 planes
	JagVector<JagLine3D> vec;
	sampleLinesOnCylinder( px0, py0, pz0, pr0, c0, nx0, ny0, NUM_SAMPLE, vec );
	JagRectangle3D rect[6];
	surfacesOfBox(w, d, h, rect );
	for ( int i=0; i<6; ++i ) {
		rect[i].transform( x,y,z, nx, ny );
	}

	for ( int i=0; i < vec.size(); ++i ) {
		for ( int j=0; j < 6; ++j ) {
			if ( line3DIntersectRectangle3D( vec[i], rect[j] ) ) return true;
		}
	}
	return false;
}

bool JagGeo::cylinderIntersectEllipsoid(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        	double x, double y, double z, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;
	if ( bound3DDisjoint( px0,py0,pz0, pr0,pr0,c0,  x,y,z, w,d,h ) ) return false;

	if ( point3DWithinBox( px0,py0,pz0,   x, y, z, w,d,h, nx, ny, false ) ) { return true; }
	if ( point3DWithinCylinder( x,y,z, px0, py0, pz0, pr0,c0, nx0, ny0, false ) ) { return true; }

	// sample lines on cynlder intersect ellipsoid
	JagVector<JagLine3D> vec;
	sampleLinesOnCylinder( px0, py0, pz0, pr0, c0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int i=0; i < vec.size(); ++i ) {
		for ( int j=0; j < 6; ++j ) {
			if ( line3DIntersectEllipsoid( vec[i].x1, vec[i].y1, vec[i].z1, vec[i].x2, vec[i].y2, vec[i].z2, 
										   x,y,z,w,d,h,nx,ny, false ) ) return true;
		}
	}

	// samples points on ellipsoid witin cylinder
	JagVector<JagPoint3D> vec2;
	samplesOnEllipsoid( x, y, z, w, d, h, nx, ny, NUM_SAMPLE, vec2 );
	for ( int i=0; i <vec2.size(); ++i ) {
		if ( point3DWithinCylinder( vec2[i].x, vec2[i].y, vec2[i].z, px0,py0,pz0, pr0,c0,nx0,ny0,false) ) {
			return true;
		}
	}
	return false;
}

bool JagGeo::cylinderIntersectCone(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        	double x, double y, double z, 
							    	double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;

	if ( bound3DDisjoint( px0,py0,pz0, pr0,pr0,c0,  x,y,z, r,r,h ) ) return false;
	if ( point3DWithinCone( px0,py0,pz0,   x, y, z, r,h, nx, ny, false ) ) { return true; }
	if ( point3DWithinCylinder( x,y,z, px0, py0, pz0, pr0,c0, nx0, ny0, false ) ) { return true; }

	// sample lines on cynlder intersect cone
	JagVector<JagLine3D> vec;
	sampleLinesOnCylinder( px0, py0, pz0, pr0, c0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int i=0; i < vec.size(); ++i ) {
		for ( int j=0; j < 6; ++j ) {
			if ( line3DIntersectCone( vec[i].x1, vec[i].y1, vec[i].z1, vec[i].x2, vec[i].y2, vec[i].z2, 
										   x,y,z,r,h,nx,ny, false ) ) return true;
		}
	}

	// samples points on ellipsoid witin cylinder
	JagVector<JagLine3D> vec2;
	sampleLinesOnCone( x, y, z, r, h, nx, ny, NUM_SAMPLE, vec2 );
	for ( int i=0; i <vec2.size(); ++i ) {
		if ( line3DIntersectCylinder( vec2[i].x1, vec2[i].y1, vec2[i].z1,
									vec2[i].x2, vec2[i].y2, vec2[i].z2,
									px0,py0,pz0, pr0,pr0,c0,nx0,ny0,false) ) {
			return true;
		}
	}
	return false;
}


///////////////////////////////////// 3D cone /////////////////////////////////
bool JagGeo::coneIntersectBox(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        double x, double y, double z, 
							    double w, double d, double h, double nx, double ny, bool strict )
{
	return boxIntersectCone( x,y,z,w,d,h,nx,ny,  px0,py0,pz0,pr0,c0,nx0,ny0,false);
}

bool JagGeo::coneIntersectEllipsoid(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        	double x, double y, double z, 
							    	double w, double d, double h, double nx, double ny, bool strict )
{
	return ellipsoidIntersectCone( x,y,z,w,d,h,nx,ny, px0,py0,pz0,pr0,c0,nx0,ny0,false);
}


bool JagGeo::coneIntersectCone(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
						        	double x, double y, double z, 
							    	double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;

	if ( bound3DDisjoint( px0,py0,pz0, pr0,pr0,c0,  x,y,z, r,r,h ) ) return false;
	if ( point3DWithinCone( px0,py0,pz0,   x, y, z, r,h, nx, ny, false ) ) { return true; }
	if ( point3DWithinCone( x,y,z, px0, py0, pz0, pr0,c0, nx0, ny0, false ) ) { return true; }

	// sample lines on cynlder intersect cone
	JagVector<JagLine3D> vec;
	sampleLinesOnCylinder( px0, py0, pz0, pr0, c0, nx0, ny0, NUM_SAMPLE, vec );
	for ( int i=0; i < vec.size(); ++i ) {
		for ( int j=0; j < 6; ++j ) {
			if ( line3DIntersectCylinder( vec[i].x1, vec[i].y1, vec[i].z1, vec[i].x2, vec[i].y2, vec[i].z2, 
										   x,y,z,r,r,h,nx,ny, false ) ) return true;
		}
	}

	// samples points on ellipsoid witin cylinder
	JagVector<JagLine3D> vec2;
	sampleLinesOnCylinder( x, y, z, r, h, nx, ny, NUM_SAMPLE, vec2 );
	for ( int i=0; i <vec2.size(); ++i ) {
		if ( line3DIntersectCylinder( vec2[i].x1, vec2[i].y1, vec2[i].z1,
									vec2[i].x2, vec2[i].y2, vec2[i].z2,
									px0,py0,pz0, pr0,pr0,c0,nx0,ny0,false) ) {
			return true;
		}
	}
	return false;

}


bool JagGeo::circle3DIntersectCone( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
                                 double x, double y, double z,
                                 double r, double h, double nx, double ny, bool strict )
{
	if ( ! validDirection(nx0, ny0) ) return false;
	if ( ! validDirection(nx, ny) ) return false;

	if ( bound3DDisjoint( px0,py0,pz0, pr0,pr0,pr0,  x,y,z, r,r,h ) ) return false;
	if ( point3DWithinCone( px0,py0,pz0,  x, y, z, r,h, nx, ny, false ) ) { return true; }

	// sample lines of cone intesect circle
	JagVector<JagLine3D> vec2;
	sampleLinesOnCone( x, y, z, r, h, nx, ny, NUM_SAMPLE, vec2 );
	for ( int i=0; i <vec2.size(); ++i ) {
		if ( line3DIntersectEllipse3D( vec2[i], px0,py0,pz0, pr0,pr0,nx0,ny0) ) {
			return true;
		}
	}
	return false;
}

/////////////////////////////// end intersect methods //////////////////////////////////////////////
bool JagGeo::doClosestPoint(  const Jstr& colType1, int srid, double px, double py, double pz,
                                 const Jstr& mark2, const Jstr &colType2, 
			                     const JagStrSplit &sp2, Jstr &res )
{
	d("s1102 doAllClosestPoint sp2:\n" );
	//sp2.print();
	//char *str, p;
	//double dx,dy,dz;

	// colType1 must be point or point3d
	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		res = "CJAG=0=0=PT=d 0:0:0:0 ";
    	if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			res = sp2[JAG_SP_START+0] + " " + sp2[JAG_SP_START+1];
    	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
    		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			double projx, projy;
			minPoint2DToLineSegDistance( px, py, x1, y1, x2, y2, srid, projx, projy );
			res += d2s(projx) + " " + d2s(projy);
    	} else if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
			double d1, d2, d3, p1x, p1y, p2x, p2y, p3x, p3y;
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str());
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str());
			double x2 = jagatof( sp2[JAG_SP_START+2].c_str());
			double y2 = jagatof( sp2[JAG_SP_START+3].c_str());
			double x3 = jagatof( sp2[JAG_SP_START+4].c_str());
			double y3 = jagatof( sp2[JAG_SP_START+5].c_str());
			d1 = minPoint2DToLineSegDistance( px, py, x1, y1, x2, y2, srid, p1x, p1y );
			d2 = minPoint2DToLineSegDistance( px, py, x1, y1, x3, y3, srid, p2x, p2y );
			d3 = minPoint2DToLineSegDistance( px, py, x2, y2, x3, y3, srid, p3x, p3y );
			if ( d1 < d2 && d1 < d3 ) {
				res += d2s(p1x) + " " + d2s(p1y);
			} else if ( d2 < d1 && d2 < d3 ) {
				res += d2s(p2x) + " " + d2s(p2y);
			} else {
				res += d2s(p3x) + " " + d2s(p3y);
			}
    	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
    		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    		double nx = safeget(sp2, JAG_SP_START+3);
			double locx, locy;
			transform2DCoordGlobal2Local( x0, y0, px, py, nx, locx, locy );
			double d1, d2, d3, d4, p1x, p1y, p2x, p2y, p3x, p3y, p4x, p4y;
			d1 = minPoint2DToLineSegDistance( locx, locy, -r, -r, -r, r, srid, p1x, p1y );
			d2 = minPoint2DToLineSegDistance( locx, locy, -r, -r, r, -r, srid, p2x, p2y );
			d3 = minPoint2DToLineSegDistance( locx, locy, r, -r, r, r, srid, p3x, p3y );
			d4 = minPoint2DToLineSegDistance( locx, locy, -r, r, r, r, srid, p4x, p4y );
			if ( d1 < d2 && d1 < d3 && d1 < d4 ) {
				res += d2s(p1x) + " " + d2s(p1y);
			} else if ( d2 < d1 && d2 < d3 && d2 < d4 ) {
				res += d2s(p2x) + " " + d2s(p2y);
			} else if ( d3 < d1 && d3 < d2 && d3 < d4 ) {
				res += d2s(p3x) + " " + d2s(p3y);
			} else {
				res += d2s(p4x) + " " + d2s(p4y);
			}
    	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
    		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double w = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    		double h = jagatof( sp2[JAG_SP_START+3].c_str() ); 
    		double nx = safeget(sp2, JAG_SP_START+4);
			double locx, locy;
			transform2DCoordGlobal2Local( x0, y0, px, py, nx, locx, locy );
			double d1, d2, d3, d4, p1x, p1y, p2x, p2y, p3x, p3y, p4x, p4y;
			d1 = minPoint2DToLineSegDistance( locx, locy, -w, -h, -w, h, srid, p1x, p1y );
			d2 = minPoint2DToLineSegDistance( locx, locy, -w, -h, w, -h, srid, p2x, p2y );
			d3 = minPoint2DToLineSegDistance( locx, locy, w, -h, w, h, srid, p3x, p3y );
			d4 = minPoint2DToLineSegDistance( locx, locy, -w, h, w, h, srid, p4x, p4y );
			if ( d1 < d2 && d1 < d3 && d1 < d4 ) {
				res += d2s(p1x) + " " + d2s(p1y);
			} else if ( d2 < d1 && d2 < d3 && d2 < d4 ) {
				res += d2s(p2x) + " " + d2s(p2y);
			} else if ( d3 < d1 && d3 < d2 && d3 < d4 ) {
				res += d2s(p3x) + " " + d2s(p3y);
			} else {
				res += d2s(p4x) + " " + d2s(p4y);
			}
    	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
    		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double r = jagatof( sp2[JAG_SP_START+2].c_str() );
			double d = distance(px, py, x, y, srid );
			if ( r < d ) {
				res += d2s(px*d/r) + " " + d2s(py*d/r);
			} else {
				res += d2s(px*r/d) + " " + d2s(py*r/d);
			}
    	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
    		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
    		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
    		double nx = safeget(sp2, JAG_SP_START+4);
			double locx, locy;
			transform2DCoordGlobal2Local( x0,y0, px, py, nx, locx, locy );
			double projx, projy, dist;
			minMaxPointOnNormalEllipse( srid, a, b, locx, locy, true, projx, projy, dist );
			res += d2s(projx) + " " + d2s(projy);
    	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			double mindist, minx, miny;
			getMinDist2DPointFraction(px,py, srid, sp2, mindist, minx, miny );
			res += d2s(minx) + " " + d2s(miny);
    	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_POLYGON ) {
			JagPolygon pgon;
    		int rc = JagParser::addPolygonData( pgon, sp2, false );
    		if ( rc < 0 ) { return false; }
			double minx, miny;
			getMinDist2DPointOnPloygonAsLineStrings( srid, px, py, pgon, minx, miny );
			res += d2s(minx) + " " + d2s(miny);
    	} else if (  colType2 == JAG_C_COL_TYPE_MULTIPOINT
					 || colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
			Jstr t;
			if ( closestPoint2DRaster( srid, px, py, mark2, sp2, t ) ) {
				res += t;
			} else return false;
    	} else {
			res = sp2[JAG_SP_START+0] + " " + sp2[JAG_SP_START+1];
		}
	} else {
		res = "CJAG=0=0=PT3=d 0:0:0:0:0:0 ";
    	if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
			res += sp2[JAG_SP_START+0] + " " + sp2[JAG_SP_START+1] + " " + sp2[JAG_SP_START+3];
    	} else if ( colType2 == JAG_C_COL_TYPE_LINE3D ) {
    		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    		double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
    		double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
    		double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
			double projx, projy, projz;
			minPoint3DToLineSegDistance( px, py, pz, x1, y1, z1, x2, y2, z2, srid, projx, projy, projz );
			res += d2s(projx) + " " + d2s(projy) + " " + d2s(projz);;
    	} else if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
    		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
    		double nx = safeget(sp2, JAG_SP_START+4);
    		double ny = safeget(sp2, JAG_SP_START+5);
			double dist;
			Jstr t;
			closestPoint3DBox( srid, px, py, pz, x0, y0, z0, r, r, r, nx, ny, dist, t ); 
			res += t;
    	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
    		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    		double a = jagatof( sp2[JAG_SP_START+3].c_str() ); 
    		double b = jagatof( sp2[JAG_SP_START+4].c_str() ); 
    		double c = jagatof( sp2[JAG_SP_START+5].c_str() ); 
    		double nx = safeget(sp2, JAG_SP_START+6);
    		double ny = safeget(sp2, JAG_SP_START+7);
			double dist;
			Jstr t;
			closestPoint3DBox( srid, px, py, pz, x0, y0, z0, a, b, c, nx, ny, dist, t ); 
			res += t;
    	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
    		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
			double d = distance(px, py, pz, x, y, z, srid );
			if ( r < d ) {
				res += d2s(px*d/r) + " " + d2s(py*d/r) + " " + d2s(pz*d/r);
			} else {
				res += d2s(px*r/d) + " " + d2s(py*r/d) + " " + d2s(pz*r/d);
			}
    	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
    		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    		double a = jagatof( sp2[JAG_SP_START+3].c_str() );
    		double b = jagatof( sp2[JAG_SP_START+4].c_str() );
    		double c = jagatof( sp2[JAG_SP_START+5].c_str() );
    		double nx = safeget(sp2, JAG_SP_START+6);
    		double ny = safeget(sp2, JAG_SP_START+7);
			double locx, locy, locz;
			transform3DCoordGlobal2Local( x0,y0,z0, px, py, pz, nx, ny, locx, locy, locz );
			double projx, projy, projz, dist;
			minMaxPoint3DOnNormalEllipsoid( srid, a, b, c,  locx, locy, locz, true, projx, projy, projz, dist );
			// reuse a b c
			transform3DCoordLocal2Global( x0, y0, z0, projx, projy, projz, nx, ny, a, b, c );
			res += d2s(a) + " " + d2s(b) + " " + d2s(c);
    	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
    		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
    		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
    		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
    		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
    		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
    		double nx = safeget(sp2, JAG_SP_START+5);
    		double ny = safeget(sp2, JAG_SP_START+6);
			// use triangle model
			double locx, locy, locz;
			transform3DCoordGlobal2Local( x0,y0,z0, px, py, pz, nx, ny, locx, locy, locz );

			// locz --> y  locx+lcy --> x
			double px = sqrt( locx*locx + locy*locy );
			double py = locz;
			double d1, d2, d3, p1x, p1y, p2x, p2y, p3x, p3y;
			double x1 = -2.0*r;
			double y1 = -h;
			double x2 = 2.0*r;
			double y2 = -h;
			double x3 = 0.0;
			double y3 = h;
			double r2;
			double rx;
			double ry;
			d1 = minPoint2DToLineSegDistance( px, py, x1, y1, x2, y2, srid, p1x, p1y );
			d2 = minPoint2DToLineSegDistance( px, py, x1, y1, x3, y3, srid, p2x, p2y );
			d3 = minPoint2DToLineSegDistance( px, py, x2, y2, x3, y3, srid, p3x, p3y );
			if ( d1 < d2 && d1 < d3 ) {
				r2 = (p1x*p1x + p1y*p1y);
				rx = sqrt( p1x*p1x/r2 );
				ry = sqrt( p1y*p1y/r2 );
				locx = rx * p1x;
				locy = ry * p1y;
				locz = -h;
			} else if ( d2 < d1 && d2 < d3 ) {
				r2 = (p2x*p2x + p2y*p2y);
				rx = sqrt( p2x*p2x/r2 );
				ry = sqrt( p2y*p2y/r2 );
				locx = rx * p2x;
				locy = ry * p2y;
				locz = -h;
			} else {
				locx = locy = 0.0; locz = h;
			}
			transform3DCoordLocal2Global( x0, y0, z0, locx, locy, locz, nx, ny, d1, d2, d3 );
			res += d2s(d1) + " " + d2s(d2) + " " + d2s(d3);;
    	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
			double mindist, minx, miny, minz;
			getMinDist3DPointFraction(px,py,pz, srid, sp2, mindist, minx, miny, minz );
			res += d2s(minx) + " " + d2s(miny) + " " + d2s(minz);
    	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D || colType2 == JAG_C_COL_TYPE_POLYGON3D ) {
			JagPolygon pgon;
    		int rc = JagParser::addPolygon3DData( pgon, sp2, false );
    		if ( rc < 0 ) { return false; }
			double minx, miny, minz;
			getMinDist3DPointOnPloygonAsLineStrings( srid, px, py, pz, pgon, minx, miny, minz );
			res += d2s(minx) + " " + d2s(miny);
    	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOINT3D
					 || colType2 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
			Jstr t;
			if ( closestPoint3DRaster( srid, px, py, pz, mark2, sp2, t ) ) {
				res += t;
			} else return false;
    	} else {
			res += sp2[JAG_SP_START+0] + " " + sp2[JAG_SP_START+1] + " " +  sp2[JAG_SP_START+2];
    	}
	}

	return true;
}

////////////////// same(equal) methods
bool JagGeo::doPointSame( const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		return jagEQ(px0, x0) && jagEQ(py0, y0);
	} else {
		return false;
	} 
}

bool JagGeo::doPoint3DSame( const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	//d("s4409 doPoint3DSame colType2=[%s]\n", colType2.c_str() );
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		return jagEQ( px0,x1) && jagEQ( py0, y1 ) && jagEQ(pz0, z1 );
	} else {
		return false;
	}
}

bool JagGeo::doCircleSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	pr0 = meterToLon( srid2, pr0, px0, py0);

	if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() );
		r = meterToLon( srid2, r, x, y);
		return jagEQ(px0,x) && jagEQ( py0,y) && jagEQ(pr0, r );
	} else {
		return false;
	}
}

// circle surface with x y z and orientation
bool JagGeo::doCircle3DSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 

	double nx0 = 0.0;
	double ny0 = 0.0;
	if ( sp1.length() >= 5 ) { nx0 = jagatof( sp1[JAG_SP_START+4].c_str() ); }
	if ( sp1.length() >= 6 ) { ny0 = jagatof( sp1[JAG_SP_START+5].c_str() ); }

	if (  colType2 == JAG_C_COL_TYPE_CIRCLE3D ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = 0.0;
		double ny = 0.0;
		if ( sp2.length() >= 5 ) { nx = jagatof( sp2[JAG_SP_START+4].c_str() ); }
		if ( sp2.length() >= 6 ) { ny = jagatof( sp2[JAG_SP_START+5].c_str() ); }
		return jagEQ(px0,x) && jagEQ( py0,y) && jagEQ(pz0, z) && jagEQ(pr0, r) && jagEQ(nx0, nx) && jagEQ(ny0, ny);
	} else {
		return false;
	}

}

bool JagGeo::doSphereSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 

	if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return jagEQ(px0,x) && jagEQ( py0,y) && jagEQ(pz0, z) && jagEQ(pr0, r);
	} else {
		return false;
	}
}

// 2D
bool JagGeo::doSquareSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	//d("s3033 doSquareSame colType2=[%s] \n", colType2.c_str() );
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	pr0 = meterToLon( srid2, pr0, px0, py0 );
	double nx0 = safeget(sp1, JAG_SP_START+3);

	if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		a = meterToLon( srid2, a, x, y );
		double nx = safeget(sp2, JAG_SP_START+3);
		return jagEQ(px0,x) && jagEQ( py0,y) && jagEQ(pr0, a) && jagEQ(nx0, nx);
	} else {
		return false;
	}
}

bool JagGeo::doSquare3DSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);
	double ny0 = safeget(sp1, JAG_SP_START+5);

	if (  colType2 == JAG_C_COL_TYPE_SQUARE3D ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return jagEQ(px0,x) && jagEQ( py0,y) && jagEQ( pz0,z) && jagEQ(pr0, r) && jagEQ(nx0, nx) && jagEQ(ny0, ny);
	} else  {
	}
	
	return false;
}


bool JagGeo::doCubeSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);
	double ny0 = safeget(sp1, JAG_SP_START+5);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return jagEQ(px0,x) && jagEQ( py0,y) && jagEQ( pz0,z) && jagEQ(pr0, r) && jagEQ(nx0, nx) && jagEQ(ny0, ny);
	} else {
		return false;
	}
}

// 2D
bool JagGeo::doRectangleSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);

	a0 = meterToLon( srid2, a0, px0, py0 );
	b0 = meterToLat( srid2, b0, px0, py0 );

	if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		a = meterToLon( srid2, a, x, y );
		b = meterToLat( srid2, b, x, y );
		double nx = safeget(sp2, JAG_SP_START+4);
		return jagEQ(px0,x) && jagEQ( py0,y) && jagEQ(a0, a) && jagEQ(b0, b) && jagEQ(nx0, nx);
	} else {
		return false;
	}
}

// 3D rectiangle
bool JagGeo::doRectangle3DSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if ( colType2 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return jagEQ(px0,x) && jagEQ( py0,y) && jagEQ( pz0,z) && jagEQ(a0, a) && jagEQ(b0, b) && jagEQ(nx0, nx) && jagEQ(ny0, ny);
	} else {
		return false;
	}

}

bool JagGeo::doBoxSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+6);
	double ny0 = safeget(sp1, JAG_SP_START+7);

	if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double c = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return jagEQ(px0,x) && jagEQ( py0,y) && jagEQ( pz0,z) && jagEQ(a0, a) && jagEQ(b0, b) 
				&& jagEQ(c0, c) && jagEQ(nx0, nx) && jagEQ(ny0, ny);
	} else {
		return false;
	}
}

// 3D
bool JagGeo::doCylinderSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 

	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if (  colType2 == JAG_C_COL_TYPE_CYLINDER ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double c = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return jagEQ(px0,x) && jagEQ(py0,y) && jagEQ(pz0,z) && jagEQ(pr0, r) && jagEQ(c0, c) 
				&& jagEQ(nx0, nx) && jagEQ(ny0, ny);
	} else {
		return false;
	}
	
}

bool JagGeo::doConeSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double c = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return jagEQ(px0,x) && jagEQ(py0,y) && jagEQ(pz0,z) && jagEQ(pr0, r) && jagEQ(c0, c) 
				&& jagEQ(nx0, nx) && jagEQ(ny0, ny);
	} else {
		return false;
	}
}

// 2D
bool JagGeo::doEllipseSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);

	a0 = meterToLon( srid2, a0, px0, py0 );
	b0 = meterToLat( srid2, b0, px0, py0 );

	if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		a = meterToLon( srid2, a, x, y );
		b = meterToLat( srid2, b, x, y );
		return jagEQ(px0,x) && jagEQ(py0,y) && jagEQ(a0, a) && jagEQ(b0, b) && jagEQ(nx0, nx);
	} else { 
		return false;
	}
}

// 3D ellipsoid
bool JagGeo::doEllipsoidSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+6);
	double ny0 = safeget(sp1, JAG_SP_START+7);

	if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() );
		double b = jagatof( sp2[JAG_SP_START+4].c_str() );
		double c = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return jagEQ(px0,x) && jagEQ(py0,y) && jagEQ(pz0,z) && jagEQ(a0, a) && jagEQ(b0, b) && jagEQ(c0,c)
				&& jagEQ(nx0, nx) && jagEQ(ny0,ny);
	} else {
		return false;
	}
}

// 2D triangle within
bool JagGeo::doTriangleSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double x30 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double y30 = jagatof( sp1[JAG_SP_START+5].c_str() );

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return    jagEQ(x10,x1)
		       && jagEQ(y10,y1)
		       && jagEQ(x20,x2)
		       && jagEQ(y20,y2)
		       && jagEQ(x30,x3)
		       && jagEQ(y30,y3);
	} else {
		return false;
	}
}

// 3D  triangle
bool JagGeo::doTriangle3DSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double z10 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double z20 = jagatof( sp1[JAG_SP_START+5].c_str() );
	double x30 = jagatof( sp1[JAG_SP_START+6].c_str() );
	double y30 = jagatof( sp1[JAG_SP_START+7].c_str() );
	double z30 = jagatof( sp1[JAG_SP_START+8].c_str() );

	if ( colType2 == JAG_C_COL_TYPE_TRIANGLE3D ) {
    	double x1 = jagatof( sp1[JAG_SP_START+0].c_str() );
    	double y1 = jagatof( sp1[JAG_SP_START+1].c_str() );
    	double z1 = jagatof( sp1[JAG_SP_START+2].c_str() );
    	double x2 = jagatof( sp1[JAG_SP_START+3].c_str() );
    	double y2 = jagatof( sp1[JAG_SP_START+4].c_str() );
    	double z2 = jagatof( sp1[JAG_SP_START+5].c_str() );
    	double x3 = jagatof( sp1[JAG_SP_START+6].c_str() );
    	double y3 = jagatof( sp1[JAG_SP_START+7].c_str() );
    	double z3 = jagatof( sp1[JAG_SP_START+8].c_str() );
		return    jagEQ(x10,x1)
		       && jagEQ(y10,y1)
		       && jagEQ(z10,z1)
		       && jagEQ(x20,x2)
		       && jagEQ(y20,y2)
		       && jagEQ(z20,z2)
		       && jagEQ(x30,x3)
		       && jagEQ(y30,y3)
		       && jagEQ(z30,z3);
	} else {
		return false;
	}
}

// 2D line
bool JagGeo::doLineSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+3].c_str() );

	// like point within
	if (  colType2 == JAG_C_COL_TYPE_LINE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		return    jagEQ(x10,x1)
		       && jagEQ(y10,y1)
		       && jagEQ(x20,x2)
		       && jagEQ(y20,y2);
	} else {
		return false;
	}
}

bool JagGeo::doLine3DSame( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double z10 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double z20 = jagatof( sp1[JAG_SP_START+5].c_str() );

	if (  colType2 == JAG_C_COL_TYPE_LINE3D ) {
		double x1 = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z1 = jagatof( sp1[JAG_SP_START+2].c_str() );
		double x2 = jagatof( sp1[JAG_SP_START+3].c_str() );
		double y2 = jagatof( sp1[JAG_SP_START+4].c_str() );
		double z2 = jagatof( sp1[JAG_SP_START+5].c_str() );
		return    jagEQ(x10,x1)
		       && jagEQ(y10,y1)
		       && jagEQ(z10,z1)
		       && jagEQ(x20,x2)
		       && jagEQ(y20,y2)
		       && jagEQ(z20,z2);
	} else {
		return false;
	}
}

// 2D linestring
bool JagGeo::doLineStringSame( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
								 const Jstr &colType2, int srid2, const JagStrSplit &sp2 )
{
	// like point within
	if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return sequenceSame(colType2, mk1, sp1, mk2, sp2 );
	} else {
		return false;
	}
}

bool JagGeo::doLineString3DSame( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2 )
{
	if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return sequenceSame( colType2, mk1, sp1, mk2, sp2 );
	} else {
		return false;
	}
}

bool JagGeo::doPolygonSame( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
								 const Jstr &colType2, int srid2, const JagStrSplit &sp2 )
{
	/***
	//sp1.print();
	i=0 [OJAG=0=test.pol2.po2=PL]
	i=1 [0.0:0.0:500.0:600.0] // bbox
	i=2 [0.0:0.0]
	i=3 [20.0:0.0]
	i=4 [8.0:9.0]
	i=5 [0.0:0.0]
	i=6 [|]
	i=7 [1.0:2.0]
	i=8 [2.0:3.0]
	i=9 [1.0:2.0]
	***/

	if ( colType2 != JAG_C_COL_TYPE_POLYGON ) {
		return false;
	} 

	return sequenceSame( colType2, mk1, sp1, mk2, sp2 );
}

bool JagGeo::doMultiPolygonSame( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
								 const Jstr &colType2, int srid2, const JagStrSplit &sp2 )
{
	if ( colType2 != JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return false;
	} 

	return sequenceSame( colType2, mk1, sp1, mk2, sp2 );
}


bool JagGeo::doPolygon3DSame( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2 )
{
	if ( colType2 != JAG_C_COL_TYPE_POLYGON ) {
		return false;
	} 
	return sequenceSame( colType2, mk1, sp1, mk2, sp2 );
}

bool JagGeo::doMultiPolygon3DSame( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2 )
{
	if ( colType2 != JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return false;
	} 
	return sequenceSame( colType2, mk1, sp1, mk2, sp2 );
}


/////////////////////////////////// misc methods /////////////////////////////////////////////////////////
bool JagGeo::locIn3DCenterBox( double loc_x, double loc_y, double loc_z, double a, double b, double c, bool strict )
{
	if ( strict ) {
		if ( jagLE(loc_x, -a) ) return false;
		if ( jagLE(loc_y, -b) ) return false;
		if ( jagLE(loc_z, -c) ) return false;
		if ( jagGE(loc_x, a) ) return false;
		if ( jagGE(loc_y, b) ) return false;
		if ( jagGE(loc_z, c) ) return false;
	} else {
		if ( loc_x < -a ) return false;
		if ( loc_y < -b ) return false;
		if ( loc_z < -c ) return false;
		if ( loc_x > a ) return false;
		if ( loc_y > b ) return false;
		if ( loc_z > c ) return false;
	}

	return true;

}

bool JagGeo::locIn2DCenterBox( double loc_x, double loc_y, double a, double b, bool strict )
{
	if ( strict ) {
		if ( jagLE(loc_x, -a) ) return false;
		if ( jagLE(loc_y, -b) ) return false;
		if ( jagGE(loc_x, a) ) return false;
		if ( jagGE(loc_y, b) ) return false;
	} else {
		if ( loc_x < -a ) return false;
		if ( loc_y < -b ) return false;
		if ( loc_x > a ) return false;
		if ( loc_y > b ) return false;
	}
	return true;
}


// line1:  y = slope1 * x + c1
// line2:  y = slope2 * x + c2
bool JagGeo::twoLinesIntersection( double slope1, double c1, double slope2, double c2,
	                                      double &outx, double &outy )
{
	if ( jagEQ(slope1, slope2) ) return false;
	outx  = (c2-c1)/(slope1-slope2);
	outy = ( slope1*c2 - slope2*c1) / (slope1-slope2);
	return true;
}

// ellipse relative to origin at (0,0) nx is unit vector on x-axis of y-diretion of ellipse
// x1,y1, .... coord of 4 corners in real x-y sys
void JagGeo::orginBoundingBoxOfRotatedEllipse( double a, double b, double nx,
                                        double &x1, double &y1,
                                        double &x2, double &y2,
                                        double &x3, double &y3,
                                        double &x4, double &y4 )
{
	// if ellipse is not rotated
	if ( jagIsZero( nx ) ) {
		x1 = -a; y1 = b;
		x2 = a; y2 = b;
		x3 = a; y3 = -b;
		x4 = -a; y4 = -b;
		return;
	}
	//  tangent lines are  = m*x + c
	// nx = sin(a)
	double ny = sqrt( 1.0 - nx*nx);  // cos(a)
	double m1, c1, m2, c2, m3, c3, m4, c4;
	double a2= a*a;
	double b2= b*b;

	m1 = nx/ny;
	c1 = sqrt( a2*m1*m1 + b2);

	m2 = m1;
	c2 = -c1;  // m2 // m1

	m3 = -1.0/m1;  // m3 _|_ m1
	c3 = -sqrt( a2*m3*m3 + b2);

	m4 = m3;    // m4 // m3
	c4 = -c3;

	// twoLinesIntersection( double slope1, double c1, double slope2, double c2, double &outx, double &outy )
	twoLinesIntersection( m1, c1, m3, c3, x1, y1 );
	twoLinesIntersection( m3, c3, m2, c2, x2, y2 );
	twoLinesIntersection( m2, c2, m4, c4, x3, y3 );
	twoLinesIntersection( m4, c4, m1, c1, x4, y4 );
}


// 2D
// get x1, y1, ... relative to x-y, absolute coords
void JagGeo::boundingBoxOfRotatedEllipse( double x0, double y0, double a, double b, double nx,
                                        double &x1, double &y1,
                                        double &x2, double &y2,
                                        double &x3, double &y3,
                                        double &x4, double &y4 )
{
	// x1 y1, .... relative to ellipse center, in x -y coord
	orginBoundingBoxOfRotatedEllipse( a, b, nx, x1, y1, x2, y2, x3, y3, x4, y4 );
	x1 += x0; y1 += y0;
	x2 += x0; y2 += y0;
	x3 += x0; y3 += y0;
	x4 += x0; y4 += y0;
}


// circle x0y0z0 r0 is in x-y sys
// output ellipse will be on x-y plane in x-y system
void JagGeo::project3DCircleToXYPlane( double x0, double y0, double z0, double r0, double nx0, double ny0,
	                                     double x, double y, double a, double b, double nx )
{
	double nz0 = sqrt( 1.0 - nx0*nx0 - ny0*ny0 );
	nx = nx0;
	a = r0;
	b = r0 * nz0;
	x = x0;
	y = y0;
}


// circle x0y0z0 r0 is in x-y sys
// output ellipse will be on y-z plane in y-z
void JagGeo::project3DCircleToYZPlane( double x0, double y0, double z0, double r0, double nx0, double ny0,
	                                   double y, double z, double a, double b, double ny )
{
	ny = ny0;
	a = r0;
	b = r0 * nx0;
	y = y0;
	z = z0;
}

// circle x0y0z0 r0 is in z-x
// circle x0y0z0 r0 is in x-y sys
// output ellipse will be on y-z plane in y-z
void JagGeo::project3DCircleToZXPlane( double x0, double y0, double z0, double r0, double nx0, double ny0,
	                                   double z, double x, double a, double b, double nz )
{
	nz = sqrt( 1.0 - nx0*nx0 - ny0*ny0 );
	a = r0;
	b = r0 * ny0;
	z = z0;
	x = x0;
}


// rotate only
// nx1, ny1 what value will be in nx2 ny2
void JagGeo::transform3DDirection( double nx1, double ny1, double nx2, double ny2, double &nx, double &ny )
{
	// rotate nx1, ny1, nz1 to nx2 ny2
	double nz1 = sqrt( 1.0 - nx1*nx1 - ny1*ny1 );
	double nz;
	rotate3DCoordGlobal2Local( nx1, ny1, nz1, nx2, ny2, nx, ny, nz );  // nx ny outz are in x'-y'-z'
}

// rotate only
// nx1 what value will be in nx2
void JagGeo::transform2DDirection( double nx1, double nx2, double &nx )
{
	// rotate nx1, ny1, nz1 to nx2 ny2
	double ny1 = sqrt( 1.0 - nx1*nx1 );
	double ny;
	rotate2DCoordGlobal2Local( nx1, ny1, nx2, nx, ny );  // nx ny outz are in x'-y'-z'
	/***
	double ny = sqrt( 1.0 - nx*nx );
	outx = inx*ny - iny*nx;      nx1*ny1 - ny1:nx1 = 0
	outy = inx*nx + iny*ny;      nx1*nx1 + ny1*ny1 = 1
	if nx2 == nx1, nx should be zero n is 1
	***/
}

void JagGeo::samplesOn2DCircle( double x0, double y0, double r, int num, JagVector<JagPoint2D> &samples )
{
	if ( num < 1 ) return;
	double delta = 2*JAG_PI/(double)num;
	double alpha = 0.0, x, y;
	for ( int i=0; i < num; ++i ) {
		x = r*cos(alpha) + x0;
		y = r*sin(alpha) + y0;
		samples.append(JagPoint2D(x,y) );
		alpha += delta;
	}
}

void JagGeo::samplesOn3DCircle( double x0, double y0, double z0, double r, double nx, double ny, int num, 
								JagVector<JagPoint3D> &samples )
{
	if ( num < 1 ) return;
	double delta = 2*JAG_PI/(double)num;
	double alpha = 0.0;
	double x,y,z, gx, gy, gz;
	for ( int i=0; i < num; ++i ) {
		x = r*cos(alpha);
		y = r*sin(alpha);
		z = 0.0;
		transform3DCoordLocal2Global( x0, y0, z0, x, y, z, nx, ny, gx, gy, gz );
		samples.append(JagPoint3D( gx, gy, gz) );
		alpha += delta;
	}
}

void JagGeo::samplesOn2DEllipse( double x0, double y0, double a, double b, double nx, int num, 
								 JagVector<JagPoint2D> &samples )
{
	if ( num < 1 ) return;
	double delta = 2*JAG_PI/(double)num;
	double t = 0.0;
	double x,y, gx, gy;
	for ( int i=0; i < num; ++i ) {
		x = a*cos(t);
		y = b*sin(t);
		transform2DCoordLocal2Global( x0, y0, x, y, nx, gx, gy ); 
		samples.append(JagPoint2D( gx, gy) );
		t += delta;
	}
}

void JagGeo::samplesOn3DEllipse( double x0, double y0, double z0, double a, double b, double nx, double ny, 
							     int num, JagVector<JagPoint3D> &samples )
{
	if ( num < 1 ) return;
	double delta = 2*JAG_PI/(double)num;
	double t = 0.0;
	double x,y,z, gx, gy, gz;
	for ( int i=0; i < num; ++i ) {
		x = a*cos(t);
		y = b*sin(t);
		z = 0.0;
		transform3DCoordLocal2Global( x0, y0, z0, x, y, z, nx, ny, gx, gy, gz );
		samples.append(JagPoint3D( gx, gy, gz) );
		t += delta;
	}
}

void JagGeo::samplesOnEllipsoid( double x0, double y0, double z0, double a, double b, double c, 
								 double nx, double ny, int num, JagVector<JagPoint3D> &samples )
{
	/***
		x = a cos(u)* sin(v)
		y = a sin(u)* sin(v)
		z = cos(v)
		u: 0-2PI     v: 0-PI
	***/
	if ( num < 1 ) return;
	double deltau = 2*JAG_PI/(double)num;
	double deltav = JAG_PI/(double)num;
	double u = 0.0;
	double v = 0.0;
	double x,y, z, gx, gy, gz;
	for ( int i=0; i < num; ++i ) {
		for ( int j=0; j < num; ++j ) {
			x = a*cos(u) * sin(v);
			y = b*sin(u) * sin(v);
			z = b*cos(v);
			transform3DCoordLocal2Global( x0, y0, z0, x, y, z, nx, ny, gx, gy, gz ); 
			samples.append(JagPoint3D( gx, gy, gz) );
			v += deltav;
		}
		u += deltau;
	}
}

// from tip to bottom circle
void JagGeo::sampleLinesOnCone( double x0, double y0, double z0, double r, double h, 
								double nx, double ny, int num, JagVector<JagLine3D> &samples )
{
	if ( num < 1 ) return;
	double delta = 2*JAG_PI/(double)num;
	double alpha = 0.0;
	double x,y,z, gx, gy, gz;
	JagLine3D line;
	// tip
	line.x1 = 0.0; line.y1 = 0.0; line.z1 = h;
	transform3DCoordLocal2Global( x0,y0,z0, line.x1, line.y1, line.z1, nx, ny, gx, gy, gz );
	line.x1 = gx; line.y1 = gy; line.z1 = gz;
	for ( int i=0; i < num; ++i ) {
		x = r*cos(alpha);
		y = r*sin(alpha);
		z = -h;
		transform3DCoordLocal2Global( x0, y0, z0, x, y, z, nx, ny, gx, gy, gz );
		line.x2 = gx; line.y2 = gy; line.z2 = gz;
		samples.append( line );
		alpha += delta;
	}
}

// from tip to bottom circle
void JagGeo::sampleLinesOnCylinder( double x0, double y0, double z0, double r, double h, 
								    double nx, double ny, int num, JagVector<JagLine3D> &samples )
{
	if ( num < 1 ) return;
	double delta = 2*JAG_PI/(double)num;
	double alpha = 0.0;
	double x,y,z, gx, gy, gz;
	JagLine3D line;
	// tip
	for ( int i=0; i < num; ++i ) {
		x = r*cos(alpha);
		y = r*sin(alpha);
		z = h;
		transform3DCoordLocal2Global( x0, y0, z0, x, y, z, nx, ny, gx, gy, gz );
		line.x1 = gx; line.y1 = gy; line.z1 = gz;

		z = -h;
		transform3DCoordLocal2Global( x0, y0, z0, x, y, z, nx, ny, gx, gy, gz );
		line.x2 = gx; line.y2 = gy; line.z2 = gz;
		samples.append( line );
		alpha += delta;
	}
}


#if 0
Jstr JagGeo::convertType2Short( const Jstr &geotypeLong )
{
	const char *p = geotypeLong.c_str();
    if ( 0==strcasecmp(p, "point" ) ) {
		return JAG_C_COL_TYPE_POINT;
	} else if ( 0==strcasecmp(p, "point3d" ) ) {
		return JAG_C_COL_TYPE_POINT3D;
	} else if ( 0==strcasecmp(p, "line" ) ) {
		return JAG_C_COL_TYPE_LINE;
	} else if ( 0==strcasecmp(p, "line3d" ) ) {
		return JAG_C_COL_TYPE_LINE3D;
	} else if ( 0==strcasecmp(p, "circle" ) ) {
		return JAG_C_COL_TYPE_CIRCLE;
	} else if ( 0==strcasecmp(p, "circle3d" ) ) {
		return JAG_C_COL_TYPE_CIRCLE3D;
	} else if ( 0==strcasecmp(p, "sphere" ) ) {
		return JAG_C_COL_TYPE_SPHERE;
	} else if ( 0==strcasecmp(p, "square" ) ) {
		return JAG_C_COL_TYPE_SQUARE;
	} else if ( 0==strcasecmp(p, "square3d" ) ) {
		return JAG_C_COL_TYPE_SQUARE3D;
	} else if ( 0==strcasecmp(p, "cube" ) ) {
		return JAG_C_COL_TYPE_CUBE;
	} else if ( 0==strcasecmp(p, "rectangle" ) ) {
		return JAG_C_COL_TYPE_RECTANGLE;
	} else if ( 0==strcasecmp(p, "rectangle3d" ) ) {
		return JAG_C_COL_TYPE_RECTANGLE3D;
	} else if ( 0==strcasecmp(p, "bbox" ) ) {
		return JAG_C_COL_TYPE_BBOX;
	} else if ( 0==strcasecmp(p, "box" ) ) {
		return JAG_C_COL_TYPE_BOX;
	} else if ( 0==strcasecmp(p, "cone" ) ) {
		return JAG_C_COL_TYPE_CONE;
	} else if ( 0==strcasecmp(p, "triangle" ) ) {
		return JAG_C_COL_TYPE_TRIANGLE;
	} else if ( 0==strcasecmp(p, "triangle3d" ) ) {
		return JAG_C_COL_TYPE_TRIANGLE3D;
	} else if ( 0==strcasecmp(p, "cylinder" ) ) {
		return JAG_C_COL_TYPE_CYLINDER;
	} else if ( 0==strcasecmp(p, "ellipse" ) ) {
		return JAG_C_COL_TYPE_ELLIPSE;
	} else if ( 0==strcasecmp(p, "ellipse3d" ) ) {
		return JAG_C_COL_TYPE_ELLIPSE3D;
	} else if ( 0==strcasecmp(p, "ellipsoid" ) ) {
		return JAG_C_COL_TYPE_ELLIPSOID;
	} else if ( 0==strcasecmp(p, "polygon" ) ) {
		return JAG_C_COL_TYPE_POLYGON;
	} else if ( 0==strcasecmp(p, "polygon3d" ) ) {
		return JAG_C_COL_TYPE_POLYGON3D;
	} else if ( 0==strcasecmp(p, "linestring" ) ) {
		return JAG_C_COL_TYPE_LINESTRING;
	} else if ( 0==strcasecmp(p, "linestring3d" ) ) {
		return JAG_C_COL_TYPE_LINESTRING3D;
	} else if ( 0==strcasecmp(p, "multipoint" ) ) {
		return JAG_C_COL_TYPE_MULTIPOINT;
	} else if ( 0==strcasecmp(p, "multipoint3d" ) ) {
		return JAG_C_COL_TYPE_MULTIPOINT3D;
	} else if ( 0==strcasecmp(p, "multilinestring" ) ) {
		return JAG_C_COL_TYPE_MULTILINESTRING;
	} else if ( 0==strcasecmp(p, "multilinestring3d" ) ) {
		return JAG_C_COL_TYPE_MULTILINESTRING3D;
	} else if ( 0==strcasecmp(p, "multipolygon" ) ) {
		return JAG_C_COL_TYPE_MULTIPOLYGON;
	} else if ( 0==strcasecmp(p, "multipolygon3d" ) ) {
		return JAG_C_COL_TYPE_MULTIPOLYGON3D;
	} else if ( 0==strcasecmp(p, "range" ) ) {
		return JAG_C_COL_TYPE_RANGE;
	} else {
		return "UNKNOWN";
	}
}
#endif


// sp was shifted
double JagGeo::safeget( const JagStrSplit &sp, int arg )
{
	double res = 0.0;
	if ( sp.slength() >= arg+1 ) {
		res = jagatof( sp[arg].c_str() );
	}
	return res;
}

// sp was shifted
Jstr JagGeo::safeGetStr( const JagStrSplit &sp, int arg )
{
	Jstr res;
	if ( sp.slength() >= arg+1 ) {
		res = sp[arg];
	}
	return res;
}


bool JagGeo::validDirection( double nx )
{
	if ( nx > 1.0 ) return false;
	if ( nx < -1.0 ) return false;
	return true;
}

bool JagGeo::validDirection( double nx, double ny )
{
	if ( nx > 1.0 || ny > 1.0 ) return false;
	if ( nx < -1.0 || ny < -1.0 ) return false;
	if ( nx*nx + ny*ny > 1.0 ) return false;
	return true;
}



double JagGeo::doSign( const JagPoint2D &p1, const JagPoint2D &p2, const JagPoint2D &p3 )
{
	return (p1.x - p3.x) * ( p2.y - p3.y) - ( p2.x - p3.x) * ( p1.y - p3.y );
}

double JagGeo::distSquarePointToSeg( const JagPoint2D &p, const JagPoint2D &p1, const JagPoint2D &p2 )
{
    double sqlen =  (p2.x - p1.x)*(p2.x - p1.x) + (p2.y - p1.y)*(p2.y - p1.y);
    double dotproduct  = ((p.x - p1.x)*(p2.x - p1.x) + (p.y - p1.y)*(p2.y - p1.y)) / sqlen;
	if ( dotproduct < 0 ) {
  		return (p.x - p1.x)*(p.x - p1.x) + (p.y - p1.y)*(p.y - p1.y);
 	}

 	if ( dotproduct <= 1.0 ) {
  		double p_p1_squareLen = (p1.x - p.x)*(p1.x - p.x) + (p1.y - p.y)*(p1.y - p.y);
  		return ( p_p1_squareLen - dotproduct * dotproduct * sqlen );
 	}

    return (p.x - p2.x)*(p.x - p2.x) + (p.y - p2.y)*(p.y - p2.y);
}

double JagGeo::distance( double fx, double fy, double gx, double gy, int srid )
{
	double res;
	if ( JAG_GEO_WGS84 == srid )  {
		const Geodesic& geod = Geodesic::WGS84();
		// (fx fy) = ( lon, lat)
		geod.Inverse(fy, fx, gy, gx, res ); 
		d("s41108 wgs84 JagGeo::distance fx=%.3f fy=%.3f gx=%.3f gy=%.3f res=%.4f\n", fx, fy, gx, gy, res );
	} else {
		res = sqrt( (fx-gx)*(fx-gx) + (fy-gy )*(fy-gy) );
		d("s41109 plain JagGeo::distance fx=%.3f fy=%.3f gx=%.3f gy=%.3f res=%.4f\n", fx, fy, gx, gy, res );
	}

	return res;
}

double JagGeo::distance( double fx, double fy, double fz, 
						 double gx, double gy, double gz,  int srid )
{
	double res;
	if ( JAG_GEO_WGS84 == srid ) {
		double x1,y1,z1, x2,y2,z2;
		JagGeo::lonLatAltToXYZ( srid, fx,fy,fz, x1, y1, z1 );
		JagGeo::lonLatAltToXYZ( srid, gx,gy,gz, x2, y2, z2 );
		res = sqrt( (x1-x2)*(x1-x2) + (y1-y2 )*(y1-y2) + (z1-z2)*(z1-z2) );
	} else {
		res = sqrt( (fx-gx)*(fx-gx) + (fy-gy )*(fy-gy) + (fz-gz)*(fz-gz) );
	}
	return res;
}

// global to local new x'-y'  with just rotation
// coord sys turn clock wise. inx iny was in old sys x-y. outx/out in new coord sys x'-y'
// y' bcomes new unit vector direction
void JagGeo::rotate2DCoordGlobal2Local( double inx, double iny, double nx, double &outx, double &outy )
{
	if ( jagLE(nx, JAG_ZERO) || fabs(nx) > 1.0  ) {
		outx = inx;
		outy = iny;
		return;
	}

	double ny = sqrt( 1.0 - nx*nx );
	outx = inx*ny - iny*nx;
	outy = inx*nx + iny*ny;
}

// local to global
// coord sys turn counter-clock wise. inx iny was in x'-y'. outx/out in old coord  x-y.
// nx is unit vector inx iny is in unit vector-as-y axis plane,
// inx iny are relative locally on unitvector plane where unit vector is y-axis
// outx outy will be in x-y where unit vector is measured
void JagGeo::rotate2DCoordLocal2Global( double inx, double iny, double nx, double &outx, double &outy )
{
	if ( jagLE(nx, JAG_ZERO) || fabs(nx) > 1.0 ) {
		outx = inx;
		outy = iny;
		return;
	}

	double ny = sqrt( 1.0 - nx*nx );
	outx = inx*ny + iny*nx;
	outy = iny*ny - inx*nx;
}



// new z-aix is along unit vector
// local is new z-axis
void JagGeo::rotate3DCoordGlobal2Local( double inx, double iny, double inz, double nx, double ny,
                                    double &outx, double &outy, double &outz )
{
	double n2 = nx*nx + ny*ny;
	if ( jagEQ(n2, 0.0) || n2 > 1.0 ) {
		outx = inx;
		outy = iny;
		outz = inz;
		return;
	}

	double sqr = sqrt( n2 );
	double nz = sqrt(1.0 - n2);
	outx = (inx*ny - iny*nx)/sqr;
	outy = (inx*nx*nz + iny*ny*nz)/sqr - inz*sqr;
	outz = inx*nx + iny*ny + inz*nz;
}

// local to global
// given loccal coords on vector-as-z axis, find original real-world coords
// rotate around origin
void JagGeo::rotate3DCoordLocal2Global( double inx, double iny, double inz, double nx, double ny,
                                    double &outx, double &outy, double &outz )
{
	double n2 = nx*nx + ny*ny;
	if ( jagEQ(n2, 0.0) || n2 > 1.0 ) {
		outx = inx;
		outy = iny;
		outz = inz;
		return;
	}

	double sqr = sqrt( n2 );
	double nz = sqrt(1.0 - n2);
	outx = (inx*ny + iny*nx*nz)/sqr + inz*nz;
	outy = (-inx*nx + iny*ny*nz)/sqr + inz*ny;
	outz =  -iny*sqr + inz*nz;
}

// global to local
// inx0/iny0:  new coord origin
// inx iny are on nx plane locally, inx0 iny are its origin of nx plane wrf to real x-y system
void JagGeo::transform2DCoordGlobal2Local( double outx0, double outy0, double inx, double iny, double nx, 
							   double &outx, double &outy )
{
	inx -= outx0;
	iny -= outy0;
	rotate2DCoordGlobal2Local( inx, iny, nx, outx, outy );  // outx outy are in real world but wrf to nx plane
	// outx += inx0; // to real x-y sys
	// outy += iny0; // to real x-y sys
}

// coord sys turn counter-clock wise. inx iny was in x'-y'. outx/out in old coord  x-y.
// nx is unit vector inx iny is in unit vector-as-y axis plane,
// inx iny are relative locally on unitvector plane where unit vector is y-axis
// outx outy will be in x-y where unit vector is measured
// x0 y0 is coord of shape in x-y sys
// inx iny are in coord of x'-y' local system
// outx/outy are in xy-sys absolute coords
// local to global transform
void JagGeo::transform2DCoordLocal2Global( double x0, double y0, double inx, double iny, double nx, double &outx, double &outy )
{
	// void JagGeo::rotate2DCoordLocal2Global( double inx, double iny, double nx, double &outx, double &outy )
	rotate2DCoordLocal2Global( inx, iny, nx, outx, outy );
	outx += x0;
	outy += y0;
	/***
	d("s0738 transform2DCoordLocal2Global inx=%.4f iny=%.4f x0=%f y0=%f nx=%f outx=%f outy=%f\n",
			inx, iny, x0, y0, nx, outx, outy );
	***/
}

// global to local
// inx iny are on nx plane locally, inx0 iny are its origin of nx plane wrf to real x-y system
void JagGeo::transform3DCoordGlobal2Local( double outx0, double outy0, double outz0, double inx, double iny, double inz, 
							   double nx,  double ny,
							   double &outx, double &outy, double &outz )
{
	inx -= outx0;
	iny -= outy0;
	inz -= outz0;
	rotate3DCoordGlobal2Local( inx, iny, inz, nx, ny, outx, outy, outz );  // outx outy outz are in x'-y'-z'
}

// global to local
// inx iny are on nx plane locally, inx0 iny are its origin of nx plane wrf to real x-y system
void JagGeo::transform3DCoordLocal2Global( double inx0, double iny0, double inz0, double inx, double iny, double inz, 
							   double nx,  double ny,
							   double &outx, double &outy, double &outz )
{
	rotate3DCoordLocal2Global( inx, iny, inz, nx, ny, outx, outy, outz );  // outx outy outz are in x'-y'-z'
	outx += inx0; // to real x-y sys
	outy += iny0; // to real x-y sys
	outz += inz0; // to real x-y sys
}

double JagGeo::jagMin( double x1, double x2, double x3 )
{
	double m = jagmin(x1, x2);
	return  jagmin(m, x3);
}
double JagGeo::jagMax( double x1, double x2, double x3 )
{
	double m = jagmax(x1, x2);
	return  jagmax(m, x3);
}


bool JagGeo::bound3DDisjoint( double x1, double y1, double z1, double w1, double d1, double h1,
                double x2, double y2, double z2, double w2, double d2, double h2 )
{
	double mxd1 = jagMax(w1,d1,h1);
	double mxd2 = jagMax(w2,d2,h2);
	if (  x1+mxd1 < x2-mxd2 || x1-mxd1 > x2+mxd2 ) return true;
	if (  y1+mxd1 < y2-mxd2 || y1-mxd1 > y2+mxd2 ) return true;
	if (  z1+mxd1 < z2-mxd2 || z1-mxd1 > z2+mxd2 ) return true;
	return false;
}

bool JagGeo::bound2DDisjoint( double x1, double y1, double w1, double h1, double x2, double y2, double w2, double h2 )
{
	double mxd1 = jagmax(w1,h1);
	double mxd2 = jagmax(w2,h2);
	if (  x1+mxd1 < x2-mxd2 || x1-mxd1 > x2+mxd2 ) return true;
	if (  y1+mxd1 < y2-mxd2 || y1-mxd1 > y2+mxd2 ) return true;
	return false;
}



// point px py  line: x1 y1 x2 y2
double JagGeo::squaredDistancePoint2Line( double px, double py, double x1, double y1, double x2, double y2 )
{
	if ( jagEQ(x1, x2) && jagEQ(y1, y2)  ) {
		return (px-x1)*(px-x1) + (py-y1)*(py-y1);
	}

	if ( jagEQ(x1, x2) ) {
		return (px-x1)*(px-x1);
	}

	double f1 = (y2-y1)*px - (x2-x1)*py + x2*y1 - y2*x1;
	double f2 = (y2-y1)*(y2-y1) + (x2-x1)*(x2-x1);
	return f1*f1/f2;
}

// return 0: disjoint, no touching points
//        1: tagent with one touching point
//        2: insersect by two touching points
// Normal ellipse
// Can be applied to circle: a=b=r
int JagGeo::relationLineNormalEllipse( double x1, double y1, double x2, double y2, double a, double b )
{
	if ( jagmax(x1,x2) < -a ) return 0;
	if ( jagmax(y1,y2) < -b ) return 0;
	if ( jagmin(x1,x2) > a ) return 0;
	if ( jagmin(y1,y2) > b ) return 0;
	if ( jagEQ(a, 0.0) ) return 0;
	if ( jagEQ(b, 0.0) ) return 0;

	// line y = mx + c
	if ( jagEQ(x1,x2) ) {
		// vertical line
		if ( jagEQ(x1, -a) || jagEQ(x1, a) )  {
			// look at  y1 y2
			if ( jagmin(y1,y2) > 0 ) return 0;
			if ( jagmax(y1,y2) < 0 ) return 0;
			return 1;  // tangent
		} else if ( x1 < -a || x1 > a ) {
			return 0;  // disjoint
		} else {
			// b2 * x2 + a2 * y2 = a2 *b2
			// y2 = ( a2*b2 - b2*x2 ) /a2;
			// y1 = sqrt( a2*b2 - b2*x1^2 ) /a;
			// y2 = -y1;
			double sy1 = b*sqrt( 1.0 - x1*x1/(a*a) );
			double sy2 = -sy1;
			if ( jagmin(y1,y2) > jagmax(sy1,sy2) ) return 0;
			if ( jagmax(y1,y2) < jagmin(sy1,sy2) ) return 0;
			return 2;
		}
	}

	double m = (y2-y1)/(x2-x1);
	double c = y1 - m*x1;
	double D = a*a*m*m + b*b - c*c;
	if ( D < 0.0 ) return 0;
	double sqrtD = sqrt(D);

	if  ( jagEQ(D, 0.0) ) {
		double denom = c*c;
		double sx = -a*a*m*c/denom;
		double sy = b*b*c/denom;
		if ( jagmin(x1,x2) > sx ) return 0;
		if ( jagmax(x1,x2) < sx ) return 0;
		if ( jagmin(y1,y2) > sy ) return 0;
		if ( jagmax(y1,y2) < sy ) return 0;
		return 1;
	} 

	double denom = D + c*c;
	double sx1 = a*( -a*m*c + b*sqrtD )/denom;
	double sy1 = b*( b*c + a*m*sqrtD )/denom;

	double sx2 = a*( -a*m*c - b*sqrtD )/denom;
	double sy2 = b*( b*c - a*m*sqrtD )/denom;

	if ( jagmin(x1,x2) > jagmax(sx1,sx2) ) return 0;
	if ( jagmax(x1,x2) < jagmin(sx1,sx2) ) return 0;

	if ( jagmin(y1,y2) > jagmax(sy1,sy2) ) return 0;
	if ( jagmax(y1,y2) < jagmin(sy1,sy2) ) return 0;

	return 2;

}
	
// return 0: disjoint, no touching points
//        1: tagent with one touching point
//        2: insersect by two touching points
// rotated ellipse
// Can be applied to circle: a=b=r
int JagGeo::relationLineCircle( double x1, double y1, double x2, double y2,
								 double x0, double y0, double r )
{
    double px1, py1, px2, py2;
	px1 = x1 - x0;
	py1 = y1 - y0;
	px2 = x2 - x0;
	py2 = y2 - y0;
	return relationLineNormalEllipse(px1, py1, px2, py2, r, r );
}

// return 0: disjoint, no touching points
//        1: tagent with one touching point
//        2: insersect by two touching points
// rotated ellipse
// Can be applied to circle: a=b=r
int JagGeo::relationLineEllipse( double x1, double y1, double x2, double y2,
								 double x0, double y0, double a, double b, double nx )
{
    double px1, py1, px2, py2;
	transform2DCoordGlobal2Local( x0, y0, x1, y1, nx, px1, py1 );
	transform2DCoordGlobal2Local( x0, y0, x2, y2, nx, px2, py2 );
	return relationLineNormalEllipse(px1, py1, px2, py2, a, b );
}


// 3D point to line distance
// point px py pz  line: x1 y1 z1 x2 y2 z2
double JagGeo::squaredDistance3DPoint2Line( double px, double py, double pz, 
							double x1, double y1, double z1, double x2, double y2, double z2 )
{
	if ( jagEQ(x1, x2) && jagEQ(y1, y2) && jagEQ(z1, z2)  ) {
		return (px-x1)*(px-x1) + (py-y1)*(py-y1) + (pz-z1)*(pz-z1);
	}

	if ( jagEQ(x1, x2) && jagEQ(y1, y2) ) {
		return (px-x1)*(px-x1) + (py-y1)*(py-y1);
	}

	double a = x2-x1;
	double b= y2-y1;
	double c = z2-z1;
	double f1 = (y1-y2)*c - b*(z1-z2);
	double f2 = (z1-z2)*a - c*(x1-x2);
	double f3 = (x1-x2)*b - a*(y1-y2);
	return (f1*f1 + f2*f2 + f3*f3)/(a*a+b*b+c*c);
}

bool JagGeo::line3DIntersectLine3D( double x1, double y1, double z1, double x2, double y2, double z2,
							double px1, double py1, double pz1, double px2, double py2, double pz2 )
{
	//d("s6703 line3DIntersectLine3D x1=%.1f y1=%.1f z1=%.1f  x2=%.1f y2=%.1f z2=%.1f\n", x1,y1,z1,x2,y2,z2 );
	//d("s6703 line3DIntersectLine3D px1=%.1f py1=%.1f pz1=%.1f  px2=%.1f py2=%.1f pz2=%.1f\n", px1,py1,pz1,px2,py2,pz2 );
	double a1 = x2-x1;
	double b1= y2-y1;
	double c1 = z2-z1;

	double a2 = px2-px1;
	double b2= py2-py1;
	double c2 = pz2-pz1;

	double D = (px1-x1)*b1*c2 + (py1-y1)*c1*a2 + (pz1-z1)*b2*a1
	           - (px1-x1)*b2*c1 - (py1-y1)*a1*c2 - (pz1-z1)*b1*a2;

    d("s6704 D = %f\n", D );

	if ( fabs(D) < 0.001f ) {
		d("s7681 D is zero, return true\n" );
		return true;
	}
	d("s7681 D is not zero, return true\n" );
	return false;

}


// 1: tangent
// 2: intersect with two two touching points
// 0: disjoint
int JagGeo::relationLine3DSphere( double x1, double y1, double z1, double x2, double y2, double z2,
									double x3, double y3, double z3, double r )
{
	double a = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1);
	double b = 2*( (x2-x1)*(x1-x3) + (y2-y1)*(y1-y3) + (z2-z1)*(z1-z3) );
	double c = x3*x3 + y3*y3 + z3*z3 + x1*x1 + y1*y1 + z1*z1 - 2*(x3*x1+y3*y1+z3*z1) - r*r;

	double D = b*b - 4.0*a*c;
	if ( jagEQ(D, 0.0) ) {
		return 1;
	} else if ( D < 0.0 ) {
		return 0;
	} else {
		return 2;
	}

}

bool JagGeo::point3DWithinRectangle2D( double px, double py, double pz,
							double x0, double y0, double z0,
							double a, double b, double nx, double ny, bool strict )
{
	double locx, locy, locz;
	transform3DCoordGlobal2Local( x0, y0, z0, px, py, pz, nx, ny, locx, locy, locz );
	if ( locIn2DCenterBox( px,py,a,b, strict ) ) { return true; }
	return false;
}

void JagGeo::cornersOfRectangle( double w, double h, JagPoint2D p[] )
{
	p[0].x = -w; p[0].y = -h;
	p[1].x = -w; p[1].y = h;
	p[2].x = w; p[2].y = h;
	p[3].x = w; p[3].y =- h;
}

void JagGeo::cornersOfRectangle3D( double w, double h, JagPoint3D p[] )
{
	p[0].x = -w; p[0].y = -h; p[0].z = 0.0;
	p[1].x = -w; p[1].y = h; p[1].z = 0.0;
	p[2].x = w; p[2].y = h; p[2].z = 0.0;
	p[3].x = w; p[3].y =- h; p[3].z = 0.0;
}

void JagGeo::cornersOfBox( double w, double d, double h, JagPoint3D p[] )
{
	p[0].x = -w; p[0].y = -d; p[0].z = -h;
	p[1].x = -w; p[1].y = d; p[1].z = -h;
	p[2].x = w; p[2].y = d; p[2].z = -h;
	p[3].x = w; p[3].y = -d; p[3].z = -h;

	p[4].x = -w; p[4].y = -d; p[4].z = h;
	p[5].x = -w; p[5].y = d; p[5].z = h;
	p[6].x = w; p[6].y = d; p[6].z = h;
	p[7].x = w; p[7].y = -d; p[7].z = h;
}

void JagGeo::edgesOfRectangle( double w, double h, JagLine2D line[] )
{
	JagPoint2D p[4];
	cornersOfRectangle(w,h, p);

	line[0].x1 = p[0].x; 
	line[0].y1 = p[0].y; 
	line[0].x2 = p[1].x; 
	line[0].y2 = p[1].y; 

	line[1].x1 = p[1].x; 
	line[1].y1 = p[1].y; 
	line[1].x2 = p[2].x; 
	line[1].y2 = p[2].y; 

	line[2].x1 = p[2].x; 
	line[2].y1 = p[2].y; 
	line[2].x2 = p[3].x; 
	line[2].y2 = p[3].y; 

	line[3].x1 = p[3].x; 
	line[3].y1 = p[3].y; 
	line[3].x2 = p[1].x; 
	line[3].y2 = p[1].y; 
}

void JagGeo::edgesOfRectangle3D( double w, double h, JagLine3D line[] )
{
	JagLine2D line2d[4];
	edgesOfRectangle( w,h, line2d);
	for ( int i=0; i<4; ++i )
	{
		line[i].x1 = line2d[i].x1;
		line[i].y1 = line2d[i].y1;
		line[i].x2 = line2d[i].x2;
		line[i].y2 = line2d[i].y2;
	}
}


// line: 12 edges
void JagGeo::edgesOfBox( double w, double d, double h, JagLine3D line[] )
{
	JagPoint3D p[8];
	cornersOfBox( w,d,h, p );

	line[0].x1 = p[0].x; 
	line[0].y1 = p[0].y; 
	line[0].z1 = p[0].z; 
	line[0].x2 = p[1].x; 
	line[0].y2 = p[1].y; 
	line[0].z2 = p[1].z; 


	line[1].x1 = p[1].x; 
	line[1].y1 = p[1].y; 
	line[1].z1 = p[1].z; 
	line[1].x2 = p[2].x; 
	line[1].y2 = p[2].y; 
	line[1].z2 = p[2].z; 

	line[2].x1 = p[2].x; 
	line[2].y1 = p[2].y; 
	line[2].z1 = p[2].z; 
	line[2].x2 = p[3].x; 
	line[2].y2 = p[3].y; 
	line[2].z2 = p[3].z; 

	line[3].x1 = p[3].x; 
	line[3].y1 = p[3].y; 
	line[3].z1 = p[3].z; 
	line[3].x2 = p[0].x; 
	line[3].y2 = p[0].y; 
	line[3].z2 = p[0].z; 

	line[4].x1 = p[4].x; 
	line[4].y1 = p[4].y; 
	line[4].z1 = p[4].z; 
	line[4].x2 = p[5].x; 
	line[4].y2 = p[5].y; 
	line[4].z2 = p[5].z; 

	line[5].x1 = p[5].x; 
	line[5].y1 = p[5].y; 
	line[5].z1 = p[5].z; 
	line[5].x2 = p[6].x; 
	line[5].y2 = p[6].y; 
	line[5].z2 = p[6].z; 

	line[6].x1 = p[6].x; 
	line[6].y1 = p[6].y; 
	line[6].z1 = p[6].z; 
	line[6].x2 = p[7].x; 
	line[6].y2 = p[7].y; 
	line[6].z2 = p[7].z; 

	line[7].x1 = p[7].x; 
	line[7].y1 = p[7].y; 
	line[7].z1 = p[7].z; 
	line[7].x2 = p[4].x; 
	line[7].y2 = p[4].y; 
	line[7].z2 = p[4].z; 


	// vertical ones
	line[8].x1 = p[0].x; 
	line[8].y1 = p[0].y; 
	line[8].z1 = p[0].z; 
	line[8].x2 = p[4].x; 
	line[8].y2 = p[4].y; 
	line[8].z2 = p[4].z; 

	line[9].x1 = p[1].x; 
	line[9].y1 = p[1].y; 
	line[9].z1 = p[1].z; 
	line[9].x2 = p[5].x; 
	line[9].y2 = p[5].y; 
	line[9].z2 = p[5].z; 

	line[10].x1 = p[2].x; 
	line[10].y1 = p[2].y; 
	line[10].z1 = p[2].z; 
	line[10].x2 = p[6].x; 
	line[10].y2 = p[6].y; 
	line[10].z2 = p[6].z; 

	line[11].x1 = p[3].x; 
	line[11].y1 = p[3].y; 
	line[11].z1 = p[3].z; 
	line[11].x2 = p[7].x; 
	line[11].y2 = p[7].y; 
	line[11].z2 = p[7].z; 

}

void JagGeo::transform3DLinesLocal2Global( double x0, double y0, double z0, double nx0, double ny0, int num, JagLine3D line[] )
{
	JagLine3D L;
	double gx, gy, gz;
	for ( int i=0; i<num; ++i ) {
		transform3DCoordLocal2Global( x0,y0,z0, line[i].x1, line[i].y1, line[i].z1, nx0, ny0, gx, gy, gz );
		L.x1 = gx; L.y1 = gy; L.z1 = gz;
		transform3DCoordLocal2Global( x0,y0,z0, line[i].x2, line[i].y2, line[i].z2, nx0, ny0, gx, gy, gz );
		L.x2 = gx; L.y2 = gy; L.z2 = gz;
		line[i] = L;
	}
}

void JagGeo::transform3DLinesGlobal2Local( double x0, double y0, double z0, double nx0, double ny0, int num, JagLine3D line[] )
{
	JagLine3D L;
	double gx, gy, gz;
	for ( int i=0; i<num; ++i ) {
		transform3DCoordGlobal2Local( x0,y0,z0, line[i].x1, line[i].y1, line[i].z1, nx0, ny0, gx, gy, gz );
		L.x1 = gx; L.y1 = gy; L.z1 = gz;
		transform3DCoordLocal2Global( x0,y0,z0, line[i].x2, line[i].y2, line[i].z2, nx0, ny0, gx, gy, gz );
		L.x2 = gx; L.y2 = gy; L.z2 = gz;
		line[i] = L;
	}
}


void JagGeo::transform2DLinesLocal2Global( double x0, double y0, double nx0, int num, JagLine2D line[] )
{
	JagLine2D L;
	double gx, gy;
	for ( int i=0; i<num; ++i ) {
		transform2DCoordLocal2Global( x0,y0,line[i].x1, line[i].y1, nx0, gx, gy );
		L.x1 = gx; L.y1 = gy; 
		transform2DCoordLocal2Global( x0,y0,line[i].x2, line[i].y2, nx0, gx, gy );
		L.x2 = gx; L.y2 = gy;
		line[i] = L;
	}
}

void JagGeo::transform2DLinesGlobal2Local( double x0, double y0, double nx0, int num, JagLine2D line[] )
{
	JagLine2D L;
	double gx, gy;
	for ( int i=0; i<num; ++i ) {
		transform2DCoordGlobal2Local( x0,y0,line[i].x1, line[i].y1, nx0, gx, gy );
		L.x1 = gx; L.y1 = gy; 
		transform2DCoordGlobal2Local( x0,y0,line[i].x2, line[i].y2, nx0, gx, gy );
		L.x2 = gx; L.y2 = gy;
		line[i] = L;
	}
}

bool JagGeo::line3DIntersectNormalRectangle( const JagLine3D &line, double w, double h )
{
	if ( jagEQ(line.z1, 0.0) &&  jagEQ(line.z2, 0.0) ) {
		if ( jagGE(line.x1, -w) && jagLE(line.x1, w) && jagGE(line.y1, -h) && jagLE(line.y1, h) ) {
			return true;
		}
		if ( jagGE(line.x2, -w) && jagLE(line.x2, w) && jagGE(line.y2, -h) && jagLE(line.y2, h) ) {
			return true;
		}
	}

	if ( jagEQ(line.z1, line.z2) ) {
		return false;
	}

	double x0 = line.x1 + (line.x2-line.x1)*line.z1/(line.z2-line.z1);
	double y0 = line.y1 + (line.y2-line.y1)*line.z1/(line.z2-line.z1);

	if ( jagGE(x0, -w) && jagLE(x0, w) && jagGE(y0, -h) && jagLE( y0, h) ) {
		return true;
	}

	return false;
}


bool JagGeo::line3DIntersectRectangle3D( const JagLine3D &line, const JagRectangle3D &rect )
{
	return line3DIntersectRectangle3D(line, rect.x0, rect.y0, rect.z0, rect.a, rect.b, rect.nx, rect.ny );
}

// line has global coord
bool JagGeo::line3DIntersectRectangle3D( const JagLine3D &line, double x0, double y0, double z0, double w, double h, double nx, double ny )
{
	double locx, locy, locz;
	JagLine3D L;
	transform3DCoordGlobal2Local( x0,y0,z0, line.x1, line.y1, line.z1, nx, ny, locx, locy, locz );
	L.x1 = locx;
	L.y1 = locy;
	L.z1 = locz;

	transform3DCoordGlobal2Local( x0,y0,z0, line.x2, line.y2, line.z2, nx, ny, locx, locy, locz );
	L.x2 = locx;
	L.y2 = locy;
	L.z2 = locz;

	return line3DIntersectNormalRectangle(L, w, h ); 
}



bool JagGeo::line2DIntersectNormalRectangle( const JagLine2D &line, double w, double h )
{
	if ( jagGE(line.x1, -w) && jagLE(line.x1, w) && jagGE(line.y1, -h) && jagLE(line.y1, h) ) {
		return true;
	}
	if ( jagGE(line.x2, -w) && jagLE(line.x2, w) && jagGE(line.y2, -h) && jagLE(line.y2, h) ) {
		return true;
	}

	return false;
}

// line has global coord
bool JagGeo::line2DIntersectRectangle( const JagLine2D &line, double x0, double y0, double w, double h, double nx )
{
	double locx, locy;
	JagLine2D L;
	transform2DCoordGlobal2Local( x0,y0, line.x1, line.y1, nx, locx, locy );
	L.x1 = locx;
	L.y1 = locy;

	transform2DCoordGlobal2Local( x0,y0, line.x2, line.y2, nx, locx, locy );
	L.x2 = locx;
	L.y2 = locy;

	return line2DIntersectNormalRectangle(L, w, h ); 
}


bool JagGeo::line3DIntersectNormalEllipse( const JagLine3D &line, double w, double h )
{
	if ( jagEQ(line.z1, 0.0) &&  jagEQ(line.z2, 0.0) ) {
		if ( pointWithinNormalEllipse( line.x1, line.y1, w,h, false ) ) return true;
		if ( pointWithinNormalEllipse( line.x2, line.y2, w,h, false ) ) return true;
	}

	if ( jagEQ(line.z1, line.z2) ) {
		return false;
	}

	double x0 = line.x1 + (line.x2-line.x1)*line.z1/(line.z2-line.z1);
	double y0 = line.y1 + (line.y2-line.y1)*line.z1/(line.z2-line.z1);
	if ( pointWithinNormalEllipse( x0, y0, w,h, false ) ) return true;
	return false;
}

// line has global coord
bool JagGeo::line3DIntersectEllipse3D( const JagLine3D &line, double x0, double y0, double z0, double w, double h, double nx, double ny )
{
	double locx, locy, locz;
	JagLine3D L;
	transform3DCoordGlobal2Local( x0,y0,z0, line.x1, line.y1, line.z1, nx, ny, locx, locy, locz );
	L.x1 = locx;
	L.y1 = locy;
	L.z1 = locz;

	transform3DCoordGlobal2Local( x0,y0,z0, line.x2, line.y2, line.z2, nx, ny, locx, locy, locz );
	L.x2 = locx;
	L.y2 = locy;
	L.z2 = locz;

	return line3DIntersectNormalEllipse(L, w, h ); 
}

bool JagGeo::lineIntersectLine( double x1, double y1, double x2, double y2,
								double x3, double y3, double x4, double y4  )
{
	#if 0
	d("s7702 lineIntersectLine x1=%f y1=%f x2=%f y2=%f  -- x3=%f y3=%f x4=%f y4=%f\n",
		   x1, y1, x2, y2, x3, y3, x4, y4 );
   #endif

	if ( jagEQ( x1, x2) && jagEQ( x3, x4) ) {
		return false;
	}

	if ( jagEQ( x1, x2) ) {
		// y = mx+b
		double m = (y3-y4)/(x3-x4);
		double b = y4 - m*x4;
		double y0 = m*x1 + b;
		if ( jagLE(y0, jagmax(y1,y2) ) && jagGE(y0, jagmin(y1,y2) ) ) {
			//d("s6802 yes\n" );
			return true;
		} else {
			return false;
		}
	}

	if ( jagEQ( x3, x4) ) {
		// y = mx+b
		double m = (y1-y2)/(x1-x2);
		double b = y1 - m*x1;
		double y0 = m*x3 + b;
		if ( jagLE(y0, jagmax(y3,y4) ) && jagGE(y0, jagmin(y3,y4) ) ) {
			//d("s6803 yes\n" );
			return true;
		} else {
			return false;
		}
	}

	double A1 = (y1-y2)/(x1-x2);
	double A2 = (y3-y4)/(x3-x4);
	if ( jagEQ( A1, A2 ) ) {
		//d("s2091 A1=%f A2=%f false\n", A1,A2 );
		return false;
	}

	double b1 = y1 - A1*x1;
	double b2 = y3 - A2*x3;
	double Xa = (b2 - b1) / (A1 - A2);

	if ( (Xa < jagmax( jagmin(x1,x2), jagmin(x3,x4) )) || (Xa > jagmin( jagmax(x1,x2), jagmax(x3,x4) )) ) {
		//d("s7521 intersection is out of bound false\n" );
    	return false; // intersection is out of bound
	} else {
		//d("s6805 yes Xa=%f b1=%f b2=%f A1=%f A2=%f\n", Xa, b1, b2, A1, A2 );
    	return true;
	}
}

bool JagGeo::lineIntersectLine(  const JagLine2D &line1, const JagLine2D &line2 )
{
	return lineIntersectLine( line1.x1, line1.y1, line1.x2, line1.y2, line2.x1, line2.y1, line2.x2, line2.y2 );
}

void JagGeo::edgesOfTriangle( double x1, double y1, double x2, double y2, double x3, double y3, JagLine2D line[] )
{
	line[0].x1 = x1;
	line[0].y1 = y1;
	line[0].x2 = x2;
	line[0].y2 = y2;

	line[1].x1 = x2;
	line[1].y1 = y2;
	line[1].x2 = x3;
	line[1].y2 = y3;
	
	line[2].x1 = x3;
	line[2].y1 = y3;
	line[2].x2 = x1;
	line[2].y2 = y1;
}

void JagGeo::edgesOfTriangle3D( double x1, double y1, double z1,
								double x2, double y2, double z2,
								double x3, double y3, double z3,
								JagLine3D line[] )
{
	line[0].x1 = x1;
	line[0].y1 = y1;
	line[0].z1 = z1;
	line[0].x2 = x2;
	line[0].y2 = y2;
	line[0].z2 = z2;

	line[1].x1 = x2;
	line[1].y1 = y2;
	line[1].z1 = z2;
	line[1].x2 = x3;
	line[1].y2 = y3;
	line[1].z2 = z3;
	
	line[2].x1 = x3;
	line[2].y1 = y3;
	line[2].z1 = z3;
	line[2].x2 = x1;
	line[2].y2 = y1;
	line[2].z2 = z1;
}

void JagGeo::pointsOfTriangle3D( double x1, double y1, double z1,
								double x2, double y2, double z2,
								double x3, double y3, double z3, JagPoint3D p[] )
{
	p[0].x = x1;
	p[0].y = y1;
	p[0].z = z1;

	p[1].x = x2;
	p[1].y = y2;
	p[1].z = z2;

	p[2].x = x3;
	p[2].y = y3;
	p[2].z = z3;
}


bool JagGeo::line3DIntersectLine3D( const JagLine3D &line1, const JagLine3D &line2 )
{
	return line3DIntersectLine3D( line1.x1, line1.y1, line1.z1,  line1.x2, line1.y2, line1.z2,
								  line2.x1, line2.y1, line2.z1,  line2.x2, line2.y2, line2.z2 );
}

void JagGeo::transform3DEdgesLocal2Global( double x0, double y0, double z0, double nx0, double ny0, int num, JagPoint3D p[] )
{
	/***
	JagPoint3D P;
	double gx, gy, gz;
	for ( int i=0; i<num; ++i ) {
		transform3DCoordLocal2Global( x0,y0,z0, p[i].x, p[i].y, p[i].z, nx0, ny0, gx, gy, gz );
		P.x = gx; P.y = gy; P.z = gz;
		p[i] = P;
	}
	***/
	for ( int i=0; i<num; ++i ) {
		transform3DCoordLocal2Global( x0,y0,z0, p[i].x, p[i].y, p[i].z, nx0, ny0, p[i].x, p[i].y, p[i].z );
	}
}

void JagGeo::transform2DEdgesLocal2Global( double x0, double y0, double nx0, int num, JagPoint2D p[] )
{
	for ( int i=0; i<num; ++i ) {
		transform2DCoordLocal2Global( x0,y0, p[i].x, p[i].y, nx0, p[i].x, p[i].y );
	}
}

double JagGeo::distanceFromPoint3DToPlane(  double x, double y, double z, 
										   double x0, double y0, double z0, double nx0, double ny0 )
{
	double nz0 = sqrt(1.0- nx0*nx0 - ny0*ny0 );
	double D = (nx0*x0 + ny0*y0 + nz0* z0); 
	return fabs( nx0*x + ny0*y + nz0*z - D);
}

// local 6 six surfaces
void JagGeo::surfacesOfBox(double w, double d, double h, JagRectangle3D rect[] )
{
	// top
	rect[0].x0 = 0.0; rect[0].y0 = 0.0; rect[0].z0 = h;
	rect[0].nx = 0.0; rect[0].ny = 0.0; 
	rect[0].a = w; rect[0].b = d; 

	// right
	rect[1].x0 = w; rect[1].y0 = 0.0; rect[1].z0 = 0;
	rect[1].nx = 1.0; rect[1].ny =0.0; 
	rect[1].a = d; rect[0].b = h; 

	// bottom
	rect[2].x0 = 0; rect[2].y0 = 0.0; rect[2].z0 = -h;
	rect[2].nx = 0.0; rect[2].ny =0.0; 
	rect[2].a = w; rect[2].b = d; 

	// left
	rect[3].x0 = -w; rect[3].y0 = 0.0; rect[3].z0 = 0.0;
	rect[3].nx = 1.0; rect[3].ny =0.0; 
	rect[3].a = d; rect[3].b = h; 

	// front
	rect[4].x0 = 0.0; rect[4].y0 = -d; rect[4].z0 = 0.0;
	rect[4].nx = 0.0; rect[4].ny =1.0; 
	rect[4].a = h; rect[4].b = w; 

	// back
	rect[5].x0 = 0.0; rect[5].y0 = d; rect[5].z0 = 0.0;
	rect[5].nx = 0.0; rect[5].ny =1.0; 
	rect[5].a = h; rect[5].b = w; 
}

// local 6 six surfaces
void JagGeo::triangleSurfacesOfBox(double w, double d, double h, JagTriangle3D tri[] )
{
	// top
	tri[0].x1 = w; tri[0].y1 = d; tri[0].z1 = h;
	tri[0].x2 = w; tri[0].y2 = -d; tri[0].z2 = h;
	tri[0].x3 = -w; tri[0].y3 = d; tri[0].z3 = h;

	// right
	tri[1].x1 = w; tri[1].y1 = d; tri[1].z1 = h;
	tri[1].x2 = w; tri[1].y2 = -d; tri[1].z2 = h;
	tri[1].x3 = w; tri[1].y3 = d; tri[1].z3 = -h;

	// bottom
	tri[2].x1 = w; tri[2].y1 = d; tri[2].z1 = -h;
	tri[2].x2 = w; tri[2].y2 = -d; tri[2].z2 = -h;
	tri[2].x3 = -w; tri[2].y3 = d; tri[2].z3 = -h;

	// left
	tri[3].x1 = -w; tri[3].y1 = d; tri[3].z1 = h;
	tri[3].x2 = -w; tri[3].y2 = -d; tri[3].z2 = h;
	tri[3].x3 = -w; tri[3].y3 = d; tri[3].z3 = -h;

	// front
	tri[4].x1 = w; tri[4].y1 = -d; tri[4].z1 = h;
	tri[4].x2 = -w; tri[4].y2 = -d; tri[4].z2 = h;
	tri[4].x3 = -w; tri[4].y3 = -d; tri[4].z3 = -h;

	// back
	tri[4].x1 = w; tri[4].y1 = d; tri[4].z1 = h;
	tri[4].x2 = -w; tri[4].y2 = d; tri[4].z2 = h;
	tri[4].x3 = -w; tri[4].y3 = d; tri[4].z3 = -h;
}

bool JagGeo::bound3DLineBoxDisjoint( double x10, double y10, double z10,
									double x20, double y20, double z20,
									double x0, double y0, double z0, double w, double d, double h )
{
	// check max
	double mxd = jagMax(w,d,h);
	double xmax = jagmax(x10,x20);
	if ( xmax < x0-mxd ) return true;

	double ymax = jagmax(y10,y20);
	if ( ymax < y0-mxd ) return true;

	double zmax = jagmax(z10,z20);
	if ( zmax < z0-mxd ) return true;


	// check min
	double xmin = jagmin(x10,x20);
	if ( xmin > x0+mxd ) return true;

	double ymin = jagmin(y10,y20);
	if ( ymin > y0+mxd ) return true;

	double zmin = jagmin(z10,z20);
	if ( zmin > z0+mxd ) return true;

	return false;
}

bool JagGeo::bound2DLineBoxDisjoint( double x10, double y10, double x20, double y20, 
									double x0, double y0, double w, double h )
{

	// check max
	double mxd = jagmax(w,h);
	double xmax = jagmax(x10,x20);
	if ( xmax < x0-mxd ) return true;

	double ymax = jagmax(y10,y20);
	if ( ymax < y0-mxd ) return true;

	// check min
	double xmin = jagmin(x10,x20);
	if ( xmin > x0+mxd ) return true;

	double ymin = jagmin(y10,y20);
	if ( ymin > y0+mxd ) return true;

	return false;
}



// return 0: disjoint, no touching points
//        1: tagent with one touching point
//        2: insersect by two touching points
// Normal ellipse
// Can be applied to circle: a=b=r
int JagGeo::relationLine3DNormalEllipsoid( double x0, double y0, double z0,  double x1, double y1, double z1,
										  double a, double b, double c )
{
	if ( jagmax(x1,x0) < -a ) return 0;
	if ( jagmin(x1,x0) > a ) return 0;

	if ( jagmax(y1,y0) < -b ) return 0;
	if ( jagmin(y1,y0) > b ) return 0;

	if ( jagmax(z1,z0) < -c ) return 0;
	if ( jagmin(z1,z0) > c ) return 0;


	if ( jagEQ(a, 0.0) ) return 0;
	if ( jagEQ(b, 0.0) ) return 0;
	if ( jagEQ(c, 0.0) ) return 0;

	if ( jagEQ(x0, x1) ) {
		// line x = x0;
		// (y-y0)/(y1-y0) = (z-z0)/(z1-z0)
		// ellipse
		// x*x/aa + yy/bb + zz/cc =1
		// x0*x0/aa + yy/bb + zz/cc = 1
		// e = 1.0 - x0*x0/(a*a)
		// yy / bb + zz / cc  = 1-x0^2/aa
		// yy (e*b*b) + zz/ (e*c*c) = 1
		// new b is b*sqrt(e)   new c is c*sqrt(e)
		// normal ellipse , newa, newb, line y0z0 y1z1 if they intersect
		if ( x0 < -a || x0 > a ) return 0;
		double e = sqrt(1.0 - x0*x0/(a*a) );
		double newa = b*e; double newb = c*e;
		int rel = relationLineNormalEllipse( y0, z0, y1, z1, newa, newb );
		return rel;
	}

	// https://johannesbuchner.github.io/intersection/intersection_line_ellipsoid.html
	// x = x0 + t
	// y = y0 + kt
	// z = z0 + ht
	// normally: x = x0 + (x1-x0)t
	// normally: y = y0 + (y1-y0)t
	// normally: z = z0 + (z1-z0)t
	// now T = (x1-x0)t   t = T/(x1-x0)
	// we have x = x0 + T
	// y = y0 + (y1-y0)T/(x1-x0)
	// z = z0 + (y1-z0)T/(x1-x0)
	double k = (y1-y0)/(x1-x0);
	double h = (z1-z0)/(x1-x0);
	double a2=a*a;
	double b2=b*b;
	double c2=c*c;
	double k2=k*k;
	double h2=h*h;
	double x0sq=x0*x0;
	double y0sq=y0*y0;
	double z0sq=z0*z0;

/***
x = x0 + (-pow(a, 2)*pow(b, 2)*l*z0 - pow(a, 2)*pow(c, 2)*k*y0 - pow(b, 2)*pow(c, 2)*x0 + sqrt(pow(a, 2)*pow(b, 2)*pow(c, 2)*(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) - pow(a, 2)*pow(k, 2)*pow(z0, 2) + 2*pow(a, 2)*k*l*y0*z0 - pow(a, 2)*pow(l, 2)*pow(y0, 2) + pow(b, 2)*pow(c, 2) - pow(b, 2)*pow(l, 2)*pow(x0, 2) + 2*pow(b, 2)*l*x0*z0 - pow(b, 2)*pow(z0, 2) - pow(c, 2)*pow(k, 2)*pow(x0, 2) + 2*pow(c, 2)*k*x0*y0 - pow(c, 2)*pow(y0, 2))))/(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) + pow(b, 2)*pow(c, 2));
y = k*(-pow(a, 2)*pow(b, 2)*l*z0 - pow(a, 2)*pow(c, 2)*k*y0 - pow(b, 2)*pow(c, 2)*x0 + sqrt(pow(a, 2)*pow(b, 2)*pow(c, 2)*(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) - pow(a, 2)*pow(k, 2)*pow(z0, 2) + 2*pow(a, 2)*k*l*y0*z0 - pow(a, 2)*pow(l, 2)*pow(y0, 2) + pow(b, 2)*pow(c, 2) - pow(b, 2)*pow(l, 2)*pow(x0, 2) + 2*pow(b, 2)*l*x0*z0 - pow(b, 2)*pow(z0, 2) - pow(c, 2)*pow(k, 2)*pow(x0, 2) + 2*pow(c, 2)*k*x0*y0 - pow(c, 2)*pow(y0, 2))))/(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) + pow(b, 2)*pow(c, 2)) + y0;
z = l*(-pow(a, 2)*pow(b, 2)*l*z0 - pow(a, 2)*pow(c, 2)*k*y0 - pow(b, 2)*pow(c, 2)*x0 + sqrt(pow(a, 2)*pow(b, 2)*pow(c, 2)*(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) - pow(a, 2)*pow(k, 2)*pow(z0, 2) + 2*pow(a, 2)*k*l*y0*z0 - pow(a, 2)*pow(l, 2)*pow(y0, 2) + pow(b, 2)*pow(c, 2) - pow(b, 2)*pow(l, 2)*pow(x0, 2) + 2*pow(b, 2)*l*x0*z0 - pow(b, 2)*pow(z0, 2) - pow(c, 2)*pow(k, 2)*pow(x0, 2) + 2*pow(c, 2)*k*x0*y0 - pow(c, 2)*pow(y0, 2))))/(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) + pow(b, 2)*pow(c, 2)) + z0;
x = x0 - (pow(a, 2)*pow(b, 2)*l*z0 + pow(a, 2)*pow(c, 2)*k*y0 + pow(b, 2)*pow(c, 2)*x0 + sqrt(pow(a, 2)*pow(b, 2)*pow(c, 2)*(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) - pow(a, 2)*pow(k, 2)*pow(z0, 2) + 2*pow(a, 2)*k*l*y0*z0 - pow(a, 2)*pow(l, 2)*pow(y0, 2) + pow(b, 2)*pow(c, 2) - pow(b, 2)*pow(l, 2)*pow(x0, 2) + 2*pow(b, 2)*l*x0*z0 - pow(b, 2)*pow(z0, 2) - pow(c, 2)*pow(k, 2)*pow(x0, 2) + 2*pow(c, 2)*k*x0*y0 - pow(c, 2)*pow(y0, 2))))/(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) + pow(b, 2)*pow(c, 2));
y = -k*(pow(a, 2)*pow(b, 2)*l*z0 + pow(a, 2)*pow(c, 2)*k*y0 + pow(b, 2)*pow(c, 2)*x0 + sqrt(pow(a, 2)*pow(b, 2)*pow(c, 2)*(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) - pow(a, 2)*pow(k, 2)*pow(z0, 2) + 2*pow(a, 2)*k*l*y0*z0 - pow(a, 2)*pow(l, 2)*pow(y0, 2) + pow(b, 2)*pow(c, 2) - pow(b, 2)*pow(l, 2)*pow(x0, 2) + 2*pow(b, 2)*l*x0*z0 - pow(b, 2)*pow(z0, 2) - pow(c, 2)*pow(k, 2)*pow(x0, 2) + 2*pow(c, 2)*k*x0*y0 - pow(c, 2)*pow(y0, 2))))/(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) + pow(b, 2)*pow(c, 2)) + y0;
z = -l*(pow(a, 2)*pow(b, 2)*l*z0 + pow(a, 2)*pow(c, 2)*k*y0 + pow(b, 2)*pow(c, 2)*x0 + sqrt(pow(a, 2)*pow(b, 2)*pow(c, 2)*(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) - pow(a, 2)*pow(k, 2)*pow(z0, 2) + 2*pow(a, 2)*k*l*y0*z0 - pow(a, 2)*pow(l, 2)*pow(y0, 2) + pow(b, 2)*pow(c, 2) - pow(b, 2)*pow(l, 2)*pow(x0, 2) + 2*pow(b, 2)*l*x0*z0 - pow(b, 2)*pow(z0, 2) - pow(c, 2)*pow(k, 2)*pow(x0, 2) + 2*pow(c, 2)*k*x0*y0 - pow(c, 2)*pow(y0, 2))))/(pow(a, 2)*pow(b, 2)*pow(l, 2) + pow(a, 2)*pow(c, 2)*pow(k, 2) + pow(b, 2)*pow(c, 2)) + z0;

*****/
/**********
double sx1 = x0 + (-a2*b2*h*z0 - a2*c2*k*y0 - b2*c2*x0 + sqrt(a2*b2*c2*(a2*b2*h2 + a2*c2*k2 - a2*k2*z0sq 
			+ 2*a2*k*h*y0*z0 - a2*h2*y0sq + b2*c2 - b2*h2*x0sq + 2*b2*h*x0*z0 - b2*z0sq - c2*k2*x0sq 
			+ 2*c2*k*x0*y0 - c2*y0sq)))/(a2*b2*h2 + a2*c2*k2 + b2*c2);

double sy1 = k*(-a2*b2*h*z0 - a2*c2*k*y0 - b2*c2*x0 + sqrt(a2*b2*c2*(a2*b2*h2 + a2*c2*k2 - a2*k2*z0sq 
			+ 2*a2*k*h*y0*z0 - a2*h2*y0sq + b2*c2 - b2*h2*x0sq + 2*b2*h*x0*z0 - b2*z0sq - c2*k2*x0sq 
			+ 2*c2*k*x0*y0 - c2*y0sq)))/(a2*b2*h2 + a2*c2*k2 + b2*c2) + y0;

double sz1 = h*(-a2*b2*h*z0 - a2*c2*k*y0 - b2*c2*x0 + sqrt(a2*b2*c2*(a2*b2*h2 + a2*c2*k2 - a2*k2*z0sq 
			+ 2*a2*k*h*y0*z0 - a2*h2*y0sq + b2*c2 - b2*h2*x0sq + 2*b2*h*x0*z0 - b2*z0sq - c2*k2*x0sq 
			+ 2*c2*k*x0*y0 - c2*y0sq)))/(a2*b2*h2 + a2*c2*k2 + b2*c2) + z0;


double sx2 = x0 - (a2*b2*h*z0 + a2*c2*k*y0 + b2*c2*x0 + sqrt(a2*b2*c2*(a2*b2*h2 + a2*c2*k2 - a2*k2*z0sq 
			+ 2*a2*k*h*y0*z0 - a2*h2*y0sq + b2*c2 - b2*h2*x0sq + 2*b2*h*x0*z0 - b2*z0sq - c2*k2*x0sq 
			+ 2*c2*k*x0*y0 - c2*y0sq)))/(a2*b2*h2 + a2*c2*k2 + b2*c2);

double sy2 = -k*(a2*b2*h*z0 + a2*c2*k*y0 + b2*c2*x0 + sqrt(a2*b2*c2*(a2*b2*h2 + a2*c2*k2 - a2*k2*z0sq 
			+ 2*a2*k*h*y0*z0 - a2*h2*y0sq + b2*c2 - b2*h2*x0sq + 2*b2*h*x0*z0 - b2*z0sq - c2*k2*x0sq 
			+ 2*c2*k*x0*y0 - c2*y0sq)))/(a2*b2*h2 + a2*c2*k2 + b2*c2) + y0;

double sz2 = -h*(a2*b2*h*z0 + a2*c2*k*y0 + b2*c2*x0 + sqrt(a2*b2*c2*(a2*b2*h2 + a2*c2*k2 - a2*k2*z0sq 
			+ 2*a2*k*h*y0*z0 - a2*h2*y0sq + b2*c2 - b2*h2*x0sq + 2*b2*h*x0*z0 - b2*z0sq - c2*k2*x0sq 
			+ 2*c2*k*x0*y0 - c2*y0sq)))/(a2*b2*h2 + a2*c2*k2 + b2*c2) + z0;
**********/

	double D = a2*b2*h2 + a2*c2*k2 - a2*k2*z0sq 
			+ 2*a2*k*h*y0*z0 - a2*h2*y0sq + b2*c2 - b2*h2*x0sq + 2*b2*h*x0*z0 - b2*z0sq - c2*k2*x0sq 
			+ 2*c2*k*x0*y0 - c2*y0sq;

	if ( D < 0.0 ) return 0;

	double T1 = ( -a2*b2*h*z0 - a2*c2*k*y0 - b2*c2*x0 + sqrt(a2*b2*c2*D) )/(a2*b2*h2 + a2*c2*k2 + b2*c2);
	double T2 = ( a2*b2*h*z0 + a2*c2*k*y0 + b2*c2*x0 + sqrt(a2*b2*c2*D) )/(a2*b2*h2 + a2*c2*k2 + b2*c2);

	double s1x = x0 + T1;
	double s1y = y0 + k*T1;
	double s1z = z0 + h*T1;

	double s2x = x0 - T2;
	double s2y = y0 - k*T2;
	double s2z = z0 - h*T2;

	if ( jagEQ(D, 0.0) ) {
		// intersect tagent T1 = -T2 --> D=0
		if ( jagmax(x0,x1) <  s1x ) return 0;
		if ( jagmin(x0,x1) >  s1x ) return 0;
		if ( jagmax(y0,y1) <  s1y ) return 0;
		if ( jagmin(y0,y1) >  s1y ) return 0;
		if ( jagmax(z0,z1) <  s1z ) return 0;
		if ( jagmin(z0,z1) >  s1z ) return 0;
		return 1;
	}

	if ( jagmax(x0,x1) <  jagmin(s1x, s2x) ) return 0;
	if ( jagmin(x0,x1) >  jagmax(s1x, s2x)) return 0;
	if ( jagmax(y0,y1) <  jagmin(s1y, s2y) ) return 0;
	if ( jagmin(y0,y1) >  jagmax(s1y, s2y) ) return 0;
	if ( jagmax(z0,z1) <  jagmin(s1z, s2z) ) return 0;
	if ( jagmin(z0,z1) >  jagmax(s1z, s2z) ) return 0;

	return 2;
}

int JagGeo::relationLine3DEllipsoid( double x1, double y1, double z1, double x2, double y2, double z2,
                                                    double x0, double y0, double z0,
                                                    double a, double b, double c, double nx, double ny )
{
    double px1, py1, pz1, px2, py2, pz2;
	transform3DCoordGlobal2Local( x0, y0, z0, x1, y1, z1, nx, ny, px1, py1, pz1 );
	transform3DCoordGlobal2Local( x0, y0, z0, x2, y2, z2, nx, ny, px2, py2, pz2 );
	return relationLine3DNormalEllipsoid(px1, py1, pz1,  px2, py2, pz2, a, b, c );
}

// return 0: disjoint, no touching points
//        1: tagent with one touching point
//        2: insersect by two touching points
// Normal ellipse
// Can be applied to circle: a=b=r
// a: radius in center (1/2 radius at bottom circle)  c: height from center (1/2 real height)
int JagGeo::relationLine3DNormalCone( double x0, double y0, double z0,  double x1, double y1, double z1,
										  double a, double c )
{
	// a = b
	if ( jagmax(x1,x0) < -a ) return 0;
	if ( jagmin(x1,x0) > a ) return 0;

	if ( jagmax(y1,y0) < -a ) return 0;
	if ( jagmin(y1,y0) > a ) return 0;

	if ( jagmax(z1,z0) < -c ) return 0;
	if ( jagmin(z1,z0) > c ) return 0;


	if ( jagEQ(a, 0.0) ) return 0;
	if ( jagEQ(c, 0.0) ) return 0;


	// https://johannesbuchner.github.io/intersection/intersection_line_ellipsoid.html
	// x = x0 + t
	// y = y0 + kt
	// z = z0 + ht
	// normally: x = x0 + (x1-x0)t
	// normally: y = y0 + (y1-y0)t
	// normally: z = z0 + (z1-z0)t
	// now T = (x1-x0)t   t = T/(x1-x0)
	// we have x = x0 + T
	// y = y0 + (y1-y0)T/(x1-x0)
	// z = z0 + (y1-z0)T/(x1-x0)
	double k, h;
	if ( jagEQ( x0,x1 ) ) {
		k = y1-y0;
		h = z1-z0;
	} else {
		k = (y1-y0)/(x1-x0);
		h = (z1-z0)/(x1-x0);
	}

	// z*z * C*C = x*x + y*y
	// what is C?
	// in our coord system:
	//        /\
	//       /  \
	//      /    \
	//     /  0   \
	//    /        \
	//   /          \
	//   ------------
	// (c-z)/r = c/a   r = (c-z)a/c
	// r*r=x*x+y*y
	// x*x+y*y = r*r = (c-z)^2 * a^2 / c^2
	// Z= c-z  C = a/c
	//double Z = c-z;
	double C = a/c;

	//double a2=a*a;
	// double b2=b*b;
	double c2=C*C;
	double k2=k*k;
	double h2=h*h;
	double x0sq=x0*x0;
	double y0sq=y0*y0;
	double z0sq=z0*z0;

	// https://johannesbuchner.github.io/intersection/intersection_line_cone.html
	double D = c2*k2*z0sq - 2*c2*k*h*y0*z0 + c2*h2*(x0sq + y0sq) - 2*c2*h*x0*z0 + c2*z0sq - k2*x0sq + 2*k*x0*y0 - y0sq;
	if ( D < 0.0 ) return 0; // disjoint
	double sqrtD = sqrt(D);

	double f1 = c2*h*z0 - k*y0 -x0;
	double denom1 = -c2*h2 +k2 + 1.0;
	double denom2 = c2*h2 -k2 - 1.0;
	double f2 = (f1+sqrtD)/denom1;
	double sx1 = x0 + f2;
	double sy1 = y0 + k*f2;
	double sz1 = z0 + h*f2;
	sz1 = c - sz1; // convert to our z-axis

	if ( jagEQ(D, 0.0) ) {
		// intersect tagent T1 = -T2 --> D=0
		if ( jagmax(x0,x1) <  sx1 ) return 0;
		if ( jagmin(x0,x1) >  sx1 ) return 0;
		if ( jagmax(y0,y1) <  sy1 ) return 0;
		if ( jagmin(y0,y1) >  sy1 ) return 0;
		if ( jagmax(z0,z1) <  sz1 ) return 0;
		if ( jagmin(z0,z1) >  sz1 ) return 0;
		return 1;
	}

	f2 = (-f1+sqrtD)/denom2;
	double sx2 = x0 + f2;
	double sy2 = y0 + k*f2;
	double sz2 = z0 + h*f2;
	sz2 = c - sz2; // convert to our z-axis

	if ( jagmax(x0,x1) <  jagmin(sx1, sx2) ) return 0;
	if ( jagmin(x0,x1) >  jagmax(sx1, sx2)) return 0;
	if ( jagmax(y0,y1) <  jagmin(sy1, sy2) ) return 0;
	if ( jagmin(y0,y1) >  jagmax(sy1, sy2) ) return 0;
	if ( jagmax(z0,z1) <  jagmin(sz1, sz2) ) return 0;
	if ( jagmin(z0,z1) >  jagmax(sz1, sz2) ) return 0;

	return 2;
}


// return 0: disjoint, no touching points
//        1: tagent with one touching point
//        2: insersect by two touching points
// Normal ellipse
// Can be applied to circle: a=b=r
// a: radius in center (1/2 radius at bottom circle)  c: height from center (1/2 real height)
int JagGeo::relationLine3DNormalCylinder( double x0, double y0, double z0,  double x1, double y1, double z1,
										  double a, double b, double c )
{
	// a = b
	if ( jagmax(x1,x0) < -a ) return 0;
	if ( jagmin(x1,x0) > a ) return 0;

	if ( jagmax(y1,y0) < -b ) return 0;
	if ( jagmin(y1,y0) > b ) return 0;

	if ( jagmax(z1,z0) < -c ) return 0;
	if ( jagmin(z1,z0) > c ) return 0;


	if ( jagEQ(a, 0.0) ) return 0;
	if ( jagEQ(b, 0.0) ) return 0;
	if ( jagEQ(c, 0.0) ) return 0;


	// https://johannesbuchner.github.io/intersection/intersection_line_cylinder.html
	// x = x0 + t
	// y = y0 + kt
	// z = z0 + ht
	// normally: x = x0 + (x1-x0)t
	// normally: y = y0 + (y1-y0)t
	// normally: z = z0 + (z1-z0)t
	// now T = (x1-x0)t   t = T/(x1-x0)
	// we have x = x0 + T
	// y = y0 + (y1-y0)T/(x1-x0)
	// z = z0 + (y1-z0)T/(x1-x0)
	double k, h;
	if ( jagEQ( x0,x1 ) ) {
		k = y1-y0;
		h = z1-z0;
	} else {
		k = (y1-y0)/(x1-x0);
		h = (z1-z0)/(x1-x0);
	}

	double a2=a*a;
	double b2=b*b;
	double k2=k*k;
	//double h2=h*h;

	double x0sq=x0*x0;
	double y0sq=y0*y0;

	double D = a2*b2*(a2*k2+b2-k2*x0sq+2*k*x0*y0-y0sq);
	if ( D < 0.0 ) return 0; // disjoint
	double sqrtD = sqrt(D);
	double denom = a2*k2+b2;

	double f1 = a2*k*y0 + b2*x0;
	double f2 = (-f1 + sqrtD )/denom;
	double sx1 = x0 + f2;
	double sy1 = y0 + k*f2;
	double sz1 = z0 + h*f2;

	if ( jagEQ(D, 0.0) ) {
		// intersect tagent T1 = -T2 --> D=0
		if ( jagmax(x0,x1) <  sx1 ) return 0;
		if ( jagmin(x0,x1) >  sx1 ) return 0;
		if ( jagmax(y0,y1) <  sy1 ) return 0;
		if ( jagmin(y0,y1) >  sy1 ) return 0;
		if ( jagmax(z0,z1) <  sz1 ) return 0;
		if ( jagmin(z0,z1) >  sz1 ) return 0;
		return 1;
	}

	f2 = (f1+sqrtD)/denom;
	double sx2 = x0 + f2;
	double sy2 = y0 + k*f2;
	double sz2 = z0 + h*f2;

	if ( jagmax(x0,x1) <  jagmin(sx1, sx2) ) return 0;
	if ( jagmin(x0,x1) >  jagmax(sx1, sx2)) return 0;
	if ( jagmax(y0,y1) <  jagmin(sy1, sy2) ) return 0;
	if ( jagmin(y0,y1) >  jagmax(sy1, sy2) ) return 0;
	if ( jagmax(z0,z1) <  jagmin(sz1, sz2) ) return 0;
	if ( jagmin(z0,z1) >  jagmax(sz1, sz2) ) return 0;

	return 2;
}

int JagGeo::relationLine3DCone( double x1, double y1, double z1, double x2, double y2, double z2,
                                double x0, double y0, double z0,
                                double a, double c, double nx, double ny )
{
    double px1, py1, pz1, px2, py2, pz2;
	transform3DCoordGlobal2Local( x0, y0, z0, x1, y1, z1, nx, ny, px1, py1, pz1 );
	transform3DCoordGlobal2Local( x0, y0, z0, x2, y2, z2, nx, ny, px2, py2, pz2 );
	return relationLine3DNormalCone(px1, py1, pz1,  px2, py2, pz2, a, c );
}

int JagGeo::relationLine3DCylinder( double x1, double y1, double z1, double x2, double y2, double z2,
                                double x0, double y0, double z0,
                                double a, double b, double c, double nx, double ny )
{
    double px1, py1, pz1, px2, py2, pz2;
	transform3DCoordGlobal2Local( x0, y0, z0, x1, y1, z1, nx, ny, px1, py1, pz1 );
	transform3DCoordGlobal2Local( x0, y0, z0, x2, y2, z2, nx, ny, px2, py2, pz2 );
	return relationLine3DNormalCylinder(px1, py1, pz1,  px2, py2, pz2, a, b, c );
}

void JagGeo::triangle3DNormal( double x1, double y1, double z1, double x2, double y2, double z2,
								 double x3, double y3, double z3, double &nx, double &ny )
{
	if ( jagEQ(x1,x2) && jagEQ(y1,y2) && jagEQ(z1,z2)
			&& jagEQ(x1,x3) && jagEQ(y1,y3) && jagEQ(z1,z3)
	     ) {

		nx = 0.0; ny=0.0;
		return;
	}

	double NX = (y2-y1)*(z3-z1) - (z2-z1)*(y3-y1);
	double NY = (z2-z1)*(x3-x1) - (x2-x1)*(z3-z1);
	double NZ = (x2-x1)*(y3-y1) - (y2-y1)*(x3-x1);
	double SQ = sqrt(NX*NX + NY*NY + NZ*NZ);
	nx = NX/SQ;  ny=NY/SQ;
}


double JagGeo::distancePoint3DToPlane( double x, double y, double z, double A, double B, double C, double D )
{
	return fabs( A*x + B*y + C*z + D)/sqrt(A*A + B*B + C*C);
}

void JagGeo::planeABCDFromNormal( double x0, double y0, double z0, double nx, double ny,
								double &A, double &B, double &C, double &D )
{
	//nx*(x-x0) + ny*(y-y0) + nz*(z-z0) = 0;
	// nx*x - nx*x0 + ny*y - ny*y + nz*z - nz*z0 = 0;
	// nx*x + ny*y + nz*z - (nx*x0+ny*y0+nz*z0) = 0;
	double nz = sqrt( 1.0 - nx*nx - ny*ny );
	A=nx;
	B=ny;
	C=nz;
	D =-(nx*x0+ny*y0+nz*z0);

	if ( A < 0.0 ) { A = -A; B = -B; C = -C; D = -D; }
}

// plane: Ax + By + Cz + D = 0;
// ellipsoid  x^2/a^2 + y^2/b^2 + z^2/c^2 = 1
bool JagGeo::planeIntersectNormalEllipsoid( double A, double B, double C, double D, double a,  double b,  double c )
{
	if ( jagIsZero(A)  && jagIsZero(B) && jagIsZero(C) &&
	     jagIsZero(a)  && jagIsZero(b) && jagIsZero(c)    ) {
		 return false;
	}

	double a2 = a*a;
	double b2 = b*b;
	double c2 = c*c;

	double t = 1.0/sqrt( A*A*a2 + B*B*b2 + C*C*c2 ); 
	double x1 = -A*a2*t;
	double y1 = -B*b2*t;
	double z1 = -C*c2*t;

	double D1 = (A*x1 + B*y1 + C*z1 );
	double D2 = -D1;
	double minD = jagmin(D1, D2);
	double maxD = jagmax(D1, D2);
	if ( jagLE( D, maxD) && jagGE(D, minD) ) {
		return true;
	}

	return false;
}


void JagGeo::triangle3DABCD( double x1, double y1, double z1, double x2, double y2, double z2,
 							 double x3, double y3, double z3, double &A, double &B, double &C, double &D )
{
	A = (y2-y1)*(z3-z1) - (y3-y1)*(z2-z1);
	B = (z2-z1)*(x3-x1) - (z3-z1)*(x2-x1);
	C = (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1);
	D = -(A* x1 + B* y1 + C* z1 );
	if ( A < 0.0 ) { A = -A; B = -B; C = -C; D = -D; }
}

bool JagGeo::planeIntersectNormalCone( double A, double B, double C, double D, double R,  double h )
{
	if ( jagLE(h, 0.0) || jagLE(R, 0.0) ) return false;
	double c = R/h;
	double c2 = c*c;
	double A2 = A*A;
	double B2 = B*B;
	double C2 = C*C;
	double  denom = A2 + B2 - C2/c2;

	if ( jagLE( denom, 0.0) ) return false;
	if ( jagIsZero(A) && jagIsZero(B) ) {
		 if (  jagIsZero(C) ) return false;
		// horizontal flat surface
		if ( -D/C > h || -D/C < -h ) return false;
		return true;
	}

	// c2= c*c
	// cone surface equation: x^2+y^2 = c^2(h-z)^2   c=R/h
	// -h <= z <= h  --> 2h >= h-z >= 0
	// x^2+y^2 >= 0
	// x^2+y^2 <= 4* c2* h2  // a ringle wihg radius ch and 2ch  (0, 2R)
	// Cz = -(Ax+By+D)
	// -h  <= z <= h       -Ch <= Cz <= Ch
	// Ch >= -Cz >= -Ch
	// -Ch <= Ax + By +D <= Ch
	// -Ch-D <= Ax + By <= Ch -D
	double d2 = fabs(C*h - D);
	double d1 = fabs(C*h + D);
	double mind = jagmin(d2, d1 );
	double dist2 = mind*mind/(A2+B2); 
	if ( dist2 > 2.0*R ) {
		return false;
	}
	return true;
}

bool JagGeo::doAllNearby( const Jstr& mark1, const Jstr &colType1, int srid1, const JagStrSplit &sp1,
                         const Jstr& mark2,  const Jstr &colType2, int srid2, const JagStrSplit &sp2, 
						 const Jstr &carg )
{
	d("s0233 doAllNearby srid1=%d srid2=%d carg=[%s]\n", srid1, srid2, carg.c_str() );

	int dim1 = getDimension( colType1 ); 
	int dim2 = getDimension( colType2 ); 
	if ( dim1 != dim2 ) {
		d("s2039 dim diff\n" );
		return false;
	}

	if ( srid1 != srid2 ) {
		d("s2039 srid diff srid1=%d srid2=%d\n", srid1, srid2 );
		return false;
	}

	double r = jagatof( carg.c_str() );
	double px,py,pz, x,y,z, R1x, R1y, R1z, R2x, R2y, R2z;
	getCoordAvg( colType1, sp1, px,py,pz, R1x, R1y, R1z );
	getCoordAvg( colType2, sp2, x,y,z, R2x, R2y, R2z );
	d("s1029 r=%.2f px=%.2f py=%.2f pz=%.2f    x=%.2f y=%.2f z=%.2f\n", r, px,py,pz, x,y,z );

	if ( 0 == srid1 ) {
    	if ( 2 == dim1 ) {
    		if ( bound2DDisjoint( px, py, R1x,R1y,  x, y, R2x+r,R2y+r ) ) {
    			d("s0291 bound2DDisjoint true\n" );
    			return false;
    		}
    	} else {
    		if ( bound3DDisjoint( px, py, pz, R1x,R1y,R1z,  x, y, z, R2x+r,R2y+r,R2z+r ) ) {
    			d("s0231 bound3DDisjoint true\n" );
    			return false;
    		}
    	}
	} else if ( JAG_GEO_WGS84 == srid1 ) {
    	if ( 2 == dim1 ) {
			double lon = meterToLon( srid1, r, x, y );
			double lat = meterToLat( srid1, r, x, y );
    		if ( bound2DDisjoint( px, py, R1x,R1y,  x, y, R2x+lon,R2y+lat ) ) {
    			d("s0291 bound2DDisjoint true\n" );
    			return false;
    		}
    	} else {
    		if ( bound3DDisjoint( px, py, pz, R1x,R1y,R1z,  x, y, z, R2x+r,R2y+r,R2z+r ) ) {
    			d("s0231 bound3DDisjoint true\n" );
    			return false;
    		}
    	}
	}

	double dist;
	if ( 2 == dim1 ) {
		dist = distance( px,py, x,y, srid1 );
		d("s1737 dist=%.2f r=%.2f\n", dist, r );
		if ( jagLE( dist, r ) ) return true; 
	} else if ( 3 == dim1 ) {
		dist = distance( px,py,pz, x,y,z, srid1 );
		if ( jagLE( dist, r ) ) return true; 
	}
	d("s3009 dist=[%f] r=[%f]\n", dist, r );
	
	return false;
}

void JagGeo::getCoordAvg( const Jstr &colType, const JagStrSplit &sp, double &x, double &y, double &z, 
							double &Rx, double &Ry, double &Rz )
{
	double px1 = jagatof( sp[JAG_SP_START+0].c_str() );
	double py1 = jagatof( sp[JAG_SP_START+1].c_str() );
	double pz1 = jagatof( sp[JAG_SP_START+2].c_str() );
	x = px1;
	y = py1;
	z = pz1;
	Rx = Ry = Rz = 0.0;

	if ( colType == JAG_C_COL_TYPE_LINE ) {
		double px2 = jagatof( sp[JAG_SP_START+2].c_str() );
		double py2 = jagatof( sp[JAG_SP_START+3].c_str() );
		x = (px1+px2)/2.0;
		y = (py1+py2)/2.0;
		Rx = jagmax( fabs(x-px1), fabs(x-px2) );
		Ry = jagmax( fabs(y-py1), fabs(y-py2) );
	} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
		double px2 = jagatof( sp[JAG_SP_START+3].c_str() );
		double py2 = jagatof( sp[JAG_SP_START+4].c_str() );
		double pz2 = jagatof( sp[JAG_SP_START+5].c_str() );
		double px3 = jagatof( sp[JAG_SP_START+6].c_str() );
		double py3 = jagatof( sp[JAG_SP_START+7].c_str() );
		double pz3 = jagatof( sp[JAG_SP_START+8].c_str() );
		x = (px1+px2+px3)/3.0;
		y = (py1+py2+py3)/3.0;
		z = (pz1+pz2+pz3)/3.0;
		Rx = jagMax( fabs(x-px1), fabs(x-px2), fabs(x-px3) );
		Ry = jagMax( fabs(y-py1), fabs(y-py2), fabs(y-py3) );
		Rz = jagMax( fabs(z-pz1), fabs(z-pz2), fabs(z-pz3) );
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
		double px2 = jagatof( sp[JAG_SP_START+2].c_str() );
		double py2 = jagatof( sp[JAG_SP_START+3].c_str() );
		double px3 = jagatof( sp[JAG_SP_START+4].c_str() );
		double py3 = jagatof( sp[JAG_SP_START+5].c_str() );
		x = (px1+px2+px3)/3.0;
		y = (py1+py2+py3)/3.0;
		Rx = jagMax( fabs(x-px1), fabs(x-px2), fabs(x-px3) );
		Ry = jagMax( fabs(y-py1), fabs(y-py2), fabs(y-py3) );
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
		double px2 = jagatof( sp[JAG_SP_START+3].c_str() );
		double py2 = jagatof( sp[JAG_SP_START+4].c_str() );
		double pz2 = jagatof( sp[JAG_SP_START+5].c_str() );
		double px3 = jagatof( sp[JAG_SP_START+6].c_str() );
		double py3 = jagatof( sp[JAG_SP_START+7].c_str() );
		double pz3 = jagatof( sp[JAG_SP_START+8].c_str() );
		x = (px1+px2+px3)/3.0;
		y = (py1+py2+py3)/3.0;
		z = (pz1+pz2+pz3)/3.0;
		Rx = jagMax( fabs(x-px1), fabs(x-px2), fabs(x-px3) );
		Ry = jagMax( fabs(y-py1), fabs(y-py2), fabs(y-py3) );
		Rz = jagMax( fabs(z-pz1), fabs(z-pz2), fabs(z-pz3) );
	} else if ( colType == JAG_C_COL_TYPE_CUBE || colType == JAG_C_COL_TYPE_SPHERE || colType == JAG_C_COL_TYPE_CIRCLE3D ) {
		Rx = jagatof( sp[JAG_SP_START+3].c_str() );
		Rz = Ry = Rx;
	} else if ( colType == JAG_C_COL_TYPE_BOX || colType == JAG_C_COL_TYPE_ELLIPSOID ) {
		Rx = jagatof( sp[JAG_SP_START+3].c_str() );
		Ry = jagatof( sp[JAG_SP_START+4].c_str() );
		Rz = jagatof( sp[JAG_SP_START+5].c_str() );
	} else if ( colType == JAG_C_COL_TYPE_CONE || colType ==  JAG_C_COL_TYPE_RECTANGLE3D ) {
		Rx = Ry = jagatof( sp[JAG_SP_START+3].c_str() );
		Rz = jagatof( sp[JAG_SP_START+4].c_str() );
	} else if ( colType == JAG_C_COL_TYPE_CIRCLE || colType == JAG_C_COL_TYPE_SQUARE ) {
		Rx = Ry = jagatof( sp[JAG_SP_START+2].c_str() );
	} else if ( colType == JAG_C_COL_TYPE_RECTANGLE ||  colType == JAG_C_COL_TYPE_ELLIPSE ) {
		Rx = jagatof( sp[JAG_SP_START+2].c_str() );
		Ry = jagatof( sp[JAG_SP_START+3].c_str() );
	}
}

bool JagGeo::bound2DTriangleDisjoint( double px1, double py1, double px2, double py2, double px3, double py3,
							  double px0, double py0, double w, double h )
{
	double mxd = jagmax(w,h);
	double m = jagMin(px1, px2, px3 );
	if ( px0 < m-mxd ) return true;

	m = jagMin(py1, py2, py3 );
	if ( py0 < m-mxd ) return true;

	m = jagMax(px1, px2, px3 );
	if ( px0 > m+mxd ) return true;

	m = jagMax(py1, py2, py3 );
	if ( px0 > m+mxd ) return true;

	return false;
}

bool JagGeo::bound3DTriangleDisjoint( double px1, double py1, double pz1, double px2, double py2, double pz2,
									  double px3, double py3, double pz3,
							  double px0, double py0, double pz0, double w, double d, double h )
{
	double mxd = jagMax(w,d,h);
	double m = jagMin(px1, px2, px3 );
	if ( px0 < m-mxd ) return true;

	m = jagMin(py1, py2, py3 );
	if ( py0 < m-mxd ) return true;

	m = jagMin(pz1, pz2, pz3 );
	if ( pz0 < m-mxd ) return true;



	m = jagMax(px1, px2, px3 );
	if ( px0 > m+mxd ) return true;

	m = jagMax(py1, py2, py3 );
	if ( py0 > m+mxd ) return true;

	m = jagMax(pz1, pz2, pz3 );
	if ( pz0 > m+mxd ) return true;

	return false;
}

void JagGeo::triangleRegion( double x1, double y1, double x2, double y2, double x3, double y3,
							 double &x0, double &y0, double &Rx, double &Ry )
{
	x0 = (x1+x2+x3)/3.0;
	y0 = (y1+y2+y3)/3.0;

	double m, R1, R2;

	m = jagMin(x1,x2,x3);
	R1 = fabs(x0-m);
	m = jagMax(x1,x2,x3);
	R2 = fabs(m-x0);
	Rx = jagmax(R1, R2 );

	m = jagMin(y1,y2,y3);
	R1 = fabs(y0-m);
	m = jagMax(y1,y2,y3);
	R2 = fabs(m-y0);
	Ry = jagmax(R1, R2 );

}

void JagGeo::triangle3DRegion( double x1, double y1, double z1,
                               double x2, double y2, double z2,
                               double x3, double y3, double z3,
                               double &x0, double &y0, double &z0, double &Rx, double &Ry, double &Rz )
{
	x0 = (x1+x2+x3)/3.0;
	y0 = (y1+y2+y3)/3.0;
	z0 = (z1+z2+z3)/3.0;

	double m, R1, R2;

	m = jagMin(x1,x2,x3);
	R1 = fabs(x0-m);
	m = jagMax(x1,x2,x3);
	R2 = fabs(m-x0);
	Rx = jagmax(R1, R2 );

	m = jagMin(y1,y2,y3);
	R1 = fabs(y0-m);
	m = jagMax(y1,y2,y3);
	R2 = fabs(m-y0);
	Ry = jagmax(R1, R2 );

	m = jagMin(z1,z2,z3);
	R1 = fabs(z0-m);
	m = jagMax(z1,z2,z3);
	R2 = fabs(m-z0);
	Rz = jagmax(R1, R2 );

}

void JagGeo::boundingBoxRegion( const Jstr &bbxstr, double &bbx, double &bby, double &brx, double &bry )
{
	// "xmin:ymin:zmin:xmax:ymax:zmax"
	double xmin, ymin, xmax, ymax;
	JagStrSplit s( bbxstr, ':');
	if ( s.length() == 4 ) {
		xmin = jagatof( s[0].c_str() );
		ymin = jagatof( s[1].c_str() );
		xmax = jagatof( s[2].c_str() );
		ymax = jagatof( s[3].c_str() );
	} else {
		xmin = jagatof( s[0].c_str() );
		ymin = jagatof( s[1].c_str() );
		xmax = jagatof( s[4].c_str() );
		ymax = jagatof( s[5].c_str() );
	}

	//d("s3376 xmin=%.2f ymin=%.2f xmax=%.2f ymax=%.2f\n", xmin, ymin, xmax, ymax );

	bbx = ( xmin + xmax ) /2.0;
	bby = ( ymin + ymax ) /2.0;
	brx = bbx - xmin;
	bry = bby - ymin;

}


void JagGeo::boundingBox3DRegion( const Jstr &bbxstr, double &bbx, double &bby, double &bbz,
								  double &brx, double &bry, double &brz )
{
	// "xmin:ymin:xmax:ymax"
	JagStrSplit s( bbxstr, ':');
	double xmin = jagatof( s[0].c_str() );
	double ymin = jagatof( s[1].c_str() );
	double zmin = jagatof( s[2].c_str() );
	double xmax = jagatof( s[3].c_str() );
	double ymax = jagatof( s[4].c_str() );
	double zmax = jagatof( s[5].c_str() );

	bbx = ( xmin + xmax ) /2.0;
	bby = ( ymin + ymax ) /2.0;
	bbz = ( zmin + zmax ) /2.0;
	brx = bbx - xmin;
	bry = bby - ymin;
	brz = bbz - zmin;
}

// M is length of sp
void JagGeo::prepareKMP( const JagStrSplit &sp, int start, int M, int *lps )
{
    int len = 0;
    lps[0] = 0; 
    int i = 1;
    while (i < M) {
        if (sp[i+start] == sp[len+start]) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) {
                len = lps[len-1];
            } else {
                lps[i] = 0;
                i++;
            }
        }
    }
}


// if sp2 contains sp1; or if sp1 is within sp2
// return -1: if not found; index to sp2 contaning sp1
int JagGeo::KMPPointsWithin( const JagStrSplit &sp1, int start1, const JagStrSplit &sp2, int start2 )
{
	/// sp2 is txt , sp1 is pat
	if ( sp2.length()-start2==0 || sp1.length()-start1==0 ) return -1;
    int N = sp2.length()-start2;
    int M = sp1.length()-start1;
	if ( M > N ) return -1;
    int lps[M];
    JagGeo::prepareKMP(sp1, start1, M, lps);
    int i = 0, j=0;
    while (i < N) {
        if (sp1[j+start1] == sp2[i+start2]) {
            j++; i++;
        }
 
        if (j == M) {
			return i-j;
        }
 
        if (i < N  &&  sp1[j+start1] != sp2[i+start2] ) {
            if (j != 0) j = lps[j-1];
            else i = i+1;
        }
    }
	return -1;
}

////////////////////////////////////////// 2D orientation //////////////////////
short JagGeo::orientation(double x1, double y1, double x2, double y2, double x3, double y3 )
{
	double v = (y2 - y1) * (x3 - x2) - (x2 - x1) * (y3 - y2);
	if ( v > 0.0 ) return JAG_CLOCKWISE;
	if ( jagEQ( v, 0.0) ) return JAG_COLINEAR;
	return JAG_COUNTERCLOCKWISE;
}

// is (x y) is "above" line x2 y2 --- x3 y3
bool JagGeo::above(double x, double y, double x2, double y2, double x3, double y3 )
{
	//d("s7638 JagGeo::above (x=%.3f y=%.3f) x2=%.3f y2=%.3f x3=%3f y3=%.3f\n", x,y,x2,y2,x3,y3 );
	bool b1 = isNull(x,y);
	bool b2 = isNull(x2,y2,x3,y3);
	if ( b1 && b2 ) return false;
	if ( b1 ) return false;
	if ( b2 ) return true;

	double v = (y2 - y) * (x3 - x2) - (x2 - x) * (y3 - y2);
	if ( v < 0.0 ) return true;
	return false;
}
// is (x y) is "above" or same (colinear) line x2 y2 --- x3 y3
bool JagGeo::aboveOrSame(double x, double y, double x2, double y2, double x3, double y3 )
{
	//d("s7634 JagGeo::aboveorsame (x=%.3f y=%.3f) x2=%.3f y2=%.3f x3=%3f y3=%.3f\n", x,y,x2,y2,x3,y3 );
	bool b1 = isNull(x,y);
	bool b2 = isNull(x2,y2,x3,y3);
	if ( b1 && b2 ) return true;
	if ( b1 ) return false;
	if ( b2 ) return true;

	double v = (y2 - y) * (x3 - x2) - (x2 - x) * (y3 - y2);
	if ( v > 0.0 ) return false;
	return true;
}

bool JagGeo::same(double x, double y, double x2, double y2, double x3, double y3 )
{
	//d("s7434 JagGeo::same (x=%.3f y=%.3f) x2=%.3f y2=%.3f x3=%3f y3=%.3f\n", x,y,x2,y2,x3,y3 );
	bool b1 = isNull(x,y);
	bool b2 = isNull(x2,y2,x3,y3);
	if ( b1 && b2 ) return true;

	double v = (y2 - y) * (x3 - x2) - (x2 - x) * (y3 - y2);
	if ( jagEQ( v, 0.0) ) return true;
	return false;
}

bool JagGeo::below(double x, double y, double x2, double y2, double x3, double y3 )
{
	//d("s5434 JagGeo::below (x=%.3f y=%.3f) x2=%.3f y2=%.3f x3=%3f y3=%.3f\n", x,y,x2,y2,x3,y3 );
	bool b1 = isNull(x,y);
	bool b2 = isNull(x2,y2,x3,y3);
	if ( b1 && b2 ) return false;
	if ( b1 ) return true;
	if ( b2 ) return false;
	double v = (y2 - y) * (x3 - x2) - (x2 - x) * (y3 - y2);
	if ( v > 0.0 ) return true;
	return false;
}

bool JagGeo::belowOrSame(double x, double y, double x2, double y2, double x3, double y3 )
{
	//d("s5430 JagGeo::beloworsame (x=%.3f y=%.3f) x2=%.3f y2=%.3f x3=%3f y3=%.3f\n", x,y,x2,y2,x3,y3 );
	bool b1 = isNull(x,y);
	bool b2 = isNull(x2,y2,x3,y3);
	if ( b1 && b2 ) return true;
	if ( b1 ) return true;
	if ( b2 ) return false;

	double v = (y2 - y) * (x3 - x2) - (x2 - x) * (y3 - y2);
	if ( v < 0.0 ) return false;
	return true;
}



////////////////////////////////////////// 3D orientation //////////////////////
short JagGeo::orientation(double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3, double z3 )
{
	double v = (z2 - z1) * (x3 - x1) - (x2 - x1) * (z3 - z1);
	if ( v > 0.0 ) return JAG_CLOCKWISE; 
	if ( jagEQ( v, 0.0) ) return JAG_COLINEAR;
	return JAG_COUNTERCLOCKWISE;
}

// is (x y) is "above" line x2 y2 --- x3 y3
bool JagGeo::above(double x, double y, double z, double x2, double y2, double z2, double x3, double y3, double z3 )
{
	d("s2424 JagGeo::above (x=%.1f y=%.1f z=%.1f) x2=%.1f y2=%.1f z2=%.1f x3=%1f y3=%.1f z3=%.1f\n", x,y,z,x2,y2,z2,x3,y3,z3 );
	bool b1 = isNull(x,y,z);
	bool b2 = isNull(x2,y2,z2,x3,y3,z3);
	if ( b1 && b2 ) return false;
	if ( b1 ) return false;
	if ( b2 ) return true;

	double v = (z2 - z) * (x3 - x) - (x2 - x) * (z3 - z);
	if ( v < 0.0 ) return true;
	return false;
}
// is (x y) is "above" or same (colinear) line x2 y2 --- x3 y3
bool JagGeo::aboveOrSame(double x, double y, double z, double x2, double y2, double z2, double x3, double y3, double z3 )
{
	d("s2428 JagGeo::aboveorsame (x=%.1f y=%.1f z=%.1f) x2=%.1f y2=%.1f z2=%.1f x3=%1f y3=%.1f z3=%.1f\n", x,y,z,x2,y2,z2,x3,y3,z3 );
	bool b1 = isNull(x,y,z);
	bool b2 = isNull(x2,y2,z2,x3,y3,z3);
	if ( b1 && b2 ) return true;
	if ( b1 ) return false;
	if ( b2 ) return true;

	double v = (z2 - z) * (x3 - x) - (x2 - x) * (z3 - z);
	if ( v > 0.0 ) return false;
	return true;
}

bool JagGeo::same(double x, double y, double z, double x2, double y2, double z2, double x3, double y3, double z3 )
{
	d("s6428 JagGeo::same (x=%.1f y=%.1f z=%.1f) x2=%.1f y2=%.1f z2=%.1f x3=%1f y3=%.1f z3=%.1f\n", x,y,z,x2,y2,z2,x3,y3,z3 );
	bool b1 = isNull(x,y,z);
	bool b2 = isNull(x2,y2,z2,x3,y3,z3);
	if ( b1 && b2 ) return true;

	double v = (z2 - z) * (x3 - x) - (x2 - x) * (z3 - z);
	if ( jagEQ( v, 0.0) ) return true;
	return false;
}

bool JagGeo::below(double x, double y, double z, double x2, double y2, double z2, double x3, double y3, double z3 )
{
	d("s6438 JagGeo::below (x=%.1f y=%.1f z=%.1f) x2=%.1f y2=%.1f z2=%.1f x3=%1f y3=%.1f z3=%.1f\n", x,y,z,x2,y2,z2,x3,y3,z3 );
	bool b1 = isNull(x,y,z);
	bool b2 = isNull(x2,y2,z2,x3,y3,z3);
	if ( b1 && b2 ) return false;
	if ( b1 ) return true;
	if ( b2 ) return false;
	double v = (z2 - z) * (x3 - x) - (x2 - x) * (z3 - z);
	if ( v > 0.0 ) return true;
	return false;
}

bool JagGeo::belowOrSame(double x, double y, double z, double x2, double y2, double z2, double x3, double y3, double z3 )
{
	d("s6430 JagGeo::beloworsame (x=%.1f y=%.1f z=%.1f) x2=%.1f y2=%.1f z2=%.1f x3=%1f y3=%.1f z3=%.1f\n", x,y,z,x2,y2,z2,x3,y3,z3 );
	bool b1 = isNull(x,y,z);
	bool b2 = isNull(x2,y2,z2,x3,y3,z3);
	if ( b1 && b2 ) return true;
	if ( b1 ) return true;
	if ( b2 ) return false;

	double v = (z2 - z) * (x3 - x) - (x2 - x) * (z3 - z);
	if ( v < 0.0 ) return false;
	return true;
}


///////////////////////////////////////////////////////////////////
bool JagGeo::isNull( double x, double y ) 
{
	if ( jagEQ(x, JAG_LONG_MIN) && jagEQ(x, JAG_LONG_MIN)  ) {
		return true;
	}
	return false;
}
bool JagGeo::isNull( double x1, double y1, double x2, double y2 ) 
{
	if ( jagEQ(x1, JAG_LONG_MIN) && jagEQ(x2, JAG_LONG_MIN) 
		 && jagEQ(y1, JAG_LONG_MIN) && jagEQ(y2, JAG_LONG_MIN) ) {
		return true;
	}
	return false;
}
bool JagGeo::isNull( double x, double y, double z ) 
{
	if ( jagEQ(x, JAG_LONG_MIN) && jagEQ(x, JAG_LONG_MIN) && jagEQ(z, JAG_LONG_MIN)  ) {
		return true;
	}
	return false;
}
bool JagGeo::isNull( double x1, double y1, double z1, double x2, double y2, double z2 ) 
{
	if ( jagEQ(x1, JAG_LONG_MIN) && jagEQ(x2, JAG_LONG_MIN) 
		 && jagEQ(y1, JAG_LONG_MIN) && jagEQ(y2, JAG_LONG_MIN) 
		 && jagEQ(z1, JAG_LONG_MIN) && jagEQ(z2, JAG_LONG_MIN) ) {
		return true;
	}
	return false;
}

#if 0
// sp: OJAG=0=test.lstr.ls=LS guarantee 3 '=' signs
// str: "x:y x:y x:y ..." or "x:y:z x:y:z x:y:z ..."
Jstr JagGeo::makeGeoJson( const JagStrSplit &sp, const char *str )
{
	//d("s3391 makeGeoJson sp[3]=[%s]\n", sp[3].c_str() );
	//d("s3392 sp.print: \n" );
	//sp.print();

	if ( sp[3] == JAG_C_COL_TYPE_LINESTRING ) {
		return makeJsonLineString("LineString", sp, str );
	} else if ( sp[3] == JAG_C_COL_TYPE_LINESTRING3D ) {
		return makeJsonLineString3D( "LineString", sp, str );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTIPOINT ) {
		return makeJsonLineString("MultiPoint", sp, str );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return makeJsonLineString3D("MultiPoint", sp, str );
	} else if ( sp[3] == JAG_C_COL_TYPE_POLYGON ) {
		return makeJsonPolygon( "Polygon", sp, str, false );
	} else if ( sp[3] == JAG_C_COL_TYPE_POLYGON3D ) {
		return makeJsonPolygon( "Polygon", sp, str, true );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return makeJsonPolygon("MultiLineString", sp, str, false );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return makeJsonPolygon( "MultiLineString", sp, str, true );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return makeJsonMultiPolygon( "MultiPolygon", sp, str, false );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return makeJsonMultiPolygon( "MultiPolygon", sp, str, true );
	} else {
		return makeJsonDefault( sp, str) ;
	}
}

/******************************************************************
** GeoJSON supports the following geometry types: 
** Point, LineString, Polygon, MultiPoint, MultiLineString, and MultiPolygon. 
** Geometric objects with additional properties are Feature objects. 
** Sets of features are contained by FeatureCollection objects.
** https://tools.ietf.org/html/rfc7946
*******************************************************************/

// sp: OJAG=0=test.lstr.ls=LS guarantee 3 '=' signs
// str: "xmin:ymin:xmax:ymax x:y x:y x:y ..." 
/*********************
    {
       "type": "Feature",
       "bbox": [-10.0, -10.0, 10.0, 10.0],
       "geometry": {
           "type": "LineString",
           "coordinates": [
                   [-10.0, -10.0],
                   [10.0, -10.0],
                   [10.0, 10.0],
                   [-10.0, -10.0]
           ]
       }
       //...
    }

    {
       "type": "Feature",
       "bbox": [-10.0, -10.0, 10.0, 10.0],
       "geometry": {
           "type": "Polygon",
           "coordinates": [
               [
                   [-10.0, -10.0],
                   [10.0, -10.0],
                   [10.0, 10.0],
                   [-10.0, -10.0]
               ]
           ]
       }
       //...
    }
****************/
Jstr JagGeo::makeJsonLineString( const Jstr &title, const JagStrSplit &sp, const char *str )
{
	//d("s2980 makeJsonLineString str=[%s]\n", str );
	const char *p = str;
	//while ( *p != ' ' && *p != '\0' ) ++p;
	if ( *p == '\0' ) return "";

	Jstr s;
	rapidjson::StringBuffer bs;
	rapidjson::Writer<rapidjson::StringBuffer> writer(bs);
	writer.StartObject();
	writer.Key("type");
	writer.String("Feature");

	JagStrSplit bsp( Jstr(str, p-str), ':' );
	if ( bsp.length() == 4 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.EndArray();
	} else if ( bsp.length() == 6 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.Double( jagatof(bsp[4].c_str()) );
		writer.Double( jagatof(bsp[5].c_str()) );
		writer.EndArray();
	}

	while ( isspace(*p) ) ++p; //  "x:y x:y x:y ..."
	char *q = (char*)p;
	d("s1038 p=[%s]\n", p );

	writer.Key("geometry");
	writer.StartObject();
	    writer.Key("type");
	    writer.String( title.c_str() );
		writer.Key("coordinates");
		writer.StartArray(); 
		while( *q != '\0' ) {
			//d("s2029 q=[%s]\n", q );
			writer.StartArray(); 
			while (*q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				writer.EndArray(); 
				break;
			}
			s = Jstr(p, q-p);
			d("s3941927 s=[%s]\n", s.s() );
			//writer.String( p );
			//writer.String( s.c_str(), s.size() );
			writer.Double( jagatof(s.c_str()) );
			//*q = ':';

			++q;
			//d("s2039 q=[%s]\n", q );
			p = q;
			//while (*q != ' ' && *q != '\0' ) ++q;
			while ( *q != ' ' && *q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				//writer.String( p );
				writer.Double( jagatof(p) );
				writer.EndArray(); 
				break;
			}
			// *q == ':' or ' '

			s = Jstr(p, q-p);
			d("s3942 s=[%s]\n", s.s() );
			//d("s2339 q=[%s]\n", q );
			//writer.String( p );
			//writer.String( s.c_str(), s.size() );
			writer.Double( jagatof(s.c_str()) );
			writer.EndArray(); 
			//*q = ' ';

			while (*q != ' ' && *q != '\0' ) ++q;
			while (*q == ' ' ) ++q;
			p = q;
			//d("s1336 q=[%s]\n", q );
		}
		writer.EndArray(); 
	writer.EndObject();

	writer.Key("properties");
	writer.StartObject();
	    writer.Key("column");
	    writer.String( sp[2].c_str() );
	    writer.Key("srid");
	    writer.String( sp[1].c_str() );
	    writer.Key("dimension");
	    writer.String( "2" );
	writer.EndObject();

	writer.EndObject();

	//d("s0301 got result=[%s]\n", (char*)bs.GetString() );

	return (char*)bs.GetString();
}

Jstr JagGeo::makeJsonLineString3D( const Jstr &title, const JagStrSplit &sp, const char *str )
{
	//d("s0823 makeJsonLineString3D str=[%s]\n", str );

	const char *p = str;
	//while ( *p != ' ' && *p != '\0' ) ++p;
	if ( *p == '\0' ) return "";

	Jstr s;
	rapidjson::StringBuffer bs;
	rapidjson::Writer<rapidjson::StringBuffer> writer(bs);
	writer.StartObject();
	writer.Key("type");
	writer.String("Feature");

	JagStrSplit bsp( Jstr(str, p-str), ':' );
	if ( bsp.length() >= 6 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.Double( jagatof(bsp[4].c_str()) );
		writer.Double( jagatof(bsp[5].c_str()) );
		writer.EndArray();
	}

	while ( isspace(*p) ) ++p;  // "x:y:z x:y:z x:y:z ..."
	char *q = (char*)p;

	writer.Key("geometry");
	writer.StartObject();
	    writer.Key("type");
	    // writer.String("LineString");
	    writer.String( title.c_str() );
		writer.Key("coordinates");
		writer.StartArray(); 
		while( *q != '\0' ) {
			//d("s8102 q=[%s]\n", q );
			writer.StartArray(); 

			while (*q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				writer.EndArray(); 
				break;
			}
			//*q = '\0';
			s = Jstr(p, q-p);
			//writer.String( p );
			//writer.String( s.c_str(), s.size() );
			writer.Double( jagatof(s.c_str()) );
			//*q = ':';

			++q;
			//d("s8103 q=[%s]\n", q );
			p = q;
			while (*q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				writer.EndArray(); 
				break;
			}
			//*q = '\0';
			s = Jstr(p, q-p);
			//writer.String( p );
			//writer.String( s.c_str(), s.size() );
			writer.Double( jagatof(s.c_str()) );
			//*q = ':';

			++q;
			//d("s8104 q=[%s]\n", q );
			p = q;
			//while (*q != ' ' && *q != '\0' ) ++q;
			while ( *q != ' ' && *q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				//writer.String( p );
				writer.Double( jagatof(p) );
				writer.EndArray(); 
				break;
			}

			//*q = '\0';
			s = Jstr(p, q-p);
			// writer.String( p );
			//writer.String( s.c_str(), s.size() );
			writer.Double( jagatof(s.c_str()) );
			writer.EndArray(); 
			//*q = ' ';

			while (*q != ' ' && *q != '\0' ) ++q;
			while (*q == ' ' ) ++q;
			//d("s8105 q=[%s]\n", q );
			p = q;
		}
		writer.EndArray(); 
	writer.EndObject();

	writer.Key("properties");
	writer.StartObject();
	    writer.Key("column");
	    writer.String( sp[2].c_str() );
	    writer.Key("srid");
	    writer.String( sp[1].c_str() );
	    writer.Key("dimension");
	    writer.String( "3" );
	writer.EndObject();

	writer.EndObject();

	return (char*)bs.GetString();
}

/********************************************************
{
   "type": "Polygon",
   "coordinates": [
       [ [100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0] ],
       [ [100.2, 0.2], [100.8, 0.2], [100.8, 0.8], [100.2, 0.8], [100.2, 0.2] ]
   ]
}
********************************************************/
Jstr JagGeo::makeJsonPolygon( const Jstr &title,  const JagStrSplit &sp, const char *str, bool is3D )
{
	//d("s7081 makeJsonPolygon str=[%s] is3D=%d\n", str, is3D );

	const char *p = str;
	//while ( *p != ' ' && *p != '\0' ) ++p;
	if ( *p == '\0' ) return "";

	rapidjson::StringBuffer bs;
	rapidjson::Writer<rapidjson::StringBuffer> writer(bs);
	writer.StartObject();
	writer.Key("type");
	writer.String("Feature");

	Jstr bbox(str, p-str);
	//d("s5640 bbox=[%s]\n", bbox.c_str() );
	JagStrSplit bsp( bbox, ':' );
	//d("s5732 bsp.len=%d\n", bsp.length() );
	if ( bsp.length() == 4 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.EndArray();
	} else if ( bsp.length() == 6 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.Double( jagatof(bsp[4].c_str()) );
		writer.Double( jagatof(bsp[5].c_str()) );
		writer.EndArray();
	}

	while ( isspace(*p) ) ++p; //  "x:y x:y x:y ..."
	char *q = (char*)p;
	Jstr s;
	//int level = 0;

	writer.Key("geometry");
	writer.StartObject();
	    writer.Key("type");
	    //writer.String("Polygon");
	    writer.String( title.c_str() );
		writer.Key("coordinates");
		writer.StartArray(); 
		//++level;
		bool startRing = true;
		while( *q != '\0' ) {
			while (*q == ' ' ) ++q; 
			p = q;
			//d("s2029 q=[%s] level=%d p=[%s]\n", q, level, p );
			if ( startRing ) {
				writer.StartArray(); 
				//++level;
				startRing = false;
			}

			while (*q != ':' && *q != '\0' && *q != '|' ) ++q;
			//d("s2132 q=[%s] level=%d\n", q, level );
			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				//--level;
				//d("s3362 level=%d break\n", level );
				break;
			}
			//*q = '\0';
			//s = Jstr(p, q-p);

			if ( *q == '|' ) {
				writer.EndArray(); // outeraray
				startRing = true;
				//--level;
				//d("s3462 level=%d continue\n", level );
				++q;
				p = q;
				continue;
			}

			s = Jstr(p, q-p);

			writer.StartArray(); 
			//++level;
			//d("s6301 write xcoord s=[%s]\n", s.c_str() );
			//writer.String( p );   // x-coord
			//writer.String( s.c_str(), s.size() );   // x-coord
			writer.Double( jagatof(s.c_str()) );
			//*q = save;

			++q;
			p = q;
			//d("s2039 q=[%s] p=[%s]\n", q, p );
			if ( is3D ) {
				while ( *q != ':' && *q != '\0' && *q != '|' ) ++q;
			} else {
				//while ( *q != ' ' && *q != '\0' && *q != '|' ) ++q;
				while ( *q != ' ' && *q != ':' && *q != '\0' && *q != '|' ) ++q;
			}

			s = Jstr(p, q-p);
			//d("s6302 write ycoord s=[%s]\n", s.c_str() );
			//writer.String( s.c_str(), s.size() );   // y-coord
			writer.Double( jagatof(s.c_str()) );

			if ( is3D && *q != '\0' ) {
				++q;
				p = q;
				//d("s2039 q=[%s] p=[%s]\n", q, p );
				//while ( *q != ' ' && *q != '\0' && *q != '|' ) ++q;
				while ( *q != ' ' && *q != ':' && *q != '\0' && *q != '|' ) ++q;
				//d("s6303 write zcoord s=[%s]\n", s.c_str() );
				s = Jstr(p, q-p);
				//writer.String( s.c_str(), s.size() );   // z-coord
				writer.Double( jagatof(s.c_str()) );
			}

			writer.EndArray(); // inner raray
			//--level;

			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				//--level;
				//d("s3162 level=%d break\n", level );
				break;
			}

			if ( *q == '|' ) {
				writer.EndArray(); // outer raray
				startRing = true;
				//--level;
				//d("s1162 level=%d continue p=[%s]\n", level, p );
				++q;
				p = q;
				continue;
			}

			while (*q != ' ' && *q != '\0' ) ++q;  // goto next x:y coord
			while (*q == ' ' ) ++q;  // goto next x:y coord
			//d("s2339 q=[%s]\n", q );
			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				//--level;
				//d("s5862 level=%d break \n", level );
				break;
			}

			p = q;
		}

		writer.EndArray(); 
		//--level;
		//d("s5869 level=%d outside loop \n", level );
	writer.EndObject();

	writer.Key("properties");
	writer.StartObject();
	    writer.Key("column");
	    writer.String( sp[2].c_str() );
	    writer.Key("srid");
	    writer.String( sp[1].c_str() );
	    writer.Key("dimension");
		if ( is3D ) {
	    	writer.String( "3" );
		} else {
	    	writer.String( "2" );
		}
	writer.EndObject();

	writer.EndObject();

	//d("s0303 got result=[%s]\n", (char*)bs.GetString() );

	return (char*)bs.GetString();
}

/***********************************************************************************
{
   "type": "MultiPolygon",
   "coordinates": [
       [
           [ [102.0, 2.0], [103.0, 2.0], [103.0, 3.0], [102.0, 3.0], [102.0, 2.0] ]
       ],
       [
           [ [100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0] ],
           [ [100.2, 0.2], [100.8, 0.2], [100.8, 0.8], [100.2, 0.8], [100.2, 0.2] ]
       ]
   ]
}
***********************************************************************************/
Jstr JagGeo::makeJsonMultiPolygon( const Jstr &title,  const JagStrSplit &sp, const char *str, bool is3D )
{
	//d("s7084 makeJsonMultiPolygon str=[%s] is3D=%d\n", str, is3D );

	const char *p = str;
	//while ( *p != ' ' && *p != '\0' ) ++p;
	if ( *p == '\0' ) return "";

	rapidjson::StringBuffer bs;
	rapidjson::Writer<rapidjson::StringBuffer> writer(bs);
	writer.StartObject();
	writer.Key("type");
	writer.String("Feature");

	Jstr bbox(str, p-str);
	//d("s5640 bbox=[%s]\n", bbox.c_str() );
	JagStrSplit bsp( bbox, ':' );
	//d("s5732 bsp.len=%d\n", bsp.length() );
	if ( bsp.length() == 4 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.EndArray();
	} else if ( bsp.length() == 6 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.Double( jagatof(bsp[4].c_str()) );
		writer.Double( jagatof(bsp[5].c_str()) );
		writer.EndArray();
	}

	while ( isspace(*p) ) ++p; //  "x:y x:y x:y ..."
	char *q = (char*)p;
	Jstr s;
	//int level = 0;

	writer.Key("geometry");
	writer.StartObject();
	    writer.Key("type");
	    writer.String( title.c_str() );
		writer.Key("coordinates");
		writer.StartArray(); 
		//++level;
		bool startPolygon = true;
		bool startRing = true;
		while( *q != '\0' ) {
			while (*q == ' ' ) ++q; 
			p = q;
			//d("s2029 q=[%s] level=%d p=[%s]\n", q, level, p );

			if ( startPolygon ) {
				writer.StartArray(); 
				//++level;
				startPolygon = false;
			}

			if ( startRing ) {
				writer.StartArray(); 
				//++level;
				startRing = false;
			}

			while (*q != ':' && *q != '\0' && *q != '|' && *q != '!' ) ++q;
			//d("s2132 q=[%s] level=%d\n", q, level );
			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				writer.EndArray(); // outeraray
				//--level;
				//--level;
				//d("s3362 level=%d break\n", level );
				break;
			}

			if ( *q == '|' ) {
				writer.EndArray(); // outeraray
				startRing = true;
				//--level;
				//d("s3462 level=%d continue\n", level );
				++q;
				p = q;
				continue;
			}

			if ( *q == '!' ) {
				writer.EndArray(); // outeraray
				writer.EndArray(); // outeraray
				startPolygon = true;
				startRing = true;
				//--level;
				//--level;
				//d("s3462 level=%d continue\n", level );
				++q;
				p = q;
				continue;
			}

			s = Jstr(p, q-p);

			writer.StartArray(); 
			//++level;
			//d("s6301 write xcoord p=[%s]\n", p );
			//writer.String( s.c_str(), s.size() );   // x-coord
			writer.Double( jagatof(s.c_str()) );

			++q;
			p = q;
			//d("s2039 q=[%s] p=[%s]\n", q, p );
			if ( is3D ) {
				while ( *q != ':' && *q != '\0' && *q != '|' && *q != '!' ) ++q;
			} else {
				while ( *q != ' ' && *q != '\0' && *q != '|' && *q != '!' ) ++q;
			}

			s = Jstr(p, q-p);
			//writer.String( s.c_str(), s.size() );   // y-coord
			writer.Double( jagatof(s.c_str()) );

			if ( is3D && *q != '\0' ) {
				++q;
				p = q;
				//d("s2039 q=[%s] p=[%s]\n", q, p );
				while ( *q != ' ' && *q != '\0' && *q != '|' ) ++q;
				s = Jstr(p, q-p);
				//writer.String( s.c_str(), s.size() );   // z-coord
				writer.Double( jagatof(s.c_str()) );
			}

			writer.EndArray(); // inner raray
			//--level;

			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				writer.EndArray(); // outeraray
				//--level;
				//--level;
				//d("s3162 level=%d break\n", level );
				break;
			}

			if ( *q == '|' ) {
				writer.EndArray(); // outer raray
				startRing = true;
				//--level;
				//d("s1162 level=%d continue p=[%s]\n", level, p );
				++q;
				p = q;
				continue;
			}

			if ( *q == '!' ) {
				writer.EndArray(); // outer raray
				writer.EndArray(); // outer raray
				startPolygon = true;
				startRing = true;
				//--level;
				//--level;
				//d("s1162 level=%d continue p=[%s]\n", level, p );
				++q;
				p = q;
				continue;
			}

			while (*q == ' ' ) ++q;  // goto next x:y coord
			//d("s2339 q=[%s]\n", q );
			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				writer.EndArray(); // outeraray
				//--level;
				//--level;
				//d("s5862 level=%d break \n", level );
				break;
			}

			p = q;
		}

		writer.EndArray(); 
		//--level;
		//d("s5869 level=%d outside loop \n", level );
	writer.EndObject();

	writer.Key("properties");
	writer.StartObject();
	    writer.Key("column");
	    writer.String( sp[2].c_str() );
	    writer.Key("srid");
	    writer.String( sp[1].c_str() );
	    writer.Key("dimension");
		if ( is3D ) {
	    	writer.String( "3" );
		} else {
	    	writer.String( "2" );
		}
	writer.EndObject();

	writer.EndObject();

	//d("s0303 got result=[%s]\n", (char*)bs.GetString() );

	return (char*)bs.GetString();
}

Jstr JagGeo::makeJsonDefault( const JagStrSplit &sp, const char *str )
{
	return "";
}
#endif

// 2D case
bool JagGeo::pointWithinPolygon( double x, double y, const JagLineString3D &linestr )
{
	if ( linestr.size() < 4 ) {
		//d("s2760 pointWithinPolygon linestr.size=%d\n", linestr.size() );
		return false;
	}
	JagPoint2D point(x,y);
	JagPoint2D p1( linestr.point[0].x, linestr.point[0].y );

	// linestr.point[i].x.y.z
	int cnt = 0;
	for ( int i = 1; i <= linestr.size()-3; ++i ) {
		JagPoint2D p2( linestr.point[i].x, linestr.point[i].y );
		JagPoint2D p3( linestr.point[i+1].x, linestr.point[i+1].y );
		if ( pointWithinTriangle( point, p1, p2, p3, false, false ) ) {
			++cnt;
		}
	}

	if ( ( cnt%2 ) == 1 ) {
		// odd number
		//d("3039 cnt=%d odd true\n" );
		return true;
	} else {
		//d("3034 cnt=%d even false\n" );
		return false;
	}

}

void JagGeo::getPolygonBound( const Jstr &mk, const JagStrSplit &sp, 
							  double &bbx, double &bby, double &rx, double &ry )
{
    if ( mk == JAG_OJAG ) {
        boundingBoxRegion(sp[0], bbx, bby, rx, ry );
    } else {
		double xmin, ymin, xmax, ymax, dx, dy;
		xmin = ymin = JAG_LONG_MAX; 
        xmax = ymax = JAG_LONG_MIN;
		const char *str; char *p; int nc;
		for ( int i = JAG_SP_START; i < sp.length(); ++i ) {
			str = sp[i].c_str();
			nc = strchrnum( str, ':');
			if ( nc < 1 ) continue; // skip bbox
			get2double(str, p, ':', dx, dy );
			if ( dx < xmin ) xmin = dx;
			if ( dx > xmax ) xmax = dx;
			if ( dy < ymin ) ymin = dy;
			if ( dy > ymax ) ymax = dy;
		}
        bbx = ( xmin+xmax)/2.0; bby = ( ymin+ymax)/2.0;
        rx = (xmax-xmin)/2.0; ry = (ymax-ymin)/2.0;
    }
}

void JagGeo::getLineStringBound( const Jstr &mk, const JagStrSplit &sp, 
							  double &bbx, double &bby, double &rx, double &ry )
{
    if ( mk == JAG_OJAG ) {
        boundingBoxRegion(sp[0], bbx, bby, rx, ry );
    } else {
		double xmin, ymin, xmax, ymax, dx, dy;
		xmin = ymin = JAG_LONG_MAX; 
        xmax = ymax = JAG_LONG_MIN;
		const char *str; char *p; int nc;
		for ( int i = JAG_SP_START; i < sp.length(); ++i ) {
			str = sp[i].c_str();
			nc = strchrnum( str, ':');
			if ( nc < 1 ) continue; // skip bbox
			get2double(str, p, ':', dx, dy );
			if ( dx < xmin ) xmin = dx;
			if ( dx > xmax ) xmax = dx;
			if ( dy < ymin ) ymin = dy;
			if ( dy > ymax ) ymax = dy;
		}
        bbx = ( xmin+xmax)/2.0; bby = ( ymin+ymax)/2.0;
        rx = (xmax-xmin)/2.0; ry = (ymax-ymin)/2.0;
    }
}

bool JagGeo::pointWithinPolygon( double x, double y, const JagPolygon &pgon )
{
	if ( pgon.size() <1 ) {
		d("s7080 pointWithinPolygon false\n" );
		return false;
	}

	if ( ! pointWithinPolygon( x, y, pgon.linestr[0] ) ) {
		d("s7082 pointWithinPolygon false\n" );
		return false;
	}

	for ( int i=1; i < pgon.size(); ++i ) {
		if ( pointWithinPolygon( x, y, pgon.linestr[i] ) ) {
			d("s7084 i=%d pointWithinPolygon false\n" );
			return false;  // in holes
		}
	}

	return true;
}


// return 0: not insersect
// return 1: intersection falls inside the triagle
// return 2: intersection does not fall in the triangle, but falls on the plane. atPoint returns coord
int JagGeo::line3DIntersectTriangle3D( const JagLine3D& line3d, 
								       const JagPoint3D &p0, const JagPoint3D &p1, const JagPoint3D &p2,
	                                   JagPoint3D &atPoint )
{
	// https://en.wikipedia.org/wiki/Line–plane_intersection
	JagPoint3D negab;
	negab.x = line3d.x1 - line3d.x2; 
	negab.y = line3d.y1 - line3d.y2; 
	negab.z = line3d.z1 - line3d.z2; 

	JagPoint3D p01;
	p01.x = p1.x - p0.x; p01.y = p1.y - p0.y; p01.z = p1.z - p0.z;

	JagPoint3D p02;
	p02.x = p2.x - p0.x; p02.y = p2.y - p0.y; p02.z = p2.z - p0.z;
	
	JagPoint3D LA0;
	LA0.x = line3d.x1 - p0.x; LA0.y = line3d.y1 - p0.y; LA0.z = line3d.z1 - p0.z;


	double crossx, crossy, crossz;
	crossProduct( p01.x, p01.y, p01.z, p02.x, p02.y, p02.z, crossx, crossy, crossz );
	double dotLow = ::dotProduct( negab.x, negab.y, negab.z, crossx, crossy, crossz );
	// t = dotUp / dotLow;
	if ( jagEQ( dotLow, 0.0 ) ) {
		// either in plane or parallel
		if ( point3DWithinTriangle3D( line3d.x1, line3d.y1, line3d.z1, p0, p1, p2 ) ) return 1;
		if ( point3DWithinTriangle3D( line3d.x2, line3d.y2, line3d.z2, p0, p1, p2 ) ) return 1;
		return 0;
	}

	double dotUp = ::dotProduct( crossx, crossy, crossz, LA0.x, LA0.y, LA0.z );
	double t = dotUp/dotLow;
	if ( t < 0.0 || t > 1.0 ) return 0; // intersection is out of line segment  line3d

	crossProduct( p02.x, p02.y, p02.z, negab.x, negab.y, negab.z, crossx, crossy, crossz );
	dotUp = ::dotProduct( crossx, crossy, crossz, LA0.x, LA0.y, LA0.z );
	double u = dotUp/dotLow;

	crossProduct( negab.x, negab.y, negab.z, p01.x, p01.y, p01.z, crossx, crossy, crossz );
	dotUp = ::dotProduct( crossx, crossy, crossz, LA0.x, LA0.y, LA0.z );
	double v = dotUp/dotLow;
	if ( jagLE( u+v, 1.0) ) return 1;

	// la + lab * t
	atPoint.x =  line3d.x1 - negab.x * t;
	atPoint.y =  line3d.y1 - negab.y * t;
	atPoint.z =  line3d.z1 - negab.z * t;
	return 2;
}

// inside the plane ?
bool JagGeo::point3DWithinTriangle3D( double x, double y, double z, const JagPoint3D &p1, const JagPoint3D &p2, const JagPoint3D &p3 )
{
	JagPoint3D pt(x,y,z);
	double u, v, w;
	int rc = getBarycentric(pt, p1, p2, p3, u, v, w );
	if ( rc < 0 ) return false;

	if ( u < 0.0 || u > 1.0 ) return false;
	if ( v < 0.0 || v > 1.0 ) return false;
	if ( w < 0.0 || w > 1.0 ) return false;
	return true;
}

double JagGeo::distancePoint3DToTriangle3D( double x, double y, double z,
											double x1, double y1, double z1, double x2, double y2, double z2,
											double x3, double y3, double z3 )
{
	double A, B, C, D;
	triangle3DABCD( x1, y1, z1, x2, y2, z2, x3, y3, z3, A, B, C, D );
	return distancePoint3DToPlane(x,y,z, A, B, C, D );
}

/***
void JagGeo::Barycentric(Point p, Point a, Point b, Point c, float &u, float &v, float &w)
{
    Vector v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = Dot(v0, v0);
    float d01 = Dot(v0, v1);
    float d11 = Dot(v1, v1);
    float d20 = Dot(v2, v0);
    float d21 = Dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    v = (d11 * d20 - d01 * d21) / denom;
    w = (d00 * d21 - d01 * d20) / denom;
    u = 1.0 - v - w;
}
***/
// -1 error  0 ok
int JagGeo::getBarycentric( const JagPoint3D &p, const JagPoint3D &a, const JagPoint3D &b, const JagPoint3D &c, 
						  double &u, double &v, double &w)
{
    //Vector v0 = b - a, v1 = c - a, v2 = p - a;
	JagPoint3D v0, v1, v2;
	minusVector(b, a, v0);
	minusVector(c, a, v1);
	minusVector(p, a, v2);

    double d00 = dotProduct(v0, v0);
    double d01 = dotProduct(v0, v1);
    double d11 = dotProduct(v1, v1);
    double d20 = dotProduct(v2, v0);
    double d21 = dotProduct(v2, v1);
    double denom = d00 * d11 - d01 * d01;
	if ( jagEQ(denom, 0.0) ) return -1;

    v = (d11 * d20 - d01 * d21) / denom;
    w = (d00 * d21 - d01 * d20) / denom;
    u = 1.0 - v - w;
	return 0;
}

void JagGeo::minusVector( const JagPoint3D &v1, const JagPoint3D &v2, JagPoint3D &pt )
{
	pt.x = v1.x - v2.x; pt.y = v1.y - v2.y; pt.z = v1.z - v2.z;
}

double JagGeo::dotProduct( const JagPoint3D &p1, const JagPoint3D &p2 )
{
	return ( p1.x*p2.x + p1.y*p2.y + p1.z*p2.z );
}

double JagGeo::dotProduct( const JagPoint2D &p1, const JagPoint2D &p2 )
{
	return ( p1.x*p2.x + p1.y*p2.y );
}


bool JagGeo::distance( const JagFixString &inlstr, const JagFixString &inrstr, const Jstr &arg, double &dist )
{
	d("s3083 JagGeo::distance inlstr=[%s] arg=[%s]\n", inlstr.c_str(), arg.c_str() );
	d("s3083 JagGeo::distance inrstr=[%s] arg=[%s]\n", inrstr.c_str(), arg.c_str() );

	Jstr lstr;
	int rc = 0;
	if ( !strnchr( inlstr.c_str(), '=', 8 ) ) {
		d("s5510 JagParser::convertConstantObjToJAG ...\n" );
		rc = JagParser::convertConstantObjToJAG( inlstr, lstr );
		if ( rc <= 0 ) return false;
		// point(0 0 0 )  to "OJAG=srid=33=33=d x y z"
		d("s5512 JagParser::convertConstantObjToJAG lstr=[%s] rc=%d ...\n", lstr.c_str(), rc );
	} else {
		lstr = inlstr.c_str();
	}

	Jstr rstr;
	if ( !strnchr( inrstr.c_str(), '=', 8 ) ) {
		rc = JagParser::convertConstantObjToJAG( inrstr, rstr );
		if ( rc <= 0 ) return false;
	} else {
		rstr = inrstr.c_str();
	}

	JagStrSplit sp1( lstr.c_str(), ' ', true );
	if ( sp1.length() < 1 ) return 0;
	JagStrSplit sp2( rstr.c_str(), ' ', true );
	if ( sp2.length() < 1 ) return 0;

	JagStrSplit co1( sp1[0], '=' );
	if ( co1.length() < 4 ) return 0;

	JagStrSplit co2( sp2[0], '=' );
	if ( co2.length() < 4 ) return 0;

	//d("s44942 co1.print  co2.print\n");

	Jstr mark1 = co1[0]; // CJAG or OJAG
	Jstr mark2 = co2[0];

	Jstr colType1 = co1[3];
	Jstr colType2 = co2[3];
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	d("s7231 dim1=%d dim2=%d\n", dim1, dim2 );
	if ( dim1 != dim2 ) { return 0; }

	int srid1 = jagatoi( co1[1].c_str() );
	int srid2 = jagatoi( co2[1].c_str() );
	d("s444401 srid1=%d srid2=%d\n", srid1, srid2 );
	int srid = srid1;
	if ( 0 == srid1 ) {
		srid = srid2;
	} else if ( 0 == srid2 ) {
		srid = srid1;
	} else if ( srid2 != srid1 ) {
		return 0;
	}

	//sp1.shift();
	//sp2.shift();
	d("s4872 colType1=[%s]\n", colType1.c_str() );
	d("s4872 colType2=[%s]\n", colType2.c_str() );
	d("s4872 srid=[%d]\n", srid );

	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		return doPointDistance( mark1, sp1, mark2, colType2, sp2, srid, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		return doPointDistance( mark2, sp2, mark1, colType1, sp1, srid, arg, dist );
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return doPoint3DDistance( mark1, sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		return doPoint3DDistance( mark2, sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		return doCircleDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		return doCircleDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		return doCircle3DDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE3D ) {
		return doCircle3DDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		return doSphereDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		return doSphereDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		return doSquareDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		return doSquareDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		return doSquare3DDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE3D ) {
		return doSquare3DDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE ) {
		return doCubeDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_CUBE ) {
		return doCubeDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		return doRectangleDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		return doRectangleDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		return doRectangle3DDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		return doRectangle3DDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_BOX ) {
		return doBoxDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		return doBoxDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE ) {
		return doTriangleDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		return doTriangleDistance( mark2, sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		return doTriangle3DDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		return doTriangle3DDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_CYLINDER ) {
		return doCylinderDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_CYLINDER ) {
		return doCylinderDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_CONE ) {
		return doConeDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		return doConeDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		return doEllipseDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		return doEllipseDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSOID ) {
		return doEllipsoidDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		return doEllipsoidDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		return doLineDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
		return doLineDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return doLine3DDistance( mark1,  sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_LINE3D ) {
		return doLine3DDistance( mark2,  sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		return doLineStringDistance( mark1,   sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return doLineStringDistance( mark2,   sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return doLineString3DDistance( mark1,   sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return doLineString3DDistance( mark2,   sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		return doPolygonDistance( mark1,   sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return doPolygonDistance( mark2,   sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		return doPolygon3DDistance( mark1,   sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON3D ) {
		return doPolygon3DDistance( mark2,   sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		return doLineStringDistance( mark1,   sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOINT ) {
		return doLineStringDistance( mark2,   sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return doLineString3DDistance( mark1,   sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return doLineString3DDistance( mark2,   sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return doPolygonDistance( mark1,   sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return doPolygonDistance( mark2,   sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return doPolygon3DDistance( mark1,   sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return doPolygon3DDistance( mark2,   sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return doMultiPolygonDistance( mark1,   sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return doMultiPolygonDistance( mark2,   sp2, mark1, colType1, sp1, srid,  arg, dist);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return doMultiPolygon3DDistance( mark1,   sp1, mark2, colType2, sp2, srid,  arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return doMultiPolygon3DDistance( mark2,   sp2, mark1, colType1, sp1, srid,  arg, dist);
	} 

	return true;
}

		
/////////////////////////// distance ///////////////////////////////////////////////////////		
bool JagGeo::doPointDistance(const Jstr& mk1, const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	d("s33303 JagGeo::doPointDistance \n");
	d("s33349 arg=[%s] srid=%d\n", arg.s(), srid );

	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		return pointDistancePoint( srid, px0, py0, x0, y0, arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		return pointDistanceLine( srid, px0, py0, x1, y1, x2, y2, arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_MULTIPOINT ) {
		return pointDistanceLineString( srid, px0, py0, mk2, sp2, arg, dist);
	} else if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		return pointDistanceTriangle( srid, jagatof( sp1[JAG_SP_START+0].c_str()), jagatof( sp1[JAG_SP_START+1].c_str()), 
									  jagatof( sp2[JAG_SP_START+0].c_str() ), jagatof( sp2[JAG_SP_START+1].c_str() ), 
									  jagatof( sp2[JAG_SP_START+2].c_str() ), jagatof( sp2[JAG_SP_START+3].c_str() ), 
									  jagatof( sp2[JAG_SP_START+4].c_str() ), jagatof( sp2[JAG_SP_START+5].c_str() ),
									  arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return pointDistanceSquare( srid, px0, py0, x0, y0, r, nx, arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return pointDistanceRectangle( srid, px0, py0, x0, y0, w,h, nx, arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() );
		return pointDistanceCircle( srid, px0, py0, x, y, r, arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+2].c_str() );
		double h = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		//double ny = safeget(sp2, JAG_SP_START+5);
		return pointDistanceEllipse( srid, px0, py0, x0, y0, w, h, nx, arg, dist);
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return pointDistancePolygon( srid, px0, py0, mk2, sp2, arg, dist);
	}
	return false;
}

bool JagGeo::doPoint3DDistance( const Jstr& mk1, const JagStrSplit& sp1, const Jstr& mk2, 
							    const Jstr& colType2,
							    const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	d("s7730 mk1=[%s] mk2=[%s] sp1:sp2:\n", mk1.c_str(), mk2.c_str() );
	//sp1.print();
	//sp2.print();

	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		return point3DDistancePoint3D( srid, px0, py0, pz0, x1, y1, z1, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_LINE3D ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		return point3DDistanceLine3D( srid, px0, py0, pz0, x1, y1, z1, x2, y2, z2, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return point3DDistanceLineString3D( srid, px0, py0, pz0, mk2, sp2, arg, dist );
	} else if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return point3DDistanceBox( srid, px0, py0, pz0, x0, y0, z0, r,r,r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double c = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return point3DDistanceBox( srid, px0, py0, pz0, x0, y0, z0, a,b,c, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return point3DDistanceSphere( srid, px0,py0,pz0, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() );
		double b = jagatof( sp2[JAG_SP_START+4].c_str() );
		double c = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return point3DDistanceEllipsoid( srid, px0, py0, pz0, x0, y0, z0, a,b,c, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return point3DDistanceCone( srid, px0,py0,pz0, x, y, z, r, h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CYLINDER ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return point3DDistanceCylinder( srid, px0,py0,pz0, x, y, z, r, h, nx, ny, arg, dist );
	}
	return false;
}

bool JagGeo::doCircleDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 

	if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() );
		return circleDistanceCircle( srid, px0,py0,pr0, x,y,r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() );
		double nx = safeget(sp2, JAG_SP_START+3);
		return circleDistanceSquare( srid, px0,py0,pr0, x,y,r, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+2].c_str() );
		double h = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return circleDistanceRectangle( srid, px0, py0, pr0, x0, y0, w, h, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		return circleDistanceTriangle( srid, px0, py0, pr0, x1, y1, x2, y2, x3, y3, arg, dist  );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return circleDistanceEllipse( srid, px0, py0, pr0, x0, y0, a, b, nx, arg, dist  );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return circleDistancePolygon( srid, px0, py0, pr0, mk2, sp2, arg, dist );
	}
	return false;
}

bool JagGeo::doCircle3DDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, 
								const Jstr& colType2, const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 

	double nx0 = 0.0;
	double ny0 = 0.0;
	if ( sp1.length() >= 5 ) { nx0 = jagatof( sp1[JAG_SP_START+4].c_str() ); }
	if ( sp1.length() >= 6 ) { ny0 = jagatof( sp1[JAG_SP_START+5].c_str() ); }

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return circle3DDistanceCube( srid, px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, r, nx, ny, arg, dist );
	} else if (  colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double c = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return circle3DDistanceBox( srid, px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, a,b,c, nx, ny, arg, dist );
	} else if (  colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return circle3DDistanceSphere( srid, px0, py0, pz0, pr0, nx0, ny0, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return circle3DDistanceEllipsoid( srid, px0, py0, pz0, pr0, nx0, ny0, x0, y0, z0, w,d,h,  nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return circle3DDistanceCone( srid, px0,py0,pz0,pr0,nx0,ny0, x, y, z, r, h, nx, ny, arg, dist );
	}

	return false;
}

double JagGeo::doSphereArea( int srid1, const JagStrSplit& sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return r * r * 4.0 * JAG_PI;
}

double JagGeo::doSphereVolume( int srid1, const JagStrSplit& sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return r * r * r * 4.0 * JAG_PI/3.0;
}

bool JagGeo::doSphereDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return sphereDistanceCube( srid, px0, py0, pz0, pr0, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 

		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return sphereDistanceBox( srid, px0, py0, pz0, pr0, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return sphereDistanceSphere( srid, px0, py0, pz0, pr0, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return sphereDistanceEllipsoid( srid, px0, py0, pz0, pr0, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return sphereDistanceCone( srid, px0, py0, pz0, pr0,    x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}

bool JagGeo::doSquareDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, 
							const Jstr& colType2, const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+3);

	// like point Distance
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		// return squareDistanceTriangle( srid, px0, py0, pr0, nx0, x1, y1, x2, y2, x3, y3, arg, dist );
		return rectangleDistanceTriangle( srid, px0, py0, pr0,pr0, nx0, x1, y1, x2, y2, x3, y3, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleDistanceSquare( srid, px0, py0, pr0,pr0, nx0, x0, y0, a, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleDistanceRectangle( srid, px0, py0, pr0,pr0, nx0, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleDistanceCircle( srid, px0, py0, pr0,pr0, nx0, x0, y0, r, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleDistanceEllipse( srid, px0, py0, pr0,pr0, nx0, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return rectangleDistancePolygon( srid, px0, py0, pr0,pr0,nx0, mk2, sp2, arg, dist );
	}
	return false;
}

bool JagGeo::doSquare3DDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, 
							const Jstr& colType2, const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);
	double ny0 = safeget(sp1, JAG_SP_START+5);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return rectangle3DDistanceCube( srid, px0, py0, pz0, pr0,pr0, nx0, ny0, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return rectangle3DDistanceBox( srid, px0, py0, pz0, pr0,pr0, nx0, ny0,x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return rectangle3DDistanceSphere( srid, px0, py0, pz0, pr0,pr0, nx0, ny0, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return rectangle3DDistanceEllipsoid( srid, px0, py0, pz0, pr0,pr0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return rectangle3DDistanceCone( srid, px0, py0, pz0, pr0,pr0, nx0, ny0, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;

}

double JagGeo::doCubeArea( int srid1, const JagStrSplit& sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return (r*r*24.0);  // 2r*2r*6
}

double JagGeo::doCubePerimeter( int srid1, const JagStrSplit& sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return (r*24.0);  // 2r*12
}

double JagGeo::doCubeVolume( int srid1, const JagStrSplit& sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return (r*r*r*8.0);  // 2r*2r*2r
}

bool JagGeo::doCubeDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);
	double ny0 = safeget(sp1, JAG_SP_START+5);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return boxDistanceCube( srid, px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxDistanceBox( srid, px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return boxDistanceSphere( srid, px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxDistanceEllipsoid( srid, px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return boxDistanceCone( srid, px0, py0, pz0, pr0,pr0,pr0, nx0, ny0, x0, y0, z0, r, h, nx,ny, arg, dist );
	}
	return false;
}

double JagGeo::doRectangleArea( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return a*b* 4.0;
}

double JagGeo::doRectanglePerimeter( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return (a+b)* 4.0;
}
double JagGeo::doRectangle3DArea( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	return a*b* 4.0;
}
double JagGeo::doRectangle3DPerimeter( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	return (a+b)* 4.0;
}

bool JagGeo::doRectangleDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, 
						const Jstr& colType2, const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);

	// like point Distance
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return rectangleDistanceTriangle( srid, px0, py0, a0, b0, nx0, x1, y1, x2, y2, x3, y3, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleDistanceSquare( srid, px0, py0, a0, b0, nx0, x0, y0, a, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleDistanceRectangle( srid, px0, py0, a0, b0, nx0, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return rectangleDistanceCircle( srid, px0, py0, a0, b0, nx0, x0, y0, r, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return rectangleDistanceEllipse( srid, px0, py0, a0, b0, nx0, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return rectangleDistancePolygon( srid, px0,py0,a0,b0,nx0, mk2, sp2, arg, dist );
	}
	return false;
}

bool JagGeo::doRectangle3DDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, 
						const Jstr& colType2, const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return rectangle3DDistanceCube( srid, px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return rectangle3DDistanceBox( srid, px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return rectangle3DDistanceSphere( srid, px0, py0, pz0, a0, b0, nx0, ny0, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return rectangle3DDistanceEllipsoid( srid, px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return rectangle3DDistanceCone( srid, px0, py0, pz0, a0, b0, nx0, ny0, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}

double JagGeo::doBoxArea( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	return  (a*b + b*c + c*a ) * 8.0;
	// ( 2a*2b + 2b*2c + 2a*2c )*2 
}
double JagGeo::doBoxPerimeter( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	return  (a + b + c ) * 8.0;
}

double JagGeo::doBoxVolume( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	return  a*b*c* 8.0;
	// ( 2a*2b*2c )
}

bool JagGeo::doBoxDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+6);
	double ny0 = safeget(sp1, JAG_SP_START+7);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return boxDistanceCube( srid, px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxDistanceBox( srid, px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return boxDistanceSphere( srid, px0, py0, pz0, a0, b0, c0, nx0, ny0, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return boxDistanceEllipsoid( srid, px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return boxDistanceCone( srid, px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}

double JagGeo::doTriangleArea( int srid1, const JagStrSplit& sp1 )
{
	double Ax = jagatof( sp1[JAG_SP_START+0].c_str() );
	double Ay = jagatof( sp1[JAG_SP_START+1].c_str() );
	double Bx = jagatof( sp1[JAG_SP_START+2].c_str() );
	double By = jagatof( sp1[JAG_SP_START+3].c_str() );
	double Cx = jagatof( sp1[JAG_SP_START+4].c_str() );
	double Cy = jagatof( sp1[JAG_SP_START+5].c_str() );
	if (  JAG_GEO_WGS84 == srid1 ) {
		/***
		double ax, ay bx, by cx cy;
		JagGeo::lonLatToXY( srid, Ax, Ay, ax, ay );
		JagGeo::lonLatToXY( srid, Bx, By, bx, by );
		JagGeo::lonLatToXY( srid, Cx, Cy, cx, cy );
		return fabs( ax*(by-cy) + bx*(cy-ay) + cx*(ay-by))/2.0;
		***/
		const Geodesic& geod = Geodesic::WGS84();
		PolygonArea poly(geod);
		poly.AddPoint( Ax, Ay );
		poly.AddPoint( Bx, By );
		poly.AddPoint( Cx, Cy );
		double perim, area;
		poly.Compute( false, false, perim, area );
		return area;
	} else {
		return fabs( Ax*(By-Cy) + Bx*(Cy-Ay) + Cx*(Ay-By))/2.0;
	}
}

double JagGeo::doTrianglePerimeter( int srid1, const JagStrSplit& sp1 )
{
	double Ax = jagatof( sp1[JAG_SP_START+0].c_str() );
	double Ay = jagatof( sp1[JAG_SP_START+1].c_str() );
	double Bx = jagatof( sp1[JAG_SP_START+2].c_str() );
	double By = jagatof( sp1[JAG_SP_START+3].c_str() );
	double Cx = jagatof( sp1[JAG_SP_START+4].c_str() );
	double Cy = jagatof( sp1[JAG_SP_START+5].c_str() );
	return distance(Ax,Ay, Bx,By, srid1) + distance(Ax,Ay, Cx,Cy, srid1) + distance(Cx,Cy, Bx,By, srid1);
}
double JagGeo::doTriangle3DPerimeter( int srid1, const JagStrSplit& sp1 )
{
	double Ax = jagatof( sp1[JAG_SP_START+0].c_str() );
	double Ay = jagatof( sp1[JAG_SP_START+1].c_str() );
	double Az = jagatof( sp1[JAG_SP_START+2].c_str() );
	double Bx = jagatof( sp1[JAG_SP_START+3].c_str() );
	double By = jagatof( sp1[JAG_SP_START+4].c_str() );
	double Bz = jagatof( sp1[JAG_SP_START+5].c_str() );
	double Cx = jagatof( sp1[JAG_SP_START+6].c_str() );
	double Cy = jagatof( sp1[JAG_SP_START+7].c_str() );
	double Cz = jagatof( sp1[JAG_SP_START+8].c_str() );
	return distance(Ax,Ay,Az, Bx,By,Bz, srid1) + distance(Ax,Ay,Az, Cx,Cy,Cz, srid1) + distance(Cx,Cy,Cz, Bx,By,Bz, srid1);
}

bool JagGeo::doTriangleDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double x30 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double y30 = jagatof( sp1[JAG_SP_START+5].c_str() );

	// like point Distance
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return triangleDistanceTriangle( srid, x10, y10, x20, y20, x30, y30, x1, y1, x2, y2, x3, y3, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return triangleDistanceSquare( srid, x10, y10, x20, y20, x30, y30, x0, y0, a, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return triangleDistanceRectangle( srid, x10, y10, x20, y20, x30, y30, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return triangleDistanceCircle( srid, x10, y10, x20, y20, x30, y30, x0, y0, r, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return triangleDistanceEllipse( srid, x10, y10, x20, y20, x30, y30, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return triangleDistancePolygon( srid, x10, y10, x20, y20, x30, y30, mk2, sp2, arg, dist );
	}
	return false;
}

double JagGeo::doTriangle3DArea( int srid1, const JagStrSplit& sp1 )
{
	double x1 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y1 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double z1 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double x2 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double y2 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double z2 = jagatof( sp1[JAG_SP_START+5].c_str() );
	double x3 = jagatof( sp1[JAG_SP_START+6].c_str() );
	double y3 = jagatof( sp1[JAG_SP_START+7].c_str() );
	double z3 = jagatof( sp1[JAG_SP_START+8].c_str() );
	if (  JAG_GEO_WGS84 == srid1 ) {
		double rx1, ry1, rz1, rx2, ry2, rz2, rx3, ry3, rz3;
		JagGeo::lonLatAltToXYZ( srid1, x1, y1, z1, rx1, ry1, rz1 );
		JagGeo::lonLatAltToXYZ( srid1, x2, y2, z2, rx2, ry2, rz2 );
		JagGeo::lonLatAltToXYZ( srid1, x3, y3, z3, rx3, ry3, rz3 );
		double dx1 = rx2-rx1; double dx2 = rx3-rx1;
		double dy1 = ry2-ry1; double dy2 = ry3-ry1;
		double dz1 = rz2-rz1; double dz2 = rz3-rz1;
		return sqrt( jagsq2(dy1*dz2-dy2*dz1) + jagsq2(dx1*dz2-dx2*dz1) + jagsq2(dx1*dy2-dx2*dy1) )/2.0;
	} else {
		double dx1 = x2-x1; double dx2 = x3-x1;
		double dy1 = y2-y1; double dy2 = y3-y1;
		double dz1 = z2-z1; double dz2 = z3-z1;
		return sqrt( jagsq2(dy1*dz2-dy2*dz1) + jagsq2(dx1*dz2-dx2*dz1) + jagsq2(dx1*dy2-dx2*dy1) )/2.0;
	}
}

bool JagGeo::doTriangle3DDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, 
						const Jstr& colType2, const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double z10 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double z20 = jagatof( sp1[JAG_SP_START+5].c_str() );
	double x30 = jagatof( sp1[JAG_SP_START+6].c_str() );
	double y30 = jagatof( sp1[JAG_SP_START+7].c_str() );
	double z30 = jagatof( sp1[JAG_SP_START+8].c_str() );

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return triangle3DDistanceCube( srid, x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return triangle3DDistanceBox( srid, x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return triangle3DDistanceSphere( srid, x10,y10,z10,x20,y20,z20,x30,y30,z30, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return triangle3DDistanceEllipsoid( srid, x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return triangle3DDistanceCone( srid, x10,y10,z10,x20,y20,z20,x30,y30,z30, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}

double JagGeo::doCylinderArea( int srid1, const JagStrSplit& sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	return 2.0*JAG_PI*r*(c*2.0 + r);  // 2πrH+2πr^2 = 2*PI*r ( H + r) 
}

double JagGeo::doCylinderVolume( int srid1, const JagStrSplit& sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	return r*r*JAG_PI *c*2.0;
}

bool JagGeo::doCylinderDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 

	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return cylinderDistanceCube( srid, px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return cylinderDistanceBox( srid, px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return cylinderDistanceSphere( srid, px0, py0, pz0, pr0, c0,  nx0, ny0, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return cylinderDistanceEllipsoid( srid, px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return cylinderDistanceCone( srid, px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}

double JagGeo::doConeArea( int srid1, const JagStrSplit& sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double R = r * 2.0;
	double h = c * 2.0;
	return JAG_PI * R * ( R + sqrt( h*h+ R*R) );
}

double JagGeo::doConeVolume( int srid1, const JagStrSplit& sp1 )
{
	double r = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double R = r * 2.0;
	double h = c * 2.0;
	return JAG_PI*R*R*h/3.0; 
}

bool JagGeo::doConeDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double pr0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+5);
	double ny0 = safeget(sp1, JAG_SP_START+6);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return coneDistanceCube( srid, px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return coneDistanceBox( srid, px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return coneDistanceSphere( srid, px0, py0, pz0, pr0, c0,  nx0, ny0, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return coneDistanceEllipsoid( srid, px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return coneDistanceCone( srid, px0, py0, pz0, pr0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}

double JagGeo::doEllipseArea( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return a*b*JAG_PI;
}

double JagGeo::doEllipsePerimeter( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	return JAG_PI*( 3.0*(a+b) - sqrt( (3.0*a+b)*(a+3.0*b) ) );
	// Ramanujan approximation
}

double JagGeo::doEllipse3DArea( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	return a*b*JAG_PI;
}

double JagGeo::doEllipse3DPerimeter( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	return JAG_PI*( 3.0*(a+b) - sqrt( (3.0*a+b)*(a+3.0*b) ) );
}

bool JagGeo::doEllipseDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+4);

	// like point Distance
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return ellipseDistanceTriangle( srid, px0, py0, a0, b0, nx0, x1, y1, x2, y2, x3, y3, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return ellipseDistanceSquare( srid, px0, py0, a0, b0, nx0, x0, y0, a, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return ellipseDistanceRectangle( srid, px0, py0, a0, b0, nx0, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return ellipseDistanceCircle( srid, px0, py0, a0, b0, nx0, x0, y0, r, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return ellipseDistanceEllipse( srid, px0, py0, a0, b0, nx0, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return ellipseDistancePolygon( srid, px0, py0, a0, b0, nx0, mk2, sp2, arg, dist );
	}
	return false;
}

double JagGeo::doEllipsoidArea( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	double p=1.6075; // Knud Thomsen approximation formula
	double ap = pow(a, p);
	double bp = pow(b, p);
	double cp = pow(c, p);
	double f = ( ap*(bp+cp)+bp*cp)/3.0;
	f = pow(f, 1.0/p);
	return 4.0*JAG_PI*f ;
}

double JagGeo::doEllipsoidVolume( int srid1, const JagStrSplit& sp1 )
{
	double a = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	return 4.0*JAG_PI*a*b*c/3.0;
}

bool JagGeo::doEllipsoidDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
	double a0 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	double b0 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
	double c0 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	double nx0 = safeget(sp1, JAG_SP_START+6);
	double ny0 = safeget(sp1, JAG_SP_START+7);

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return ellipsoidDistanceCube( srid, px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return ellipsoidDistanceBox( srid, px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return ellipsoidDistanceSphere( srid, px0, py0, pz0, a0, b0, c0, nx0, ny0, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return ellipsoidDistanceEllipsoid( srid, px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return ellipsoidDistanceCone( srid, px0, py0, pz0, a0, b0, c0, nx0, ny0, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}

bool JagGeo::doLineDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+3].c_str() );

	// like point Distance
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return lineDistanceTriangle( srid, x10, y10, x20, y20, x1, y1, x2, y2, x3, y3, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return lineDistanceSquare( srid, x10, y10, x20, y20, x0, y0, a, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return lineDistanceLineString( srid, x10, y10, x20, y20, mk2, sp2, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineDistanceRectangle( srid, x10, y10, x20, y20, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return lineDistanceCircle( srid, x10, y10, x20, y20, x0, y0, r, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineDistanceEllipse( srid, x10, y10, x20, y20, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return lineDistancePolygon( srid, x10, y10, x20, y20, mk2, sp2, arg, dist );
	}
	return false;
}

bool JagGeo::doLine3DDistance(const Jstr& mk1,  const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	double x10 = jagatof( sp1[JAG_SP_START+0].c_str() );
	double y10 = jagatof( sp1[JAG_SP_START+1].c_str() );
	double z10 = jagatof( sp1[JAG_SP_START+2].c_str() );
	double x20 = jagatof( sp1[JAG_SP_START+3].c_str() );
	double y20 = jagatof( sp1[JAG_SP_START+4].c_str() );
	double z20 = jagatof( sp1[JAG_SP_START+5].c_str() );

	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return line3DDistanceCube( srid, x10,y10,z10,x20,y20,z20, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return line3DDistanceLineString3D( srid, x10,y10,z10,x20,y20,z20, mk2, sp2, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return line3DDistanceBox( srid, x10,y10,z10,x20,y20,z20, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return line3DDistanceSphere( srid, x10,y10,z10,x20,y20,z20, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return line3DDistanceEllipsoid( srid, x10,y10,z10,x20,y20,z20, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return line3DDistanceCone( srid, x10,y10,z10,x20,y20,z20, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}

bool JagGeo::doLineStringDistance(const Jstr& mk1, const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{

	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		// JAG_C_COL_TYPE_TRIANGLE is 2D already
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return lineStringDistanceTriangle( srid, mk1, sp1, x1, y1, x2, y2, x3, y3, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return lineStringDistanceLineString( srid, mk1, sp1, mk2, sp2, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		//d("s0881 lineStringDistanceSquare ...\n" );
		return lineStringDistanceSquare( srid, mk1, sp1, x0, y0, a, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineStringDistanceRectangle( srid, mk1, sp1, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return lineStringDistanceCircle( srid, mk1, sp1, x0, y0, r, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return lineStringDistanceEllipse( srid, mk1, sp1, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return lineStringDistancePolygon( srid, mk1, sp1, mk2, sp2, arg, dist );
	}
	return false;
}

bool JagGeo::doLineString3DDistance(const Jstr& mk1, const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		return doPoint3DDistance( mk2, sp2, mk1, JAG_C_COL_TYPE_LINESTRING3D, sp1, srid,  arg, dist);
	} else if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return lineString3DDistanceCube( srid, mk1, sp1, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return lineString3DDistanceLineString3D( srid, mk1, sp1, mk2, sp2, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return lineString3DDistanceBox( srid, mk1, sp1, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return lineString3DDistanceSphere( srid, mk1, sp1, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return lineString3DDistanceEllipsoid( srid, mk1, sp1, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return lineString3DDistanceCone( srid, mk1, sp1, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}

bool JagGeo::doPolygonDistance(const Jstr& mk1, const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		return polygonDistanceTriangle( srid, mk1, sp1, x1, y1, x2, y2, x3, y3, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return polygonDistanceSquare( srid, mk1, sp1, x0, y0, a, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return polygonDistanceRectangle( srid, mk1, sp1, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return polygonDistanceCircle( srid, mk1, sp1, x0, y0, r, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return polygonDistanceEllipse( srid, mk1, sp1, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return polygonDistancePolygon( srid, mk1, sp1, mk2, sp2, arg, dist );
	}
	return false;
}

bool JagGeo::doPolygon3DDistance(const Jstr& mk1, const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return polygon3DDistanceCube( srid, mk1, sp1, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return polygon3DDistanceBox( srid, mk1, sp1, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return polygon3DDistanceSphere( srid, mk1, sp1, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return polygon3DDistanceEllipsoid( srid, mk1, sp1, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return polygon3DDistanceCone( srid, mk1, sp1, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}

bool JagGeo::doMultiPolygonDistance(const Jstr& mk1, const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	if (  colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		double x1 = jagatof( sp2[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp2[JAG_SP_START+1].c_str() );
		double x2 = jagatof( sp2[JAG_SP_START+2].c_str() );
		double y2 = jagatof( sp2[JAG_SP_START+3].c_str() );
		double x3 = jagatof( sp2[JAG_SP_START+4].c_str() );
		double y3 = jagatof( sp2[JAG_SP_START+5].c_str() );
		d("x1:%f\n y1:%f\n x2:%f\n y2:%f\n x3:%f\n y3:%f\n", x1, y1, x2, y2, x3, y3);
		return multiPolygonDistanceTriangle( srid, mk1, sp1, x1, y1, x2, y2, x3, y3, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		d("s6040 JAG_C_COL_TYPE_SQUARE sp2 print():\n");
		//sp2.print();
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return multiPolygonDistanceSquare( srid, mk1, sp1, x0, y0, a, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double b = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		return multiPolygonDistanceRectangle( srid, mk1, sp1, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+3);
		return multiPolygonDistanceCircle( srid, mk1, sp1, x0, y0, r, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double a = jagatof( sp2[JAG_SP_START+2].c_str() );
		double b = jagatof( sp2[JAG_SP_START+3].c_str() );
		double nx = safeget(sp2, JAG_SP_START+4);
		return multiPolygonDistanceEllipse( srid, mk1, sp1, x0, y0, a, b, nx, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return multiPolygonDistancePolygon( srid, mk1, sp1, mk2, sp2, arg, dist );
	}
	return false;
}

bool JagGeo::doMultiPolygon3DDistance(const Jstr& mk1, const JagStrSplit& sp1, const Jstr& mk2, const Jstr& colType2,
										 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist)
{
	if (  colType2 == JAG_C_COL_TYPE_CUBE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+4);
		double ny = safeget(sp2, JAG_SP_START+5);
		return multiPolygon3DDistanceCube( srid, mk1, sp1, x0, y0, z0, r, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() ); 
		double d = jagatof( sp2[JAG_SP_START+4].c_str() ); 
		double h = jagatof( sp2[JAG_SP_START+5].c_str() ); 
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return multiPolygon3DDistanceBox( srid, mk1, sp1, x0, y0, z0, w,d,h, nx, ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		return multiPolygon3DDistanceSphere( srid, mk1, sp1, x, y, z, r, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double w = jagatof( sp2[JAG_SP_START+3].c_str() );
		double d = jagatof( sp2[JAG_SP_START+4].c_str() );
		double h = jagatof( sp2[JAG_SP_START+5].c_str() );
		double nx = safeget(sp2, JAG_SP_START+6);
		double ny = safeget(sp2, JAG_SP_START+7);
		return multiPolygon3DDistanceEllipsoid( srid, mk1, sp1, x0, y0, z0, w,d,h, nx,ny, arg, dist );
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
		double r = jagatof( sp2[JAG_SP_START+3].c_str() );
		double h = jagatof( sp2[JAG_SP_START+4].c_str() );
		double nx = safeget(sp2, JAG_SP_START+5);
		double ny = safeget(sp2, JAG_SP_START+6);
		return multiPolygon3DDistanceCone( srid, mk1, sp1, x0, y0, z0, r,h, nx,ny, arg, dist );
	}
	return false;
}



// 2D point
bool JagGeo::pointDistancePoint( int srid, double px, double py, double x1, double y1, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px, py, x1, y1, srid );
    return true;
}

bool JagGeo::pointDistanceLine( int srid, double px, double py, double x1, double y1, double x2, double y2, 
								const Jstr& arg, double &dist )
{
	bool rc = true;
	double projx, projy;
	if ( arg.caseEqual( "min" ) ) {
		dist = minPoint2DToLineSegDistance( px, py, x1, y1, x2, y2, srid, projx, projy );
	} else if ( arg.caseEqual( "max" ) ) {
		double d1 = JagGeo::distance( px, py, x1, y1, srid );
		double d2 = JagGeo::distance( px, py, x2, y2, srid );
		dist = jagmax( d1, d2 );
	} else if ( arg.caseEqual( "center" ) ) {
		distance(px,py, (x1+x2)/2.0, (y1+y2)/2.0, srid );
	} else if ( arg.caseEqual( "perpendicular" ) ) {
		if ( jagEQ(y1,y2) && jagEQ(x1,x2) ) return false;
		if ( 0 == srid ) {
			dist = fabs( (y2-y1)*px - (x2-x1)*py + x2*y1 - y2*x1 )/ sqrt( (y2-y1)*(y2-y1) + (x2-x1)*(x2-x1) );
		} else if ( JAG_GEO_WGS84 == srid ) {
			dist = pointToLineGeoDistance( y1, x1, y2, x2, py, px );
		} else {
			dist = 0.0;
			return false;
		}
	} 

    return rc;
}

// arg: "min" "max"
// dist in number or meters
bool JagGeo::pointDistanceLineString( int srid,  double x, double y, const Jstr &mk2, const JagStrSplit &sp2, 
									  const Jstr& arg, double &dist )
{
	int start = JAG_SP_START;
	if ( arg.caseEqual( "center" ) ) {
		double avgx, avgy;
		bool rc = lineStringAverage( mk2, sp2, avgx, avgy );
		if ( ! rc ) { 
			dist = 0.0;
			return false;
		}
		dist = JagGeo::distance( x, y, avgx, avgy, srid );
		return true;
	}

    double dx1, dy1, d, dx2, dy2;
	double mind = JAG_LONG_MAX;
	double maxd = JAG_LONG_MIN;
    const char *str;
    char *p;
	double projx, projy;
	for ( int i=start; i < sp2.length()-1; ++i ) {
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx1, dy1 );

		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx2, dy2 );

		if ( arg.caseEqual( "max" ) ) {
			d = JagGeo::distance( x, y, dx1, dy1, srid );
			if ( d > maxd ) maxd = d;
			d = JagGeo::distance( x, y, dx2, dy2, srid );
			if ( d > maxd ) maxd = d;
		} else {
			d = minPoint2DToLineSegDistance( x, y, dx1, dy1, dx2, dy2, srid, projx, projy );
			if ( d < mind ) mind = d;
		}
	}

	if ( arg.caseEqual( "max" ) ) {
		dist = maxd;
	} else if ( arg.caseEqual( "min" ) ) {
		dist = mind;
	} else {
		return false;
	}

	return true;
}

// min max center
bool JagGeo::pointDistanceSquare( int srid, double x, double y, double px0, double py0, double r, double nx, 
								  const Jstr& arg, double &dist )
{
    return pointDistanceRectangle( srid, x, y, px0, py0, r, r, nx, arg, dist );
}

// min max center
bool JagGeo::pointDistanceCircle(int srid, double px, double py, double x0, double y0, double r, const Jstr& arg, double &dist )
{
	double d;
	d = JagGeo::distance( px, py, x0, y0, srid );
	if ( arg.caseEqual("center") ) {
		dist = d;
		return true;
	}

	if ( arg.caseEqual("max") ) {
		dist = r + d;
	} else {
		if ( r > d ) dist = r-d;
		else dist = d - r;
	}
	return true;
}

// min max center
bool JagGeo::pointDistanceRectangle( int srid, double x, double y, double px0, double py0, double a0, double b0, double nx0, 
									 const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		dist = JagGeo::distance( x, y, px0, py0, srid );
		return true;
	}

    double mind1, maxd1, px, py;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    transform2DCoordGlobal2Local( px0, py0, x, y, nx0, px, py );
    if(fabs(px) <= a0){
        //point to up and down lines
        mind1 = fabs(fabs(py) - b0);
        maxd1 = JagGeo::distance( fabs(px), fabs(py), -a0, -b0, srid);
        if ( mind1 < mind ) { mind = mind1; }
        if ( maxd1 > maxd ) { maxd = maxd1; }
    } else if ( fabs(py) <= b0){
        //point to left and right lines
        mind1 = fabs(fabs(px) - a0);
        maxd1 = JagGeo::distance( fabs(px), fabs(py), -a0, -b0, srid );
        if ( mind1 < mind ) mind = mind1;
        if ( maxd1 > maxd ) maxd = maxd1;
    } else {
        //point to 4 points
        if ( arg.caseEqual( "min" ) ) {
            mind1 = JagGeo::distance( fabs(px), fabs(py), a0, b0, srid );
            if (mind1 < mind) {
                mind = mind1;
            }
        }
        if ( arg.caseEqual( "max" ) ) {
            maxd1 = JagGeo::distance( fabs(px), fabs(py),-a0, -b0, srid );
            if ( maxd1 > maxd ) {
                maxd = maxd1;
            }
        }
    }

    if ( arg.caseEqual("max") ) {
        dist = maxd;
    } else if (arg.caseEqual("min")){
        dist = mind;
    } else {
        dist = JagGeo::distance( fabs(px), fabs(py), px0, py0, srid );
    }
    return true;

}

// x y is center of ellipse. px py is the external point
bool JagGeo::pointDistanceEllipse( int srid, double px, double py, double x0, double y0, double a, double b, double nx, 
								   const Jstr& arg, double &dist )
{

    if ( arg.caseEqual("center") ) {
        dist = JagGeo::distance( x0, y0, px, py, srid );
        return true;
    }

    double locx, locy;
    transform2DCoordGlobal2Local( x0,y0, px, py, nx, locx, locy );
    //double mx, my;
    if ( arg.caseEqual("max") ) {
        dist = pointDistanceToEllipse( srid, px, py, x0, y0, a, b, nx, false );
    } else {
        dist = pointDistanceToEllipse( srid, px, py, x0, y0, a, b, nx, true );
    }

    return true;
}

double JagGeo::pointDistanceToEllipse( int srid, double px, double py, double x0, double y0, double a, double b, double nx, bool isMin )
{
	double locx, locy;
	transform2DCoordGlobal2Local( x0,y0, px, py, nx, locx, locy );
	double mx, my, dist;
	if ( isMin ) {
		minMaxPointOnNormalEllipse( srid, a, b, px, py, true, mx, my, dist );
	} else {
		minMaxPointOnNormalEllipse( srid, a, b, px, py, false, mx, my, dist );
	}
	return dist;
}

double JagGeo::point3DDistanceToEllipsoid( int srid, double px, double py, double pz,
                                           double x0, double y0, double z0, double a, double b, double c, double nx, double ny, bool isMin )
{
	double locx, locy, locz;
	transform3DCoordGlobal2Local( x0,y0,z0, px, py, pz, nx, ny, locx, locy, locz );
	double mx, my, mz, dist;
	if ( isMin ) {
		minMaxPoint3DOnNormalEllipsoid( srid, a, b, c, px, py, pz, true, mx, my, mz, dist );
	} else {
		minMaxPoint3DOnNormalEllipsoid( srid, a, b, c, px, py, pz, false, mx, my, mz, dist );
	}
	return dist;
}


// min max
bool JagGeo::pointDistanceTriangle(int srid, double px, double py, double x1, double y1,
								  double x2, double y2, double x3, double y3,
				 				  const Jstr& arg, double &dist )
{
	double d1, d2, d3;
	d1 = JagGeo::distance( x1, y1, px, py, srid );
	d2 = JagGeo::distance( x2, y2, px, py, srid );
	d3 = JagGeo::distance( x3, y3, px, py, srid );
	if ( arg.caseEqual("max") ) {
		dist = jagmax3(d1, d2, d3);
	} else {
		dist = jagmin3(d1, d2, d3);
	}

    return true;
}

bool JagGeo::pointDistancePolygon( int srid, double x, double y, const Jstr &mk2, const JagStrSplit &sp2, 
								   const Jstr& arg, double &dist )
{
    //const char *str;
    //char *p;
	JagPolygon pgon;
	int rc;
	rc = JagParser::addPolygonData( pgon, sp2, true );
	if ( rc < 0 ) { return false; }

	double mind = JAG_LONG_MAX;
	double maxd = JAG_LONG_MIN;
	double d;
	const JagLineString3D &linestr = pgon.linestr[0];
	for ( int i=0; i < linestr.size(); ++i ) {
		d = JagGeo::distance( x, y, linestr.point[i].x, linestr.point[i].y, srid );
		if ( d > maxd ) maxd = d;
		if ( d < mind ) mind = d;
	}

	if ( arg.caseEqual("max") ) {
		dist = maxd;
	} else {
		dist = mind;
	}

	return true;
}


// 3D point
bool JagGeo::point3DDistancePoint3D( int srid, double px, double py, double pz, double x1, double y1, double z1, 
									 const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px, py, pz, x1, y1, z1, srid );
	d("s2083 px=%f py=%f pz=%f srid=%d\n", px, py, pz, srid );
	d("s3028  x1, y1, z1= %f %f %f\n",  x1, y1, z1 );
    return true;
}
bool JagGeo::point3DDistanceLine3D(int srid, double px, double py, double pz, double x1, double y1, double z1, 
								double x2, double y2, double z2, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px, py, pz, x1, y1, z1, srid );
	double dist2 = JagGeo::distance( px, py, pz, x2, y2, z2, srid );

	if ( arg.caseEqual("max") ) {
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}

    return true;
}
bool JagGeo::point3DDistanceLineString3D(int srid,  double x, double y, double z, const Jstr &mk2, 
										const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
	d("s2038 srid=%d x y z = %f %f %f\n", srid, x, y, z );
	//d("s5780 sp2:\n" );
	//sp2.print(); 
	// OJAG=0=test.linestr3d.l3=LS3=0 1.0:2.0:3.0:5.0:6.0:7.0 1.0:2.0:3.0 2.0:3.0:4.0 5.0:6.0:7.0

	if ( arg.caseEqual( "center" ) ) {
		double avgx, avgy, avgz;
		bool rc = lineString3DAverage( mk2, sp2, avgx, avgy, avgz );
		if ( ! rc ) { 
			dist = 0.0;
			return false;
		}
		dist = JagGeo::distance( x, y, z, avgx, avgy, avgz, srid );
		return true;
	}

    double dx1, dy1, dz1;
    double dx2, dy2, dz2;
    double projx, projy, projz;
    const char *str;
    char *p;
	double max = JAG_LONG_MIN;
	double min = JAG_LONG_MAX;
	bool isMax;

	if ( arg.caseEqual("max") ) { 
        isMax = true;	
    } else { 
        isMax = false;	
    }

	for ( int i = JAG_SP_START; i < sp2.length()-1; ++i ) {
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx1, dy1, dz1 );
		str = sp2[i+1].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', dx2, dy2, dz2 );

		if ( isMax ) {
			dist = JagGeo::distance( dx1, dy1, dz1, x, y, z, srid );
			if ( dist > max ) max = dist;
			dist = JagGeo::distance( dx2, dy2, dz2, x, y, z, srid );
			if ( dist > max ) max = dist;
		} else {
			dist = minPoint3DToLineSegDistance( x, y, z, dx1, dy1, dz1, dx2, dy2, dz2, srid, projx, projy, projz );
			if ( dist < min ) min = dist;
		}
	}

	if ( isMax ) { dist = max; }
	else { dist = min; }

    return true;
}
//dx dy dz are the point3d's coordinate;
//d == a; w == b; h == c;
bool JagGeo::point3DDistanceBox(int srid, double dx, double dy, double dz,
								  double x0, double y0, double z0, double d, double w, double h, double nx, double ny,
								  const Jstr& arg, double &dist )
{
    double px, py, pz, maxd1, mind1;
    //double xsum = 0, ysum = 0, zsum = 0;
    //long counter = 0;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    //const char *str;
    //char *p;

    transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, px, py, pz );
    if (fabs(px) <= d && fabs(py) <= w){
        //point to up and down sides
        mind1 = fabs(fabs(pz) - h);
        maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
        if ( mind1 < mind ) mind = mind1;
        if ( maxd1 > maxd ) maxd = maxd1;
    }else if(fabs(py) <= w && fabs(pz) < h){
        //point to front and back sides
        mind1 = fabs(fabs(px) - d);
        maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
        if ( mind1 < mind ) mind = mind1;
        if ( maxd1 > maxd ) maxd = maxd1;
    }else if(fabs(px) <= d && fabs(pz) < h){
        //point to left and right sides
        mind1 = fabs(fabs(py) - w);
        maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
        if ( mind1 < mind ) mind = mind1;
        if ( maxd1 > maxd ) maxd = maxd1;
    }else if(fabs(px) <= d){
        //point to 4 depth lines
        mind1 = DistanceOfPointToLine(fabs(px), fabs(py), fabs(pz), d, w, h, -d, w, h);
        maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
        if ( mind1 < mind ) mind = mind1;
        if ( maxd1 > maxd ) maxd = maxd1;
    }else if(fabs(py) <= w){
        //point to 4 width lines
        mind1 = DistanceOfPointToLine(fabs(px), fabs(py), fabs(pz), d, w, h, d, -w, h);
        maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
        if ( mind1 < mind ) mind = mind1;
        if ( maxd1 > maxd ) maxd = maxd1;
    }else if(fabs(pz) <= h){
        //point to 4 height lines
        mind1 = DistanceOfPointToLine(fabs(px), fabs(py), fabs(pz), d, w, h, d, w, -h);
        maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
        if ( mind1 < mind ) mind = mind1;
        if ( maxd1 > maxd ) maxd = maxd1;
    }else{
        //point to 8 points
        mind1 = distance( fabs(px), fabs(py), fabs(pz), d, w, h, srid );
        maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
        if ( mind1 < mind ) mind = mind1;
        if ( maxd1 > maxd ) maxd = maxd1;
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else if (arg.caseEqual("min" )){
        dist = mind;
    }else {
        dist = distance( dx, dy, dz, x0, y0, z0, srid );
    }
    return true;
}

bool JagGeo::point3DDistanceSphere(int srid, double px, double py, double pz, double x, double y, double z, double r, 
									const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px, py, pz, x, y, z, srid );
    if ( arg.caseEqual( "max" ) ) { return true; }
    if ( arg.caseEqual( "min" ) ) { dist -= r; dist = fabs(dist); return true; }
    else if ( arg.caseEqual( "max" ) ) { dist += r; return true; }

	return false;
}

bool JagGeo::point3DDistanceEllipsoid(int srid, double px, double py, double pz,  
								  double x0, double y0, double z0, double a, double b, double c, double nx, double ny, 
								  const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		dist = JagGeo::distance( px, py, pz, x0, y0, z0, srid );
		return true;
	}

	if ( arg.caseEqual("min") ) {
		dist = point3DDistanceToEllipsoid( srid, px, py, pz, x0, y0, z0, a, b, c, nx, ny, true );
	} else {
		dist = point3DDistanceToEllipsoid( srid, px, py, pz, x0, y0, z0, a, b, c, nx, ny, false );
	}

	return true;
}

bool JagGeo::point3DDistanceCone(int srid, double px, double py, double pz, 
								double x, double y, double z,
								 double r, double h,  double nx, double ny, const Jstr& arg, double &dist )
{
	// transform px py pz to local normal cone
	double lx, ly, lz; // x y z is coord of cone's center
	transform3DCoordGlobal2Local( x,y,z, px,py,pz, nx, ny, lx, ly, lz );
	d("s5018 point3DDistanceCone x=%f y=%f z=%f  px=%f py=%f pz=%f\n", x,y,z, px,py,pz );
	d("s5018 point3DDistanceCone lx=%f ly=%f lz=%f\n", lx,ly,lz );
	return point3DDistanceNormalCone(srid, lx, ly, lz, r, h, arg, dist ); 
}

bool JagGeo::point3DDistanceSquare3D(int srid, double px, double py, double pz, 
								double x, double y, double z,
								 double a, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px, py, pz, x, y, z, srid );
	if ( arg.caseEqual("center") ) { return true; }
	if ( arg.caseEqual("min") ) { 
		dist -= a;
		dist = fabs( dist );
	} else {
		dist += a;
	}
	return true; 

}

bool JagGeo::point3DDistanceCylinder(int srid,  double px, double py, double pz,
                                    double x, double y, double z,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px, py, pz, x, y, z, srid );
	if ( arg.caseEqual("center") ) { return true; }
	if ( arg.caseEqual("min") ) { 
		dist -= 0.5*(r+h);
		dist = fabs( dist );
	} else {
		dist += 0.5*(r+h);
	}
    return true;
}

// 2D circle
bool JagGeo::circleDistanceCircle(int srid, double px, double py, double pr, double x, double y, double r, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px, py, x, y, srid );
	if ( arg.caseEqual("center") ) { return true; }
	if ( arg.caseEqual("min") ) { 
		dist -= pr+r;
		dist = fabs( dist );
	} else {
		dist += pr+r;
	}
    return true;
}

bool JagGeo::circleDistanceSquare( int srid, double px0, double py0, double pr, double x0, double y0, double r, 
								   double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, x0, y0, srid );
	if ( arg.caseEqual("center") ) { return true; }
	if ( arg.caseEqual("min") ) { 
		dist -= pr+r;
		dist = fabs( dist );
	} else {
		dist += pr+r;
	}
    return true;
}

bool JagGeo::circleDistanceEllipse(int srid, double px, double py, double pr, 
								 	 double x, double y, double w, double h, double nx, 
								 	 const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px, py, x, y, srid );
	if ( arg.caseEqual("center") ) { return true; }
	if ( arg.caseEqual("min") ) { 
		dist -= pr+ 0.5*(w+h);
		dist = fabs( dist );
	} else {
		dist += pr+ 0.5*(w+h);
	}
    return true;
}

bool JagGeo::circleDistanceRectangle(int srid, double px0, double py0, double pr, double x0, double y0,
									  double w, double h, double nx,  const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, x0, y0, srid );
	if ( arg.caseEqual("center") ) { return true; }
	if ( arg.caseEqual("min") ) { 
		dist -= pr+ 0.5*(w+h);
		dist = fabs( dist );
	} else {
		dist += pr+ 0.5*(w+h);
	}
    return true;
}

bool JagGeo::circleDistanceTriangle(int srid, double px, double py, double pr, double x1, double y1, 
								  double x2, double y2, double x3, double y3, const Jstr& arg, double &dist ) 
{
	dist = JagGeo::distance( px, py, x1, y1, srid );
	double dist2 = JagGeo::distance( px, py, x2, y2, srid );
	double dist3 = JagGeo::distance( px, py, x3, y3, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}

bool JagGeo::circleDistancePolygon(int srid, double px0, double py0, double pr, 
								 const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
	d("s2030 sp2:\n" );
	// sp2.print();
	// [OJAG=0=test.pol1.pol=PL=0 0.0:0.0:80.0:80.0 0.0:0.0 80.0:0.0 80.0:80.0 0.0:80.0 0.0:0.0
    double dx, dy;
    const char *str;
    char *p;
	double max = JAG_LONG_MIN;
	double min = JAG_LONG_MAX;
	bool isMax;
	if ( arg.caseEqual("max") ) { isMax = true;	} else { isMax = false;	}
	for ( int i = JAG_SP_START; i < sp2.length(); ++i ) {
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx, dy );
		dist = JagGeo::distance( dx, dy, px0, py0, srid );
		if ( isMax ) {
			if ( dist > max ) max = dist;
		} else {
			if ( dist < min ) min = dist;
		}
	}

	if ( isMax ) { dist = max; }
	else { dist = min; }

    return true;
}

// 3D circle
bool JagGeo::circle3DDistanceCube(int srid, double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
									double x0, double y0, double z0,  double r, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, pz0, x0, y0, z0, srid );
	if ( arg.caseEqual("center") ) { return true; }
	if ( arg.caseEqual("min") ) { 
		dist -= pr0+r;
		dist = fabs( dist );
	} else {
		dist += pr0+r;
	}
    return true;
}

bool JagGeo::circle3DDistanceBox(int srid, double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
				                   double x0, double y0, double z0,  double a, double b, double c,
				                   double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, pz0, x0, y0, z0, srid );
	if ( arg.caseEqual("center") ) { return true; }
	if ( arg.caseEqual("min") ) { 
		dist -= pr0+(a+b+c)/3.0;
		dist = fabs( dist );
	} else {
		dist += pr0+(a+b+c)/3.0;
	}
    return true;
}

bool JagGeo::circle3DDistanceSphere(int srid, double px0, double py0, double pz0, double pr0,   double nx0, double ny0,
   									 double x, double y, double z, double r, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, pz0, x, y, z, srid );
	if ( arg.caseEqual("center") ) { return true; }
	if ( arg.caseEqual("min") ) { 
		dist -= pr0+r;
		dist = fabs( dist );
	} else {
		dist += pr0+r;
	}
    return true;
}

bool JagGeo::circle3DDistanceEllipsoid(int srid, double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
									  double x0, double y0, double z0, 
								 	   double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, pz0, x0, y0, z0, srid );
	if ( arg.caseEqual("center") ) { return true; }
	if ( arg.caseEqual("min") ) { 
		dist -= pr0+(w+d+h)/3.0;
		dist = fabs( dist );
	} else {
		dist += pr0+(w+d+h)/3.0;
	}
    return true;
}
bool JagGeo::circle3DDistanceCone(int srid, double px0, double py0, double pz0, double pr0, double nx0, double ny0,
									  double x0, double y0, double z0, 
								 	   double r, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		dist = JagGeo::distance( px0, py0, pz0, x0, y0, z0, srid );
		return true;
	}
	
	point3DDistanceCone(srid, px0, py0, pz0, x0, y0, z0, r, h, nx, ny,  arg, dist );
	if ( arg.caseEqual("min") ) {
		dist -= pr0 + (r+h)/2.0;
		dist = fabs( dist );
	} else {
		dist += pr0 + (r+h)/2.0;
	}
    return true;
}

// 3D sphere
bool JagGeo::sphereDistanceCube(int srid,  double px0, double py0, double pz0, double pr0,
	                               double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, pz0, x0, y0, z0, srid );
	if ( arg.caseEqual("min") ) {
		dist -= pr0+r;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += pr0+r;
	}
    return true;
}

bool JagGeo::sphereDistanceBox(int srid,  double px0, double py0, double pz0, double r,
	                                double x0, double y0, double z0, double w, double d, double h, 
									double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, pz0, x0, y0, z0, srid );
	if ( arg.caseEqual("min") ) {
		dist -= r + (w+d+h)/3.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += r + (w+d+h)/3.0;
	}
    return true;
}

bool JagGeo::sphereDistanceSphere(int srid,  double px0, double py0, double pz0, double pr, 
									double x, double y, double z, double r, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, pz0, x, y, z, srid );
	if ( arg.caseEqual("min") ) {
		dist -= r+pr;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += r+pr;
	}
    return true;
}

bool JagGeo::sphereDistanceEllipsoid(int srid,  double px0, double py0, double pz0, double pr,
	                                    double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, pz0, x0, y0, z0, srid );
	if ( arg.caseEqual("min") ) {
		dist -= pr + (w+d+h)/3.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += pr + (w+d+h)/3.0;
	}
    return true;
}

bool JagGeo::sphereDistanceCone(int srid,  double px0, double py0, double pz0, double pr,
	                                    double x0, double y0, double z0, double r, double h, 
										double nx, double ny, const Jstr& arg, double &dist )
{
	point3DDistanceCone(srid, px0, py0, pz0, x0, y0, z0, r, h, nx, ny,  arg, dist );
	if ( arg.caseEqual("min") ) {
		dist -= pr + (r+h)/2.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += pr + (r+h)/2.0;
	}
    return true;
}

// 2D rectangle
bool JagGeo::rectangleDistanceTriangle(int srid, double px, double py, double a0, double b0, double nx0, 
									 double x1, double y1, double x2, double y2, double x3, double y3, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px, py, x1, y1, srid );
	double dist2 = JagGeo::distance( px, py, x2, y2, srid );
	double dist3 = JagGeo::distance( px, py, x3, y3, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}

bool JagGeo::rectangleDistanceSquare(int srid, double px0, double py0, double a0, double b0, double nx0,
	                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, x0, y0, srid );
	if ( arg.caseEqual("min") ) {
		dist -= r;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += r;
	}
    return true;
}
bool JagGeo::rectangleDistanceRectangle(int srid, double px0, double py0, double a0, double b0, double nx0,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, x0, y0, srid );
	if ( arg.caseEqual("min") ) {
		dist -= (a0+b0+a+b)/4.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+a+b)/4.0;
	}
    return true;
}

bool JagGeo::rectangleDistanceEllipse(int srid, double px0, double py0, double a0, double b0, double nx0,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, x0, y0, srid );
	if ( arg.caseEqual("min") ) {
		dist -= (a0+b0+a+b)/4.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+a+b)/4.0;
	}
    return true;
}

bool JagGeo::rectangleDistanceCircle(int srid, double px0, double py0, double a0, double b0, double nx0, 
								    double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( px0, py0, x0, y0, srid );
	if ( arg.caseEqual("min") ) {
		dist -= (a0+b0)/2.0 + r;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0)/2.0 + r;
	}
    return true;
}

bool JagGeo::rectangleDistancePolygon(int srid, double px0, double py0, double a0, double b0, double nx0, 
									const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
	dist = 0.0;
	if ( sp2.length() < 1 ) { return false; }

    double dx, dy;
    const char *str;
    char *p;
	double max = JAG_LONG_MIN;
	double min = JAG_LONG_MAX;
	bool isMax;
	if ( arg.caseEqual("max") ) { isMax = true;	} else { isMax = false;	}
	long cnt = 0;
	double xsum = 0.0, ysum = 0.0;
	for ( int i = 2; i < sp2.length(); ++i ) {
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx, dy );
		if ( arg.caseEqual("center") ) {
			xsum += dx; ysum += dy; ++ cnt;
		} else {
			dist = JagGeo::distance( dx, dy, px0, py0, srid );
			if ( isMax ) {
				if ( dist > max ) max = dist;
			} else {
				if ( dist < min ) min = dist;
			}
		}
	}
	
	if ( arg.caseEqual("center") ) { 
		if ( cnt < 1 ) return false;
		dx = xsum/cnt;
		dy = ysum/cnt;
		dist = JagGeo::distance( dx, dy, px0, py0, srid );
	} else {
		if ( isMax ) { dist = max; }
		else { dist = min; }
	}
    return true;
}

// 2D triangle
bool JagGeo::triangleDistanceTriangle(int srid, double x10, double y10, double x20, double y20, double x30, double y30,
									 double x1, double y1, double x2, double y2, double x3, double y3, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x10, y10, x1, y1, srid );
	double dist2 = JagGeo::distance( x20, y20, x2, y2, srid );
	double dist3 = JagGeo::distance( x30, y30, x3, y3, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}

bool JagGeo::triangleDistanceSquare(int srid, double x10, double y10, double x20, double y20, double x30, double y30,
	                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x10, y10, x0, y0, srid );
	double dist2 = JagGeo::distance( x20, y20, x0, y0, srid );
	double dist3 = JagGeo::distance( x30, y30, x0, y0, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}
bool JagGeo::triangleDistanceRectangle(int srid, double x10, double y10, double x20, double y20, double x30, double y30,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x10, y10, x0, y0, srid );
	double dist2 = JagGeo::distance( x20, y20, x0, y0, srid );
	double dist3 = JagGeo::distance( x30, y30, x0, y0, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}
bool JagGeo::triangleDistanceEllipse(int srid, double x10, double y10, double x20, double y20, double x30, double y30,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x10, y10, x0, y0, srid );
	double dist2 = JagGeo::distance( x20, y20, x0, y0, srid );
	double dist3 = JagGeo::distance( x30, y30, x0, y0, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}

bool JagGeo::triangleDistanceCircle(int srid, double x10, double y10, double x20, double y20, double x30, double y30,
								    double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x10, y10, x0, y0, srid );
	double dist2 = JagGeo::distance( x20, y20, x0, y0, srid );
	double dist3 = JagGeo::distance( x30, y30, x0, y0, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}

bool JagGeo::triangleDistancePolygon(int srid, double x10, double y10, double x20, double y20, double x30, double y30,
								    const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
    double dx, dy;
    const char *str;
    char *p;
	double max = JAG_LONG_MIN;
	double min = JAG_LONG_MAX;
	double dist2, dist3;
	bool isMax;
	if ( arg.caseEqual("max") ) { isMax = true;	} else { isMax = false;	}
	for ( int i = 2; i < sp2.length(); ++i ) {
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx, dy );
		dist = JagGeo::distance( dx, dy, x10, y10, srid );
		dist2 = JagGeo::distance( dx, dy, x20, y20, srid );
		dist3 = JagGeo::distance( dx, dy, x30, y30, srid );
		if ( isMax ) {
			dist = jagmax3( dist, dist2, dist3 );
			if ( dist > max ) max = dist;
		} else {
			dist = jagmin3( dist, dist2, dist3 );
			if ( dist < min ) min = dist;
		}
	}

	if ( isMax ) { dist = max; }
	else { dist = min; }
    return true;
}
									
// 2D ellipse
bool JagGeo::ellipseDistanceTriangle(int srid, double px0, double py0, double a0, double b0, double nx0, 
									 double x1, double y1, double x2, double y2, double x3, double y3, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x1, y1, px0, py0, srid );
	double dist2 = JagGeo::distance( x2, y2, px0, py0, srid );
	double dist3 = JagGeo::distance( x3, y3, px0, py0, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}

bool JagGeo::ellipseDistanceSquare(int srid, double px0, double py0, double a0, double b0, double nx0,
	                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, px0, py0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= r; 
		dist = fabs( dist );
	} else {
		dist += r; 
	}
    return true;
}

bool JagGeo::ellipseDistanceRectangle(int srid, double px0, double py0, double a0, double b0, double nx0,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, px0, py0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+a+b)/4.0; 
		dist = fabs( dist );
	} else {
		dist += (a0+b0+a+b)/4.0; 
	}
    return true;
}

bool JagGeo::ellipseDistanceEllipse(int srid, double px0, double py0, double a0, double b0, double nx0,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, px0, py0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+a+b)/4.0; 
		dist = fabs( dist );
	} else {
		dist += (a0+b0+a+b)/4.0; 
	}
    return true;
}

bool JagGeo::ellipseDistanceCircle(int srid, double px0, double py0, double a0, double b0, double nx0, 
								    double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, px0, py0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+r)/3.0; 
		dist = fabs( dist );
	} else {
		dist += (a0+b0+r)/3.0; 
	}
    return true;
}

bool JagGeo::ellipseDistancePolygon(int srid, double px0, double py0, double a0, double b0, double nx0, 
								    const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
	dist = 0.0;
    double dx, dy;
    const char *str;
    char *p;
	double max = JAG_LONG_MIN;
	double min = JAG_LONG_MAX;
	bool isMax;
	long cnt = 0;
	double xsum = 0.0;
	double ysum = 0.0;
	if ( arg.caseEqual("max") ) { isMax = true;	} else { isMax = false;	}
	for ( int i = 2; i < sp2.length(); ++i ) {
		str = sp2[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx, dy );
		if ( arg.caseEqual("center") ) { 
			++cnt;
			xsum += dx; ysum +=dy;
		} else {
			dist = JagGeo::distance( dx, dy, px0, py0, srid );
			if ( isMax ) {
				if ( dist > max ) max = dist;
			} else {
				if ( dist < min ) min = dist;
			}
		}
	}

	if ( arg.caseEqual("center") ) { 
		if ( cnt < 1 ) return false;
		dx = xsum/cnt; dy = ysum/cnt;
		dist = JagGeo::distance( dx, dy, px0, py0, srid );
		return true;
	}

	if ( isMax ) { dist = max; }
	else { dist = min; }
    return true;
}

// rect 3D
bool JagGeo::rectangle3DDistanceCube(int srid,  double px0, double py0, double pz0, double a0, double b0, double nx0, double ny0,
                                double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+r)/3.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+r)/3.0;
	}
    return true;
}

bool JagGeo::rectangle3DDistanceBox(int srid,  double px0, double py0, double pz0, double a0, double b0,
                                double nx0, double ny0,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+w+d+h)/5.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+w+d+h)/5.0;
	}
    return true;
}
bool JagGeo::rectangle3DDistanceSphere(int srid,  double px0, double py0, double pz0, double a0, double b0,
                                       double nx0, double ny0,
                                       double x, double y, double z, double r, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x, y, z, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+r)/3.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+r)/3.0;
	}
    return true;
}
bool JagGeo::rectangle3DDistanceEllipsoid(int srid,  double px0, double py0, double pz0, double a0, double b0,
                                    double nx0, double ny0,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+w+d+h)/5.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+w+d+h)/5.0;
	}
    return true;
}
bool JagGeo::rectangle3DDistanceCone(int srid,  double px0, double py0, double pz0, double a0, double b0,
                                    double nx0, double ny0,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+r+h)/4.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+r+h)/4.0;
	}
    return true;
}


// triangle 3D
bool JagGeo::triangle3DDistanceCube(int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
								double x30, double y30,  double z30,
								double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x10, y10, z10, x0, y0, z0, srid );
	double dist2 = JagGeo::distance( x20, y20, z20, x0, y0, z0, srid );
	double dist3 = JagGeo::distance( x30, y30, z30, x0, y0, z0, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else if ( arg.caseEqual("min") ) {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}

bool JagGeo::triangle3DDistanceBox(int srid,  double x10, double y10, double z10, double x20, double y20, double z20, 
								double x30, double y30, double z30,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x10, y10, z10, x0, y0, z0, srid );
	double dist2 = JagGeo::distance( x20, y20, z20, x0, y0, z0, srid );
	double dist3 = JagGeo::distance( x30, y30, z30, x0, y0, z0, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else if ( arg.caseEqual("min") ) {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}
bool JagGeo::triangle3DDistanceSphere(int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
								   double x30, double y30, double z30,
                                       double x0, double y0, double z0, double r, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x10, y10, z10, x0, y0, z0, srid );
	double dist2 = JagGeo::distance( x20, y20, z20, x0, y0, z0, srid );
	double dist3 = JagGeo::distance( x30, y30, z30, x0, y0, z0, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else if ( arg.caseEqual("min") ) {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}
bool JagGeo::triangle3DDistanceEllipsoid(int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
										double x30, double y30, double z30,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x10, y10, z10, x0, y0, z0, srid );
	double dist2 = JagGeo::distance( x20, y20, z20, x0, y0, z0, srid );
	double dist3 = JagGeo::distance( x30, y30, z30, x0, y0, z0, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else if ( arg.caseEqual("min") ) {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}
bool JagGeo::triangle3DDistanceCone(int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
										double x30, double y30, double z30,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x10, y10, z10, x0, y0, z0, srid );
	double dist2 = JagGeo::distance( x20, y20, z20, x0, y0, z0, srid );
	double dist3 = JagGeo::distance( x30, y30, z30, x0, y0, z0, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else if ( arg.caseEqual("min") ) {
		dist = jagmin3( dist, dist2, dist3 );
	}
    return true;
}



// 3D box
bool JagGeo::boxDistanceCube(int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0,
                            double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+c0+r)/4.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+c0+r)/4.0;
	}
    return true;
}
bool JagGeo::boxDistanceBox(int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
					       double x0, double y0, double z0, double w, double d, double h, double nx, double ny, 
						   const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+c0+w+d+h)/6.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+c0+w+d+h)/6.0;
	}
    return true;
}
bool JagGeo::boxDistanceSphere(int srid, double px0, double py0, double pz0, double a0, double b0, double c0,
                                 double nx0, double ny0, double x, double y, double z, double r,
							 const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x, y, z, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+c0+r)/4.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+c0+r)/4.0;
	}
    return true;
}

bool JagGeo::boxDistanceEllipsoid(int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double w, double d, double h, 
								double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+c0+w+d+h)/6.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+c0+w+d+h)/6.0;
	}
    return true;
}

bool JagGeo::boxDistanceCone(int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double r, double h, 
								double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+c0+r+h)/5.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+c0+r+h)/5.0;
	}
    return true;
}

// ellipsoid
bool JagGeo::ellipsoidDistanceCube(int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0,
                            double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+c0+r)/4.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+c0+r)/4.0;
	}
    return true;
}
bool JagGeo::ellipsoidDistanceBox(int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
					       double x0, double y0, double z0, double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+c0+w+d+h)/6.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+c0+w+d+h)/6.0;
	}
    return true;
}
bool JagGeo::ellipsoidDistanceSphere(int srid, double px0, double py0, double pz0, double a0, double b0, double c0,
                                 double nx0, double ny0, double x0, double y0, double z0, double r,
							 const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+c0+r)/4.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+c0+r)/4.0;
	}
    return true;
}
bool JagGeo::ellipsoidDistanceEllipsoid(int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double w, double d, double h, 
								double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+c0+w+d+h)/6.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+c0+w+d+h)/6.0;
	}
    return true;
}

bool JagGeo::ellipsoidDistanceCone(int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double r, double h, 
								double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (a0+b0+c0+r+h)/5.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (a0+b0+c0+r+h)/5.0;
	}
    return true;
}

// 3D cyliner
bool JagGeo::cylinderDistanceCube(int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
	                               double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (pr0+c0+r)/3.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (pr0+c0+r)/3.0;
	}
    return true;
}
bool JagGeo::cylinderDistanceBox(int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
	                                double x0, double y0, double z0, double w, double d, double h, 
									double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (pr0+c0+w+d+h)/5.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (pr0+c0+w+d+h)/5.0;
	}
    return true;
}

bool JagGeo::cylinderDistanceSphere(int srid,  double px, double py, double pz, double pr0, double c0,  double nx0, double ny0,
									double x0, double y0, double z0, double r, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px, py, pz, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (pr0+c0+r)/3.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (pr0+c0+r)/3.0;
	}
    return true;
}
bool JagGeo::cylinderDistanceEllipsoid(int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
	                                    double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (pr0+c0+w+d+h)/5.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (pr0+c0+w+d+h)/5.0;
	}
    return true;
}

bool JagGeo::cylinderDistanceCone(int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
	                                    double x0, double y0, double z0, double r, double h, 
										double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (pr0+c0+r+h)/4.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (pr0+c0+r+h)/4.0;
	}
    return true;
}

// 3D cone
bool JagGeo::coneDistanceCube(int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
	                               double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (pr0+c0+r)/3.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (pr0+c0+r)/3.0;
	}
    return true;
}

bool JagGeo::coneDistanceBox(int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
	                                double x0, double y0, double z0, double w, double d, double h, 
									double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (pr0+c0+w+d+h)/5.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (pr0+c0+w+d+h)/5.0;
	}
    return true;
}

bool JagGeo::coneDistanceSphere(int srid,  double px, double py, double pz, double pr0, double c0,  double nx0, double ny0,
									double x0, double y0, double z0, double r, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px, py, pz, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (pr0+c0+r)/3.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (pr0+c0+r)/3.0;
	}
    return true;
}

bool JagGeo::coneDistanceEllipsoid(int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
	                                    double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (pr0+c0+w+d+h)/5.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (pr0+c0+w+d+h)/5.0;
	}
    return true;
}
bool JagGeo::coneDistanceCone(int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
	                                    double x0, double y0, double z0, double r, double h, 
										double nx, double ny, const Jstr& arg, double &dist )
{
	dist = JagGeo::distance( x0, y0, z0, px0, py0, pz0, srid );
	if ( arg.caseEqual("min") ) { 
		dist -= (pr0+c0+r+h)/4.0;
		dist = fabs( dist );
	} else if ( arg.caseEqual("max") ) {
		dist += (pr0+c0+r+h)/4.0;
	}
    return true;
}


// 2D line
bool JagGeo::lineDistanceTriangle(int srid, double x10, double y10, double x20, double y20, 
									 double x1, double y1, double x2, double y2, double x3, double y3,  const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		double tx, ty, rx, ry;
		triangleRegion( x1, y1, x2, y2, x3, y3, tx, ty, rx, ry );
		dist = JagGeo::distance( tx, ty, midx, midy, srid );
		return true;
	}

	dist = JagGeo::distance( x10, y10, x1, y1, srid );
	double dist2 = JagGeo::distance( x10, y10, x2, y2, srid );
	double dist3 = JagGeo::distance( x20, y20, x3, y3, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax3( dist, dist2, dist3 );
	} else {
		dist = jagmin3( dist, dist2, dist3 );
	} 
    return true;
}

bool JagGeo::lineDistanceLineString(int srid, double x10, double y10, double x20, double y20, 
	                              const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
	if ( sp2.length() < 3 ) { dist = 0.0; return false; }

	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		pointDistanceLineString( srid,  midx, midy, mk2, sp2, arg, dist );
		return true;
	}

	double dist1, dist2;  
	pointDistanceLineString( srid,  x10, y10,  mk2, sp2, arg, dist1 );
	pointDistanceLineString( srid,  x10, y10,  mk2, sp2, arg, dist2 );
	if ( arg.caseEqual("min") ) {
		dist = jagmin( dist1, dist2 );
	} else {
		dist = jagmax( dist1, dist2 );
	}
    return true;
}

bool JagGeo::lineDistanceSquare(int srid, double x10, double y10, double x20, double y20, 
	                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		dist = JagGeo::distance( midx, midy, x0, y0, srid );
		return true;
	}


	dist = JagGeo::distance( x0, y0, x10, y10, srid );
	double dist2 = JagGeo::distance( x0, y0, x20, y20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else if ( arg.caseEqual("min") ){
		dist = jagmin( dist, dist2 );
	} 
    return true;
}

bool JagGeo::lineDistanceRectangle(int srid, double x10, double y10, double x20, double y20, 
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		dist = JagGeo::distance( midx, midy, x0, y0, srid );
		return true;
	}

	dist = JagGeo::distance( x0, y0, x10, y10, srid );
	double dist2 = JagGeo::distance( x0, y0, x20, y20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}
    return true;
}

bool JagGeo::lineDistanceEllipse(int srid, double x10, double y10, double x20, double y20, 
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		dist = JagGeo::distance( midx, midy, x0, y0, srid );
		return true;
	}

	dist = JagGeo::distance( x0, y0, x10, y10, srid );
	double dist2 = JagGeo::distance( x0, y0, x20, y20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}
    return true;
}

bool JagGeo::lineDistanceCircle(int srid, double x10, double y10, double x20, double y20, 
								    double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		dist = JagGeo::distance( midx, midy, x0, y0, srid );
		return true;
	}

	dist = JagGeo::distance( x0, y0, x10, y10, srid );
	double dist2 = JagGeo::distance( x0, y0, x20, y20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}
    return true;
}

bool JagGeo::lineDistancePolygon(int srid, double x10, double y10, double x20, double y20, 
								const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
	if ( sp2.length() < 3 ) { dist = 0.0; return false; }
	const char *str = sp2[JAG_SP_START+2].c_str();
	char *p;
	double dx, dy;
	get2double(str, p, ':', dx, dy );
	dist = JagGeo::distance( dx, dy, x10, y10, srid );
	double dist2 = JagGeo::distance( dx, dy, x20, y20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}
    return true;
}


// line 3D
bool JagGeo::line3DDistanceLineString3D(int srid, double x10, double y10, double z10, double x20, double y20, double z20, 
	                              const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
	if ( sp2.length() < 3 ) { dist = 0.0; return false; }
	const char *str = sp2[JAG_SP_START+2].c_str();
	char *p;
	double dx, dy, dz;
	get3double(str, p, ':', dx, dy, dz );
	dist = JagGeo::distance( dx, dy, dz, x10, y10, z10, srid );
	double dist2 = JagGeo::distance( dx, dy, dz, x20, y20, z20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}
    return true;
}

bool JagGeo::line3DDistanceCube(int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
								double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		double midz = (z10+z20)/2.0;
		dist = JagGeo::distance( midx, midy, midz, x0, y0, z0, srid );
		return true;
	}

	dist = JagGeo::distance( x0, y0, z0, x10, y10, z10, srid );
	double dist2 = JagGeo::distance( x0, y0, z0, x20, y20, z20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}
    return true;
}

bool JagGeo::line3DDistanceBox(int srid,  double x10, double y10, double z10, double x20, double y20, double z20, 
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		double midz = (z10+z20)/2.0;
		dist = JagGeo::distance( midx, midy, midz, x0, y0, z0, srid );
		return true;
	}

	dist = JagGeo::distance( x0, y0, z0, x10, y10, z10, srid );
	double dist2 = JagGeo::distance( x0, y0, z0, x20, y20, z20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}
    return true;
}

bool JagGeo::line3DDistanceSphere(int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
                                       double x0, double y0, double z0, double r, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		double midz = (z10+z20)/2.0;
		dist = JagGeo::distance( midx, midy, midz, x0, y0, z0, srid );
		return true;
	}

	dist = JagGeo::distance( x0, y0, z0, x10, y10, z10, srid );
	double dist2 = JagGeo::distance( x0, y0, z0, x20, y20, z20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}
    return true;
}
bool JagGeo::line3DDistanceEllipsoid(int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		double midz = (z10+z20)/2.0;
		dist = JagGeo::distance( midx, midy, midz, x0, y0, z0, srid );
		return true;
	}

	dist = JagGeo::distance( x0, y0, z0, x10, y10, z10, srid );
	double dist2 = JagGeo::distance( x0, y0, z0, x20, y20, z20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}
    return true;
}
bool JagGeo::line3DDistanceCone(int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual("center") ) {
		double midx = (x10+x20)/2.0;
		double midy = (y10+y20)/2.0;
		double midz = (z10+z20)/2.0;
		dist = JagGeo::distance( midx, midy, midz, x0, y0, z0, srid );
		return true;
	}

	dist = JagGeo::distance( x0, y0, z0, x10, y10, z10, srid );
	double dist2 = JagGeo::distance( x0, y0, z0, x20, y20, z20, srid );
	if ( arg.caseEqual("max") ) { 
		dist = jagmax( dist, dist2 );
	} else {
		dist = jagmin( dist, dist2 );
	}
    return true;
}



// linestring 2d
bool JagGeo::lineStringDistanceLineString(int srid, const Jstr &mk1, const JagStrSplit &sp1,
										const Jstr &mk2, const JagStrSplit &sp2,  const Jstr& arg, double &dist )
{
	int start = JAG_SP_START;
	int start2 = JAG_SP_START;
    if (arg.caseEqual( "center" )) {
            double px, py, px1, py1;
            lineStringAverage(mk1, sp1, px, py);
            lineStringAverage(mk2, sp2, px1, py1);
            dist = JagGeo::distance( px, py, px1, py1, srid );
            return true;
     }

    double dx, dy, dd, dx2, dy2;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    const char *str;
    const char *str1;
    char *p;
    char *p1;

    for ( int i = start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );

        for ( int j = start2; j < sp2.length(); ++j ) {
                str1 = sp2[j].c_str();
                d("%s\n", str1);
                if ( strchrnum( str, ':') < 1 ) continue;
                get2double(str1, p1, ':', dx2, dy2 );
                dd = JagGeo::distance( dx, dy, dx2, dy2, srid );
                d("dx:%f\n dy:%f\n dx2:%f\n dy2:%f\n d:%f\n", dx, dy, dx2, dy2,dd);
                if ( dd < mind ) mind = dd;
                if ( dd > maxd ) maxd = dd;
                }
    }
    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else {
        dist = mind;
    }

    return true;
}


bool JagGeo::lineStringDistanceTriangle(int srid, const Jstr &mk1, const JagStrSplit &sp1,
									 double x1, double y1, double x2, double y2, double x3, double y3,  const Jstr& arg, double &dist )
{
	int start = JAG_SP_START;
    double dx, dy, min, max, d1, d2, d3;
	double mind = JAG_LONG_MAX;
	double maxd = JAG_LONG_MIN;
    const char *str;
    char *p;

    if (arg.caseEqual( "center" )) {
            double px, py;
            lineStringAverage(mk1, sp1, px, py);
            dist = JagGeo::distance( px, py, (x1 + x2 + x3) / 3, (y1 + y2 + y3) / 3, srid );
            return true;
     }

	for ( int i=start; i < sp1.length(); ++i ) {
		str = sp1[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', dx, dy );
		d1 = JagGeo::distance( x1, y1, dx, dy, srid );
		d2 = JagGeo::distance( x2, y2, dx, dy, srid );
		d3 = JagGeo::distance( x3, y3, dx, dy, srid );
		min = jagmin3(d1,d2,d3);
		max = jagmax3(d1,d2,d3);
		if ( min < mind ) mind = min;
		if ( max > maxd ) maxd = max;
	}

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else {
        dist = mind;
    }

    return true;

}
bool JagGeo::lineStringDistanceSquare(int srid, const Jstr &mk1, const JagStrSplit &sp1,
	                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
	int start = JAG_SP_START;

    if (arg.caseEqual( "center" )) {
            double px, py;
            lineStringAverage(mk1, sp1, px, py);
            dist = JagGeo::distance( px, py, x0, y0, srid );
            return true;
     }

    double dx, dy, d;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );
        d = JagGeo::distance( x0, y0, dx, dy, srid );
        if ( d < mind ) mind = d;
        if ( d > maxd ) maxd = d;
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd + r;
    } else {
        dist = mind - r;
    }

    return true;
}



bool JagGeo::lineStringDistanceRectangle(int srid, const Jstr &mk1, const JagStrSplit &sp1,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
	int start = JAG_SP_START;

    if (arg.caseEqual( "center" )) {
            double px, py;
            lineStringAverage(mk1, sp1, px, py);
            dist = JagGeo::distance( px, py, x0, y0, srid );
            return true;
     }
    d("rect----------------------\n");
    double dx, dy, mind1, maxd1, px, py;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    const char *str;
    char *p;
    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );
        transform2DCoordGlobal2Local( x0, y0, dx, dy, nx, px, py );
        if(fabs(px) <= a){
            //point to up and down lines
            mind1 = fabs(fabs(py) - b);
            maxd1 = JagGeo::distance( fabs(px), fabs(py), -a, -b, srid);
            if ( mind1 < mind ) mind = mind1;
            if ( maxd1 > maxd ) maxd = maxd1;
            d("4 min---%f\n", mind1);
            d("4 max---%f\n", maxd1);
            continue;
        }else if(fabs(py) <= b){
            //point to left and right lines
            mind1 = fabs(fabs(px) - a);
            maxd1 = JagGeo::distance( fabs(px), fabs(py), -a, -b, srid );
            if ( mind1 < mind ) mind = mind1;
            if ( maxd1 > maxd ) maxd = maxd1;
            d("5 min---%f\n", mind1);
            d("5 max---%f\n", maxd1);
            continue;
        }else{
            //point to 4 points
            if ( arg.caseEqual( "min" ) ) {
                mind1 = JagGeo::distance( fabs(px), fabs(py), a, b, srid );
                if (mind1 < mind) {
                mind = mind1;
                }
            }
            if ( arg.caseEqual( "max" ) ) {
                maxd1 = JagGeo::distance( fabs(px), fabs(py),-a, -b, srid );
                if ( maxd1 > maxd ) {
                maxd = maxd1;
                }
            }
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else {
        dist = mind;
    }

    return true;
}
bool JagGeo::lineStringDistanceEllipse(int srid, const Jstr &mk1, const JagStrSplit &sp1,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
	int start = JAG_SP_START;

    double dx, dy, d;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    //double xsum = 0.0, ysum = 0.0;
    //long counter = 0;
    const char *str;
    char *p;

    if (arg.caseEqual( "center" )) {
        double px, py;
        lineStringAverage(mk1, sp1, px, py);
        dist = JagGeo::distance( px, py, x0, y0, srid );
        return true;
    }

    for (int i = start; i<sp1.length(); ++i){
        str = sp1[i].c_str();
        //printf("%s\n",str.c_str());
        if(strchrnum(str, ':') < 1) continue;
        get2double(str,p,':',dx,dy);
        //double px, py;
        //transform2DCoordGlobal2Local( x0,y0, dx, dy, nx, px, py ); //relocate ellipse center to origin point. transform (dx,dy) to (px,py)
        //d = JagGeo::distance( x0, y0, px, py, srid ); //calculate each point on linestring to center of ellipse
        if ( arg.caseEqual( "max" ) ) {
            d = pointDistanceToEllipse( srid, dx, dy, x0, y0, a, b, nx, false );
            if ( d > maxd ) maxd = d;
        } else if ( arg.caseEqual( "min" ) ) {
            d = pointDistanceToEllipse( srid, dx, dy, x0, y0, a, b, nx, true );
            if ( d < mind ) mind = d;
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        // dist = maxd + jagmax(a,b);
        dist = maxd;
    } else if ( arg.caseEqual( "min" ) ){
        // dist = mind - jagmax(a,b);
        dist = mind;
    }
    return true;
}

bool JagGeo::lineStringDistanceCircle(int srid, const Jstr &mk1, const JagStrSplit &sp1,
								    double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
	int start = JAG_SP_START;
    double dx, dy, d;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    //double xsum = 0, ysum = 0;
    //long counter = 0;
    const char *str;
    char *p;

    if (arg.caseEqual( "center" )) {
        double px, py;
        lineStringAverage(mk1, sp1, px, py);
        dist = JagGeo::distance( px, py, x0, y0, srid );
        return true;
    }

    for (int i = start; i<sp1.length(); ++i){
        str = sp1[i].c_str();
        if(strchrnum(str, ':') < 1) continue;
        get2double(str,p,':',dx,dy);//get current (dx, dy), each pair is one point on the linestring
        d = JagGeo::distance( x0, y0, dx, dy, srid ); //calculate each point on linestring to center of circle
        if ( d < mind ) mind = d;
        if ( d > maxd ) maxd = d;
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd + r;
    } else if (arg.caseEqual( "min" )) {
        dist = mind - r;
    }
    return true;
}

bool JagGeo::lineStringDistancePolygon(int srid, const Jstr &mk1, const JagStrSplit &sp1,
								     const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
    JagPolygon pgon;
    int rc;
	int start = JAG_SP_START;
	//int start2 = JAG_SP_START;
    double dx, dy, dd;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    const char *str;
    char *p;
    //char *p2;
    //d("s10001 colType2=[%s]\n", colType2.c_str() );
    //d("s10001 mk1=[%s]\n , mk2=[%s]\n", mk1.c_str(), mk2.c_str() );
    rc = JagParser::addPolygonData( pgon, sp2, false );
    if ( rc < 0 ) {
        return false;
    }
    //pgon.print();

    if (arg.caseEqual( "center" )) {
        double px, py;
        lineStringAverage(mk1, sp1, px, py);
        double cx, cy;
        pgon.center2D(cx, cy);
        //d("s10006 px=[%f] py=[%f] cx=[%f] cy=[%f]", px, py, cx, cy);
        dist = JagGeo::distance( px, py, cx, cy, srid );
        return true;
    }

    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get2double(str, p, ':', dx, dy );
        for (int j = 0 ; j < pgon.linestr.size(); ++j){
            d("linestr.size()=[%]", pgon.linestr.size());
            JagLineString3D &linestr = pgon.linestr[j];
            //linestr.print();
            for ( int k=0; k < linestr.size()-1; ++k ) {  //first point = last point,so pass
                d("s10003 dx=[%f] dy=[%f] px=[%f] py=[%f]\n",linestr.point[k].x, linestr.point[k].y);
                dd = JagGeo::distance( dx, dy, linestr.point[k].x, linestr.point[k].y, srid );
                d("s10003 d=[%f]\n",dd);
                if ( dd > maxd ) maxd = dd;
                if ( dd < mind ) mind = dd;
            }
        }

    }
    //d("s10005 xsum=[%f] ysum=[%f] xsum2=[%f] ysum2=[%f]\n",  xsum, ysum, xsum2, ysum2 );

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else  {
        dist = mind;
    }

    return true;
}


// linestring3d
bool JagGeo::lineString3DDistanceLineString3D(int srid, const Jstr &mk1, const JagStrSplit &sp1,
										    const Jstr &mk2, const JagStrSplit &sp2,  const Jstr& arg, double &dist )
{
	int start = JAG_SP_START;
	int start2 = JAG_SP_START;

    double dx, dy, dz, d, dx2, dy2, dz2;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    //double xsum = 0, ysum = 0, zsum = 0, xsum2 = 0, ysum2 = 0, zsum2 = 0;
    //long counter = 0;
    const char *str;
    const char *str2;
    char *p;
    char *p2;

    if (arg.caseEqual( "center" )){
        double px, py, pz;
        lineString3DAverage(mk1, sp1, px, py, pz);
        double cx, cy, cz;
        lineString3DAverage(mk2, sp2, cx, cy, cz);
        dist = JagGeo::distance( px, py, pz, cx, cy, cz, srid );
        return true;
    }

    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 2  ) continue;
        get3double(str, p, ':', dx, dy, dz );
        for ( int i=start2; i < sp2.length(); ++i ) {
                str2 = sp2[i].c_str();
                if ( strchrnum( str2, ':') < 1 ) continue;
                get3double(str2, p2, ':', dx2, dy2, dz2 );
                d = JagGeo::distance( dx, dy, dz, dx2, dy2, dz2, srid );
                if ( d < mind ) mind = d;
                if ( d > maxd ) maxd = d;

        }
    }

    if (arg.caseEqual( "max" )) {
        dist = maxd;
    } else {
        dist = mind;
    }

    return true;
}
bool JagGeo::lineString3DDistanceCube(int srid, const Jstr &mk1, const JagStrSplit &sp1,
								double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
    lineString3DDistanceBox(srid, mk1, sp1, x0, y0, z0, r, r, r, nx, ny, arg, dist);
    return true;
}

double JagGeo::DistanceOfPointToLine(double x0 ,double y0 ,double z0 ,double x1 ,double y1 ,double z1 ,double x2 ,double y2 ,double z2)
{
    double ab = sqrt(pow((x1 - x2), 2.0) + pow((y1 - y2), 2.0) + pow((z1 - z2), 2.0));
    double as = sqrt(pow((x1 - x0), 2.0) + pow((y1 - y0), 2.0) + pow((z1 - z0), 2.0));
    double bs = sqrt(pow((x0 - x2), 2.0) + pow((y0 - y2), 2.0) + pow((z0 - z2), 2.0));
    double cos_A = (pow(as, 2.0) + pow(ab, 2.0) - pow(bs, 2.0)) / (2 * ab*as);
    double sin_A = sqrt(1 - pow(cos_A, 2.0));
    return as*sin_A;
}

bool JagGeo::lineString3DDistanceBox(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
    // sp1.print();
    // sp2.print();
    //	dist = 0.0;
    //sp1.print();
	int start = JAG_SP_START;
    double dx, dy, dz, px, py, pz, maxd1, mind1;
    double xsum = 0, ysum = 0, zsum = 0;
    long counter = 0;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    const char *str;
    char *p;

    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get3double(str, p, ':', dx, dy, dz );
        transform3DCoordGlobal2Local( x0, y0, z0, dx, dy, dz, nx, ny, px, py, pz );
        if (fabs(px) <= d && fabs(py) <= w){
            //point to up and down sides
            mind1 = fabs(fabs(pz) - h);
            maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
            if ( mind1 < mind ) mind = mind1;
            if ( maxd1 > maxd ) maxd = maxd1;
            continue;
        }else if(fabs(py) <= w && fabs(pz) < h){
            //point to front and back sides
            mind1 = fabs(fabs(px) - d);
            maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
            if ( mind1 < mind ) mind = mind1;
            if ( maxd1 > maxd ) maxd = maxd1;
            continue;
        }else if(fabs(px) <= d && fabs(pz) < h){
            //point to left and right sides
            mind1 = fabs(fabs(py) - w);
            maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
            if ( mind1 < mind ) mind = mind1;
            if ( maxd1 > maxd ) maxd = maxd1;
            continue;
        }else if(fabs(px) <= d){
            //point to 4 depth lines
            mind1 = DistanceOfPointToLine(fabs(px), fabs(py), fabs(pz), d, w, h, -d, w, h);
            maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
            if ( mind1 < mind ) mind = mind1;
            if ( maxd1 > maxd ) maxd = maxd1;
            continue;
        }else if(fabs(py) <= w){
            //point to 4 width lines
            mind1 = DistanceOfPointToLine(fabs(px), fabs(py), fabs(pz), d, w, h, d, -w, h);
            maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
            if ( mind1 < mind ) mind = mind1;
            if ( maxd1 > maxd ) maxd = maxd1;
            continue;
        }else if(fabs(pz) <= h){
            //point to 4 height lines
            mind1 = DistanceOfPointToLine(fabs(px), fabs(py), fabs(pz), d, w, h, d, w, -h);
            maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
            if ( mind1 < mind ) mind = mind1;
            if ( maxd1 > maxd ) maxd = maxd1;
            continue;
        }else{
            //point to 8 points
            mind1 = distance( fabs(px), fabs(py), fabs(pz), d, w, h, srid );
            maxd1 = distance( fabs(px), fabs(py), fabs(pz), -d, -w, -h, srid );
            if ( mind1 < mind ) mind = mind1;
            if ( maxd1 > maxd ) maxd = maxd1;
            continue;
        }
        if  (arg.caseEqual("center" )) {
            xsum = xsum + px;
            ysum = ysum + py;
            zsum = zsum + pz;
            counter ++;
        }
    }
    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else if (arg.caseEqual("min" )){
        dist = mind;
    } else if  (arg.caseEqual("center" )){
        if ( 0 == counter ) dist = 0.0;
        else dist = JagGeo::distance( xsum / counter, ysum / counter, zsum / counter, 0, 0, 0, srid );
    }
    return true;
}
bool JagGeo::lineString3DDistanceSphere(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, const Jstr& arg, double &dist )
{
	int start = JAG_SP_START;

    double dx, dy, dz, dd;
    double xsum = 0, ysum = 0, zsum = 0;
    long counter = 0;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    const char *str;
    char *p;

    for ( int i=start; i < sp1.length(); ++i ) {
        str = sp1[i].c_str();
        if ( strchrnum( str, ':') < 1 ) continue;
        get3double(str, p, ':', dx, dy, dz );
        if ( arg.caseEqual( "max" ) || arg.caseEqual("min" ) ) {
            dd = JagGeo::distance( dx, dy, dz, x, y, z, srid );
            d("dx:%f,\n dy:%f\n dz:%f\n x:%f\n y:%f\n z:%f\n d:%f\n", dx, dy, dz, x, y, z, d);
            if ( dd < mind ) mind = dd;
            if ( dd > maxd ) maxd = dd;
        } else if  (arg.caseEqual("center" )) {
            xsum = xsum + dx;
            ysum = ysum + dy;
            zsum = zsum + dz;
            counter ++;
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd + r;
    } else if (arg.caseEqual("min" )){
        dist = mind - r;
    } else if  (arg.caseEqual("center" )){
        if ( 0 == counter ) dist = 0.0;
        else dist = JagGeo::distance( xsum / counter, ysum / counter, zsum / counter, x, y, z, srid );
    }

    return true;

}
bool JagGeo::lineString3DDistanceEllipsoid(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double dd, double h, double nx, double ny, const Jstr& arg, double &dist )
{
    if  (arg.caseEqual("center" )){
		// get center average of linestring3d
		double cx, cy, cz;
		lineString3DAverage( mk1, sp1, cx, cy, cz );
		dist = JagGeo::distance( cx, cy, cz, x0, y0, z0, srid );
		return true;
	}

    const char *str;
    char *p;
	int start = JAG_SP_START;
    double dx, dy, dz, dst;
    if  (arg.caseEqual("min" )){
        double mind = JAG_LONG_MAX;
        for ( int i=start; i < sp1.length(); ++i ) {
            str = sp1[i].c_str();
            d("i:%d\n",i);
            d("%s\n", str);
            if ( strchrnum( str, ':') < 1 ) continue;
            get3double(str, p, ':', dx, dy, dz );
			point3DDistanceEllipsoid( srid, dx, dy, dz, x0,y0,z0, w, dd, h, nx,ny, arg, dst );
            d("dx:%f,\n dy:%f\n dz:%f\n x:%f\n y:%f\n z:%f\n d:%f", dx, dy, dz, x0, y0, z0, dst);
           	if ( dst < mind ) mind = dst;
        }
		dist = mind;
	}

    if  (arg.caseEqual("max" )){
        double maxd = JAG_LONG_MIN;
        for ( int i=start; i < sp1.length(); ++i ) {
            str = sp1[i].c_str();
            if ( strchrnum( str, ':') < 1 ) continue;
            get3double(str, p, ':', dx, dy, dz );
			point3DDistanceEllipsoid( srid, dx, dy, dz, x0,y0,z0, w, dd, h, nx,ny, arg, dst );
           	if ( dst < maxd ) maxd = dst;
        }
		dist = maxd;
	}

    return true;
}

bool JagGeo::lineString3DDistanceCone(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist )
{
    if  (arg.caseEqual("center" )){
		// get center average of linestring3d
		double cx,cy,cz;
		lineString3DAverage( mk1, sp1, cx, cy, cz );
		dist = JagGeo::distance( cx, cy, cz, x0, y0, z0, srid );
		return true;
	}

    const char *str;
    char *p;
	int start = JAG_SP_START;
    double dx, dy, dz, d;
    if  (arg.caseEqual("min" )){
        double mind = JAG_LONG_MAX;
        for ( int i=start; i < sp1.length(); ++i ) {
            str = sp1[i].c_str();
            if ( strchrnum( str, ':') < 1 ) continue;
            get3double(str, p, ':', dx, dy, dz );
			point3DDistanceCone( srid, dx, dy, dz, x0,y0,z0, r, h, nx,ny, arg, d );
           	if ( d < mind ) mind = d;
        }
		return mind;
	}

    if  (arg.caseEqual("max" )){
        double maxd = JAG_LONG_MIN;
        for ( int i=start; i < sp1.length(); ++i ) {
            str = sp1[i].c_str();
            if ( strchrnum( str, ':') < 1 ) continue;
            get3double(str, p, ':', dx, dy, dz );
			point3DDistanceCone( srid, dx, dy, dz, x0,y0,z0, r, h, nx,ny, arg, d );
           	if ( d < maxd ) maxd = d;
        }
		return maxd;
	}

    return true;
}


// polygon
bool JagGeo::polygonDistanceTriangle(int srid, const Jstr &mk1, const JagStrSplit &sp1,
									 double x1, double y1, double x2, double y2, double x3, double y3,  const Jstr& arg, double &dist )
{

    JagPolygon pgon;

    //const char *str;
    //char *p1;
    int rc;
    double d1, d2, d3, mind, maxd;
    double min = JAG_LONG_MAX;
    double max = JAG_LONG_MIN;
    double xsum = 0, ysum = 0;
    long counter = 0;

	//int start = JAG_SP_START;
    rc = JagParser::addPolygonData( pgon, sp1, true );
    if ( rc < 0 ) { return false; }

    const JagLineString3D &linestr = pgon.linestr[0];
    for ( int i=0; i < linestr.size()-1; ++i ) {//first point = last point,so pass
        d1 = JagGeo::distance( x1, y1, linestr.point[i].x, linestr.point[i].y, srid );
        d2 = JagGeo::distance( x2, y2, linestr.point[i].x, linestr.point[i].y, srid );
        d3 = JagGeo::distance( x3, y3, linestr.point[i].x, linestr.point[i].y, srid );
        if ( arg.caseEqual( "min" ) ) {
            mind = jagmin3(d1,d2,d3);
            d("mind:%f,\n",mind);
            if (mind < min){
                min = mind;
            }
        } else if (arg.caseEqual( "max" )) {
            maxd = jagmax3(d1,d2,d3);
            if (maxd > max){
                max = maxd;
            }
        } else if (arg.caseEqual("center" )) {
            xsum = xsum + linestr.point[i].x;
            ysum = ysum + linestr.point[i].y;
            counter++;
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = max;
    } else if (arg.caseEqual( "min" )) {
        dist = min;
    } else if (arg.caseEqual( "center" )) {
        //double px, py;
        if ( 0 == counter ) dist = 0.0;
        else dist = JagGeo::distance( (x1 + x2 + x3) / 3, (y1 + y2 +y3) / 3, xsum / counter, ysum / counter, srid );
    }
    return true;
}
bool JagGeo::polygonDistanceSquare(int srid, const Jstr &mk1, const JagStrSplit &sp1,
	                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
    polygonDistanceRectangle(srid, mk1, sp1, x0, y0, r, r, nx, arg, dist);
    return true;
}
bool JagGeo::polygonDistanceRectangle(int srid, const Jstr &mk1, const JagStrSplit &sp1,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
    JagPolygon pgon;
    int rc;
    double d;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    rc = JagParser::addPolygonData( pgon, sp1, false );
    if ( rc < 0 ) { return false; }

    if (arg.caseEqual( "center" )) {
        double cx, cy;
        pgon.center2D(cx, cy);
        dist = JagGeo::distance( cx, cy, x0, y0, srid );
        return true;
    }

    for (int j = 0; j < pgon.linestr.size(); ++j){
        const JagLineString3D &linestr = pgon.linestr[j];
        for ( int i=0; i < linestr.size()-1; ++i ) {//first point = last point,so pass
            pointDistanceRectangle(srid,x0,y0,linestr.point[i].x, linestr.point[i].y, a, b, nx, arg, d);
            if ( d > maxd ) maxd = d;
            if ( d < mind ) mind = d;
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else if (arg.caseEqual( "min" )) {
        dist = mind;
    } 

    return true;
}

bool JagGeo::polygonDistanceEllipse(int srid, const Jstr &mk1, const JagStrSplit &sp1,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
    JagPolygon pgon;
    int rc;
	double d;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    rc = JagParser::addPolygonData( pgon, sp1, false );
    if ( rc < 0 ) {
        return false;
    }

    if (arg.caseEqual( "center" )) {
        double cx, cy;
        pgon.center2D(cx,cy);
        dist = JagGeo::distance( cx, cy, x0, y0, srid );
        return true;
    }

    for (int j = 0; j < pgon.linestr.size(); ++j){
        const JagLineString3D &linestr = pgon.linestr[j];
        for ( int i=0; i < linestr.size()-1; ++i ) {//first point = last point,so pass
            pointDistanceEllipse( srid,linestr.point[i].x, linestr.point[i].y, x0, y0, a, b, nx, arg, d);
            if ( d > maxd ) maxd = d;
            if ( d < mind ) mind = d;
        }
    }

    if ( arg.caseEqual( "max" ) ) {
           dist = maxd;
    } else {
           dist = mind;
    }

    return true;

}

bool JagGeo::polygonDistanceCircle(int srid, const Jstr &mk1, const JagStrSplit &sp1,
								    double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
    JagPolygon pgon;
    int rc;
    double d;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    rc = JagParser::addPolygonData( pgon, sp1, false );
    if ( rc < 0 ) { return false; }

    if (arg.caseEqual( "center" )) {
        double cx, cy;
        pgon.center2D(cx,cy);
        dist = JagGeo::distance( cx, cy, x0, y0, srid );
        return true;
    }

    for (int j = 0; j < pgon.linestr.size(); ++j){
        const JagLineString3D &linestr = pgon.linestr[j];
        for ( int i=0; i < linestr.size()-1; ++i ) {//first point = last point,so pass
            pointDistanceCircle(srid,linestr.point[i].x, linestr.point[i].y, x0, y0, r, arg, d);
            if ( d > maxd ) maxd = d;
            if ( d < mind ) mind = d;
        }
    }

    if ( arg.caseEqual( "max" ) ) {
           dist = maxd;
    } else {
           dist = mind;
    }

    return true;
}
bool JagGeo::polygonDistancePolygon(int srid, const Jstr &mk1, const JagStrSplit &sp1,
									const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
    JagPolygon pgon1;
    JagPolygon pgon2;

    int rc1, rc2;
    double d;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    rc1 = JagParser::addPolygonData( pgon1, sp1, false );
    if ( rc1 < 0 ) { return false; }
    rc2 = JagParser::addPolygonData( pgon2, sp2, false );
    if ( rc2 < 0 ) { return false; }

    if (arg.caseEqual( "center" )) {
        double px, py, cx, cy;
        pgon1.center2D(px,py);
        pgon2.center2D(cx,cy);
        dist = JagGeo::distance( px, py, cx, cy, srid );
		return true;
    }

    //pgon1.print();
    //pgon2.print();
    for ( int i=0; i < pgon1.size(); ++i ) {
        JagLineString3D &linestr1 = pgon1.linestr[i];
        for ( int j=0; j < linestr1.size()-1; ++j ) {
            for ( int k=0; k < pgon2.size(); ++k ) {
                JagLineString3D &linestr2 = pgon2.linestr[k];
                for ( int m=0; m < linestr2.size()-1; ++m ) {
                   d = JagGeo::distance( linestr1.point[j].x, linestr1.point[j].y, linestr2.point[m].x, linestr2.point[m].y, srid );
                   if ( d > maxd ) maxd = d;
                   if ( d < mind ) mind = d;
                }
            }
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else {
        dist = mind;
    }

    return true;
}

// polygon3d Distance
bool JagGeo::polygon3DDistanceCube(int srid, const Jstr &mk1, const JagStrSplit &sp1,
								double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
    polygon3DDistanceBox(srid, mk1, sp1, x0, y0, z0, r, r, r, nx, ny, arg, dist);
    return true;
}

bool JagGeo::polygon3DDistanceBox(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
    JagPolygon pgon;
    int rc;
    //double d2, d3, min, max;
    double xsum = 0, ysum = 0, zsum = 0;
    long counter = 0;
    double dist1;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;

    rc = JagParser::addPolygon3DData( pgon, sp1, true );
    if ( rc < 0 ) { return false; }

    const JagLineString3D &linestr1 = pgon.linestr[0];

    for ( int i=0; i < linestr1.size()-1; ++i ) {//first point = last point,so pass
        if ( arg.caseEqual( "max" ) ) {
            point3DDistanceBox( srid, linestr1.point[i].x, linestr1.point[i].y, linestr1.point[i].z, x0, y0, z0, w, d, h, nx, ny, arg, dist1 );
            if (dist1 > maxd){
                maxd = dist1;
         }
        } else if ( arg.caseEqual( "min" ) ) {
            point3DDistanceBox( srid, linestr1.point[i].x, linestr1.point[i].y, linestr1.point[i].z, x0, y0, z0, w, d, h, nx, ny, arg, dist1 );
            if (dist1 < mind){
                mind = dist1;
            }
        } else if ( arg.caseEqual("center" ) ) {
            xsum = xsum + linestr1.point[i].x;
            ysum = ysum + linestr1.point[i].y;
            zsum = zsum + linestr1.point[i].z;
            counter++;
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else if (arg.caseEqual( "min" )) {
        dist = mind;
    } else if (arg.caseEqual( "center" )) {
        if ( 0 == counter ) dist = 0.0;
        else dist = JagGeo::distance( x0, y0, z0, xsum / counter, ysum / counter, zsum / counter, srid );
    }

    return true;
}
bool JagGeo::polygon3DDistanceSphere(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, const Jstr& arg, double &dist )
{
	JagPolygon pgon;
    int rc;
    //double d1, d2, d3, min, max;
    double xsum = 0, ysum = 0, zsum = 0;
    long counter = 0;
    double dist1;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;

    rc = JagParser::addPolygon3DData( pgon, sp1, true );
    if ( rc < 0 ) { return false; }

    const JagLineString3D &linestr1 = pgon.linestr[0];

    for ( int i=0; i < linestr1.size()-1; ++i ) {//first point = last point,so pass
        dist1 = JagGeo::distance( linestr1.point[i].x, linestr1.point[i].y, linestr1.point[i].z, x, y, z, srid );
        if ( arg.caseEqual( "max" ) ) {
            if (dist1 > maxd){
                maxd = dist1;
         }
        }else if ( arg.caseEqual( "min" ) ) {
            if (dist1 < mind){
                mind = dist1;
            }
        }else if ( arg.caseEqual("center" ) ) {
            xsum = xsum + linestr1.point[i].x;
            ysum = ysum + linestr1.point[i].y;
            zsum = zsum + linestr1.point[i].z;
            counter++;
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd + r;
    } else if (arg.caseEqual( "min" )) {
        dist = mind - r;
    } else if (arg.caseEqual( "center" )) {
        if ( 0 == counter ) dist = 0.0;
        else dist = JagGeo::distance( x, y, z, xsum / counter, ysum / counter, zsum / counter, srid );
    }

    return true;
}
bool JagGeo::polygon3DDistanceEllipsoid(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
    JagPolygon pgon;
    int rc;
    //double d1, d2, d3, min, max;
    double xsum = 0, ysum = 0, zsum = 0;
    long counter = 0;
    double dist1;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;

    rc = JagParser::addPolygon3DData( pgon, sp1, true );
    if ( rc < 0 ) { return false; }

    const JagLineString3D &linestr1 = pgon.linestr[0];

    for ( int i=0; i < linestr1.size()-1; ++i ) {//first point = last point,so pass
        if ( arg.caseEqual( "max" ) ) {
            point3DDistanceEllipsoid( srid, linestr1.point[i].x, linestr1.point[i].y, linestr1.point[i].z, x0, y0, z0, w, d, h, nx, ny, arg, dist1 );
            if (dist1 > maxd){
                maxd = dist1;
         }
        }else if ( arg.caseEqual( "min" ) ) {
            point3DDistanceEllipsoid( srid, linestr1.point[i].x, linestr1.point[i].y, linestr1.point[i].z, x0, y0, z0, w, d, h, nx, ny, arg, dist1 );
            if (dist1 < mind){
                mind = dist1;
            }
        }else if ( arg.caseEqual("center" ) ) {
            xsum = xsum + linestr1.point[i].x;
            ysum = ysum + linestr1.point[i].y;
            zsum = zsum + linestr1.point[i].z;
            counter++;
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else if (arg.caseEqual( "min" )) {
        dist = mind;
    } else if (arg.caseEqual( "center" )) {
        if ( 0 == counter ) dist = 0.0;
        else dist = JagGeo::distance( x0, y0, z0, xsum / counter, ysum / counter, zsum / counter, srid );
    }

    return true;
}

bool JagGeo::polygon3DDistanceCone(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist )
{
    JagPolygon pgon;
    int rc;
    //double d1, d2, d3, min, max;
    double xsum = 0, ysum = 0, zsum = 0;
    long counter = 0;
    double dist1;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    rc = JagParser::addPolygon3DData( pgon, sp1, true );
    if ( rc < 0 ) { return false; }

    const JagLineString3D &linestr1 = pgon.linestr[0];

    for ( int i=0; i < linestr1.size()-1; ++i ) {//first point = last point,so pass
        if ( arg.caseEqual( "max" ) ) {
            point3DDistanceCone( srid, linestr1.point[i].x, linestr1.point[i].y, linestr1.point[i].z, x0, y0, z0, r, h, nx, ny, arg, dist1 );
            if (dist1 > maxd){
                maxd = dist1;
         }
        }else if ( arg.caseEqual( "min" ) ) {
            point3DDistanceCone( srid, linestr1.point[i].x, linestr1.point[i].y, linestr1.point[i].z, x0, y0, z0, r, h, nx, ny, arg, dist1 );
            if (dist1 < mind){
                mind = dist1;
            }
        }else if ( arg.caseEqual("center" ) ) {
            xsum = xsum + linestr1.point[i].x;
            ysum = ysum + linestr1.point[i].y;
            zsum = zsum + linestr1.point[i].z;
            counter++;
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else if (arg.caseEqual( "min" )) {
        dist = mind;
    } else if (arg.caseEqual( "center" )) {
        if ( 0 == counter ) dist = 0.0;
        else dist = JagGeo::distance( x0, y0, z0, xsum / counter, ysum / counter, zsum / counter, srid );
    }

    return true;
}


// multipolygon -- 2D
bool JagGeo::multiPolygonDistanceTriangle(int srid, const Jstr &mk1, const JagStrSplit &sp1,
									 double x1, double y1, double x2, double y2, double x3, double y3,  const Jstr& arg, double &dist )
{
    int rc;
    JagVector<JagPolygon> pgvec;
    double d1, d2, d3, min, max;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;

    rc = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
    if ( rc <= 0 ) { return false; }

    int len = pgvec.size();
    d("s10008 len=[%d]\n", len);
    if ( len < 1 ) return false;

    if (arg.caseEqual( "center" )) {
         double cx,cy;
         center2DMultiPolygon(pgvec, cx, cy);
         dist = JagGeo::distance( cx, cy, (x1 + x2 + x3) / 3, (y1 + y2 +y3) / 3, srid );
         return true;
    }

    for ( int i=0; i < len; ++i ) {
    	const JagLineString3D &linestr = pgvec[i].linestr[0];
    	//pgvec[i].print();
    	for ( int j=0; j < linestr.size()-1; ++j ) {
                d1 = JagGeo::distance( linestr.point[j].x, linestr.point[j].y, x1, y1, srid );
                d2 = JagGeo::distance( linestr.point[j].x, linestr.point[j].y, x2, y2, srid );
                d3 = JagGeo::distance( linestr.point[j].x, linestr.point[j].y, x3, y3, srid );
                if ( arg.caseEqual( "max" )){
                    max = jagmax3(d1,d2,d3);
                    if ( max > maxd ) maxd = max;
                }else{
                    min = jagmin3(d1,d2,d3);
                    if ( min < mind ) mind = min;
                    }
                }

    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else  {
        dist = mind;
    }

    return true;
}

bool JagGeo::multiPolygonDistanceSquare(int srid, const Jstr &mk1, const JagStrSplit &sp1,
	                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
    multiPolygonDistanceRectangle(srid, mk1, sp1, x0, y0,r, r, nx, arg, dist);
    return true;
}
bool JagGeo::multiPolygonDistanceRectangle(int srid, const Jstr &mk1, const JagStrSplit &sp1,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
    int rc;
    JagVector<JagPolygon> pgvec;
    double dd;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;

    rc = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
    if ( rc <= 0 ) { return false; }

    if ( arg.caseEqual( "center" )) {
            double cx,cy;
            center2DMultiPolygon(pgvec, cx, cy);
            dist = JagGeo::distance( cx, cy, x0, y0, srid );
            return true;
    }

    int len = pgvec.size();
    d("s10009 len=[%d]\n", len);
    if ( len < 1 ) return false;

    for ( int i=0; i < len; ++i ) {
        for (int j = 0; j < pgvec[i].linestr.size(); ++j ){
            const JagLineString3D &linestr = pgvec[i].linestr[j];
             for ( int k=0; k < linestr.size()-1; ++k ) {
                 if ( arg.caseEqual( "max" )){
                      pointDistanceRectangle(srid,linestr.point[k].x, linestr.point[k].y, x0, y0 , a, b, nx, arg, dd);
                      if ( dd > maxd ) maxd = dd;
                 } else if (arg.caseEqual( "min" )){
                      pointDistanceRectangle(srid,linestr.point[k].x, linestr.point[k].y, x0, y0 , a, b, nx, arg, dd);
                      if ( dd < mind ) mind = dd;
                 }
             }

        }

    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else {
        dist = mind;
    }

    return true;
    }

bool JagGeo::multiPolygonDistanceEllipse(int srid, const Jstr &mk1, const JagStrSplit &sp1,
	                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist )
{
    int rc;
    JagVector<JagPolygon> pgvec;
    double dd;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;

    rc = JagParser::addMultiPolygonData( pgvec, sp1, true , false );
    if ( rc <= 0 ) { return false; }

    if (arg.caseEqual( "center" )) {
        double cx,cy;
        center2DMultiPolygon(pgvec, cx, cy);
        dist = JagGeo::distance( cx, cy, x0, y0, srid );
        return true;
    }

    int len = pgvec.size();
    d("s10010 len=[%d]\n", len);
    if ( len < 1 ) return true;
    for ( int i=0; i < len; ++i ) {
        const JagLineString3D &linestr = pgvec[i].linestr[0];
        //pgvec[i].print();
        for ( int j=0; j < linestr.size()-1; ++j ) {
            dd = JagGeo::distance( linestr.point[j].x, linestr.point[j].y, x0, y0, srid );
            if ( dd < mind ) mind = dd;
            if ( dd > maxd ) maxd = dd;
            d("s10010 d=[%f] mind=[%f] maxd=[%f]\n",  dd, mind, maxd);
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd+a;
    } else {
        dist = mind-a;
    }

    return true;
}
bool JagGeo::multiPolygonDistanceCircle(int srid, const Jstr &mk1, const JagStrSplit &sp1,
								    double x0, double y0, double r, double nx, const Jstr& arg, double &dist )
{
    int rc;
    JagVector<JagPolygon> pgvec;
    double dd;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;

    rc = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
    if ( rc <= 0 ) { return false; }

    if (arg.caseEqual( "center" )) {
        double cx,cy;
        center2DMultiPolygon(pgvec, cx, cy);
        dist = JagGeo::distance( cx, cy, x0, y0, srid );
        return true;
    }

    int len = pgvec.size();
    d("s10010 len=[%d]\n", len);
    if ( len < 1 ) return true;
    for ( int i=0; i < len; ++i ) {
        //pgvec[i].print();
        for ( int j=0; j < pgvec[i].linestr.size(); ++j ) {
            const JagLineString3D &linestr = pgvec[i].linestr[j];
            for (int k = 0; k <linestr.size()-1; ++k ){
                 dd = JagGeo::distance( linestr.point[k].x, linestr.point[k].y, x0, y0, srid );
                 if ( dd < mind ) mind = dd;
                 if ( dd > maxd ) maxd = dd;
                 d("s10010 d=[%f] mind=[%f] maxd=[%f]\n",  dd, mind, maxd);
            }
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd+r;
    } else  {
        dist = mind-r;
    }

    return true;
}

bool JagGeo::multiPolygonDistancePolygon(int srid, const Jstr &mk1, const JagStrSplit &sp1,
									const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist )
{
    int rc1, rc2;
    JagVector<JagPolygon> pgvec;
    JagPolygon pgon;
    double dd;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;

    rc1 = JagParser::addMultiPolygonData( pgvec, sp1, false , false );
	if ( rc1 <= 0 ) { return false; }
    rc2 = JagParser::addPolygonData( pgon, sp2, false );
	if ( rc2 <= 0 ) { return false; }

    if (arg.caseEqual( "center" )) {
        double cx,cy, cx2,cy2;
        center2DMultiPolygon( pgvec, cx, cy );
        pgon.center2D(cx2, cy2);
        dist = JagGeo::distance( cx, cy, cx2, cy2, srid );
		return true;
    }

    int len = pgvec.size();
    d("s10011 len=[%d]\n", len);
    if ( len < 1 ) return true;

    for ( int i=0; i < len; ++i ) {
        //pgvec[i].print();
        for ( int j=0; j < pgvec[i].linestr.size(); ++j ) {
            const JagLineString3D &linestr = pgvec[i].linestr[j];
            for (int k = 0; k <linestr.size()-1; ++k ){
                 for (int m = 0; m < pgon.linestr.size(); ++m){
                    const JagLineString3D &linestr2 = pgon.linestr[m];
                    for (int n = 0; n < linestr2.size()-1; ++n) {
                        dd = JagGeo::distance( linestr.point[k].x, linestr.point[k].y, linestr2.point[n].x, linestr2.point[n].y, srid );
                        d("%f %f %f %f\n", linestr.point[j].x, linestr.point[j].y, linestr2.point[k].x, linestr2.point[k].y);
                        if ( dd < mind ) mind = dd;
                        if ( dd > maxd ) maxd = dd;
                    }
                 }
            }
        }
    }


    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else  {
        dist = mind;
    }

    return true;
}

// multipolygon3d Distance
bool JagGeo::multiPolygon3DDistanceCube(int srid, const Jstr &mk1, const JagStrSplit &sp1,
								double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist )
{
    int rc;
    //double d;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    double dist1;
    JagVector<JagPolygon> pgvec;
    rc = JagParser::addMultiPolygonData( pgvec, sp1, false , true );
    if ( rc <=0 ) { return false; }

    int len = pgvec.size();
    if ( len < 1 ) return false;

    if (arg.caseEqual( "center" )) {
        double cx,cy,cz;
        center3DMultiPolygon( pgvec, cx, cy, cz );
        dist = JagGeo::distance( cx, cy, cz, x0, y0, z0, srid );
		return true;
    }

    for ( int i=0; i < len; ++i ) {
        const JagLineString3D &linestr1 = pgvec[i].linestr[0];
        for ( int j=0; j < linestr1.size()-1; ++j ) {
             if ( arg.caseEqual( "max" ) ) {
                point3DDistanceBox( srid, linestr1.point[j].x, linestr1.point[j].y, linestr1.point[j].z, x0, y0, z0, r, r, r, nx, ny, arg, dist1 );
                if (dist1 > maxd){
                    maxd = dist1;
                    }
             }else if ( arg.caseEqual( "min" ) ) {
                point3DDistanceBox( srid, linestr1.point[j].x, linestr1.point[j].y, linestr1.point[j].z, x0, y0, z0, r, r, r, nx, ny, arg, dist1 );
                if (dist1 < mind){
                    mind = dist1;
                }
             }
        }
    }
    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else if(arg.caseEqual( "min" )) {
        dist = mind;
    }
    return true;
}

bool JagGeo::multiPolygon3DDistanceBox(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
    //int rc;
    int rc1;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    double dist1;
    JagVector<JagPolygon> pgvec;
    rc1 = JagParser::addMultiPolygonData( pgvec, sp1, false , true );
	if ( rc1 <= 0 ) { return false; }

    int len = pgvec.size();
    if ( len < 1 ) return false;

    if (arg.caseEqual( "center" )) {
        double cx,cy,cz;
        center3DMultiPolygon( pgvec, cx, cy, cz );
        dist = JagGeo::distance( cx, cy, cz, x0, y0, z0, srid );
		return true;
    }

    for ( int i=0; i < len; ++i ) {
        const JagLineString3D &linestr1 = pgvec[i].linestr[0];
        for ( int j=0; j < linestr1.size()-1; ++j ) {
             if ( arg.caseEqual( "max" ) ) {
                point3DDistanceBox( srid, linestr1.point[j].x, linestr1.point[j].y, linestr1.point[j].z, x0, y0, z0, w, d, h, nx, ny, arg, dist1 );
                if (dist1 > maxd){
                    maxd = dist1;
                    }
             }else if ( arg.caseEqual( "min" ) ) {
                point3DDistanceBox( srid, linestr1.point[j].x, linestr1.point[j].y, linestr1.point[j].z, x0, y0, z0, w, d, h, nx, ny, arg, dist1 );
                if (dist1 < mind){
                    mind = dist1;
                }
             }
        }
    }
    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else if(arg.caseEqual( "min" )) {
        dist = mind;
    }
    return true;
}
bool JagGeo::multiPolygon3DDistanceSphere(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, const Jstr& arg, double &dist )
{
    //int rc;
    int rc1;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    double dist1;
    JagVector<JagPolygon> pgvec;
    rc1 = JagParser::addMultiPolygonData( pgvec, sp1, false , true );
	if ( rc1 <= 0 ) { return false; }

    int len = pgvec.size();
    if ( len < 1 ) return false;
    if (arg.caseEqual( "center" )) {
        double cx,cy,cz;
        center3DMultiPolygon( pgvec, cx, cy, cz );
        dist = JagGeo::distance( cx, cy, cz, x, y, z, srid );
		return true;
    }

    for ( int i=0; i < len; ++i ) {
        const JagLineString3D &linestr1 = pgvec[i].linestr[0];
        for ( int j=0; j < linestr1.size()-1; ++j ) {
             if ( arg.caseEqual( "max" ) ) {
                dist1 = JagGeo::distance( x, y, z, linestr1.point[j].x, linestr1.point[j].y, linestr1.point[j].z, srid );
                if (dist1 > maxd){
                    maxd = dist1;
                    }
             }else if ( arg.caseEqual( "min" ) ) {
                dist1 = JagGeo::distance( x, y, z, linestr1.point[j].x, linestr1.point[j].y, linestr1.point[j].z, srid );
                if (dist1 < mind){
                    mind = dist1;
                }
             }
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd + r;
    } else if(arg.caseEqual( "min" )) {
        dist = mind - r;
    }
    return true;
}
bool JagGeo::multiPolygon3DDistanceEllipsoid(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist )
{
    //int rc;
    int rc1;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    double dist1;
    JagVector<JagPolygon> pgvec;
    rc1 = JagParser::addMultiPolygonData( pgvec, sp1, false , true );
	if ( rc1 <= 0 ) { return false; }

    int len = pgvec.size();
    if ( len < 1 ) return true;
    if (arg.caseEqual( "center" )) {
        double cx,cy,cz;
        center3DMultiPolygon( pgvec, cx, cy, cz );
        dist = JagGeo::distance( cx, cy, cz, x0, y0, z0, srid );
		return true;
    }

    for ( int i=0; i < len; ++i ) {
        const JagLineString3D &linestr1 = pgvec[i].linestr[0];
        for ( int j=0; j < linestr1.size()-1; ++j ) {
             if ( arg.caseEqual( "max" ) ) {
                point3DDistanceEllipsoid( srid, linestr1.point[j].x, linestr1.point[j].y, linestr1.point[j].z,
										 x0, y0, z0, w,d,h, nx, ny, arg, dist1 );
                if (dist1 > maxd){ maxd = dist1; }
             }else if ( arg.caseEqual( "min" ) ) {
                point3DDistanceEllipsoid( srid, linestr1.point[j].x, linestr1.point[j].y, linestr1.point[j].z, 
										 x0, y0, z0, w,d,h, nx, ny, arg, dist1 );
                if (dist1 < mind){ mind = dist1; }
             }
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else if(arg.caseEqual( "min" )) {
        dist = mind;
    }
    return true;
}
bool JagGeo::multiPolygon3DDistanceCone(int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist )
{
    //int rc;
    int rc1;
    double mind = JAG_LONG_MAX;
    double maxd = JAG_LONG_MIN;
    double dist1;
    JagVector<JagPolygon> pgvec;
    rc1 = JagParser::addMultiPolygonData( pgvec, sp1, false , true );
	if ( rc1 <= 0 ) { return false; }

    int len = pgvec.size();
    if ( len < 1 ) return false;
    if (arg.caseEqual( "center" )) {
        double cx,cy,cz;
        center3DMultiPolygon( pgvec, cx, cy, cz );
        dist = JagGeo::distance( cx, cy, cz, x0, y0, z0, srid );
		return true;
    }
    for ( int i=0; i < len; ++i ) {
        const JagLineString3D &linestr1 = pgvec[i].linestr[0];
        for ( int j=0; j < linestr1.size()-1; ++j ) {
             if ( arg.caseEqual( "max" ) ) {
                point3DDistanceCone( srid, linestr1.point[j].x, linestr1.point[j].y, linestr1.point[j].z,
											 x0, y0, z0, r, h, nx, ny, arg, dist1 );
                if (dist1 > maxd){ maxd = dist1; }
             }else if ( arg.caseEqual( "min" ) ) {
                point3DDistanceCone( srid, linestr1.point[j].x, linestr1.point[j].y, linestr1.point[j].z, 
											 x0, y0, z0, r, h, nx, ny, arg, dist1 );
                if (dist1 < mind){ mind = dist1; }
             }
        }
    }

    if ( arg.caseEqual( "max" ) ) {
        dist = maxd;
    } else if(arg.caseEqual( "min" )) {
        dist = mind;
    }
    return true;
}

bool JagGeo::point3DDistanceNormalCone(int srid, double px, double py, double pz, 
									 double r, double h, const Jstr& arg, double &dist )
{
	if ( arg.caseEqual( "center" ) ) {
		dist = distance( px, py, pz, 0.0, 0.0, 0.0, srid );
		return true;
	}

	// z-p plane (p=sqrt(x^2+y^2))
	double  pr2 = px*px + py*py;
	double  pr = sqrt( pr2 );
	if ( arg.caseEqual( "min" ) ) {
		if ( pz >= h ) {
			dist = distance(px, py, pz, 0.0, 0.0, h, srid );
		} else if ( pz <= -h ) {
			double  dx = pr - 2.0*r;
			double  dy = pz + h;
			dist = sqrt( dx*dx + dy*dy );
		} else {
			double d = r*( 1.0 - pz/h );
			dist = fabs(d - pr) * h*h/(h*h+r*r);
		}
	} else if ( arg.caseEqual( "max" ) ) {
		d("s4401 pz=%f  h=%f r=%f srid=%d\n", pz, h, r, srid );
		if ( pz >= h ) {
			double  pr2 = px*px + py*py;
			double  pr = sqrt( pr2 );
			dist = distance(0.0, pr, pz,  0.0, -2.0*r, -h, srid );
		} else if ( pz <= -h ) {
			double  dx = pr + 2.0*r;
			double  dy = pz + h;
			dist = sqrt( dx*dx + dy*dy );
		} else {
			double d = r*( 1.0 - pz/h );
			dist = fabs(d + pr) * h*h/(h*h+r*r);
		}
	}
    return true;
}

// geo distance
double JagGeo::pointToLineGeoDistance( double lata1, double lona1, double lata2, double lona2, double latb1, double lonb1 )
{
	double lat0 = (lata1 + lata2 )/2.0;
	double lon0 = (lona1 + lona2 )/2.0;

	const Geodesic& geod = Geodesic::WGS84();
  	const GeographicLib::Gnomonic gn(geod);

	double xa1, ya1, xa2, ya2;
	double xb1, yb1;
	double lat1, lon1;
	for (int i = 0; i < 10; ++i) {
		gn.Forward(lat0, lon0, lata1, lona1, xa1, ya1);
	    gn.Forward(lat0, lon0, lata2, lona2, xa2, ya2);
	    gn.Forward(lat0, lon0, latb1, lonb1, xb1, yb1);
	    jagvector3 va1(xa1, ya1); 
		jagvector3 va2(xa2, ya2);
	    jagvector3 la = va1.cross(va2);
	    jagvector3 vb1(xb1, yb1);
	    jagvector3 lb(la._y, -la._x, la._x * yb1 - la._y * xb1);
	    jagvector3 p0 = la.cross(lb);
	    p0.norm();
	    gn.Reverse(lat0, lon0, p0._x, p0._y, lat1, lon1);
	    lat0 = lat1;
	    lon0 = lon1;
    }

	//d("s2938 close point lat0=%.3f lon0=%.3f\n", lat0, lon0 );
	geod.Inverse( latb1, lonb1, lat0, lon0, xa1 );  // use xa1
	return xa1;
}

double JagGeo::meterToLon( int srid, double meter, double lon, double lat)
{
	if ( 0 == srid ) return meter;  // geometric unitless length

	const Geodesic& geod = Geodesic::WGS84();
	double lon2 = lon + 1.0;
	double s12;
	geod.Inverse( lat, lon, lat, lon2, s12 );
	return meter/s12;
}

double JagGeo::meterToLat( int srid, double meter, double lon, double lat)
{
	if ( 0 == srid ) return meter;  // geometric unitless length

	const Geodesic& geod = Geodesic::WGS84();
	double lat2 = lat + 1.0;
	double s12;
	geod.Inverse( lat, lon, lat2, lon, s12 );
	return meter/s12;
}

bool JagGeo::lineStringAverage( const Jstr &mk, const JagStrSplit &sp, double &x, double &y )
{
	int start = JAG_SP_START;

	double dx, dy;
    double xsum = 0.0, ysum = 0.0;
	long  counter = 0;
	const char *str;
    char *p;
	for (int i = start; i<sp.length(); ++i){
		str = sp[i].c_str();
		if(strchrnum(str, ':') < 1) continue;
		get2double(str,p,':',dx,dy);
        xsum = xsum + dx;
        ysum = ysum + dy;
        counter ++;
	}

    if ( 0 == counter ) { 
		x = y = 0.0;
		return false; 
	} else {
		x = xsum/(double)counter;
		y = ysum/(double)counter;
	}

	return true;
}


bool JagGeo::lineString3DAverage( const Jstr &mk, const JagStrSplit &sp, double &x, double &y, double &z )
{
	int start = JAG_SP_START;

	double dx, dy, dz;
    double xsum = 0.0, ysum = 0.0, zsum = 0.0;
	long  counter = 0;
	const char *str;
    char *p;
	for (int i = start; i<sp.length(); ++i){
		str = sp[i].c_str();
		if(strchrnum(str, ':') < 2) continue;
		get3double(str,p,':', dx,dy,dz);
        xsum = xsum + dx;
        ysum = ysum + dy;
        zsum = zsum + dz;
        counter ++;
	}

    if ( 0 == counter ) { 
		x = y = z = 0.0;
		return false; 
	} else {
		x = xsum/(double)counter;
		y = ysum/(double)counter;
		z = zsum/(double)counter;
	}

	return true;
}

void JagGeo::multiPolygonToWKT( const JagVector<JagPolygon> &pgvec, bool is3D, Jstr &str )
{
	if ( pgvec.size() < 1 ) { str=""; return; }
	str = "multipolygon(";

	for ( int k=0; k < pgvec.size(); ++k ) {
    	if ( k==0 ) { str += "("; } else { str += ",("; }
		const JagPolygon &pgon = pgvec[k];
    	for ( int i=0; i < pgon.linestr.size(); ++i ) {
    		if ( i==0 ) { str += "("; } else { str += ",("; }
    		const JagLineString3D &lstr = pgon.linestr[i];
    		for (  int j=0; j< lstr.size(); ++j ) {
    			if ( j>0) { str += Jstr(","); }
    			str += d2s(lstr.point[j].x) + " " +  d2s(lstr.point[j].y);
    			if ( is3D ) { str += Jstr(" ") + d2s(lstr.point[j].z); }
    		}
    		str += ")";
    	}
    	
    	str += ")";
	}
   	str += ")";
}



void JagGeo::center2DMultiPolygon( const JagVector<JagPolygon> &pgvec, double &cx, double &cy )
{
	cx = cy = 0.0;
	int len = pgvec.size();
	if ( len < 1 ) return;
	double x, y;
	for ( int i=0; i < len; ++i ) {
		pgvec[i].center2D(x, y);
		cx += x;
		cy += y;
	}

	cx = cx / len;
	cy = cy / len;
}

void JagGeo::center3DMultiPolygon( const JagVector<JagPolygon> &pgvec, double &cx, double &cy, double &cz )
{
	cx = cy = cz = 0.0;
	int len = pgvec.size();
	if ( len < 1 ) return;
	double x, y, z;
	for ( int i=0; i < len; ++i ) {
		pgvec[i].center3D(x, y, z);
		cx += x;
		cy += y;
		cz += z;
	}

	cx = cx / len;
	cy = cy / len;
	cz = cz / len;
}


// ax^4 + bx^3 + cx^2 + dx + e = 0   a==1.0
// https://en.wikipedia.org/wiki/Quartic_function
void JagGeo::fourthOrderEquation( double b, double c, double d, double e, int &num, double *root )
{
	//d("s1102 fourthOrderEquation b=%f c=%f d=%f e=%f\n", b, c, d, e );
	/***
	double DELTA = 256.0*e*e*e - 192.0*b*d*e*e - 128.0*c*c*e*e + 144.0* c*d*d*e - 27.0 * d*d*d*d 
                   + 144.0* b*b*c*e*e - 6.0*b*b*d*d*e -80.0*b*c*c*d*e + 18.0*b*c*d*d*d + 16.0*c*c*c*c*e
                   - 4.0*c*c*c*d*d - 27.0 * b*b*b*b*e*e + 18.0*b*b*b*c*d*e - 4.0*b*b*b*d*d*d - 4.0*b*b*c*c*c*e + b*b*c*c*d*d;
	double P = 8.0*c - 3.0*b*b*b;
 	double R = b*b*b + 8.0*d - 4.0*b*c;
	double D = 64.0* e - 16*c*c + 16.0*b*b*c - 16.0* b*d - 3.0*b*b*b*b;
	***/
	num = 0;
	double DELTA0 = c*c - 3.0*b*d + 12.0*e;
	double DELTA1 = 2.0* c*c - 9.0*b*c*d + 27.0*(b*b*e + d*d) - 72.0*c*e;
	double DD = DELTA1*DELTA1 - 4.0*DELTA0*DELTA0*DELTA0;
	//d("s48821 DELTA0=%f DELTA1=%f DD=%f\n", DELTA0, DELTA1, DD );
	if ( DD < 0.00000001 ) {
		return;
	}

	double p = (8.0*c-3.0*b*b)/8.0;
	double q = (b*b*b - 4.0*b*c + 8.0*d)/8.0;
	double Q = cbrt( 0.5*(DELTA1 + sqrt(DD) ) );
	//d("s3934 Q=%f\n", Q );
	double s = -2.0*p/3.0 + (Q + DELTA0/Q)/3.0;
	if ( s < 0.0000001 ) { 
		//d("s3993 s=%f\n", s );
		return;
	}
	//d("s3994 s=%f\n", s );
	double S = 0.5*sqrt(s);

	double f = -4.0*S*S - 2.0*p;
	double f1 = f + q/S;
	double f2 = f - q/S;
	//d("s4088 f1=%f f2=%f S=%f p=%f q=%f Q=%f\n", f1, f2, S, p, q, Q );
	if ( f1 > 0.0 || f1 >= 0.00001 ) {
		num += 2;
		root[0] = -b/4.0 -S + 0.5*sqrt(f1);
		root[1] = -b/4.0 -S - 0.5*sqrt(f1);
	}

	if ( f2 > 0.0 || f2 >= 0.00001 ) {
		num += 2;
		root[2] = -b/4.0 +S + 0.5*sqrt(f2);
		root[3] = -b/4.0 +S - 0.5*sqrt(f2);
	}
}

// u v is external point, x & y is point on ellipse
// dist is min dist
void JagGeo::minMaxPointOnNormalEllipse( int srid, double a, double b, double u, double v, bool isMin, double &x, double &y, double &dist )
{
	// x = a*cos(THETA)
	// y = b*sin(THETA)
	double left = 0.0;
	double right = 2.0*JAG_PI-0.001;
	double mid, dL, dR, d1, d2;
	double x1, x2, y1, y2;
	double prevDist = JAG_LONG_MIN;
	while ( true ) {
		mid = (left+right)/2.0;
		dL = mid - 0.1;
		dR = mid + 0.1;
		x1 = a* cos(dL);
		y1 = b* sin(dL);
		x2 = a* cos(dR);
		y2 = b* sin(dR);
		d1 = (u-x1)*(u-x1) + (v-y1)*(v-y1);
		d2 = (u-x2)*(u-x2) + (v-y2)*(v-y2);
		dist = (d1+d2)/2.0;
		x = ( x1+x2)/2.0;
		y = ( y1+y2)/2.0;
		if ( fabs((dist-prevDist)/prevDist) < 0.01 ) {
			break;
		}

		if ( isMin ) {
			if ( d1 < d2 ) {
				right = mid;
			} else {
				left = mid;
			}
		} else {
			if ( d1 > d2 ) {
				right = mid;
			} else {
				left = mid;
			}
		}

		prevDist = dist;
	}
	
	dist = sqrt(dist);
}

void JagGeo::minMaxPoint3DOnNormalEllipsoid( int srid, double a, double b, double c,
                                             double u, double v, double w, bool isMin,
                                             double &x, double &y, double &z, double &dist )
{
	// x = a*cos(THETA) * cos( PHI)
	// y = b*cos(THETA) * sin (PHI)
	// z = c * sin(THETA)
	// THETA: [-0.5*PI, 0.5*PI]   PHI: [-PI, PI)
	double down = -0.5*JAG_PI;  // THETA
	double up = 0.5*JAG_PI -0.001;
	double left = 0.0;
	double right = 2.0*JAG_PI-0.001;
	double midx, midy, Lx, Rx, Ly, Ry, d1, d2, d3, d4;
	double x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
	double prevDist = JAG_LONG_MIN;
	while ( true ) {
		midx = (left+right)/2.0;  // PHI
		midy = (down+up)/2.0;  // THETA
		Lx = midx - 0.1;
		Rx = midx + 0.1;
		Ly = midy - 0.1;
		Ry = midy + 0.1;

		x1 = a* cos(Ly) * cos(Lx);
		y1 = b* cos(Ly) * sin(Lx);
		z1 = c* sin(Ly);
		d1 = (u-x1)*(u-x1) + (v-y1)*(v-y1) + (w-z1)*(w-z1);

		x2 = a* cos(Ry) * cos(Lx);
		y2 = b* cos(Ry) * sin(Lx);
		z2 = c* sin(Ry);
		d2 = (u-x2)*(u-x2) + (v-y2)*(v-y2) + (w-z2)*(w-z2);

		x3 = a* cos(Ly) * cos(Rx);
		y3 = b* cos(Ly) * sin(Rx);
		z3 = c* sin(Ly);
		d3 = (u-x3)*(u-x3) + (v-y3)*(v-y3) + (w-z3)*(w-z3);

		x4 = a* cos(Ry) * cos(Rx);
		y4 = b* cos(Ry) * sin(Rx);
		z4 = c* sin(Ry);
		d4 = (u-x4)*(u-x4) + (v-y4)*(v-y4) + (w-z4)*(w-z4);

		dist = (d1+d2+d3+d4)/4.0;
		x = ( x1+x2+x3+x4)/4.0;
		y = ( y1+y2+y3+y4)/4.0;
		z = ( z1+z2+z3+z4)/4.0;
		if ( fabs((dist-prevDist)/prevDist) < 0.01 ) {
			break;
		}

		if ( isMin ) {
			findMinBoundary(d1, d2, d3, d4, midx, midy, left, right, up, down );
		} else {
			findMaxBoundary(d1, d2, d3, d4, midx, midy, left, right, up, down );
		}

		prevDist = dist;
	}
	
	dist = sqrt(dist);
}

/***
   left   ------              right     up
        d2                 d4           ^
              midx/midy                 |
        d1                 d3           v
   left                       right    down
***/

void JagGeo::findMinBoundary( double d1,  double d2,  double d3,  double d4, double midx, double midy, 
							  double &left, double &right, double &up, double &down )
{
	if ( d1 < d2 && d1 < d3 && d1 < d4 ) {
		right = midx;
		up = midy;
		return;
	}

	if ( d2 < d1 && d2 < d3 && d2 < d4 ) {
		right = midx;
		down = midy;
		return;
	}

	if ( d3 < d1 && d3 < d2 && d3 < d4 ) {
		left = midx;
		up = midy;
		return;
	}

	left = midx;
	down = midy;
}

void JagGeo::findMaxBoundary( double d1,  double d2,  double d3,  double d4, double midx, double midy, 
							  double &left, double &right, double &up, double &down )
{
	if ( d1 > d2 && d1 > d3 && d1 > d4 ) {
		right = midx;
		up = midy;
		return;
	}

	if ( d2 > d1 && d2 > d3 && d2 > d4 ) {
		right = midx;
		down = midy;
		return;
	}

	if ( d3 > d1 && d3 > d2 && d3 > d4 ) {
		left = midx;
		up = midy;
		return;
	}

	left = midx;
	down = midy;
}

double JagGeo::minPoint2DToLineSegDistance( double px, double py, double x1, double y1, double x2, double y2, int srid,
											double &projx, double &projy )
{
	double d2 = (x1-x2)*(x1-x2)+ (y1-y2)*(y1-y2);
	if ( d2 < 0.0001 ) {
		projx = x1; projy = y1;
		return distance( px,py, x1, y2, srid );
	}

	double t = ::dotProduct( px-x1,py-y1, x2-x1,y2-y1 );
	t = jagmax( 0, jagmin(1, t/d2) );
	projx = x1 + t*(x2-x1);
	projy = y1 + t*(y2-y1);
	return distance(px,py, projx, projy, srid );
}

double JagGeo::minPoint3DToLineSegDistance( double px, double py, double pz, 
											double x1, double y1, double z1, double x2, double y2, double z2, int srid,
											double &projx, double &projy, double &projz ) 
{
    double d2 = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2);
    if ( d2 < 0.0001 ) {
		projx = x1; projy = y1; projz = z1;
    	return distance( px, py, pz, x1, y1, z1, srid);
    }

    double t = (px - x1) * (x2 - x1) + (py - y1) * (y2 - y1) + (pz - z1) * (z2 - z1);
	t = jagmax( 0, jagmin(1, t/d2) );
	projx = x1 + t*(x2 - x1);
	projy = y1 + t*(y2 - y1);
	projz = z1 + t*(z2 - z1); 
    return distance( px, py, pz, projx, projy, projz, srid );
}


bool JagGeo::closestPoint2DPolygon( int srid, double px, double py, const Jstr &mk,
                                    const JagStrSplit &sp, Jstr &res )
{
	JagPolygon pgon;
	bool rc;
    rc = JagParser::addPolygonData( pgon, sp, false );
	if ( rc <= 0 ) return false;

	double projx, projy;
	double d, mind = JAG_LONG_MAX;
	bool found = false;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &lstr = pgon.linestr[i];
		for ( int j=0; j < lstr.point.size(); ++j ) {
			d = distance( px, py, lstr.point[j].x, lstr.point[j].y, srid );
			if ( d < mind ) {
				mind = d;
				projx = lstr.point[j].x;
				projy = lstr.point[j].y;
				found = true;
			}
		}
	}

	if ( ! found ) return false;
	res = d2s( projx ) + " " + d2s( projy );
	return true;
}

bool JagGeo::closestPoint2DRaster( int srid, double px, double py, const Jstr &mk,
                                    const JagStrSplit &sp, Jstr &res )
{
	//bool rc;

	double x, y, projx, projy;
	double d, mind = JAG_LONG_MAX;
	bool found = false;
	const char *str; char *p;
	for ( int i=JAG_SP_START; i < sp.size(); ++i ) {
		if ( sp[i] == "|" || sp[i] == "!" ) continue;
		str = sp[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', x, y );
		d = distance( px, py, x, y, srid );
		if ( d < mind ) {
			mind = d;
			projx = x;
			projy = y;
			found = true;
		}
	}

	if ( ! found ) return false;
	res = d2s( projx ) + " " + d2s( projy );
	return true;
}

bool JagGeo::closestPoint3DPolygon( int srid, double px, double pz, double py, const Jstr &mk,
                                    const JagStrSplit &sp, Jstr &res )
{
	JagPolygon pgon;
	bool rc;
    rc = JagParser::addPolygon3DData( pgon, sp, false );
	if ( rc < 0 ) return false;

	double projx, projy, projz;
	double d, mind = JAG_LONG_MAX;
	bool found = false;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &lstr = pgon.linestr[i];
		for ( int j=0; j < lstr.point.size(); ++j ) {
			d = distance( px, py, pz, lstr.point[j].x, lstr.point[j].y, lstr.point[j].z, srid );
			if ( d < mind ) {
				mind = d;
				projx = lstr.point[j].x;
				projy = lstr.point[j].y;
				projz = lstr.point[j].z;
				found = true;
			}
		}
	}

	if ( ! found ) return false;
	res = d2s( projx ) + " " + d2s( projy ) + " " + d2s( projz );
	return true;
}

bool JagGeo::closestPoint3DRaster( int srid, double px, double py, double pz, const Jstr &mk,
                                    const JagStrSplit &sp, Jstr &res )
{
	//bool rc;

	double x, y, z, projx, projy, projz;
	double d, mind = JAG_LONG_MAX;
	bool found = false;
	const char *str; char *p;
	for ( int i=JAG_SP_START; i < sp.size(); ++i ) {
		if ( sp[i] == "|" || sp[i] == "!" ) continue;
		str = sp[i].c_str();
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', x, y, z );
		d = distance( px, py, pz, x, y, z, srid );
		if ( d < mind ) {
			mind = d;
			projx = x;
			projy = y;
			projz = z;
			found = true;
		}
	}

	if ( ! found ) return false;
	res = d2s( projx ) + " " + d2s( projy ) + " " + d2s( projz );
	return true;
}

bool JagGeo::closestPoint2DMultiPolygon( int srid, double px, double py, const Jstr &mk,
                                    const JagStrSplit &sp, Jstr &res )
{
	JagVector<JagPolygon> pgvec;
	bool rc;
	/**
	if ( mk == JAG_OJAG ) {
        rc = JagParser::addMultiPolygonData( pgvec, sp, false, false );
    } else {
        const char *p = secondTokenStart( sp.c_str() );
        rc = JagParser::addMultiPolygonData( pgvec, p, false, false, false );
    }
	***/

    rc = JagParser::addMultiPolygonData( pgvec, sp, false, false );
    if ( rc <= 0 ) {
        return false;
    }

	double projx, projy;
	double d, mind = JAG_LONG_MAX;
	bool found = false;
	for ( int i=0; i < pgvec.size(); ++i ) {
		const JagPolygon &pgon = pgvec[i];
		for ( int j = 0; j < pgon.size(); ++j ) {
			const JagLineString3D &lstr = pgon.linestr[j];
    		for ( int k=0; k < lstr.point.size(); ++k ) {
    			d = distance( px, py, lstr.point[k].x, lstr.point[k].y, srid );
    			if ( d < mind ) {
    				mind = d;
    				projx = lstr.point[k].x;
    				projy = lstr.point[k].y;
    				found = true;
    			}
    		}
		}
	}

	if ( ! found ) return false;
	res = d2s( projx ) + " " + d2s( projy );
	return true;
}

bool JagGeo::closestPoint3DMultiPolygon( int srid, double px, double py, double pz, const Jstr &mk,
                                    const JagStrSplit &sp, Jstr &res )
{
	JagVector<JagPolygon> pgvec;
	bool rc;
	/***
	if ( mk == JAG_OJAG ) {
        rc = JagParser::addMultiPolygonData( pgvec, sp, false, true );
    } else {
        const char *p = secondTokenStart( sp.c_str() );
        rc = JagParser::addMultiPolygonData( pgvec, p, false, false, true );
    }
	***/
    rc = JagParser::addMultiPolygonData( pgvec, sp, false, true );
    if ( rc <= 0 ) {
        return false;
    }

	double projx, projy, projz;
	double d, mind = JAG_LONG_MAX;
	bool found = false;
	for ( int i=0; i < pgvec.size(); ++i ) {
		const JagPolygon &pgon = pgvec[i];
		for ( int j = 0; j < pgon.size(); ++j ) {
			const JagLineString3D &lstr = pgon.linestr[j];
    		for ( int k=0; k < lstr.point.size(); ++k ) {
    			d = distance( px, py, pz, lstr.point[k].x, lstr.point[k].y, lstr.point[k].z, srid );
    			if ( d < mind ) {
    				mind = d;
    				projx = lstr.point[k].x;
    				projy = lstr.point[k].y;
    				projz = lstr.point[k].z;
    				found = true;
    			}
    		}
		}
	}

	if ( ! found ) return false;
	res = d2s( projx ) + " " + d2s( projy ) + " " + d2s( projz );
	return true;
}

bool JagGeo::closestPoint3DBox( int srid, double px, double py, double pz, double x0, double y0, double z0, 
									double a, double b, double c, double nx, double ny, double &dist, Jstr &res )
{
	double locx, locy, locz;
	transform3DCoordGlobal2Local( x0, y0, z0, px, py, pz, nx, ny, locx, locy, locz );
	double dx = fabs(locx) - a;
	double absdx = fabs(dx);
	double dy = fabs(locy) - b;
	double absdy = fabs(dy);
	double dz = fabs(locz) - c;
	double absdz = fabs(dz);
	double mx, my, mz;

    if  ( jagLE(dx, 0.0) ) {
         if ( jagLE( dy, 0.0 ) ) {
			if ( jagLE( dz, 0.0) ) {
				// inside box
				dist = jagmin3( fabs(dx), fabs(dy), fabs(dz) );
    			if ( absdx <  absdy && absdx < absdz ) {
    				if ( jagLE(locx, 0.0) ) mx = -a; else mx = a;
    				my = locy; mz = locz;
    			} else if ( absdy <  absdx && absdy < absdz ) {
    				if ( jagLE(locy, 0.0) ) my = -b; else my = b;
    				mz = locz; mx = locx;
    			} else {
    				if ( jagLE(locz, 0.0) ) mz = -c; else mz = c;
    				mx = locx; my = locy;
    			}
			} else {
				dist = dz;
    			mx = locx; my = locy;
    			if ( jagLE(locz, 0.0) ) mz = -c; else mz = c;
			}
         } else {
		 	// locy outside of box
            if ( jagLE(dz, 0.0) ) {
				dist = absdy;
				if ( locy < 0.0 ) { my = -b; } else { my = b; }
				mx = locx; mz = locz;
			} else {
				// locz outside of box
				dist =  sqrt( dy*dy + dz*dz );
				mx = locx; 
				if ( locy < 0.0 ) { my = -b; } else { my = b; }
				if ( locz < 0.0 ) { mz = -c; } else { mz = c; }
			}
         }
    } else {
		// locx is outside of box
         if ( jagLE(dy, 0.0) ) {
		 	// locy is inside
            if ( jagLE(dz, 0.0) ) {
			   dist = absdx;
			   my = locy; mz = locz;
			   if ( locx < 0.0 ) { mx = -a; } else { mx = a; }
            } else {
				// locz outside
				dist =  sqrt( dx*dx + dz*dz );
				my = locy;
				if ( locx < 0.0 ) { mx = -a; } else { mx = a; }
				if ( locz < 0.0 ) { mz = -c; } else { mz = c; }
			}
         } else {
              if ( jagLE(dz, 0.0 ) ) {
				  dist =  sqrt( dx*dx + dy*dy );
				  mz = locz;
				  if ( locx < 0.0 ) { mx = -a; } else { mx = a; }
				  if ( locy < 0.0 ) { my = -b; } else { my = b; }
              } else {
				  dist =  sqrt( dx*dx + dy*dy + dz*dz );
				  if ( locx < 0.0 ) { mx = -a; } else { mx = a; }
				  if ( locy < 0.0 ) { my = -b; } else { my = b; }
				  if ( locz < 0.0 ) { mz = -c; } else { mz = c; }
			  }
         }
    }


	// transform3DCoordLocal2Global( px0, py0, pz0, vec[i].x, vec[i].y, vec[i].z, nx0, ny0, sq_x, sq_y, sq_z );
	transform3DCoordLocal2Global( x0, y0, z0, mx, my, mz, nx, ny, dx, dy, dz );
	res = d2s(dx) + " " + d2s(dy) + " " + d2s(dz); 
	return true;
}

bool JagGeo::getBBox2D( const JagVector<JagPolygon> &pgvec, double &xmin, double &ymin, double &xmax, double &ymax )
{
	if ( pgvec.size() < 1 ) return false;

	xmin = JAG_LONG_MAX; ymin = JAG_LONG_MAX;
	xmax = JAG_LONG_MIN; ymax = JAG_LONG_MIN;
	double xmi, ymi, xma, yma;
	int cnt = 0;
	for ( int i = 0; i < pgvec.size(); ++i ) {
		if ( pgvec[i].linestr.size() <1 ) continue;
		// using outer ring only works well enough
		pgvec[i].linestr[0].bbox2D( xmi, ymi, xma, yma );
		if ( xmi < xmin ) xmin = xmi;
		if ( ymi < ymin ) ymin = ymi;
		if ( xma > xmax ) xmax = xma;
		if ( yma > ymax ) ymax = yma;
		++cnt;
	}

	return ( cnt > 0 );
}

bool JagGeo::getBBox2DInner( const JagVector<JagPolygon> &pgvec, double &xmin, double &ymin, double &xmax, double &ymax )
{
	if ( pgvec.size() < 1 ) return false;

	xmin = JAG_LONG_MAX; ymin = JAG_LONG_MAX;
	xmax = JAG_LONG_MIN; ymax = JAG_LONG_MIN;
	double xmi, ymi, xma, yma;
	int cnt = 0;
	for ( int i = 0; i < pgvec.size(); ++i ) {
		if ( pgvec[i].linestr.size() <1 ) continue;
		// using outer ring only works well enough
		for ( int j=1; j < pgvec[i].linestr.size(); ++j ) {
			pgvec[i].linestr[j].bbox2D( xmi, ymi, xma, yma );
			if ( xmi < xmin ) xmin = xmi;
			if ( ymi < ymin ) ymin = ymi;
			if ( xma > xmax ) xmax = xma;
			if ( yma > ymax ) ymax = yma;
			++cnt;
		}
	}

	return ( cnt > 0 );
}



bool JagGeo::getBBox3D( const JagVector<JagPolygon> &pgvec, double &xmin, double &ymin, double &zmin, 
						double &xmax, double &ymax, double &zmax )
{
	if ( pgvec.size() < 1 ) return false;

	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	double xmi, ymi, zmi, xma, yma, zma;
	int cnt = 0;
	for ( int i = 0; i < pgvec.size(); ++i ) {
		if ( pgvec[i].linestr.size() <1 ) continue;
		// using outer ring only works well enough
		pgvec[i].linestr[0].bbox3D( xmi, ymi, zmi, xma, yma, zma );
		if ( xmi < xmin ) xmin = xmi;
		if ( ymi < ymin ) ymin = ymi;
		if ( zmi < zmin ) zmin = zmi;

		if ( xma > xmax ) xmax = xma;
		if ( yma > ymax ) ymax = yma;
		if ( zma > zmax ) zmax = zma;

		++cnt;
	}

	return ( cnt > 0 );
}

bool JagGeo::getBBox3DInner( const JagVector<JagPolygon> &pgvec, double &xmin, double &ymin, double &zmin, 
						double &xmax, double &ymax, double &zmax )
{
	if ( pgvec.size() < 1 ) return false;

	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	double xmi, ymi, zmi, xma, yma, zma;
	int cnt = 0;
	for ( int i = 0; i < pgvec.size(); ++i ) {
		if ( pgvec[i].linestr.size() <1 ) continue;
		// using outer ring only works well enough
		for ( int j=1; j < pgvec[i].linestr.size(); ++j ) {
    		pgvec[i].linestr[j].bbox3D( xmi, ymi, zmi, xma, yma, zma );
    		if ( xmi < xmin ) xmin = xmi;
    		if ( ymi < ymin ) ymin = ymi;
    		if ( zmi < zmin ) zmin = zmi;
    
    		if ( xma > xmax ) xmax = xma;
    		if ( yma > ymax ) ymax = yma;
    		if ( zma > zmax ) zmax = zma;
    
    		++cnt;
		}
	}

	return ( cnt > 0 );
}

#if 0
// return n>0: OK ,  <=0 error
// instr: "point3d(...)"  "polygon((...),(...))"
// outstr: "CJAG=0=0=type=subtype  bbox data1 data2 data3 ..."
int JagGeo::JagParser::convertConstantObjToJAG( const JagFixString &instr, Jstr &outstr )
{
	int cnt = 0;
	Jstr othertype;
	int rc = 0;
	char *p = (char*)instr.c_str();
	if ( ! p || *p == '\0' ) return 0;
	if ( strncasecmp( p, "point(", 6 ) == 0 || strncasecmp( p, "point3d(", 8 ) == 0 ) {
		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -10;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -381; } }
		if ( sp.length() == 3 ) {
			outstr = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0 ") + sp[0] + " " + sp[1] + " " + sp[2];
		} else if ( sp.length() == 2 ) {
			outstr = Jstr("CJAG=0=0=PT=d 0:0:0:0 ") + sp[0] + " " + sp[1];
		} else {
			outstr = "";
			return -9;
		}
		++cnt;
	} else if ( strncasecmp( p, "circle(", 7 ) == 0 ) {
		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -20;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() < 3 ) { return -30; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -382; } }
		outstr = Jstr("CJAG=0=0=CR=d 0:0:0:0 ") + sp[0] + " " + sp[1] + " " + sp[2];
		++cnt;
	} else if ( strncasecmp( p, "square(", 7 )==0 ) {
		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -20;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() < 3 ) { return -30; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -383; } }
		if ( sp.length() <= 3 ) {
			outstr = Jstr("CJAG=0=0=SQ=d 0:0:0:0 ") + sp[0] + " " + sp[1] + " " + sp[2] + " 0.0";
		} else {
			outstr = Jstr("CJAG=0=0=SQ=d 0:0:0:0 ") + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3];
		}
		++cnt;
	} else if (  strncasecmp( p, "cube(", 5 )==0 || strncasecmp( p, "sphere(", 7 )==0 ) {
		// cube( x y z radius )   inner circle radius
		if ( strncasecmp( p, "cube", 4 )==0 ) {
			othertype =  JAG_C_COL_TYPE_CUBE;
		} else {
			othertype =  JAG_C_COL_TYPE_SPHERE;
		}

		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -30;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() < 4 ) { return -32; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -384; } }
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 " + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3] + " ";
		++cnt;
		if ( othertype == JAG_C_COL_TYPE_CUBE ) {
			Jstr nx, ny;
			if ( sp.length() >= 5 ) { nx = sp[4]; } else { nx="0.0"; }
			if ( sp.length() >= 6 ) { ny = sp[5]; } else { ny="0.0"; }
			outstr += nx + " " + ny;
		}
	} else if ( strncasecmp( p, "circle3d(", 9 )==0
				|| strncasecmp( p, "square3d(", 9 )==0 ) {
		// circle3d( x y z radius nx ny )   inner circle radius
		if ( strncasecmp( p, "circ", 4 )==0 ) {
			othertype =  JAG_C_COL_TYPE_CIRCLE3D;
		} else {
			othertype =  JAG_C_COL_TYPE_SQUARE3D;
		} 

		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -40;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() < 4 ) { return -42; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -385; } }
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 " + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3] + " ";
		Jstr nx, ny;
		if ( sp.length() >= 5 ) { nx = sp[4]; } else { nx="0.0"; }
		if ( sp.length() >= 6 ) { ny = sp[5]; } else { ny="0.0"; }
		outstr += nx + " " + ny;
		++cnt;
	} else if (  strncasecmp( p, "rectangle(", 10 )==0 || strncasecmp( p, "ellipse(", 8 )==0 ) {
		// rectangle( x y width height nx ny) 
		if ( strncasecmp( p, "rect", 4 )==0 ) {
			othertype =  JAG_C_COL_TYPE_RECTANGLE;
		} else {
			othertype =  JAG_C_COL_TYPE_ELLIPSE;
		}
		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -45;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() < 4 ) { return -46; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -386; } }
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0 " + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3] + " ";
		Jstr nx;
		if ( sp.length() >= 5 ) { nx = sp[4]; } else { nx="0.0"; }
		outstr += nx;
		++cnt;
	} else if ( strncasecmp( p, "rectangle3d(", 12 )==0 || strncasecmp( p, "ellipse3d(", 10 )==0 ) {
		// rectangle( x y z width height nx ny ) 
		if ( strncasecmp( p, "rect", 4 )==0 ) {
			othertype =  JAG_C_COL_TYPE_RECTANGLE3D;
		} else {
			othertype =  JAG_C_COL_TYPE_ELLIPSE3D;
		}
		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -55;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() < 5 ) { return -48; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -387; } }
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 " + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3] + " ";
		outstr += sp[4] + " ";
		Jstr nx, ny;
		if ( sp.length() >= 6 ) { nx = sp[5]; } else { nx="0.0"; }
		if ( sp.length() >= 7 ) { ny = sp[6]; } else { ny="0.0"; }
		outstr += nx + " " + ny;
		++cnt;
	} else if (  strncasecmp( p, "box(", 4 )==0 || strncasecmp( p, "ellipsoid(", 10 )==0 ) {
		// box( x y z width depth height nx ny ) 
		if ( strncasecmp( p, "box", 3 )==0 ) {
			othertype =  JAG_C_COL_TYPE_BOX;
		} else {
			othertype =  JAG_C_COL_TYPE_ELLIPSOID;
		}
		while ( *p != '(' ) ++p;
		++p;  // (p
		if ( *p == 0 ) return -56;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() < 6 ) { return -49; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -388; } }
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 " + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3] + " ";
		outstr += sp[4] + " " + sp[5];
		Jstr nx, ny;
		if ( sp.length() >= 7 ) { nx = sp[6]; } else { nx="0.0"; }
		if ( sp.length() >= 8 ) { ny = sp[7]; } else { ny="0.0"; }
		outstr += nx + " " + ny;
		++cnt;
	} else if ( strncasecmp( p, "cylinder(", 9 )==0 || strncasecmp( p, "cone(", 5 )==0 ) {
		// cylinder( x y z r height  nx ny 
		if ( strncasecmp( p, "cone", 4 )==0 ) {
			othertype =  JAG_C_COL_TYPE_CONE;
		} else {
			othertype =  JAG_C_COL_TYPE_CYLINDER;
		}
		while ( *p != '(' ) ++p;
		++p;  // (p
		if ( *p == 0 ) return -58;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() < 5 ) { return -52; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -390; } }
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 " + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3] + " ";
		outstr += sp[4];
		Jstr nx, ny;
		if ( sp.length() >= 6 ) { nx = sp[5]; } else { nx="0.0"; }
		if ( sp.length() >= 7 ) { ny = sp[6]; } else { ny="0.0"; }
		outstr += nx + " " + ny;
		++cnt;
	} else if ( strncasecmp( p, "line(", 5 )==0 ) {
		// line( x1 y1 x2 y2)
		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -61;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() != 4 ) { return -55; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -391; } }
		othertype =  JAG_C_COL_TYPE_LINE;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0 " + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3];
		++cnt;
	} else if ( strncasecmp( p, "line3d(", 7 )==0 ) {
		// line3d(x1 y1 z1 x2 y2 z2)
		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -64;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() != 6 ) { return -57; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -440; } }
		othertype =  JAG_C_COL_TYPE_LINE3D;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 " + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3] + " ";
		outstr += sp[4] + " " + sp[5];
		++cnt;
	} else if ( strncasecmp( p, "linestring(", 11 )==0 ) {
		while ( *p != '(' ) ++p;  ++p;
		if ( *p == 0 ) return -64;
		othertype =  JAG_C_COL_TYPE_LINESTRING;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0 ";
		JagStrSplit sp(p, ',', true );
		int len = sp.length();
		for ( int i = 0; i < len; ++i ) {
			JagStrSplit ss( sp[i], ' ', true );
			if ( ss.length() < 2 ) {  continue; }
			outstr += Jstr(" ") + ss[0] + ":" + ss[1];
		}
		++cnt;
	} else if ( strncasecmp( p, "linestring3d(", 13 )==0 ) {
		// linestring( x1 y1 z1, x2 y2 z2, x3 y3 z3, x4 y4 z4)
		//d("s2836 linestring3d( p=[%s]\n", p );
		while ( *p != '(' ) ++p; ++p;
		if ( *p == 0 ) return -65;
		othertype =  JAG_C_COL_TYPE_LINESTRING3D;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 ";
		JagStrSplit sp(p, ',', true );
		int len = sp.length();
		for ( int i = 0; i < len; ++i ) {
			JagStrSplit ss( sp[i], ' ', true );
			if ( ss.length() < 3 ) {  continue; }
			outstr += Jstr(" ") + ss[0] + ":" + ss[1]  + ":" + ss[2];
			++cnt;
		}
	} else if ( strncasecmp( p, "multipoint(", 11 )==0 ) {
		// multipoint( x1 y1, x2 y2, x3 y3, x4 y4)
		//d("s2834 multipoint( p=[%s]\n", p );
		while ( *p != '(' ) ++p;  ++p;
		if ( *p == 0 ) return -67;
		othertype =  JAG_C_COL_TYPE_MULTIPOINT;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0 ";
		JagStrSplit sp(p, ',', true );
		int len = sp.length();
		for ( int i = 0; i < len; ++i ) {
			JagStrSplit ss( sp[i], ' ', true );
			if ( ss.length() < 2 ) {  continue; }
			outstr += Jstr(" ") + ss[0] + ":" + ss[1];
			++cnt;
		}
	} else if ( strncasecmp( p, "multipoint3d(", 13 )==0 ) {
		// multipoint3d( x1 y1 z1, x2 y2 z2, x3 y3 z3, x4 y4 z4)
		//d("s2836 multipoint3d( p=[%s]\n", p );
		while ( *p != '(' ) ++p; ++p;
		if ( *p == 0 ) return -68;
		othertype =  JAG_C_COL_TYPE_MULTIPOINT3D;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 ";
		JagStrSplit sp(p, ',', true );
		int len = sp.length();
		for ( int i = 0; i < len; ++i ) {
			JagStrSplit ss( sp[i], ' ', true );
			if ( ss.length() < 3 ) {  continue; }
			outstr += Jstr(" ") + ss[0] + ":" + ss[1]  + ":" + ss[2];
			++cnt;
		}
	} else if ( strncasecmp( p, "polygon(", 8 )==0 ) {
		// polygon( ( x1 y1, x2 y2, x3 y3, x4 y4), ( 2 3, 3 4, 9 8, 2 3 ), ( ...) )
		//d("s3834 polygon( p=[%s]\n", p );
		while ( *p != '(' ) ++p; ++p;
		if ( *p == 0 ) return -72;
		othertype =  JAG_C_COL_TYPE_POLYGON;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0 ";
		//rc = JagParser::addPolygonData( Jstr &pgon, const char *p, bool firstOnly, bool mustClose );
		Jstr pgonstr;
		rc = JagParser::addPolygonData( pgonstr, p, false, true );
		if ( rc <= 0 ) return rc; 
		outstr += pgonstr;
		++cnt;
	} else if ( strncasecmp( p, "polygon3d(", 10 )==0 ) {
		// polygon( ( x1 y1 z1, x2 y2 z2, x3 y3 z3, x4 y4 z4), ( 2 3 8, 3 4 0, 9 8 2, 2 3 8 ), ( ...) )
		//d("s3835 polygon3d( p=[%s] )\n", p );
		while ( *p != '(' ) ++p; ++p;
		if ( *p == 0 ) return -73;
		othertype =  JAG_C_COL_TYPE_POLYGON3D;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 ";
		Jstr pgonstr;
		rc = JagParser::addPolygon3DData( pgonstr, p, false, true );
		if ( rc <= 0 ) return rc; 
		outstr += pgonstr;
		++cnt;
	} else if ( strncasecmp( p, "multipolygon(", 13 )==0 ) {
		// multipolygon( (( x1 y1, x2 y2, x3 y3, x4 y4), ( 2 3, 3 4, 9 8, 2 3 ), ( ...)), ( (..), (..) ) )
		d("s3834 multipolygon( p=[%s]\n", p );
		while ( *p != '(' ) ++p;  // p: "( ((...), (...), (...)), (...), ... )
		othertype =  JAG_C_COL_TYPE_MULTIPOLYGON;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0 ";
		Jstr mgon;
		rc = JagParser::addMultiPolygonData( mgon, p, false, false, false );
		d("s3238 addMultiPolygonData mgon=[%s] rc=%d\n", mgon.c_str(), rc );
		if ( rc <= 0 ) return rc; 
		outstr += mgon;
		++cnt;
	} else if ( strncasecmp( p, "multipolygon3d(", 10 )==0 ) {
		// polygon( ( x1 y1 z1, x2 y2 z2, x3 y3 z3, x4 y4 z4), ( 2 3 8, 3 4 0, 9 8 2, 2 3 8 ), ( ...) )
		//d("s3835 polygon3d( p=[%s] )\n", p );
		while ( *p != '(' ) ++p;  // "(p ((...), (...), (...)), (...), ... ) 
		othertype =  JAG_C_COL_TYPE_MULTIPOLYGON3D;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 ";
		Jstr mgon;
		rc = JagParser::addMultiPolygonData( mgon, p, false, true, true );
		if ( rc <= 0 ) return rc; 
		outstr += mgon;
		++cnt;
	} else if ( strncasecmp( p, "multilinestring(", 16 )==0 ) {
		// multilinestring( ( x1 y1, x2 y2, x3 y3, x4 y4), ( 2 3, 3 4, 9 8, 2 3 ), ( ...) )
		//d("s3834 polygon( p=[%s]\n", p );
		while ( *p != '(' ) ++p; ++p;
		if ( *p == 0 ) return -74;
		othertype =  JAG_C_COL_TYPE_MULTILINESTRING;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0 ";
		Jstr pgonstr;
		rc = JagParser::addPolygonData( pgonstr, p, false, false );
		if ( rc <= 0 ) return rc; 
		outstr += pgonstr;
		++cnt;
	} else if ( strncasecmp( p, "multilinestring3d(", 10 )==0 ) {
		// multilinestring3d( ( x1 y1 z1, x2 y2 z2, x3 y3 z3, x4 y4 z4), ( 2 3 8, 3 4 0, 9 8 2, 2 3 8 ), ( ...) )
		//d("s3835 multilinestring3d( p=[%s] )\n", p );
		while ( *p != '(' ) ++p; ++p;
		if ( *p == 0 ) return -78;
		othertype =  JAG_C_COL_TYPE_MULTILINESTRING3D;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 ";
		Jstr pgonstr;
		rc = JagParser::addPolygon3DData( pgonstr, p, false, false );
		if ( rc <= 0 ) return rc; 
		outstr += pgonstr;
		++cnt;
	} else if ( strncasecmp( p, "triangle(", 9 )==0 ) {
		// triangle(x1 y1 x2 y2 x3 y3 )
		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -82;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() != 6 ) { return -387; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -318; } }
		othertype =  JAG_C_COL_TYPE_TRIANGLE;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0 " + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3] + " ";
		outstr += sp[4] + " " + sp[5];
		++cnt;
	} else if ( strncasecmp( p, "triangle3d(", 11 )==0 ) {
		// triangle3d(x1 y1 z1 x2 y2 z2 x3 y3 z3 )
		while ( *p != '(' ) ++p; ++p;  // (p
		if ( *p == 0 ) return -88;
		JagParser::replaceChar(p, ',', ' ', ')' );
		JagStrSplit sp(p, ' ', true );
		if (  sp.length() != 9 ) { return -390; }
		for ( int k=0; k < sp.length(); ++k ) { if ( sp[k].length() >= JAG_POINT_LEN ) { return -3592; } }
		othertype =  JAG_C_COL_TYPE_TRIANGLE3D;
		outstr = Jstr("CJAG=0=0=") + othertype + "=d 0:0:0:0:0:0 " + sp[0] + " " + sp[1] + " " + sp[2] + " " + sp[3] + " ";
		outstr += sp[4] + " " + sp[5] + " " + sp[6] + " " + sp[7] + " " + sp[8];
		++cnt;
	}

	return cnt;
}
#endif


// mk: OJAG or CJAG
// *** not supported lstr: CJAG=0=0=PL=0 ( (0 0, 1 1, 4 6, 9 3, 0 0))
// lstr: CJAG=0=0=LN=0  0  0  1 1
// lstr: OJAG=srid=db.tab.col=LN=0 0 0  1 1
// lstr: OJAG=0=0=LS=0 1:2:3:4 0:0 1:1 4:6 9:3
// lstr: CJAG=0=0=LS=0 0:0 1:1 4:6 9:3
// lstr: CJAG=0=0=ML=0 ( (0 0, 1 1, 4 6, 9 3, 0 0),( 3 4 , 2 1, 9 2, 3 4 ) )
// lstr: OJAG=0=0=ML=0 1:2:3:4 x:y ...|x:y ...
// type: line/3d, linestring/3d, multilinestring/3d
double JagGeo::getGeoLength( const JagFixString &inlstr )
{
	if ( inlstr.size() < 1 ) return 0.0;
	Jstr lstr;
	if ( !strnchr( inlstr.c_str(), '=', 8 ) ) {
		int rc1 = JagParser::convertConstantObjToJAG( inlstr, lstr );
		if ( rc1 <= 0 ) return 0.0;
	} else {
		lstr = inlstr.c_str();
	}

	Jstr colobjstr1 = lstr.firstToken(' ');
	// colobjstr1: "OJAG=srid=db.obj.col=type"
	Jstr gtype;
	JagStrSplit spcol1(colobjstr1, '=');  // OJAG=srid=name=type
	int srid = 0;
	Jstr mark1, colName1;  // colname: "db.tab.col"
	if ( spcol1.length() >= 4 ) {
		mark1 = spcol1[0];
		srid = jagatoi( spcol1[1].c_str() );
		colName1 = spcol1[2];
		gtype = spcol1[3];
	}
	JagStrSplit sp( lstr.c_str(), ' ', true );

	double sum = 0.0;
	char *p;
	const char *str;

	if ( sp.size() > 0 ) {
		if ( gtype == JAG_C_COL_TYPE_LINE ) {
			if ( sp.length() < 5 ) return 0.0;
			return distance( jagatof( sp[1]), jagatof( sp[2]), jagatof( sp[3]), jagatof( sp[4]), srid );
		} else if ( gtype == JAG_C_COL_TYPE_LINE3D ) {
			if ( sp.length() < 7 ) return 0.0;
			return distance( jagatof( sp[1]), jagatof( sp[2]), jagatof( sp[3]), 
							 jagatof( sp[4]), jagatof( sp[5]), jagatof( sp[6]), srid );
		} else if ( gtype == JAG_C_COL_TYPE_LINESTRING || gtype == JAG_C_COL_TYPE_MULTILINESTRING ) {
			double x1, y1, x2, y2;
			for ( int i = JAG_SP_START; i < sp.length()-1; ++i ) {
				str = sp[i].c_str();
				if ( strchrnum( str, ':') < 1 ) continue;
				get2double(str, p, ':', x1, y1 );
				str = sp[i+1].c_str();
				if ( strchrnum( str, ':') < 1 ) continue;
				get2double(str, p, ':', x2, y2 );
				sum += distance( x1, y1, x2, y2, srid );
			}
			return sum;
		} else if ( gtype == JAG_C_COL_TYPE_LINESTRING3D || gtype == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
			double x1, y1, z1, x2, y2, z2;
			for ( int i = JAG_SP_START; i < sp.length()-1; ++i ) {
				str = sp[i].c_str();
				if ( strchrnum( str, ':') < 2 ) continue;
				get3double(str, p, ':', x1, y1, z1 );
				str = sp[i+1].c_str();
				if ( strchrnum( str, ':') < 2 ) continue;
				get3double(str, p, ':', x2, y2, z2 );
				sum += distance( x1, y1, z1, x2, y2, z2, srid );
			}
			return sum;
		} else {
		}
	}

	return 0.0;
}

Jstr JagGeo::bboxstr( const JagStrSplit &sp, bool skipInnerRings ) 
{
	d("s7330 JagGeo::bboxstr sp:\n" );
	//sp.print();

	double xmin, ymin, zmin, xmax, ymax, zmax;
	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	double dx, dy, dz;
	bool is3D = false;
	const char *str;
	char *p;
	int cnum;
	bool firstRing = true;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		if (  sp[i] == "!" ) { firstRing = true; continue; }
		if (  sp[i] == "|" ) { firstRing = false; continue; }

		if ( !firstRing && skipInnerRings ) { continue; }

		cnum = strchrnum( sp[i].c_str(), ':');
		if ( cnum < 1 ) continue;
		str = sp[i].c_str();
		if ( cnum == 2 ) { 
			is3D = true; 
			get3double(str, p, ':', dx, dy, dz );
			if ( dx < xmin ) xmin = dx;
			if ( dx > xmax ) xmax = dx;
			if ( dy < ymin ) ymin = dy;
			if ( dy > ymax ) ymax = dy;
			if ( dz < zmin ) zmin = dz;
			if ( dz > zmax ) zmax = dz;
		} else {
			get2double(str, p, ':', dx, dy );
			if ( dx < xmin ) xmin = dx;
			if ( dx > xmax ) xmax = dx;
			if ( dy < ymin ) ymin = dy;
			if ( dy > ymax ) ymax = dy;
		}
	}

	Jstr res;
	if ( is3D ) {
		res = d2s(xmin) + " " + d2s(ymin) + " " + d2s(zmin) 
		      + " " + d2s(xmax) + " " + d2s(ymax) + " " + d2s(zmax)  ;
	} else {
		res = d2s(xmin) + " " + d2s(ymin) + " " + d2s(xmax) + " " + d2s(ymax);
	}
	d("s2239 is3D=%d res=[%s]\n", is3D, res.c_str() );

	return res;
}

void JagGeo::bbox2D( const JagStrSplit &sp, JagBox2D &bbox )
{
	double xmin, ymin, xmax, ymax ;
	xmin = ymin = JAG_LONG_MAX;
	xmax = ymax = JAG_LONG_MIN;
	double dx, dy;
	const char *str;
	char *p;
	int cnum;
	bool firstRing = true;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		if (  sp[i] == "!" ) { firstRing = true; continue; }
		if (  sp[i] == "|" ) { firstRing = false; continue; }

		if ( !firstRing ) { continue; }

		cnum = strchrnum( sp[i].c_str(), ':');
		if ( cnum < 1 ) continue;
		str = sp[i].c_str();
		get2double(str, p, ':', dx, dy );
		if ( dx < xmin ) xmin = dx;
		if ( dx > xmax ) xmax = dx;
		if ( dy < ymin ) ymin = dy;
		if ( dy > ymax ) ymax = dy;
	}

	bbox.xmin = xmin;
	bbox.ymin = ymin;
	bbox.xmax = xmax;
	bbox.ymax = ymax;
}

void JagGeo::bbox3D( const JagStrSplit &sp, JagBox3D &bbox )
{
	double xmin, ymin, zmin, xmax, ymax, zmax;
	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	double dx, dy, dz;
	const char *str;
	char *p;
	int cnum;
	bool firstRing = true;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		if (  sp[i] == "!" ) { firstRing = true; continue; }
		if (  sp[i] == "|" ) { firstRing = false; continue; }

		if ( !firstRing ) { continue; }

		cnum = strchrnum( sp[i].c_str(), ':');
		if ( cnum < 2 ) continue;
		str = sp[i].c_str();
			get3double(str, p, ':', dx, dy, dz );
			if ( dx < xmin ) xmin = dx;
			if ( dx > xmax ) xmax = dx;
			if ( dy < ymin ) ymin = dy;
			if ( dy > ymax ) ymax = dy;
			if ( dz < zmin ) zmin = dz;
			if ( dz > zmax ) zmax = dz;
	}

	bbox.xmin = xmin;
	bbox.ymin = ymin;
	bbox.zmin = zmin;
	bbox.xmax = xmax;
	bbox.ymax = ymax;
	bbox.zmax = zmax;
}

// of linestring/3d, multilinestring/3d
// ps: "hdr bbx x:y ... | x:y ..."
// ps: "hdr x:y ... | x:y ..."
// ps: "hdr x:y ... x:y ..."
int JagGeo::numberOfSegments( const JagStrSplit &sp )
{
	int num = 0;
	int nc;
   	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
   		if ( sp[i] == "|" || sp[i] == "!" ) { 
			--num;
		} else {
       		nc = strchrnum( sp[i].c_str(), ':');
       		if ( nc < 1 ) continue;
			++num;
   		}
	}

	--num;
	if ( num < 0 ) return 0;
	return num;
}

// sum over (x2-x1)(y2+y1): if < 0: CCW  if > 0: CW
bool JagGeo::isPolygonCCW( const JagStrSplit &sp )
{
    double dx, dy;
    const char *str;
    char *p;
	bool firstRing = true;
	double sum = 0.0;
	double lastx, lasty;
	bool firstPoint = true;
	int n = 0;
   	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
   		if ( sp[i] == "!" ) { 
			d("s1296 sum=%f\n", sum );
			if ( firstRing ) {
				if ( sum > 0.0 ) { return false; } // outer-ring is CW
			} else {
				if ( sum < 0.0 ) { return false; } // inner-ring is CCW
			}
			sum = 0.0;
			firstRing = true; firstPoint = true; 
		} else if ( sp[i] == "|" ) { 
			d("s9293 sum=%f\n", sum );
			if ( firstRing ) {
				if ( sum > 0.0 ) { return false; } // outer-ring is CW
			} else {
				if ( sum < 0.0 ) { return false; } // inner-ring is CCW
			}

			sum = 0.0;
			firstRing = false; firstPoint = true; 
		} else {
       		str = sp[i].c_str();
       		if ( strchrnum( str, ':') < 1 ) continue;
       		get2double(str, p, ':', dx, dy );
			if ( ! firstPoint ) {
				sum += (dx - lastx )* (dy + lasty );
				++n;
			} else {
				firstPoint = false;
			}
			lastx = dx;
			lasty = dy;
   		}
	}

	if ( firstRing ) {
		d("s4294 sum=%f\n", sum );
		if ( sum > 0.0 ) { return false; } // outer-ring is CW
	} else {
		if ( sum < 0.0 ) { return false; } // inner-ring is CCW
	}

	d("s3748 n=%d\n", n );
	if ( n < 1 ) return false;
	return true;
}

// sum over (x2-x1)(y2+y1): if < 0: CCW  if > 0: CW
bool JagGeo::isPolygonCW( const JagStrSplit &sp )
{
    double dx, dy;
    const char *str;
    char *p;
	bool firstRing = true;
	double sum = 0.0;
	double lastx, lasty;
	bool firstPoint = true;
	int n = 0;
   	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
   		if ( sp[i] == "!" ) { 
			d("s1294 sum=%f\n", sum );
			if ( firstRing ) {
				if ( sum < 0.0 ) { return false; } // outer-ring is CCW
			} else {
				if ( sum > 0.0 ) { return false; } // inner-ring is CW
			}
			sum = 0.0;
			firstRing = true; firstPoint = true; 
		} else if ( sp[i] == "|" ) { 
			d("s0294 sum=%f\n", sum );
			if ( firstRing ) {
				if ( sum < 0.0 ) { return false; } // outer-ring is CCW
			} else {
				if ( sum > 0.0 ) { return false; } // inner-ring is CW
			}

			sum = 0.0;
			firstRing = false; firstPoint = true; 
		} else {
       		str = sp[i].c_str();
       		if ( strchrnum( str, ':') < 1 ) continue;
       		get2double(str, p, ':', dx, dy );
			if ( ! firstPoint ) {
				sum += (dx - lastx )* (dy + lasty );
				++n;
			} else {
				firstPoint = false;
			}
			lastx = dx;
			lasty = dy;
   		}
	}

	if ( firstRing ) {
		d("s1594 sum=%f\n", sum );
		if ( sum < 0.0 ) { return false; } // outer-ring is CCW
	} else {
		if ( sum > 0.0 ) { return false; } // inner-ring is CW
	}

	d("s3798 n=%d\n", n );
	if ( n < 1 ) return false;
	return true;
}


///////// additon or union
Jstr JagGeo::doPointAddition( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	Jstr val;
	if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		val = Jstr("CJAG=0=0=LN=d 0:0:0:0 ") + sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1] + " " 
		        + sp2[JAG_SP_START+0] + " " + sp2[JAG_SP_START+1]; 
	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
		val = Jstr("CJAG=0=0=LS=d 0:0:0:0 ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1] + " " 
		        + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + " " + sp2[JAG_SP_START+2] + ":" + sp2[JAG_SP_START+3];
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOINT ) {
		val = Jstr("CJAG=0=0=MP=d 0:0:0:0 ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1];
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		val = Jstr("CJAG=0=0=LS=d 0:0:0:0 ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1];
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else {
	}

	return val;
}

Jstr JagGeo::doPoint3DAddition( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	Jstr val;
	if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		val = Jstr("CJAG=0=0=LN3=d 0:0:0:0:0:0 ") + sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1]  +" " + sp1[JAG_SP_START+2]
					+ " " + sp2[JAG_SP_START+0] + " " + sp2[JAG_SP_START+1] + " " + sp2[JAG_SP_START+2];
	} else if ( colType2 == JAG_C_COL_TYPE_LINE3D ) {
		val = Jstr("CJAG=0=0=LS3=d 0:0:0:0:0:0 ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1] + ":" + sp1[JAG_SP_START+2] + " " 
		   + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2] + " "
		   + sp2[JAG_SP_START+3] + ":" + sp2[JAG_SP_START+4] + ":" + sp2[JAG_SP_START+5];
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0 ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1] + ":" + sp1[JAG_SP_START+2];
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		val = Jstr("CJAG=0=0=LS3=d 0:0:0:0:0:0 ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1]  + ":" + sp1[JAG_SP_START+2];
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else {
	}

	return val;
}

// 2D line
Jstr JagGeo::doLineAddition( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	Jstr val;
	Jstr p2 = sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1] + " " + sp1[JAG_SP_START+2] + ":" + sp1[JAG_SP_START+3];
	if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		val = Jstr("CJAG=0=0=ML=d 0:0:0:0 ") + p2 + " |"; 
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		val = Jstr("CJAG=0=0=LS=d 0:0:0:0 ") + p2 + " " + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1]; 
	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
		val = Jstr("CJAG=0=0=LS=d 0:0:0:0 ") + p2 + " " + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1]
		        + sp2[JAG_SP_START+2] + ":" + sp2[JAG_SP_START+3]; 
	} else {
	}
	return val;
}

Jstr JagGeo::doLine3DAddition( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
										 int srid2, const JagStrSplit &sp2 )
{
	Jstr val;
	Jstr p2 = sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1] + ":" + sp1[JAG_SP_START+2] 
	                    + " " + sp1[JAG_SP_START+3] + ":" + sp1[JAG_SP_START+4] + ":" + sp1[JAG_SP_START+5];
	if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		val = Jstr("CJAG=0=0=ML3=d 0:0:0:0:0:0 ") + p2 + " |"; 
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		val = Jstr("CJAG=0=0=LS3=d 0:0:0:0:0:0 ") + p2 + " " 
				+ sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2]; 
	} else if ( colType2 == JAG_C_COL_TYPE_LINE3D ) {
		val = Jstr("CJAG=0=0=LS3=d 0:0:0:0:0:0 ") + p2 + " " 
		        + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2]
		        + " " + sp2[JAG_SP_START+3] + ":" + sp2[JAG_SP_START+4] + ":" + sp2[JAG_SP_START+5]; 
	} else {
	}
	return val;
}

// 2D linestring
Jstr JagGeo::doLineStringAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
								    const Jstr &mk2, const Jstr &colType2, 
								    int srid2, const JagStrSplit &sp2 )
{
	Jstr val;
	if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		val = Jstr("CJAG=0=0=ML=d 0:0:0:0");
		for ( int i= JAG_SP_START; i < sp1.size(); ++i ) {
			val += Jstr(" ") + sp1[i];
		}
		val += Jstr(" |");
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		val = Jstr("CJAG=0=0=LS=d 0:0:0:0");
		for ( int i= JAG_SP_START; i < sp1.size(); ++i ) {
			val += Jstr(" ") + sp1[i];
		}
		val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1];
	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
		val = Jstr("CJAG=0=0=LS=d 0:0:0:0");
		for ( int i= JAG_SP_START; i < sp1.size(); ++i ) {
			val += Jstr(" ") + sp1[i];
		}
		val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + " " 
		         + sp2[JAG_SP_START+2] + ":" + sp2[JAG_SP_START+3];
	} else { 
	}
	return val;
}

Jstr JagGeo::doLineString3DAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
									  const Jstr &mk2, const Jstr &colType2, 
									  int srid2, const JagStrSplit &sp2 )
{
	Jstr val;
	if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		val = Jstr("CJAG=0=0=ML3=d 0:0:0:0:0:0");
		for ( int i= JAG_SP_START; i < sp1.size(); ++i ) {
			val += Jstr(" ") + sp1[i];
		}
		val += Jstr(" |");
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		val = Jstr("CJAG=0=0=LS3=d 0:0:0:0:0:0");
		for ( int i= JAG_SP_START; i < sp1.size(); ++i ) {
			val += Jstr(" ") + sp1[i];
		}
		val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2];
	} else if ( colType2 == JAG_C_COL_TYPE_LINE3D ) {
		val = Jstr("CJAG=0=0=LS=d 0:0:0:0:0:0");
		for ( int i= JAG_SP_START; i < sp1.size(); ++i ) {
			val += Jstr(" ") + sp1[i];
		}
		val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2] 
		         + " " + sp2[JAG_SP_START+3] + ":" + sp2[JAG_SP_START+4] + ":" + sp2[JAG_SP_START+5];
	} else { 
	}
	return val;
}

Jstr JagGeo::doPolygonAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
								    const Jstr &mk2, const Jstr &colType2, 
								    int srid2, const JagStrSplit &sp2 )
{
	Jstr val;
	if ( colType2 == JAG_C_COL_TYPE_POLYGON || colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		// multipolygon
		val = Jstr("CJAG=0=0=MG=d 0:0:0:0");
		for ( int i= JAG_SP_START; i < sp1.size(); ++i ) {
			val += Jstr(" ") + sp1[i];
		}
		val += Jstr(" !");
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else {
	} 
	return val;
}

Jstr JagGeo::doPolygon3DAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
									  const Jstr &mk2, const Jstr &colType2, 
									  int srid2, const JagStrSplit &sp2 )
{
	Jstr val;
	if ( colType2 == JAG_C_COL_TYPE_POLYGON3D || colType2 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		val = Jstr("CJAG=0=0=MG3=d 0:0:0:0:0:0");
		for ( int i= JAG_SP_START; i < sp1.size(); ++i ) {
			val += Jstr(" ") + sp1[i];
		}
		val += Jstr(" !");
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else {
	} 
	return val;
}

Jstr JagGeo::doMultiPolygonAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
								    const Jstr &mk2, const Jstr &colType2, 
								    int srid2, const JagStrSplit &sp2 )
{
	Jstr val;
	if ( colType2 == JAG_C_COL_TYPE_POLYGON || colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		// multipolygon
		val = Jstr("CJAG=0=0=MG=d 0:0:0:0");
		for ( int i= JAG_SP_START; i < sp1.size(); ++i ) {
			val += Jstr(" ") + sp1[i];
		}
		val += Jstr(" !");
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else {
	} 
	return val;
}

Jstr JagGeo::doMultiPolygon3DAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
									  const Jstr &mk2, const Jstr &colType2, 
									  int srid2, const JagStrSplit &sp2 )
{
	Jstr val;
	if ( colType2 == JAG_C_COL_TYPE_POLYGON3D || colType2 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		val = Jstr("CJAG=0=0=MG3=d 0:0:0:0:0:0");
		for ( int i= JAG_SP_START; i < sp1.size(); ++i ) {
			val += Jstr(" ") + sp1[i];
		}
		val += Jstr(" !");
		for ( int i= JAG_SP_START; i < sp2.size(); ++i ) {
			val += Jstr(" ") + sp2[i];
		}
	} else {
	} 
	return val;
}

// Union of two polygons
Jstr JagGeo::doPolygonUnion( const Jstr &mk1, int srid1, const JagStrSplit &sp1, 
								    const Jstr &mk2, const Jstr &colType2, 
								    int srid2, const JagStrSplit &sp2 )
{
	d("s4811 doPolygonUnion colType2=%s\n", colType2.c_str() );
	Jstr val;
	Jstr t;
	JagFixString txt;
	std::vector< std::string> vec;
	Jstr uwkt;
	//char *p;
	int rc;
	if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		rc = JagCGAL::unionOfTwoPolygons( sp1, sp2, uwkt );
		if ( rc < 0 ) {
			d("s2038 error unionOfTwoPolygons rc=%d\n", rc );
			return "";
		}
		rc = JagParser::convertConstantObjToJAG( JagFixString(uwkt.c_str()), val );
		if ( rc <= 0 ) { val = ""; }
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		Jstr res;
		rc = JagCGAL::unionOfPolygonAndMultiPolygons( sp1, sp2, res );
		// res is WKT
		if ( rc < 0 ) {
			d("s2039 error unionOfPolygonAndMultiPolygons rc=%d res=[%s]\n", rc, res.c_str() );
			return "";
		}
		d("s1728 res=[%s] rc=%d\n", res.c_str(), rc );
		if ( res.size() < 1 ) return "";
		rc = JagParser::convertConstantObjToJAG( JagFixString(res.c_str()), val );
		d("s1728 val=[%s] rc=%d\n", val.c_str(), rc );
		if ( rc <= 0 ) {
			d("s2038 error JagParser::convertConstantObjToJAG rc=%d\n", rc );
			return "";
		}
	} 
	return val;

	// int JagParser::convertConstantObjToJAG( const JagFixString &instr, Jstr &outstr )

}


Jstr JagGeo::doPointIntersection( const Jstr &colType1,const JagStrSplit &sp1, 
											 const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";

	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0;
	if ( 3 == dim1 ) { pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); } 

	Jstr val;
	if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		if ( 3 == dim2 ) { 
			double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			if ( jagEQ(px0,x0) && jagEQ(py0,y0) && jagEQ(pz0, z0) ) {
				val = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0 ") 
				      + sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1] + " " + sp1[JAG_SP_START+2];
			} 
		} else {
			if ( jagEQ(px0,x0) && jagEQ(py0,y0) ) {
				val = Jstr("CJAG=0=0=PT=d 0:0:0:0 ") + sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1];
			} 
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		if ( 3 == dim2 ) { 
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
			double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
			if ( jagEQ(px0,x1) && jagEQ(py0,y1) && jagEQ(pz0, z1) ) {
				val = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0 ") 
				      + sp2[JAG_SP_START+0] + " " + sp2[JAG_SP_START+1] + " " + sp2[JAG_SP_START+2];
			} else if ( jagEQ(px0,x2) && jagEQ(py0,y2) && jagEQ(pz0, z2) ) {
				val = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0 ") 
				      + sp2[JAG_SP_START+3] + " " + sp2[JAG_SP_START+4] + " " + sp2[JAG_SP_START+5];
			}
		} else {
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			if ( jagEQ(px0,x1) && jagEQ(py0,y1) ) {
				val = Jstr("CJAG=0=0=PT=d 0:0:0:0 ") + sp2[JAG_SP_START+0] + " " + sp2[JAG_SP_START+1];
			} else if ( jagEQ(px0,x2) && jagEQ(py0,y2) ) {
				val = Jstr("CJAG=0=0=PT=d 0:0:0:0 ") + sp2[JAG_SP_START+2] + " " + sp2[JAG_SP_START+3];
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D
	            || colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D
	            || colType2 == JAG_C_COL_TYPE_POLYGON || colType2 == JAG_C_COL_TYPE_POLYGON3D
	            || colType2 == JAG_C_COL_TYPE_MULTIPOINT || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		// point or point3d
		if ( 2 == dim2 ) {
			Jstr xs, ys;
			if ( matchPoint2D( px0,py0, sp2, xs, ys ) ) {
				val = Jstr("CJAG=0=0=PT=d 0:0:0:0 ") + xs + " " + ys;
			}
		} else if ( 3 == dim2 ) {
			Jstr xs, ys, zs;
			if ( matchPoint3D( px0,py0,pz0, sp2, xs, ys, zs ) ) {
				val = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0 ") + xs + " " + ys + " " + zs;
			}
		}
	} 
	return val;
}

// No need to check points
// output is multipoint MP/MP3
Jstr  JagGeo::doLineIntersection( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";

	double px1,py1,pz1,px2,py2,pz2;
	if ( 3 == dim1 ) { 
		px1 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
		py1 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
		pz1 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
		px2 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
		py2 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
		pz2 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
	} else {
		px1 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
		py1 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
		px2 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
		py2 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
	}

	Jstr val;
	int cnt = 0;
	if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		if ( 3 == dim2 ) { 
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
			double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
			JagLine3D line1(px1,py1,pz1, px2,py2,pz2);
			JagLine3D line2(x1,y1,z1, x2,y2,z2);
			JagVector<JagPoint3D> res;
			val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0") ;
			bool sect = JagCGAL::hasIntersection( line1, line2, res );
			if ( sect ) {
				for ( int i=0; i < res.size(); ++i ) {
					val += Jstr(" ") + d2s(res[i].x) + ":" + d2s(res[i].y) + ":" + d2s(res[i].z);
					++cnt;
				}
			}
		} else {
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			JagLine2D line1(px1,py1,px2,py2);
			JagLine2D line2(x1,y1, x2,y2);
			JagVector<JagPoint2D> res;
			val = Jstr("CJAG=0=0=MP=d 0:0:0:0");
			bool sect = JagCGAL::hasIntersection( line1, line2, res );
			if ( sect ) {
				for ( int i=0; i < res.size(); ++i ) {
					val += Jstr(" ") + d2s(res[i].x) + ":" + d2s(res[i].y);
					++cnt;
				}
			}

		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D
	            || colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D
	            || colType2 == JAG_C_COL_TYPE_POLYGON || colType2 == JAG_C_COL_TYPE_POLYGON3D ) {
		// point or point3d
		if ( 2 == dim2 ) {
			val = Jstr("CJAG=0=0=MP=d 0:0:0:0");
			JagLine2D line1(px1,py1,px2,py2);
			JagVector<JagPoint2D> vec;
			if ( line2DLineStringIntersection( line1, sp2, vec )) {
				for ( int i=0; i < vec.size(); ++i ) {
					val += Jstr(" ") + d2s(vec[i].x) + ":" + d2s(vec[i].y);
					++cnt;
				}
			}
		} else if ( 3 == dim2 ) {
			val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0");
			JagLine3D line1(px1,py1,pz1, px2,py2,pz2);
			JagVector<JagPoint3D> vec;
			if ( line3DLineStringIntersection( line1, sp2, vec )) {
				for ( int i=0; i < vec.size(); ++i ) {
					val += Jstr(" ") + d2s(vec[i].x) + ":" + d2s(vec[i].y) + ":" + d2s(vec[i].z);
					++cnt;
				}
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOINT || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		// point or point3d
		if ( 2 == dim2 ) {
			val = Jstr("CJAG=0=0=MP=d 0:0:0:0");
			Jstr xs, ys;
			if ( matchPoint2D( px1,py1, sp2, xs, ys ) ) {
				val += Jstr(" ") + xs + ":" + ys;
				++cnt;
			}
			if ( matchPoint2D( px2,py2, sp2, xs, ys ) ) {
				val += Jstr(" ") + xs + ":" + ys;
				++cnt;
			}

		} else if ( 3 == dim2 ) {
			val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0");
			Jstr xs, ys, zs;
			if ( matchPoint3D( px1,py1,pz1, sp2, xs, ys, zs ) ) {
				val += Jstr(" ") + xs + ":" + ys + ":" + zs;
				++cnt;
			}
			if ( matchPoint3D( px2,py2,pz2, sp2, xs, ys, zs ) ) {
				val += Jstr(" ") + xs + ":" + ys + ":" + zs;
				++cnt;
			}
		}
	} 

	if ( cnt > 0 ) {
		return val;
	} else {
		return "";
	}
}

Jstr  JagGeo::doLineStringIntersection( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	Jstr val;
	if ( 2 == dim2 ) {
		val = Jstr("CJAG=0=0=MP=d 0:0:0:0");
	} else {
		val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0");
	}

	//const char *str; char *p;
	//double px1,py1,pz1,px2,py2,pz2;
	int cnt = 0;
	if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		if ( 2 == dim2 ) {
    		JagVector<JagPoint2D> vec;
			bool intsect =  lineStringIntersectLineString( "",sp1, "", sp2, true, vec );
			if ( intsect ) {
   				for ( int i=0; i < vec.size(); ++i ) {
    				val += Jstr(" ") + d2s(vec[i].x) + ":" + d2s(vec[i].y);
    				++cnt;
				}
			}
		} else if ( 3 == dim2 ) {
    		JagVector<JagPoint3D> vec;
			bool intsect =  lineString3DIntersectLineString3D( "",sp1, "", sp2, true, vec );
			if ( intsect ) {
   				for ( int i=0; i < vec.size(); ++i ) {
   					val += Jstr(" ") + d2s(vec[i].x) + ":" + d2s(vec[i].y) + ":" + d2s(vec[i].z);
   					++cnt;
   				}
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D
				|| colType2 == JAG_C_COL_TYPE_POLYGON || colType2 == JAG_C_COL_TYPE_POLYGON3D ) {
		if ( 2 == dim2 ) {
			JagPolygon pgon;
			int n = JagParser::addPolygonData( pgon, sp2, false );
			if ( n <= 0 ) return "";
			JagVector<Jstr> svec;
			splitPolygonToVector( pgon, false, svec );
			bool intsect;
			for ( int i=0; i < svec.size(); ++i ) {
				JagStrSplit sp( svec[i], ' ', true );
    			JagVector<JagPoint2D> pvec;
				intsect =  lineStringIntersectLineString( "",sp1, "", sp, true, pvec );
				if ( intsect ) {
   					for ( int i=0; i < pvec.size(); ++i ) {
   						val += Jstr(" ") + d2s(pvec[i].x) + ":" + d2s(pvec[i].y);
   						++cnt;
   					}
				}
			}
		} else if ( 3 == dim2 ) {
			JagPolygon pgon;
			int n = JagParser::addPolygon3DData( pgon, sp2, false );
			if ( n <= 0 ) return "";
			JagVector<Jstr> svec;
			splitPolygonToVector( pgon, true, svec );
			bool intsect;
			for ( int i=0; i < svec.size(); ++i ) {
				JagStrSplit sp( svec[i], ' ', true );
    			JagVector<JagPoint3D> pvec;
				intsect =  lineString3DIntersectLineString3D( "",sp1, "", sp, true, pvec );
				if ( intsect ) {
   					for ( int i=0; i < pvec.size(); ++i ) {
   						val += Jstr(" ") + d2s(pvec[i].x) + ":" + d2s(pvec[i].y);
   						++cnt;
   					}
				}
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOINT || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		if ( 2 == dim2 ) {
			JagVector<JagPoint2D> vec1;
			JagVector<JagPoint2D> vec2;
			JagCGAL::split2DSPToVector( sp1, vec1 );
			JagCGAL::split2DSPToVector( sp2, vec2 );
			int len1 = vec1.size();
			int len2 = vec1.size();
			JagPoint2D *pt1 =new JagPoint2D[len1];
			JagPoint2D *pt2 =new JagPoint2D[len2];
			for ( int i=0; i < len1; ++i ) { pt1[i] = vec1[i]; }
			for ( int i=0; i < len2; ++i ) { pt2[i] = vec2[i]; }
			inlineQuickSort<JagPoint2D>( pt1, len1 );
			inlineQuickSort<JagPoint2D>( pt2, len2 );
			JagVector<JagPoint2D> pvec;
			JagSortedSetJoin<JagPoint2D>( pt1, len1, pt2, len2, pvec );
			for ( int i=0; i < pvec.size(); ++i ) {
   				val += Jstr(" ") + d2s(pvec[i].x) + ":" + d2s(pvec[i].y);
   				++cnt;
			}
			delete [] pt1;
			delete [] pt2;
		} else if ( 3 == dim2 ) {
			JagVector<JagPoint3D> vec1;
			JagVector<JagPoint3D> vec2;
			JagCGAL::split3DSPToVector( sp1, vec1 );
			JagCGAL::split3DSPToVector( sp2, vec2 );
			int len1 = vec1.size();
			int len2 = vec1.size();
			JagPoint3D *pt1 =new JagPoint3D[len1];
			JagPoint3D *pt2 =new JagPoint3D[len2];
			for ( int i=0; i < len1; ++i ) { pt1[i] = vec1[i]; }
			for ( int i=0; i < len2; ++i ) { pt2[i] = vec2[i]; }
			inlineQuickSort<JagPoint3D>( pt1, len1 );
			inlineQuickSort<JagPoint3D>( pt2, len2 );
			JagVector<JagPoint3D> pvec;
			JagSortedSetJoin<JagPoint3D>( pt1, len1, pt2, len2, pvec );
			for ( int i=0; i < pvec.size(); ++i ) {
   				val += Jstr(" ") + d2s(pvec[i].x) + ":" + d2s(pvec[i].y);
   				++cnt;
			}
			delete [] pt1;
			delete [] pt2;
		}
	} 

	if ( cnt > 0 ) {
		return val;
	} else {
		return "";
	}
}

// sp1 is m-linestring;  sp2: m-linestring/polygon
Jstr  JagGeo::doMultiLineStringIntersection( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	Jstr val;
	if ( 2 == dim2 ) {
		val = Jstr("CJAG=0=0=MP=d 0:0:0:0");
	} else {
		val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0");
	}

	int cnt = 0;
    bool intsect;
	if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D
				|| colType2 == JAG_C_COL_TYPE_POLYGON || colType2 == JAG_C_COL_TYPE_POLYGON3D ) {
		if ( 2 == dim2 ) {
        	JagPolygon pgon1;
        	int n = JagParser::addPolygonData( pgon1, sp1, false );
        	if ( n <= 0 ) return "";
        	JagVector<Jstr> svec1;
        	splitPolygonToVector( pgon1, false, svec1 );
        
        	JagPolygon pgon2;
        	n = JagParser::addPolygonData( pgon2, sp2, false );
        	if ( n <= 0 ) return "";
        	JagVector<Jstr> svec2;
        	splitPolygonToVector( pgon2, false, svec2 );
        
        	for ( int k=0; k < svec1.size(); ++k ) {
        		JagStrSplit sp11( svec1[k], ' ', true );
        		for ( int i=0; i < svec2.size(); ++i ) {
        			JagStrSplit sp22( svec2[i], ' ', true );
            		JagVector<JagPoint2D> pvec;
        			intsect = lineStringIntersectLineString( "",sp11, "", sp22, true, pvec );
        			if ( intsect ) {
           				for ( int i=0; i < pvec.size(); ++i ) {
           					val += Jstr(" ") + d2s(pvec[i].x) + ":" + d2s(pvec[i].y);
           					++cnt;
           				}
        			}
        		}
        	}
		} else {
        	JagPolygon pgon1;
        	int n = JagParser::addPolygon3DData( pgon1, sp1, false );
        	if ( n <= 0 ) return "";
        	JagVector<Jstr> svec1;
        	splitPolygonToVector( pgon1, false, svec1 );
        
        	JagPolygon pgon2;
        	n = JagParser::addPolygon3DData( pgon2, sp2, false );
        	if ( n <= 0 ) return "";
        	JagVector<Jstr> svec2;
        	splitPolygonToVector( pgon2, false, svec2 );
        
        	for ( int k=0; k < svec1.size(); ++k ) {
        		JagStrSplit sp11( svec1[k], ' ', true );
        		for ( int i=0; i < svec2.size(); ++i ) {
        			JagStrSplit sp22( svec2[i], ' ', true );
            		JagVector<JagPoint3D> pvec;
        			intsect = lineString3DIntersectLineString3D( "",sp11, "", sp22, true, pvec );
        			if ( intsect ) {
           				for ( int i=0; i < pvec.size(); ++i ) {
           					val += Jstr(" ") + d2s(pvec[i].x) 
							       + ":" + d2s(pvec[i].y) + ":" + d2s(pvec[i].z);
           					++cnt;
           				}
        			}
        		}
        	}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON || colType2 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		if ( 2 == dim2 ) {
        	JagPolygon pgon1;
        	int n = JagParser::addPolygonData( pgon1, sp1, false );
        	if ( n <= 0 ) return "";
        	JagVector<Jstr> svec1;
        	splitPolygonToVector( pgon1, false, svec1 );
        
			JagVector<JagPolygon> pgvec;
			n = JagParser::addMultiPolygonData( pgvec, sp2, false, false );
        	if ( n <= 0 ) return "";
        	for ( int m=0; m < svec1.size(); ++m ) {
        		JagStrSplit sp11( svec1[m], ' ', true );
    			for ( int j=0; j < pgvec.size(); ++j ) {
                	const JagPolygon &pgon2 = pgvec[j];
                	JagVector<Jstr> svec2;
                	splitPolygonToVector( pgon2, false, svec2 );
                	for ( int k=0; k < svec2.size(); ++k ) {
                		JagStrSplit sp22( svec2[k], ' ', true );
                   		JagVector<JagPoint2D> pvec;
                		intsect = lineStringIntersectLineString( "",sp11, "", sp22, true, pvec );
                		if ( intsect ) {
                   			for ( int i=0; i < pvec.size(); ++i ) {
                   				val += Jstr(" ") + d2s(pvec[i].x) + ":" + d2s(pvec[i].y);
                   				++cnt;
                   			}
                		}
                	}
    			}
			}
		} else {
        	JagPolygon pgon1;
        	int n = JagParser::addPolygon3DData( pgon1, sp1, false );
        	if ( n <= 0 ) return "";
        	JagVector<Jstr> svec1;
        	splitPolygonToVector( pgon1, false, svec1 );
        
			JagVector<JagPolygon> pgvec;
			n = JagParser::addMultiPolygonData( pgvec, sp2, false, true );
        	if ( n <= 0 ) return "";
        	for ( int m=0; m < svec1.size(); ++m ) {
        		JagStrSplit sp11( svec1[m], ' ', true );
    			for ( int j=0; j < pgvec.size(); ++j ) {
                	const JagPolygon &pgon2 = pgvec[j];
                	JagVector<Jstr> svec2;
                	splitPolygonToVector( pgon2, false, svec2 );
                	for ( int k=0; k < svec2.size(); ++k ) {
                		JagStrSplit sp22( svec2[k], ' ', true );
                   		JagVector<JagPoint3D> pvec;
                		intsect = lineString3DIntersectLineString3D( "",sp11, "", sp22, true, pvec );
                		if ( intsect ) {
                   			for ( int i=0; i < pvec.size(); ++i ) {
                   				val += Jstr(" ") + d2s(pvec[i].x) + ":" 
									+ d2s(pvec[i].y) + ":" + d2s(pvec[i].z);
                   				++cnt;
                   			}
                		}
                	}
    			}
			}
		}
		
	} 

	if ( cnt > 0 ) {
		return val;
	} else {
		return "";
	}
}

// polygon intersect polygon? multi-polygon?
Jstr  JagGeo::doPolygonIntersection( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	d("s1029 doPolygonIntersection sp1: sp2:\n" );
	//sp1.print();
	//sp2.print();

	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";

	if ( 2 != dim2 ) return "";  // no 3D polygons

	Jstr val;
	val = Jstr("CJAG=0=0=MG=d 0:0:0:0");

	int cnt = 0;
	int n;
	if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		if ( 2 == dim2 ) {
        	JagPolygon pgon1, pgon2;
        	n = JagParser::addPolygonData( pgon1, sp1, false );
        	if ( n <= 0 ) return "";
        	n = JagParser::addPolygonData( pgon2, sp2, false );
        	if ( n <= 0 ) return "";
			JagVector<JagPolygon> vec;
			n = JagCGAL::getTwoPolygonIntersection( pgon1, pgon2, vec );
        	if ( n <= 0 ) return "";
			for ( int i=0; i <vec.size(); ++i ) {
				if ( vec[i].size() < 1 ) continue;
			    if ( i > 0 ) { val += " !"; }
			    for ( int k=0; k < vec[i].size(); ++k ) {
				    const JagLineString3D &lstr = vec[i].linestr[k];
					if ( lstr.size() < 1 ) continue;
			    	if ( k > 0 ) { val += " |"; }
					for ( int j=0; j < lstr.size(); ++j ) {
						val += Jstr(" ") + d2s(lstr.point[j].x) + ":" + d2s(lstr.point[j].y);
						++cnt;
					}
			  	}
			}
		} 
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
       	JagPolygon pgon1;
       	n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";

		JagVector<JagPolygon> pgvec;
    	n = JagParser::addMultiPolygonData( pgvec, sp2, false, false );
       	if ( n <= 0 ) return "";

		for ( int i=0; i < pgvec.size(); ++i ) {
			const JagPolygon &pgon2 = pgvec[i];
			if ( pgon2.size() < 1 ) continue;
			JagVector<JagPolygon> vec;
			n = JagCGAL::getTwoPolygonIntersection( pgon1, pgon2, vec );
			if ( n <= 0 ) continue;
			for ( int i=0; i <vec.size(); ++i ) {
				if ( vec[i].size() < 1 ) continue;
			    if ( i > 0 ) { val += " !"; }
			    for ( int k=0; k < vec[i].size(); ++k ) {
					const JagLineString3D &lstr = vec[i].linestr[k];
					if ( lstr.size() < 1 ) continue;
			    	if ( k > 0 ) { val += " |"; }
					for ( int j=0; j < lstr.size(); ++j ) {
						val += Jstr(" ") + d2s(lstr.point[j].x) + ":" + d2s(lstr.point[j].y);
						++cnt;
					}
			  	}
			}
		}
	}

	if ( cnt > 0 ) {
		return val;
	} else {
		return "";
	}
}


// colType1 is polygon3d, colType2 can be box, sphere, cube, etc
Jstr  JagGeo::doPolygon3DIntersection( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	d("s1029 doPolygon3DIntersection sp1: sp2:\n" );
	//sp1.print();
	//sp2.print();

	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";

	if ( 3 != dim2 ) return "";  // no 2D polygons

	Jstr val;
	val = Jstr("CJAG=0=0=MG3=d 0:0:0:0:0:0");

	int cnt = 0;
    JagVector<JagSimplePoint3D>  vec;

	if ( colType2 == JAG_C_COL_TYPE_CUBE ) {
        double x0 = jagatof( sp2[JAG_SP_START+0].c_str() );
        double y0 = jagatof( sp2[JAG_SP_START+1].c_str() );
        double z0 = jagatof( sp2[JAG_SP_START+2].c_str() );
        double r = jagatof( sp2[JAG_SP_START+3].c_str() );
        double nx = safeget(sp2, JAG_SP_START+4);
        double ny = safeget(sp2, JAG_SP_START+5);

        polygon3DWithinCubePoints( vec, sp1, x0, y0, z0, r, nx, ny, true );
        cnt += add3DPointsToStr( val, vec );

	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
        double x0 = jagatof( sp2[JAG_SP_START+0].c_str() );
        double y0 = jagatof( sp2[JAG_SP_START+1].c_str() );
        double z0 = jagatof( sp2[JAG_SP_START+2].c_str() );
        double a = jagatof( sp2[JAG_SP_START+3].c_str() );
        double b = jagatof( sp2[JAG_SP_START+4].c_str() );
        double c = jagatof( sp2[JAG_SP_START+5].c_str() );
        double nx = safeget(sp2, JAG_SP_START+6);
        double ny = safeget(sp2, JAG_SP_START+7);

        polygon3DWithinBoxPoints( vec, sp1, x0, y0, z0, a,b,c, nx, ny, true );
        cnt += add3DPointsToStr( val, vec );

	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
        double x = jagatof( sp2[JAG_SP_START+0].c_str() );
        double y = jagatof( sp2[JAG_SP_START+1].c_str() );
        double z = jagatof( sp2[JAG_SP_START+2].c_str() );
        double r = jagatof( sp2[JAG_SP_START+3].c_str() );

        polygon3DWithinSpherePoints( vec, sp1, x, y, z, r, true );
        cnt += add3DPointsToStr( val, vec );

	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
        double x0 = jagatof( sp2[JAG_SP_START+0].c_str() );
        double y0 = jagatof( sp2[JAG_SP_START+1].c_str() );
        double z0 = jagatof( sp2[JAG_SP_START+2].c_str() );
        double a = jagatof( sp2[JAG_SP_START+3].c_str() );
        double b = jagatof( sp2[JAG_SP_START+4].c_str() );
        double c = jagatof( sp2[JAG_SP_START+5].c_str() );
        double nx = safeget(sp2, JAG_SP_START+6);
        double ny = safeget(sp2, JAG_SP_START+7);

        polygon3DWithinEllipsoidPoints( vec, sp1, x0, y0, z0, a,b,c, nx,ny, true );
        cnt += add3DPointsToStr( val, vec );

	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
        double x = jagatof( sp2[JAG_SP_START+0].c_str() );
        double y = jagatof( sp2[JAG_SP_START+1].c_str() );
        double z = jagatof( sp2[JAG_SP_START+2].c_str() );
        double r = jagatof( sp2[JAG_SP_START+3].c_str() );
        double h = jagatof( sp2[JAG_SP_START+4].c_str() );
        double nx = safeget(sp2, JAG_SP_START+5);
        double ny = safeget(sp2, JAG_SP_START+6);

        polygon3DWithinConePoints( vec, sp1, x, y, z, r, h, nx, ny, true );
        cnt += add3DPointsToStr( val, vec );

	}

	if ( cnt > 0 ) {
		return val;
	} else {
		return "";
	}
}

int JagGeo::add3DPointsToStr( Jstr &val, const JagVector<JagSimplePoint3D> &vec )
{
    char buf[128];

    for ( int i=0; i < vec.size(); ++i ) {
        sprintf(buf, "%.5f:%.5f:%.5f", vec[i].x, vec[i].y, vec[i].z );
        val += Jstr(" ") + buf;
    }

    return vec.size();
}

// geom difference = geom1 - geom1(intersection)geom2
Jstr JagGeo::doPointDifference( const Jstr &colType1,const JagStrSplit &sp1, 
											 const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";

	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0;
	int cnt = 0;
	Jstr val;
	if ( 3 == dim1 ) {
		pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
		val = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0 ");
	} else {
		val = Jstr("CJAG=0=0=PT=d 0:0:0:0 ");
	}

	if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		if ( 3 == dim2 ) { 
			double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			if ( ! (jagEQ(px0,x0) && jagEQ(py0,y0) && jagEQ(pz0, z0) ) ) {
				val += sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1] + " " + sp1[JAG_SP_START+2];
				++cnt;
			} 
		} else {
			if ( ! ( jagEQ(px0,x0) && jagEQ(py0,y0) ) ) {
				val += sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1];
				++cnt;
			} 
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		if ( 3 == dim2 ) { 
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
			double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
			if ( ! ( jagEQ(px0,x1) && jagEQ(py0,y1) && jagEQ(pz0, z1) ) &&
			     ! ( jagEQ(px0,x2) && jagEQ(py0,y2) && jagEQ(pz0, z2) ) ) {
				val += sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1] + " " + sp1[JAG_SP_START+2];
				++cnt;
			} 
		} else {
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			if ( ! (jagEQ(px0,x1) && jagEQ(py0,y1) ) && ! ( jagEQ(px0,x2) && jagEQ(py0,y2) ) ) {
				val += sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1];
				++cnt;
			} 
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D
	            || colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D
	            || colType2 == JAG_C_COL_TYPE_POLYGON || colType2 == JAG_C_COL_TYPE_POLYGON3D
	            || colType2 == JAG_C_COL_TYPE_MULTIPOINT || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		// point or point3d
		if ( 2 == dim2 ) {
			Jstr xs, ys;
			if ( ! matchPoint2D( px0,py0, sp2, xs, ys ) ) {
				val += sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1];
				++cnt;
			}
		} else if ( 3 == dim2 ) {
			Jstr xs, ys, zs;
			if ( ! matchPoint3D( px0,py0,pz0, sp2, xs, ys, zs ) ) {
				val += sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1] + " " + sp1[JAG_SP_START+2];
				++cnt;
			}
		}
	} 

	if ( cnt < 1) return "";
	return val;
}


// geom difference = geom1 - geom1(intersection)geom2
// No need to check points
// output is multipoint MP/MP3
Jstr  JagGeo::doLineDifference( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";

	double px1,py1,pz1,px2,py2,pz2;
	JagVector<JagPoint3D> vec1; 
	Jstr val;
	if ( 3 == dim1 ) { 
		px1 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
		py1 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
		pz1 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
		px2 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
		py2 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
		pz2 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
		vec1.append(JagPoint3D(px1,py1,pz1) );
		vec1.append(JagPoint3D(px2,py2,pz2) );
		val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0") ;
	} else {
		px1 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
		py1 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
		px2 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
		py2 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
		vec1.append(JagPoint3D(px1,py1,0) );
		vec1.append(JagPoint3D(px2,py2,0) );
		val = Jstr("CJAG=0=0=MP=d 0:0:0:0") ;
	}

	int cnt = 0;
	if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D  ) {
		if ( 3 == dim2 ) { 
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			if ( ! ( jagEQ(px1,x1) && jagEQ(py1,y1) && jagEQ(pz1, z1) ) ) {
				val += Jstr(" ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1] + ":" + sp1[JAG_SP_START+2];
				++cnt;
			} 

			if ( ! ( jagEQ(px2,x1) && jagEQ(py2,y1) && jagEQ(pz2, z1) ) ) {
				val += Jstr(" ") + sp1[JAG_SP_START+3] + ":" + sp1[JAG_SP_START+4] + ":" + sp1[JAG_SP_START+5];
				++cnt;
			} 
		} else {
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			if ( ! ( jagEQ(px1,x1) && jagEQ(py1,y1) ) ) {
				val += Jstr(" ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1];
				++cnt;
			} 

			if ( ! ( jagEQ(px2,x1) && jagEQ(py2,y1) ) ) {
				val += Jstr(" ") + sp1[JAG_SP_START+2] + ":" + sp1[JAG_SP_START+3];
				++cnt;
			} 
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		JagVector<JagPoint3D> vec2; 
		if ( 3 == dim2 ) { 
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
			double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
			vec2.append(JagPoint3D(x1,y1,z1) );
			vec2.append(JagPoint3D(x2,y2,z2) );

			JagHashSetStr hash;
			getIntersectionPoints( vec1, vec2, hash ); 
			Jstr hs = vec1[0].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec1[0].str3D(); ++cnt;
			}
			hs = vec1[1].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec1[1].str3D(); ++cnt;
			}
		} else {
			// 2D
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			vec2.append(JagPoint3D(x1,y1,0) );
			vec2.append(JagPoint3D(x2,y2,0) );
			JagHashSetStr hash;
			getIntersectionPoints( vec1, vec2, hash ); 
			Jstr hs = vec1[0].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec1[0].str2D();
				++cnt;
			}
			hs = vec1[1].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec1[1].str2D();
				++cnt;
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D
				|| colType2 == JAG_C_COL_TYPE_MULTIPOINT || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D
				|| colType2 == JAG_C_COL_TYPE_POLYGON ||  colType2 == JAG_C_COL_TYPE_POLYGON3D
	            || colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		// point or point3d
		if ( 2 == dim2 ) {
			JagHashSetStr hash;
			getIntersectionPoints( vec1, sp2, false, hash );
			Jstr hs1, hs2;
			hs1 = vec1[0].hashString();
			hs2 = vec1[1].hashString();
			if ( ! hash.keyExist(hs1) ) {
				val += Jstr(" ") + vec1[0].str2D();
				++cnt;
			}
			if ( ! hash.keyExist(hs2) ) {
				val += Jstr(" ") + vec1[1].str2D();
				++cnt;
			}
		} else if ( 3 == dim2 ) {
			JagHashSetStr hash;
			getIntersectionPoints( vec1, sp2, true, hash );
			Jstr hs = vec1[0].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec1[0].str3D();
				++cnt;
			}
			hs = vec1[1].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec1[1].str3D();
				++cnt;
			}
		}
	} 

	if ( cnt > 0 ) { return val; } else { return ""; }
}

// boost: pgon/pgon  pgon/mpgon  mpgon/mpgon ---> mpgon.  mpgon->toWKT-->convert to JAG
// boost: linestr or m-linestr / pgon or m-pgon --> mlinestring. mlinestring->toWKT-->convert to JAG

Jstr  JagGeo::doLineStringDifference( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	Jstr val;
	JagVector<JagPoint3D> vec1;
	if ( 2 == dim2 ) {
		val = Jstr("CJAG=0=0=MP=d 0:0:0:0");
		getVectorPoints( sp1, false, vec1 );
	} else {
		val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0");
		getVectorPoints( sp1, true, vec1 );
	}

	//const char *str; char *p;
	double px1,py1,pz1,px2,py2,pz2;
	int cnt = 0;
	if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
		px1 = sp2[JAG_SP_START+0].tof();
		py1 = sp2[JAG_SP_START+1].tof();
		if ( 3 == dim2 ) {
			pz1 = sp2[JAG_SP_START+2].tof();
			JagPoint3D p3(px1,py1,pz1);
			for ( int i=0; i < vec1.size(); ++i ) {
				if ( p3 != vec1[i] ) {
					val += Jstr(" ") + vec1[i].str3D(); ++cnt;
				}
			}
		} else {
			JagPoint3D p3(px1,py1,0.0);
			for ( int i=0; i < vec1.size(); ++i ) {
				if ( p3 != vec1[i] ) {
					val += Jstr(" ") + vec1[i].str2D(); ++cnt;
				}
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		if ( 3 == dim2 ) {
			px1 = sp2[JAG_SP_START+0].tof();
			py1 = sp2[JAG_SP_START+1].tof();
			pz1 = sp2[JAG_SP_START+2].tof();
			JagPoint3D p3(px1,py1,pz1);
			px2 = sp2[JAG_SP_START+3].tof();
			py2 = sp2[JAG_SP_START+4].tof();
			pz2 = sp2[JAG_SP_START+5].tof();
			JagPoint3D p4(px2,py2,pz2);
			for ( int i=0; i < vec1.size(); ++i ) {
				if ( p3 != vec1[i] && p4 != vec1[i] ) {
					val += Jstr(" ") + vec1[i].str3D(); ++cnt;
				}
			}
		} else {
			px1 = sp2[JAG_SP_START+0].tof();
			py1 = sp2[JAG_SP_START+1].tof();
			JagPoint3D p3(px1,py1,0.0);
			px2 = sp2[JAG_SP_START+2].tof();
			py2 = sp2[JAG_SP_START+3].tof();
			JagPoint3D p4(px2,py2,0.0);
			for ( int i=0; i < vec1.size(); ++i ) {
				if ( p3 != vec1[i] && p4 != vec1[i] ) {
					val += Jstr(" ") + vec1[i].str2D(); ++cnt;
				}
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		if ( 2 == dim2 ) {
    		Jstr wkt1 = "linestring(";
    		for ( int i=0;i<vec1.size(); ++i ) {
    			if ( 0 != i ) { wkt1 += Jstr(","); }
    			wkt1 += vec1[i].str2D();
    		}
    		wkt1 += ")";

			JagVector<JagPoint3D> vec2;
			getVectorPoints( sp2, false, vec2 );
    		Jstr wkt2 = "linestring(";
    		for ( int i=0;i<vec2.size(); ++i ) {
    			if ( 0 != i ) { wkt2 += Jstr(","); }
    			wkt2 += vec2[i].str2D();
    		}
    		wkt2 += ")";
    
    		Jstr reswkt;
    		JagCGAL::getTwoGeomDifference<BoostLineString2D,BoostLineString2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
    		int n = JagParser::convertConstantObjToJAG(reswkt.c_str(), val );
    		if ( n <= 0 ) return "";
			++cnt;
		} else if ( 3 == dim2 ) {
			JagHashSetStr hash;
			getIntersectionPoints( vec1, sp2, true, hash );
			Jstr hs;
			for ( int k=0; k < vec1.size(); ++k ) {
				hs = vec1[k].hashString();
				if ( ! hash.keyExist(hs) ) {
    				val += Jstr(" ") + vec1[k].str3D(); ++cnt;
    			}
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
    		Jstr wkt1 = "linestring(";
    		for ( int i=0;i<vec1.size(); ++i ) {
    			if ( 0 != i ) { wkt1 += Jstr(","); }
    			wkt1 += vec1[i].str2D();
    		}
    		wkt1 += ")";

			JagPolygon pgon2;
    		Jstr wkt2;
			int n = JagParser::addPolygonData( pgon2, sp2, false );
			if ( n <= 0 ) return "";
			pgon2.toWKT( false, true, "multilinestring", wkt2 );

    		Jstr reswkt;
    		JagCGAL::getTwoGeomDifference<BoostLineString2D,BoostMultiLineString2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
    		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), val );
    		if ( n <= 0 ) return "";
			++cnt;
	} else if (  colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D
				|| colType2 == JAG_C_COL_TYPE_POLYGON3D || colType2 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
				// JAG_C_COL_TYPE_POLYGON is handled separately
			JagPolygon pgon;
			int n = JagParser::addPolygon3DData( pgon, sp2, false );
			if ( n <= 0 ) return "";
			JagVector<Jstr> svec;
			splitPolygonToVector( pgon, true, svec );
			JagHashSetStr hash;
			for ( int i=0; i < svec.size(); ++i ) {
				JagStrSplit sp2( svec[i], ' ', true );
    			getIntersectionPoints( vec1, sp2, true, hash );
			}

			Jstr hs;
    		for ( int k=0; k < vec1.size(); ++k ) {
				hs = vec1[k].hashString();
				if ( ! hash.keyExist(hs) ) {
        			val += Jstr(" ") + vec1[k].str3D(); ++cnt;
        		}
    		}

	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOINT || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		if ( 2 == dim2 ) {
			JagHashSetStr hash;
    		getIntersectionPoints( vec1, sp2, false, hash );
			Jstr hs;
			for ( int k=0; k < vec1.size(); ++k ) {
				hs = vec1[k].hashString();
				if ( ! hash.keyExist(hs) ) {
   				    val += Jstr(" ") + vec1[k].str2D(); ++cnt;
				}
			}
		} else if ( 3 == dim2 ) {
			JagHashSetStr hash;
    		getIntersectionPoints( vec1, sp2, true, hash );
			Jstr hs;
			for ( int k=0; k < vec1.size(); ++k ) {
				hs = vec1[k].hashString();
				if ( ! hash.keyExist(hs) ) {
   				    val += Jstr(" ") + vec1[k].str3D(); ++cnt;
				}
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		// result is m-linestring
		Jstr wkt1 = "linestring(";
		for ( int i=0;i<vec1.size(); ++i ) {
			if ( 0 != i ) { wkt1 += Jstr(","); }
			wkt1 += vec1[i].str2D();
		}
		wkt1 += ")";

		JagPolygon pgon2;
		int n = JagParser::addPolygonData( pgon2, sp2, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		pgon2.toWKT( false, true, "polygon", wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomDifference<BoostLineString2D,BoostPolygon2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), val );
		if ( n <= 0 ) return "";
		++cnt;
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		Jstr wkt1 = "linestring(";
		for ( int i=0;i<vec1.size(); ++i ) {
			if ( 0 != i ) { wkt1 += Jstr(","); }
			wkt1 += vec1[i].str2D();
		}
		wkt1 += ")";

		JagVector<JagPolygon> pgvec2;
		int n = JagParser::addMultiPolygonData( pgvec2, sp2, false, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		multiPolygonToWKT( pgvec2, false, wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomDifference<BoostLineString2D,BoostMultiPolygon2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), val );
		if ( n <= 0 ) return "";
		++cnt;
	}

	if ( cnt > 0 ) { return val; } else { return ""; }
}

// sp1 is m-linestring;  sp2: m-linestring/polygon
Jstr  JagGeo::doMultiLineStringDifference( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	Jstr value;

	int cnt = 0;
    //bool intsect;
	if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	int n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "multilinestring", wkt1 );

		JagPolygon pgon2;
		n = JagParser::addPolygonData( pgon2, sp2, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		pgon1.toWKT( false, true, "linestring", wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomDifference<BoostMultiLineString2D,BoostLineString2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	int n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "multilinestring", wkt1 );

		JagPolygon pgon2;
		n = JagParser::addPolygonData( pgon2, sp2, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		pgon1.toWKT( false, true, "multilinestring", wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomDifference<BoostMultiLineString2D,BoostMultiLineString2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	} else if (  colType2 == JAG_C_COL_TYPE_POLYGON ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	int n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "multilinestring", wkt1 );

		JagPolygon pgon2;
		n = JagParser::addPolygonData( pgon2, sp2, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		pgon1.toWKT( false, true, "polygon", wkt2 );
		Jstr reswkt;
		//JagCGAL::getMultiLineStringPolygonDifference( wkt1, wkt2, reswkt );
		JagCGAL::getTwoGeomDifference<BoostMultiLineString2D,BoostPolygon2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	} else if (  colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D || colType2 == JAG_C_COL_TYPE_POLYGON3D ) {
		// hard to define
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		// hard to define
	} 

	if ( cnt > 0 ) { return value; } else { return ""; }
}

// polygon intersect polygon? multi-polygon?
Jstr  JagGeo::doPolygonDifference( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	d("s1029 doPolygonDifference sp1: sp2:\n" );
	//sp1.print();
	//sp2.print();

	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	if ( 2 != dim2 ) return "";  // no 3D polygons
	Jstr value;
	int cnt = 0;
	int n;
	if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "polygon", wkt1 );

		JagPolygon pgon2;
		n = JagParser::addPolygonData( pgon2, sp2, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		pgon2.toWKT( false, true, "polygon", wkt2 );
		Jstr reswkt;
		//JagCGAL::getPolygonPolygonDifference( wkt1, wkt2, reswkt );
		JagCGAL::getTwoGeomDifference<BoostPolygon2D,BoostPolygon2D,BoostMultiPolygon2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "polygon", wkt1 );

		JagVector<JagPolygon> pgvec2;
		Jstr wkt2;
		n = JagParser::addMultiPolygonData( pgvec2, sp2, false, false );
		if ( n <= 0 ) return "";
		multiPolygonToWKT( pgvec2, false, wkt2 );
		Jstr reswkt;
		//JagCGAL::getPolygonMultiPolygonDifference( wkt1, wkt2, reswkt );
		JagCGAL::getTwoGeomDifference<BoostPolygon2D,BoostMultiPolygon2D,BoostMultiPolygon2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	}

	if ( cnt > 0 ) {
		return value;
	} else {
		return "";
	}
}

// multipolygon multipolygon
Jstr  JagGeo::doMultiPolygonDifference( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	d("s1029 doMultiPolygonDifference sp1: sp2:\n" );
	//sp1.print();
	//sp2.print();

	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	if ( 2 != dim2 ) return "";  // no 3D polygons
	Jstr value;
	int cnt = 0;
	int n;
	if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "polygon", wkt1 );

		JagVector<JagPolygon> pgvec2;
		Jstr wkt2;
		n = JagParser::addMultiPolygonData( pgvec2, sp2, false, false );
		if ( n <= 0 ) return "";
		multiPolygonToWKT( pgvec2, false, wkt2 );
		Jstr reswkt;

		JagCGAL::getTwoGeomDifference<BoostMultiPolygon2D,BoostMultiPolygon2D,BoostMultiPolygon2D>( wkt1, wkt2, reswkt );

		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	}

	if ( cnt > 0 ) { return value; } else { return ""; }
}

//////////////////////////////////////////
// geom symdifference = geom1+geom2 - geom1(intersection)geom2
Jstr JagGeo::doPointSymDifference( const Jstr &colType1,const JagStrSplit &sp1, 
											 const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";

	double px0 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
	double py0 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
	double pz0;
	Jstr val;
	JagPoint3D p1;
	int cnt = 0;
	if ( 3 == dim1 ) {
		pz0 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
		val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0");
		p1.x = px0; p1.y = py0; p1.z = pz0;
	} else {
		val = Jstr("CJAG=0=0=MP=d 0:0:0:0");
		p1.x = px0; p1.y = py0; p1.z = 0.0;
	}

	if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
		double x0 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
		double y0 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
		if ( 3 == dim2 ) { 
			val = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0");
			double z0 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			if ( ! (jagEQ(px0,x0) && jagEQ(py0,y0) && jagEQ(pz0, z0) ) ) {
				val += Jstr(" ") + sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1] + " " + sp1[JAG_SP_START+2];
				++cnt;
			} 
		} else {
			val = Jstr("CJAG=0=0=PT=d 0:0:0:0 ");
			if ( ! ( jagEQ(px0,x0) && jagEQ(py0,y0) ) ) {
				val += Jstr(" ") + sp1[JAG_SP_START+0] + " " + sp1[JAG_SP_START+1];
				++cnt;
			} 
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		//point1  <---> (p2, p3)
		if ( 3 == dim2 ) { 
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			JagPoint3D p2(x1,y1,z1);

			//double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			//double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
			//double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
			JagPoint3D p3(x1,y1,z1);

			if ( p1 == p2 && p1 != p3 ) {
				val += Jstr(" ") + sp2[JAG_SP_START+3] + ":" + sp2[JAG_SP_START+4] + ":" + sp2[JAG_SP_START+5];
				++cnt;
			} else if ( p1 == p3 && p1 != p2 ) {
				val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2];
				++cnt;
			} else if ( p1 != p2 && p1 != p3 ) {
				val += Jstr(" ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1] + ":" + sp1[JAG_SP_START+2];
				val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2];
				val += Jstr(" ") + sp2[JAG_SP_START+3] + ":" + sp2[JAG_SP_START+4] + ":" + sp2[JAG_SP_START+5];
				++cnt;
			} 
		} else {
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			JagPoint3D p2(x1,y1,0.0);

			//double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			//double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			JagPoint3D p3(x1,y1,0.0);

			if ( p1 == p2 && p1 != p3 ) {
				val += Jstr(" ") + sp2[JAG_SP_START+2] + ":" + sp2[JAG_SP_START+3];
				++cnt;
			} else if ( p1 == p3 && p1 != p2 ) {
				val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1];
				++cnt;
			} else if ( p1 != p2 && p1 != p3 ) {
				val += Jstr(" ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1];
				val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1];
				val += Jstr(" ") + sp2[JAG_SP_START+2] + ":" + sp2[JAG_SP_START+3];
				++cnt;
			} 
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D
	            || colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D
	            || colType2 == JAG_C_COL_TYPE_POLYGON || colType2 == JAG_C_COL_TYPE_POLYGON3D
	            || colType2 == JAG_C_COL_TYPE_MULTIPOINT || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		// point or point3d
		if ( 2 == dim2 ) {
			Jstr vecs;
			nonMatchPoint2DVec( px0,py0, sp2, vecs );
			if ( vecs.size() > 0 ) { val += vecs; ++cnt; }
		} else if ( 3 == dim2 ) {
			Jstr vecs;
			nonMatchPoint3DVec( px0,py0,pz0, sp2, vecs );
			if ( vecs.size() > 0 ) { val += vecs; ++cnt; }
		}
	} 

	if ( cnt > 0 ) { return val; } else { return ""; }
}   // end  doPointSymDifference()


// geom difference = geom1 - geom1(intersection)geom2
// No need to check points
// output is multipoint MP/MP3
Jstr  JagGeo::doLineSymDifference( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";

	double px1,py1,pz1,px2,py2,pz2;
	JagVector<JagPoint3D> vec1; 
	Jstr val;
	if ( 3 == dim1 ) { 
		px1 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
		py1 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
		pz1 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
		px2 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
		py2 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
		pz2 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
		vec1.append(JagPoint3D(px1,py1,pz1) );
		vec1.append(JagPoint3D(px2,py2,pz2) );
		val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0") ;
	} else {
		px1 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
		py1 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
		px2 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
		py2 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
		vec1.append(JagPoint3D(px1,py1,0) );
		vec1.append(JagPoint3D(px2,py2,0) );
		val = Jstr("CJAG=0=0=MP=d 0:0:0:0") ;
	}

	int cnt = 0;
	if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D  ) {
		int ndiff = 0;
		if ( 3 == dim2 ) { 
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			if ( ! ( jagEQ(px1,x1) && jagEQ(py1,y1) && jagEQ(pz1, z1) ) ) {
				val += Jstr(" ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1] + ":" + sp1[JAG_SP_START+2];
				++ ndiff;
				++cnt;
			} 

			if ( ! ( jagEQ(px2,x1) && jagEQ(py2,y1) && jagEQ(pz2, z1) ) ) {
				val += Jstr(" ") + sp1[JAG_SP_START+3] + ":" + sp1[JAG_SP_START+4] + ":" + sp1[JAG_SP_START+5];
				++ ndiff;
				++cnt;
			} 

			if ( 2 == ndiff ) {
				// include x1 y1 z1
				val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2];
				++cnt;
			}
		} else {
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			if ( ! ( jagEQ(px1,x1) && jagEQ(py1,y1) ) ) {
				val += Jstr(" ") + sp1[JAG_SP_START+0] + ":" + sp1[JAG_SP_START+1];
				++cnt;
				++ ndiff;
			} 

			if ( ! ( jagEQ(px2,x1) && jagEQ(py2,y1) ) ) {
				val += Jstr(" ") + sp1[JAG_SP_START+2] + ":" + sp1[JAG_SP_START+3];
				++cnt;
				++ ndiff;
			} 
			if ( 2 == ndiff  ) {
				// include x1 y1 
				val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1];
				++cnt;
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		JagVector<JagPoint3D> vec2;  // line-line
		if ( 3 == dim2 ) { 
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double z1 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double x2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			double y2 = jagatof( sp2[JAG_SP_START+4].c_str() ); 
			double z2 = jagatof( sp2[JAG_SP_START+5].c_str() ); 
			vec2.append(JagPoint3D(x1,y1,z1) );
			vec2.append(JagPoint3D(x2,y2,z2) );

			JagHashSetStr hash;
			getIntersectionPoints( vec1, vec2, hash ); 
			Jstr hs = vec1[0].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec1[0].str3D(); ++cnt;
			}
			hs = vec1[1].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec1[1].str3D(); ++cnt;
			}

			hs = vec2[0].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec2[0].str3D(); ++cnt;
			}

			hs = vec2[1].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec2[1].str3D(); ++cnt;
			}
		} else {
			// 2D
			double x1 = jagatof( sp2[JAG_SP_START+0].c_str() ); 
			double y1 = jagatof( sp2[JAG_SP_START+1].c_str() ); 
			double x2 = jagatof( sp2[JAG_SP_START+2].c_str() ); 
			double y2 = jagatof( sp2[JAG_SP_START+3].c_str() ); 
			vec2.append(JagPoint3D(x1,y1,0) );
			vec2.append(JagPoint3D(x2,y2,0) );
			JagHashSetStr hash;
			getIntersectionPoints( vec1, vec2, hash ); 
			Jstr hs = vec1[0].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec1[0].str2D(); ++cnt;
			}
			hs = vec1[1].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec1[1].str2D(); ++cnt;
			}

			hs = vec2[0].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec2[0].str2D(); ++cnt;
			}
			hs = vec2[1].hashString();
			if ( ! hash.keyExist(hs) ) {
				val += Jstr(" ") + vec2[1].str2D(); ++cnt;
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D
				|| colType2 == JAG_C_COL_TYPE_MULTIPOINT || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D
				|| colType2 == JAG_C_COL_TYPE_POLYGON ||  colType2 == JAG_C_COL_TYPE_POLYGON3D
	            || colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		// point or point3d
		if ( 2 == dim2 ) {
			Jstr vecs;
			nonMatchTwoPoint2DVec( px1,py1,px2,py2, sp2, vecs );
			if ( vecs.size() > 0 ) { val += vecs; ++cnt; }
		} else if ( 3 == dim2 ) {
			Jstr vecs;
			nonMatchTwoPoint3DVec( px1,py1,pz1,px2,py2,pz2, sp2, vecs );
			if ( vecs.size() > 0 ) { val += vecs; ++cnt; }
		}
	} 

	if ( cnt > 0 ) { return val; } else { return ""; }
}

// boost: pgon/pgon  pgon/mpgon  mpgon/mpgon ---> mpgon.  mpgon->toWKT-->convert to JAG
// boost: linestr or m-linestr / pgon or m-pgon --> mlinestring. mlinestring->toWKT-->convert to JAG

Jstr  JagGeo::doLineStringSymDifference( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	Jstr val;
	JagVector<JagPoint3D> vec1;
	if ( 2 == dim2 ) {
		val = Jstr("CJAG=0=0=MP=d 0:0:0:0");
		getVectorPoints( sp1, false, vec1 );
	} else {
		val = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0");
		getVectorPoints( sp1, true, vec1 );
	}

	//const char *str; char *p;
	double px1,py1,pz1,px2,py2,pz2;
	int cnt = 0;
	if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
		px1 = sp2[JAG_SP_START+0].tof();
		py1 = sp2[JAG_SP_START+1].tof();
		if ( 3 == dim2 ) {
			pz1 = sp2[JAG_SP_START+2].tof();
			JagPoint3D p3(px1,py1,pz1);
			for ( int i=0; i < vec1.size(); ++i ) {
				if ( p3 != vec1[i] ) {
					val += Jstr(" ") + vec1[i].str3D(); ++cnt;
				}
			}
		} else {
			JagPoint3D p3(px1,py1,0.0);
			for ( int i=0; i < vec1.size(); ++i ) {
				if ( p3 != vec1[i] ) {
					val += Jstr(" ") + vec1[i].str2D(); ++cnt;
				}
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		if ( 3 == dim2 ) {
			px1 = sp2[JAG_SP_START+0].tof();
			py1 = sp2[JAG_SP_START+1].tof();
			pz1 = sp2[JAG_SP_START+2].tof();

			px2 = sp2[JAG_SP_START+3].tof();
			py2 = sp2[JAG_SP_START+4].tof();
			pz2 = sp2[JAG_SP_START+5].tof();

			Jstr vecs;
			nonMatchTwoPoint3DVec( px1,py1,pz1,px2,py2,pz2, sp1, vecs );
			if ( vecs.size() > 0 ) { val += vecs; ++cnt; }
		} else {
			px1 = sp2[JAG_SP_START+0].tof();
			py1 = sp2[JAG_SP_START+1].tof();
			px2 = sp2[JAG_SP_START+2].tof();
			py2 = sp2[JAG_SP_START+3].tof();
			Jstr vecs;
			nonMatchTwoPoint2DVec( px1,py1,px2,py2, sp1, vecs );
			if ( vecs.size() > 0 ) { val += vecs; ++cnt; }
		}
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		if ( 2 == dim2 ) {
    		Jstr wkt1 = "linestring(";
    		for ( int i=0;i<vec1.size(); ++i ) {
    			if ( 0 != i ) { wkt1 += Jstr(","); }
    			wkt1 += vec1[i].str2D();
    		}
    		wkt1 += ")";

			JagVector<JagPoint3D> vec2;
			getVectorPoints( sp2, false, vec2 );
    		Jstr wkt2 = "linestring(";
    		for ( int i=0;i<vec2.size(); ++i ) {
    			if ( 0 != i ) { wkt2 += Jstr(","); }
    			wkt2 += vec2[i].str2D();
    		}
    		wkt2 += ")";
    
    		Jstr reswkt;
    		JagCGAL::getTwoGeomSymDifference<BoostLineString2D,BoostLineString2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
    		int n = JagParser::convertConstantObjToJAG(reswkt.c_str(), val );
    		if ( n <= 0 ) return "";
			++cnt;
		} else if ( 3 == dim2 ) {
			JagVector<JagPoint3D> vec2;
			getVectorPoints( sp2, true, vec2 );
			JagHashSetStr hash;
			getIntersectionPoints( vec1, vec2, hash );
			Jstr hs;
			for ( int k=0; k < vec1.size(); ++k ) {
				hs = vec1[k].hashString();
				if ( ! hash.keyExist(hs) ) {
    				val += Jstr(" ") + vec1[k].str3D(); ++cnt;
    			}
			}
			for ( int k=0; k < vec2.size(); ++k ) {
				hs = vec2[k].hashString();
				if ( ! hash.keyExist(hs) ) {
    				val += Jstr(" ") + vec2[k].str3D(); ++cnt;
    			}
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
    		Jstr wkt1 = "linestring(";
    		for ( int i=0;i<vec1.size(); ++i ) {
    			if ( 0 != i ) { wkt1 += Jstr(","); }
    			wkt1 += vec1[i].str2D();
    		}
    		wkt1 += ")";

			JagPolygon pgon2;
    		Jstr wkt2;
			int n = JagParser::addPolygonData( pgon2, sp2, false );
			if ( n <= 0 ) return "";
			pgon2.toWKT( false, true, "multilinestring", wkt2 );

    		Jstr reswkt;
    		JagCGAL::getTwoGeomSymDifference<BoostLineString2D,BoostMultiLineString2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
    		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), val );
    		if ( n <= 0 ) return "";
			++cnt;
	} else if (  colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D
				|| colType2 == JAG_C_COL_TYPE_POLYGON3D || colType2 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
				// JAG_C_COL_TYPE_POLYGON is handled separately
			JagVector<JagPoint3D> vec2;
			getVectorPoints( sp2, true, vec2 );
			JagHashSetStr hash;
    		getIntersectionPoints( vec1, vec2, hash );
			Jstr hs;
    		for ( int k=0; k < vec1.size(); ++k ) {
				hs = vec1[k].hashString();
				if ( ! hash.keyExist(hs) ) {
        			val += Jstr(" ") + vec1[k].str3D(); ++cnt;
        		}
    		}
    		for ( int k=0; k < vec2.size(); ++k ) {
				hs = vec2[k].hashString();
				if ( ! hash.keyExist(hs) ) {
        			val += Jstr(" ") + vec2[k].str3D(); ++cnt;
        		}
    		}
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOINT || colType2 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		if ( 2 == dim2 ) {
			JagVector<JagPoint3D> vec2;
			getVectorPoints( sp2, false, vec2 );
			JagHashSetStr hash;
    		getIntersectionPoints( vec1, vec2, hash );
			Jstr hs;
			for ( int k=0; k < vec1.size(); ++k ) {
				hs = vec1[k].hashString();
				if ( ! hash.keyExist(hs) ) {
   				    val += Jstr(" ") + vec1[k].str2D(); ++cnt;
				}
			}
			for ( int k=0; k < vec2.size(); ++k ) {
				hs = vec2[k].hashString();
				if ( ! hash.keyExist(hs) ) {
   				    val += Jstr(" ") + vec2[k].str2D(); ++cnt;
				}
			}
		} else if ( 3 == dim2 ) {
			JagVector<JagPoint3D> vec2;
			getVectorPoints( sp2, true, vec2 );
			JagHashSetStr hash;
    		getIntersectionPoints( vec1, vec2, hash );
			Jstr hs;
			for ( int k=0; k < vec1.size(); ++k ) {
				hs = vec1[k].hashString();
				if ( ! hash.keyExist(hs) ) {
   				    val += Jstr(" ") + vec1[k].str3D(); ++cnt;
				}
			}
			for ( int k=0; k < vec2.size(); ++k ) {
				hs = vec2[k].hashString();
				if ( ! hash.keyExist(hs) ) {
   				    val += Jstr(" ") + vec2[k].str3D(); ++cnt;
				}
			}
		}
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		// result is m-linestring
		Jstr wkt1 = "linestring(";
		for ( int i=0;i<vec1.size(); ++i ) {
			if ( 0 != i ) { wkt1 += Jstr(","); }
			wkt1 += vec1[i].str2D();
		}
		wkt1 += ")";

		JagPolygon pgon2;
		int n = JagParser::addPolygonData( pgon2, sp2, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		pgon2.toWKT( false, true, "polygon", wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomSymDifference<BoostLineString2D,BoostPolygon2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), val );
		if ( n <= 0 ) return "";
		++cnt;
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		Jstr wkt1 = "linestring(";
		for ( int i=0;i<vec1.size(); ++i ) {
			if ( 0 != i ) { wkt1 += Jstr(","); }
			wkt1 += vec1[i].str2D();
		}
		wkt1 += ")";

		JagVector<JagPolygon> pgvec2;
		int n = JagParser::addMultiPolygonData( pgvec2, sp2, false, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		multiPolygonToWKT( pgvec2, false, wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomSymDifference<BoostLineString2D,BoostMultiPolygon2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), val );
		if ( n <= 0 ) return "";
		++cnt;
	}

	if ( cnt > 0 ) { return val; } else { return ""; }
}

// sp1 is m-linestring;  sp2: m-linestring/polygon
Jstr  JagGeo::doMultiLineStringSymDifference( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	Jstr value;

	int cnt = 0;
    //bool intsect;
	if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	int n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "multilinestring", wkt1 );

		JagPolygon pgon2;
		n = JagParser::addPolygonData( pgon2, sp2, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		pgon1.toWKT( false, true, "linestring", wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomSymDifference<BoostMultiLineString2D,BoostLineString2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	int n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "multilinestring", wkt1 );

		JagPolygon pgon2;
		n = JagParser::addPolygonData( pgon2, sp2, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		pgon1.toWKT( false, true, "multilinestring", wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomSymDifference<BoostMultiLineString2D,BoostMultiLineString2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	} else if (  colType2 == JAG_C_COL_TYPE_POLYGON ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	int n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "multilinestring", wkt1 );

		JagPolygon pgon2;
		n = JagParser::addPolygonData( pgon2, sp2, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		pgon1.toWKT( false, true, "polygon", wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomSymDifference<BoostMultiLineString2D,BoostPolygon2D,BoostMultiLineString2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	} else if (  colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D || colType2 == JAG_C_COL_TYPE_POLYGON3D ) {
		// hard to define
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		// hard to define
	} 

	if ( cnt > 0 ) {
		return value;
	} else {
		return "";
	}
}

// polygon intersect polygon? multi-polygon?
Jstr  JagGeo::doPolygonSymDifference( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	d("s1029 doPolygonSymDifference sp1: sp2:\n" );
	//sp1.print();
	//sp2.print();

	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	if ( 2 != dim2 ) return "";  // no 3D polygons
	Jstr value;
	int cnt = 0;
	int n;
	if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "polygon", wkt1 );

		JagPolygon pgon2;
		n = JagParser::addPolygonData( pgon2, sp2, false );
		if ( n <= 0 ) return "";
		Jstr wkt2;
		pgon2.toWKT( false, true, "polygon", wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomSymDifference<BoostPolygon2D,BoostPolygon2D,BoostMultiPolygon2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "polygon", wkt1 );

		JagVector<JagPolygon> pgvec2;
		Jstr wkt2;
		n = JagParser::addMultiPolygonData( pgvec2, sp2, false, false );
		if ( n <= 0 ) return "";
		multiPolygonToWKT( pgvec2, false, wkt2 );
		Jstr reswkt;
		JagCGAL::getTwoGeomSymDifference<BoostPolygon2D,BoostMultiPolygon2D,BoostMultiPolygon2D>( wkt1, wkt2, reswkt );
		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	}

	if ( cnt > 0 ) {
		return value;
	} else {
		return "";
	}
}

// multipolygon multipolygon
Jstr  JagGeo::doMultiPolygonSymDifference( const Jstr &colType1,const JagStrSplit &sp1,
                                               const Jstr &colType2,const JagStrSplit &sp2 )
{
	d("s1029 doMultiPolygonSymDifference sp1: sp2:\n" );
	//sp1.print();
	//sp2.print();

	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	if ( 2 != dim2 ) return "";  // no 3D polygons
	Jstr value;
	int cnt = 0;
	int n;
	if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
       	JagPolygon pgon1;  // use pgon to hold m-lines
       	n = JagParser::addPolygonData( pgon1, sp1, false );
       	if ( n <= 0 ) return "";
		Jstr wkt1;
		pgon1.toWKT( false, true, "polygon", wkt1 );

		JagVector<JagPolygon> pgvec2;
		Jstr wkt2;
		int n = JagParser::addMultiPolygonData( pgvec2, sp2, false, false );
		if ( n <= 0 ) return "";
		multiPolygonToWKT( pgvec2, false, wkt2 );
		Jstr reswkt;

		JagCGAL::getTwoGeomSymDifference<BoostMultiPolygon2D,BoostMultiPolygon2D,BoostMultiPolygon2D>( wkt1, wkt2, reswkt );

		n = JagParser::convertConstantObjToJAG(reswkt.c_str(), value );
		if ( n <= 0 ) return "";
		++cnt;
	}

	if ( cnt > 0 ) { return value; } else { return ""; }
}

// sp1 is line, linestring
// sp2 is point, point3d
Jstr  JagGeo::doLocatePoint( int srid, const Jstr &colType1,const JagStrSplit &sp1,
                             const Jstr &colType2,const JagStrSplit &sp2 )
{
	d("s1029 doPolygonSymDifference sp1: sp2:\n" );
	//sp1.print();
	//sp2.print();

	int dim1 = getDimension( colType1 );
	int dim2 = getDimension( colType2 );
	if ( dim1 != dim2 ) return "";
	if ( 2 != dim2 ) return "";  // no 3D polygons

	if ( colType1 != JAG_C_COL_TYPE_LINE && colType1 != JAG_C_COL_TYPE_LINE3D 
	     && colType1 != JAG_C_COL_TYPE_LINESTRING && colType1 != JAG_C_COL_TYPE_LINESTRING3D) { return ""; }

	if ( colType2 != JAG_C_COL_TYPE_POINT && colType2 != JAG_C_COL_TYPE_POINT3D ) { return ""; }

	double pz = 0.0;
	double px = sp2[JAG_SP_START+0].tof();
	double py = sp2[JAG_SP_START+1].tof(); 
	if ( 3 == dim2 ) pz = sp2[JAG_SP_START+2].tof();

	double projx, projy, projz, d2;
	if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		double x1 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
		double x2 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
		double y2 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
		minPoint2DToLineSegDistance( px, py, x1, y1, x2, y2, srid, projx, projy );
		d2 = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
		if ( jagEQ(d2, 0.0) ) return "0.0";
		return d2s( ((projx-x1)*(projx-x1) + (projy-y1)*(projy-y1) ) / d2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		double x1 = jagatof( sp1[JAG_SP_START+0].c_str() ); 
		double y1 = jagatof( sp1[JAG_SP_START+1].c_str() ); 
		double z1 = jagatof( sp1[JAG_SP_START+2].c_str() ); 
		double x2 = jagatof( sp1[JAG_SP_START+3].c_str() ); 
		double y2 = jagatof( sp1[JAG_SP_START+4].c_str() ); 
		double z2 = jagatof( sp1[JAG_SP_START+5].c_str() ); 
		minPoint3DToLineSegDistance( px, py, pz, x1, y1, z1, x2, y2, z2, srid, projx, projy, projz );
		d2 = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + ( z2-z1)*(z2-z1);
		if ( jagEQ(d2, 0.0) ) return "0.0";
		return d2s( ((projx-x1)*(projx-x1) + (projy-y1)*(projy-y1) + (projz-z1)*(projz-z1) ) / d2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		double mindist, minx, miny;
		double frac = getMinDist2DPointFraction(px,py, srid, sp1, mindist, minx, miny );
		return d2s( frac );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		double mindist, minx, miny, minz;
		double frac = getMinDist3DPointFraction(px,py,pz, srid, sp1, mindist, minx, miny, minz );
		return d2s( frac );
	}

	return "";
}

double JagGeo::getMinDist2DPointFraction( double px, double py, int srid, const JagStrSplit &sp, 
										  double &mindist, double &minx, double &miny )
{
	if ( sp.length() < 2 ) return 0.0;
	mindist = JAG_LONG_MAX;
	double d, projx, projy;
	double x1, y1, x2, y2;
	double sum = 0.0;
	double minpointsum;
	const char *str; char *p;
	for ( int i=JAG_SP_START; i < sp.length() -1; ++i ) {
		str = sp[i].c_str();
		if ( strchrnum( str, ':' ) < 1 ) continue;
       	get2double(str, p, ':', x1, y1 );
		str = sp[i+1].c_str();
		if ( strchrnum( str, ':' ) < 1 ) continue;
       	get2double(str, p, ':', x2, y2 );
		d = minPoint2DToLineSegDistance( px, py, x1, y1, x2, y2, srid, projx, projy );
		if ( d < mindist ) {
			minpointsum = sum + distance( x1, y1, projx, projy, srid );
			mindist = d;
			minx = projx; miny = projy;
		} 
		sum += distance( x1, y1, x2, y2, srid );
	}
	if ( jagEQ(sum, 0.0) ) return 0.0;
	return minpointsum/sum;
}

void JagGeo::getMinDist2DPointOnPloygonAsLineStrings( int srid, double px, double py, const JagPolygon &pgon, double &minx, double &miny )
{
	minx = miny = 0.0;
	if ( pgon.size() < 2 ) return;
	double mindist = JAG_LONG_MAX;
	double d, projx, projy;

	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &lstr = pgon.linestr[i];
		for ( int j=0; j < lstr.size() -1; ++j ) {
			d = minPoint2DToLineSegDistance( px, py, lstr.point[j].x, lstr.point[j].y,  lstr.point[j+1].x, lstr.point[j+1].y, 
											srid, projx, projy );
			if ( d < mindist ) {
				mindist = d;
				minx = projx; miny = projy;
			}
		}
	}
}

void JagGeo::getMinDist3DPointOnPloygonAsLineStrings( int srid, double px, double py, double pz, const JagPolygon &pgon, 
								double &minx, double &miny, double &minz )
{
	minx = miny = minz = 0.0;
	if ( pgon.size() < 2 ) return;
	double mindist = JAG_LONG_MAX;
	double d, projx, projy, projz;

	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &lstr = pgon.linestr[i];
		for ( int j=0; j < lstr.size() -1; ++j ) {
			d = minPoint3DToLineSegDistance( px, py, pz, lstr.point[j].x, lstr.point[j].y, lstr.point[j].z,
											lstr.point[j+1].x, lstr.point[j+1].y,  lstr.point[j+1].z,
											srid, projx, projy, projz );
			if ( d < mindist ) {
				mindist = d;
				minx = projx; miny = projy; minz = projz;
			}
		}
	}
}

double JagGeo::getMinDist3DPointFraction( double px, double py, double pz, int srid, const JagStrSplit &sp, 
									  double &mindist, double &minx, double &miny, double &minz )
{
	if ( sp.length() < 2 ) return 0.0;
	mindist = JAG_LONG_MAX;
	double d, projx, projy, projz;
	double x1, y1, z1, x2, y2, z2;
	double sum = 0.0;
	double minpointsum;
	const char *str; 
	char *p;

	for ( int i=JAG_SP_START; i < sp.length() -1; ++i ) {
		str = sp[i].c_str();
		if ( strchrnum( str, ':' ) < 2 ) continue;
       	get3double(str, p, ':', x1, y1, z1 );
		str = sp[i+1].c_str();
		if ( strchrnum( str, ':' ) < 2 ) continue;
       	get3double(str, p, ':', x2, y2, z2 );
		d = minPoint3DToLineSegDistance( px, py, pz, x1, y1, z1, x2, y2, z2, srid, projx, projy, projz );
		if ( d < mindist ) {
			minpointsum = sum + distance( x1, y1, z1, projx, projy, projz, srid );
			mindist = d;
			minx = projx; miny = projy; minz = projz;
		} 
		sum += distance( x1, y1, z1, x2, y2, z2, srid );
	}
	if ( jagEQ(sum, 0.0) ) return 0.0;
	return minpointsum/sum;
}


// sp: linestring/m-linestring/polygon/m-polygon
bool JagGeo::matchPoint2D( double px, double py, const JagStrSplit &sp, Jstr &xs, Jstr &ys )
{
	double x, y;
	const char *str; char *p;
	bool rc = false;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		if ( sp[i] == "|" || sp[i] == "!" ) continue;
		str = sp[i].c_str();
		if ( strchrnum( str, ':' ) < 1 ) continue;
       	get2double(str, p, ':', x, y );
		if ( jagEQ(px,x) && jagEQ(py,y) ) {
			JagStrSplit ss(sp[i], ':');
			xs = ss[0]; ys = ss[1];
			rc = true;
			break;
		}
	}

	return rc;
}

// sp: linestring/m-linestring/polygon/m-polygon
bool JagGeo::matchPoint3D( double px, double py, double pz, const JagStrSplit &sp, 
							Jstr &xs, Jstr &ys, Jstr &zs )
{
	double x, y, z;
	const char *str; char *p;
	bool rc = false;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		if ( sp[i] == "|" || sp[i] == "!" ) continue;
		str = sp[i].c_str();
		if ( strchrnum( str, ':' ) < 2 ) continue;
    	get3double(str, p, ':', x, y, z );
		if ( jagEQ(px,x) && jagEQ(py,y) && jagEQ(pz,z) ) {
			JagStrSplit ss(sp[i], ':');
			xs = ss[0]; ys = ss[1]; zs = ss[2];
			rc = true;
			break;
		}
	}
	return rc;
}

void JagGeo::nonMatchPoint2DVec( double px, double py, const JagStrSplit &sp, Jstr &vecs )
{
	double x, y;
	const char *str; char *p;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		if ( sp[i] == "|" || sp[i] == "!" ) continue;
		str = sp[i].c_str();
		if ( strchrnum( str, ':' ) < 1 ) continue;
       	get2double(str, p, ':', x, y );
		if ( ! (jagEQ(px,x) && jagEQ(py,y) ) ) {
			vecs += Jstr(" ") + sp[i];
		}
	}
}

void JagGeo::nonMatchPoint3DVec( double px, double py, double pz, const JagStrSplit &sp, Jstr &vecs )
{
	double x, y, z;
	const char *str; char *p;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		if ( sp[i] == "|" || sp[i] == "!" ) continue;
		str = sp[i].c_str();
		if ( strchrnum( str, ':' ) < 2 ) continue;
       	get3double(str, p, ':', x, y, z );
		if ( ! (jagEQ(px,x) && jagEQ(py,y) && jagEQ(pz,z) ) ) {
			vecs += Jstr(" ") + sp[i];
		}
	}
}

void JagGeo::nonMatchTwoPoint2DVec( double px1, double py1, double px2, double py2, const JagStrSplit &sp, Jstr &vecs )
{
	double x, y;
	const char *str; char *p;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		if ( sp[i] == "|" || sp[i] == "!" ) continue;
		str = sp[i].c_str();
		if ( strchrnum( str, ':' ) < 1 ) continue;
       	get2double(str, p, ':', x, y );
		if ( ! (jagEQ(px1,x) && jagEQ(py1,y) ) && ! ( jagEQ(px2,x) && jagEQ(py2,y)  ) ) {
			vecs += Jstr(" ") + sp[i];
		}
	}
}

void JagGeo::nonMatchTwoPoint3DVec( double px1, double py1, double pz1, double px2, double py2, double pz2, const JagStrSplit &sp, Jstr &vecs )
{
	double x, y, z;
	const char *str; char *p;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		if ( sp[i] == "|" || sp[i] == "!" ) continue;
		str = sp[i].c_str();
		if ( strchrnum( str, ':' ) < 2 ) continue;
       	get3double(str, p, ':', x, y, z );
		if ( ! (jagEQ(px1,x) && jagEQ(py1,y) && jagEQ(pz1,z) ) && ! ( jagEQ(px2,x) && jagEQ(py2,y) && jagEQ(pz2,z) ) ) {
			vecs += Jstr(" ") + sp[i];
		}
	}
}

// sp is of linestring or multi-linestring
bool  JagGeo::line2DLineStringIntersection( const JagLine2D &line1, const JagStrSplit &sp, JagVector<JagPoint2D> &vec )
{
	JagPolygon pgon;
	int n = JagParser::addPolygonData( pgon, sp, false );
	if ( n <= 0 ) return false;
	int cnt = 0;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &lstr = pgon.linestr[i];
		for ( int j=0; j < lstr.size() -1; ++j ) {
			JagLine2D line2(lstr.point[j].x, lstr.point[j].y,  lstr.point[j+1].x, lstr.point[j+1].y );
			if ( JagCGAL::hasIntersection( line1, line2, vec ) ) {
				cnt += vec.size();
			}
		}
	}
	if ( cnt < 1 ) return false;
	return true;
}

// sp is of linestring or multi-linestring
bool  JagGeo::line3DLineStringIntersection( const JagLine3D &line1, const JagStrSplit &sp, JagVector<JagPoint3D> &vec )
{
	JagPolygon pgon;
	int n = JagParser::addPolygon3DData( pgon, sp, false );
	if ( n <= 0 ) return false;
	int cnt = 0;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &lstr = pgon.linestr[i];
		for ( int j=0; j < lstr.size() -1; ++j ) {
			JagLine3D line2( lstr.point[j].x, lstr.point[j].y, lstr.point[j].z, 
							 lstr.point[j+1].x, lstr.point[j+1].y, lstr.point[j+1].z );
			if ( JagCGAL::hasIntersection( line1, line2, vec ) ) {
				cnt += vec.size();
			}
		}
	}

	if ( cnt < 1 ) return false;
	return true;
}

void JagGeo::appendLine2DLine2DIntersection(double x1, double y1, double x2, double y2,
                                            double mx1, double my1, double mx2, double my2, JagVector<JagPoint2D> &vec )
{
	JagLine2D line1(x1,y1,x2,y2);
	JagLine2D line2(mx1,my1,mx2,my2);
	JagCGAL::hasIntersection( line1, line2, vec );
}

void JagGeo::appendLine3DLine3DIntersection(double x1, double y1, double z1, double x2, double y2, double z2,
                                            double mx1, double my1, double mz1, double mx2, double my2, double mz2, 
											JagVector<JagPoint3D> &vec )
{
	JagLine3D line1(x1,y1,z1, x2,y2,z2);
	JagLine3D line2(mx1,my1,mz1, mx2,my2,mz2);
	JagCGAL::hasIntersection( line1, line2, vec );
}

void JagGeo::splitPolygonToVector( const JagPolygon &pgon, bool is3D, JagVector<Jstr> &svec )
{
	for ( int i=0; i < pgon.size(); ++i ) {
		Jstr str = "CJAG=0=0=XX=0 0:0:0:0";
		const JagLineString3D &lstr = pgon.linestr[i];
		for ( int j=0; j< lstr.size(); ++j ) {
			if ( is3D ) {
				str += Jstr(" ") + lstr.point[j].str3D();
			} else {
				str += Jstr(" ") + lstr.point[j].str2D();
			}
			svec.append( str );
		}
	}
}


//get common points
void JagGeo::getIntersectionPoints( const JagVector<JagPoint3D> &vec1, const JagVector<JagPoint3D> &vec2,
                                    JagVector<JagPoint3D> &resvec )
{
	JagPoint3D *arr1 = new JagPoint3D[vec1.size()];
	JagPoint3D *arr2 = new JagPoint3D[vec2.size()];
	for ( int i=0; i< vec1.size(); ++i ) { arr1[i] = vec1[i]; }
	for ( int i=0; i< vec2.size(); ++i ) { arr2[i] = vec2[i]; }
	JagSetJoin<JagPoint3D>( arr1, vec1.size(), arr2, vec2.size(), resvec );
	delete [] arr1;
	delete [] arr2;
}


void JagGeo::getIntersectionPoints( const JagVector<JagPoint3D> &vec1, const JagVector<JagPoint3D> &vec2, JagHashSetStr &hashset )
{
	JagVector<JagPoint3D> resvec;
	getIntersectionPoints( vec1, vec2, resvec ); 
	vectorToHash( resvec, hashset );
}

void JagGeo::getIntersectionPoints( const JagVector<JagPoint3D> &vec1, const JagStrSplit &sp, 
                                    bool is3D, JagHashSetStr &hashset )
{
	JagVector<JagPoint3D> resvec;
	getIntersectionPoints( vec1, sp, is3D, resvec ); 
	vectorToHash( resvec, hashset );
}

void JagGeo::getIntersectionPoints( const JagVector<JagPoint3D> &vec1, const JagStrSplit &sp, 
                                    bool is3D, JagVector<JagPoint3D> &resvec )
{
	//double x,y,z;
	//const char *str; char *p;
	JagPoint3D *arr1 = new JagPoint3D[vec1.size()];
	for ( int i=0; i< vec1.size(); ++i ) { arr1[i] = vec1[i]; }
	JagVector<JagPoint3D>  vec2;
	getVectorPoints( sp, is3D, vec2 );
	JagPoint3D *arr2 = new JagPoint3D[vec2.size()];
	for ( int i=0; i< vec2.size(); ++i ) { arr2[i] = vec2[i]; }
	JagSetJoin<JagPoint3D>( arr1, vec1.size(), arr2, vec2.size(), resvec );
	delete [] arr1;
	delete [] arr2;
}

void JagGeo::vectorToHash( const JagVector<JagPoint3D> &resvec, JagHashSetStr &hashset )
{
	//char buf[32];
	Jstr s;
	for ( int i=0; i < resvec.size(); ++i ) {
		s = resvec[i].hashString();
		hashset.addKey( s );
	}
}


//get points to a vector
void JagGeo::getVectorPoints( const JagStrSplit &sp, bool is3D, JagVector<JagPoint3D> &vec )
{
	double x,y,z;
	const char *str; char *p;
	for ( int i=JAG_SP_START; i < sp.size(); ++i ) { 
		if (  sp[i] == "!" ||  sp[i] == "|" ) continue;
       	str = sp[i].c_str();
		if ( is3D ) {
        	if ( strchrnum( str, ':') < 2 ) continue;
        	get3double(str, p, ':', x, y, z );
    		vec.append( JagPoint3D(x,y,z) );
		} else {
        	if ( strchrnum( str, ':') < 1 ) continue;
        	get2double(str, p, ':', x, y );
    		vec.append( JagPoint3D(x,y,0) );
		}
	}
}

// JAG_GEO_WGS84
bool JagGeo::interpolatePoint2D(double segdist, double segfrac, const JagPoint3D &p1, const JagPoint3D &p2,
                                JagPoint3D &point )
{
	const Geodesic& geod = Geodesic::WGS84();

	//GeodesicLine invline =  geod.InverseLine(real lat1, real lon1, real lat2, real lon2, unsigned caps=ALL) const
	GeodesicLine invline = geod.InverseLine(p1.y, p1.x, p2.y, p2.x, GeodesicLine::DISTANCE_IN );

	//invline.Position (real s12, real &lat2, real &lon2) const
	double s12 = segdist * segfrac;
	double lat2, lon2;
	if ( Math::NaN() != invline.Position (s12, lat2, lon2) ) {
		point.x = lon2;
		point.y = lat2;
		return true;
	} else {
		return false;
	}

}

bool JagGeo::toJAG( const JagVector<JagPolygon> &pgvec, bool is3D, bool hasHdr, const Jstr &inbbox, int srid, Jstr &str ) 
{
	if ( pgvec.size() < 1 ) { str=""; return false; }
	if ( hasHdr ) {
		Jstr srids = intToStr( srid );
		Jstr bbox;
		Jstr mk = "OJAG=";
		if ( is3D ) {
			if ( inbbox.size() < 1 )  { bbox = "0:0:0:0:0:0"; mk="CJAG="; } else { bbox = inbbox; }
			str = mk + srids + "=0=MG3=d " + bbox;
		} else {
			if ( inbbox.size() < 1 )  { bbox = "0:0:0:0"; mk="CJAG="; } else { bbox = inbbox; }
			str = mk + srids + "=0=MG=d " + bbox;
		}
	} 

	for ( int k=0; k < pgvec.size(); ++k ) {
    	if ( k>0 ) { str += " !"; }
		const JagPolygon &pgon = pgvec[k];
    	for ( int i=0; i < pgon.linestr.size(); ++i ) {
    		if ( i>0 ) { str += " |"; }
    		const JagLineString3D &lstr = pgon.linestr[i];
    		for (  int j=0; j< lstr.size(); ++j ) {
    			str += Jstr(" ") + d2s(lstr.point[j].x) + ":" +  d2s(lstr.point[j].y);
    			if ( is3D ) { str += Jstr(":") + d2s(lstr.point[j].z); }
    		}
    	}
	}

	return true;
	
}

void JagGeo::multiPolygonToVector2D( int srid, const JagVector<JagPolygon> &pgvec, bool outerRingOnly, JagVector<JagPoint2D> &vec )
{
	if ( pgvec.size() < 1 ) return;
	double x, y;
	for ( int k=0; k < pgvec.size(); ++k ) {
		const JagPolygon &pgon = pgvec[k];
    	for ( int i=0; i < pgon.linestr.size(); ++i ) {
    		const JagLineString3D &lstr = pgon.linestr[i];
    		for (  int j=0; j< lstr.size(); ++j ) {
				JagGeo::lonLatToXY( srid, lstr.point[j].x, lstr.point[j].y, x, y );
    			vec.append(JagPoint2D(x,y));
    		}
			if ( outerRingOnly ) break;
    	}
	}
}

void JagGeo::multiPolygonToVector3D( int srid, const JagVector<JagPolygon> &pgvec, bool outerRingOnly, JagVector<JagPoint3D> &vec )
{
	if ( pgvec.size() < 1 ) return;
	double x, y, z;
	for ( int k=0; k < pgvec.size(); ++k ) {
		const JagPolygon &pgon = pgvec[k];
    	for ( int i=0; i < pgon.linestr.size(); ++i ) {
    		const JagLineString3D &lstr = pgon.linestr[i];
    		for (  int j=0; j< lstr.size(); ++j ) {
				JagGeo::lonLatAltToXYZ( srid, lstr.point[j].x, lstr.point[j].y, lstr.point[j].z, x, y, z );
    			vec.append(JagPoint3D(x,y,z));
    		}
			if ( outerRingOnly ) break;
    	}
	}
}

// XYZ in meters
void JagGeo::lonLatAltToXYZ( int srid, double lon, double lat, double alt, double &x, double &y, double &z )
{
	if ( 0 == srid ) {
		x = lon; y = lat; z = alt;
		return;
	}

	const GeographicLib::Geocentric& earth = Geocentric::WGS84();
	//LocalCartesian proj(lat0, lon0, 0, earth);
	GeographicLib::LocalCartesian proj(0.0, 0.0, 0.0, earth);
	proj.Forward(lat, lon, alt, x, y, z);
}

// Alt in meters above sea-level
void JagGeo::XYZToLonLatAlt( int srid, double x, double y, double z, double &lon, double &lat, double &alt )
{
	if ( 0 == srid ) {
		lon = x; lat = y;  alt = z;
		return;
	}

	const GeographicLib::Geocentric& earth = Geocentric::WGS84();
	GeographicLib::LocalCartesian proj(0.0, 0.0, 0.0, earth);
	proj.Reverse(x,y,z, lat, lon, alt );
}

// lat lon in degrees
void JagGeo::lonLatToXY( int srid, double lon, double lat, double &x, double &y )
{
	if ( 0 == srid ) {
		x = lon; y = lat;
		return;
	}

	x = JAG_METER_MAX_PER_LON_DEGREE * lon * cos(lat*JAG_RADIAN_PER_DEGREE) ;
	y = JAG_METER_PER_LAT_DEGREE * lat;

	/***
	const Geodesic& geod = Geodesic::WGS84();
	double s12;
	geod.Inverse( lat, 0.0, lat, lon, s12 );
	x = s12;
	if ( lon < 0.0 ) x = -s12; else x= s12;

	geod.Inverse( 0.0, lon, lat, lon, s12 );
	y = s12;
	if ( lat < 0.0 ) y = -s12; else y= s12;
	****/
}

void JagGeo::XYToLonLat( int srid, double x, double y, double &lon, double &lat )
{
	if ( 0 == srid ) {
		lon = x; lat = y;
		return;
	}

	lat = y/JAG_METER_PER_LAT_DEGREE;
	double cosv = cos(lat*JAG_RADIAN_PER_DEGREE);
	if ( jagLE(fabs(cosv), JAG_ZERO) ) {
		lon = 180.0;
	} else {
		lon = x/(JAG_METER_MAX_PER_LON_DEGREE*cosv);
	}
	 
}

void JagGeo::kNN2D( int srid, const std::vector<JagSimplePoint2D> &points, const JagSimplePoint2D &point, int K,
                    const JagMinMaxDistance &minmax, std::vector<JagSimplePoint2D> &neighbors )
{
	if ( JAG_GEO_WGS84 == srid ) {
		kNN2DWGS84(points, point, K, minmax, neighbors ); 
	} else {
		kNN2DCart(points, point, K, minmax, neighbors ); 
	}
}

void JagGeo::kNN3D( int srid, const std::vector<JagSimplePoint3D> &points, const JagSimplePoint3D &point, int K,
                    const JagMinMaxDistance &minmax, std::vector<JagSimplePoint3D> &neighbors )
{
	if ( JAG_GEO_WGS84 == srid ) {
		kNN3DWGS84(points, point, K, minmax, neighbors ); 
	} else {
		kNN3DCart(points, point, K, minmax, neighbors ); 
	}
}


void JagGeo::kNN2DCart(const std::vector<JagSimplePoint2D> &points, const JagSimplePoint2D &point, int K,
                        const JagMinMaxDistance &minmax, std::vector<JagSimplePoint2D> &neighbors )
{
	std::vector<int> kvec;
	DistanceCalculator2DCart distCalc;
	NearestNeighbor<double, JagSimplePoint2D, DistanceCalculator2DCart> pointset;
	pointset.Initialize( points, distCalc );
	pointset.Search(points, distCalc, point, kvec, K, minmax.max, minmax.min );
	for ( int i=0; i < kvec.size(); ++i ) {
		neighbors.push_back( JagSimplePoint2D(points[kvec[i]].x, points[kvec[i]].y) );
	}
}

void JagGeo::kNN2DWGS84(const std::vector<JagSimplePoint2D> &points, const JagSimplePoint2D &point, int K,
                        const JagMinMaxDistance &minmax, std::vector<JagSimplePoint2D> &neighbors )
{
	std::vector<int> kvec;
	DistanceCalculator2DWGS84 distCalc;
	NearestNeighbor<double, JagSimplePoint2D, DistanceCalculator2DWGS84> pointset;
	pointset.Initialize( points, distCalc );
	pointset.Search(points, distCalc, point, kvec, K, minmax.max, minmax.min );
	for ( int i=0; i < kvec.size(); ++i ) {
		neighbors.push_back( JagSimplePoint2D(points[kvec[i]].x, points[kvec[i]].y) );
	}
}

void JagGeo::kNN3DCart(const std::vector<JagSimplePoint3D> &points, const JagSimplePoint3D &point, int K,
                        const JagMinMaxDistance &minmax, std::vector<JagSimplePoint3D> &neighbors )
{
	std::vector<int> kvec;
	DistanceCalculator3DCart distCalc;
	NearestNeighbor<double, JagSimplePoint3D, DistanceCalculator3DCart> pointset;
	pointset.Initialize( points, distCalc );
	pointset.Search(points, distCalc, point, kvec, K, minmax.max, minmax.min );
	for ( int i=0; i < kvec.size(); ++i ) {
		neighbors.push_back( JagSimplePoint3D(points[kvec[i]].x, points[kvec[i]].y, points[kvec[i]].z ) );
	}
}

void JagGeo::kNN3DWGS84(const std::vector<JagSimplePoint3D> &points, const JagSimplePoint3D &point, int K,
                        const JagMinMaxDistance &minmax, std::vector<JagSimplePoint3D> &neighbors )
{
	std::vector<int> kvec;
	DistanceCalculator3DWGS84 distCalc;
	NearestNeighbor<double, JagSimplePoint3D, DistanceCalculator3DWGS84> pointset;
	pointset.Initialize( points, distCalc );
	pointset.Search(points, distCalc, point, kvec, K, minmax.max, minmax.min );
	for ( int i=0; i < kvec.size(); ++i ) {
		neighbors.push_back( JagSimplePoint3D(points[kvec[i]].x, points[kvec[i]].y, points[kvec[i]].z ) );
	}
}

void JagGeo::knnfromVec( const JagVector<JagPolygon> &pgvec, int dim, int srid, double px,double py,double pz,
                         int K, double min, double max, Jstr &value )
{
	JagMinMaxDistance minmax(min,max);
	if ( 2 == dim ) {
		std::vector<JagSimplePoint2D> points;

		for ( int k=0; k<pgvec.size(); ++k ) {
			const JagPolygon &pgon = pgvec[k];
    		for (int i=0; i < pgon.linestr.size(); ++i ) {
    			const JagLineString3D &lstr = pgon.linestr[i];
    			for ( int j=0; j < lstr.size(); ++j ) {
    				points.push_back( JagSimplePoint2D(lstr.point[j].x, lstr.point[j].y) );
    			}
    		}
		}

		JagSimplePoint2D point(px,py);
		std::vector<JagSimplePoint2D> neighb;
		JagGeo::kNN2D(srid, points, point, K, minmax, neighb);
		for (int i=0; i < neighb.size(); ++i ) {
			if ( 0 == i ) {
				value += d2s( neighb[i].x ) + ":" + d2s( neighb[i].y );
			} else {
				value += Jstr(" ") + d2s( neighb[i].x ) + ":" + d2s( neighb[i].y );
			}
		}

	} else {
		std::vector<JagSimplePoint3D> points;

		for ( int k=0; k<pgvec.size(); ++k ) {
			const JagPolygon &pgon = pgvec[k];
    		for (int i=0; i < pgon.linestr.size(); ++i ) {
    			const JagLineString3D &lstr = pgon.linestr[i];
    			for ( int j=0; j < lstr.size(); ++j ) {
    				points.push_back( JagSimplePoint3D(lstr.point[j].x, lstr.point[j].y, lstr.point[j].z) );
    			}
    		}
		}

		JagSimplePoint3D point(px,py,pz);
		std::vector<JagSimplePoint3D> neighb;
		JagGeo::kNN3D(srid, points, point, K, minmax, neighb);
		for (int i=0; i < neighb.size(); ++i ) {
			if ( 0 == i ) {
				value += d2s( neighb[i].x ) + ":" + d2s( neighb[i].y ) + ":" + d2s( neighb[i].z );
			} else {
				value += Jstr(" ") + d2s( neighb[i].x ) + ":" + d2s( neighb[i].y ) + ":" + d2s( neighb[i].z );
			}
		}
	}

}


