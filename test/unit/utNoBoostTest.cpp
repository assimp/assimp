/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/
#include "UnitTestPCH.h"

#include "BoostWorkaround/boost/tuple/tuple.hpp"

#define ASSIMP_FORCE_NOBOOST
#include "BoostWorkaround/boost/format.hpp"
#include <assimp/TinyFormatter.h>


using namespace std;
using namespace Assimp;
using namespace Assimp::Formatter;

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

    boost::tuple<int, float, double, bool, another> second=
    		boost::make_tuple(1,1.0f,0.0,false,another());
    bool b = second.get<3>();

    // check empty tuple
    boost::tuple<> third;
    third;

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
