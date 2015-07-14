#include "UnitTestPCH.h"

#include "BoostWorkaround/boost/tuple/tuple.hpp"

#define ASSIMP_FORCE_NOBOOST
#include "BoostWorkaround/boost/format.hpp"


using namespace std;
using namespace Assimp;

using boost::format;
using boost::str;

// ------------------------------------------------------------------------------------------------
TEST(NoBoostTest, testFormat)
{
    EXPECT_EQ( "Ahoi!", boost::str( boost::format("Ahoi!") ));
    EXPECT_EQ( "Ahoi! %", boost::str( boost::format("Ahoi! %%") ));
    EXPECT_EQ( "Ahoi! ", boost::str( boost::format("Ahoi! %s") ));
    EXPECT_EQ( "Ahoi! !!", boost::str( boost::format("Ahoi! %s") % "!!" ));
    EXPECT_EQ( "Ahoi! !!", boost::str( boost::format("Ahoi! %s") % "!!" % "!!" ));
    EXPECT_EQ( "abc", boost::str( boost::format("%s%s%s") % "a" % std::string("b") % "c" ));
}

struct another
{
    int dummy;
};

// ------------------------------------------------------------------------------------------------
TEST(NoBoostTest, Tuple) {
    // Implicit conversion
    boost::tuple<unsigned,unsigned,unsigned> first = boost::make_tuple(4,4,4);
    EXPECT_EQ(4U, first.get<0>());
    EXPECT_EQ(4U, first.get<1>());
    EXPECT_EQ(4U, first.get<2>());

    boost::tuple<int,float,double,bool,another> second;
    bool b = second.get<3>();
    EXPECT_FALSE(b);

    // check empty tuple, ignore compile warning
    boost::tuple<> third;

    // FIXME: Explicit conversion not really required yet
    boost::tuple<float,float,float> last =
        (boost::tuple<float,float,float>)boost::make_tuple(1.,2.,3.);
    EXPECT_EQ(1.f, last.get<0>());
    EXPECT_EQ(2.f, last.get<1>());
    EXPECT_EQ(3.f, last.get<2>());

    // Non-const access
    first.get<0>() = 1;
    first.get<1>() = 2;
    first.get<2>() = 3;
    EXPECT_EQ(1U, first.get<0>());
    EXPECT_EQ(2U, first.get<1>());
    EXPECT_EQ(3U, first.get<2>());

    // Const cases
    const boost::tuple<unsigned,unsigned,unsigned> constant = boost::make_tuple(5,5,5);
    first.get<0>() = constant.get<0>();
    EXPECT_EQ(5U, constant.get<0>());
    EXPECT_EQ(5U, first.get<0>());

    // Direct assignment w. explicit conversion
    last = first;
    EXPECT_EQ(5.f, last.get<0>());
    EXPECT_EQ(2.f, last.get<1>());
    EXPECT_EQ(3.f, last.get<2>());
}
