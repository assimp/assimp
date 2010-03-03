#include "BoostWorkaround/boost/tuple/tuple.hpp"

struct another
{int dummy;};

boost::tuple<unsigned,unsigned,unsigned> first;
boost::tuple<int,float,double,bool,another> second;
boost::tuple<> third;
boost::tuple<float,float,float> last;

void test () {

	// Implicit conversion
	first = boost::make_tuple(4,4,4);

	// FIXME: Explicit conversion not really required yet
	last  = (boost::tuple<float,float,float>)boost::make_tuple(4.,4.,4.);	

	// Non-const access
	first.get<0>() = 1;
	first.get<1>() = 2;
	first.get<2>() = 3;

	float f = last.get<2>();
	bool  b = second.get<3>();

	// Const cases
	const boost::tuple<unsigned,unsigned,unsigned> constant = boost::make_tuple(4,4,4);
	first.get<0>() = constant.get<0>();

	// Direct assignment w. explicit conversion
	last = first;
}
