#include <JagGlobalDef.h>
#include <JagGeom.h>

double JagLineString::lineLength( bool removeLast, bool is3D, int srid )
{
	double sum = 0.0;
	int len =  size();
	if ( removeLast ) --len;
	for ( int i=0; i < len-1; ++i ) {
		if ( is3D ) {
			sum += JagGeo::distance( jagatof(point[i].x), jagatof(point[i].y), jagatof(point[i].z),
									 jagatof(point[i+1].x), jagatof(point[i+1].y), jagatof(point[i+1].z), srid );
		} else {
			sum += JagGeo::distance( jagatof(point[i].x), jagatof(point[i].y),
									 jagatof(point[i+1].x), jagatof(point[i+1].y), srid );
		}
	}
	return sum;
}

void JagLineString::toJAG( const Jstr &colType, bool is3D, bool hasHdr, const Jstr &inbbox, int srid, Jstr &str ) const
{
	if ( point.size() < 1 ) { str=""; return; }
	if ( hasHdr ) {
		Jstr srids = intToStr( srid );
		Jstr bbox;
		Jstr mk = "OJAG=";
		if ( is3D ) {
			if ( inbbox.size() < 1 )  { bbox = "0:0:0:0:0:0"; mk="CJAG="; } else { bbox = inbbox; }
		} else {
			if ( inbbox.size() < 1 )  { bbox = "0:0:0:0"; mk="CJAG="; } else { bbox = inbbox; }
		}
		str = mk + srids + "=0=" + colType + "=d " + bbox;
	} 

	for ( int i=0; i < point.size(); ++i ) {
		str += Jstr(" ") + Jstr(point[i].x) + ":" +  Jstr(point[i].y);
		if ( is3D ) { str += Jstr(":") + Jstr(point[i].z); }
	}
}

void JagLineString3D::toJAG( const Jstr &colType, bool is3D, bool hasHdr, const Jstr &inbbox, int srid, Jstr &str ) const
{
	if ( point.size() < 1 ) { str=""; return; }
	if ( hasHdr ) {
		Jstr srids = intToStr( srid );
		Jstr bbox;
		Jstr mk = "OJAG=";
		if ( is3D ) {
			if ( inbbox.size() < 1 )  { bbox = "0:0:0:0:0:0"; mk="CJAG="; } else { bbox = inbbox; }
		} else {
			if ( inbbox.size() < 1 )  { bbox = "0:0:0:0"; mk="CJAG="; } else { bbox = inbbox; }
		}
		str = mk + srids + "=0=" + colType + "=d " + bbox;
	} 

	for ( int i=0; i < point.size(); ++i ) {
		str += Jstr(" ") + d2s(point[i].x) + ":" +  d2s(point[i].y);
		if ( is3D ) { str += Jstr(":") + d2s(point[i].z); }
	}
}

double JagLineString3D::lineLength( bool removeLast, bool is3D, int srid )
{
	double sum = 0.0;
	int len =  size();
	if ( removeLast ) --len;
	for ( int i=0; i < len-1; ++i ) {
		if ( is3D ) {
			sum += JagGeo::distance( point[i].x, point[i].y, point[i].z,
									 point[i+1].x, point[i+1].y, point[i+1].z, srid );
		} else {
			sum += JagGeo::distance( point[i].x, point[i].y,
									 point[i+1].x, point[i+1].y, srid);
		}
	}
	return sum;
}

// ratio: [0.0, 1.0]  returns point
bool JagLineString3D::interpolatePoint( short dim, int srid, double ratio, JagPoint3D &point )
{
	double length = lineLength(false, false, srid );
	double len = length * ratio;
	double segDist, segFraction;
	JagPoint3D p1, p2;
	bool rc = getBetweenPointsFromLen( dim, len, srid, p1, p2, segDist, segFraction );
	//d("s2831 getBetweenPointsFromLen rc=%d\n", rc );
	if ( ! rc ) return false;

	if ( JAG_2D == dim && JAG_GEO_WGS84 == srid ) {
		JagGeo::interpolatePoint2D( segDist, segFraction, p1, p2, point );
	} else {
		point.x = p1.x + segFraction*(p2.x - p1.x);
		point.y = p1.y + segFraction*(p2.y - p1.y);
		if ( dim == JAG_3D ) { point.z = p1.z + segFraction*(p2.z - p1.z); }
	}

	return true;
}

bool JagLineString3D::getBetweenPointsFromLen( short dim, double len, int srid, JagPoint3D &p1, JagPoint3D &p2, 
											double &segDist, double &segFraction )
{
	//d("s6110 getBetweenPointsFromLen  dim=%d len=%f \n", dim, len );
	double prevsegsum = 0.0;  // total len of all previous segments
	double segsum = 0.0;      // total len of all segments including current segment
	double dist = 0.0;
	int plen =  point.size();
	for ( int i=0; i < plen-1; ++i ) {
		if ( JAG_3D == dim  ) {
			dist = JagGeo::distance( point[i].x, point[i].y, point[i].z,
									 point[i+1].x, point[i+1].y, point[i+1].z, srid );
		} else {
			dist = JagGeo::distance( point[i].x, point[i].y,
									 point[i+1].x, point[i+1].y, srid );
		}

		segsum += dist;
		//d("s1028 dist=%.3f prevsegsum=%.3f segsum=%.3f\n", dist, prevsegsum, segsum );

		if ( segsum > len || jagEQ( segsum, len) ) {
			p1.x = point[i].x; p1.y = point[i].y; 
			p2.x = point[i+1].x; p2.y = point[i+1].y; 
			if ( JAG_3D == dim  ) { p1.z = point[i].z; p2.z = point[i+1].z; }
			segDist = dist;
			segFraction = (len-prevsegsum)/dist;
			//d("s9483 retrn true\n" );
			return true;
		}

		prevsegsum = segsum;
	}
	//d("s9283 retrn false\n" );
	return false;
}

// ratio: [0.0, 1.0]  returns point
bool JagLineString3D::substring( short dim, int srid, double startFrac, double endFrac, Jstr &retLstr )
{
	double length = lineLength(false, false, srid );
	double len1 = length * startFrac;
	double len2 = length * endFrac;

	double prevsegsum = 0.0;  // total len of all previous segments
	double segsum = 0.0;      // total len of all segments including current segment
	double dist = 0.0;
	int plen =  point.size();
	bool inside = false;
	JagPoint3D p11, p12;
	JagPoint3D p21, p22;
	double segDist1, segFraction1;
	double segDist2, segFraction2;
	JagVector<JagPoint3D> pvec;

	for ( int i=0; i < plen-1; ++i ) {
		if ( JAG_3D == dim  ) {
			dist = JagGeo::distance( point[i].x, point[i].y, point[i].z,
									 point[i+1].x, point[i+1].y, point[i+1].z, srid );
		} else {
			dist = JagGeo::distance( point[i].x, point[i].y,
									 point[i+1].x, point[i+1].y, srid );
		}

		segsum += dist;
		//d("s1028 dist=%.3f prevsegsum=%.3f segsum=%.3f\n", dist, prevsegsum, segsum );

		if ( ! inside ) {
    		if ( segsum > len1 || jagEQ( segsum, len1) ) {
    			p11.x = point[i].x; p11.y = point[i].y; 
    			p12.x = point[i+1].x; p12.y = point[i+1].y; 
    			if ( JAG_3D == dim  ) { p11.z = point[i].z; p12.z = point[i+1].z; }
    			segDist1 = dist;
    			segFraction1 = (len1-prevsegsum)/dist;
    			inside = true;
    		}
		}


		if ( inside ) {
    		if ( segsum > len2 || jagEQ( segsum, len2) ) {
    			p21.x = point[i].x; p21.y = point[i].y; 
    			p22.x = point[i+1].x; p22.y = point[i+1].y; 
    			if ( JAG_3D == dim  ) { p21.z = point[i].z; p22.z = point[i+1].z; }
    			segDist2 = dist;
    			segFraction2 = (len2-prevsegsum)/dist;
    			break;
    		} else {
				if ( JAG_3D == dim ) {
					pvec.append( JagPoint3D(point[i+1].x, point[i+1].y, point[i+1].z ) );
				} else {
					pvec.append( JagPoint3D(point[i+1].x, point[i+1].y, 0.0) );
				}
			}
		}

		prevsegsum = segsum;
	}

	JagPoint3D startPoint;
	if ( JAG_2D == dim && JAG_GEO_WGS84 == srid ) {
		JagGeo::interpolatePoint2D( segDist1, segFraction1, p11, p12, startPoint );
	} else {
		startPoint.x = p11.x + segFraction1*(p12.x - p11.x);
		startPoint.y = p11.y + segFraction1*(p12.y - p11.y);
		if ( dim == JAG_3D ) { startPoint.z = p11.z + segFraction1*(p12.z - p11.z); }
	}

	JagPoint3D endPoint;
	if ( JAG_2D == dim && JAG_GEO_WGS84 == srid ) {
		JagGeo::interpolatePoint2D( segDist2, segFraction2, p21, p22, endPoint );
	} else {
		endPoint.x = p21.x + segFraction2*(p22.x - p21.x);
		endPoint.y = p21.y + segFraction2*(p22.y - p21.y);
		if ( dim == JAG_3D ) { endPoint.z = p21.z + segFraction2*(p22.z - p21.z); }
	}


	if ( JAG_2D == dim ) {
		retLstr = d2s(startPoint.x) + ":" + d2s(startPoint.y);
		for ( int i=0; i < pvec.size(); ++i ) {
			retLstr += Jstr(" ") +  d2s(pvec[i].x) + ":" + d2s(pvec[i].y);
		}
		retLstr += Jstr(" " ) +  d2s(endPoint.x) + ":" + d2s(endPoint.y);
	} else {
		retLstr = d2s(startPoint.x) + ":" + d2s(startPoint.y) + ":" + d2s(startPoint.z);
		for ( int i=0; i < pvec.size(); ++i ) {
			retLstr += Jstr(" ") +  d2s(pvec[i].x) + ":" + d2s(pvec[i].y) + ":" + d2s(pvec[i].z);
		}
		retLstr += Jstr(" " ) +  d2s(endPoint.x) + ":" + d2s(endPoint.y) + ":" + d2s(endPoint.z);
	}
	
	return true;
}

bool JagLineString3D::pointOnLeft( double px, double py) const
{
    CGALPoint2D pt(px,py);
    for ( int i=0; i < point.size() -1; ++i ) {
        CGALPoint2D p( point[i].x, point[i].y);
        CGALPoint2D q( point[i+1].x, point[i+1].y);
        if ( CGAL::LEFT_TURN !=  CGAL::orientation(p,q, pt) ) {
            return false;
        }
    }
    return true;
}

bool JagLineString3D::pointOnRight( double px, double py) const
{
    CGALPoint2D pt(px,py);
    for ( int i=0; i < point.size() -1; ++i ) {
        CGALPoint2D p( point[i].x, point[i].y);
        CGALPoint2D q( point[i+1].x, point[i+1].y);
        if ( CGAL::RIGHT_TURN !=  CGAL::orientation(p,q, pt) ) {
            return false;
        }
    }
    return true;
}

double JagLineString3D::pointOnLeftRatio(double px, double py) const
{
	if ( point.size() < 1 ) { return 0.0; }
	int cnt = 0; int tot = 0;
	double x1,y1, x2,y2;
	for ( int k=0; k < point.size()-1; ++k ) {
		x1 = point[k].x; y1 = point[k].y;
		x2 = point[k+1].x; y2 = point[k+1].y;
		if ( JAG_LEFT == JagCGAL::pointRelateLine( px,py, x1,y1, x2,y2 ) ) ++cnt;
		++tot;
	}
	if ( tot < 1 ) return 0.0;
	return (double)cnt/(double)tot;
}

double JagLineString3D::pointOnRightRatio(double px, double py) const
{
	if ( point.size() < 1 ) { return 0.0; }
	int cnt = 0; int tot = 0;
	double x1,y1, x2,y2;
	for ( int k=0; k < point.size()-1; ++k ) {
		x1 = point[k].x; y1 = point[k].y;
		x2 = point[k+1].x; y2 = point[k+1].y;
		if ( JAG_RIGHT == JagCGAL::pointRelateLine( px,py, x1,y1, x2,y2 ) ) ++cnt;
		++tot;
	}
	if ( tot < 1 ) return 0.0;
	return (double)cnt/(double)tot;
}

void JagLineString::bbox2D( double &xmin, double &ymin, double &xmax, double &ymax ) const
{
	xmin = ymin = JAG_LONG_MAX;
	xmax = ymax = JAG_LONG_MIN;
	double f;
	for ( int i=0; i < point.size(); ++i ) {
		f = jagatof(point[i].x);
		if ( f < xmin ) xmin = f;
		if ( f > xmax ) xmax = f;

		f = jagatof(point[i].y);
		if ( f < ymin ) ymin = f;
		if ( f > ymax ) ymax = f;
	}
}

// Depending on op, maybe only xmin,ymin  or xmax,ymax is found
void JagLineString::minmax2D( int op, double &xmin, double &ymin, double &xmax, double &ymax ) const
{
	xmin = ymin = JAG_LONG_MAX;
	xmax = ymax = JAG_LONG_MIN;
	double f;
	for ( int i=0; i < point.size(); ++i ) {
		if ( JAG_FUNC_XMINPOINT == op ) {
			f = jagatof(point[i].x);
			if ( f < xmin ) { xmin = f; ymin = jagatof(point[i].y); }
		} else if ( JAG_FUNC_YMINPOINT == op ) {
			f = jagatof(point[i].y);
			if ( f < ymin ) { ymin = f; xmin = jagatof(point[i].x); }
		} else if ( JAG_FUNC_XMAXPOINT == op ) {
			f = jagatof(point[i].x);
			if ( f > xmax ) { xmax = f; ymax = jagatof(point[i].y); }
		} else if ( JAG_FUNC_YMAXPOINT == op ) {
			f = jagatof(point[i].y);
			if ( f > ymax ) { ymax = f; xmax = jagatof(point[i].x); }
		}
	}
}


void JagLineString::bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	double f;
	for ( int i=0; i < point.size(); ++i ) {
		f = jagatof(point[i].x);
		if ( f < xmin ) xmin = f;
		if ( f > xmax ) xmax = f;

		f = jagatof(point[i].y);
		if ( f < ymin ) ymin = f;
		if ( f > ymax ) ymax = f;

		f = jagatof(point[i].z);
		if ( f < zmin ) zmin = f;
		if ( f > zmax ) zmax = f;
	}
}

// Depending on op, maybe only xmin,ymin  or xmax,ymax or other is found
void JagLineString::minmax3D( int op, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	double f;
	for ( int i=0; i < point.size(); ++i ) {
		if ( JAG_FUNC_XMINPOINT == op ) {
			f = jagatof(point[i].x);
			if ( f < xmin ) { xmin = f; ymin = jagatof(point[i].y); zmin = jagatof(point[i].z); }
		} else if ( JAG_FUNC_YMINPOINT == op ) {
			f = jagatof(point[i].y);
			if ( f < ymin ) { ymin = f; xmin = jagatof(point[i].x); zmin = jagatof(point[i].z); }
		} else if ( JAG_FUNC_ZMINPOINT == op ) {
			f = jagatof(point[i].z);
			if ( f < zmin ) { zmin = f; xmin = jagatof(point[i].x); ymin = jagatof(point[i].y); }
		} else if ( JAG_FUNC_XMAXPOINT == op ) {
			f = jagatof(point[i].x);
			if ( f > xmax ) { xmax = f; ymax = jagatof(point[i].y); zmax = jagatof(point[i].z); }
		} else if ( JAG_FUNC_YMAXPOINT == op ) {
			f = jagatof(point[i].y);
			if ( f > ymax ) { ymax = f; xmax = jagatof(point[i].x); zmax = jagatof(point[i].z); }
		} else if ( JAG_FUNC_ZMAXPOINT == op ) {
			f = jagatof(point[i].z);
			if ( f > zmax ) { zmax = f; xmax = jagatof(point[i].x); ymax = jagatof(point[i].y); }
		}
	}
}

void JagLineString::center2D( double &cx, double &cy, bool dropLast ) const
{
	cx = cy = 0.0;

	int len = point.size();
	if ( dropLast) --len;
	if ( len < 1 ) return;
	for ( int i=0; i < len; ++i ) {
		cx += jagatof(point[i].x);
		cy += jagatof(point[i].y);
	}
	cx = cx/len;
	cy = cy/len;
}
void JagLineString::center3D( double &cx, double &cy, double &cz, bool dropLast ) const
{
	cx = cy = cz = 0.0;
	int len = point.size();
	if ( dropLast) --len;
	if ( len < 1 ) return;
	for ( int i=0; i < len; ++i ) {
		cx += jagatof(point[i].x);
		cy += jagatof(point[i].y);
		cz += jagatof(point[i].z);
	}
	cx = cx/len;
	cy = cy/len;
	cz = cz/len;
}


void JagLineString3D::center3D( double &cx, double &cy, double &cz, bool dropLast ) const
{
	cx = cy = cz = 0.0;
	int len = point.size();
	if ( dropLast ) --len;
	if ( len < 1 ) return;
	for ( int i=0; i < len; ++i ) {
		cx += point[i].x;
		cy += point[i].y;
		cz += point[i].z;
	}
	cx = cx/len;
	cy = cy/len;
	cz = cz/len; 
}

void JagLineString3D::center2D( double &cx, double &cy, bool dropLast ) const
{
	cx = cy = 0.0;
	int len = point.size();
    if ( dropLast ) --len;
   	if ( len < 1 ) return;
	for ( int i=0; i < len; ++i ) {
		cx += point[i].x;
		cy += point[i].y;
	}
	cx = cx/len;
	cy = cy/len;
}

void JagLineString3D::bbox2D( double &xmin, double &ymin, double &xmax, double &ymax ) const
{
	xmin = ymin = JAG_LONG_MAX;
	xmax = ymax = JAG_LONG_MIN;
	double f;
	for ( int i=0; i < point.size(); ++i ) {
		f = point[i].x;
		if ( f < xmin ) xmin = f;
		if ( f > xmax ) xmax = f;

		f = point[i].y;
		if ( f < ymin ) ymin = f;
		if ( f > ymax ) ymax = f;
	}
}

void JagLineString3D::bbox3D( double &xmin, double &ymin, double &zmin,double &xmax, double &ymax, double &zmax ) const
{
	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	double f;
	for ( int i=0; i < point.size(); ++i ) {
		f = point[i].x;
		if ( f < xmin ) xmin = f;
		if ( f > xmax ) xmax = f;

		f = point[i].y;
		if ( f < ymin ) ymin = f;
		if ( f > ymax ) ymax = f;

		f = point[i].z;
		if ( f < zmin ) zmin = f;
		if ( f > zmax ) zmax = f;
	}
}

// Depending on op, maybe only xmin,ymin  or xmax,ymax or other is found
void JagLineString3D::minmax3D( int op, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	double f;
	for ( int i=0; i < point.size(); ++i ) {
		if ( JAG_FUNC_XMINPOINT == op ) {
			f = jagatof(point[i].x);
			if ( f < xmin ) { xmin = f; ymin = jagatof(point[i].y); zmin = jagatof(point[i].z); }
		} else if ( JAG_FUNC_YMINPOINT == op ) {
			f = jagatof(point[i].y);
			if ( f < ymin ) { ymin = f; xmin = jagatof(point[i].x); zmin = jagatof(point[i].z); }
		} else if ( JAG_FUNC_ZMINPOINT == op ) {
			f = jagatof(point[i].z);
			if ( f < zmin ) { zmin = f; xmin = jagatof(point[i].x); ymin = jagatof(point[i].y); }
		} else if ( JAG_FUNC_XMAXPOINT == op ) {
			f = jagatof(point[i].x);
			if ( f > xmax ) { xmax = f; ymax = jagatof(point[i].y); zmax = jagatof(point[i].z); }
		} else if ( JAG_FUNC_YMAXPOINT == op ) {
			f = jagatof(point[i].y);
			if ( f > ymax ) { ymax = f; xmax = jagatof(point[i].x); zmax = jagatof(point[i].z); }
		} else if ( JAG_FUNC_ZMAXPOINT == op ) {
			f = jagatof(point[i].z);
			if ( f > zmax ) { zmax = f; xmax = jagatof(point[i].x); ymax = jagatof(point[i].y); }
		}
	}
}

void JagLineString3D::reverse()
{
	int len = point.size();
	JagPoint3D t;
	for ( int i=0; i < len/2; ++i ) {
		if ( i != len-i-1 ) {
			t = point[i];
			point[i] = point[len-i-1];
			point[len-i-1] = t;
		}
	}
}

void JagLineString::reverse()
{
	int len = point.size();
	JagPoint t;
	for ( int i=0; i < len/2; ++i ) {
		if ( i != len-i-1 ) {
			t = point[i];
			point[i] = point[len-i-1];
			point[len-i-1] = t;
		}
	}
}

void JagLineString3D::scale( double fx, double fy, double fz, bool is3D )
{
	for ( int i=0; i < point.size(); ++i ) {
		point[i].x *= fx;
		point[i].y *= fy;
		if ( is3D ) point[i].z *= fz;
	}
}

void JagLineString3D::translate( double fx, double fy, double fz, bool is3D )
{
	for ( int i=0; i < point.size(); ++i ) {
		point[i].x += fx;
		point[i].y += fy;
		if ( is3D ) point[i].z += fz;
	}
}

void JagLineString3D::transscale( double dx, double dy, double dz, double fx, double fy, double fz, bool is3D )
{
	for ( int i=0; i < point.size(); ++i ) {
		point[i].x += dx;
		point[i].x *= fx;
		point[i].y += dy;
		point[i].y *= fy;
		if ( is3D ) { point[i].z += dz; point[i].z *= fz; }
	}
}

void JagLineString3D::scaleat(double x0, double y0, double z0, double fx, double fy, double fz, bool is3D )
{
	for ( int i=0; i < point.size(); ++i ) {
		point[i].x = x0 + fx*(point[i].x-x0);
		point[i].y = y0 + fy*(point[i].y-y0);
		if ( is3D ) point[i].z = z0 + fz*(point[i].z-z0);
	}
}

void JagLineString3D::rotateat( double alpha, double x0, double y0 )
{
	double newx, newy;
	for ( int i=0; i < point.size(); ++i ) {
		::rotateat( point[i].x, point[i].y, alpha, x0, y0, newx, newy );
		point[i].x = newx;
		point[i].y = newy;
	}
}

void JagLineString3D::rotateself( double alpha )
{
	double cx, cy;
	center2D( cx, cy, false );
	rotateat( alpha, cx, cy );
}


void JagLineString3D::affine2d( double a, double b, double d, double e, double dx,double dy )
{
	double newx, newy;
	for ( int i=0; i < point.size(); ++i ) {
		::affine2d( point[i].x, point[i].y, a, b, d, e, dx, dy, newx, newy );
		point[i].x = newx;
		point[i].y = newy;
	}
}

void JagLineString3D::affine3d( double a, double b, double c, double d, double e, double f, double g, double h, double i, 
								double dx, double dy, double dz )
{
	double newx, newy, newz;
	for ( int i=0; i < point.size(); ++i ) {
		::affine3d( point[i].x, point[i].y, point[i].z, a, b, c, d, e, f, g, h, i, dx, dy, dz, newx, newy, newz );
		point[i].x = newx;
		point[i].y = newy;
		point[i].z = newz;
	}
}


void JagLineString::scale( double fx, double fy, double fz, bool is3D )
{
	for ( int i=0; i < point.size(); ++i ) {
		strcpy( point[i].x, d2s(jagatof(point[i].x)*fx).c_str() );
		strcpy( point[i].y, d2s(jagatof(point[i].y)*fy).c_str() );
		if ( is3D ) strcpy( point[i].z, d2s(jagatof(point[i].z)*fz).c_str() );
	}
}

void JagLineString::translate( double fx, double fy, double fz, bool is3D )
{
	for ( int i=0; i < point.size(); ++i ) {
		strcpy( point[i].x, d2s(jagatof(point[i].x)+fx).c_str() );
		strcpy( point[i].y, d2s(jagatof(point[i].y)+fy).c_str() );
		if ( is3D ) strcpy( point[i].z, d2s(jagatof(point[i].z)+fz).c_str() );
	}
}

void JagLineString::transscale( double dx, double dy, double dz, double fx, double fy, double fz, bool is3D )
{
	for ( int i=0; i < point.size(); ++i ) {
		strcpy( point[i].x, d2s(fx*(jagatof(point[i].x)+dx)).c_str() );
		strcpy( point[i].y, d2s(fy*(jagatof(point[i].y)+dy)).c_str() );
		if ( is3D ) strcpy( point[i].z, d2s(fz*(jagatof(point[i].z)+dz)).c_str() );
	}
}

void JagLineString::scaleat(double x0, double y0, double z0, double fx, double fy, double fz, bool is3D )
{
	double x, y, z;
	for ( int i=0; i < point.size(); ++i ) {
		x = jagatof(point[i].x);
		y = jagatof(point[i].y);
		x = x0 + fx*(x-x0);
		y = y0 + fy*(y-y0);
		strcpy( point[i].x, d2s(x).c_str() );
		strcpy( point[i].y, d2s(y).c_str() );
		if ( is3D ) {
			z = jagatof(point[i].z);
			z = z0 + fz*(z-z0);
			strcpy( point[i].z, d2s(z).c_str() );
		}
	}
}


///////// JagSquare2D
JagSquare2D::JagSquare2D(double inx, double iny, double ina, double innx, int insrid )
{
	init(inx, iny, ina, innx, insrid );
}

JagSquare2D::JagSquare2D( const JagStrSplit &sp, int insrid )
{
    double px = jagatof( sp[JAG_SP_START+0] );
    double py = jagatof( sp[JAG_SP_START+1] );
    double a1 = jagatof( sp[JAG_SP_START+2] );
    double nx1 = jagatof( sp[JAG_SP_START+3] );
    init(px,py, a1, nx1, insrid );
}

void JagSquare2D::init(double inx, double iny, double ina, double innx, int insrid )
{
	if ( jagGE( innx, 1.0 ) ) innx = 1.0;
	if ( jagLE( innx, -1.0 ) ) innx = -1.0;

	double a1, b1;
	if ( 0 == srid ) {
		a1 = ina; b1 = ina;
	} else {
		double ux = JagGeo::meterToLon( insrid, ina, inx, iny );
		double uy = JagGeo::meterToLat( insrid, ina, inx, iny );
		a1 = ux * ( 1.0-innx*innx ) + uy * innx * innx;
		b1 = uy * ( 1.0-innx*innx ) + ux * innx * innx;
	}

	x0 =inx; y0 = iny; a=ina; nx = innx; srid=insrid;
	d("s9280 JagSquare2D::init a1=%.3f b1=%.3f a=%.3f nx=%.2f\n", a1, b1, a, nx );

	if ( fabs(nx) <= JAG_ZERO ) {
    	JagPoint2D p( x0-a1, y0-b1 ); 
    	point[0] = p;
    	p.x = x0+a1; p.y = y0-b1;
    	point[1] = p;
    	p.x = x0+a1; p.y = y0+b1;
    	point[2] = p;
    	p.x = x0-a1; p.y = y0+b1;
    	point[3] = p;
	} else if ( jagGE(fabs(nx), 1.0) ) {
    	JagPoint2D p( x0-b1, y0-a1 ); 
    	point[0] = p;
    	p.x = x0+b1; p.y = y0-a1;
    	point[1] = p;
    	p.x = x0+b1; p.y = y0+a1;
    	point[2] = p;
    	p.x = x0-b1; p.y = y0+a1;
    	point[3] = p;
	} else {
		JagRectangle2D::setPoint( x0, y0, a1, b1, nx, point );
	}
}


double JagSquare2D::getXMin( double &y )
{
    double v = JAG_LONG_MAX;
    for ( int i=0; i < 4; ++i ) {
        if ( point[i].x < v ) { v = point[i].x; y = point[i].y; }
    }
    return v;
}

double JagSquare2D::getYMin( double &x )
{
    double v = JAG_LONG_MAX;
    for ( int i=0; i < 4; ++i ) {
        if ( point[i].y < v ) { v = point[i].y; x = point[i].x; }
    }
    return v;
}
double JagSquare2D::getXMax( double &y )
{
    double v = JAG_LONG_MIN;
    for ( int i=0; i < 4; ++i ) {
        if ( point[i].x > v ) { v = point[i].x; y = point[i].y; }
    }
    return v;
}

double JagSquare2D::getYMax( double &x )
{
    double v = JAG_LONG_MIN;
    for ( int i=0; i < 4; ++i ) {
        if ( point[i].y > v ) { v = point[i].y; x = point[i].x; }
    }
    return v;
}


// JagSquare3D
JagSquare3D::JagSquare3D( const JagStrSplit &sp, int insrid )
{
    double px = jagatof( sp[JAG_SP_START+0] );
    double py = jagatof( sp[JAG_SP_START+1] );
    double pz = jagatof( sp[JAG_SP_START+2] );
    double a1 = jagatof( sp[JAG_SP_START+3] );
    double nx1 = jagatof( sp[JAG_SP_START+4] );
    double ny1 = jagatof( sp[JAG_SP_START+5] );
    init(px,py, pz, a1, nx1, ny1, insrid );
}

JagSquare3D::JagSquare3D(double inx, double iny, double inz, double ina, double innx, double inny, int insrid )
{
    init(inx,iny,inz, ina, innx, inny, insrid );
}

void JagSquare3D::init(double inx, double iny, double inz, double ina, double innx, double inny, int insrid )
{
	//d("s28039 JagRectangle2D::init inx=%f iny=%f ina=%f inb=%f innx=%f insrid=%d\n", inx, iny, ina, inb, innx, insrid );
	if ( jagGE( innx, 1.0 ) ) innx = 1.0;
	if ( jagLE( innx, -1.0 ) ) innx = -1.0;
	if ( jagGE( inny, 1.0 ) ) inny = 1.0;
	if ( jagLE( inny, -1.0 ) ) inny = -1.0;
	double a1, b1;

	if ( 0 == srid ) {
		a1 = ina; b1 = ina;
	} else {
		double ux = JagGeo::meterToLon( insrid, ina, inx, iny );
		double uy = JagGeo::meterToLat( insrid, ina, inx, iny );
		a1 = ux * ( 1.0-innx*innx ) + uy * innx * innx;
		b1 = uy * ( 1.0-innx*innx ) + ux * innx * innx;
	}

	//d("s2049 JagRectangle2D::init ux=%f uy=%f a=%f b=%f\n", ux, uy, a, b );
	x0 =inx; y0 = iny; z0 = inz; a=ina; nx = innx; ny=inny; srid=insrid;
	JagSquare3D::setPoint( x0, y0, z0, a1, b1, nx, ny, point );
}


void JagSquare3D::setPoint( double x0, double y0, double z0, double a, double b, double nx, double ny, JagPoint3D point[] )
{
	// counter-clock-wise
   	JagPoint3D p1( -a, -b, 0.0 ); 
   	p1.transform( x0,y0,z0, nx, ny);
   	point[0] = p1;
   
   	JagPoint3D p2( a, -b, 0.0 ); 
    p2.transform( x0,y0,z0,nx,ny);
    point[1] = p2;
    
   	JagPoint3D p3( a, b, 0.0 ); 
    p3.transform( x0,y0,z0,nx,ny);
    point[2] = p3;
    
   	JagPoint3D p4( -a, b, 0.0 ); 
    p4.transform( x0,y0,z0,nx,ny);
    point[3] = p4;
}


/////////// JagRectangle2D
JagRectangle2D::JagRectangle2D( const JagStrSplit &sp, int insrid )
{
    double px = jagatof( sp[JAG_SP_START+0] );
    double py = jagatof( sp[JAG_SP_START+1] );
    double a = jagatof( sp[JAG_SP_START+2] );
    double b = jagatof( sp[JAG_SP_START+3] );
    double nx = jagatof( sp[JAG_SP_START+4] );
    init(px,py, a, b, nx, insrid );
}

JagRectangle2D::JagRectangle2D(double inx, double iny, double ina, double inb, double innx, int insrid )
{
	init(inx, iny, ina, inb, innx, insrid );
}

void JagRectangle2D::init(double inx, double iny, double ina, double inb, double innx, int insrid )
{
	d("s28039 JagRectangle2D::init inx=%f iny=%f ina=%f inb=%f innx=%f insrid=%d\n", inx, iny, ina, inb, innx, insrid );
	if ( jagGE( innx, 1.0 ) ) innx = 1.0;
	if ( jagLE( innx, -1.0 ) ) innx = -1.0;
	double a1, b1;
	if ( 0 == srid ) {
		a1 = ina;
		b1 = inb;
	} else {
		double ux = JagGeo::meterToLon( insrid, ina, inx, iny );
		double uy = JagGeo::meterToLat( insrid, inb, inx, iny );
		a1 = ux * ( 1.0-innx*innx ) + uy * innx * innx;
		b1 = uy * ( 1.0-innx*innx ) + ux * innx * innx;
		d("s2049 JagRectangle2D::init ux=%f uy=%f a1=%f b1=%f\n", ux, uy, a1, b1 );
	}

	x0 =inx; y0 = iny; a=ina; b=inb; nx = innx; srid=insrid;
	if ( fabs(nx) < JAG_ZERO ) {
    	JagPoint2D p( x0-a1, y0-b1 ); 
    	point[0] = p;
    	p.x = x0+a1; p.y = y0-b1;
    	point[1] = p;
    	p.x = x0+a1; p.y = y0+b1;
    	point[2] = p;
    	p.x = x0-a1; p.y = y0+b1;
    	point[3] = p;
	} else if ( jagGE( fabs(nx), 1.0) ) {
    	JagPoint2D p( x0-b1, y0-a1 ); 
    	point[0] = p;
    	p.x = x0+b1; p.y = y0-a1;
    	point[1] = p;
    	p.x = x0+b1; p.y = y0+a1;
    	point[2] = p;
    	p.x = x0-b1; p.y = y0+a1;
    	point[3] = p;
	} else {
		JagRectangle2D::setPoint( x0, y0, a1, b1, nx, point );
	}
}


void JagRectangle2D::setPoint( double x0, double y0, double a, double b, double nx, JagPoint2D point[] )
{
	// counter-clock-wise
   	JagPoint2D p1( -a, -b ); 
   	p1.transform( x0,y0,nx);
   	point[0] = p1;
   
   	JagPoint2D p2( a, -b ); 
    p2.transform( x0,y0,nx);
    point[1] = p2;
    
   	JagPoint2D p3( a, b ); 
    p3.transform( x0,y0,nx);
    point[2] = p3;
    
   	JagPoint2D p4( -a, b ); 
    p4.transform( x0,y0,nx);
    point[3] = p4;
}

double JagRectangle2D::getXMin( double &y )
{
	printf("s2222081 JagRectangle2D getXMin\n");
    double v = JAG_LONG_MAX;
    for ( int i=0; i < 4; ++i ) {
		printf("s2222081 JagRectangle2D getXMin i=%d x=%f\n", i, point[i].x );
        if ( point[i].x < v ) { v = point[i].x; y = point[i].y; }
    }
	fflush(stdout);
    return v;
}

double JagRectangle2D::getYMin( double &x )
{
    double v = JAG_LONG_MAX;
    for ( int i=0; i < 4; ++i ) {
        if ( point[i].y < v ) { v = point[i].y; x = point[i].x; }
    }
    return v;
}

double JagRectangle2D::getXMax( double &y )
{
    double v = JAG_LONG_MIN;
    for ( int i=0; i < 4; ++i ) {
        if ( point[i].x > v ) { v = point[i].x; y = point[i].y; }
    }
    return v;
}

double JagRectangle2D::getYMax( double &x )
{
    double v = JAG_LONG_MIN;
    for ( int i=0; i < 4; ++i ) {
        if ( point[i].y > v ) { v = point[i].y; x = point[i].x; }
    }
    return v;
}



////////// JagRectangle3D
JagRectangle3D::JagRectangle3D( const JagStrSplit &sp, int insrid )
{
    double px = jagatof( sp[JAG_SP_START+0] );
    double py = jagatof( sp[JAG_SP_START+1] );
    double pz = jagatof( sp[JAG_SP_START+2] );
    double a = jagatof( sp[JAG_SP_START+3] );
    double b = jagatof( sp[JAG_SP_START+4] );
    double nx = jagatof( sp[JAG_SP_START+5] );
    double ny = jagatof( sp[JAG_SP_START+6] );
    init(px,py, pz, a, b, nx, ny, insrid );
}

JagRectangle3D::JagRectangle3D(double inx, double iny, double inz, 
			double ina, double inb, double innx, double inny, int insrid )
{
    init(inx,iny,inz, ina, inb, innx, inny, insrid );
}

void JagRectangle3D::init(double inx, double iny, double inz, double ina, double inb, double innx, double inny, int insrid )
{
	//d("s28039 JagRectangle2D::init inx=%f iny=%f ina=%f inb=%f innx=%f insrid=%d\n", inx, iny, ina, inb, innx, insrid );
	if ( jagGE( innx, 1.0 ) ) innx = 1.0;
	if ( jagLE( innx, -1.0 ) ) innx = -1.0;
	if ( jagGE( inny, 1.0 ) ) inny = 1.0;
	if ( jagLE( inny, -1.0 ) ) inny = -1.0;
	double ux = JagGeo::meterToLon( insrid, ina, inx, iny );
	double uy = JagGeo::meterToLat( insrid, inb, inx, iny );
	double a1 = ux * ( 1.0-innx*innx ) + uy * innx * innx;
	double b1 = uy * ( 1.0-innx*innx ) + ux * innx * innx;

	//d("s2049 JagRectangle2D::init ux=%f uy=%f a=%f b=%f\n", ux, uy, a, b );
	x0 =inx; y0 = iny; z0 = inz; a=ina; b=inb; nx = innx; ny=inny; srid=insrid;
	JagRectangle3D::setPoint( x0, y0, z0, a1, b1, nx, ny, point );
}


void JagRectangle3D::setPoint( double x0, double y0, double z0, double a, double b, double nx, double ny, JagPoint3D point[] )
{
	// counter-clock-wise
   	JagPoint3D p1( -a, -b, 0.0 ); 
   	p1.transform( x0,y0,z0, nx, ny);
   	point[0] = p1;
   
   	JagPoint3D p2( a, -b, 0.0 ); 
    p2.transform( x0,y0,z0,nx,ny);
    point[1] = p2;
    
   	JagPoint3D p3( a, b, 0.0 ); 
    p3.transform( x0,y0,z0,nx,ny);
    point[2] = p3;
    
   	JagPoint3D p4( -a, b, 0.0 ); 
    p4.transform( x0,y0,z0,nx,ny);
    point[3] = p4;
}


///////// JagCircle2D
JagCircle2D::JagCircle2D( double inx, double iny, double inr, int insrid )
{
	init( inx, iny, inr, insrid );
}

JagCircle2D::JagCircle2D( const JagStrSplit &sp, int insrid )
{
	//d("s2277 JagCircle2D ctor sp:  insrid=%d\n", srid );
	//sp.print();
    double px = jagatof( sp[JAG_SP_START+0] );
    double py = jagatof( sp[JAG_SP_START+1] );
    double r = jagatof( sp[JAG_SP_START+2] );
	//d("s9283 ctor of JagCircle2D px=%f py=%f r=%f\n", px, py, r );
    init(px,py, r, insrid );
}
void JagCircle2D::init( double inx, double iny, double inr, int insrid )
{
	x0 =inx; y0 = iny; r = inr; srid = insrid;
}

///////// JagCircle3D
JagCircle3D::JagCircle3D( double inx, double iny, double inz, double inr, double innx, double inny, int insrid )
{
	init( inx, iny, inz, inr, innx, inny, insrid );
}

JagCircle3D::JagCircle3D( const JagStrSplit &sp, int insrid )
{
	//d("s2277 JagCircle3D ctor sp:  insrid=%d\n", srid );
	//sp.print();
    double px = jagatof( sp[JAG_SP_START+0] );
    double py = jagatof( sp[JAG_SP_START+1] );
    double pz = jagatof( sp[JAG_SP_START+2] );
    double r = jagatof( sp[JAG_SP_START+3] );
    double nx = jagatof( sp[JAG_SP_START+4] );
    double ny = jagatof( sp[JAG_SP_START+5] );
    init(px,py,pz, r, nx, ny, insrid );
}
void JagCircle3D::init( double inx, double iny, double inz, double inr, double innx, double inny, int insrid )
{
	x0 =inx; y0 = iny; z0=inz; r = inr; nx=innx; ny=inny; srid = insrid;
}

////// JagEllipse2D
JagEllipse2D::JagEllipse2D( double inx, double iny, double ina, double inb, double innx, int insrid )
{
    init( inx, iny, ina, inb, innx, insrid );
}

JagEllipse2D::JagEllipse2D( const JagStrSplit &sp, int insrid )
{
    double px = jagatof( sp[JAG_SP_START+0] );
    double py = jagatof( sp[JAG_SP_START+1] );
    double a = jagatof( sp[JAG_SP_START+2] );
    double b = jagatof( sp[JAG_SP_START+3] );
    double nx = jagatof( sp[JAG_SP_START+4] );
    init(px,py, a, b, nx, insrid );
}

void JagEllipse2D::init( double inx, double iny, double ina, double inb, double innx, int insrid )
{
	x0 =inx; y0 = iny; a = ina; b = inb; nx = innx; srid=insrid;
}

void JagEllipse2D::bbox2D( double &xmin, double &ymin, double &xmax, double &ymax ) const
{
	double ux = JagGeo::meterToLon( srid, a, x0, y0 );
	double uy = JagGeo::meterToLat( srid, b, x0, y0 );
	double a1 = ux * ( 1.0-nx*nx ) + uy * nx * nx;
	double b1 = uy * ( 1.0-nx*nx ) + ux * nx * nx;
	ellipseBoundBox( x0, y0, a1, b1, nx, xmin, xmax, ymin, ymax );
}

void JagEllipse2D::minmax2D( int op, double &xmin, double &ymin, double &xmax, double &ymax ) const
{
	double ux = JagGeo::meterToLon( srid, a, x0, y0 );
	double uy = JagGeo::meterToLat( srid, b, x0, y0 );
	double a1 = ux * ( 1.0-nx*nx ) + uy * nx * nx;
	double b1 = uy * ( 1.0-nx*nx ) + ux * nx * nx;
	ellipseMinMax( op, x0, y0, a1, b1, nx, xmin, xmax, ymin, ymax );
}


///////// JagEllipse3D
JagEllipse3D::JagEllipse3D( double inx, double iny, double inz, double ina, double inb, double innx, double inny, int insrid )
{
    init( inx, iny, inz, ina, inb, innx, inny, insrid );
}

JagEllipse3D::JagEllipse3D( const JagStrSplit &sp, int insrid )
{
    double px = jagatof( sp[JAG_SP_START+0] );
    double py = jagatof( sp[JAG_SP_START+1] );
    double pz = jagatof( sp[JAG_SP_START+2] );
    double a = jagatof( sp[JAG_SP_START+3] );
    double b = jagatof( sp[JAG_SP_START+4] );
    double nx = jagatof( sp[JAG_SP_START+5] );
    double ny = jagatof( sp[JAG_SP_START+6] );
    init(px,py, pz,a, b, nx,ny, insrid );
}

void JagEllipse3D::init( double inx, double iny, double inz, double ina, double inb, double innx, double inny, int insrid )
{
	x0 =inx; y0 = iny; z0 = inz; a = ina; b = inb; nx = innx; ny=inny; srid=insrid;
}

void JagEllipse3D::bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	double ux = JagGeo::meterToLon( srid, a, x0, y0 );
	double uy = JagGeo::meterToLat( srid, b, x0, y0 );
	double a1 = ux * ( 1.0-nx*nx ) + uy * nx * nx;
	double b1 = uy * ( 1.0-nx*nx ) + ux * nx * nx;
	//ellipseBoundBox( x0, y0, a1, b, nx, xmin, xmax, ymin, ymax );

	// project to x-y plane
	double nxy2  = nx*nx + ny*ny;
	double sqrt_nxy2  = sqrt(nx*nx + ny*ny);
	double nz = sqrt( 1.0 - nxy2 );
	double a2 = a1*nz;
	double b2 = b1;  // short semi-axis no change given the way of nx,ny,nz projection 
	
	ellipseBoundBox( x0, y0, a2, b2, nx, xmin, xmax, ymin, ymax );
	zmin  = z0 - a*sqrt_nxy2;
	zmax  = z0 + a*sqrt_nxy2;
}


// JagTriangle2D
JagTriangle2D::JagTriangle2D( const JagStrSplit &sp, int insrid )
{
    double x1 = jagatof( sp[JAG_SP_START+0] );
    double y1 = jagatof( sp[JAG_SP_START+1] );
    double x2 = jagatof( sp[JAG_SP_START+2] );
    double y2 = jagatof( sp[JAG_SP_START+3] );
    double x3 = jagatof( sp[JAG_SP_START+4] );
    double y3 = jagatof( sp[JAG_SP_START+5] );
	init( x1, y1, x2, y2, x3, y3, insrid );
}

JagTriangle2D::JagTriangle2D( double inx1, double iny1, double inx2, double iny2, double inx3, double iny3, int insrid )
{
	init( inx1, iny1, inx2, iny2, inx3, iny3, insrid );
}

void JagTriangle2D::init( double inx1, double iny1, double inx2, double iny2, double inx3, double iny3, int insrid )
{
	x1 = inx1; y1 = iny1;
	x2 = inx2; y2 = iny2;
	x3 = inx3; y3 = iny3;
	srid = insrid;
}

// JagTriangle3D
JagTriangle3D::JagTriangle3D( const JagStrSplit &sp, int insrid )
{
    double x1 = jagatof( sp[JAG_SP_START+0] );
    double y1 = jagatof( sp[JAG_SP_START+1] );
    double z1 = jagatof( sp[JAG_SP_START+2] );
    double x2 = jagatof( sp[JAG_SP_START+3] );
    double y2 = jagatof( sp[JAG_SP_START+4] );
    double z2 = jagatof( sp[JAG_SP_START+5] );
    double x3 = jagatof( sp[JAG_SP_START+6] );
    double y3 = jagatof( sp[JAG_SP_START+7] );
    double z3 = jagatof( sp[JAG_SP_START+8] );
	init( x1, y1, z1, x2, y2, z2, x3, y3, z3, insrid );
}

JagTriangle3D::JagTriangle3D( double inx1, double iny1, double inz1, 
							  double inx2, double iny2, double inz2, 
							  double inx3, double iny3, double inz3, int insrid )
{
	init( inx1, iny1, inz1, inx2, iny2, inz2, inx3, iny3, inz3, insrid );
}

void JagTriangle3D::init( double inx1, double iny1, double inz1, double inx2, 
					double iny2, double inz2, double inx3, double iny3, double inz3, int insrid )
{
	x1 = inx1; y1 = iny1; z1=inz1;
	x2 = inx2; y2 = iny2; z2=inz2;
	x3 = inx3; y3 = iny3; z3=inz3;
	srid = insrid;
}


/////////// JagPolygon
void JagPolygon::center2D(  double &cx, double &cy ) const
{
	cx = cy = 0.0;
	// JagVector<JagLineString3D> linestr;
	int len = linestr.size();
	//d("s10007 len=[%d]\n", len );
	if ( len < 1 ) return;
	double x, y;
	for ( int i=0; i < len; ++i ) {
		linestr[i].center2D(x, y, true);
		cx += x;
		cy += y;
	}

	cx = cx / len;
	cy = cy / len;
}

void JagPolygon::center3D(  double &cx, double &cy, double &cz ) const
{
	cx = cy = cz = 0.0;
	int len = linestr.size();
	if ( len < 1 ) return;
	double x, y, z;
	for ( int i=0; i < len; ++i ) {
		linestr[i].center3D(x, y, z, true);

		cx += x;
		cy += y;
		cz += z;
	}

	cx = cx / len;
	cy = cy / len;
	cz = cz / len;
}

bool JagPolygon::bbox2D( double &xmin, double &ymin, double &xmax, double &ymax ) const
{
	int len = linestr.size();
	if ( len < 1 ) return false;
	double xmi, ymi, xma, yma;
	xmin = ymin =  JAG_LONG_MAX;
	xmax = ymax = JAG_LONG_MIN;
	int cnt = 0;
	for ( int i=0; i < len; ++i ) {
		if ( linestr[i].size() < 1 ) continue;
		linestr[i].bbox2D( xmi, ymi, xma, yma );
		if ( xmi < xmin ) xmin = xmi;
		if ( ymi < ymin ) ymin = ymi;
		if ( xma > xmax ) xmax = xma;
		if ( yma > ymax ) ymax = yma;
		++cnt;
	}

	d("s2928 JagPolygon::bbox2D cnt=%d\n", cnt );
	return (cnt > 0 );
}

bool JagPolygon::bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	int len = linestr.size();
	if ( len < 1 ) return false;
	double xmi, ymi, zmi, xma, yma, zma;
	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	for ( int i=0; i < len; ++i ) {
		linestr[i].bbox3D( xmi, ymi, zmi, xma, yma, zma );
		if ( xmi < xmin ) xmin = xmi;
		if ( ymi < ymin ) ymin = ymi;
		if ( zmi < zmin ) zmin = zmi;
		if ( xma > xmax ) xmax = xma;
		if ( yma > ymax ) ymax = yma;
		if ( zma > zmax ) zmax = zma;
	}

	return true;
}

bool JagPolygon::minmax2D( int op, double &xmin, double &ymin, double &xmax, double &ymax ) const
{
	xmin = ymin = xmax = ymax = 0.0;
	int len = linestr.size();
	if ( len < 1 ) return false;
	xmin = ymin =  JAG_LONG_MAX;
	xmax = ymax = JAG_LONG_MIN;
	int cnt = 0;
	for ( int i=0; i < len; ++i ) {
		if ( linestr[i].size() < 1 ) continue;
		const JagLineString3D  &lstr =  linestr[i];
		for ( int j=0; j < lstr.size(); ++j ) {
			const JagPoint3D &p = lstr[j];
			if ( JAG_FUNC_XMINPOINT == op ) { 
				if ( p.x < xmin ) { xmin = p.x; ymin = p.y; }
			} else if ( JAG_FUNC_XMAXPOINT == op ) {
				if ( p.x > xmax ) { xmax = p.x; ymax = p.y; }
			} else if ( JAG_FUNC_YMINPOINT == op ) {
				if ( p.y < ymin ) { ymin = p.y; xmin = p.x; }
			} else if ( JAG_FUNC_YMAXPOINT == op ) {
				if ( p.y > ymax ) { ymax = p.y; xmax = p.x; }
			}
			++cnt;
		}
	}

	d("s2928 JagPolygon::bbox2D cnt=%d\n", cnt );
	return (cnt > 0 );
}

bool JagPolygon::minmax3D( int op, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	xmin = ymin = zmin= xmax = ymax = zmax = 0.0;
	int len = linestr.size();
	if ( len < 1 ) return false;
	xmin = ymin = zmin=  JAG_LONG_MAX;
	xmax = ymax = zmax= JAG_LONG_MIN;
	int cnt = 0;
	for ( int i=0; i < len; ++i ) {
		if ( linestr[i].size() < 1 ) continue;
		const JagLineString3D  &lstr =  linestr[i];
		for ( int j=0; j < lstr.size(); ++j ) {
			const JagPoint3D &p = lstr[j];
			if ( JAG_FUNC_XMINPOINT == op ) { 
				if ( p.x < xmin ) { xmin = p.x; ymin = p.y; zmin = p.z; }
			} else if ( JAG_FUNC_XMAXPOINT == op ) {
				if ( p.x > xmax ) { xmax = p.x; ymax = p.y; zmax = p.z; }
			} else if ( JAG_FUNC_YMINPOINT == op ) {
				if ( p.y < ymin ) { ymin = p.y; xmin = p.x; zmin = p.z; }
			} else if ( JAG_FUNC_YMAXPOINT == op ) {
				if ( p.y > ymax ) { ymax = p.y; xmax = p.x; zmax = p.z; }
			} else if ( JAG_FUNC_ZMINPOINT == op ) {
				if ( p.z < zmin ) { zmin = p.z; xmin = p.x; ymin = p.y; }
			} else if ( JAG_FUNC_ZMAXPOINT == op ) {
				if ( p.z > zmax ) { zmax = p.z; xmax = p.x; ymax = p.y; }
			}
			++cnt;
		}
	}

	return (cnt > 0 );
}


double JagPolygon::lineLength( bool removeLast, bool is3D, int srid )
{
	int len = linestr.size();
	if ( len < 1 ) return 0.0;
	double sum = 0.0;
	for ( int i=0; i < len; ++i ) {
		sum += linestr[i].lineLength( removeLast, is3D, srid );
		//d("s0393 i=%d/%d sum=%f\n", i, len, sum );
	}
	return sum;
}

void JagPolygon::toWKT( bool is3D, bool hasHdr, const Jstr &objname, Jstr &str ) const
{
	if ( linestr.size() < 1 ) { str=""; return; }
	if ( hasHdr ) {
		str = objname +  "(";
	} else {
		str = "(";
	}

	for ( int i=0; i < linestr.size(); ++i ) {
		if ( i==0 ) {
			str += "(";
		} else {
			str += ",(";
		}
		const JagLineString3D &lstr = linestr[i];
		for (  int j=0; j< lstr.size(); ++j ) {
			if ( j>0) { str += Jstr(","); }
			str += d2s(lstr.point[j].x) + " " +  d2s(lstr.point[j].y);
			if ( is3D ) { str += Jstr(" ") + d2s(lstr.point[j].z); }
		}
		str += ")";
	}
	
	str += ")";
}

// linestring 
void JagPolygon::toOneWKT( bool is3D, bool hasHdr, const Jstr &objname, Jstr &str ) const
{
	if ( linestr.size() < 1 ) { str=""; return; }
	if ( hasHdr ) {
		str = objname +  "(";
	} else {
		str = "(";
	}
	const JagLineString3D &lstr = linestr[0];
	for (  int j=0; j< lstr.size(); ++j ) {
		if ( j>0) { str += Jstr(","); }
		str += d2s(lstr.point[j].x) + " " +  d2s(lstr.point[j].y);
		if ( is3D ) { str += Jstr(" ") + d2s(lstr.point[j].z); }
	}
	str += ")";
}

void JagPolygon::toJAG( bool is3D, bool hasHdr,  const Jstr &inbbox, int srid, Jstr &str ) const
{
	if ( linestr.size() < 1 ) { str=""; return; }
	if ( hasHdr ) {
		Jstr srids = intToStr( srid );
		Jstr bbox;
		Jstr mk = "OJAG=";
		if ( is3D ) {
			if ( inbbox.size() < 1 )  { bbox = "0:0:0:0:0:0"; mk="CJAG="; } else { bbox = inbbox; }
			str = mk + srids + "=0=PL3=d " + bbox;
		} else {
			if ( inbbox.size() < 1 )  { bbox = "0:0:0:0"; mk="CJAG="; } else { bbox = inbbox; }
			str = mk + srids + "=0=PL=d " + bbox;
		}
	} 

	for ( int i=0; i < linestr.size(); ++i ) {
		if ( i>0 ) {
			str += " |";
		}
		const JagLineString3D &lstr = linestr[i];
		d("s1127 JagPolygon::toJAG i=%d lstr.size=%d\n", i, lstr.size() );
		for (  int j=0; j< lstr.size(); ++j ) {
			str += Jstr(" ") + d2s(lstr.point[j].x) + ":" +  d2s(lstr.point[j].y);
			if ( is3D ) { str += Jstr(":") + d2s(lstr.point[j].z); }
		}
	}
	
}

// convert vector 2D shapes to polygon
JagPolygon::JagPolygon( const JagSquare2D &sq, bool isClosed )
{
	JagLineString3D ls;
	for ( int i=0; i <4; ++i ) {
		ls.add( sq.point[i].x, sq.point[i].y );
	}

	if ( isClosed ) {
		ls.add( sq.point[0].x, sq.point[0].y );
	}

	linestr.append( ls );
	//d("s2938 JagPolygon JagSquare2D linestr.print():\n" );
	//linestr.print();
}

JagPolygon::JagPolygon( const JagSquare3D &sq, bool isClosed )
{
	JagLineString3D ls;
	for ( int i=0; i <4; ++i ) {
		ls.add( sq.point[i].x, sq.point[i].y, sq.point[i].z );
	}

	if ( isClosed ) {
		ls.add( sq.point[0].x, sq.point[0].y, sq.point[0].z );
	}

	linestr.append( ls );
	//d("s2938 JagPolygon JagSquare2D linestr.print():\n" );
	//linestr.print();
}


JagPolygon::JagPolygon( const JagRectangle2D &rect, bool isClosed )
{
	JagLineString3D ls;
	for ( int i=0; i <4; ++i ) {
		ls.add( rect.point[i].x, rect.point[i].y );
	}
	if ( isClosed ) {
		ls.add( rect.point[0].x, rect.point[0].y );
	}
	linestr.append( ls );
}

JagPolygon::JagPolygon( const JagRectangle3D &rect, bool isClosed )
{
	JagLineString3D ls;
	for ( int i=0; i <4; ++i ) {
		ls.add( rect.point[i].x, rect.point[i].y, rect.point[i].z );
	}
	if ( isClosed ) {
		ls.add( rect.point[0].x, rect.point[0].y, rect.point[0].z );
	}
	linestr.append( ls );
}

JagPolygon::JagPolygon( const JagCircle2D &cir, int samples, bool isClosed )
{
	//d("s0125 JagPolygon JagCircle2D samples=%d cir.r=%f\n", samples, cir.r );
	JagLineString3D ls;
	JagVector<JagPoint2D> vec;
	JagGeo::samplesOn2DCircle( cir.x0, cir.y0, cir.r, samples, vec );
	//d("s0123 vec.size=%d\n", vec.size() );
	for ( int i=0; i < vec.size(); ++i ) {
		ls.add( vec[i].x, vec[i].y );
	}
	if ( isClosed ) {
		ls.add( vec[0].x, vec[0].y );
	}
	linestr.append( ls );
}

JagPolygon::JagPolygon( const JagCircle3D &cir, int samples, bool isClosed )
{
	//d("s0125 JagPolygon JagCircle2D samples=%d cir.r=%f\n", samples, cir.r );
	JagLineString3D ls;
	JagVector<JagPoint3D> vec;
	JagGeo::samplesOn3DCircle( cir.x0, cir.y0, cir.z0, cir.r, cir.nx, cir.ny, samples, vec );
	//d("s0123 vec.size=%d\n", vec.size() );
	for ( int i=0; i < vec.size(); ++i ) {
		ls.add( vec[i].x, vec[i].y, vec[i].z );
	}

	if ( isClosed ) {
		ls.add( vec[0].x, vec[0].y, vec[0].z );
	}
	linestr.append( ls );
}

JagPolygon::JagPolygon( const JagEllipse2D &e, int samples, bool isClosed )
{
	JagLineString3D ls;
	JagVector<JagPoint2D> vec;
	JagGeo::samplesOn2DEllipse( e.x0, e.y0, e.a, e.b, e.nx, samples, vec );
	for ( int i=0; i < vec.size(); ++i ) {
		ls.add( vec[i].x, vec[i].y );
	}

	if ( isClosed ) {
		ls.add( vec[0].x, vec[0].y );
	}
	linestr.append( ls );
}

JagPolygon::JagPolygon( const JagEllipse3D &e, int samples, bool isClosed )
{
	JagLineString3D ls;
	JagVector<JagPoint3D> vec;
	JagGeo::samplesOn3DEllipse( e.x0, e.y0, e.z0, e.a, e.b, e.nx, e.ny, samples, vec );
	for ( int i=0; i < vec.size(); ++i ) {
		ls.add( vec[i].x, vec[i].y, vec[i].z );
	}

	if ( isClosed ) {
		ls.add( vec[0].x, vec[0].y, vec[0].z );
	}
	linestr.append( ls );
}

JagPolygon::JagPolygon( const JagTriangle2D &t, bool isClosed )
{
	JagLineString3D ls;
	ls.add( t.x1, t.y1 ); 
	ls.add( t.x2, t.y2 ); 
	ls.add( t.x3, t.y3 ); 
	if ( isClosed ) {
		ls.add( t.x1, t.y1 ); 
	}
	linestr.append( ls );
}

JagPolygon::JagPolygon( const JagTriangle3D &t, bool isClosed )
{
	JagLineString3D ls;
	ls.add( t.x1, t.y1, t.z1 ); 
	ls.add( t.x2, t.y2, t.z2 ); 
	ls.add( t.x3, t.y3, t.z3 ); 
	if ( isClosed ) {
		ls.add( t.x1, t.y1, t.z1 ); 
	}
	linestr.append( ls );
}

void JagPolygon::toVector2D( int srid, JagVector<JagPoint2D> &vec, bool outerRingOnly )
{
	if ( linestr.size() < 1 ) { return; }
	double x, y;
	for ( int i=0; i < linestr.size(); ++i ) {
		const JagLineString3D &lstr = linestr[i];
		for (  int j=0; j< lstr.size(); ++j ) {
			JagGeo::lonLatToXY( srid, lstr.point[j].x, lstr.point[j].y, x, y );
			vec.append( JagPoint2D(x,y));
		}
		if ( outerRingOnly ) break;
	}
}

void JagPolygon::toVector3D( int srid, JagVector<JagPoint3D> &vec, bool outerRingOnly )
{
	if ( linestr.size() < 1 ) { return; }
	double x, y, z;
	for ( int i=0; i < linestr.size(); ++i ) {
		const JagLineString3D &lstr = linestr[i];
		for (  int j=0; j< lstr.size(); ++j ) {
			JagGeo::lonLatAltToXYZ( srid, lstr.point[j].x, lstr.point[j].y, lstr.point[j].z, x, y, z );
			d("s3418 toVector3D point srid=%d j=%d x=%f y=%f z=%f  --> x=%f y=%f z=%f\n", srid, j,  lstr.point[j].x, lstr.point[j].y, lstr.point[j].z, x, y, z ) ;
			vec.append( JagPoint3D(x,y,z) );
		}
		if ( outerRingOnly ) break;
	}
}

double JagPolygon::pointOnLeftRatio(double px, double py) const
{
	if ( linestr.size() < 1 ) { return 0.0; }
	int cnt = 0; int tot = 0;
	for ( int k=0; k < linestr.size(); ++k ) {
		const JagLineString3D &lstr = linestr[k];
		if ( lstr.pointOnLeft(px,py) ) ++cnt;
		++tot;
	}
	if ( tot < 1 ) return 0.0;
	return (double)cnt/(double)tot;
}

double JagPolygon::pointOnRightRatio(double px, double py) const
{
	if ( linestr.size() < 1 ) { return 0.0; }
	int cnt = 0; int tot = 0;
	for ( int k=0; k < linestr.size(); ++k ) {
		const JagLineString3D &lstr = linestr[k];
		if ( lstr.pointOnRight(px,py) ) ++cnt;
		++tot;
	}
	if ( tot < 1 ) return 0.0;
	return (double)cnt/(double)tot;
}

bool JagPolygon::pointOnLeft( double px, double py) const
{
    for ( int k = 0; k < linestr.size(); ++k ) {
        const JagLineString3D &lstr = linestr[k];
        if ( ! lstr.pointOnLeft( px, py ) ) return false;
    }
    return true;
}

bool JagPolygon::pointOnRight( double px, double py) const
{
    for ( int k = 0; k < linestr.size(); ++k ) {
        const JagLineString3D &lstr = linestr[k];
        if ( ! lstr.pointOnRight( px, py ) ) return false;
    }
    return true;
}

void JagPolygon::knn( int dim, int srid, double px, double py, double pz, int K, double min, double max, Jstr &value )
{
	JagMinMaxDistance minmax(min,max);
	if ( 2 == dim ) {
		std::vector<JagSimplePoint2D> points;
		for (int i=0; i < linestr.size(); ++i ) {
			const JagLineString3D &lstr = linestr[i];
			for ( int j=0; j < lstr.size(); ++j ) {
				points.push_back( JagSimplePoint2D(lstr.point[j].x, lstr.point[j].y) );
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
		for (int i=0; i < linestr.size(); ++i ) {
			const JagLineString3D &lstr = linestr[i];
			for ( int j=0; j < lstr.size(); ++j ) {
				points.push_back( JagSimplePoint3D(lstr.point[j].x, lstr.point[j].y, lstr.point[j].z) );
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

// JagCube
JagCube::JagCube( const JagStrSplit &sp, int insrid )
{
    x0 = jagatof( sp[JAG_SP_START+0] );
    y0 = jagatof( sp[JAG_SP_START+1] );
    z0 = jagatof( sp[JAG_SP_START+2] );
    a = jagatof( sp[JAG_SP_START+3] );
    nx = jagatof( sp[JAG_SP_START+4] );
    ny = jagatof( sp[JAG_SP_START+5] );
	srid = insrid;
	/**
	d("s222228 JagCube x0=%f y0=%f z0=%f  a=%f nx=%f ny=%f \n", x0, y0, z0, a, nx, ny );
	// sp.print();
	**/

	if ( jagGE( nx, 1.0 ) ) nx = 1.0;
	if ( jagLE( nx, -1.0 ) ) nx = -1.0;
	if ( jagGE( ny, 1.0 ) ) ny = 1.0;
	if ( jagLE( ny, -1.0 ) ) ny = -1.0;
	double a1, b1;
	if ( 0 == insrid ) {
		a1 = a;
		b1 = a;
	} else {
		double ux = JagGeo::meterToLon( insrid, a, x0, y0 );
		double uy = JagGeo::meterToLat( insrid, a, x0, y0 );
		a1 = ux * ( 1.0-nx*nx ) + uy * nx * nx;
		b1 = uy * ( 1.0-nx*nx ) + ux * nx * nx;
	}

	// counter-clock-wise
   	JagPoint3D p1( -a1, -b1, -a ); 
   	p1.transform( x0,y0,z0, nx, ny);
   	point[0] = p1;
	//p1.print();
   
   	JagPoint3D p2( a1, -b1, -a ); 
    p2.transform( x0,y0,z0,nx,ny);
    point[1] = p2;
	//p2.print();
    
   	JagPoint3D p3( a1, b1, -a ); 
    p3.transform( x0,y0,z0,nx,ny);
    point[2] = p3;
	//p3.print();
    
   	JagPoint3D p4( -a1, b1, -a ); 
    p4.transform( x0,y0,z0,nx,ny);
    point[3] = p4;
	//p4.print();

	// upper plane
   	JagPoint3D p5( -a1, -b1, a ); 
    p5.transform( x0,y0,z0,nx,ny);
    point[4] = p5;
	//p5.print();

   	JagPoint3D p6( a1, -b1, a ); 
    p6.transform( x0,y0,z0,nx,ny);
    point[5] = p6;
	//p6.print();
    
   	JagPoint3D p7( a1, b1, a ); 
    p7.transform( x0,y0,z0,nx,ny);
    point[6] = p7;
	//p7.print();
    
   	JagPoint3D p8( -a1, b1, a ); 
    p8.transform( x0,y0,z0,nx,ny);
    point[7] = p8;
	//p8.print();
}

void JagCube::bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	int len = 8; // 8 vertices
	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	for ( int i=0; i < len; ++i ) {
		if ( point[i].x < xmin ) xmin = point[i].x;
		if ( point[i].x > xmax ) xmax = point[i].x;

		if ( point[i].y < ymin ) ymin = point[i].y;
		if ( point[i].y > ymax ) ymax = point[i].y;

		if ( point[i].z < zmin ) zmin = point[i].z;
		if ( point[i].z > zmax ) zmax = point[i].z;
	}
}

void JagCube::minmax3D( int op, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	int len = 8; // 8 vertices
	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	double f;
	for ( int i=0; i < len; ++i ) {
		if ( JAG_FUNC_XMINPOINT == op ) {
			f = point[i].x;
			if ( f < xmin ) { xmin = f; ymin = point[i].y; zmin = point[i].z; }
		} else if ( JAG_FUNC_YMINPOINT == op ) {
			f = point[i].y;
			if ( f < ymin ) { ymin = f; xmin = point[i].x; zmin = point[i].z; }
		} else if ( JAG_FUNC_XMAXPOINT == op ) {
			f = point[i].x;
			if ( f > xmax ) { xmax = f; ymax = point[i].y; zmax = point[i].z; }
		} else if ( JAG_FUNC_YMAXPOINT == op ) {
			f = point[i].y;
			if ( f > ymax ) { ymax = f; xmax = point[i].x; zmax = point[i].z; }
		} else if ( JAG_FUNC_ZMINPOINT == op ) {
			f = point[i].z;
			if ( f < zmin ) { zmin = f; xmin = point[i].x; ymin = point[i].y; }
		} else if ( JAG_FUNC_ZMAXPOINT == op ) {
			f = point[i].z;
			if ( f > zmax ) { zmax = f; xmax = point[i].x; ymax = point[i].y; }
		}
	}
}

void JagCube::print()
{
	int len = 8; 
	for ( int i=0; i < len; ++i ) {
		point[i].print();
		printf("\n");
	}
}

// JagBox
JagBox::JagBox( const JagStrSplit &sp, int insrid )
{
    x0 = jagatof( sp[JAG_SP_START+0] );
    y0 = jagatof( sp[JAG_SP_START+1] );
    z0 = jagatof( sp[JAG_SP_START+2] );
    a = jagatof( sp[JAG_SP_START+3] );
    b = jagatof( sp[JAG_SP_START+4] );
    c = jagatof( sp[JAG_SP_START+5] );
    nx = jagatof( sp[JAG_SP_START+6] );
    ny = jagatof( sp[JAG_SP_START+7] );
	srid = insrid;

	if ( jagGE( nx, 1.0 ) ) nx = 1.0;
	if ( jagLE( nx, -1.0 ) ) nx = -1.0;
	if ( jagGE( ny, 1.0 ) ) ny = 1.0;
	if ( jagLE( ny, -1.0 ) ) ny = -1.0;
	double a1;
	double b1;
	if ( 0 == insrid ) {
		a1 = a; b1 = b;
	} else {
		double ux = JagGeo::meterToLon( insrid, a, x0, y0 );
		double uy = JagGeo::meterToLat( insrid, b, x0, y0 );
		a1 = ux * ( 1.0-nx*nx ) + uy * nx * nx;
		b1 = uy * ( 1.0-nx*nx ) + ux * nx * nx;
	}

	// counter-clock-wise
   	JagPoint3D p1( -a1, -b1, -c ); 
   	p1.transform( x0,y0,z0, nx, ny);
   	point[0] = p1;
   
   	JagPoint3D p2( a1, -b1, -c ); 
    p2.transform( x0,y0,z0,nx,ny);
    point[1] = p2;
    
   	JagPoint3D p3( a1, b1, -c ); 
    p3.transform( x0,y0,z0,nx,ny);
    point[2] = p3;
    
   	JagPoint3D p4( -a1, b1, -c ); 
    p4.transform( x0,y0,z0,nx,ny);
    point[3] = p4;

	// upper plane
   	JagPoint3D p5( -a1, -b1, c ); 
    p5.transform( x0,y0,z0,nx,ny);
    point[4] = p5;

   	JagPoint3D p6( a1, -b1, c ); 
    p6.transform( x0,y0,z0,nx,ny);
    point[5] = p6;
    
   	JagPoint3D p7( a1, b1, c ); 
    p7.transform( x0,y0,z0,nx,ny);
    point[6] = p7;
    
   	JagPoint3D p8( -a1, b1, c ); 
    p8.transform( x0,y0,z0,nx,ny);
    point[7] = p8;
}

void JagBox::bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	int len = 8; // 8 vertices
	xmin = ymin = zmin = JAG_LONG_MAX;
	xmax = ymax = zmax = JAG_LONG_MIN;
	for ( int i=0; i < len; ++i ) {
		if ( point[i].x < xmin ) xmin = point[i].x;
		if ( point[i].x > xmax ) xmax = point[i].x;

		if ( point[i].y < ymin ) ymin = point[i].y;
		if ( point[i].y > ymax ) ymax = point[i].y;

		if ( point[i].z < zmin ) zmin = point[i].z;
		if ( point[i].z > zmax ) zmax = point[i].z;
	}
}

// JagSphere
JagSphere::JagSphere( const JagStrSplit &sp, int insrid )
{
    x0 = jagatof( sp[JAG_SP_START+0] );
    y0 = jagatof( sp[JAG_SP_START+1] );
    z0 = jagatof( sp[JAG_SP_START+2] );
    r = jagatof( sp[JAG_SP_START+3] );
	srid = insrid;
}

void JagSphere::bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	double a1 = JagGeo::meterToLon( srid, r, x0, y0 );
	double b1 = JagGeo::meterToLat( srid, r, x0, y0 );

    xmin = x0-a1; xmax=x0+a1;
    ymin = y0-b1; ymax=y0+b1;
    zmin = z0-r; zmax=z0+r;
}


// JagEllipsoid
JagEllipsoid::JagEllipsoid( const JagStrSplit &sp, int insrid )
{
    x0 = jagatof( sp[JAG_SP_START+0] );
    y0 = jagatof( sp[JAG_SP_START+1] );
    z0 = jagatof( sp[JAG_SP_START+2] );
    a = jagatof( sp[JAG_SP_START+3] );
    b = jagatof( sp[JAG_SP_START+4] );
    c = jagatof( sp[JAG_SP_START+5] );
    nx = jagatof( sp[JAG_SP_START+6] );
    ny = jagatof( sp[JAG_SP_START+7] );
	srid = insrid;
}

void JagEllipsoid::bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	double a1;
	double b1;
	if ( 0 == srid ) {
		a1 = a; b1 = b;
	} else {
		double ux = JagGeo::meterToLon( srid, a, x0, y0 );
		double uy = JagGeo::meterToLat( srid, b, x0, y0 );
		a1 = ux * ( 1.0-nx*nx ) + uy * nx * nx;
		b1 = uy * ( 1.0-nx*nx ) + ux * nx * nx;
	}

    xmin = x0-a1; xmax=x0+a1;
    ymin = y0-b1; ymax=y0+b1;
    zmin = z0-c; zmax=z0+c;
}


// JagCylinder
JagCylinder::JagCylinder( const JagStrSplit &sp, int insrid )
{
    x0 = jagatof( sp[JAG_SP_START+0] );
    y0 = jagatof( sp[JAG_SP_START+1] );
    z0 = jagatof( sp[JAG_SP_START+2] );
    a = jagatof( sp[JAG_SP_START+3] );
    c = jagatof( sp[JAG_SP_START+4] );
    nx = jagatof( sp[JAG_SP_START+5] );
    ny = jagatof( sp[JAG_SP_START+6] );
	srid = insrid;
}

void JagCylinder::bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	double a1;
	double b1;

	if ( 0 == srid ) {
		a1 = b1 = a;
	} else {
		double ux = JagGeo::meterToLon( srid, a, x0, y0 );
		double uy = JagGeo::meterToLat( srid, a, x0, y0 );
		a1 = ux * ( 1.0-nx*nx ) + uy * nx * nx;
		b1 = uy * ( 1.0-nx*nx ) + ux * nx * nx;
	}
	//ellipseBoundBox( x0, y0, a1, b, nx, xmin, xmax, ymin, ymax );

	// project to x-y plane
	double nxy2  = nx*nx + ny*ny;
	double sqrt_nxy2  = sqrt(nx*nx + ny*ny);  // sin(theta)
	double nz = sqrt( 1.0 - nxy2 );
	double a2 = a1*nz;
	double b2 = b1;  // short semi-axis no change given the way of nx,ny,nz projection 
	
	double xmin2, xmax2, ymin2, ymax2; //top circle
	double xmin1, xmax1, ymin1, ymax1; // bottom circle
	ellipseBoundBox( x0+a*sqrt_nxy2*nx, y0+a*sqrt_nxy2*ny, a2, b2, nx, xmin2, xmax2, ymin2, ymax2 );
	ellipseBoundBox( x0-a*sqrt_nxy2*nx, y0-a*sqrt_nxy2*ny, a2, b2, nx, xmin1, xmax1, ymin1, ymax1 );
	xmin = jagmin(xmin1,xmin2);
	xmax = jagmax(xmax1,xmax2);

	ymin = jagmin(ymin1,ymin2);
	ymax = jagmax(ymax1,ymax2);

	zmin  = z0 - c*nz - a*sqrt_nxy2;
	zmax  = z0 + c*nz + a*sqrt_nxy2;
}


// JagCone
JagCone::JagCone( const JagStrSplit &sp, int insrid )
{
    x0 = jagatof( sp[JAG_SP_START+0] );
    y0 = jagatof( sp[JAG_SP_START+1] );
    z0 = jagatof( sp[JAG_SP_START+2] );
    a = jagatof( sp[JAG_SP_START+3] );
    c = jagatof( sp[JAG_SP_START+4] );
    nx = jagatof( sp[JAG_SP_START+5] );
    ny = jagatof( sp[JAG_SP_START+6] );
	srid = insrid;
}

void JagCone::bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const
{
	double a1;
	double b1;
	if ( 0 == srid ) {
		a1 = b1 = a;
	} else {
		double ux = JagGeo::meterToLon( srid, a, x0, y0 );
		double uy = JagGeo::meterToLat( srid, a, x0, y0 );
		a1 = ux * ( 1.0-nx*nx ) + uy * nx * nx;
		b1 = uy * ( 1.0-nx*nx ) + ux * nx * nx;
	}

	// project to x-y plane
	double nxy2  = nx*nx + ny*ny;
	double sqrt_nxy2  = sqrt(nx*nx + ny*ny);
	double nz = sqrt( 1.0 - nxy2 );
	double a2 = a1*nz;
	double b2 = b1;  // short semi-axis no change given the way of nx,ny,nz projection 

	double xmin2, xmax2, ymin2, ymax2; // top tip
	double xmin1, xmax1, ymin1, ymax1; // bottom circle
	xmin2 = xmax2 =  x0+a*sqrt_nxy2*nx;
	ymin2 = ymax2 =  y0+a*sqrt_nxy2*ny;

	ellipseBoundBox( x0-a*sqrt_nxy2*nx, y0-a*sqrt_nxy2*ny, a2, b2, nx, xmin1, xmax1, ymin1, ymax1 );
	xmin = jagmin(xmin1,xmin2);
	xmax = jagmax(xmax1,xmax2);

	ymin = jagmin(ymin1,ymin2);
	ymax = jagmax(ymax1,ymax2);

	zmin  = z0 - c*nz - a*sqrt_nxy2;
	zmax  = z0 + c*nz + a*sqrt_nxy2;
	
}


void JagPoint2D::transform( double x0, double y0, double nx0 )
{
	double inx, iny;
	inx = x; iny = y;
	JagGeo::transform2DCoordLocal2Global( x0,y0, x,y, nx0, x, y );
}

void JagPoint3D::transform( double x0, double y0, double z0, double nx0, double ny0 )
{
	double inx, iny, inz;
	inx = x; iny = y; inz = z;
	JagGeo::transform3DCoordLocal2Global( x0,y0,z0, inx,iny,inz, nx0, ny0, x, y, z );
}

void JagLine2D::transform( double x0, double y0, double nx0 )
{
	double inx, iny;
	inx = x1; iny = y1;
	JagGeo::transform2DCoordLocal2Global( x0,y0, inx,iny, nx0, x1, y1 );

	inx = x2; iny = y2;
	JagGeo::transform2DCoordLocal2Global( x0,y0, inx,iny, nx0, x2, y2 );
}

void JagLine3D::transform( double x0, double y0, double z0, double nx0, double ny0  )
{
	double inx, iny, inz;
	inx = x1; iny = y1; inz = z1;
	JagGeo::transform3DCoordLocal2Global( x0,y0,z0, inx,iny,inz, nx0,ny0, x1, y1, z1 );

	inx = x2; iny = y2; inz = z2;
	JagGeo::transform3DCoordLocal2Global( x0,y0,z0, inx,iny,inz, nx0,ny0, x2,y2,z2 );
}


void JagRectangle3D::transform( double x20, double y20, double z20, double nx20, double ny20 )
{
	// xyx nx ny is self value
	JagGeo::transform3DCoordLocal2Global( x20,y20,z20, x0,y0,z0, nx20, ny20, x0, y0, z0 );
	// transform3DDirection( double nx1, double ny1, double nx2, double ny2, double &nx, double &ny );
	JagGeo::transform3DDirection( nx, ny, nx20, ny20, nx, ny );
}

void JagTriangle3D::transform( double x0, double y0, double z0, double nx0, double ny0 )
{
	JagGeo::transform3DCoordLocal2Global( x0,y0,z0, x1,y1,z1, nx0, ny0, x1, y1, z1 );
	JagGeo::transform3DCoordLocal2Global( x0,y0,z0, x2,y2,z2, nx0, ny0, x2, y2, z2 );
	JagGeo::transform3DCoordLocal2Global( x0,y0,z0, x3,y3,z3, nx0, ny0, x3, y3, z3 );
}


////////////// JagLineSeg2D ///////////////
bool JagLineSeg2D::operator>( const JagLineSeg2D &o) const
{
	if ( JagGeo::above(x1,y1, o.x1,o.y1,o.x2,o.y2) ) return true;
	return false;
}

bool JagLineSeg2D::operator>=( const JagLineSeg2D &o) const
{
	if ( JagGeo::aboveOrSame(x1,y1, o.x1,o.y1,o.x2,o.y2) ) return true;
	return false;
}

bool JagLineSeg2D::operator<( const JagLineSeg2D &o) const
{
	if ( JagGeo::below(x1,y1, o.x1,o.y1,o.x2,o.y2) ) return true;
	return false;
}

bool JagLineSeg2D::operator<=( const JagLineSeg2D &o) const
{
	if ( JagGeo::belowOrSame(x1,y1, o.x1,o.y1,o.x2,o.y2) ) return true;
	return false;
}

bool JagLineSeg2D::operator==( const JagLineSeg2D &o) const
{
	bool b1 =  isNull(); bool b2 =  o.isNull();
	//d("s0284 JagLineSeg2D '==' b1=%d b2=%d\n", b1, b2 );
	if ( b1 && b2 ) return true;
	if (  b1 || b2 ) return false;

	//d("s0284 JagLineSeg2D '==' b1=%d b2=%d\n", b1, b2 );
	//d("s0284 x1=%.1f y1=%.1f o.x1=%.1f o.y1=%.1f o.x2=%.1f o.y2=%.1f\n", x1,y1, o.x1,o.y1,o.x2,o.y2 );

	if ( JagGeo::same(x1,y1, o.x1,o.y1,o.x2,o.y2) ) return true;
	return false;
}
bool JagLineSeg2D::operator!=( const JagLineSeg2D &o) const
{
	bool b1 =  isNull(); bool b2 =  o.isNull();
	//d("s0284 JagLineSeg2D '!=' b1=%d b2=%d\n", b1, b2 );
	if ( b1 && b2 ) return false;
	if (  b1 || b2 ) return true;

	//d("s1284 JagLineSeg2D '!=' b1=%d b2=%d\n", b1, b2 );
	//d("s1284 x1=%.1f y1=%.1f o.x1=%.1f o.y1=%.1f o.x2=%.1f o.y2=%.1f\n", x1,y1, o.x1,o.y1,o.x2,o.y2 );

	if ( JagGeo::same(x1,y1, o.x1,o.y1,o.x2,o.y2) ) return false;
	return true;
}

jagint JagLineSeg2D::hashCode() const
{
	return (x1+y1+x2+y2)*4129.293/7;
}

bool JagLineSeg2D::isNull() const 
{
	if ( jagEQ(x1, JAG_LONG_MIN) && jagEQ(x2, JAG_LONG_MIN) 
		 && jagEQ(y1, JAG_LONG_MIN) && jagEQ(y2, JAG_LONG_MIN) ) {
		return true;
	}
	return false;
}

////////////// JagLineSeg3D ///////////////
bool JagLineSeg3D::operator>( const JagLineSeg3D &o) const
{
	if ( JagGeo::above(x1,y1,z1, o.x1,o.y1,o.z1,o.x2,o.y2,o.z2) ) return true;
	return false;
}

bool JagLineSeg3D::operator>=( const JagLineSeg3D &o) const
{
	if ( JagGeo::aboveOrSame(x1,y1,z1, o.x1,o.y1,o.z1,o.x2,o.y2,o.z2) ) return true;
	return false;
}

bool JagLineSeg3D::operator<( const JagLineSeg3D &o) const
{
	if ( JagGeo::below(x1,y1,z1, o.x1,o.y1,o.z1,o.x2,o.y2,o.z2) ) return true;
	return false;
}

bool JagLineSeg3D::operator<=( const JagLineSeg3D &o) const
{
	if ( JagGeo::belowOrSame(x1,y1,z1, o.x1,o.y1,o.z1,o.x2,o.y2,o.z2) ) return true;
	return false;
}

bool JagLineSeg3D::operator==( const JagLineSeg3D &o) const
{
	bool b1 =  isNull(); bool b2 =  o.isNull();
	//d("s0284 JagLineSeg3D '==' b1=%d b2=%d\n", b1, b2 );
	if ( b1 && b2 ) return true;
	if (  b1 || b2 ) return false;
	if ( JagGeo::same(x1,y1,z1, o.x1,o.y1,o.z1,o.x2,o.y2,o.z2) ) return true;
	return false;
}
bool JagLineSeg3D::operator!=( const JagLineSeg3D &o) const
{
	bool b1 =  isNull(); bool b2 =  o.isNull();
	if ( b1 && b2 ) return false;
	if (  b1 || b2 ) return true;
	if ( JagGeo::same(x1,y1,z1, o.x1,o.y1,o.z1,o.x2,o.y2,o.z2) ) return false;
	return true;
}

jagint JagLineSeg3D::hashCode() const
{
	return (x1+y1+z1+x2+y2+z2)*49.293/7;
}

bool JagLineSeg3D::isNull() const 
{
	if ( jagEQ(x1, JAG_LONG_MIN) && jagEQ(x2, JAG_LONG_MIN) 
		 && jagEQ(y1, JAG_LONG_MIN) && jagEQ(y2, JAG_LONG_MIN)
		 && jagEQ(z1, JAG_LONG_MIN) && jagEQ(z2, JAG_LONG_MIN) ) {
		return true;
	}
	return false;
}

