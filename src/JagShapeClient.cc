#include <JagGlobalDef.h>
#include <JagShape.h>

JagPoint2D::JagPoint2D()
{
	x = y = 0.0;
}

JagPoint2D::JagPoint2D( const char *sx, const char *sy)
{
	x = jagatof(sx); y = jagatof( sy );
}

JagPoint2D::JagPoint2D( double inx, double iny )
{
	x = inx; y = iny;
}

JagPoint3D::JagPoint3D()
{
	x = y = z = 0.0;
}

JagPoint3D::JagPoint3D( const char *sx, const char *sy, const char *sz)
{
	x = jagatof(sx); y = jagatof( sy ); z = jagatof( sz );
}

JagPoint3D::JagPoint3D( double inx, double iny, double inz )
{
	x = inx; y = iny; z = inz;
}

Jstr JagPoint3D::hashString() const
{
	char buf[32];
	Jstr s;
	sprintf(buf, "%.6f", x );
	s = buf;

	sprintf(buf, "%.6f", y );
	s += Jstr(":") + buf;

	sprintf(buf, "%.6f", z );
	s += Jstr(":") + buf;

	return s;
}


bool JagSortPoint2D::operator<( const JagSortPoint2D &o) const
{
	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( x1 < o.x1 ) return true;
		} else {
			if ( x1 < o.x2 ) return true;
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( x2 < o.x1 ) return true;
		} else {
			if ( x2 < o.x2 ) return true;
		}
	}

	return false;
}
bool JagSortPoint2D::operator<=( const JagSortPoint2D &o) const
{
	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( x1 < o.x1 ) return true;
		} else {
			if ( x1 < o.x2 ) return true;
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( x2 < o.x1 ) return true;
		} else {
			if ( x2 < o.x2 ) return true;
		}
	}

	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( jagEQ(x1, o.x1) ) return true;
		} else {
			if ( jagEQ(x1, o.x2) ) return true;
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( jagEQ(x2, o.x1) ) return true;
		} else {
			if ( jagEQ(x2, o.x2) ) return true;
		}
	}
	return false;
}

bool JagSortPoint2D::operator>( const JagSortPoint2D &o) const
{
	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( x1 > o.x1 ) return true;
		} else {
			if ( x1 > o.x2 ) return true;
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( x2 > o.x1 ) return true;
		} else {
			if ( x2 > o.x2 ) return true;
		}
	}
	return false;
}


bool JagSortPoint2D::operator>=( const JagSortPoint2D &o) const
{
	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( x1 > o.x1 ) return true;
		} else {
			if ( x1 > o.x2 ) return true;
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( x2 > o.x1 ) return true;
		} else {
			if ( x2 > o.x2 ) return true;
		}
	}

	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( jagEQ(x1, o.x1) ) return true;
		} else {
			if ( jagEQ(x1, o.x2) ) return true;
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( jagEQ(x2, o.x1) ) return true;
		} else {
			if ( jagEQ(x2, o.x2) ) return true;
		}
	}

	return false;
}


bool JagSortPoint3D::operator<( const JagSortPoint3D &o) const
{
	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( x1 < o.x1 ) return true;
			if ( jagEQ( x1, o.x1) ) {
				if ( y1 < o.y1 ) return true;
			}
		} else {
			if ( x1 < o.x2 ) return true;
			if ( jagEQ( x1, o.x2) ) {
				if ( y1 < o.y2 ) return true;
			}
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( x2 < o.x1 ) return true;
			if ( jagEQ( x2, o.x1) ) {
				if ( y2 < o.y1 ) return true;
			}
		} else {
			if ( x2 < o.x2 ) return true;
			if ( jagEQ( x2, o.x2) ) {
				if ( y2 < o.y2 ) return true;
			}
		}
	}

	return false;
}

bool JagSortPoint3D::operator<=( const JagSortPoint3D &o) const
{

	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( x1 < o.x1 ) return true;
			if ( jagEQ( x1, o.x1) ) {
				if ( y1 < o.y1 ) return true;
			}
		} else {
			if ( x1 < o.x2 ) return true;
			if ( jagEQ( x1, o.x2) ) {
				if ( y1 < o.y2 ) return true;
			}
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( x2 < o.x1 ) return true;
			if ( jagEQ( x2, o.x1) ) {
				if ( y2 < o.y1 ) return true;
			}
		} else {
			if ( x2 < o.x2 ) return true;
			if ( jagEQ( x2, o.x2) ) {
				if ( y2 < o.y2 ) return true;
			}
		}
	}


	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( jagEQ(x1, o.x1) && jagEQ(y1, o.y1) ) return true;
		} else {
			if ( jagEQ(x1, o.x2) && jagEQ(y1, o.y2) ) return true;
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( jagEQ(x2, o.x1) && jagEQ(y2, o.y1) ) return true;
		} else {
			if ( jagEQ(x2, o.x2) && jagEQ(y2, o.y2) ) return true;
		}
	}
	return false;
}

bool JagSortPoint3D::operator>( const JagSortPoint3D &o) const
{
	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( x1 > o.x1 ) return 1;
			if ( jagEQ( x1, o.x1) ) {
				if ( y1 > o.y1 ) return -1;
			}
		} else {
			if ( x1 > o.x2 ) return 1;
			if ( jagEQ( x1, o.x2) ) {
				if ( y1 > o.y2 ) return 1;
			}
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( x2 > o.x1 ) return 1;
			if ( jagEQ( x2, o.x1) ) {
				if ( y2 > o.y1 ) return 1;
			}
		} else {
			if ( x2 > o.x2 ) return 1;
			if ( jagEQ( x2, o.x2) ) {
				if ( y2 > o.y2 ) return 1;
			}
		}
	}

	return false;
}


bool JagSortPoint3D::operator>=( const JagSortPoint3D &o) const
{
	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( x1 > o.x1 ) return true;
			if ( jagEQ( x1, o.x1) ) {
				if ( y1 > o.y1 ) return true;
			}
		} else {
			if ( x1 > o.x2 ) return true;
			if ( jagEQ( x1, o.x2) ) {
				if ( y1 > o.y2 ) return true;
			}
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( x2 > o.x1 ) return true;
			if ( jagEQ( x2, o.x1) ) {
				if ( y2 > o.y1 ) return true;
			}
		} else {
			if ( x2 > o.x2 ) return true;
			if ( jagEQ( x2, o.x2) ) {
				if ( y2 > o.y2 ) return true;
			}
		}
	}

	if ( JAG_LEFT == end ) {
		if ( JAG_LEFT == o.end ) {
			if ( jagEQ(x1, o.x1) && jagEQ(y1, o.y1) ) return true;
		} else {
			if ( jagEQ(x1, o.x2) && jagEQ(y1, o.y2) ) return true;
		}
	} else {
		if ( JAG_LEFT == o.end ) {
			if ( jagEQ(x2, o.x1) && jagEQ(y2, o.y1) ) return true;
		} else {
			if ( jagEQ(x2, o.x2) && jagEQ(y2, o.y2) ) return true;
		}
	}

	return false;
}


JagPoint::JagPoint() { init(); }
JagPoint::JagPoint( const char *inx, const char *iny )
{
	init();
	strcpy( x, inx );
	strcpy( y, iny );
}

JagPoint::JagPoint( const char *inx, const char *iny, const char *inz )
{
	init();
	strcpy( x, inx );
	strcpy( y, iny );
	strcpy( z, inz );
}

JagPoint& JagPoint::operator=( const JagPoint& p2 ) 
{
	if ( this == &p2 ) { return *this; }
	copyData( p2 );
	return *this; 
}

bool JagPoint::equal2D(const JagPoint &p ) const
{
	return jagEQ( jagatof(x), jagatof(p.x) ) && jagEQ( jagatof(y), jagatof(p.y) );
}
bool JagPoint::equal3D(const JagPoint &p ) const
{
	return jagEQ( jagatof(x), jagatof(p.x) ) && jagEQ( jagatof(y), jagatof(p.y) ) && jagEQ( jagatof(z), jagatof(p.z) );
}


JagPoint::JagPoint( const JagPoint& p2 ) 
{
	copyData( p2 );
}
void JagPoint::init() 
{
	memset( x, 0, JAG_POINT_LEN); memset(y, 0, JAG_POINT_LEN ); memset( z, 0, JAG_POINT_LEN); 
	memset( a, 0, JAG_POINT_LEN); memset( b, 0, JAG_POINT_LEN); memset( c, 0, JAG_POINT_LEN);
	memset( nx, 0, JAG_POINT_LEN); memset( ny, 0, JAG_POINT_LEN); 
}

void JagPoint::copyData( const JagPoint& p2 )
{
	memcpy( x, p2.x, JAG_POINT_LEN); 
	memcpy( y, p2.y, JAG_POINT_LEN ); 
	memcpy( z, p2.z, JAG_POINT_LEN); 
	memcpy( a, p2.a, JAG_POINT_LEN );
	memcpy( b, p2.b, JAG_POINT_LEN );
	memcpy( c, p2.c, JAG_POINT_LEN );
	memcpy( nx, p2.nx, JAG_POINT_LEN );
	memcpy( ny, p2.ny, JAG_POINT_LEN );
	metrics = p2.metrics;
}

void JagPoint::print()  const
{
	d("x=[%s] y=[%s] z=[%s] a=[%s] b=[%s] c=[%s] nx=[%s] ny=[%s]\n", x,y,z,a,b,c,nx,ny );

}

JagLineString& JagLineString::operator=( const JagLineString3D& L2 )
{
	init();
	for ( int i=0; i < L2.size(); ++i ) {
		add( L2.point[i] );
	}
	return *this;
}

JagLineString& JagLineString::copyFrom( const JagLineString3D& L2, bool removeLast )
{
	init();

	int len =  L2.size();

	if ( removeLast ) --len;

	for ( int i=0; i < len; ++i ) {
		add( L2.point[i] );
	}
	return *this;
}

JagLineString& JagLineString::appendFrom( const JagLineString3D& L2, bool removeLast )
{
	//init();
	int len =  L2.size();
	if ( removeLast ) --len;
	for ( int i=0; i < len; ++i ) {
		add( L2.point[i] );
	}
	return *this;
}


JagLineString3D& JagLineString3D::appendFrom( const JagLineString3D& L2, bool removeLast )
{
	//init();
	int len =  L2.size();
	if ( removeLast ) --len;
	for ( int i=0; i < len; ++i ) {
		add( L2.point[i] );
	}
	return *this;
}



void JagLineString::add( const JagPoint2D &p )
{
	JagPoint pp( d2s(p.x).c_str(),  d2s(p.y).c_str() );
	pp.metrics = p.metrics;
	point.append(pp);
}

void JagLineString::add( const JagPoint3D &p )
{
	JagPoint pp( d2s(p.x).c_str(),  d2s(p.y).c_str(), d2s(p.z).c_str() );
	pp.metrics = p.metrics;
	point.append(pp);
}

void JagLineString::add( double x, double y )
{
	JagPoint pp( d2s(x).c_str(),  d2s(y).c_str() );
	point.append(pp);
}

void JagLineString::add( double x, double y, double z )
{
	JagPoint pp( d2s(x).c_str(),  d2s(y).c_str(), d2s(z).c_str() );
	point.append(pp);
}

void JagLineString3D::add( const JagPoint2D &p )
{
	JagPoint3D pp( d2s(p.x).c_str(),  d2s(p.y).c_str(), "0.0" );
	pp.metrics = p.metrics;
	point.append(pp);
}

void JagLineString3D::add( const JagPoint3D &p )
{
	JagPoint3D pp( d2s(p.x).c_str(),  d2s(p.y).c_str(), d2s(p.z).c_str() );
	pp.metrics = p.metrics;
	point.append(pp);
}

void JagLineString3D::add( double x, double y, double z )
{
	JagPoint3D pp;
	pp.x = x; pp.y = y; pp.z = z;
	point.append(pp);
}

