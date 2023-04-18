#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <cassert>
#include <unordered_map>
#include <cstdint>
#include <ACConcaveHull.h>

#define _OPENMP 1

#ifndef DBL_EPSILON
#define DBL_EPSILON 0.000000000000001
#endif

#pragma warning(push, 0)
#include <flann/flann.hpp>
#pragma warning(pop)

#define USE_OPENMP

#if defined USE_OPENMP
#if !defined _OPENMP
#pragma message("You've chosen OpenMP but have not give a compilation option.")
#endif
#endif

using std::uint64_t;
static const size_t stride = 24; // size in bytes of x, y, id

struct ACPointValue
{
    ACPoint point;
    double distance = 0.0;
    double angle = 0.0;
};

using ACPointValueVector = std::vector<ACPointValue>;
using ACLineSegment = std::pair<ACPoint, ACPoint>;

// Floating point comparisons
auto ACEqual(double a, double b) -> bool;
auto ACZero(double a) -> bool;
auto ACLessThan(double a, double b) -> bool;
auto ACLessThanOrACEqual(double a, double b) -> bool;
auto ACGreaterThan(double a, double b) -> bool;

// I/O
auto ACUsage() -> void;
auto ACFindArgument(int argc, char **argv, const std::string &name) -> int;
auto ACParseArgument(int argc, char **argv, const std::string &name, std::string &val) -> int;
auto ACParseArgument(int argc, char **argv, const std::string &name, int &val) -> int;
auto ACHasExtension(const std::string &filename, const std::string &ext) -> bool;
auto ACReadFile(const std::string &filename, int fieldX = 1, int fieldY = 2) -> ACPointVector;
auto Print(const std::string &filename, const ACPointVector &points) -> void;
auto Print(FILE *out, const ACPointVector &points, const char *format = "%.3f  %.3f\n") -> void;
auto ACSplit(const std::string &value, const char *delims) -> std::vector<std::string>;

// Algorithm-specific
auto ACNearestNeighboursFlann(flann::Index<flann::L2<double>> &index, const ACPoint &p, size_t k) -> ACPointValueVector;
auto ACConcaveHull(ACPointVector &dataset, size_t k, ACPointVector &hull) -> bool;
auto ACSortByACAngle(ACPointValueVector &values, const ACPoint &p, double prevACAngle, ACPointVector &vec) -> void;
auto AddACPoint(ACPointVector &points, const ACPoint &p) -> void;

// General maths
auto ACPointsACEqual(const ACPoint &a, const ACPoint &b) -> bool;
auto ACAngle(const ACPoint &a, const ACPoint &b) -> double;
auto NormaliseACAngle(double radians) -> double;
auto ACPointInPolygon(const ACPoint &p, const ACPointVector &list) -> bool;
auto ACIntersects(const ACLineSegment &a, const ACLineSegment &b) -> bool;

// ACPoint list utilities
auto FindMinYACPoint(const ACPointVector &points) -> ACPoint;
auto ACRemoveHull(ACPointVector &points, const ACPointVector &hull) -> ACPointVector::iterator;
auto MultipleACPointInPolygon(ACPointVector::iterator begin, ACPointVector::iterator end, const ACPointVector &hull) -> bool;
auto ACRemoveDuplicates(ACPointVector &points) -> void;
auto IdentifyACPoints(ACPointVector &points) -> void;

// Unit tests
auto TestACAngle() -> void;
auto TestACIntersects() -> void;
auto TestACSplit() -> void;



#if 0
int main(int argc, char *argv[])
{
	//TestACAngle();
	//TestACIntersects();
	//TestACSplit();

	std::cout << "Concave hull: A k-nearest neighbours approach.\n";

	// input filename is the only requirement
	if (argc == 1) { ACUsage(); return EXIT_FAILURE; }

	std::string filename(argv[1]);

	// The input field numbers for x and y coordinates
	int fieldX = 1;
	int fieldY = 2;
	if (ACFindArgument(argc, argv, "-field_for_x") != -1) ACParseArgument(argc, argv, "-field_for_x", fieldX);
	if (ACFindArgument(argc, argv, "-field_for_y") != -1) ACParseArgument(argc, argv, "-field_for_y", fieldY);

	// Read input
	ACPointVector points = ACReadFile(filename, fieldX, fieldY);
	size_t uncleanCount = points.size();

	// Remove duplicates and id the points
	ACRemoveDuplicates(points);
	size_t cleanCount = points.size();
	IdentifyACPoints(points);

	// Starting k-value
	int k = 0;
	if (ACFindArgument(argc, argv, "-k") != -1) ACParseArgument(argc, argv, "-k", k);
	k = std::min(std::max(k, 3), (int)points.size() - 1);

	// For debug purposes, optionally disable iterating k.
	bool iterate = true;
	if (ACFindArgument(argc, argv, "-no_iterate") != -1) iterate = false;

	std::cout << "Filename         : " << filename << "\n";
	std::cout << "Input points     : " << uncleanCount << "\n";
	std::cout << "Input (cleaned)  : " << cleanCount << "\n";
	std::cout << "Initial 'k'      : " << k << "\n";
	std::cout << "Final 'k'        : " << k;

	auto startTime = std::chrono::high_resolution_clock::now();

	ACPointVector hull = ACConcaveHull(points, (size_t)k, iterate);

	auto endTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

	std::cout << "\n";
	std::cout << "Output points    : " << hull.size() << "\n";
	std::cout << "Time (excl. i/o) : " << std::fixed << std::setprecision(1) << (double)duration / 1000.0 << "s\n";
	std::cout << "\n";

	// Optional no further output
	if (ACFindArgument(argc, argv, "-no_out") != -1) {
		if (ACFindArgument(argc, argv, "-out") != -1)
			std::cout << "Output to file overridden by switch -no_out.\n";
		return EXIT_SUCCESS;
	}

	// Output to file or stdout
	if (ACFindArgument(argc, argv, "-out") != -1) {
		std::string output;
		ACParseArgument(argc, argv, "-out", output);

		Print(output, hull);
		std::cout << output << " written.\n";
	} else {
		// Nothing specified, so output to console
		Print(stdout, hull);
	}

	return EXIT_SUCCESS;
}
#endif


// Print program usage info.
auto ACUsage() -> void
{
	std::cout << "ACUsage: concave.exe filename [-out arg] [-k arg] [-field_for_x arg] [-field_for_y arg] [-no_out] [-no_iterate]\n";
	std::cout << "\n";
	std::cout << " filename      (required) : file of input coordinates, one row per point.\n";
	std::cout << " -out          (optional) : output file for the hull polygon coordinates. Default=stdout.\n";
	std::cout << " -k            (optional) : start iteration K value. Default=3.\n";
	std::cout << " -field_for_x  (optional) : 1-based column number of input for x-coordinate. Default=1.\n";
	std::cout << " -field_for_y  (optional) : 1-based column number of input for y-coordinate. Default=2.\n";
	std::cout << " -no_out       (optional) : disable output of the hull polygon coordinates.\n";
	std::cout << " -no_iterate   (optional) : stop after only one iteration of K, irrespective of result.\n";
}

// Get command line index of name
auto ACFindArgument(int argc, char **argv, const std::string &name) -> int
{
	for (int i = 1; i < argc; ++i) {
		if (std::string(argv[i]) == name) return i;
	}
	return -1;
}

// Get the command line value (string) for name
auto ACParseArgument(int argc, char **argv, const std::string &name, std::string &val) -> int
{
	int index = ACFindArgument(argc, argv, name) + 1;
	if (index > 0 && index < argc) val = argv[index];

	return index - 1;
}

// Get the command line value (int) for name
auto ACParseArgument(int argc, char **argv, const std::string &name, int &val) -> int
{
	int index = ACFindArgument(argc, argv, name) + 1;

	if (index > 0 && index < argc) val = atoi(argv[index]);
	return (index - 1);
}

// Check whether a string ends with a specified suffix.
auto ACHasExtension(const std::string &filename, const std::string &ext) -> bool
{
	if (filename.length() >= ext.length())
		return (0 == filename.compare(filename.length() - ext.length(), ext.length(), ext));
	return false;
}

// Read a file of coordinates into a vector. First two fields of comma/tab/space delimited input are used.
auto ACReadFile(const std::string &filename, int fieldX, int fieldY) -> ACPointVector
{
	fieldX--; // from 1-based index to 0-based
	fieldY--;

	ACPointVector list;
	ACPoint p;
	std::string line;
	std::vector<std::string> tokens;

	std::ifstream fin(filename.c_str());
	if (fin.is_open()) {
		while (fin.good()) {
			getline(fin, line);
			if (!line.empty()) {
				tokens = ACSplit(line, " ,\t");
				if (tokens.size() >= 2) {
					p.x = std::atof(tokens[fieldX].c_str());
					p.y = std::atof(tokens[fieldY].c_str());
					list.push_back(p);
				}
			}
		}
	}

	return list;
}

// Output a point list to a file
auto Print(const std::string &filename, const ACPointVector &points) -> void
{
	std::string format;
	if (ACHasExtension(filename, ".csv")) format = "%.3f,%.3f\n"; else format = "%.3f  %.3f\n";

	FILE *out = fopen(filename.c_str(), "w+");
	if (out) {
		Print(out, points, format.c_str());
		fclose(out);
	}
}

// Output a point list to a stream with a given format string
auto Print(FILE *out, const ACPointVector &points, const char *format) -> void
{
	for (const auto &p : points) { fprintf(out, format, p.x, p.y); }
}

#if 0
// Iteratively call the main algorithm with an increasing k until success
auto ACConcaveHull(ACPointVector &dataset, size_t k, bool iterate) -> ACPointVector
{
	while (k < dataset.size()) {
		ACPointVector hull;
		if (ACConcaveHull(dataset, k, hull) || !iterate) {
			return hull;
		}
		k++;
	}

	return{};
}
#endif

// Iteratively call the main algorithm with an increasing k until success
auto ACConcaveHull(ACPointVector &points, ACPointVector &hull) -> bool
{
    ACRemoveDuplicates(points);
	IdentifyACPoints(points);
	int k = std::min(std::max(k, 3), (int)points.size() - 1);

	while (k < points.size()) {
		if (ACConcaveHull(points, k, hull) ) { return true; }
		k++;
	}

	return false;
}

// The main algorithm from the Moreira-Santos paper.
auto ACConcaveHull(ACPointVector &pointList, size_t k, ACPointVector &hull) -> bool
{
	hull.clear();

	if (pointList.size() < 3) { return true; }
	if (pointList.size() == 3) { hull = pointList; return true; }

	// construct a randomized kd-tree index using 4 kd-trees
	// 2 columns, but stride = 24 bytes in width (x, y, ignoring id)
	flann::Matrix<double> matrix(&(pointList.front().x), pointList.size(), 2, stride);
	flann::Index<flann::L2<double>> flannIndex(matrix, flann::KDTreeIndexParams(4));
	flannIndex.buildIndex();

	//std::cout << "\rFinal 'k'        : " << k;

	// Initialise hull with the min-y point
	ACPoint firstACPoint = FindMinYACPoint(pointList);
	AddACPoint(hull, firstACPoint);

	// Until the hull is of size > 3 we want to ignore the first point from nearest neighbour searches
	ACPoint currentACPoint = firstACPoint;
	flannIndex.removePoint(firstACPoint.id);

	double prevACAngle = 0.0;
	int step = 1;
	bool its;
	size_t i,j;

	// Iterate until we reach the start, or until there's no points left to process
	while ((!ACPointsACEqual(currentACPoint, firstACPoint) || step == 1) && hull.size() != pointList.size()) {
		if ( 4 == step ) {
			// Put back the first point into the dataset and into the flann index
			firstACPoint.id = pointList.size();
			flann::Matrix<double> firstACPointMatrix(&firstACPoint.x, 1, 2, stride);
			flannIndex.addPoints(firstACPointMatrix);
		}

		ACPointValueVector kNearestNeighbours = ACNearestNeighboursFlann(flannIndex, currentACPoint, k);
		ACPointVector cACPoints(kNearestNeighbours.size());
		ACSortByACAngle(kNearestNeighbours, currentACPoint, prevACAngle, cACPoints );
		its = true;
		i = 0;
		while (its && i < cACPoints.size()) {
			size_t lastACPoint = 0;
			if (ACPointsACEqual(cACPoints[i], firstACPoint)) lastACPoint = 1;

			j = 2;
			its = false;
			while (!its && j < hull.size() - lastACPoint) {
				auto line1 = std::make_pair(hull[step - 1], cACPoints[i]);
				auto line2 = std::make_pair(hull[step - j - 1], hull[step - j]);
				its = ACIntersects(line1, line2);
				j++;
			}

			if (its) i++;
		}

		if (its) return false;

		currentACPoint = cACPoints[i];
		AddACPoint(hull, currentACPoint);
		prevACAngle = ACAngle(hull[step], hull[step - 1]);
		flannIndex.removePoint(currentACPoint.id);
		step++;
	}

	// The original points less the points belonging to the hull need to be fully enclosed by the hull in order to return true.
	ACPointVector dataset = pointList;

	auto newEnd = ACRemoveHull(dataset, hull);
	bool allEnclosed = MultipleACPointInPolygon(begin(dataset), newEnd, hull);

	return allEnclosed;
}

// Compare a and b for equality
auto ACEqual(double a, double b) -> bool
{
	return fabs(a - b) <= DBL_EPSILON;
}

// Compare value to zero
auto ACZero(double a) -> bool
{
	return fabs(a) <= DBL_EPSILON;
}

// Compare for a < b
auto ACLessThan(double a, double b) -> bool
{
	return a < (b - DBL_EPSILON);
}

// Compare for a <= b
auto ACLessThanOrACEqual(double a, double b) -> bool
{
	return a <= (b + DBL_EPSILON);
}

// Compare for a > b
auto ACGreaterThan(double a, double b) -> bool
{
	return a > (b + DBL_EPSILON);
}

// Compare whether two points have the same x and y
auto ACPointsACEqual(const ACPoint &a, const ACPoint &b) -> bool
{
	return ACEqual(a.x, b.x) && ACEqual(a.y, b.y);
}

// Remove duplicates in a list of points
auto ACRemoveDuplicates(ACPointVector &points) -> void
{
	sort(begin(points), end(points), [](const ACPoint & a, const ACPoint & b)
		{
		if (ACEqual(a.x, b.x))
			return ACLessThan(a.y, b.y);
		else
			return ACLessThan(a.x, b.x);
		});

	auto newEnd = unique(begin(points), end(points), [](const ACPoint & a, const ACPoint & b)
		{
		return ACPointsACEqual(a, b);
		});

	points.erase(newEnd, end(points));
}

// Uniquely id the points for binary searching
auto IdentifyACPoints(ACPointVector &points) -> void
{
	uint64_t id = 0;
	for (auto itr = begin(points); itr != end(points); ++itr, ++id) { itr->id = id; }
}

// Find the point having the smallest y-value
auto FindMinYACPoint(const ACPointVector &points) -> ACPoint
{
	assert(!points.empty());
	auto itr = min_element(begin(points), end(points), [](const ACPoint & a, const ACPoint & b)
		{
		if (ACEqual(a.y, b.y))
			return ACGreaterThan(a.x, b.x);
		else
			return ACLessThan(a.y, b.y);
		});

	return *itr;
}

// Lookup by ID and remove a point from a list of points
auto RemoveACPoint(ACPointVector &list, const ACPoint &p) -> void
{
	auto itr = std::lower_bound(begin(list), end(list), p, [](const ACPoint & a, const ACPoint & b)
		{
		return a.id < b.id;
		});

	assert(itr != end(list) && itr->id == p.id);
	if (itr != end(list)) list.erase(itr);
}

// Add a point to a list of points
auto AddACPoint(ACPointVector &points, const ACPoint &p) -> void
{
	points.push_back(p);
}

// Return the k-nearest points in a list of points from the given point p (uses Flann library).
auto ACNearestNeighboursFlann(flann::Index<flann::L2<double>> &index, const ACPoint &p, size_t k) -> ACPointValueVector
{
	std::vector<int> vIndices(k);
	std::vector<double> vDists(k);
	double test[] = { p.x, p.y };

	flann::Matrix<double> query(test, 1, 2);
	flann::Matrix<int> mIndices(vIndices.data(), 1, static_cast<int>(vIndices.size()));
	flann::Matrix<double> mDists(vDists.data(), 1, static_cast<int>(vDists.size()));

	int count_ = index.knnSearch(query, mIndices, mDists, k, flann::SearchParams(128));
	size_t count = static_cast<size_t>(count_);

	ACPointValueVector result(count);

	for (size_t i = 0; i < count; ++i) {
		int id = vIndices[i];
		const double *point = index.getPoint(id);
		result[i].point.x = point[0];
		result[i].point.y = point[1];
		result[i].point.id = id;
		result[i].distance = vDists[i];
	}

	return result;
}

// Returns a list of points sorted in descending order of clockwise angle
auto ACSortByACAngle(ACPointValueVector &values, const ACPoint &from, double prevACAngle, ACPointVector &angled ) -> void
{
	for_each(begin(values), end(values), [from, prevACAngle](ACPointValue & to)
		{
		to.angle = NormaliseACAngle(ACAngle(from, to.point) - prevACAngle);
		});

	sort(begin(values), end(values), [](const ACPointValue & a, const ACPointValue & b)
		{
		return ACGreaterThan(a.angle, b.angle);
		});

	//ACPointVector angled(values.size());

	transform(begin(values), end(values), begin(angled), [](const ACPointValue & pv)
		{
		return pv.point;
		});
}

// Get the angle in radians measured clockwise from +'ve x-axis
auto ACAngle(const ACPoint &a, const ACPoint &b) -> double
{
	double angle = -atan2(b.y - a.y, b.x - a.x);

	return NormaliseACAngle(angle);
}

// Return angle in range: 0 <= angle < 2PI
auto NormaliseACAngle(double radians) -> double
{
	if (radians < 0.0)
		return radians + M_PI + M_PI;
	else
		return radians;
}

// Return the new logical end after removing points from dataset having ids belonging to hull
auto ACRemoveHull(ACPointVector &points, const ACPointVector &hull) -> ACPointVector::iterator
{
	std::vector<uint64_t> ids(hull.size());

	transform(begin(hull), end(hull), begin(ids), [](const ACPoint & p)
		{
		return p.id;
		});

	sort(begin(ids), end(ids));

	return remove_if(begin(points), end(points), [&ids](const ACPoint & p)
		{
		return binary_search(begin(ids), end(ids), p.id);
		});
}

// Uses OpenMP to determine whether a condition exists in the specified range of elements. https://msdn.microsoft.com/en-us/library/ff521445.aspx
template <class InIt, class Predicate>
bool omp_parallel_any_of(InIt first, InIt last, const Predicate &pr)
{
	typedef typename std::iterator_traits<InIt>::value_type item_type;

	// A flag that indicates that the condition exists.
	bool found = false;

	#pragma omp parallel for
	for (int i = 0; i < static_cast<int>(last - first); ++i) {
		if (!found) {
			item_type &cur = *(first + i);
			// If the element satisfies the condition, set the flag to cancel the operation.
			if (pr(cur)) { found = true; }
		}
	}

	return found;
}

// Check whether all points in a begin/end range are inside hull.
auto MultipleACPointInPolygon(ACPointVector::iterator begin, ACPointVector::iterator end, const ACPointVector &hull) -> bool
{
	auto test = [&hull](const ACPoint & p)
		{
		return !ACPointInPolygon(p, hull);
		};

	bool anyOutside = true;

#if defined USE_OPENMP

	anyOutside = omp_parallel_any_of(begin, end, test); // multi-threaded

#else

	anyOutside = std::any_of(begin, end, test); // single-threaded

#endif

	return !anyOutside;
}

// ACPoint-in-polygon test
auto ACPointInPolygon(const ACPoint &p, const ACPointVector &list) -> bool
{
	if (list.size() <= 2) return false;

	const double &x = p.x;
	const double &y = p.y;

	int inout = 0;
	auto v0 = list.begin();
	auto v1 = v0 + 1;

	while (v1 != list.end()) {
		if ((ACLessThanOrACEqual(v0->y, y) && ACLessThan(y, v1->y)) || (ACLessThanOrACEqual(v1->y, y) && ACLessThan(y, v0->y))) {
			if (!ACZero(v1->y - v0->y)) {
				double tdbl1 = (y - v0->y) / (v1->y - v0->y);
				double tdbl2 = v1->x - v0->x;
				if (ACLessThan(x, v0->x + (tdbl2 * tdbl1))) inout++;
			}
		}

		v0 = v1;
		v1++;
	}

	if (inout == 0) return false;
	else if (inout % 2 == 0) return false;
	else return true;
}

// Test whether two line segments intersect each other
auto ACIntersects(const ACLineSegment &a, const ACLineSegment &b) -> bool
{
	// https://www.topcoder.com/community/data-science/data-science-tutorials/geometry-concepts-line-intersection-and-its-applications/

	const double &ax1 = a.first.x;
	const double &ay1 = a.first.y;
	const double &ax2 = a.second.x;
	const double &ay2 = a.second.y;
	const double &bx1 = b.first.x;
	const double &by1 = b.first.y;
	const double &bx2 = b.second.x;
	const double &by2 = b.second.y;

	double a1 = ay2 - ay1;
	double b1 = ax1 - ax2;
	double c1 = a1 * ax1 + b1 * ay1;
	double a2 = by2 - by1;
	double b2 = bx1 - bx2;
	double c2 = a2 * bx1 + b2 * by1;
	double det = a1 * b2 - a2 * b1;

	if (ACZero(det)) { return false; }
	else {
		double x = (b2 * c1 - b1 * c2) / det;
		double y = (a1 * c2 - a2 * c1) / det;

		bool on_both = true;
		on_both = on_both && ACLessThanOrACEqual(std::min(ax1, ax2), x) && ACLessThanOrACEqual(x, std::max(ax1, ax2));
		on_both = on_both && ACLessThanOrACEqual(std::min(ay1, ay2), y) && ACLessThanOrACEqual(y, std::max(ay1, ay2));
		on_both = on_both && ACLessThanOrACEqual(std::min(bx1, bx2), x) && ACLessThanOrACEqual(x, std::max(bx1, bx2));
		on_both = on_both && ACLessThanOrACEqual(std::min(by1, by2), y) && ACLessThanOrACEqual(y, std::max(by1, by2));
		return on_both;
	}
}

// Unit test of ACAngle() function
auto TestACAngle() -> void
{
	auto ToDegrees = [](double radians)
		{
		return radians * 180.0 / M_PI;
		};

	auto Test = [&](const ACPoint &p, double expected)
		{
		double actual = ToDegrees(ACAngle({ 0.0, 0.0 }, p));
		assert(ACEqual(actual, expected));
		};

	double value = ToDegrees(atan(3.0 / 4.0));

	Test({  5.0,  0.0 }, 0.0);
	Test({  4.0,  3.0 }, 360.0 - value);
	Test({  3.0,  4.0 }, 270.0 + value);
	Test({  0.0,  5.0 }, 270.0);
	Test({ -3.0,  4.0 }, 270.0 - value);
	Test({ -4.0,  3.0 }, 180.0 + value);
	Test({ -5.0,  0.0 }, 180.0);
	Test({ -4.0, -3.0 }, 180.0 - value);
	Test({ -3.0, -4.0 }, 90.0 + value);
	Test({  0.0, -5.0 }, 90.0);
	Test({  3.0, -4.0 }, 90.0 - value);
	Test({  4.0, -3.0 }, 0.0 + value);
}

// Unit test the ACIntersects() function
auto TestACIntersects() -> void
{
	using std::make_pair;

	std::unordered_map<char, ACPoint> values;
	values['A'] = {  0.0,  0.0 };
	values['B'] = { -1.5,  3.0 };
	values['C'] = {  2.0,  2.0 };
	values['D'] = { -2.0,  1.0 };
	values['E'] = { -2.5,  5.0 };
	values['F'] = { -1.5,  7.0 };
	values['G'] = {  1.0,  9.0 };
	values['H'] = { -4.0,  7.0 };
	values['I'] = {  3.0, 10.0 };
	values['J'] = {  2.0, 11.0 };
	values['K'] = { -1.0, 11.0 };
	values['L'] = { -3.0, 11.0 };
	values['M'] = { -5.0,  9.5 };
	values['N'] = { -6.0,  7.5 };
	values['O'] = { -6.0,  4.0 };
	values['P'] = { -5.0,  2.0 };

	auto Test = [&values](const char a1, const char a2, const char b1, const char b2, bool expected)
		{
		assert(ACIntersects(make_pair(values[a1], values[a2]), make_pair(values[b1], values[b2])) == expected);
		assert(ACIntersects(make_pair(values[a2], values[a1]), make_pair(values[b1], values[b2])) == expected);
		assert(ACIntersects(make_pair(values[a1], values[a2]), make_pair(values[b2], values[b1])) == expected);
		assert(ACIntersects(make_pair(values[a2], values[a1]), make_pair(values[b2], values[b1])) == expected);
		};

	Test('B', 'D', 'A', 'C', false);
	Test('A', 'B', 'C', 'D', true);
	Test('L', 'K', 'H', 'F', false);
	Test('E', 'C', 'F', 'B', true);
	Test('P', 'C', 'E', 'B', false);
	Test('P', 'C', 'A', 'B', true);
	Test('O', 'E', 'C', 'F', false);
	Test('L', 'C', 'M', 'N', false);
	Test('L', 'C', 'N', 'B', false);
	Test('L', 'C', 'M', 'K', true);
	Test('L', 'C', 'G', 'I', false);
	Test('L', 'C', 'I', 'E', true);
	Test('M', 'O', 'N', 'F', true);
}

auto TestACSplit() -> void
{
	std::vector<double> expected = { -123.456, -987.654 };

	auto Test = [&expected](const std::string &input)
		{
		auto actual = ACSplit(input, " ,\t");
		assert(actual.size() >= 2);
		assert(ACEqual(atof(actual[0].c_str()), expected[0]));
		assert(ACEqual(atof(actual[1].c_str()), expected[1]));
		};

	Test("-123.456 -987.654");
	Test("-123.4560 -987.6540");
	Test("-123.45600 -987.65400");
	Test("-123.456 -987.654 ");
	Test("-123.456 -987.654 100.5");
	Test("-123.456 -987.654 hello");
	Test("-123.456 -987.654 hello world");

	Test("-123.456,-987.654");
	Test("-123.4560,-987.6540");
	Test("-123.45600,-987.65400");
	Test("-123.456,-987.654,");
	Test("-123.456,-987.654,100.5");
	Test("-123.456,-987.654,hello");
	Test("-123.456,-987.654,hello,world");

	Test("-123.456\t-987.654");
	Test("-123.4560\t-987.6540");
	Test("-123.45600\t-987.65400");
	Test("-123.456\t-987.654\t");
	Test("-123.456\t-987.654\t100.5");
	Test("-123.456\t-987.654\thello");
	Test("-123.456\t-987.654\thello\tworld");

	Test(" -123.456   -987.654   ");
	Test(" -123.4560  -987.6540  ");
	Test(" -123.45600 -987.65400 ");
	Test(" -123.456   -987.654  ");
	Test(" -123.456   -987.654  100.5");
	Test(" -123.456   -987.654  hello");
	Test(" -123.456   -987.654  hello   world");
}

// String tokenise using any one of delimiters, adjacent spaces are treated as one
auto ACSplit(const std::string &value, const char *delims) -> std::vector<std::string>
{
	std::vector<std::string> ret;

	size_t start = value.find_first_not_of(' ', 0);
	while (start != std::string::npos) {
		size_t pos = value.find_first_of(delims, start);
		if (pos == std::string::npos) {
			ret.push_back(value.substr(start));
			break;
		} else {
			ret.push_back(value.substr(start, pos - start));

			if (value[pos] == ' ' && strchr(delims, ' '))
				start = value.find_first_not_of(' ', pos);
			else start = pos + 1;
		}
	}

	return ret;
}
