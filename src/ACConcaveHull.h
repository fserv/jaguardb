#ifndef _ac_concave_hull_
#define _ac_concave_hull_

#include <vector>

struct ACPoint
{
	double x = 0.0;
	double y = 0.0;
	uint64_t id = 0;
	ACPoint() = default;
	ACPoint(double x, double y) : x(x) , y(y) {}
};

using ACPointVector = std::vector<ACPoint>;
auto ACConcaveHull(ACPointVector &points, ACPointVector &hull) -> bool;

#endif

