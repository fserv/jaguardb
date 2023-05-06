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
#ifndef _jag_geom_h_
#define _jag_geom_h_

#include <GeographicLib/Gnomonic.hpp>
#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/Geocentric.hpp>
#include <GeographicLib/PolygonArea.hpp>
#include <GeographicLib/Constants.hpp>
#include <GeographicLib/LocalCartesian.hpp>
#include <GeographicLib/NearestNeighbor.hpp>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <JagDef.h>
#include <JagVector.h>
#include <JagUtil.h>
#include <JagShape.h>
#include <JagHashSetStr.h>
//#include <JaguarCPPClient.h>
#include <JagParser.h>
#include <JagCGAL.h>
using namespace GeographicLib;



class JagGeo
{
  public:

	/////////////////////////////////////// Within function ////////////////////////////////////////////////////
   	static bool doPointWithin(  const JagStrSplit &sp1, const Jstr &mk2, 
								const Jstr &colType2, int srid2, 
								const JagStrSplit &sp2, bool strict=true );
   	static bool doPoint3DWithin( const JagStrSplit &sp1, const Jstr &mk2, 
								const Jstr &colType2, int srid2, 
								const JagStrSplit &sp2, bool strict=true );
   	static bool doTriangleWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2,  bool strict=true );
   	static bool doTriangle3DWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2,  bool strict=true );
   	static bool doCircleWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2,  bool strict=true );
   	static bool doCircle3DWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2,  bool strict=true );
   	static bool doSphereWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doBoxWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doRectangleWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doRectangle3DWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doSquareWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doSquare3DWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									  int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doCubeWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									 int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doCylinderWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									 int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doConeWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doEllipseWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doEllipsoidWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doLineWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doLine3DWithin(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doLineStringWithin(  const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doLineString3DWithin( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									  const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doPolygonWithin(  const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doPolygon3DWithin( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									  const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doMultiPolygonWithin(  const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doMultiPolygon3DWithin(  const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );


	// 2D point
	static bool pointWithinPoint( double px, double py, double x1, double y1, bool strict );
	static bool pointWithinLine( double px, double py, double x1, double y1, double x2, double y2, bool strict );
	static bool pointWithinLineString(  double x, double y, const Jstr &mk2, const JagStrSplit &sp2, bool strict );
	//static bool pointWithinSquare( double px, double py, double x0, double y0, double r, double nx, bool strict );
	static bool pointWithinCircle( double px, double py, double x0, double y0, double r, bool strict );
	static bool pointWithinRectangle( double px, double py, double x, double y, double a, double b, double nx, bool strict );
	static bool pointWithinEllipse( double px, double py, double x, double y, double a, double b, double nx, bool strict );
	static bool pointWithinTriangle( const JagPoint2D &point, 
									  const JagPoint2D &point1, const JagPoint2D &point2,
					 				  const JagPoint2D &point3, bool strict = false, bool boundcheck = true );
	static bool pointInTriangle( double px, double py, double x1, double y1,
									  double x2, double y2, double x3, double y3,
					 				  bool strict = false, bool boundcheck = true );
	static bool pointWithinPolygon( double x, double y, const Jstr &mk2, const JagStrSplit &sp2, bool strict );
	static bool pointWithinPolygon( double x, double y, const JagLineString3D &linestr );
	static bool pointWithinPolygon( double x, double y, const JagPolygon &pgon );

	// 3D point
	static bool point3DWithinPoint3D( double px, double py, double pz, double x1, double y1, double z1, bool strict );
	static bool point3DWithinLine3D( double px, double py, double pz, double x1, double y1, double z1, 
									double x2, double y2, double z2, bool strict );
	static bool point3DWithinLineString3D(  double x, double y, double z, const Jstr &mk2, const JagStrSplit &sp2, bool strict );
	static bool point3DWithinBox( double px, double py, double pz,  
									  double x, double y, double z, double a, double b, double c, double nx, double ny, 
									  bool strict );
    static bool point3DWithinSphere( double px, double py, double pz, double x, double y, double z, double r, bool strict );
	static bool point3DWithinEllipsoid( double px, double py, double pz,  
									  double x, double y, double z, double a, double b, double c, double nx, double ny, 
									  bool strict );

	static bool point3DWithinCone( double px, double py, double pz, 
									double x0, double y0, double z0,
									 double r, double h,  double nx, double ny, bool strict );
	static bool point3DWithinSquare3D( double px, double py, double pz, 
									double x0, double y0, double z0,
									 double a, double nx, double ny, bool strict );
	static bool point3DWithinCylinder(  double x, double y, double z,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );
	static bool point3DWithinNormalCylinder(  double x, double y, double z,
                                    double r, double h, bool strict );

	// 2D circle
	static bool circleWithinCircle( double px, double py, double pr, double x, double y, double r, bool strict );
	static bool circleWithinSquare( double px0, double py0, double pr, double x0, double y0, double r, double nx, bool strict );
	static bool circleWithinEllipse( double px, double py, double pr, 
									 	 double x, double y, double w, double h, double nx, 
									 	 bool strict=true, bool bound=true );
	static bool circleWithinRectangle( double px0, double py0, double pr, double x0, double y0,
										  double w, double h, double nx,  bool strict );
	static bool circleWithinTriangle( double px, double py, double pr, double x1, double y1, 
									  double x2, double y2, double x3, double y3, bool strict=true, bool bound = true );
	static bool circleWithinPolygon( double px0, double py0, double pr, 
									 const Jstr &mk2, const JagStrSplit &sp2, bool strict );

	// 3D circle
	static bool circle3DWithinCube( double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
										double x0, double y0, double z0,  double r, double nx, double ny, bool strict );

	static bool circle3DWithinBox( double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
					                   double x0, double y0, double z0,  double a, double b, double c,
					                   double nx, double ny, bool strict );
   	static bool circle3DWithinSphere( double px0, double py0, double pz0, double pr0,   double nx0, double ny0,
	   									 double x, double y, double z, double r, bool strict );
	static bool circle3DWithinEllipsoid( double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
										  double x0, double y0, double z0, 
									 	   double w, double d, double h, double nx, double ny, bool strict );
	static bool circle3DWithinCone( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
										  double x0, double y0, double z0, 
									 	   double r, double h, double nx, double ny, bool strict );

	// 3D sphere
	static bool sphereWithinCube(  double px0, double py0, double pz0, double pr0,
		                               double x0, double y0, double z0, double r, double nx, double ny, bool strict );
	static bool sphereWithinBox(  double px0, double py0, double pz0, double r,
		                                double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, bool strict );
	static bool sphereWithinSphere(  double px, double py, double pz, double pr, 
										double x, double y, double z, double r, bool strict );
	static bool sphereWithinEllipsoid(  double px0, double py0, double pz0, double pr,
		                                    double x0, double y0, double z0, double w, double d, double h, 
											double nx, double ny, bool strict );
	static bool sphereWithinCone(  double px0, double py0, double pz0, double pr,
		                                    double x0, double y0, double z0, double r, double h, 
											double nx, double ny, bool strict );

	// 2D rectangle
	static bool rectangleWithinTriangle( double px0, double py0, double a0, double b0, double nx0, 
										 double x1, double y1, double x2, double y2, double x3, double y3, bool strict );
	static bool rectangleWithinSquare( double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double r, double nx, bool strict );
	static bool rectangleWithinRectangle( double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool rectangleWithinEllipse( double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, bool strict );
 	static bool rectangleWithinCircle( double px0, double py0, double a0, double b0, double nx0, 
									    double x0, double y0, double r, double nx, bool strict );
 	static bool rectangleWithinPolygon( double px0, double py0, double a0, double b0, double nx0, 
										const Jstr &mk2, const JagStrSplit &sp2, bool strict );

	// 2D triangle
	static bool triangleWithinTriangle( double x10, double y10, double x20, double y20, double x30, double y30,
										 double x1, double y1, double x2, double y2, double x3, double y3, bool strict );
	 /***
	static bool triangleWithinSquare( double x10, double y10, double x20, double y20, double x30, double y30,
		                                double x0, double y0, double r, double nx, bool strict );
	***/
	static bool triangleWithinRectangle( double x10, double y10, double x20, double y20, double x30, double y30,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool triangleWithinEllipse( double x10, double y10, double x20, double y20, double x30, double y30,
		                                double x0, double y0, double a, double b, double nx, bool strict );
 	static bool triangleWithinCircle( double x10, double y10, double x20, double y20, double x30, double y30,
									    double x0, double y0, double r, double nx, bool strict );
 	static bool triangleWithinPolygon( double x10, double y10, double x20, double y20, double x30, double y30,
									    const Jstr &mk2, const JagStrSplit &sp2, bool strict );
										
	// 2D ellipse
	static bool ellipseWithinTriangle( double px0, double py0, double a0, double b0, double nx0, 
										 double x1, double y1, double x2, double y2, double x3, double y3, bool strict );
	static bool ellipseWithinSquare( double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double r, double nx, bool strict );
	static bool ellipseWithinRectangle( double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool ellipseWithinEllipse( double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, bool strict );
 	static bool ellipseWithinCircle( double px0, double py0, double a0, double b0, double nx0, 
									    double x0, double y0, double r, double nx, bool strict );
 	static bool ellipseWithinPolygon( double px0, double py0, double a0, double b0, double nx0, 
									    const Jstr &mk2, const JagStrSplit &sp2, bool strict );

	// rect 3D
	static bool rectangle3DWithinCube(  double px0, double py0, double pz0, double a0, double b0, double nx0, double ny0,
                                double x0, double y0, double z0, double r, double nx, double ny, bool strict );

	static bool rectangle3DWithinBox(  double px0, double py0, double pz0, double a0, double b0,
                                double nx0, double ny0,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool rectangle3DWithinSphere(  double px0, double py0, double pz0, double a0, double b0,
                                       double nx0, double ny0,
                                       double x, double y, double z, double r, bool strict );
	static bool rectangle3DWithinEllipsoid(  double px0, double py0, double pz0, double a0, double b0,
                                    double nx0, double ny0,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool rectangle3DWithinCone(  double px0, double py0, double pz0, double a0, double b0,
                                    double nx0, double ny0,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );


	// triangle 3D
	static bool triangle3DWithinCube(  double x10, double y10, double z10, double x20, double y20, double z20,
									double x30, double y30,  double z30,
									double x0, double y0, double z0, double r, double nx, double ny, bool strict );

	static bool triangle3DWithinBox(  double x10, double y10, double z10, double x20, double y20, double z20, 
									double x30, double y30, double z30,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool triangle3DWithinSphere(  double x10, double y10, double z10, double x20, double y20, double z20,
									   double x30, double y30, double z30,
                                       double x, double y, double z, double r, bool strict );
	static bool triangle3DWithinEllipsoid(  double x10, double y10, double z10, double x20, double y20, double z20,
											double x30, double y30, double z30,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool triangle3DWithinCone(  double x10, double y10, double z10, double x20, double y20, double z20,
											double x30, double y30, double z30,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );





	// 3D box
	static bool boxWithinCube(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0,
	                            double x0, double y0, double z0, double r, double nx, double ny, bool strict );
	static bool boxWithinBox(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
						       double x0, double y0, double z0, double w, double d, double h, double nx, double ny, bool strict );
	static bool boxWithinSphere( double px0, double py0, double pz0, double a0, double b0, double c0,
                                 double nx0, double ny0, double x, double y, double z, double r,
								 bool strict );
    static bool boxWithinEllipsoid(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double w, double d, double h, 
									double nx, double ny, bool strict );
    static bool boxWithinCone(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double r, double h, 
									double nx, double ny, bool strict );
	
	// ellipsoid
	static bool ellipsoidWithinCube(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0,
	                            double x0, double y0, double z0, double r, double nx, double ny, bool strict );
	static bool ellipsoidWithinBox(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
						       double x0, double y0, double z0, double w, double d, double h, double nx, double ny, bool strict );
	static bool ellipsoidWithinSphere( double px0, double py0, double pz0, double a0, double b0, double c0,
                                 double nx0, double ny0, double x, double y, double z, double r,
								 bool strict );
    static bool ellipsoidWithinEllipsoid(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double w, double d, double h, 
									double nx, double ny, bool strict );
    static bool ellipsoidWithinCone(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double r, double h, 
									double nx, double ny, bool strict );

	// 3D cyliner
	static bool cylinderWithinCube(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                               double x0, double y0, double z0, double r, double nx, double ny, bool strict );
	static bool cylinderWithinBox(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, bool strict );
	static bool cylinderWithinSphere(  double px, double py, double pz, double pr0, double c0,  double nx0, double ny0,
										double x, double y, double z, double r, bool strict );
	static bool cylinderWithinEllipsoid(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double w, double d, double h, 
											double nx, double ny, bool strict );
	static bool cylinderWithinCone(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double r, double h, 
											double nx, double ny, bool strict );

	// 3D cone
	static bool coneWithinCube(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                               double x0, double y0, double z0, double r, double nx, double ny, bool strict );
	static bool coneWithinCube_test(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                               double x0, double y0, double z0, double r, double nx, double ny, bool strict );
	static bool coneWithinBox(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, bool strict );
	static bool coneWithinSphere(  double px, double py, double pz, double pr0, double c0,  double nx0, double ny0,
										double x, double y, double z, double r, bool strict );
	static bool coneWithinEllipsoid(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double w, double d, double h, 
											double nx, double ny, bool strict );
	static bool coneWithinCone(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double r, double h, 
											double nx, double ny, bool strict );


	// 2D line
	static bool lineWithinTriangle( double x10, double y10, double x20, double y20, 
										 double x1, double y1, double x2, double y2, double x3, double y3,  bool strict );
	static bool lineWithinLineString( double x10, double y10, double x20, double y20, 
		                              const Jstr &mk2, const JagStrSplit &sp2, bool strict );
	static bool lineWithinSquare( double x10, double y10, double x20, double y20, 
		                                double x0, double y0, double r, double nx, bool strict );
	static bool lineWithinRectangle( double x10, double y10, double x20, double y20, 
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool lineWithinEllipse( double x10, double y10, double x20, double y20, 
		                                double x0, double y0, double a, double b, double nx, bool strict );
 	static bool lineWithinCircle( double x10, double y10, double x20, double y20, 
									    double x0, double y0, double r, double nx, bool strict );
	static bool lineWithinPolygon( double x10, double y10, double x20, double y20, 
									const Jstr &mk2, const JagStrSplit &sp2, bool strict );


	// line 3D
	static bool line3DWithinLineString3D( double x10, double y10, double z10, double x20, double y20, double z20, 
		                              const Jstr &mk2, const JagStrSplit &sp2, bool strict );
	static bool line3DWithinCube(  double x10, double y10, double z10, double x20, double y20, double z20,
									double x0, double y0, double z0, double r, double nx, double ny, bool strict );

	static bool line3DWithinBox(  double x10, double y10, double z10, double x20, double y20, double z20, 
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool line3DWithinSphere(  double x10, double y10, double z10, double x20, double y20, double z20,
                                       double x, double y, double z, double r, bool strict );
	static bool line3DWithinEllipsoid(  double x10, double y10, double z10, double x20, double y20, double z20,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool line3DWithinCone(  double x10, double y10, double z10, double x20, double y20, double z20,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );



	// linestring 2d
	static bool lineStringWithinLineString( const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2,  bool strict );
	static bool lineStringWithinTriangle( const Jstr &mk1, const JagStrSplit &sp1,
										 double x1, double y1, double x2, double y2, double x3, double y3,  bool strict );
	static bool lineStringWithinSquare( const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double r, double nx, bool strict );
	static bool lineStringWithinRectangle( const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool lineStringWithinEllipse( const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );
 	static bool lineStringWithinCircle( const Jstr &mk1, const JagStrSplit &sp1,
									    double x0, double y0, double r, double nx, bool strict );
 	static bool lineStringWithinPolygon( const Jstr &mk1, const JagStrSplit &sp1,
									     const Jstr &mk2, const JagStrSplit &sp2, bool strict );

	// linestring3d
	static bool lineString3DWithinLineString3D( const Jstr &mk1, const JagStrSplit &sp1,
											    const Jstr &mk2, const JagStrSplit &sp2,  bool strict );
	static bool lineString3DWithinCube( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0, double r, double nx, double ny, bool strict );

	static bool lineString3DWithinBox(  const Jstr &mk1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool lineString3DWithinSphere(  const Jstr &mk1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, bool strict );
	static bool lineString3DWithinEllipsoid(  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool lineString3DWithinCone(  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );


	// polygon
	static bool polygonWithinTriangle( const Jstr &mk1, const JagStrSplit &sp1,
										 double x1, double y1, double x2, double y2, double x3, double y3,  bool strict );
	static bool polygonWithinSquare( const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double r, double nx, bool strict );
	static bool polygonWithinRectangle( const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool polygonWithinEllipse( const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );
 	static bool polygonWithinCircle( const Jstr &mk1, const JagStrSplit &sp1,
									    double x0, double y0, double r, double nx, bool strict );
 	static bool polygonWithinPolygon( const Jstr &mk1, const JagStrSplit &sp1,
										const Jstr &mk2, const JagStrSplit &sp2 );

	// polygon3d within
	static bool polygon3DWithinCube( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0, double r, double nx, double ny, bool strict );

	static void polygon3DWithinCubePoints( JagVector<JagSimplePoint3D> &vec, const JagStrSplit &sp1,
									double x0, double y0, double z0, double r, double nx, double ny, bool strict );

	static bool polygon3DWithinBox(  const Jstr &mk1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );

	static void polygon3DWithinBoxPoints( JagVector<JagSimplePoint3D> &vec, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );

	static bool polygon3DWithinSphere(  const Jstr &mk1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, bool strict );

	static void polygon3DWithinSpherePoints( JagVector<JagSimplePoint3D> &vec, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, bool strict );


	static bool polygon3DWithinEllipsoid(  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );

	static void polygon3DWithinEllipsoidPoints( JagVector<JagSimplePoint3D> &vec, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );


	static bool polygon3DWithinCone(  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );

	static void polygon3DWithinConePoints( JagVector<JagSimplePoint3D> &vec, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );


	// multipolygon
	static bool multiPolygonWithinTriangle( const Jstr &mk1, const JagStrSplit &sp1,
										 double x1, double y1, double x2, double y2, double x3, double y3,  bool strict );
	static bool multiPolygonWithinSquare( const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double r, double nx, bool strict );
	static bool multiPolygonWithinRectangle( const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool multiPolygonWithinEllipse( const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );
 	static bool multiPolygonWithinCircle( const Jstr &mk1, const JagStrSplit &sp1,
									    double x0, double y0, double r, double nx, bool strict );
 	static bool multiPolygonWithinPolygon( const Jstr &mk1, const JagStrSplit &sp1,
										const Jstr &mk2, const JagStrSplit &sp2 );

	// multipolygon3d within
	static bool multiPolygon3DWithinCube( const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0, double r, double nx, double ny, bool strict );

	static bool multiPolygon3DWithinBox(  const Jstr &mk1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool multiPolygon3DWithinSphere(  const Jstr &mk1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, bool strict );
	static bool multiPolygon3DWithinEllipsoid(  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool multiPolygon3DWithinCone(  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );



	static bool pointWithinNormalEllipse( double px, double py, double w, double h, bool strict );
	static bool point3DWithinNormalEllipsoid( double px, double py, double pz, 
											   double w, double d, double h, bool strict );
	static bool point3DWithinNormalCone( double px, double py, double pz, 
										 double r, double h, bool strict );

	/////////////////////////////////////// Intersect function ////////////////////////////////////////////////////
   	static bool doPointIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2, bool strict=true );
   	static bool doPoint3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
								const JagStrSplit &sp2, bool strict=true );
   	static bool doTriangleIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2,  bool strict=true );
   	static bool doTriangle3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2,  bool strict=true );
   	static bool doCircleIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2,  bool strict=true );
   	static bool doCircle3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2,  bool strict=true );
   	static bool doSphereIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doBoxIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doRectangleIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doRectangle3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doSquareIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doSquare3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									  int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doCubeIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									 int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doCylinderIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									 int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doConeIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doEllipseIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doEllipsoidIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doLineIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doLine3DIntersect( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doLineStringIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doLineString3DIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doPolygonIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doPolygon3DIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doMultiPolygonIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );
   	static bool doMultiPolygon3DIntersect( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict=true );

	// addtion or collection/concatenation
   	static Jstr doPointAddition( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2 );
   	static Jstr doPoint3DAddition( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
								const JagStrSplit &sp2 );
   	static Jstr doLineAddition( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static Jstr doLine3DAddition( int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static Jstr doLineStringAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static Jstr doLineString3DAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static Jstr doPolygonAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static Jstr doPolygon3DAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static Jstr doMultiPolygonAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static Jstr doMultiPolygon3DAddition( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static Jstr doPolygonUnion( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2 );

	static Jstr lineAdditionLineString( double x10, double y10, double x20, double y20, 
													const Jstr &mk2, const JagStrSplit &sp2 );
	static Jstr line3DAdditionLineString3D( double x10, double y10, double z10, double x20, double y20, double z20,
	                                      const Jstr &mk2, const JagStrSplit &sp2);
	static Jstr lineString3DAdditionLineString3D( const Jstr &mk1, const JagStrSplit &sp1,
                                            const Jstr &mk2, const JagStrSplit &sp2 );

   	static Jstr doPointIntersection( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doLineIntersection( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doLineStringIntersection( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doMultiLineStringIntersection( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doPolygonIntersection( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doPolygon3DIntersection( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );

   	static Jstr doPointDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doLineDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doLineStringDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doMultiLineStringDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doPolygonDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doMultiPolygonDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );

   	static Jstr doPointSymDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doLineSymDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doLineStringSymDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doMultiLineStringSymDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doPolygonSymDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doMultiPolygonSymDifference( const Jstr &type1,const JagStrSplit &sp1, 
											   const Jstr &type2,const JagStrSplit &sp2 );
   	static Jstr doLocatePoint( int srid, const Jstr &type1,const JagStrSplit &sp1, 
							   const Jstr &type2,const JagStrSplit &sp2 );


	// 2D circle
	static bool circleIntersectCircle( double px, double py, double pr, double x, double y, double r, bool strict );
	static bool circleIntersectEllipse( double px, double py, double pr, 
									 	 double x, double y, double w, double h, double nx, 
									 	 bool strict=true, bool bound=true );
	static bool circleIntersectRectangle( double px0, double py0, double pr, double x0, double y0,
										  double w, double h, double nx,  bool strict );
	static bool circleIntersectTriangle( double px, double py, double pr, double x1, double y1, 
									  double x2, double y2, double x3, double y3, bool strict=true, bool bound = true );
	static bool circleIntersectPolygon( double px, double py, double pr, const Jstr &mk2, const JagStrSplit &sp2,
									   bool strict=true );

	// 3D circle
	static bool circle3DIntersectBox( double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
					                   double x0, double y0, double z0,  double a, double b, double c,
					                   double nx, double ny, bool strict );
   	static bool circle3DIntersectSphere( double px0, double py0, double pz0, double pr0,   double nx0, double ny0,
	   									 double x, double y, double z, double r, bool strict );
	static bool circle3DIntersectEllipsoid( double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
										  double x0, double y0, double z0, 
									 	   double w, double d, double h, double nx, double ny, bool strict );
	static bool circle3DIntersectCone( double px0, double py0, double pz0, double pr0, double nx0, double ny0,
										  double x0, double y0, double z0, 
									 	   double r, double h, double nx, double ny, bool strict );

	// 2D square
	static bool squareIntersectTriangle( double px0, double py0, double pr0, double nx0, 
										 double x1, double y1, double x2, double y2, double x3, double y3, bool strict );
	static bool squareIntersectSquare( double px0, double py0, double pr0, double nx0,
		                                double x0, double y0, double r, double nx, bool strict );
	static bool squareIntersectRectangle( double px0, double py0, double pr0, double nx0,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool squareIntersectEllipse( double px0, double py0, double pr0, double nx0,
		                                double x0, double y0, double a, double b, double nx, bool strict );

 	static bool squareIntersectCircle( double px0, double py0, double pr0, double nx0, 
									    double x0, double y0, double r, double nx, bool strict );

	// 2D rectangle
	static bool rectangleIntersectTriangle( double px0, double py0, double a0, double b0, double nx0, 
										 double x1, double y1, double x2, double y2, double x3, double y3, bool strict );
	static bool rectangleIntersectRectangle( double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool rectangleIntersectEllipse( double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool rectangleIntersectPolygon( double px0, double py0, double a0, double b0, double nx0,
										   const Jstr &mk2, const JagStrSplit &sp2 );

	// 2D triangle
	static bool triangleIntersectTriangle( double x10, double y10, double x20, double y20, double x30, double y30,
										 double x1, double y1, double x2, double y2, double x3, double y3, bool strict );
	static bool triangleIntersectRectangle( double x10, double y10, double x20, double y20, double x30, double y30,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool triangleIntersectEllipse( double x10, double y10, double x20, double y20, double x30, double y30,
		                                double x0, double y0, double a, double b, double nx, bool strict );
 	static bool triangleIntersectCircle( double x10, double y10, double x20, double y20, double x30, double y30,
									    double x0, double y0, double r, double nx, bool strict );
 	static bool triangleIntersectPolygon( double x10, double y10, double x20, double y20, double x30, double y30,
										  const Jstr &mk2, const JagStrSplit &sp2 );
	static bool triangleIntersectLine( double x10, double y10, double x20, double y20, double x30, double y30,
										 double x1, double y1, double x2, double y2 );
 	static bool triangleIntersectLineString( double x10, double y10, double x20, double y20, double x30, double y30,
										  const Jstr &mk2, const JagStrSplit &sp2 );
										
	// 2D ellipse
	static bool ellipseIntersectTriangle( double px0, double py0, double a0, double b0, double nx0, 
										 double x1, double y1, double x2, double y2, double x3, double y3, bool strict );
	static bool ellipseIntersectRectangle( double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool ellipseIntersectEllipse( double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool ellipseIntersectPolygon( double px0, double py0, double a0, double b0, double nx0,
										 const Jstr &mk2, const JagStrSplit &sp2 );

	// rect 3D

	static bool rectangle3DIntersectBox(  double px0, double py0, double pz0, double a0, double b0,
                                double nx0, double ny0,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool rectangle3DIntersectEllipsoid(  double px0, double py0, double pz0, double a0, double b0,
                                    double nx0, double ny0,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool rectangle3DIntersectCone(  double px0, double py0, double pz0, double a0, double b0,
                                    double nx0, double ny0,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );


	// triangle 3D

	static bool triangle3DIntersectBox(  double x10, double y10, double z10, double x20, double y20, double z20, 
									double x30, double y30, double z30,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool triangle3DIntersectEllipsoid(  double x10, double y10, double z10, double x20, double y20, double z20,
											double x30, double y30, double z30,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool triangle3DIntersectCone(  double x10, double y10, double z10, double x20, double y20, double z20,
											double x30, double y30, double z30,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );





	// 3D box
	static bool boxIntersectBox(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
						       double x0, double y0, double z0, double w, double d, double h, double nx, double ny, bool strict );
    static bool boxIntersectEllipsoid(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double w, double d, double h, 
									double nx, double ny, bool strict );
    static bool boxIntersectCone(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double r, double h, 
									double nx, double ny, bool strict );
	
	static bool ellipsoidIntersectBox(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
						       double x0, double y0, double z0, double w, double d, double h, double nx, double ny, bool strict );
    static bool ellipsoidIntersectEllipsoid(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double w, double d, double h, 
									double nx, double ny, bool strict );
    static bool ellipsoidIntersectCone(  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double r, double h, 
									double nx, double ny, bool strict );

	// 3D cyliner
	static bool cylinderIntersectBox(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, bool strict );
	static bool cylinderIntersectEllipsoid(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double w, double d, double h, 
											double nx, double ny, bool strict );
	static bool cylinderIntersectCone(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double r, double h, 
											double nx, double ny, bool strict );

	// 3D cone
	static bool coneIntersectBox(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, bool strict );
	static bool coneIntersectEllipsoid(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double w, double d, double h, 
											double nx, double ny, bool strict );
	static bool coneIntersectCone(  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double r, double h, 
											double nx, double ny, bool strict );


	// 2D line
	static bool lineIntersectTriangle( double x10, double y10, double x20, double y20, 
										 double x1, double y1, double x2, double y2, double x3, double y3,  bool strict );
	static bool lineIntersectLineString( double x10, double y10, double x20, double y20, 
		                              const Jstr &mk2, const JagStrSplit &sp2, bool strict );
	static bool lineIntersectRectangle( double x10, double y10, double x20, double y20, 
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool lineIntersectEllipse( double x10, double y10, double x20, double y20, 
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool lineIntersectLine(  double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4 );
	static bool lineIntersectLine(  const JagLine2D &line1, const JagLine2D &line2 );
	static bool lineIntersectPolygon( double x10, double y10, double x20, double y20, 
		                              const Jstr &mk2, const JagStrSplit &sp2 );


	static bool line3DIntersectLineString3D( double x10, double y10, double z10, double x20, double y20, double z20,
		                              const Jstr &mk2, const JagStrSplit &sp2, bool strict );
	static bool line3DIntersectBox( const JagLine3D &line, double x0, double y0, double z0,

									double w, double d, double h, double nx, double ny, bool strict );
	static bool line3DIntersectBox(  double x10, double y10, double z10, double x20, double y20, double z20, 
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool line3DIntersectSphere(  double x10, double y10, double z10, double x20, double y20, double z20,
                                       double x, double y, double z, double r, bool strict );
	static bool line3DIntersectEllipsoid(  double x10, double y10, double z10, double x20, double y20, double z20,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool line3DIntersectCone(  double x10, double y10, double z10, double x20, double y20, double z20,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );
	static bool line3DIntersectCylinder(  double x10, double y10, double z10, double x20, double y20, double z20,
                                    double x0, double y0, double z0,
                                    double a, double b, double c, double nx, double ny, bool strict );

	// linestring intersect
	static bool lineStringIntersectLineString( const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2, bool doRes, JagVector<JagPoint2D> &vec );
	static bool lineStringIntersectTriangle( const Jstr &m1, const JagStrSplit &sp1,
										 double x1, double y1, double x2, double y2, double x3, double y3,  bool strict );
	static bool lineStringIntersectRectangle( const Jstr &m1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool lineStringIntersectEllipse( const Jstr &m1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );



	static bool lineString3DIntersectLineString3D( const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2, bool doRes, JagVector<JagPoint3D> &vec  );

	static bool lineString3DIntersectBox(  const Jstr &m1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool lineString3DIntersectSphere(  const Jstr &m1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, bool strict );
	static bool lineString3DIntersectEllipsoid(  const Jstr &m1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool lineString3DIntersectCone(  const Jstr &m1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );
	static bool lineString3DIntersectCylinder( const Jstr &m1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double a, double b, double c, double nx, double ny, bool strict );
	static bool lineString3DIntersectTriangle3D( const Jstr &m1, const JagStrSplit &sp1,
												 const Jstr &m2, const JagStrSplit &sp2 );
	static bool lineString3DIntersectSquare3D( const Jstr &m1, const JagStrSplit &sp1,
												 const Jstr &m2, const JagStrSplit &sp2 );
	static bool lineString3DIntersectRectangle3D( const Jstr &m1, const JagStrSplit &sp1,
												 const Jstr &m2, const JagStrSplit &sp2 );


	// polygon intersect
	static bool polygonIntersectLineString( const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2  );
	static bool polygonIntersectTriangle( const Jstr &m1, const JagStrSplit &sp1,
										 double x1, double y1, double x2, double y2, double x3, double y3,  bool strict );
	static bool polygonIntersectRectangle( const Jstr &m1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool polygonIntersectEllipse( const Jstr &m1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool polygonIntersectLine( const Jstr &m1, const JagStrSplit &sp1,
										 double x1, double y1, double x2, double y2 );

	// polygon3d intersect
	static bool polygon3DIntersectLineString3D( const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2  );

	static bool polygon3DIntersectBox(  const Jstr &m1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool polygon3DIntersectSphere(  const Jstr &m1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, bool strict );
	static bool polygon3DIntersectEllipsoid(  const Jstr &m1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool polygon3DIntersectCone(  const Jstr &m1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );
	static bool polygon3DIntersectCylinder( const Jstr &m1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double a, double b, double c, double nx, double ny, bool strict );

	// 2D area or 3D surface area
   	static double doCircleArea(  int srid1, const JagStrSplit &sp1 );
   	static double doCircle3DArea(  int srid1, const JagStrSplit &sp1 );
   	static double doSphereArea(  int srid1, const JagStrSplit &sp1 );
   	static double doSphereVolume(  int srid1, const JagStrSplit &sp1 );
   	static double doSquareArea(  int srid1, const JagStrSplit &sp1 );
   	static double doSquare3DArea(  int srid1, const JagStrSplit &sp1 );
   	static double doCubeArea(  int srid1, const JagStrSplit &sp1 );
   	static double doCubeVolume(  int srid1, const JagStrSplit &sp1 );
   	static double doRectangleArea(  int srid1, const JagStrSplit &sp1 );
   	static double doRectangle3DArea(  int srid1, const JagStrSplit &sp1 );
   	static double doBoxArea(  int srid1, const JagStrSplit &sp1 );
   	static double doBoxVolume(  int srid1, const JagStrSplit &sp1 );
   	static double doTriangleArea(  int srid1, const JagStrSplit &sp1 );
   	static double doTriangle3DArea(  int srid1, const JagStrSplit &sp1 );
   	static double doCylinderArea(  int srid1, const JagStrSplit &sp1 );
   	static double doCylinderVolume(  int srid1, const JagStrSplit &sp1 );
   	static double doConeArea(  int srid1, const JagStrSplit &sp1 );
   	static double doConeVolume(  int srid1, const JagStrSplit &sp1 );
   	static double doEllipseArea(  int srid1, const JagStrSplit &sp1 );
   	static double doEllipse3DArea(  int srid1, const JagStrSplit &sp1 );
   	static double doEllipsoidArea(  int srid1, const JagStrSplit &sp1 );
   	static double doEllipsoidVolume(  int srid1, const JagStrSplit &sp1 );
   	static double doPolygonArea(  const Jstr &mk1, int srid1, const JagStrSplit &sp1 );
   	static double doMultiPolygonArea(  const Jstr &mk1, int srid1, const JagStrSplit &sp1 );

	// 2D area or 3D perimeter
   	static double doCirclePerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doCircle3DPerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doSquarePerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doSquare3DPerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doCubePerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doRectanglePerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doRectangle3DPerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doBoxPerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doTrianglePerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doTriangle3DPerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doEllipsePerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doEllipse3DPerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doEllipsoidPerimeter(  int srid1, const JagStrSplit &sp1 );
   	static double doPolygonPerimeter(  const Jstr &mk1, int srid1, const JagStrSplit &sp1 );
   	static double doPolygon3DPerimeter(  const Jstr &mk1, int srid1, const JagStrSplit &sp1 );
   	static double doMultiPolygonPerimeter(  const Jstr &mk1, int srid1, const JagStrSplit &sp1 );
   	static double doMultiPolygon3DPerimeter(  const Jstr &mk1, int srid1, const JagStrSplit &sp1 );

	/////////////////////////////////////// same(equal) function ///////////////////////////////////////////
   	static bool doPointSame(  const JagStrSplit &sp1, const Jstr &mk2, 
								const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static bool doPoint3DSame( const JagStrSplit &sp1, const Jstr &mk2, 
								const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static bool doTriangleSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2 );
   	static bool doTriangle3DSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2 );
   	static bool doCircleSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2 );
   	static bool doCircle3DSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, int srid2, 
									const JagStrSplit &sp2 );
   	static bool doSphereSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static bool doBoxSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static bool doRectangleSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static bool doRectangle3DSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static bool doSquareSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static bool doSquare3DSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									  int srid2, const JagStrSplit &sp2 );
   	static bool doCubeSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									 int srid2, const JagStrSplit &sp2 );
   	static bool doCylinderSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									 int srid2, const JagStrSplit &sp2 );
   	static bool doConeSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static bool doEllipseSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static bool doEllipsoidSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static bool doLineSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static bool doLine3DSame(  int srid1, const JagStrSplit &sp1, const Jstr &mk2, const Jstr &colType2, 
									int srid2, const JagStrSplit &sp2 );
   	static bool doLineStringSame(  const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static bool doLineString3DSame( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									  const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static bool doPolygonSame(  const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static bool doPolygon3DSame( const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									  const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static bool doMultiPolygonSame(  const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
   	static bool doMultiPolygon3DSame(  const Jstr &mk1, int srid1, const JagStrSplit &sp1, const Jstr &mk2, 
									const Jstr &colType2, int srid2, const JagStrSplit &sp2 );

	static bool sequenceSame( const Jstr &colType, const Jstr &mk1, const JagStrSplit &sp1, const Jstr &mk2, const JagStrSplit &sp2  );
	/*****
	// polygon intersect
	static bool multiPolygonIntersectLineString( const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2  );
	static bool multiPolygonIntersectTriangle( const Jstr &m1, const JagStrSplit &sp1,
										 double x1, double y1, double x2, double y2, double x3, double y3,  bool strict );
	static bool multiPolygonIntersectRectangle( const Jstr &m1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );
	static bool multiPolygonIntersectEllipse( const Jstr &m1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, bool strict );

	// multiPolygon3d intersect
	static bool multiPolygon3DIntersectLineString3D( const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2  );

	static bool multiPolygon3DIntersectBox(  const Jstr &m1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, bool strict );
	static bool multiPolygon3DIntersectSphere(  const Jstr &m1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, bool strict );
	static bool multiPolygon3DIntersectEllipsoid(  const Jstr &m1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, bool strict );
	static bool multiPolygon3DIntersectCone(  const Jstr &m1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, bool strict );
	static bool multiPolygon3DIntersectCylinder( const Jstr &m1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double a, double b, double c, double nx, double ny, bool strict );
	*********/

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// misc
	static double distance( double fx, double fy, double gx, double gy, int srid );
	static double distance( double fx, double fy, double fz, double gx, double gy, double gz, int srid );
	static double squaredDistance( double fx, double fy, double gx, double gy, int srid );
	static double squaredDistance( double fx, double fy, double fz, double gx, double gy, double gz, int srid );

    static double DistanceOfPointToLine(double x0 ,double y0 ,double z0 ,double x1 ,double y1 ,double z1 ,double x2 ,double y2 ,double z2);


    static bool twoLinesIntersection( double slope1, double c1, double slope2, double c2,
									  double &outx, double &outy );
	static void orginBoundingBoxOfRotatedEllipse( double a, double b, double nx, 
										double &x1, double &y1,
										double &x2, double &y2,
										double &x3, double &y3,
										double &x4, double &y4 );
	static void boundingBoxOfRotatedEllipse( double x0, double y0, double a, double b, double nx, 
										double &x1, double &y1,
										double &x2, double &y2,
										double &x3, double &y3,
										double &x4, double &y4 );
		
	static void project3DCircleToXYPlane( double x0, double y0, double z0, double r0, double nx0, double ny0,
									 double x, double y, double a, double b, double nx );
	static void project3DCircleToYZPlane( double x0, double y0, double z0, double r0, double nx0, double ny0,
									 double y, double z, double a, double b, double ny );
	static void project3DCircleToZXPlane( double x0, double y0, double z0, double r0, double nx0, double ny0,
									 double z, double x, double a, double b, double nz );

	static void transform3DDirection( double nx1, double ny1, double nx2, double ny2, double &nx, double &ny );
	static void transform2DDirection( double nx1, double nx2, double &nx );
	static void samplesOn2DCircle( double x0, double y0, double r, int num, JagVector<JagPoint2D> &samples );
	static void samplesOn3DCircle( double x0, double y0, double z0, double r, double nx, double ny, 
								   int num, JagVector<JagPoint3D> &samples );

	static void samplesOn2DEllipse( double x0, double y0, double a, double b, double nx, int num, JagVector<JagPoint2D> &samples );
	static void samplesOn3DEllipse( double x0, double y0, double z0, double a, double b, double nx, double ny, int num, JagVector<JagPoint3D> &samples );
	static void samplesOnEllipsoid( double x0, double y0, double z0, double a, double b, double c,
									   double nx, double ny, int num2, JagVector<JagPoint3D> &samples );
	static void sampleLinesOnCone( double x0, double y0, double z0, double r, double h,
									   double nx, double ny, int num2, JagVector<JagLine3D> &samples );
	static void sampleLinesOnCylinder( double x0, double y0, double z0, double r, double h,
									   double nx, double ny, int num2, JagVector<JagLine3D> &samples );

	// static Jstr convertType2Short( const Jstr &geotypeLong );
	static bool jagSquare(double f );

	static void rotate2DCoordGlobal2Local( double inx, double iny, double nx, double &outx, double &outy );
	static void transform2DCoordGlobal2Local( double outx0, double outy0, double inx, double iny, double nx,
                               	      double &outx, double &outy );
	static void rotate2DCoordLocal2Global( double inx, double iny, double nx, double &outx, double &outy );
	static void transform2DCoordLocal2Global( double inx0, double iny0, double inx, double iny, double nx,
                               	      double &outx, double &outy );


	static void rotate3DCoordGlobal2Local( double inx, double iny, double inz, double nx, double ny, 
								    double &outx, double &outy, double &outz );
    static void transform3DCoordGlobal2Local( double outx0, double outy0, double outz0, double inx, double iny, double inz,
									  double nx,  double ny, double &outx, double &outy, double &outz );
	static void rotate3DCoordLocal2Global( double inx, double iny, double inz, double nx, double ny, 
								    double &outx, double &outy, double &outz );
    static void transform3DCoordLocal2Global( double inx0, double iny0, double inz0, double inx, double iny, double inz,
									  double nx,  double ny, double &outx, double &outy, double &outz );
   static bool doAllNearby( const Jstr& mark1, const Jstr &colType1, int srid1, const JagStrSplit &sp1,
                             const Jstr& mark2, const Jstr &colType2, int srid2, const JagStrSplit &sp2, 
							 const Jstr &carg );

	static bool above(double x, double y, double x2, double y2, double x3, double y3 );
	static bool above(double x, double y, double z, double x2, double y2, double z2, double x3, double y3, double z3 );
	static bool aboveOrSame(double x, double y, double x2, double y2, double x3, double y3 );
	static bool aboveOrSame(double x, double y, double z, double x2, double y2, double z2, double x3, double y3, double z3 );
	static bool below(double x, double y, double x2, double y2, double x3, double y3 );
	static bool below(double x, double y, double z, double x2, double y2, double z2, double x3, double y3, double z3 );
	static bool belowOrSame(double x, double y, double x2, double y2, double x3, double y3 );
	static bool belowOrSame(double x, double y, double z, double x2, double y2, double z2, double x3, double y3, double z3 );
	static bool same(double x, double y, double x2, double y2, double x3, double y3 );
	static bool same(double x, double y, double z, double x2, double y2, double z2, double x3, double y3, double z3 );
	static bool isNull( double x, double y );
	static bool isNull( double x, double y, double z );
	static bool isNull(double x1, double y1, double x2, double y2 );
	static bool isNull(double x1, double y1, double z1, double x2, double y2, double z2 );
	/**
	static Jstr makeGeoJson( const JagStrSplit &sp, const char *str );
	static Jstr makeJsonLineString( const Jstr &title, const JagStrSplit &sp, const char *str );
	static Jstr makeJsonLineString3D( const Jstr &title, const JagStrSplit &sp, const char *str );
	static Jstr makeJsonPolygon( const Jstr &title, const JagStrSplit &sp, const char *str, bool is3D );
	static Jstr makeJsonMultiPolygon( const Jstr &title, const JagStrSplit &sp, const char *str, bool is3D );
	static Jstr makeJsonDefault( const JagStrSplit &sp, const char *str );
	**/
	static bool distance( const JagFixString &lstr, const JagFixString &rstr, const Jstr &arg, double &dist );
	static bool similarity( const JagFixString &lstr, const JagFixString &rstr, const Jstr &arg, double &dist );

    static double computeSimilarity( const JagStrSplit& sp1, const JagStrSplit& sp2, const Jstr& arg );
	static double cosineSimilarity( const JagStrSplit& sp1, const JagStrSplit& sp2 );

	////////////// distance //////////////////

	static bool doPointDistance( const Jstr& mark1, const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist );
	static bool doPoint3DDistance( const Jstr& mark1, const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doCircleDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doCircle3DDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doSphereDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doSquareDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doSquare3DDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doCubeDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doRectangleDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doRectangle3DDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doBoxDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doTriangleDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doTriangle3DDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doCylinderDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doConeDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doEllipseDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doEllipsoidDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doLineDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doLine3DDistance( const Jstr& mark1,  const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doLineStringDistance( const Jstr& mark1, const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doLineString3DDistance( const Jstr& mark1, const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doPolygonDistance( const Jstr& mark1, const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doPolygon3DDistance( const Jstr& mark1, const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doMultiPolygonDistance( const Jstr& mark1, const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	static bool doMultiPolygon3DDistance( const Jstr& mark1, const JagStrSplit& sp1, const Jstr& mark2,
										 const Jstr& colType2,
								 const JagStrSplit& sp2, int srid, const Jstr& arg, double &dist);
	
	// 2D point
	static bool pointDistancePoint( int srid, double px, double py, double x1, double y1, const Jstr& arg, double &dist );
	static bool pointDistanceLine( int srid, double px, double py, double x1, double y1, double x2, double y2, const Jstr& arg, double &dist );
	static bool pointDistanceLineString( int srid,  double x, double y, const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );
	static bool pointDistanceSquare( int srid, double px, double py, double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
	static bool pointDistanceCircle( int srid, double px, double py, double x0, double y0, double r, const Jstr& arg, double &dist );
	static bool pointDistanceRectangle( int srid, double px, double py, double x, double y, double a, double b, double nx, const Jstr& arg, double &dist );
	static bool pointDistanceEllipse( int srid, double px, double py, double x, double y, double a, double b, double nx, const Jstr& arg, double &dist );
	static bool pointDistanceTriangle( int srid, double px, double py, double x1, double y1,
									  double x2, double y2, double x3, double y3,
					 				  const Jstr& arg, double &dist ); 
	static bool pointDistancePolygon( int srid, double x, double y, const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );
	static bool pointDistancePolygon( int srid, double x, double y, const JagLineString3D &linestr );
	static bool pointDistancePolygon( int srid, double x, double y, const JagPolygon &pgon );

	// 3D point
	static bool point3DDistancePoint3D( int srid, double px, double py, double pz, double x1, double y1, double z1, const Jstr& arg, double &dist );
	static bool point3DDistanceLine3D( int srid, double px, double py, double pz, double x1, double y1, double z1, 
									double x2, double y2, double z2, const Jstr& arg, double &dist );
	static bool point3DDistanceLineString3D( int srid,  double x, double y, double z, const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );
	static bool point3DDistanceBox( int srid, double px, double py, double pz,  
									  double x, double y, double z, double a, double b, double c, double nx, double ny, 
									  const Jstr& arg, double &dist );
    static bool point3DDistanceSphere( int srid, double px, double py, double pz, double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool point3DDistanceEllipsoid( int srid, double px, double py, double pz,  
									  double x, double y, double z, double a, double b, double c, double nx, double ny, 
									  const Jstr& arg, double &dist );

	static bool point3DDistanceCone( int srid, double px, double py, double pz, 
									double x0, double y0, double z0,
									 double r, double h,  double nx, double ny, const Jstr& arg, double &dist );
	static bool point3DDistanceSquare3D( int srid, double px, double py, double pz, 
									double x0, double y0, double z0,
									 double a, double nx, double ny, const Jstr& arg, double &dist );
	static bool point3DDistanceCylinder( int srid,  double x, double y, double z,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool point3DDistanceNormalCylinder( int srid,  double x, double y, double z,
                                    double r, double h, const Jstr& arg, double &dist );

	// 2D circle
	static bool circleDistanceCircle( int srid, double px, double py, double pr, double x, double y, double r, const Jstr& arg, double &dist );
	static bool circleDistanceSquare( int srid, double px0, double py0, double pr, double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
	static bool circleDistanceEllipse( int srid, double px, double py, double pr, 
									 	 double x, double y, double w, double h, double nx, 
									 	 const Jstr& arg, double &dist );
	static bool circleDistanceRectangle( int srid, double px0, double py0, double pr, double x0, double y0,
										  double w, double h, double nx,  const Jstr& arg, double &dist );
	static bool circleDistanceTriangle( int srid, double px, double py, double pr, double x1, double y1, 
									  double x2, double y2, double x3, double y3, const Jstr& arg, double &dist );
	static bool circleDistancePolygon( int srid, double px0, double py0, double pr, 
									 const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );

	// 3D circle
	static bool circle3DDistanceCube( int srid, double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
										double x0, double y0, double z0,  double r, double nx, double ny, const Jstr& arg, 
										double &dist );

	static bool circle3DDistanceBox( int srid, double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
					                   double x0, double y0, double z0,  double a, double b, double c,
					                   double nx, double ny, const Jstr& arg, double &dist );
   	static bool circle3DDistanceSphere( int srid, double px0, double py0, double pz0, double pr0,   double nx0, double ny0,
	   									 double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool circle3DDistanceEllipsoid( int srid, double px0, double py0, double pz0, double pr0,  double nx0, double ny0,
										  double x0, double y0, double z0, 
									 	   double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool circle3DDistanceCone( int srid, double px0, double py0, double pz0, double pr0, double nx0, double ny0,
										  double x0, double y0, double z0, 
									 	   double r, double h, double nx, double ny, const Jstr& arg, double &dist );

	// 3D sphere
	static bool sphereDistanceCube( int srid,  double px0, double py0, double pz0, double pr0,
		                               double x0, double y0, double z0, double r, double nx, double ny, 
										const Jstr& arg, double &dist );
	static bool sphereDistanceBox( int srid,  double px0, double py0, double pz0, double r,
		                                double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, const Jstr& arg, double &dist );
	static bool sphereDistanceSphere( int srid,  double px, double py, double pz, double pr, 
										double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool sphereDistanceEllipsoid( int srid,  double px0, double py0, double pz0, double pr,
		                                    double x0, double y0, double z0, double w, double d, double h, 
											double nx, double ny, const Jstr& arg, double &dist );
	static bool sphereDistanceCone( int srid,  double px0, double py0, double pz0, double pr,
		                                    double x0, double y0, double z0, double r, double h, 
											double nx, double ny, const Jstr& arg, double &dist );

	// 2D rectangle
	static bool rectangleDistanceTriangle( int srid, double px0, double py0, double a0, double b0, double nx0, 
										 double x1, double y1, double x2, double y2, double x3, double y3, const Jstr& arg, double &dist );
	static bool rectangleDistanceSquare( int srid, double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
	static bool rectangleDistanceRectangle( int srid, double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
	static bool rectangleDistanceEllipse( int srid, double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
 	static bool rectangleDistanceCircle( int srid, double px0, double py0, double a0, double b0, double nx0, 
									    double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
 	static bool rectangleDistancePolygon( int srid, double px0, double py0, double a0, double b0, double nx0, 
										const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );

	// 2D triangle
	static bool triangleDistanceTriangle( int srid, double x10, double y10, double x20, double y20, double x30, double y30,
										 double x1, double y1, double x2, double y2, double x3, double y3, const Jstr& arg, double &dist );
	static bool triangleDistanceSquare( int srid, double x10, double y10, double x20, double y20, double x30, double y30,
		                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
	static bool triangleDistanceRectangle( int srid, double x10, double y10, double x20, double y20, double x30, double y30,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
	static bool triangleDistanceEllipse( int srid, double x10, double y10, double x20, double y20, double x30, double y30,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
 	static bool triangleDistanceCircle( int srid, double x10, double y10, double x20, double y20, double x30, double y30,
									    double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
 	static bool triangleDistancePolygon( int srid, double x10, double y10, double x20, double y20, double x30, double y30,
									    const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );
										
	// 2D ellipse
	static bool ellipseDistanceTriangle( int srid, double px0, double py0, double a0, double b0, double nx0, 
										 double x1, double y1, double x2, double y2, double x3, double y3, const Jstr& arg, double &dist );
	static bool ellipseDistanceSquare( int srid, double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
	static bool ellipseDistanceRectangle( int srid, double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
	static bool ellipseDistanceEllipse( int srid, double px0, double py0, double a0, double b0, double nx0,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
 	static bool ellipseDistanceCircle( int srid, double px0, double py0, double a0, double b0, double nx0, 
									    double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
 	static bool ellipseDistancePolygon( int srid, double px0, double py0, double a0, double b0, double nx0, 
									    const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );

	// rect 3D
	static bool rectangle3DDistanceCube( int srid,  double px0, double py0, double pz0, double a0, double b0, double nx0, double ny0,
                                double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );

	static bool rectangle3DDistanceBox( int srid,  double px0, double py0, double pz0, double a0, double b0,
                                double nx0, double ny0,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool rectangle3DDistanceSphere( int srid,  double px0, double py0, double pz0, double a0, double b0,
                                       double nx0, double ny0,
                                       double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool rectangle3DDistanceEllipsoid( int srid,  double px0, double py0, double pz0, double a0, double b0,
                                    double nx0, double ny0,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool rectangle3DDistanceCone( int srid,  double px0, double py0, double pz0, double a0, double b0,
                                    double nx0, double ny0,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist );


	// triangle 3D
	static bool triangle3DDistanceCube( int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
									double x30, double y30,  double z30,
									double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );

	static bool triangle3DDistanceBox( int srid,  double x10, double y10, double z10, double x20, double y20, double z20, 
									double x30, double y30, double z30,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool triangle3DDistanceSphere( int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
									   double x30, double y30, double z30,
                                       double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool triangle3DDistanceEllipsoid( int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
											double x30, double y30, double z30,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool triangle3DDistanceCone( int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
											double x30, double y30, double z30,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist );





	// 3D box
	static bool boxDistanceCube( int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0,
	                            double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );
	static bool boxDistanceBox( int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
						       double x0, double y0, double z0, double w, double d, double h, double nx, double ny, 
								const Jstr& arg, double &dist );
	static bool boxDistanceSphere( int srid, double px0, double py0, double pz0, double a0, double b0, double c0,
                                 double nx0, double ny0, double x, double y, double z, double r,
								 const Jstr& arg, double &dist );
    static bool boxDistanceEllipsoid( int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double w, double d, double h, 
									double nx, double ny, const Jstr& arg, double &dist );
    static bool boxDistanceCone( int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double r, double h, 
									double nx, double ny, const Jstr& arg, double &dist );
	
	// ellipsoid
	static bool ellipsoidDistanceCube( int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0,
	                            double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );
	static bool ellipsoidDistanceBox( int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
						       double x0, double y0, double z0, double w, double d, double h, double nx, double ny, 
							   const Jstr& arg, double &dist );
	static bool ellipsoidDistanceSphere( int srid, double px0, double py0, double pz0, double a0, double b0, double c0,
                                 double nx0, double ny0, double x, double y, double z, double r,
								 const Jstr& arg, double &dist );
    static bool ellipsoidDistanceEllipsoid( int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double w, double d, double h, 
									double nx, double ny, const Jstr& arg, double &dist );
    static bool ellipsoidDistanceCone( int srid,  double px0, double py0, double pz0, double a0, double b0, double c0, double nx0, double ny0, 
                                    double x0, double y0, double z0, double r, double h, 
									double nx, double ny, const Jstr& arg, double &dist );

	// 3D cyliner
	static bool cylinderDistanceCube( int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                               double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );
	static bool cylinderDistanceBox( int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, const Jstr& arg, double &dist );
	static bool cylinderDistanceSphere( int srid,  double px, double py, double pz, double pr0, double c0,  double nx0, double ny0,
										double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool cylinderDistanceEllipsoid( int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double w, double d, double h, 
											double nx, double ny, const Jstr& arg, double &dist );
	static bool cylinderDistanceCone( int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double r, double h, 
											double nx, double ny, const Jstr& arg, double &dist );

	// 3D cone
	static bool coneDistanceCube( int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                               double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );
	static bool coneDistanceCube_test( int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                               double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );
	static bool coneDistanceBox( int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                double x0, double y0, double z0, double w, double d, double h, 
										double nx, double ny, const Jstr& arg, double &dist );
	static bool coneDistanceSphere( int srid,  double px, double py, double pz, double pr0, double c0,  double nx0, double ny0,
										double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool coneDistanceEllipsoid( int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double w, double d, double h, 
											double nx, double ny, const Jstr& arg, double &dist );
	static bool coneDistanceCone( int srid,  double px0, double py0, double pz0, double pr0, double c0, double nx0, double ny0,
		                                    double x0, double y0, double z0, double r, double h, 
											double nx, double ny, const Jstr& arg, double &dist );


	// 2D line
	static bool lineDistanceTriangle( int srid, double x10, double y10, double x20, double y20, 
										 double x1, double y1, double x2, double y2, double x3, double y3,  const Jstr& arg, double &dist );
	static bool lineDistanceLineString( int srid, double x10, double y10, double x20, double y20, 
		                              const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );
	static bool lineDistanceSquare( int srid, double x10, double y10, double x20, double y20, 
		                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
	static bool lineDistanceRectangle( int srid, double x10, double y10, double x20, double y20, 
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
	static bool lineDistanceEllipse( int srid, double x10, double y10, double x20, double y20, 
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
 	static bool lineDistanceCircle( int srid, double x10, double y10, double x20, double y20, 
									    double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
	static bool lineDistancePolygon( int srid, double x10, double y10, double x20, double y20, 
									const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );


	// line 3D
	static bool line3DDistanceLineString3D( int srid, double x10, double y10, double z10, double x20, double y20, double z20, 
		                              const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );
	static bool line3DDistanceCube( int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
									double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );

	static bool line3DDistanceBox( int srid,  double x10, double y10, double z10, double x20, double y20, double z20, 
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool line3DDistanceSphere( int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
                                       double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool line3DDistanceEllipsoid( int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool line3DDistanceCone( int srid,  double x10, double y10, double z10, double x20, double y20, double z20,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist );



	// linestring 2d
	static bool lineStringDistanceLineString( int srid, const Jstr &mk1, const JagStrSplit &sp1,
											const Jstr &mk2, const JagStrSplit &sp2,  const Jstr& arg, double &dist );
	static bool lineStringDistanceTriangle( int srid, const Jstr &mk1, const JagStrSplit &sp1,
										 double x1, double y1, double x2, double y2, double x3, double y3,  const Jstr& arg, double &dist );
	static bool lineStringDistanceSquare( int srid, const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
	static bool lineStringDistanceRectangle( int srid, const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
	static bool lineStringDistanceEllipse( int srid, const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
 	static bool lineStringDistanceCircle( int srid, const Jstr &mk1, const JagStrSplit &sp1,
									    double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
 	static bool lineStringDistancePolygon( int srid, const Jstr &mk1, const JagStrSplit &sp1,
									     const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );

	// linestring3d
	static bool lineString3DDistanceLineString3D( int srid, const Jstr &mk1, const JagStrSplit &sp1,
											    const Jstr &mk2, const JagStrSplit &sp2,  const Jstr& arg, double &dist );
	static bool lineString3DDistanceCube( int srid, const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );

	static bool lineString3DDistanceBox( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool lineString3DDistanceSphere( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool lineString3DDistanceEllipsoid( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool lineString3DDistanceCone( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist );


	// polygon
	static bool polygonDistanceTriangle( int srid, const Jstr &mk1, const JagStrSplit &sp1,
										 double x1, double y1, double x2, double y2, double x3, double y3,  const Jstr& arg, double &dist );
	static bool polygonDistanceSquare( int srid, const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
	static bool polygonDistanceRectangle( int srid, const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
	static bool polygonDistanceEllipse( int srid, const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
 	static bool polygonDistanceCircle( int srid, const Jstr &mk1, const JagStrSplit &sp1,
									    double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
 	static bool polygonDistancePolygon( int srid, const Jstr &mk1, const JagStrSplit &sp1,
										const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );

	// polygon3d Distance
	static bool polygon3DDistanceCube( int srid, const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );

	static bool polygon3DDistanceBox( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool polygon3DDistanceSphere( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool polygon3DDistanceEllipsoid( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool polygon3DDistanceCone( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist );


	// multipolygon
	static bool multiPolygonDistanceTriangle( int srid, const Jstr &mk1, const JagStrSplit &sp1,
										 double x1, double y1, double x2, double y2, double x3, double y3,  const Jstr& arg, double &dist );
	static bool multiPolygonDistanceSquare( int srid, const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
	static bool multiPolygonDistanceRectangle( int srid, const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
	static bool multiPolygonDistanceEllipse( int srid, const Jstr &mk1, const JagStrSplit &sp1,
		                                double x0, double y0, double a, double b, double nx, const Jstr& arg, double &dist );
 	static bool multiPolygonDistanceCircle( int srid, const Jstr &mk1, const JagStrSplit &sp1,
									    double x0, double y0, double r, double nx, const Jstr& arg, double &dist );
 	static bool multiPolygonDistancePolygon( int srid, const Jstr &mk1, const JagStrSplit &sp1,
										const Jstr &mk2, const JagStrSplit &sp2, const Jstr& arg, double &dist );

	// multipolygon3d Distance
	static bool multiPolygon3DDistanceCube( int srid, const Jstr &mk1, const JagStrSplit &sp1,
									double x0, double y0, double z0, double r, double nx, double ny, const Jstr& arg, double &dist );

	static bool multiPolygon3DDistanceBox( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                double x0, double y0, double z0,
                                double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool multiPolygon3DDistanceSphere( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                       double x, double y, double z, double r, const Jstr& arg, double &dist );
	static bool multiPolygon3DDistanceEllipsoid( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double w, double d, double h, double nx, double ny, const Jstr& arg, double &dist );
	static bool multiPolygon3DDistanceCone( int srid,  const Jstr &mk1, const JagStrSplit &sp1,
                                    double x0, double y0, double z0,
                                    double r, double h, double nx, double ny, const Jstr& arg, double &dist );

	//static bool pointDistanceNormalEllipse( int srid, double px, double py, double w, double h, const Jstr& arg, double &dist );
	static bool point3DDistanceNormalEllipsoid( int srid, double px, double py, double pz, 
											   double w, double d, double h, const Jstr& arg, double &dist );
	static bool point3DDistanceNormalCone( int srid, double px, double py, double pz, 
										 double r, double h, const Jstr& arg, double &dist );

	static double pointToLineGeoDistance( double lata1, double lona1, double lata2, double lona2, double latb1, double lonb1 );
	static double safeget( const JagStrSplit &sp, int arg );
	static Jstr safeGetStr( const JagStrSplit &sp, int arg );
	static void center2DMultiPolygon( const JagVector<JagPolygon> &pgvec, double &cx, double &cy );
	static void center3DMultiPolygon( const JagVector<JagPolygon> &pgvec, double &cx, double &cy, double &cz );
	static void fourthOrderEquation( double a, double b, double c, double d, double e, int &num, double *root );
    static void minMaxPointOnNormalEllipse( int srid, double a, double b, double u, double v, bool isMin, double &x, double &y, double &dist );
    static void minMaxPoint3DOnNormalEllipsoid( int srid, double a, double b, double c,  
											    double u, double v, double w, bool isMin, 
												double &x, double &y, double &z, double &dist );
	static void fourthOrderEquation( double b, double c, double d, double e, int &num, double *root );
	static double pointDistanceToEllipse( int srid, double px, double py, double x0, double y0, double a, double b, double nx, bool isMin );
	static double point3DDistanceToEllipsoid( int srid, double px, double py, double pz,
											  double x0, double y0, double z0, 
											  double a, double b, double c, double nx, double ny, bool isMin );
	static bool doClosestPoint(  const Jstr& colType1, int srid, double px, double py, double pz,
									const Jstr& mark2, const Jstr &colType2, 
									const JagStrSplit &sp2, Jstr &res );

	static bool getBBox2D( const JagVector<JagPolygon> &pgvec, double &xmin, double &ymin, double &xmax, double &ymax );
	static bool getBBox3D( const JagVector<JagPolygon> &pgvec, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax );
	static bool getBBox2DInner( const JagVector<JagPolygon> &pgvec, double &xmin, double &ymin, double &xmax, double &ymax );
	static bool getBBox3DInner( const JagVector<JagPolygon> &pgvec, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax );
	static double getGeoLength( const JagFixString &lstr );
	static Jstr bboxstr( const JagStrSplit &sp, bool skipInnerRings );
	static void bbox2D( const JagStrSplit &sp, JagBox2D &box );
	static void bbox3D( const JagStrSplit &sp, JagBox3D &box );
	static int convertConstantObjToJAG( const JagFixString &instr, Jstr &outstr );
	static int numberOfSegments( const JagStrSplit &sp );
	static bool isPolygonCCW( const JagStrSplit &sp );
	static bool isPolygonCW( const JagStrSplit &sp );
	static void multiPolygonToWKT( const JagVector<JagPolygon> &pgvec, bool is3D, Jstr &wkt );
    static double meterToLon( int srid, double meter, double lon, double lat);
    static double meterToLat( int srid, double meter, double lon, double lat);
	static bool interpolatePoint2D(double segdist, double segfrac, const JagPoint3D &p1, const JagPoint3D &p2, JagPoint3D &point );
	static bool toJAG( const JagVector<JagPolygon> &pgvec, bool is3D, bool hasHdr, const Jstr &inbbox, int srid, Jstr &str );
	static void multiPolygonToVector2D( int srid, const JagVector<JagPolygon> &pgvec, bool outerRingOnly, JagVector<JagPoint2D> &vec );
	static void multiPolygonToVector3D( int srid, const JagVector<JagPolygon> &pgvec, bool outerRingOnly, JagVector<JagPoint3D> &vec );
	static void lonLatToXY( int srid, double lon, double lat, double &x, double &y );
	static void XYToLonLat( int srid, double x, double y, double &lon, double &lat );
	static void lonLatAltToXYZ( int srid,  double lon, double lat, double alt, double &x, double &y, double &z );
	static void XYZToLonLatAlt(  int srid, double x, double y, double z , double &lon, double &lat, double &alt );
	static void kNN2D(int srid, const std::vector<JagSimplePoint2D> &points, const JagSimplePoint2D &point, int K, 
						const JagMinMaxDistance &minmax, std::vector<JagSimplePoint2D> &neighbors );
	static void kNN3D(int srid, const std::vector<JagSimplePoint3D> &points, const JagSimplePoint3D &point, int K, 
						const JagMinMaxDistance &minmax, std::vector<JagSimplePoint3D> &neighbors );
	static void knnfromVec( const JagVector<JagPolygon> &pgvec, int dim, int srid, double px,double py,double pz,
							int K, double min, double max, Jstr &val );



  protected:
	static const double NUM_SAMPLE;
	static double doSign( const JagPoint2D &p1, const JagPoint2D &p2, const JagPoint2D &p3 );
	static double distSquarePointToSeg( const JagPoint2D &p,
										const JagPoint2D &p1, const JagPoint2D &p2 );
	static bool locIn3DCenterBox( double x, double y, double z, double a, double b, double c, bool strict ); 
	static bool locIn2DCenterBox( double x, double y, double a, double b, bool strict ); 
	static bool validDirection( double nx );
	static bool validDirection( double nx, double ny );

	static bool pointIntersectNormalEllipse( double px, double py, double w, double h, bool strict );
	static bool point3DIntersectNormalEllipsoid( double px, double py, double pz, 
											   double w, double d, double h, bool strict );
	static bool point3DIntersectNormalCone( double px, double py, double pz, 
										 double r, double h, bool strict );

	static double jagMin( double x1, double x2, double x3 );
	static double jagMax( double x1, double x2, double x3 );
	static bool bound2DDisjoint( double px1, double py1, double w1, double h1, double px2, double py2, double w2, double h2 );
	static bool bound3DDisjoint( 
				double px1, double py1, double pz1, double w1, double d1, double h1,
				double px2, double py2, double pz2, double w2, double d2, double h2 );

	static bool bound2DLineBoxDisjoint( double x10, double y10, double x20, double y20, 
				 						double x0, double y0, double w, double h );

	static bool bound3DLineBoxDisjoint( double x10, double y10, double z10,
				 						double x20, double y20, double z20, 
				 						double x0, double y0, double z0, double w, double d, double h );

	static bool bound2DTriangleDisjoint( double px1, double py1, double px2, double py2, double px3, double py3,
									     double px0, double py0, double w, double h );
	static bool bound3DTriangleDisjoint( double px1, double py1, double pz1, double px2, double py2, double pz2, 
										double px3, double py3, double pz3,
									     double px0, double py0, double pz0, double w, double d, double h );

	static double squaredDistancePoint2Line( double px, double py, double x1, double y1, double x2, double y2 );
	static int relationLineCircle( double x1, double y1, double x2, double y2, 
								    double x0, double y0, double r );
	static int relationLineEllipse( double x1, double y1, double x2, double y2, 
								    double x0, double y0, double a, double b, double nx );
	static int relationLineNormalEllipse( double x1, double y1, double x2, double y2, double a, double b );
	static double squaredDistance3DPoint2Line( double px, double py, double pz,
			double x1, double y1, double z1, double x2, double y2, double z2 );
	static int relationLine3DSphere( double x1, double y1, double z1, double x2, double y2, double z2,
									  double x3, double y3, double z3, double r );
	static int relationLine3DNormalEllipsoid( double x1, double y1, double z1,  
													double x2, double y2, double z2,
													double a, double b, double c );
	static int relationLine3DEllipsoid( double x1, double y1, double z1,  
													double x2, double y2, double z2,
													double x0, double y0, double z0,
													double a, double b, double c, double nx, double ny );

	static int relationLine3DCone( double x1, double y1, double z1, double x2, double y2, double z2,
								double x0, double y0, double z0,
								double a, double c, double nx, double ny );
	static int relationLine3DNormalCone( double x0, double y0, double z0,  double x1, double y1, double z1,
										double a, double c );

	static int relationLine3DCylinder( double x1, double y1, double z1, double x2, double y2, double z2,
								double x0, double y0, double z0,
								double a, double b, double c, double nx, double ny );
	static int relationLine3DNormalCylinder( double x0, double y0, double z0,  double x1, double y1, double z1,
										double a, double b, double c );


	static bool point3DWithinRectangle2D(  double px, double py, double pz, 
											double x0, double y0, double z0,
	 										double a, double b, double nx, double ny, bool strict );


	static void cornersOfRectangle( double w, double h, JagPoint2D p[] );
	static void cornersOfRectangle3D( double w, double h, JagPoint3D p[] );
	static void cornersOfBox( double w, double d, double h, JagPoint3D p[] );
	static void edgesOfRectangle( double w, double h, JagLine2D line[] );
	static void edgesOfRectangle3D( double w, double h, JagLine3D line[] );
	static void edgesOfBox( double w, double d, double h, JagLine3D line[] );
	static void edgesOfTriangle( double x1, double y1, double x2, double y2, double x3, double y3, JagLine2D line[] );
	static void edgesOfTriangle3D( double x1, double y1, double z1, double x2, double y2, double z2, 
									double x3, double y3, double z3, JagLine3D line[] );
	static void surfacesOfBox(double w, double dd, double h, JagRectangle3D rect[] );
	static void triangleSurfacesOfBox(double w, double dd, double h, JagTriangle3D tri[] );
	static void pointsOfTriangle3D( double x1, double y1, double z1, double x2, double y2, double z2, 
									double x3, double y3, double z3, JagPoint3D point[] );


	static void transform3DEdgesLocal2Global( double x0, double y0, double z0, double nx0, double ny0, 
											  int num, JagPoint3D[] );
	static void transform2DEdgesLocal2Global( double x0, double y0, double nx0, int num, JagPoint2D[] );

	static void transform3DLinesLocal2Global( double x0, double y0, double z0, double nx0, double ny0, 
											 int num, JagLine3D line[] );
	static void transform3DLinesGlobal2Local( double x0, double y0, double z0, double nx0, double ny0, 
											 int num, JagLine3D line[] );
	static void transform2DLinesLocal2Global( double x0, double y0, double nx0, int num, JagLine2D line[] );
	static void transform2DLinesGlobal2Local( double x0, double y0, double nx0, int num, JagLine2D line[] );

	static bool line2DIntersectNormalRectangle( const JagLine2D &line, double w, double h );
	static bool line2DIntersectRectangle( const JagLine2D &line, double x0, double y0, double w, double h, double nx );
	static bool line3DIntersectNormalRectangle( const JagLine3D &line, double w, double h );
	static bool line3DIntersectRectangle3D( const JagLine3D &line, 
										    double x0, double y0, double z0, double w, double h, double nx, double ny );
	static bool line3DIntersectRectangle3D( const JagLine3D &line, const JagRectangle3D &rect );

	static bool line3DIntersectEllipse3D( const JagLine3D &line, 
									      double x0, double y0, double w, double dd, double h, double nx, double ny );
	static bool line3DIntersectNormalEllipse( const JagLine3D &line, double w, double h );
	static bool line3DIntersectLine3D( double x1, double y1, double z1, double x2, double y2, double z2,
										double x3, double y3, double z3, double x4, double y4, double z4 );
	static bool line3DIntersectLine3D( const JagLine3D &line1, const JagLine3D &line2 );


	static double distanceFromPoint3DToPlane(  double x, double y, double z, 
									 double x0, double y0, double z0, double nx0, double ny0 );

	static void triangle3DNormal( double x1, double y1, double z1, double x2, double y2, double z2,
								  double x3, double y3, double z3, double &nx, double &ny );
	static void triangle3DABCD( double x1, double y1, double z1, double x2, double y2, double z2,
								  double x3, double y3, double z3, double &A, double &B, double &C, double &D );

	static double distancePoint3DToPlane( double x, double y, double z, double A, double B, double C, double D );
	static double distancePoint3DToTriangle3D( double x, double y, double z, 
									double x1, double y1, double z1, double x2, double y2, double z2,
									double x3, double y3, double z3 );

	static void planeABCDFromNormal( double x0, double y0, double z0, double nx, double ny, 
								    double &A, double &B, double &C, double &D );
			
	static bool planeIntersectNormalEllipsoid( double A, double B, double C, double D,
										  double a,  double b,  double c );
	static bool planeIntersectNormalCone( double A, double B, double C, double D,
										  double R,  double h );
	static void getCoordAvg( const Jstr& colType, const JagStrSplit &sp, double &x, double &y, double &z, 
							double &Rx, double &Ry, double &Rz );

	static void triangleRegion( double x1, double y1, double x2, double y2, double x3, double y3,
								double &x0, double &y0, double &Rx, double &Ry );
	static void triangle3DRegion( double x1, double y1, double z1, 
								  double x2, double y2, double z2,
								  double x3, double y3, double z3,
								double &x0, double &y0, double &z0, double &Rx, double &Ry, double &Rz );

	static void boundingBoxRegion( const Jstr &bbxstr, double &bbx, double &bby, double &brx, double &bry );
	static void boundingBox3DRegion( const Jstr &bbxstr, double &bbx, double &bby, double &bbz, 
									double &brx, double &bry, double &brz );
	static void prepareKMP( const JagStrSplit &sp, int start, int M, int *lps );
	static int KMPPointsWithin( const JagStrSplit &sp1, int start1, const JagStrSplit &sp2, int start2 );
	static short orientation(double x1, double y1, double x2, double y2, double x3, double y3 );
	static short orientation(double x1, double y1, double z1, double x2, double y2, double z2, 
							 double x3, double y3, double z3 );
    static void getPolygonBound( const Jstr &mk, const JagStrSplit &sp, double &bbx, double &bby, 
								  double &rx, double &ry );
    static void getLineStringBound( const Jstr &mk, const JagStrSplit &sp, double &bbx, double &bby, 
								  double &rx, double &ry );

	static double dotProduct( const JagPoint2D &p1, const JagPoint2D &p2 ); 
	static double dotProduct( const JagPoint3D &p1, const JagPoint3D &p2 ); 
	static void minusVector( const JagPoint3D &v1, const JagPoint3D &v2, JagPoint3D &pt );

	static int line3DIntersectTriangle3D( const JagLine3D& line3d, const JagPoint3D &p1, 
										  const JagPoint3D &p2, const JagPoint3D &p3,
										  JagPoint3D &atPoint );

	static int getBarycentric( const JagPoint3D &pt, const JagPoint3D &a, const JagPoint3D &b, const JagPoint3D &c,
							   double &u, double &v, double &w);

	static bool point3DWithinTriangle3D( double x, double y, double z, 
										 const JagPoint3D &p1, const JagPoint3D &p2, const JagPoint3D &p3 );


	static double computePolygonArea( const JagVector<std::pair<double,double>> &vec );
	static double computePolygonPerimeter( const JagVector<std::pair<double,double>> &vec, int srid );
	static double computePolygon3DPerimeter( const JagVector<JagPoint3D> &vec, int srid );
	static bool lineStringAverage( const Jstr &mk, const JagStrSplit &sp, double &x, double &y );
	static bool lineString3DAverage( const Jstr &mk, const JagStrSplit &sp, double &x, double &y, double &z );
	static void findMinBoundary( double d1,  double d2,  double d3,  double d4, double midx, double midy,
	                              double &left, double &right, double &up, double &down );
	static void findMaxBoundary( double d1,  double d2,  double d3,  double d4, double midx, double midy,
	                              double &left, double &right, double &up, double &down );

	static double minPoint2DToLineSegDistance( double px, double py, double x1, double y1, double x2, double y2, int srid,
											   double &projx, double  &projy );
	static double minPoint3DToLineSegDistance( double px, double py, double pz, 
											   double x1, double y1, double z1, 
											   double x2, double y2, double z2, int srid,
											   double &projx, double  &projy, double &projz);

	static bool  closestPoint2DPolygon( int srid, double px, double py, const Jstr &mk, 
										const JagStrSplit &sp, Jstr &res );
	static bool  closestPoint2DRaster( int srid, double px, double py, const Jstr &mk, 
										const JagStrSplit &sp, Jstr &res );
	static bool  closestPoint3DPolygon( int srid, double px, double py, double pz, const Jstr &mk, 
										const JagStrSplit &sp, Jstr &res );
	static bool  closestPoint3DRaster( int srid, double px, double py, double pz, const Jstr &mk, 
										const JagStrSplit &sp, Jstr &res );
	static bool  closestPoint2DMultiPolygon( int srid, double px, double py, const Jstr &mk, 
										const JagStrSplit &sp, Jstr &res );
	static bool  closestPoint3DMultiPolygon( int srid, double px, double py, double pz, const Jstr &mk, 
										const JagStrSplit &sp, Jstr &res );

	static bool  closestPoint3DBox( int srid, double px, double py, double pz, double x0, double y0, double z0,
								    double a, double b, double c, double nx, double ny, double &dist, Jstr &res );

	static bool  matchPoint2D( double px, double py, const JagStrSplit &sp, Jstr &xs, Jstr &ys );
	static bool  matchPoint3D( double px, double py, double pz, const JagStrSplit &sp, 
							   Jstr &xs, Jstr &ys, Jstr &zs );
	static void  nonMatchPoint2DVec( double px, double py, const JagStrSplit &sp, Jstr &vecs );
	static void  nonMatchPoint3DVec( double px, double py, double pz, const JagStrSplit &sp, Jstr &vecs );
	static void  nonMatchTwoPoint2DVec( double px1, double py1, double px2, double py2, const JagStrSplit &sp, Jstr &vecs );
	static void  nonMatchTwoPoint3DVec( double px1, double py1, double pz1,  double px2, double py2, double pz2,
										const JagStrSplit &sp, Jstr &vecs );

	static bool  line2DLineStringIntersection( const JagLine2D &line1, const JagStrSplit &sp, JagVector<JagPoint2D> &vec );
	static bool  line3DLineStringIntersection( const JagLine3D &line1, const JagStrSplit &sp, JagVector<JagPoint3D> &vec );

	static void  appendLine2DLine2DIntersection(double x1, double y1, double x2, double y2, 
												double mx1, double my1, double mx2, double my2, JagVector<JagPoint2D> &vec ); 
	static void  appendLine3DLine3DIntersection(double x1, double y1, double z1, double x2, double y2, double z2, 
												double mx1, double my1, double mz1, double mx2, double my2, double mz2, 
												JagVector<JagPoint3D> &vec ); 

	static void splitPolygonToVector( const JagPolygon &pgon, bool is3D, JagVector<Jstr> &svec );
	static void getIntersectionPoints( const JagVector<JagPoint3D> &vec1, const JagVector<JagPoint3D> &vec2, 
									   JagVector<JagPoint3D> &resvec );
	static void getIntersectionPoints( const JagVector<JagPoint3D> &vec1, const JagStrSplit &sp,
									   bool is3D, JagVector<JagPoint3D> &resvec );
	static void getIntersectionPoints( const JagVector<JagPoint3D> &vec1, const JagVector<JagPoint3D> &vec2, 
									   JagHashSetStr &hashset );
	static void getIntersectionPoints( const JagVector<JagPoint3D> &vec1, const JagStrSplit &sp,
									   bool is3D, JagHashSetStr &hashset );

	static void vectorToHash( const JagVector<JagPoint3D> &resvec, JagHashSetStr &hashset );

	static void getVectorPoints( const JagStrSplit &sp, bool is3D, JagVector<JagPoint3D> &resvec );

	static double getMinDist2DPointFraction( double px, double py, int srid, const JagStrSplit &sp, 
							double &mindist, double &minx, double &miny );
	static double getMinDist3DPointFraction( double px, double py, double pz, int srid, const JagStrSplit &sp, 
							double &mindist, double &minx, double &miny, double &minz );

	static void getMinDist2DPointOnPloygonAsLineStrings( int srid, double px, double py, const JagPolygon &pgon, 
													     double &minx, double &miny );
	static void getMinDist3DPointOnPloygonAsLineStrings( int srid, double px, double py, double pz, const JagPolygon &pgon, 
													     double &minx, double &miny, double &minz );

	static void kNN2DCart(const std::vector<JagSimplePoint2D> &points, const JagSimplePoint2D &point, int K, 
						const JagMinMaxDistance &minmax, std::vector<JagSimplePoint2D> &neighbors );
	static void kNN2DWGS84(const std::vector<JagSimplePoint2D> &points, const JagSimplePoint2D &point, int K, 
						const JagMinMaxDistance &minmax, std::vector<JagSimplePoint2D> &neighbors );

	static void kNN3DCart( const std::vector<JagSimplePoint3D> &points, const JagSimplePoint3D &point, int K, 
						const JagMinMaxDistance &minmax, std::vector<JagSimplePoint3D> &neighbors );
	static void kNN3DWGS84( const std::vector<JagSimplePoint3D> &points, const JagSimplePoint3D &point, int K, 
						const JagMinMaxDistance &minmax, std::vector<JagSimplePoint3D> &neighbors );

    static int  add3DPointsToStr( Jstr &val, const JagVector<JagSimplePoint3D> &vec );

};  // end of class JagGeo

class DistanceCalculator2DWGS84 
{
  public:
  	explicit DistanceCalculator2DWGS84() { }
  	double operator() (const JagSimplePoint2D& a, const JagSimplePoint2D& b) const 
	{
    	double dist;
    	_geod.Inverse(a.y, a.x, b.y, b.x, dist);
    	return dist;
  	}
  private:
    const Geodesic &_geod = Geodesic::WGS84();
};

class DistanceCalculator2DCart
{
  public:
  	explicit DistanceCalculator2DCart() { }
  	double operator() (const JagSimplePoint2D& a, const JagSimplePoint2D& b) const 
	{
    	return sqrt( (a.x-b.x)*(a.x-b.x) + (a.y-b.y )*(a.y-b.y) );
  	}
};


class DistanceCalculator3DWGS84 
{
  public:
  	explicit DistanceCalculator3DWGS84() { }
  	double operator() (const JagSimplePoint3D& a, const JagSimplePoint3D& b) const 
	{
    	return JagGeo::distance( a.x, a.y, a.z, b.x, b.y, b.z, JAG_GEO_WGS84 );
  	}
  private:
    const Geodesic &_geod = Geodesic::WGS84();
};
class DistanceCalculator3DCart
{
  public:
  	explicit DistanceCalculator3DCart() { }
  	double operator() (const JagSimplePoint3D& a, const JagSimplePoint3D& b) const 
	{
    	return sqrt( (a.x-b.x)*(a.x-b.x) + (a.y-b.y )*(a.y-b.y) + (a.z-b.z )*(a.z-b.z) );
  	}
};
#endif
