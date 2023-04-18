#include <JagGlobalDef.h>
#include <JagCGAL.h>
#include <JagParser.h>
#include <JagUtil.h>
#include <ACConcaveHull.h>

/**
    https://www.boost.org/doc/libs/1_81_0/libs/geometry/doc/html/geometry/reference/algorithms.html
	https://www.boost.org/doc/libs/1_81_0/libs/geometry/doc/html/geometry/reference/algorithms/intersects/intersects_2_two_geometries.html
	https://www.boost.org/doc/libs/1_81_0/libs/geometry/doc/html/geometry/reference/algorithms/is_valid/is_valid_2_with_strategy.html
	https://www.boost.org/doc/libs/1_81_0/libs/geometry/doc/html/geometry/reference/algorithms/is_simple/is_simple_2_with_strategy.html
	https://www.boost.org/doc/libs/1_81_0/libs/geometry/doc/html/geometry/reference/algorithms/envelope/envelope_3_with_strategy.html polygon
**/


void JagCGAL::getConvexHull2DStr( const JagLineString &line, const Jstr &hdr, const Jstr &bbox, Jstr &value )
{
	JagLineString hull;
	getConvexHull2D( line, hull );
	const JagVector<JagPoint> &point = hull.point;
	Jstr sx, sy;

	JagStrSplit ss(hdr, '=');
	value = Jstr("CJAG=") + ss[1] + "=0=PL=d " + bbox;
	int len = point.size();
	for ( int i = 0; i <  len; ++i ) {
		sx = point[i].x; sy = point[i].y; 
		value += Jstr(" ") + sx + ":" + sy;
	}

	if ( ! (jagEQ(jagatof(point[0].x), jagatof(point[len-1].x) )
	       && jagEQ(jagatof(point[0].y), jagatof(point[len-1].y) ) )) {
		value += Jstr(" ") + point[0].x + ":" + point[0].y;
	}
}

void JagCGAL::getConvexHull2D( const JagLineString &line, JagLineString &hull )
{
	std::vector<CGALKernel::Point_2> vpoints, vresult;
	const JagVector<JagPoint> &point = line.point;
	for ( int i = 0; i <  point.size(); ++i ) {
		vpoints.push_back(  CGALKernel::Point_2( jagatof(point[i].x), jagatof(point[i].y) ));
	}

	CGAL::convex_hull_2( vpoints.begin(), vpoints.end(), std::back_inserter(vresult) );
	for( int i = 0; i < vresult.size(); i++) {
		hull.add( vresult[i].x(), vresult[i].y() );
	}
}

void JagCGAL::getConvexHull3D( const JagLineString &line, JagLineString &hull )
{
	std::vector<CGALKernel::Point_3> vpoints;
	const JagVector<JagPoint> &point = line.point;
	for ( int i = 0; i <  point.size(); ++i ) {
		vpoints.push_back(  CGALKernel::Point_3( jagatof(point[i].x), jagatof(point[i].y), jagatof(point[i].z) ));
	}

	CGALSurfaceMesh sm;
	CGAL::convex_hull_3( vpoints.begin(), vpoints.end(), sm );

	BOOST_FOREACH( CGALSurfaceMesh::Vertex_index vi, sm.vertices()){
	    hull.add( sm.point(vi).x(), sm.point(vi).y(), sm.point(vi).z() );
    }
}

void JagCGAL::getConvexHull3DStr( const JagLineString &line, const Jstr &hdr, const Jstr &bbox, Jstr &value )
{
	JagLineString hull;
	getConvexHull3D( line, hull );
	const JagVector<JagPoint> &point = hull.point;
	Jstr sx, sy, sz;
	JagStrSplit ss(hdr, '=');
	value = Jstr("CJAG=") + ss[1] + "=0=PL3=d " + bbox;

	int len = point.size();
	for ( int i = 0; i <  len; ++i ) {
		sx = point[i].x; sy = point[i].y; sz = point[i].z; 
		value += Jstr(" ") + sx + ":" + sy + ":" + sz;
	}
	if ( ! (jagEQ(jagatof(point[0].x), jagatof(point[len-1].x) )
	       && jagEQ(jagatof(point[0].y), jagatof(point[len-1].y) ) 
	       && jagEQ(jagatof(point[0].z), jagatof(point[len-1].z) ) ) 
		   ) {
		value += Jstr(" ") + point[0].x + ":" + point[0].y + ":" +  point[0].z;
	}
}

template <class TGeo>
bool JagCGAL::getBuffer2D( const TGeo &ls, const Jstr &arg, JagVector<JagPolygon> &mpgon )
{
	bool rc;
	JagStrategy *sptr[5];
	rc = createStrategies( sptr, arg );
	if ( ! rc ) {
		return false;
	}

	boost::geometry::model::multi_polygon<BoostPolygon2D> result;
	if ( sptr[0]->t == JAG_DIST_SYMMETRIC && sptr[2]->t == JAG_JOIN_ROUND && sptr[3]->t == JAG_END_ROUND && sptr[4]->t==JAG_POINT_CIRCLE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceSymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinRound*)sptr[2]->ptr), 
							 *( (JagEndRound*)sptr[3]->ptr), *( (JagPointCircle*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_ASYMMETRIC && sptr[2]->t == JAG_JOIN_ROUND && sptr[3]->t == JAG_END_ROUND && sptr[4]->t==JAG_POINT_CIRCLE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceASymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinRound*)sptr[2]->ptr), 
							 *( (JagEndRound*)sptr[3]->ptr), *( (JagPointCircle*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_SYMMETRIC && sptr[2]->t == JAG_JOIN_MITER && sptr[3]->t == JAG_END_ROUND && sptr[4]->t==JAG_POINT_CIRCLE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceSymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinMiter*)sptr[2]->ptr), 
							 *( (JagEndRound*)sptr[3]->ptr), *( (JagPointCircle*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_ASYMMETRIC && sptr[2]->t == JAG_JOIN_MITER && sptr[3]->t == JAG_END_ROUND && sptr[4]->t==JAG_POINT_CIRCLE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceASymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinMiter*)sptr[2]->ptr), 
							 *( (JagEndRound*)sptr[3]->ptr), *( (JagPointCircle*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_SYMMETRIC && sptr[2]->t == JAG_JOIN_ROUND && sptr[3]->t == JAG_END_FLAT && sptr[4]->t==JAG_POINT_CIRCLE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceSymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinRound*)sptr[2]->ptr), 
							 *( (JagEndFlat*)sptr[3]->ptr), *( (JagPointCircle*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_SYMMETRIC && sptr[2]->t == JAG_JOIN_MITER && sptr[3]->t == JAG_END_FLAT && sptr[4]->t==JAG_POINT_CIRCLE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceSymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinMiter*)sptr[2]->ptr), 
							 *( (JagEndFlat*)sptr[3]->ptr), *( (JagPointCircle*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_ASYMMETRIC && sptr[2]->t == JAG_JOIN_ROUND && sptr[3]->t == JAG_END_FLAT && sptr[4]->t==JAG_POINT_CIRCLE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceASymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinRound*)sptr[2]->ptr), 
							 *( (JagEndFlat*)sptr[3]->ptr), *( (JagPointCircle*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_ASYMMETRIC && sptr[2]->t == JAG_JOIN_MITER && sptr[3]->t == JAG_END_FLAT && sptr[4]->t==JAG_POINT_CIRCLE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceASymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinMiter*)sptr[2]->ptr), 
							 *( (JagEndFlat*)sptr[3]->ptr), *( (JagPointCircle*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_SYMMETRIC && sptr[2]->t == JAG_JOIN_ROUND && sptr[3]->t == JAG_END_ROUND && sptr[4]->t == JAG_POINT_SQUARE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceSymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinRound*)sptr[2]->ptr), 
							 *( (JagEndRound*)sptr[3]->ptr), *( (JagPointSquare*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_ASYMMETRIC && sptr[2]->t == JAG_JOIN_ROUND && sptr[3]->t == JAG_END_ROUND && sptr[4]->t == JAG_POINT_SQUARE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceASymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinRound*)sptr[2]->ptr), 
							 *( (JagEndRound*)sptr[3]->ptr), *( (JagPointSquare*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_SYMMETRIC && sptr[2]->t == JAG_JOIN_MITER && sptr[3]->t == JAG_END_ROUND && sptr[4]->t == JAG_POINT_SQUARE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceSymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinMiter*)sptr[2]->ptr), 
							 *( (JagEndRound*)sptr[3]->ptr), *( (JagPointSquare*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_ASYMMETRIC && sptr[2]->t == JAG_JOIN_MITER && sptr[3]->t == JAG_END_ROUND && sptr[4]->t == JAG_POINT_SQUARE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceASymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinMiter*)sptr[2]->ptr), 
							 *( (JagEndRound*)sptr[3]->ptr), *( (JagPointSquare*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_SYMMETRIC && sptr[2]->t == JAG_JOIN_ROUND && sptr[3]->t == JAG_END_FLAT && sptr[4]->t == JAG_POINT_SQUARE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceSymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinRound*)sptr[2]->ptr), 
							 *( (JagEndFlat*)sptr[3]->ptr), *( (JagPointSquare*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_SYMMETRIC && sptr[2]->t == JAG_JOIN_MITER && sptr[3]->t == JAG_END_FLAT && sptr[4]->t == JAG_POINT_SQUARE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceSymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinMiter*)sptr[2]->ptr), 
							 *( (JagEndFlat*)sptr[3]->ptr), *( (JagPointSquare*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_ASYMMETRIC && sptr[2]->t == JAG_JOIN_ROUND && sptr[3]->t == JAG_END_FLAT && sptr[4]->t == JAG_POINT_SQUARE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceASymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinRound*)sptr[2]->ptr), 
							 *( (JagEndFlat*)sptr[3]->ptr), *( (JagPointSquare*)sptr[4]->ptr) );
	} else if ( sptr[0]->t == JAG_DIST_ASYMMETRIC && sptr[2]->t == JAG_JOIN_MITER && sptr[3]->t == JAG_END_FLAT && sptr[4]->t == JAG_POINT_SQUARE ) {
		boost::geometry::buffer( ls, result, *( (JagDistanceASymmetric*)sptr[0]->ptr), 
							 *( (JagSideStraight*)sptr[1]->ptr), *( (JagJoinMiter*)sptr[2]->ptr), 
							 *( (JagEndFlat*)sptr[3]->ptr), *( (JagPointSquare*)sptr[4]->ptr) );
	} else {
	}
	
	boost::geometry::model::multi_polygon<BoostPolygon2D>::iterator iter; 
	for ( iter = result.begin(); iter != result.end(); ++iter ) {
		const BoostRing2D &ring = iter->outer();
		JagLineString3D lstr;
		for ( auto it = boost::begin(ring); it != boost::end(ring); ++it ) {
			lstr.add( it->x(), it->y() );
		}
		JagPolygon pgon;
		pgon.linestr.append( lstr );
		mpgon.append( pgon );
	}

	destroyStrategies( sptr );
	return true;
}

bool JagCGAL::getBufferLineString2DStr( const JagLineString &line, int srid, const Jstr &arg, Jstr &value )
{
	JagVector<JagPolygon> pgvec;
	boost::geometry::model::linestring<BoostPoint2D> ls;
	for ( int i=0; i < line.size(); ++i ) {
		boost::geometry::append( ls, BoostPoint2D( jagatof(line.point[i].x), jagatof(line.point[i].y) ) );
	}

	bool rc = getBuffer2D<boost::geometry::model::linestring<BoostPoint2D> >( ls, arg, pgvec );
	if ( ! rc ) {
		return false;
	}

	rc = get2DStrFromMultiPolygon( pgvec, srid, value );
	return rc;

}

bool JagCGAL::getBufferMultiPoint2DStr( const JagLineString &line, int srid, const Jstr &arg, Jstr &value )
{
	boost::geometry::model::multi_point<BoostPoint2D> mpoint;
	for ( int i=0; i < line.size(); ++i ) {
		boost::geometry::append( mpoint, BoostPoint2D( jagatof(line.point[i].x), jagatof(line.point[i].y) ) );
	}
	JagVector<JagPolygon> pgvec;
	bool rc = getBuffer2D<boost::geometry::model::multi_point<BoostPoint2D> >( mpoint, arg, pgvec );
	if ( ! rc ) {
		return false;
	}

	rc = get2DStrFromMultiPolygon( pgvec, srid, value );
	return rc;
}

bool JagCGAL::getBufferPolygon2DStr(  const JagPolygon &pgon, int srid, const Jstr &arg, Jstr &value )
{
	boost::geometry::model::polygon<BoostPoint2D,false> bgon;
	convertPolygonJ2B( pgon,bgon);
	JagVector<JagPolygon> pgvec;
	bool rc = getBuffer2D<boost::geometry::model::polygon<BoostPoint2D,false> >( bgon, arg, pgvec );
	if ( ! rc ) {
		return false;
	}

	rc = get2DStrFromMultiPolygon( pgvec, srid, value );
	return rc;
}

bool JagCGAL::getBufferMultiLineString2DStr(  const JagPolygon &pgon, int srid, const Jstr &arg, Jstr &value )
{
	boost::geometry::model::multi_linestring<BoostLineString2D> mlstr;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &linestr = pgon.linestr[i];
		std::vector< BoostPoint2D > pointList; 
		for (  int j=0; j< linestr.size(); ++j ) {
			BoostPoint2D p2( linestr.point[j].x, linestr.point[j].y );
			pointList.push_back(p2);
		}

		boost::geometry::append( mlstr, pointList );
	}

	JagVector<JagPolygon> pgvec;
	bool rc = getBuffer2D<boost::geometry::model::multi_linestring<BoostLineString2D> >( mlstr, arg, pgvec );
	if ( ! rc ) {
		return false;
	}

	rc = get2DStrFromMultiPolygon( pgvec, srid, value );
	return rc;
}

bool JagCGAL::getBufferMultiPolygon2DStr(  const JagVector<JagPolygon> &pgvec, int srid, const Jstr &arg, Jstr &value )
{
	boost::geometry::model::multi_polygon<BoostPolygon2D> mbgon;
	std::string ss;
	for ( int k=0; k < pgvec.size(); ++k ) {
		boost::geometry::model::polygon<BoostPoint2D,false> bgon;
		const JagPolygon &pgon = pgvec[k];
		convertPolygonJ2B( pgon,bgon);
    	mbgon.push_back( bgon );
	}

	JagVector<JagPolygon> pgvec2;
	bool rc = getBuffer2D<boost::geometry::model::multi_polygon<BoostPolygon2D> >( mbgon, arg, pgvec2 );
	if ( ! rc ) {
		return false;
	}

	rc = get2DStrFromMultiPolygon( pgvec2, srid, value );
	return rc;
}


bool JagCGAL::get2DStrFromMultiPolygon( const JagVector<JagPolygon> &pgvec, int srid, Jstr &value )
{
	if ( pgvec.size() < 1 ) {
		return false;
	}

	double xmin, ymin, xmax, ymax;
	Jstr s1, s2, s3, s4;
	bool rc = JagGeo::getBBox2D( pgvec, xmin, ymin, xmax, ymax );
	if ( ! rc ) return false;
	Jstr newbbox;
	s1 = doubleToStr( xmin ).trimEndZeros();
	s2 = doubleToStr( ymin ).trimEndZeros();
	s3 = doubleToStr( xmax ).trimEndZeros();
	s4 = doubleToStr( ymax ).trimEndZeros();
	newbbox = s1 + ":" + s2 + ":" + s3 + ":" + s4;
	Jstr nhdr = Jstr(JAG_OJAG) + "=" + intToStr(srid) + "=dummy.dummy.dummy=MG=d";
	value = nhdr + " " + newbbox;
	Jstr sx, sy;

	/***
	const JagPolygon &pgon = pgvec[0];
	const JagLineString3D &linestr = pgon.linestr[0];
	for ( int i=0; i < linestr.size(); ++i ) {
		value += Jstr(" ") + doubleToStr(linestr.point[i].x) + ":" 
			     +  doubleToStr(linestr.point[i].y);
	}
	***/
	for ( int i = 0; i < pgvec.size(); ++i ) {
		const JagPolygon &pgon = pgvec[i];
		if ( i > 0 ) { value += "!"; }
		for ( int j=0; j < pgon.size(); ++j ) {
			const JagLineString3D &linestr = pgon.linestr[j];
			if ( j > 0 ) { value += "|"; }
			for ( int k=0; k < linestr.size(); ++k ) {
				value += Jstr(" ") + doubleToStr(linestr.point[k].x) + ":" 
			     		 +  doubleToStr(linestr.point[k].y);
			}
		}
	}

	return true;
}


bool JagCGAL::createStrategies( JagStrategy *sptr[], const Jstr &arg )
{
	JagStrSplit sp( arg, ',', true );
	Jstr eq;
	int cnt = 0;
	bool hasDistance = false;
	bool hasSide = false;
	bool hasJoin = false;
	bool hasEnd = false;
	bool hasPoint = false;
	for ( int i=0; i < 5; ++i ) { sptr[i] = NULL; }

	for ( int i=0; i < sp.length(); ++i ) {
		eq = sp[i];
		eq.remove(' ');
		JagStrSplit speq(eq, '=');
		if ( speq.length() < 2 ) { continue; }

		if ( speq[0].caseEqual("distance") ) {
			JagStrSplit s3(speq[1], ':');
			if ( s3.length() < 2 ) continue;
			if ( s3[0] == "symmetric" ) {
				sptr[0] = new JagStrategy("distance_symmetric", jagatof( s3[1] ), 0 );
				++cnt;
			} else {
				if ( s3.length() < 3 ) continue;
				sptr[0] = new JagStrategy("distance_asymmetric", jagatof( s3[1] ), jagatof( s3[2] ) );
				++cnt;
			}
			hasDistance = true;
		} else if ( speq[0].caseEqual("side") ) {
			sptr[1] = new JagStrategy("side_straight", 0, 0 );
			++cnt;
			hasSide = true;
		} else if ( speq[0].caseEqual("join") ) {
			JagStrSplit s3(speq[1], ':');
			if ( s3.length() < 2 ) continue;
			if ( s3[0] == "round" ) {
				sptr[2] = new JagStrategy("join_round", jagatof( s3[1] ), 0 );
				++cnt;
			} else {
				sptr[2] = new JagStrategy("join_miter", jagatof( s3[1] ), 0 );
				++cnt;
			}
			hasJoin = true;
		} else if ( speq[0].caseEqual("end") ) {
			if ( speq[1] == "flat" ) {
				sptr[3] = new JagStrategy("end_flat", 0, 0 );
				++cnt;
			} else {
				JagStrSplit s3(speq[1], ':');
				if ( s3.length() < 2 ) continue;
				sptr[3] = new JagStrategy("end_round", jagatof( s3[1] ), 0 );
				++cnt;
			}
			hasEnd = true;
		} else if ( speq[0].caseEqual("point") ) {
			if ( speq[1] == "square" ) {
				sptr[4] = new JagStrategy("point_square", 0, 0 );
				++cnt;
			} else {
				JagStrSplit s3(speq[1], ':');
				if ( s3.length() < 2 ) continue;
				sptr[4] = new JagStrategy("point_circle", jagatof( s3[1] ), 0 );
				++cnt;
			}
			hasPoint = true;
		}
	}

	if ( ! hasDistance ) {
		sptr[0] = new JagStrategy("distance_symmetric", 10.0, 0 );
		++cnt;
	}

	if ( ! hasSide ) {
		sptr[1] = new JagStrategy("side_straight", 0, 0 );
		++cnt;
	}

	if ( ! hasJoin ) {
		sptr[2] = new JagStrategy("join_round", 10, 0 );
		++cnt;
	}

	if ( ! hasEnd ) {
		sptr[3] = new JagStrategy("end_round", 10, 0 );
		++cnt;
	}

	if ( ! hasPoint ) {
		sptr[4] = new JagStrategy("point_circle", 10, 0 );
		++cnt;
	}

	return true;
}

void JagCGAL::destroyStrategies( JagStrategy *sptr[] )
{
	for ( int i=0; i < 5; ++i ) { 
		if ( sptr[i] ) { delete sptr[i]; sptr[i] = NULL; }
	}
}

JagStrategy::JagStrategy( const Jstr &nm, double a1, double a2 )
{
	name = nm;
	f1 = a1;
	f2 = a2;
	if ( name == "distance_symmetric" ) {
		JagDistanceSymmetric *p = new JagDistanceSymmetric(f1);
		ptr = (void*)p;
		t = JAG_DIST_SYMMETRIC;
	} else if ( name == "distance_asymmetric" ) {
		JagDistanceASymmetric *p = new JagDistanceASymmetric(f1,f2);
		ptr = (void*)p;
		t = JAG_DIST_ASYMMETRIC;
	} else if ( name == "side_straight" ) {
		JagSideStraight *p = new JagSideStraight();
		ptr = (void*)p;
		t = JAG_SIDE_STRAIGHT;
	} else if ( name == "join_round" ) {
		JagJoinRound *p = new JagJoinRound(int(f1));
		ptr = (void*)p;
		t = JAG_JOIN_ROUND;;
	} else if ( name == "join_miter" ) {
		JagJoinMiter *p = new JagJoinMiter(f1);
		ptr = (void*)p;
		t = JAG_JOIN_MITER;;
	} else if ( name == "end_round" ) {
		JagEndRound *p = new JagEndRound(int(f1));
		ptr = (void*)p;
		t = JAG_END_ROUND;
	} else if ( name == "end_flat" ) {
		JagEndFlat *p = new JagEndFlat();
		ptr = (void*)p;
		t = JAG_END_FLAT;
	} else if ( name == "point_circle" ) {
		JagPointCircle *p = new JagPointCircle((int)f1);
		ptr = (void*)p;
		t = JAG_POINT_CIRCLE;
	} else if ( name == "point_square" ) {
		JagPointSquare *p = new JagPointSquare();
		ptr = (void*)p;
		t = JAG_POINT_SQUARE;
	} else {
		ptr = NULL;
		t = 0;
	}

}

JagStrategy::~JagStrategy()
{
	if ( name == "distance_symmetric" ) {
		JagDistanceSymmetric *p;
		p = (JagDistanceSymmetric*)ptr;
		delete p;
	} else if ( name == "distance_asymmetric" ) {
		JagDistanceASymmetric *p;
		p = (JagDistanceASymmetric*)ptr;
		delete p;
	} else if ( name == "side_straight" ) {
		JagSideStraight *p;
		p = (JagSideStraight*)ptr;
		delete p;
	} else if ( name == "join_round" ) {
		JagJoinRound *p;
		p = (JagJoinRound*)ptr;
		ptr = (void*)p;
	} else if ( name == "join_miter" ) {
		JagJoinMiter *p;
		p = new JagJoinMiter(f1);
		delete p;
	} else if ( name == "end_round" ) {
		JagEndRound *p;
		p = (JagEndRound*)ptr;
		delete p;
	} else if ( name == "end_flat" ) {
		JagEndFlat *p;
		p = (JagEndFlat*)ptr;
		delete p;
	} else if ( name == "point_circle" ) {
		JagPointCircle *p;
		p = (JagPointCircle*)ptr;
		delete p;
	} else if ( name == "point_square" ) {
		JagPointSquare *p;
		p = (JagPointSquare*)ptr;
		delete p;
	} else {
	}

}

bool JagCGAL::getIsSimpleLineString2DStr( const JagLineString &line )
{
	boost::geometry::model::linestring<BoostPoint2D> ls;
	for ( int i=0; i < line.size(); ++i ) {
		boost::geometry::append( ls, BoostPoint2D( jagatof(line.point[i].x), jagatof(line.point[i].y) ) );
	}

	bool rc = boost::geometry::is_simple( ls );
	return rc;
}

bool JagCGAL::isPolygonConvex(  const JagPolygon &pgon ) 
{
	using SS = boost::geometry::model::polygon<BoostPoint2D,false>;

	boost::geometry::model::polygon<BoostPoint2D,false> bgon;
	if ( ! convertPolygonJ2B( pgon,bgon) ) return false;
	bool rc = boost::geometry::is_convex( bgon.outer() );
	return rc;

}

bool JagCGAL::getIsSimplePolygon2DStr(  const JagPolygon &pgon ) 
{
	boost::geometry::model::polygon<BoostPoint2D,false> bgon;
	if ( ! convertPolygonJ2B( pgon,bgon) ) return false;
	bool rc = boost::geometry::is_simple( bgon );
	return rc;
}

bool JagCGAL::getIsSimpleMultiLineString2DStr(  const JagPolygon &pgon )
{
	boost::geometry::model::multi_linestring<BoostLineString2D> mlstr;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &linestr = pgon.linestr[i];
		std::vector< BoostPoint2D > pointList; 
		for (  int j=0; j< linestr.size(); ++j ) {
			BoostPoint2D p2( linestr.point[j].x, linestr.point[j].y );
			pointList.push_back(p2);
		}

		boost::geometry::append( mlstr, pointList );
	}

	bool rc = boost::geometry::is_simple( mlstr );
	return rc;
}

bool JagCGAL::getIsSimpleMultiPolygon2DStr(  const JagVector<JagPolygon> &pgvec )
{
	boost::geometry::model::multi_polygon<BoostPolygon2D> mbgon;
	for ( int k=0; k < pgvec.size(); ++k ) {
		boost::geometry::model::polygon<BoostPoint2D,false> bgon;
		const JagPolygon &pgon = pgvec[k];
		if ( ! convertPolygonJ2B( pgon,bgon) ) return false;
    	mbgon.push_back( bgon );
	}

	bool rc = boost::geometry::is_simple( mbgon );
	return rc;
}


bool JagCGAL::getIsValidLineString2DStr( const JagLineString &line )
{
	boost::geometry::model::linestring<BoostPoint2D> ls;
	for ( int i=0; i < line.size(); ++i ) {
		boost::geometry::append( ls, BoostPoint2D( jagatof(line.point[i].x), jagatof(line.point[i].y) ) );
	}

	boost::geometry::validity_failure_type failure;
	bool rc = boost::geometry::is_valid( ls, failure );
	return rc;
}

bool JagCGAL::getIsValidPolygon2DStr(  const JagPolygon &pgon ) 
{
	boost::geometry::model::polygon<BoostPoint2D,false> bgon;
	if ( ! convertPolygonJ2B( pgon, bgon ) ) return 0;
	boost::geometry::validity_failure_type failure;
	bool rc = boost::geometry::is_valid( bgon, failure );
	return rc;
}

bool JagCGAL::getIsValidMultiLineString2DStr(  const JagPolygon &pgon )
{
	boost::geometry::model::multi_linestring<BoostLineString2D> mlstr;
	for ( int i=0; i < pgon.size(); ++i ) {
		const JagLineString3D &linestr = pgon.linestr[i];
		std::vector< BoostPoint2D > pointList; 
		for (  int j=0; j< linestr.size(); ++j ) {
			BoostPoint2D p2( linestr.point[j].x, linestr.point[j].y );
			pointList.push_back(p2);
		}

		boost::geometry::append( mlstr, pointList );
	}

	boost::geometry::validity_failure_type failure;
	bool rc = boost::geometry::is_valid( mlstr, failure );
	return rc;
}

bool JagCGAL::getIsValidMultiPolygon2DStr(  const JagVector<JagPolygon> &pgvec )
{
	boost::geometry::model::multi_polygon<BoostPolygon2D> mbgon;
	for ( int k=0; k < pgvec.size(); ++k ) {
		boost::geometry::model::polygon<BoostPoint2D,false> bgon;
		const JagPolygon &pgon = pgvec[k];
		if ( ! convertPolygonJ2B( pgon, bgon ) ) continue;
    	mbgon.push_back( bgon );
	}

	boost::geometry::validity_failure_type failure;
	bool rc = boost::geometry::is_valid( mbgon, failure );
	return rc;
}

bool JagCGAL::getIsValidMultiPoint2DStr( const JagLineString &line )
{
	boost::geometry::model::multi_point<BoostPoint2D> mpoint;
	for ( int i=0; i < line.size(); ++i ) {
		boost::geometry::append( mpoint, BoostPoint2D( jagatof(line.point[i].x), jagatof(line.point[i].y) ) );
	}

	boost::geometry::validity_failure_type failure;
	bool rc = boost::geometry::is_valid( mpoint, failure );
	return rc;
}


bool JagCGAL::getIsRingLineString2DStr( const JagLineString &line )
{
	boost::geometry::model::linestring<BoostPoint2D> ls;
	for ( int i=0; i < line.size(); ++i ) {
		boost::geometry::append( ls, BoostPoint2D( jagatof(line.point[i].x), jagatof(line.point[i].y) ) );
	}

	bool rc = boost::geometry::is_simple( ls );
	if ( ! rc ) return false;

	rc = ( line.point[0].equal2D( line.point[line.size()-1] ) );
	if ( ! rc ) return false;
	return true;
}

void JagCGAL::getRingStr( const JagLineString &line, const Jstr &inhdr, const Jstr &bbox, bool is3D,
							   Jstr &value )
{
    dn("s0202288 getRingStr bbox=%s inhdr=%s", bbox.s(), inhdr.s() );

	const JagVector<JagPoint> &point = line.point;
	Jstr sx, sy, sz;

	Jstr hdr = inhdr;
	if ( is3D ) {
		hdr.replace("=PL3=", "=LS3=");
	} else {
		hdr.replace("=PL=", "=LS=");
	}

	if ( bbox.size() < 1 ) {
    	double xmin, ymin, xmax, ymax;
    	Jstr s1, s2, s3, s4;
    	Jstr newbbox;
		if ( is3D ) {
			double zmin, zmax;
    		Jstr s5, s6;
    		line.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
    		s1 = doubleToStr( xmin ).trimEndZeros();
    		s2 = doubleToStr( ymin ).trimEndZeros();
    		s3 = doubleToStr( zmin ).trimEndZeros();
    		s4 = doubleToStr( xmax ).trimEndZeros();
    		s5 = doubleToStr( ymax ).trimEndZeros();
    		s6 = doubleToStr( zmax ).trimEndZeros();
    		newbbox = s1 + ":" + s2 + ":" + s3 + ":" + s4  + ":" + s5 + ":" + s6;
		} else {
    		line.bbox2D( xmin, ymin, xmax, ymax );
    		s1 = doubleToStr( xmin ).trimEndZeros();
    		s2 = doubleToStr( ymin ).trimEndZeros();
    		s3 = doubleToStr( xmax ).trimEndZeros();
    		s4 = doubleToStr( ymax ).trimEndZeros();
    		newbbox = s1 + ":" + s2 + ":" + s3 + ":" + s4;
		}
    	value = hdr + " " + newbbox;
	} else {
		value = hdr + " " + bbox;
	}

	for ( int i = 0; i < point.size(); ++i ) {
		sx = point[i].x; sx.trimEndZeros();
		sy = point[i].y; sy.trimEndZeros();
		if ( is3D ) {
			sz = point[i].z; sz.trimEndZeros();
			value += Jstr(" ") + sx + ":" + sy + ":" + sz;
		} else {
			value += Jstr(" ") + sx + ":" + sy;
		}
	}

    dn("s2200188 returned value=[%s]", value.s() );

}

void JagCGAL::getOuterRingsStr( const JagVector<JagPolygon> &pgvec, const Jstr &inhdr, const Jstr &bbox, 
								bool is3D, Jstr &value )
{
	Jstr sx, sy, sz;
	Jstr hdr = inhdr;
	if ( is3D ) {
		hdr.replace("=MG3=", "=ML3=");
	} else {
		hdr.replace("=MG=", "=ML=");
	}

	if ( bbox.size() < 1 ) {
    	double xmin, ymin, xmax, ymax;
    	Jstr s1, s2, s3, s4;
    	Jstr newbbox;
		if ( is3D ) {
			double zmin, zmax;
    		Jstr s5, s6;
    		JagGeo::getBBox3D( pgvec, xmin, ymin, zmin, xmax, ymax, zmax );
    		s1 = doubleToStr( xmin ).trimEndZeros();
    		s2 = doubleToStr( ymin ).trimEndZeros();
    		s3 = doubleToStr( zmin ).trimEndZeros();
    		s4 = doubleToStr( xmax ).trimEndZeros();
    		s5 = doubleToStr( ymax ).trimEndZeros();
    		s6 = doubleToStr( zmax ).trimEndZeros();
    		newbbox = s1 + ":" + s2 + ":" + s3 + ":" + s4  + ":" + s5 + ":" + s6;
		} else {
			JagGeo::getBBox2D( pgvec, xmin, ymin, xmax, ymax );
    		s1 = doubleToStr( xmin ).trimEndZeros();
    		s2 = doubleToStr( ymin ).trimEndZeros();
    		s3 = doubleToStr( xmax ).trimEndZeros();
    		s4 = doubleToStr( ymax ).trimEndZeros();
    		newbbox = s1 + ":" + s2 + ":" + s3 + ":" + s4;
		}
    	value = hdr + " " + newbbox;
	} else {
		value = hdr + " " + bbox;
	}

	for ( int i = 0; i <  pgvec.size(); ++i ) {
		const JagLineString3D &line = pgvec[i].linestr[0];
		if ( i > 0 && line.size() > 0 ) {
			value += Jstr(" |");
		}

		for ( int j = 0; j < line.size(); ++j ) {
			const JagVector<JagPoint3D> &point = line.point;
    		sx = doubleToStr(point[j].x); sx.trimEndZeros();
    		sy = doubleToStr(point[j].y); sy.trimEndZeros();
    		if ( is3D ) {
    			sz = doubleToStr(point[j].z); sz.trimEndZeros();
    			value += Jstr(" ") + sx + ":" + sy + ":" + sz;
    		} else {
    			value += Jstr(" ") + sx + ":" + sy;
    		}
		}
	}

}


void JagCGAL::getInnerRingsStr( const JagVector<JagPolygon> &pgvec, const Jstr &inhdr, const Jstr &bbox, 
								bool is3D, Jstr &value )
{
	Jstr sx, sy, sz;
	Jstr hdr = inhdr;
	if ( is3D ) {
		hdr.replace("=MG3=", "=ML3=");
	} else {
		hdr.replace("=MG=", "=ML=");
	}

	if ( bbox.size() < 1 ) {
    	double xmin, ymin, xmax, ymax;
    	Jstr s1, s2, s3, s4;
    	Jstr newbbox;
		if ( is3D ) {
			double zmin, zmax;
    		Jstr s5, s6;
    		JagGeo::getBBox3DInner( pgvec, xmin, ymin, zmin, xmax, ymax, zmax );
    		s1 = doubleToStr( xmin ).trimEndZeros();
    		s2 = doubleToStr( ymin ).trimEndZeros();
    		s3 = doubleToStr( zmin ).trimEndZeros();
    		s4 = doubleToStr( xmax ).trimEndZeros();
    		s5 = doubleToStr( ymax ).trimEndZeros();
    		s6 = doubleToStr( zmax ).trimEndZeros();
    		newbbox = s1 + ":" + s2 + ":" + s3 + ":" + s4  + ":" + s5 + ":" + s6;
		} else {
			JagGeo::getBBox2DInner( pgvec, xmin, ymin, xmax, ymax );
    		s1 = doubleToStr( xmin ).trimEndZeros();
    		s2 = doubleToStr( ymin ).trimEndZeros();
    		s3 = doubleToStr( xmax ).trimEndZeros();
    		s4 = doubleToStr( ymax ).trimEndZeros();
    		newbbox = s1 + ":" + s2 + ":" + s3 + ":" + s4;
		}
    	value = hdr + " " + newbbox;
	} else {
		value = hdr + " " + bbox;
	}

	int numrings = 0;
	for ( int i = 0; i <  pgvec.size(); ++i ) {
		numrings =  pgvec[i].linestr.size();
		for ( int k = 1; k < numrings; ++k ) {
    		const JagLineString3D &line = pgvec[i].linestr[k];
    		if ( i > 0 && line.size() > 0 ) {
    			value += Jstr(" |");
    		}
    
    		for ( int j = 0; j < line.size(); ++j ) {
    			const JagVector<JagPoint3D> &point = line.point;
        		sx = doubleToStr(point[j].x); sx.trimEndZeros();
        		sy = doubleToStr(point[j].y); sy.trimEndZeros();
        		if ( is3D ) {
        			sz = doubleToStr(point[j].z); sz.trimEndZeros();
        			value += Jstr(" ") + sx + ":" + sy + ":" + sz;
        		} else {
        			value += Jstr(" ") + sx + ":" + sy;
        		}
    		}
		}
	}
}


void JagCGAL::getPolygonNStr( const JagVector<JagPolygon> &pgvec, const Jstr &inhdr, const Jstr &bbox, 
								bool is3D, int N, Jstr &value )
{
	if ( N > pgvec.size() ) {
		value = "";
		return;
	}
	Jstr sx, sy, sz;
	Jstr hdr = inhdr;
	if ( is3D ) {
		hdr.replace("=MG3=", "=PL3=");
	} else {
		hdr.replace("=MG=", "=PL=");
	}

	const JagPolygon &pgon = pgvec[N-1];

	if ( bbox.size() < 1 ) {
    	double xmin, ymin, xmax, ymax;
    	Jstr s1, s2, s3, s4;
    	Jstr newbbox;
		if ( is3D ) {
			double zmin, zmax;
    		Jstr s5, s6;
    		pgon.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
    		s1 = doubleToStr( xmin ).trimEndZeros();
    		s2 = doubleToStr( ymin ).trimEndZeros();
    		s3 = doubleToStr( zmin ).trimEndZeros();
    		s4 = doubleToStr( xmax ).trimEndZeros();
    		s5 = doubleToStr( ymax ).trimEndZeros();
    		s6 = doubleToStr( zmax ).trimEndZeros();
    		newbbox = s1 + ":" + s2 + ":" + s3 + ":" + s4  + ":" + s5 + ":" + s6;
		} else {
			pgon.bbox2D( xmin, ymin, xmax, ymax );
    		s1 = doubleToStr( xmin ).trimEndZeros();
    		s2 = doubleToStr( ymin ).trimEndZeros();
    		s3 = doubleToStr( xmax ).trimEndZeros();
    		s4 = doubleToStr( ymax ).trimEndZeros();
    		newbbox = s1 + ":" + s2 + ":" + s3 + ":" + s4;
		}
    	value = hdr + " " + newbbox;
	} else {
		value = hdr + " " + bbox;
	}

	for ( int k = 0; k < pgon.size(); ++k ) {
    	const JagLineString3D &line = pgon.linestr[k];
    	if ( k > 0 && line.size() > 0 ) {
    		value += Jstr(" |");
    	}
   
   		for ( int j = 0; j < line.size(); ++j ) {
   			const JagVector<JagPoint3D> &point = line.point;
       		sx = doubleToStr(point[j].x); sx.trimEndZeros();
       		sy = doubleToStr(point[j].y); sy.trimEndZeros();
       		if ( is3D ) {
       			sz = doubleToStr(point[j].z); sz.trimEndZeros();
       			value += Jstr(" ") + sx + ":" + sy + ":" + sz;
       		} else {
       			value += Jstr(" ") + sx + ":" + sy;
      		}
   		}
	}
}

void JagCGAL::getUniqueStr( const JagStrSplit &sp, const Jstr &hdr, const Jstr &bbox, Jstr &value )
{
	value = hdr;
	if ( bbox.size() > 0 ) {
		value += Jstr(" ") + bbox;
	}

	bool firstPoint = true;
	int num;
	Jstr lastP;
	for  ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		if ( sp[i] == "|" || sp[i] == "!" ) {
			firstPoint = true;
			value += Jstr(" ") + sp[i];
		} else {
			num = strchrnum( sp[i].c_str(), ':');
			if ( num < 1 ) continue;
			if ( ! firstPoint ) {
				if ( sp[i] != lastP ) {
					value += Jstr(" ") + sp[i];
				} 
			} else {
				firstPoint = false;
				value += Jstr(" ") + sp[i];
			}

			lastP = sp[i];
		}
	}
}

int JagCGAL::unionOfPolygonAndMultiPolygons( const JagStrSplit &sp1, const JagStrSplit &sp2, Jstr &unionWKT )
{
 	JagPolygon pgon1;
    int rc = JagParser::addPolygonData( pgon1, sp1, false );
	if ( rc < 0 ) { return -1;}
	BoostPolygon2D  bgon1;
	if ( ! convertPolygonJ2B( pgon1, bgon1 ) ) return -1;

	JagVector<JagPolygon> pgvec;
	rc = JagParser::addMultiPolygonData( pgvec, sp2, false, false );
	if ( rc <= 0 ) { return -2;}

	Jstr wkt;
	JagGeo::multiPolygonToWKT( pgvec, false, wkt );

	boost::geometry::model::multi_polygon<BoostPolygon2D> mgon;
	std::string ss = std::string( wkt.c_str(), wkt.size() );
	boost::geometry::read_wkt( ss, mgon );

	boost::geometry::model::multi_polygon<BoostPolygon2D> output;
	boost::geometry::union_( bgon1, mgon, output );

	std::stringstream ifs;
	ifs << boost::geometry::wkt(output);
	unionWKT = ifs.str().c_str();
	if ( unionWKT.size() < 1 ) return -3;
	return 0;
}

int JagCGAL::unionOfTwoPolygons( const JagStrSplit &sp1, const JagStrSplit &sp2, Jstr &unionWKT )
{
 	JagPolygon pgon1;
    int rc = JagParser::addPolygonData( pgon1, sp1, false );
	if ( rc < 0 ) { return -1;}
	BoostPolygon2D  bgon1;
	if ( ! convertPolygonJ2B( pgon1, bgon1 ) ) return -1;

 	JagPolygon pgon2;
    rc = JagParser::addPolygonData( pgon2, sp2, false );
	if ( rc < 0 ) return -2;
	BoostPolygon2D  bgon2;
	if ( ! convertPolygonJ2B( pgon2, bgon2 ) ) return -2;
	boost::geometry::model::multi_polygon<BoostPolygon2D> output;
	boost::geometry::union_( bgon1, bgon2, output );
	std::stringstream ifs;
	ifs << boost::geometry::wkt(output);
	unionWKT = ifs.str().c_str();
	if ( unionWKT.size() < 1 ) return -3;
	return 0;
}

bool JagCGAL::hasIntersection( const JagLine2D &line1, const JagLine2D &line2, JagVector<JagPoint2D> &res )
{
	CGALKernel::Segment_2 seg1(CGALPoint2D(line1.x1, line1.y1), CGALPoint2D(line1.x2, line1.y2) );
	CGALKernel::Segment_2 seg2(CGALPoint2D(line2.x1, line2.y1), CGALPoint2D(line2.x2, line2.y2) );
	auto result = intersection(seg1, seg2);
	if ( result ) {
		boost::apply_visitor(JagIntersectionVisitor2D(res), *result);
		return true;
	} else {
		return false;
	}
}

bool JagCGAL::hasIntersection( const JagLine3D &line1, const JagLine3D &line2, JagVector<JagPoint3D> &res )
{
	CGALKernel::Segment_3 seg1(CGALPoint3D(line1.x1, line1.y1, line1.z1), CGALPoint3D(line1.x2, line1.y2, line1.z2) );
	CGALKernel::Segment_3 seg2(CGALPoint3D(line2.x1, line2.y1, line2.z1), CGALPoint3D(line2.x2, line2.y2, line2.z2) );
	auto result = intersection(seg1, seg2);
	if ( result ) {
		boost::apply_visitor(JagIntersectionVisitor3D(res), *result);
		return true;
	} else {
		return false;
	}
}

void JagCGAL::split2DSPToVector( const JagStrSplit &sp, JagVector<JagPoint2D> &vec )
{
	const char *str;
	char *p;
	double x,y;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		str = sp[i].c_str();
		if ( *str == '|' || *str == '!' ) continue;
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', x, y );
		vec.append(JagPoint2D(x,y) );
	}
}

void JagCGAL::split3DSPToVector( const JagStrSplit &sp, JagVector<JagPoint3D> &vec )
{
	const char *str;
	char *p;
	double x,y,z;
	for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
		str = sp[i].c_str();
		if ( *str == '|' || *str == '!' ) continue;
		if ( strchrnum( str, ':') < 2 ) continue;
		get3double(str, p, ':', x, y,z );
		vec.append(JagPoint3D(x,y,z) );
	}
}

int JagCGAL::getTwoPolygonIntersection( const JagPolygon &pgon1, const JagPolygon &pgon2, JagVector<JagPolygon> &vec )
{
	if ( pgon1.size() < 1 || pgon2.size() < 1 ) return 0;

	boost::geometry::model::polygon<BoostPoint2D,false> bgon1;
	if ( ! convertPolygonJ2B( pgon1, bgon1 ) ) return 0;

	boost::geometry::model::polygon<BoostPoint2D,false> bgon2;
	if ( ! convertPolygonJ2B( pgon2, bgon2 ) ) return 0;

	std::vector<BoostPolygon2D> output;
	boost::geometry::intersection( bgon1, bgon2, output);

	double x, y;
	BOOST_FOREACH( BoostPolygon2D const& poly, output) 
	{
		JagPolygon pgon;
		JagLineString3D outlstr;
		for( auto it = boost::begin(boost::geometry::exterior_ring(poly)); 
			 it != boost::end(boost::geometry::exterior_ring(poly)); ++it ) {
			x =  boost::geometry::get<0>(*it);
			y =  boost::geometry::get<1>(*it);
			outlstr.add( JagPoint2D(x,y) );
		}
		pgon.add( outlstr );

		const std::vector<BoostRing2D> &inners = poly.inners();
		for ( int i=0; i < inners.size(); ++i ) {
			JagLineString3D inlstr;
			BOOST_FOREACH( BoostPoint2D const& pt, inners[i] ) {
				x =  boost::geometry::get<0>(pt);
				y =  boost::geometry::get<1>(pt);
				inlstr.add( JagPoint2D(x,y) );
			}
			pgon.add( inlstr );
		}

		vec.append( pgon );
	}

	return vec.size();
}

bool JagCGAL::convertPolygonJ2B( const JagPolygon &pgon, BoostPolygon2D &bgon )
{
	Jstr wkt;
	pgon.toWKT(false, true, "polygon", wkt);
	if ( wkt.size() < 1 ) return false;
	std::string ss = std::string( wkt.c_str(), wkt.size() );
	boost::geometry::read_wkt( ss, bgon );
	return true;
}


void JagCGAL::getVoronoiPolygons2D( int srid, const JagStrSplit &sp, double tolerance,
	                                double xmin, double ymin, double xmax, double ymax, const Jstr &retType, Jstr &vor )
{
	if ( jagLE(tolerance, JAG_ZERO ) ) {
		tolerance = 1.0;
	}
	if ( sp.size() < 3 ) return;

	double factor = 100.0;
	std::vector<BoostPointLong2D> points;
	double x1, y1, x2, y2, bx, by;
	double vx, vy;  
	double unitlon, unitlat;
	const char *str; char *p;
	long long ix, iy;

	if ( 0 == srid  ) {
		xmin = jagint(xmin/tolerance) * factor;
		ymin = jagint(ymin/tolerance) * factor;
		xmax = jagint(xmax/tolerance) * factor;
		ymax = jagint(ymax/tolerance) * factor;
	} else {
		unitlon = JagGeo::meterToLon( srid, tolerance, (xmin+xmax)/2.0, (ymin+ymax)/2.0 );
		unitlat = JagGeo::meterToLat( srid, tolerance, (xmin+xmax)/2.0, (ymin+ymax)/2.0 );
		xmin = jagint(xmin/unitlon) * factor;
		ymin = jagint(ymin/unitlat) * factor;
		xmax = jagint(xmax/unitlon) * factor;
		ymax = jagint(ymax/unitlat) * factor;
	}
	JagBox2D bbox(xmin, ymin, xmax, ymax );

	for ( int i=JAG_SP_START; i < sp.size(); ++i ) {
		str = sp[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', vx, vy );
		if ( 0 == srid  ) {
			ix = (vx/tolerance) * factor;
			iy = (vy/tolerance) * factor;
		} else {
			ix = (vx/unitlon) * factor;
			iy = (vy/unitlat) * factor;
		}
		points.push_back(BoostPointLong2D(ix, iy));
	}
	
	voronoi_diagram<double> vd;
	construct_voronoi(points.begin(), points.end(), &vd);

	const voronoi_diagram<double>::vertex_type *pv0;
	const voronoi_diagram<double>::vertex_type *pv1;
	int ncell = 0;
   	const voronoi_diagram<double>::edge_type *edge; 
	BoostPointLong2D p1, p2;
	int rc;
	int cnt;
	int lastPoints = 0;

	ncell = 0;
	for (voronoi_diagram<double>::const_cell_iterator it = vd.cells().begin(); it != vd.cells().end(); ++it) {
		++ncell;
    	const voronoi_diagram<double>::cell_type &cell = *it;
    	edge = cell.incident_edge();
		if ( ! edge ) {
			continue;
		}

		if ( ncell > 1 && lastPoints > 0 ) { 
			if ( retType == JAG_C_COL_TYPE_MULTIPOLYGON  ) {
				vor += Jstr(" !"); 
			} else if ( retType == JAG_C_COL_TYPE_MULTILINESTRING ) {
				vor += Jstr(" |"); 
			} else {
				vor += Jstr(" |"); 
			}
		}

		cnt = 0;
		JagVector<JagVoronoiPoint2D> vec1, vec2;
		lastPoints = 0;
		while ( true ) {
      		if (! edge->is_primary()) {
				continue;
			}
			++cnt;
			pv0 = edge->vertex0();
			pv1 = edge->vertex1();
			if ( pv0 && pv1 ) {
				x1 = pv0->x(); y1 =  pv0->y();
				x2 = pv1->x(); y2 =  pv1->y();
				vec1.append( JagVoronoiPoint2D(x1,y1,0) );
			} else {
				if ( pv0 && ! pv1 ) {
					p1 = points[edge->cell()->source_index()];
					p2 = points[edge->twin()->cell()->source_index()];
					vx = pv0->x(); vy =  pv0->y();
   				} else if ( pv1 && ! pv0 ) {
					p2 = points[edge->cell()->source_index()];
					p1 = points[edge->twin()->cell()->source_index()];
					vx = pv1->x(); vy =  pv1->y();
				} else {
					break;  
				}

				rc = getIntersectionPointWithBox( vx, vy, p1, p2, xmin, ymin, xmax, ymax, bx, by );
				if ( rc < 0 ) break;
				if ( pv0 ) {
					x1 = vx; y1 = vy;
					x2 = bx; y2 = by;
					vec1.append( JagVoronoiPoint2D(x1,y1,0));
					vec1.append( JagVoronoiPoint2D(x2,y2,rc));
				} else {
					x1 = bx; y1 = by;
					x2 = vx; y2 = vy;
					vec1.append( JagVoronoiPoint2D(x1,y1,rc));
				}

			}

      		edge = edge->next(); 
			if ( edge == cell.incident_edge() ) break;
    	} 

		if ( vec1.size() > 0 ) {
			if ( retType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
				fillInCorners(bbox, vec1, vec2);
			} 

			JagVector<JagVoronoiPoint2D> &vec3 = ( retType == JAG_C_COL_TYPE_MULTIPOLYGON ) ? vec2 : vec1;
			for ( int i=0; i < vec3.size(); ++i ) {
    			if ( 0 == srid  ) {
    				vec3[i].x *= tolerance; vec3[i].y *= tolerance;
    			} else {
    				vec3[i].x *= unitlon; vec3[i].y *= unitlat;
    			}
				vec3[i].x /= factor;  vec3[i].y /= factor;

				vor += Jstr(" ") + d2s(vec3[i].x) + ":" + d2s(vec3[i].y);
				++lastPoints;
			}

			if ( retType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
				if ( vec3[vec3.size()-1] != vec3[0] ) {
					vor += Jstr(" ") + d2s(vec3[0].x) + ":" + d2s(vec3[0].y);
				}
			}
		}
    } 
}


int JagCGAL::getIntersectionPointWithBox( double vx, double vy, 
   					const BoostPointLong2D &p1, const BoostPointLong2D &p2, 
   					double xmin, double ymin, double xmax, double ymax, double &bx, double &by )
{
	double k;
	long long dx = p1.y - p2.y;
	long long dy = p2.x - p1.x;

	if (  0 == dy ) {
		if ( dx < 0 ) {
			bx = xmin; by = vy;
			return JAG_LEFT_SIDE;
		}

		if ( dx > 0 ) {
			bx = xmax; by = vy;
			return JAG_RIGHT_SIDE;
		}

		return -1;
	}

	if ( 0 == dx ) {
		if ( dy > 0 ) {
			bx = vx; by = ymax;
			return JAG_TOP_SIDE;
		}

		if ( dy < 0 ) {
			bx = vx; by = ymin;
			return JAG_BOTTOM_SIDE;
		}

		return -2;
	}

	k = (double)(dy)/ (double)(dx);

	if ( dx < 0 ) {
		bx = xmin;
		by = k*bx + vy - k*vx;
		if (  jagGE(by, ymin ) && jagLE(by, ymax) ) { 
			return JAG_LEFT_SIDE;
		}
	}

	if ( dx > 0 ) {
    	bx = xmax;
    	by = k*bx + vy - k*vx;
    	if (  jagGE(by, ymin ) && jagLE(by, ymax) ) {
    		return JAG_RIGHT_SIDE;
    	}
	}
    
	if ( dy < 0 ) {
    	by = ymin;
    	bx = ( by - vy + k*vx)/k;
    	if ( jagGE(bx, xmin) && jagLE(bx, xmax) ) {
    		return JAG_BOTTOM_SIDE;
    	}
	}
    
	if ( dy > 0 ) {
    	by = ymax;
    	bx = ( by - vy + k*vx)/k;
    	if ( jagGE(bx, xmin) && jagLE(bx, xmax) ) {
    		return JAG_TOP_SIDE;
    	}
	}
	
	return -10;
}

void JagCGAL::fillInCorners(const JagBox2D &bbox, const JagVector<JagVoronoiPoint2D> &vec1, JagVector<JagVoronoiPoint2D> &vec2)
{
	int nexti;
	int len1 = vec1.size();
	JagVoronoiPoint2D p1(bbox.xmin, bbox.ymin, 0);
	JagVoronoiPoint2D p2(bbox.xmax, bbox.ymin, 0);
	JagVoronoiPoint2D p3(bbox.xmax, bbox.ymax, 0);
	JagVoronoiPoint2D p4(bbox.xmin, bbox.ymax, 0);
	for ( int i=0; i < len1; ++i ) {
		vec2.append( JagVoronoiPoint2D(vec1[i].x, vec1[i].y, vec1[i].side ) );
		nexti = (i+1)%len1;
		if ( vec1[i].side > 0 && vec1[nexti].side > 0 ) {
			if ( JAG_TOP_SIDE == vec1[i].side ) {
				if ( JAG_LEFT_SIDE == vec1[nexti].side ) {
					if (   vec1[i] != p4 ) { vec2.append( p4 ); }
				} else if ( JAG_BOTTOM_SIDE == vec1[nexti].side ) {
					if (   vec1[i] != p4 ) { vec2.append( p4 ); }
					if ( vec1[nexti] != p1 ) vec2.append( p1 );
				} else if ( JAG_RIGHT_SIDE == vec1[nexti].side ) {
					if (   vec1[i] != p4 ) { vec2.append( p4 ); }
					if ( vec1[nexti] != p1 ) vec2.append( p1 );
					if ( vec1[nexti] != p2 ) vec2.append( p2 );
				}
			} else if ( JAG_LEFT_SIDE == vec1[i].side ) {
				if ( JAG_BOTTOM_SIDE == vec1[nexti].side ) {
					if ( vec1[i] != p1 ) { vec2.append( p1 ); }
				} else if ( JAG_RIGHT_SIDE == vec1[nexti].side ) {
					if ( vec1[i] != p1 ) { vec2.append( p1 ); }
					if (  vec1[nexti] != p2 ) vec2.append( p2 );
				} else if ( JAG_TOP_SIDE == vec1[nexti].side ) {
					if ( vec1[i] != p1 ) { vec2.append( p1 ); }
					if (  vec1[nexti] != p2 ) vec2.append( p2 );
					if (  vec1[nexti] != p3 ) vec2.append( p3 );
				}
			} else if ( JAG_BOTTOM_SIDE == vec1[i].side ) {
				if ( JAG_RIGHT_SIDE == vec1[nexti].side ) {
					if ( vec1[i] != p2 ) { vec2.append( p2 ); }
				} else if ( JAG_TOP_SIDE == vec1[nexti].side ) {
					if ( vec1[i] != p2 ) { vec2.append( p2 ); }
					if (  vec1[nexti] != p3 )vec2.append( p3 );
				} else if ( JAG_LEFT_SIDE == vec1[nexti].side ) {
					if ( vec1[i] != p2 ) { vec2.append( p2 ); }
					if (  vec1[nexti] != p3 )vec2.append( p3 );
					if (  vec1[nexti] != p4 )vec2.append( p4 );
				}
			} else if ( JAG_RIGHT_SIDE == vec1[i].side ) {
				if ( JAG_TOP_SIDE == vec1[nexti].side ) {
					if ( vec1[i] != p3 ) { vec2.append( p3 ); }
				} else if ( JAG_LEFT_SIDE == vec1[nexti].side ) {
					if ( vec1[i] != p3 ) { vec2.append( p3 ); }
					if (  vec1[nexti] != p4 ) vec2.append( p4 );
				} else if ( JAG_BOTTOM_SIDE == vec1[nexti].side ) {
					if ( vec1[i] != p3 ) { vec2.append( p3 ); }
					if (  vec1[nexti] != p4 ) vec2.append( p4 );
					if (  vec1[nexti] != p1 ) vec2.append( p1 );
				}
			}
		} 
	}
}

void JagCGAL::getDelaunayTriangles2D( int srid, const JagStrSplit &sp, double tolerance, Jstr &mpg )
{
	if ( sp.size() < 3 ) return;
	if ( jagLE(tolerance, JAG_ZERO ) ) {
		tolerance = 1.0;
	}

	double factor = 100.0;
	double vx, vy;
	const char *str; char *p;
	long long ix, iy;
	std::vector<BoostPointLong2D> points;

	double unitlon;
	double unitlat;
	if ( srid > 0 ) {
		str = sp[JAG_SP_START].c_str();
		if ( strchrnum( str, ':') < 1 ) { return; }
		get2double(str, p, ':', vx, vy );
		unitlon = JagGeo::meterToLon( srid, tolerance, vx, vy );
		unitlat = JagGeo::meterToLat( srid, tolerance, vx, vy );
	}

	for ( int i=JAG_SP_START; i < sp.size(); ++i ) {
		str = sp[i].c_str();
		if ( strchrnum( str, ':') < 1 ) continue;
		get2double(str, p, ':', vx, vy );
		if ( 0 == srid  ) {
			ix = (vx/tolerance) * factor;
			iy = (vy/tolerance) * factor;
		} else {
			ix = (vx/unitlon) * factor;
			iy = (vy/unitlat) * factor;
		}
		points.push_back(BoostPointLong2D(ix, iy));
	}

	voronoi_diagram<double> vd;
	construct_voronoi(points.begin(), points.end(), &vd);
   	const voronoi_diagram<double>::edge_type *edge; 
	JagVector<JagVector<JagPoint2D> > allTriangles;
	for (const auto& vertex: vd.vertices()) {
    	edge = vertex.incident_edge();
		if ( ! edge ) { continue; }
		JagVector<JagPoint2D> triangle;
		while ( true ) {
			auto cell = edge->cell();
			const BoostPointLong2D &pt = points[cell->source_index()];
			triangle.append( JagPoint2D(pt.x, pt.y) );
			if ( 3 == triangle.size() ) {
				allTriangles.append( triangle );
				triangle.clear();
			}
      		edge = edge->rot_next(); 
			if ( edge == vertex.incident_edge() ) break;
    	} 
    } 

	if ( allTriangles.size() > 0 ) {
		for ( int k=0; k < allTriangles.size(); ++k ) {
			if ( k > 0 ) { mpg += Jstr(" !"); }
			JagVector<JagPoint2D> &vec3 = allTriangles[k];
			for ( int i=0; i < vec3.size(); ++i ) {
       			if ( 0 == srid  ) {
       				vec3[i].x *= tolerance; vec3[i].y *= tolerance;
       			} else {
       				vec3[i].x *= unitlon; vec3[i].y *= unitlat;
       			}
   				vec3[i].x /= factor;  vec3[i].y /= factor;
   				mpg += Jstr(" ") + d2s(vec3[i].x) + ":" + d2s(vec3[i].y);
			}
   			mpg += Jstr(" ") + d2s(vec3[0].x) + ":" + d2s(vec3[0].y);
		}
	}
}

bool JagCGAL::getMinBoundCircle( int srid, const JagVector<JagPoint2D> &vec, Jstr &res )
{
	int len = vec.size();
	if ( len < 1 ) return false;
	std::vector<CGALCircle> C;
	const int DIM = 2;
	double cord[DIM];
	for ( int i=0; i < len; ++i ) {
		cord[0] = vec[i].x;
		cord[1] = vec[i].y;
		CGALCartPoint point(DIM, cord, cord+DIM );
		C.push_back( CGALCircle(point, 0.0) );
	}

	CGAL_Min_circle mc(C.begin(),C.end());
	if ( ! mc.is_valid() ) return false;
	double r = mc.radius();
	auto c =  mc.center_cartesian_begin();
	double x = *c;
	++c;
	if ( c == mc.center_cartesian_end() ) return false;
	double y = *c;
	double lon, lat;
	JagGeo::XYToLonLat(  srid, x, y, lon, lat );
	res = d2s(lon) + " " + d2s(lat) +  " " + d2s(r);
	return true;
}


bool JagCGAL::getMinBoundSphere( int srid, const JagVector<JagPoint3D> &vec, Jstr &res )
{
	int len = vec.size();
	if ( len < 1 ) return false;
	std::vector<CGALSphere> S;
	const int DIM = 3;
	double cord[DIM];
	for ( int i=0; i < len; ++i ) {
		cord[0] = vec[i].x;
		cord[1] = vec[i].y;
		cord[2] = vec[i].z;
		CGALCartPoint point(DIM, cord, cord+DIM );
		S.push_back( CGALSphere(point, 0.0) );
	}

	CGAL_Min_sphere ms(S.begin(), S.end());
	if ( ! ms.is_valid() ) return false;
	double r = ms.radius();
	auto c =  ms.center_cartesian_begin();
	double x = *c;
	++c;
	if ( c == ms.center_cartesian_end() ) return false;
	double y = *c;
	++c;
	if ( c == ms.center_cartesian_end() ) return false;
	double z = *c;

	double lon, lat, alt;
	JagGeo::XYZToLonLatAlt(  srid, x, y, z , lon, lat, alt );
	res = d2s(lon) + " " + d2s(lat) +  " " + d2s(alt) + " " + d2s(r);
	return true;
}

int JagCGAL::pointRelateLine( double px, double py, double x1, double y1, double x2, double y2)
{
	CGALPoint2D pt(px,py);
	CGALPoint2D p(x1,y1);
	CGALPoint2D q(x2,y2);
	CGAL::Orientation ort = CGAL::orientation(p,q, pt);
	if ( ort == CGAL::LEFT_TURN ) return JAG_LEFT;
	else if ( ort == CGAL::RIGHT_TURN ) return JAG_RIGHT;
	else return JAG_ISON;
}

int JagCGAL::lineRelateLine( double x1, double y1, double x2, double y2, double mx1, double my1, double mx2, double my2 )
{
	CGALPoint2D pt1(x1,y1);
	CGALPoint2D pt2(x2,y2);
	CGALPoint2D p(mx1,my1);
	CGALPoint2D q(mx2,my2);
	CGAL::Orientation ort1 = CGAL::orientation(p,q, pt1);
	CGAL::Orientation ort2 = CGAL::orientation(p,q, pt2);
	if ( ort1 == CGAL::LEFT_TURN && ort2 == CGAL::LEFT_TURN ) return JAG_LEFT;
	else if ( ort1 == CGAL::RIGHT_TURN && ort2 == CGAL::RIGHT_TURN ) return JAG_RIGHT;
	else if ( ort1 == CGAL::COLLINEAR && ort2 ==  CGAL::COLLINEAR ) return JAG_ISON;
	else return 0;
}

void JagCGAL::getConcaveHull2DStr( const JagLineString3D &line, const Jstr &hdr, const Jstr &bbox, Jstr &value )
{
	if ( line.size() < 2 ) return;

	ACPointVector points;
	ACPointVector hull;
	for ( int i=0; i < line.size(); ++i ) {
		points.push_back( ACPoint( line.point[i].x, line.point[i].y ) );
	}

	if ( ! ACConcaveHull( points, hull) ) { return; }

	JagStrSplit ss(hdr, '=' );
	value = Jstr("CJAG=") + ss[1] + "=0=PL=d " + bbox;
	int len = hull.size();
	for ( int i = 0; i < len; ++i ) {
		value += Jstr(" ") + d2s(hull[i].x) + ":" + d2s(hull[i].y);
	}

	if ( ! (jagEQ(hull[0].x, hull[len-1].x )
	       && jagEQ(hull[0].y, hull[len-1].y ) ) ) {
		value += Jstr(" ") + d2s(hull[0].x) + ":" + d2s(hull[0].y);
	}
}

