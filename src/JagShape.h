#ifndef _jag_shape_h_
#define _jag_shape_h_

#include <math.h>
#include <JagVector.h>
#include <JagUtil.h>

#define jagmin(a, b) ( (a) < (b) ? (a) : (b))
#define jagmax(a, b) ( (a) > (b) ? (a) : (b))
#define jagmin3(a, b, c) jagmin(jagmin(a, b),(c))
#define jagmax3(a, b, c) jagmax(jagmax(a, b),(c))
#define jagsq2(a) ( (a)*(a) )
#define jagIsPosZero(a) (((a)<JAG_ZERO) ? 1 : 0)
#define jagIsZero(a) (( fabs(a)<JAG_ZERO) ? 1 : 0)

class JagSimplePoint2D
{
  public: 
    JagSimplePoint2D() {}
    JagSimplePoint2D(double x1, double y1) { x= x1; y = y1;}
    double x, y;
};

class JagSimplePoint3D
{
  public:
    JagSimplePoint3D() {}
    JagSimplePoint3D(double x1, double y1, double z1) { x= x1; y = y1; z=z1;}
  	double x, y, z;
};

class JagMinMaxDistance
{
  public: 
  	JagMinMaxDistance() { min = -1.0; max = JAG_LONG_MAX; }
  	JagMinMaxDistance( double mi, double ma ) { min = mi; max = ma; }
  	double min, max;
};

class JagPoint2D
{
  public:
     JagPoint2D();
     JagPoint2D( const char *sx, const char *sy);
     JagPoint2D( double inx, double iny );
	 void transform( double x0, double y0, double nx0 );
	 void print() const { printf("x=%.1f y=%.1f ", x, y ); }
	 int operator< ( const JagPoint2D &p2 ) const 
	 { 
	 	return (x < p2.x && y < p2.y );
	 }
	 int operator<= ( const JagPoint2D &p2 ) const 
	 { 
	 	return (x <= p2.x && y <= p2.y );
	 }
	 int operator>= ( const JagPoint2D &p2 ) const 
	 { 
	 	return (x >= p2.x && y >= p2.y );
	 }
	 int operator== ( const JagPoint2D &p2 ) const 
	 { 
	 	return ( fabs(x-p2.x) < 0.0000001 && fabs(y - p2.y) < 0.0000001 );
	 }
	 int operator!= ( const JagPoint2D &p2 ) const 
	 { 
	 	return ( fabs(x-p2.x) > 0.0000001 || fabs(y - p2.y) > 0.0000001 );
	 }
     double x;
	 double y;
	 JagVector<Jstr> metrics;

};

class JagPoint3D
{
  public:
     JagPoint3D();
     JagPoint3D( const char *sx, const char *sy, const char *sz );
     JagPoint3D( double inx, double iny, double inz );
	 void transform( double x0, double y0, double z0, double nx0, double ny0 );
	 void print() const { printf("x=%.1f y=%.1f z=%.1f ", x, y, z ); }
	 Jstr hashString() const;
	 Jstr str3D() const { return d2s(x) + ":" + d2s(y) + ":" + d2s(z); }
	 Jstr str2D() const { return d2s(x) + ":" + d2s(y); }
	 int operator< ( const JagPoint3D &p2 ) const 
	 { 
	 	return (x < p2.x && y < p2.y && z < p2.z );
	 }
	 int operator<= ( const JagPoint3D &p2 ) const 
	 { 
	 	return (x <= p2.x && y <= p2.y && z <= p2.z );
	 }
	 int operator>= ( const JagPoint3D &p2 ) const 
	 { 
	 	return (x >= p2.x && y >= p2.y && z >= p2.z );
	 }
	 int operator== ( const JagPoint3D &p2 ) const 
	 { 
	 	return ( fabs(x-p2.x) < 0.0000001 && fabs(y - p2.y) < 0.0000001 && fabs(z - p2.z ) < 0.0000001 );
	 }
	 int operator!= ( const JagPoint3D &p2 ) const 
	 { 
	 	return ( fabs(x-p2.x) > 0.0000001 || fabs(y - p2.y) > 0.0000001 || fabs(z - p2.z ) > 0.0000001 );
	 }

     double x;
	 double y;
	 double z;
	 JagVector<Jstr> metrics;
};

class JagBox2D
{
  public:
  	 JagBox2D() {}
  	 JagBox2D( double d1, double d2, double d3, double d4 ) 
	     : xmin(d1), ymin(d2), xmax(d3), ymax(d4) {}
     double xmin, ymin, xmax, ymax;
};

class JagBox3D
{
  public:
     double xmin, ymin, zmin, xmax, ymax, zmax;
};

class JagSquare2D
{
  public:
  	JagSquare2D(){ srid = 0; }
  	JagSquare2D( double inx, double iny, double ina, double innx, int srid=0 );
  	void init( double inx, double iny, double ina, double innx, int srid );
  	JagSquare2D( const JagStrSplit &sp, int srid=0 );
	double x0, y0, a, nx; 
	int srid;
	JagPoint2D point[4];  // counter-clockwise polygon points
	double getXMin( double &y );
	double getXMax( double &y );
	double getYMin( double &x );
	double getYMax( double &x );
};

class JagSquare3D
{
  public:
  	JagSquare3D(){ srid = 0; }
  	JagSquare3D( double inx, double iny, double inz, double ina, double innx, double inny, int srid=0 );
  	void init( double inx, double iny, double inz, double ina, double innx, double inny, int srid );
  	JagSquare3D( const JagStrSplit &sp, int srid=0 );
	double x0, y0, z0, a, nx, ny; 
	static void setPoint( double x0, double y0, double z0, double a, double b, double nx, double ny, JagPoint3D point[] );
	int srid;
	JagPoint3D point[4];  // counter-clockwise polygon points
};

class JagRectangle2D
{
  public:
  	JagRectangle2D(){ srid=0;};
  	JagRectangle2D( double inx, double iny, double ina, double inb, double innx, int srid=0 );
  	JagRectangle2D( const JagStrSplit &sp, int srid=0 );
  	void init( double inx, double iny, double ina, double inb, double innx, int srid );
	static void setPoint( double x0, double y0, double a, double b, double nx, JagPoint2D point[] );
	double x0, y0, a, b, nx; 
	int srid;
	JagPoint2D point[4];  // counter-clockwise polygon points
	double getXMin( double &y );
	double getXMax( double &y );
	double getYMin( double &x );
	double getYMax( double &x );
};

class JagRectangle3D
{
  public:
  	JagRectangle3D(){ srid=0;};
  	JagRectangle3D( double inx, double iny, double inz, 
					double ina, double inb, double innx, double inny, int srid=0 );
  	JagRectangle3D( const JagStrSplit &sp, int srid=0 );
  	void init( double inx, double iny, double inz, double ina, double inb, double innx, double inny, int srid );
	static void setPoint( double x0, double y0, double z0, double a, double b, double nx, double ny, 
							JagPoint3D point[] );
	void transform( double x0, double y0, double z0, double nx0, double ny0 );

	double x0, y0, z0, a, b, nx, ny; 
	int srid;
	JagPoint3D point[4];  // counter-clockwise polygon points
};

class JagCircle2D
{
  public:
  	JagCircle2D(){ srid=0;};
  	JagCircle2D( double inx, double iny, double inr, int srid=0 );
  	void init( double inx, double iny, double inr, int srid );
  	JagCircle2D( const JagStrSplit &sp, int srid=0 );
	int srid;
	double x0, y0, r;
	JagVector<Jstr> metrics;
};

class JagCircle3D
{
  public:
  	JagCircle3D(){ srid=0;};
  	JagCircle3D( double inx, double iny, double inz, double inr, double nx, double ny, int srid=0 );
  	void init( double inx, double iny, double inz, double inr, double nx, double ny, int srid );
  	JagCircle3D( const JagStrSplit &sp, int srid=0 );
	int srid;
	double x0, y0, z0, r, nx, ny;
	JagVector<Jstr> metrics;
};

class JagEllipse2D
{
  public:
  	JagEllipse2D(){ srid = 0;};
  	JagEllipse2D( double inx, double iny, double ina, double inb, double innx, int srid=0 );
  	void init( double inx, double iny, double ina, double inb, double innx, int srid );
  	JagEllipse2D( const JagStrSplit &sp, int srid=0 );
	void bbox2D( double &xmin, double &ymin, double &xmax, double &ymax ) const;
	void minmax2D( int op, double &xmin, double &ymin, double &xmax, double &ymax ) const;
	double x0, y0, a, b, nx; 
	int srid;
	JagVector<Jstr> metrics;
};

class JagEllipse3D
{
  public:
  	JagEllipse3D(){ srid = 0;};
  	JagEllipse3D( double inx, double iny, double inz, double ina, double inb, double innx, double inny, int srid=0 );
  	void init( double inx, double iny, double inz, double ina, double inb, double innx, double inny, int srid );
  	JagEllipse3D( const JagStrSplit &sp, int srid=0 );
	void bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
	double x0, y0, z0, a, b, nx, ny; 
	int srid;
	JagVector<Jstr> metrics;
};

class JagTriangle2D
{
  public:
  	JagTriangle2D(){ srid=0;};
  	JagTriangle2D( double inx1, double iny1, double inx2, double iny2, double inx3, double iny3, int srid=0 );
  	void init( double inx1, double iny1, double inx2, double iny2, double inx3, double iny3, int srid );
  	JagTriangle2D( const JagStrSplit &sp, int srid=0 );
	double x1,y1, x2,y2, x3,y3;
	int srid;
	JagVector<Jstr> metrics;
};

class JagTriangle3D
{
  public:
  	JagTriangle3D(){ srid=0;};
  	JagTriangle3D( double inx1, double iny1, double inz1, 
					double inx2, double iny2, double inz2, 
					double inx3, double iny3, double inz3, int srid=0 );
  	void init( double inx1, double iny1, double inz1, double inx2, double iny2, double inz2, 
			   double inx3, double iny3, double inz3, int srid );
  	JagTriangle3D( const JagStrSplit &sp, int srid=0 );
	void transform( double x0, double y0, double z0, double nx0, double ny0 );
	double x1,y1,z1, x2,y2,z2, x3,y3,z3;
	int srid;
	JagVector<Jstr> metrics;
};

class JagCube
{
  public:
  	JagCube( const JagStrSplit &sp, int srid=0 );
	void bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
	void minmax3D( int op, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
	void print();
	JagPoint3D point[8];
	double x0,y0,z0, a, nx, ny;
	int srid;
};

class JagBox
{
  public:
  	JagBox( const JagStrSplit &sp, int srid=0 );
	void bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
	JagPoint3D point[8];
	double x0,y0,z0, a, b,c, nx, ny;
	int srid;
};

class JagSphere
{
  public:
  	JagSphere( const JagStrSplit &sp, int srid=0 );
	void bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
	double x0,y0,z0, r;
	int srid;
	JagVector<Jstr> metrics;
};

class JagEllipsoid
{
  public:
  	JagEllipsoid( const JagStrSplit &sp, int srid=0 );
	void bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
	double x0,y0,z0, a, b, c, nx, ny;
	int srid;
	JagVector<Jstr> metrics;
};

class JagCylinder
{
  public:
  	JagCylinder( const JagStrSplit &sp, int srid=0 );
	void bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
	double x0,y0,z0, a, c, nx, ny;
	int srid;
	JagVector<Jstr> metrics;
};

class JagCone
{
  public:
  	JagCone( const JagStrSplit &sp, int srid=0 );
	void bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
	double x0,y0,z0, a, c, nx, ny;
	int srid;
	JagVector<Jstr> metrics;
};

class JagSortPoint2D
{
  public:
     double x1, y1;
     double x2, y2;
	 unsigned char    end;
	 unsigned char    color;
	 bool operator < ( const JagSortPoint2D &o) const;
	 bool operator <= ( const JagSortPoint2D &o) const;
	 bool operator > ( const JagSortPoint2D &o) const;
	 bool operator >= ( const JagSortPoint2D &o) const;
	 void print() const { printf("x1=%.1f y1=%.1f x2=%.1f y2=%.1f ", x1, y1, x2, y2 ); }
	 
};



class JagSortPoint3D
{
  public:
     double x1, y1, z1;
     double x2, y2, z2;
	 unsigned char   end;
	 unsigned char   color;
	 bool operator < ( const JagSortPoint3D &o) const;
	 bool operator <= ( const JagSortPoint3D &o) const;
	 bool operator > ( const JagSortPoint3D &o) const;
	 bool operator >= ( const JagSortPoint3D &o) const;
	 void print() const { printf("x1=%.1f y1=%.1f z1=%.1f x2=%.1f y2=%.1f z2=%.1f ", x1, y1, z1, x2, y2, z2 ); }
};

class JagLine2D
{
  public:
  	 JagLine2D() { };
  	 JagLine2D( double ix1, double iy1, double ix2, double iy2 )
	 { x1 = ix1; y1 = iy1; x2 = ix2; y2 = iy2; }
	 void transform( double x0, double y0, double nx0 );
	 void center(double &cx, double &cy ) const { cx=(x1+x2)/2.0; cy=(y1+y2)/2.0; }
     double x1;
	 double y1;
     double x2;
	 double y2;
	 JagVector<Jstr> metrics;
};

class JagLine3D
{
  public:
  	 JagLine3D() { };
  	 JagLine3D( double ix1, double iy1, double iz1, double ix2, double iy2, double iz2 )
	 { x1 = ix1; y1 = iy1; z1 = iz1; x2 = ix2; y2 = iy2; z2 = iz2; }
	 void transform( double x0, double y0, double z0, double nx0, double ny0 );
	 void center( double &cx, double &cy, double &cz ) const { cx=(x1+x2)/2.0; cy=(y1+y2)/2.0; cz=(z1+z2)/2.0; }
     double x1;
	 double y1;
	 double z1;
     double x2;
	 double y2;
	 double z2;
	 JagVector<Jstr> metrics;

};

class JagLineSeg2D
{
  public:
    JagLineSeg2D() {}
    JagLineSeg2D( double f1, double f2, double f3, double f4 ) { 
		x1=f1; y1=f2; x2=f3; y2=f4; 
	}
	bool operator > ( const JagLineSeg2D &o) const;
	bool operator < ( const JagLineSeg2D &o) const;
	bool operator >= ( const JagLineSeg2D &o) const;
	bool operator <= ( const JagLineSeg2D &o) const;
	bool operator == ( const JagLineSeg2D &o) const;
	bool operator != ( const JagLineSeg2D &o) const;
	jagint hashCode() const ;
    double x1;
	double y1;
    double x2;
	double y2;
	unsigned char color;
	jagint size() const { return 32; }
	static JagLineSeg2D NULLVALUE;
	void println() const { printf("JagLineSeg2D: x1=%.3f y1=%.3f x2=%.3f y2=%.3f\n", x1, y1, x2, y2 ); }
	void print() const { printf("JagLineSeg2D: x1=%.3f y1=%.3f x2=%.3f y2=%.3f ", x1, y1, x2, y2 ); }
	bool  isNull() const;
};

class JagLineSeg3D
{
  public:
    JagLineSeg3D() {}
    JagLineSeg3D( double f1, double f2, double f3, double f4, double f5, double f6 ) { 
		x1=f1; y1=f2; z1=f3; x2=f4; y2=f5; z2=f6; }
	bool operator > ( const JagLineSeg3D &o) const;
	bool operator < ( const JagLineSeg3D &o) const;
	bool operator >= ( const JagLineSeg3D &o) const;
	bool operator <= ( const JagLineSeg3D &o) const;
	bool operator == ( const JagLineSeg3D &o) const;
	bool operator != ( const JagLineSeg3D &o) const;
	jagint hashCode() const ;
    double x1;
	double y1;
	double z1;
    double x2;
	double y2;
	double z2;
	unsigned char color;
	jagint size() const { return 64; }
	static JagLineSeg3D NULLVALUE;
	void println() const { printf("JagLineSeg3D: x1=%.1f y1=%.1f z1=%.1f x2=%.1f y2=%.1f z2=%.1f\n", x1, y1, z1, x2, y2, z2 ); }
	void print() const { printf("JagLineSeg3D: x1=%.1f y1=%.1f z1=%.1f x2=%.1f y2=%.1f z2=%.1f ", x1, y1, z1, x2, y2, z2 ); }
	bool  isNull() const;
};



class JagLineSeg2DPair
{
    public:
		JagLineSeg2DPair() {}
        JagLineSeg2DPair( const JagLineSeg2D &k ) : key(k) {}
        JagLineSeg2D  key;
        bool value;
		unsigned char color;
		static  JagLineSeg2DPair  NULLVALUE;

		// operators
        inline int operator< ( const JagLineSeg2DPair &d2 ) const {
    		return (key < d2.key);
		}
        inline int operator<= ( const JagLineSeg2DPair &d2 ) const {
    		return (key <= d2.key );
		}
		
        inline int operator> ( const JagLineSeg2DPair &d2 ) const {
    		return (key > d2.key );
		}

        inline int operator >= ( const JagLineSeg2DPair &d2 ) const
        {
    		return (key >= d2.key );
        }
        inline int operator== ( const JagLineSeg2DPair &d2 ) const
        {
    		return ( key == d2.key );
        }
        inline int operator!= ( const JagLineSeg2DPair &d2 ) const
        {
    		return ( key != d2.key );
        }

		inline void valueDestroy( AbaxDestroyAction action ) { }

		inline jagint hashCode() const {
			return key.hashCode();
		}

		inline jagint size() { return 32; }
		void print() const { key.print(); }
		void println() const { key.println(); }
};

class JagLineSeg3DPair
{
    public:
		JagLineSeg3DPair() {}
        JagLineSeg3DPair( const JagLineSeg3D &k ) : key(k) {}
        JagLineSeg3D  key;
        bool value;
		unsigned char color;
		static  JagLineSeg3DPair  NULLVALUE;

		// operators
        inline int operator< ( const JagLineSeg3DPair &d2 ) const {
    		return (key < d2.key);
		}
        inline int operator<= ( const JagLineSeg3DPair &d2 ) const {
    		return (key <= d2.key );
		}
		
        inline int operator> ( const JagLineSeg3DPair &d2 ) const {
    		return (key > d2.key );
		}

        inline int operator >= ( const JagLineSeg3DPair &d2 ) const
        {
    		return (key >= d2.key );
        }
        inline int operator== ( const JagLineSeg3DPair &d2 ) const
        {
    		return ( key == d2.key );
        }
        inline int operator!= ( const JagLineSeg3DPair &d2 ) const
        {
    		return ( key != d2.key );
        }

		inline void valueDestroy( AbaxDestroyAction action ) { }
		inline jagint hashCode() const {
			return key.hashCode();
		}

		inline jagint size() { return 64; }
		void print() const { key.print(); }
		void println() const { key.println(); }
};


/***
class JagRectangle3D
{
  public:
    JagRectangle3D() { nx=ny=0.0f; }
  	double x, y, z, w, h, nx, ny;
	void transform( double x0, double y0, double z0, double nx0, double ny0 );
};

class JagTriangle3D
{
  public:
  	double x1, y1, z1, x2,y2,z2, x3,y3,z3;
	void transform( double x0, double y0, double z0, double nx0, double ny0 );
};
***/


class JagPoint
{
	public:
		JagPoint();
		JagPoint( const char *x, const char *y );
		JagPoint( const char *x, const char *y, const char *z );
		JagPoint& operator=( const JagPoint& p2 );
		bool equal2D(const JagPoint &JagPoint ) const;
		bool equal3D(const JagPoint &JagPoint ) const;
		JagPoint( const JagPoint& p2 );
		void init();
		void copyData( const JagPoint& p2 );
		void print() const;

		char x[JAG_POINT_LEN]; // or longitue
		char y[JAG_POINT_LEN]; // or latitude
		char z[JAG_POINT_LEN]; // or azimuth, altitude

		char a[JAG_POINT_LEN];  // width
		char b[JAG_POINT_LEN];  // depth in a box
		char c[JAG_POINT_LEN];  // height

		char nx[JAG_POINT_LEN];  // normalized basis vector in x
		char ny[JAG_POINT_LEN];  // normalized basis vector in y
		JagVector<Jstr> metrics;

};

class JagLineString3D
{
    public:
		JagLineString3D() {};
		JagLineString3D& operator=( const JagLineString3D& L2 ) { point = L2.point; return *this; }
		JagLineString3D( const JagLineString3D& L2 ) { point = L2.point; }
		void init() { point.clean(); };
		void clear() { point.clean(); };
		jagint size() const { return point.size(); }
		void add( const JagPoint2D &p );
		void add( const JagPoint3D &p );
		void add( double x, double y, double z=0.0 );
		void print() const { point.print(); }
		void center2D( double &cx, double &cy, bool dropLast=false ) const;
		void center3D( double &cx, double &cy, double &cz, bool dropLast=false ) const;
		void bbox2D( double &xmin, double &ymin, double &xmax, double &ymax ) const;
		void bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
		void minmax3D( int op, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
		const JagPoint3D& operator[](int i ) const { return point[i]; }
		double lineLength( bool removeLast, bool is3D, int srid );
		void reverse();
		void scale( double fx, double fy, double fz, bool is3D);
		void translate( double dx, double dy, double dz, bool is3D);
		void transscale( double dx, double dy, double dz, double fx, double fy, double fz, bool is3D);
		void scaleat(double x0, double y0, double z0, double fx, double fy, double fz, bool is3D);
		void rotateat( double alpha, double x0, double y0 );
		void rotateself( double alpha );
		void affine2d( double a, double b, double d, double e, double dx,double dy );
		void affine3d( double a, double b, double c, double d, double e, double f, double g, double h, double i, 
						double dx, double dy, double dz );

		void toJAG( const Jstr &colType, bool is3D, bool hasHdr, const Jstr &inbbox, int srid, Jstr &str ) const;

		bool  interpolatePoint( short dim, int srid, double fraction, JagPoint3D &point );
		bool  getBetweenPointsFromLen( short dim, double len, int srid, JagPoint3D &p1, JagPoint3D &p2, 
										double &segdist, double &segFraction );
		bool  substring( short dim, int srid, double startFrac, double endFrac, Jstr &lstr );
		double pointOnLeftRatio(double px, double py) const;
		double pointOnRightRatio(double px, double py) const;
		bool pointOnLeft( double px, double py ) const;
		bool pointOnRight( double px, double py ) const;
		JagLineString3D& appendFrom( const JagLineString3D& L2, bool removeLast = false );
		JagVector<JagPoint3D> point;
};

class JagVectorString
{
    public:
		JagVectorString() {};
		JagVectorString& operator=( const JagVectorString& L2 ) { point = L2.point; return *this; }
		JagVectorString( const JagVectorString& L2 ) { point = L2.point; }
		void init() { point.clean(); };
		void clear() { point.clean(); };
		jagint size() const { return point.size(); }
		void add( double x );
		void add( const JagPoint2D &p );
		void print() const { point.print(); }

        /***
		void center2D( double &cx, double &cy, bool dropLast=false ) const;
		void center3D( double &cx, double &cy, double &cz, bool dropLast=false ) const;
		void bbox2D( double &xmin, double &ymin, double &xmax, double &ymax ) const;
		void minmax2D( int op, double &xmin, double &ymin, double &xmax, double &ymax ) const;
		void bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
		void minmax3D( int op, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
        ***/
		void minmax( int op, double &xmin, double &xmax ) const;

		const JagPoint &operator[](int i ) const { return point[i]; }
		//double lineLength( bool removeLast, bool is3D, int srid );
		//void reverse();
		void scale( double fx );
		void translate( double dx );
		void transscale( double dx, double fx );
		// void scaleat(double x0, double y0, double z0, double fx, double fy, double fz, bool is3D);
		void toJAG( const Jstr &colType, bool hasHdr, Jstr &str ) const;

		// JagVector<double> point;
		JagVector<JagPoint> point;
};

class JagLineString
{
    public:
		JagLineString() {};
		JagLineString& operator=( const JagLineString& L2 ) { point = L2.point; return *this; }
		JagLineString& operator=( const JagLineString3D& L2 );
		JagLineString& copyFrom( const JagLineString3D& L2, bool removeLast = false );
		JagLineString& appendFrom( const JagLineString3D& L2, bool removeLast = false );
		JagLineString( const JagLineString& L2 ) { point = L2.point; }
		void init() { point.clean(); };
		void clear() { point.clean(); };
		jagint size() const { return point.size(); }
		void add( const JagPoint2D &p );
		void add( const JagPoint3D &p );
		void add( double x, double y );
		void add( double x, double y, double z );
		void print() const { point.print(); }
		void center2D( double &cx, double &cy, bool dropLast=false ) const;
		void center3D( double &cx, double &cy, double &cz, bool dropLast=false ) const;
		void bbox2D( double &xmin, double &ymin, double &xmax, double &ymax ) const;
		void minmax2D( int op, double &xmin, double &ymin, double &xmax, double &ymax ) const;
		void bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
		void minmax3D( int op, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
		const JagPoint& operator[](int i ) const { return point[i]; }
		double lineLength( bool removeLast, bool is3D, int srid );
		void reverse();
		void scale( double fx, double fy, double fz, bool is3D);
		void translate( double dx, double dy, double dz, bool is3D);
		void transscale( double dx, double dy, double dz, double fx, double fy, double fz, bool is3D);
		void scaleat(double x0, double y0, double z0, double fx, double fy, double fz, bool is3D);
		void toJAG( const Jstr &colType, bool is3D, bool hasHdr, const Jstr &inbbox, int srid, Jstr &str ) const;

		JagVector<JagPoint> point;
};

class JagPolygon
{
	public:
		JagPolygon() {};
		JagPolygon( const JagPolygon &p2 ) { linestr = p2.linestr; }
		JagPolygon& operator=( const JagPolygon &p2 ) { linestr = p2.linestr; return *this; }
		JagPolygon( const JagSquare2D &sq, bool isClosed=true );
		JagPolygon( const JagSquare3D &sq, bool isClosed=true );
		JagPolygon( const JagRectangle2D &rect, bool isClosed=true );
		JagPolygon( const JagRectangle3D &rect, bool isClosed=true );
		JagPolygon( const JagCircle2D &cir, int samples = 100, bool isClosed=true );
		JagPolygon( const JagCircle3D &cir, int samples = 100, bool isClosed=true );
		JagPolygon( const JagEllipse2D &e, int samples = 100, bool isClosed=true );
		JagPolygon( const JagEllipse3D &e, int samples = 100, bool isClosed=true );
		JagPolygon( const JagTriangle2D &t, bool isClosed=true );
		JagPolygon( const JagTriangle3D &t, bool isClosed=true );
		void init() { linestr.clean(); }
		jagint size() const { return linestr.size(); }
		void add( const JagLineString3D &linestr3d ) { linestr.append(linestr3d); }
		void center2D( double &cx, double &cy ) const;
		void center3D( double &cx, double &cy, double &cz ) const;
		bool bbox2D( double &xmin, double &ymin, double &xmax, double &ymax ) const;
		bool bbox3D( double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
		bool minmax2D( int op, double &xmin, double &ymin, double &xmax, double &ymax ) const;
		bool minmax3D( int op, double &xmin, double &ymin, double &zmin, double &xmax, double &ymax, double &zmax ) const;
		void print() const { linestr.print(); }
		double lineLength( bool removeLast, bool is3D, int srid );
		void toWKT( bool is3D, bool hasHdr, const Jstr &objname, Jstr &str ) const;
		void toOneWKT( bool is3D, bool hasHdr, const Jstr &objname, Jstr &str ) const;
		void toJAG( bool is3D, bool hasHdr,  const Jstr &inbbox, int srid, Jstr &str ) const;
		void reverse() { for (int i=0; i < linestr.size(); ++i ) linestr[i].reverse(); }
		void   scale( double fx, double fy, double fz, bool is3D) { 
			for (int i=0; i < linestr.size(); ++i ) linestr[i].scale(fx,fy,fz,is3D); 
		}
		void scaleat(double x0, double y0, double z0, double fx, double fy, double fz, bool is3D) {
			for (int i=0; i < linestr.size(); ++i ) linestr[i].scaleat(x0,y0,z0, fx,fy,fz,is3D);
		}
		void   translate( double dx, double dy, double dz, bool is3D) { 
			for (int i=0; i < linestr.size(); ++i ) linestr[i].translate(dx,dy,dz,is3D); 
		}
		void transscale( double dx, double dy, double dz, double fx, double fy, double fz, bool is3D) {
			for (int i=0; i < linestr.size(); ++i ) linestr[i].transscale(dx,dy,dz,fx,fy,fz,is3D); 
		}
		void rotateat( double alpha, double x0, double y0 ) {
			for (int i=0; i < linestr.size(); ++i ) linestr[i].rotateat( alpha, x0, y0 ); 
		}
		void rotateself( double alpha ) {
			for (int i=0; i < linestr.size(); ++i ) linestr[i].rotateself( alpha ); 
		}
		void affine2d( double a, double b, double d, double e, double dx,double dy ) {
			for (int i=0; i < linestr.size(); ++i ) linestr[i].affine2d( a, b, d, e, dx, dy ); 
		}
		void affine3d( double a, double b, double c, double d, double e, double f, double g, double h, double i, 
						double dx, double dy, double dz ) {
			for (int i=0; i < linestr.size(); ++i ) linestr[i].affine3d( a, b, c, d, e, f, g, h, i, dx, dy, dz ); 
		}
		void toVector2D( int srid, JagVector<JagPoint2D> &vec, bool outerRingOnly );
		void toVector3D( int srid, JagVector<JagPoint3D> &vec, bool outerRingOnly );
		double pointOnLeftRatio(double px, double py) const;
		double pointOnRightRatio(double px, double py) const;
		bool pointOnLeft( double px, double py ) const;
		bool pointOnRight( double px, double py ) const;
		void add( const JagPoint3D &p ) { linestr[0].point.append( p ); }
		void add( double x, double y, double z=0.0) { linestr[0].point.append( JagPoint3D(x,y,z) ); }
		void knn( int dim, int srid, double x, double y, double z, int K, double min, double max, Jstr &value );
		size_t numPoints() const {
			size_t cnt = 0;
			for (int i=0; i < linestr.size(); ++i ) cnt += linestr[i].size();
			return cnt;
		}


		JagVector<JagLineString3D> linestr;
		
};  // end of JagPolygon


class jagvector3 
{
  public:
  double _x, _y, _z;
  jagvector3(double x, double y, double z = 1):_x(x), _y(y),_z(z){}
  jagvector3 cross(const jagvector3& b) const 
  {
    return jagvector3(_y * b._z - _z * b._y,
                   _z * b._x - _x * b._z,
                   _x * b._y - _y * b._x);
  }
  void norm() { _x /= _z; _y /= _z; _z = 1; }
};


#endif
