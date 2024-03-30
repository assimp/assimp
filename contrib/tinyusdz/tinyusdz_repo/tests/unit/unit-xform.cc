#ifdef _MSC_VER
#define NOMINMAX
#endif

#include <iostream>

#define TEST_NO_MAIN
#include "acutest.h"

#include "value-types.hh"
#include "unit-value-types.h"
#include "prim-types.hh"
#include "xform.hh"
#include "unit-common.hh"
#include "value-pprint.hh"


using namespace tinyusdz;
using namespace tinyusdz_test;

void xformOp_test(void) {

  {
    value::double3 scale = {1.0, 2.0, 3.0};

    XformOp op;
    op.op_type = XformOp::OpType::Scale;
    op.inverted = true;
    op.set_value(scale);


    Xformable x;
    x.xformOps.push_back(op);

    value::matrix4d m;
    bool resetXformStack;
    std::string err;
    double t = value::TimeCode::Default();
    value::TimeSampleInterpolationType tinterp = value::TimeSampleInterpolationType::Held;

    bool ret = x.EvaluateXformOps(t, tinterp, &m, &resetXformStack, &err);
    TEST_CHECK(ret == true);

    TEST_CHECK(float_equals(m.m[0][0], 1.0));
    TEST_CHECK(float_equals(m.m[1][1], 1.0/2.0));
    TEST_CHECK(float_equals(m.m[2][2], 1.0/3.0));

  }

  {
    value::matrix4d a;
    a.m[0][0] = 0;
    a.m[0][1] = 0;
    a.m[0][2] = 1;
    a.m[0][3] = 0;
    a.m[1][0] = 0;
    a.m[1][1] = 1;
    a.m[1][2] = 0;
    a.m[1][3] = 0;

    a.m[2][0] = -1;
    a.m[2][1] = 0;
    a.m[2][2] = 0;
    a.m[2][3] = 0;
    a.m[3][0] = 0.44200000166893005;
    a.m[3][1] = -7.5320000648498535;
    a.m[3][2] = 18.611000061035156;
    a.m[3][3] = 1;

    value::matrix4d b = value::matrix4d::identity();
    b.m[3][2] = -30.0;

    value::matrix4d c = a * b;
    std::cout << c << "\n";

    // expected: (0, 0, 1, 0), (0, 1, 0, 0), (-1, 0, 0, 0), (0.442, -7.532, -11.389, 1)
    TEST_CHECK(float_equals(c.m[0][0], 0.0));
    TEST_CHECK(float_equals(c.m[0][1], 0.0));
    TEST_CHECK(float_equals(c.m[0][2], 1.0));
    TEST_CHECK(float_equals(c.m[0][3], 0.0));

    TEST_CHECK(float_equals(c.m[1][0], 0.0));
    TEST_CHECK(float_equals(c.m[1][1], 1.0));
    TEST_CHECK(float_equals(c.m[1][2], 0.0));
    TEST_CHECK(float_equals(c.m[1][3], 0.0));

    TEST_CHECK(float_equals(c.m[2][0], -1.0));
    TEST_CHECK(float_equals(c.m[2][1], 0.0));
    TEST_CHECK(float_equals(c.m[2][2], 0.0));
    TEST_CHECK(float_equals(c.m[2][3], 0.0));

    TEST_CHECK(float_equals(c.m[3][0], 0.442, 0.00001));
    TEST_CHECK(float_equals(c.m[3][1], -7.532, 0.00001));
    TEST_CHECK(float_equals(c.m[3][2], -11.389, 0.00001));
    TEST_CHECK(float_equals(c.m[3][3], 1.0));


  }

  // RotateXYZ 000
  {
    value::double3 rotXYZ = {90.0, 0.0, 0.0};

    XformOp op;
    op.op_type = XformOp::OpType::RotateXYZ;
    op.inverted = false;
    op.set_value(rotXYZ);

    Xformable x;
    x.xformOps.push_back(op);

    value::matrix4d m;
    bool resetXformStack;
    std::string err;
    double t = value::TimeCode::Default();
    value::TimeSampleInterpolationType tinterp = value::TimeSampleInterpolationType::Held;

    bool ret = x.EvaluateXformOps(t, tinterp, &m, &resetXformStack, &err);

    TEST_CHECK(ret);
    
    std::cout << "rotXYZ = " << m << "\n";

    //double eps = std::numeric_limits<double>::epsilon();

    // NOTE: in pxrUSD ret = ( (1, 0, 0, 0), (0, 6.12323e-17, 1, 0), (0, -1, 6.12323e-17, 0), (0, 0, 0, 1) )
    TEST_CHECK(float_equals(m.m[0][0], 1.0));
    TEST_CHECK(float_equals(m.m[0][1], 0.0));
    TEST_CHECK(float_equals(m.m[0][2], 0.0));
    TEST_CHECK(float_equals(m.m[0][3], 0.0));

    TEST_CHECK(float_equals(m.m[1][0], 0.0));
    TEST_CHECK(float_equals(m.m[1][1], 0.0));
    TEST_CHECK(float_equals(m.m[1][2], 1.0));
    TEST_CHECK(float_equals(m.m[1][3], 0.0));

    TEST_CHECK(float_equals(m.m[2][0], 0.0));
    TEST_CHECK(float_equals(m.m[2][1], -1.0));
    TEST_CHECK(float_equals(m.m[2][2], 0.0));
    TEST_CHECK(float_equals(m.m[2][3], 0.0));

    TEST_CHECK(float_equals(m.m[3][0], 0.0));
    TEST_CHECK(float_equals(m.m[3][1], 0.0));
    TEST_CHECK(float_equals(m.m[3][2], 0.0));
    TEST_CHECK(float_equals(m.m[3][3], 1.0));



  }

  // RotateXYZ 001
  {
    value::double3 rotXYZ = {0.0, 0.0, -65.66769};

    XformOp op;
    op.op_type = XformOp::OpType::RotateXYZ;
    op.inverted = false;
    op.set_value(rotXYZ);

    Xformable x;
    x.xformOps.push_back(op);

    value::matrix4d m;
    bool resetXformStack;
    std::string err;
    double t = value::TimeCode::Default();
    value::TimeSampleInterpolationType tinterp = value::TimeSampleInterpolationType::Held;

    bool ret = x.EvaluateXformOps(t, tinterp, &m, &resetXformStack, &err);

    TEST_CHECK(ret);
    
    std::cout << "rotXYZ = " << m << "\n";

    // 0.4120283041870241, -0.9111710468121587, 0, 0
    // 0.9111710468121587, 0.4120283041870241, 0, 0
    // 0, 0, 1, 0
    // 0, 0, 0, 1
    TEST_CHECK(float_equals(m.m[0][0], 0.4120283041870241, 0.00001));
    TEST_CHECK(float_equals(m.m[0][1], -0.9111710468121587, 0.00001));
    TEST_CHECK(float_equals(m.m[0][2], 0.0));
    TEST_CHECK(float_equals(m.m[0][3], 0.0));

    TEST_CHECK(float_equals(m.m[1][0], 0.9111710468121587, 0.00001));
    TEST_CHECK(float_equals(m.m[1][1], 0.4120283041870241, 0.00001));
    TEST_CHECK(float_equals(m.m[1][2], 0.0));
    TEST_CHECK(float_equals(m.m[1][3], 0.0));

    TEST_CHECK(float_equals(m.m[2][0], 0.0));
    TEST_CHECK(float_equals(m.m[2][1], 0.0));
    TEST_CHECK(float_equals(m.m[2][2], 1.0));
    TEST_CHECK(float_equals(m.m[2][3], 0.0));

    TEST_CHECK(float_equals(m.m[3][0], 0.0));
    TEST_CHECK(float_equals(m.m[3][1], 0.0));
    TEST_CHECK(float_equals(m.m[3][2], 0.0));
    TEST_CHECK(float_equals(m.m[3][3], 1.0));
  }

  // RotateXYZ 002
  {
    value::double3 rotXYZ = {10.0, 23.0, 43.2};

    XformOp op;
    op.op_type = XformOp::OpType::RotateXYZ;
    op.inverted = false;
    op.set_value(rotXYZ);

    Xformable x;
    x.xformOps.push_back(op);

    value::matrix4d m;
    bool resetXformStack;
    std::string err;
    double t = value::TimeCode::Default();
    value::TimeSampleInterpolationType tinterp = value::TimeSampleInterpolationType::Held;

    bool ret = x.EvaluateXformOps(t, tinterp, &m, &resetXformStack, &err);

    TEST_CHECK(ret);
    
    std::cout << "rotXYZ = " << m << "\n";

    double eps = std::numeric_limits<double>::epsilon();

    // numeric value is grabbed from pxrUSD.
    // There are slight eps error for [0][1], [1][0] and [1][1], so twice eps
    TEST_CHECK(float_equals(m.m[0][0], 0.6710191595559729, eps));
    TEST_CHECK(float_equals(m.m[0][1], 0.6301289334241799, eps));
    TEST_CHECK(float_equals(m.m[0][2], -0.39073112848927377, eps));
    TEST_CHECK(float_equals(m.m[0][3], 0.0));

    TEST_CHECK(float_equals(m.m[1][0], -0.6246869592440953, eps));
    TEST_CHECK(float_equals(m.m[1][1], 0.7643403049061097, eps));
    TEST_CHECK(float_equals(m.m[1][2], 0.15984399033558103, eps));
    TEST_CHECK(float_equals(m.m[1][3], 0.0));

    TEST_CHECK(float_equals(m.m[2][0], 0.3993738730302244, eps));
    TEST_CHECK(float_equals(m.m[2][1], 0.13682626048292368, eps));
    TEST_CHECK(float_equals(m.m[2][2], 0.9065203163653295, eps));
    TEST_CHECK(float_equals(m.m[2][3], 0.0));

    TEST_CHECK(float_equals(m.m[3][0], 0.0));
    TEST_CHECK(float_equals(m.m[3][1], 0.0));
    TEST_CHECK(float_equals(m.m[3][2], 0.0));
    TEST_CHECK(float_equals(m.m[3][3], 1.0));
  }

  // Rotate 003
  {
    value::double3 rotXYZ = {-10.0, 13.0, 43.2};

    XformOp op;
    op.op_type = XformOp::OpType::RotateXYZ;
    op.inverted = true;
    op.set_value(rotXYZ);

    Xformable x;
    x.xformOps.push_back(op);

    value::matrix4d m;
    bool resetXformStack;
    std::string err;
    double t = value::TimeCode::Default();
    value::TimeSampleInterpolationType tinterp = value::TimeSampleInterpolationType::Held;

    bool ret = x.EvaluateXformOps(t, tinterp, &m, &resetXformStack, &err);

    TEST_CHECK(ret);
    
    std::cout << "rotXYZ = " << m << "\n";

    double eps = std::numeric_limits<double>::epsilon();

  
    TEST_CHECK(float_equals(m.m[0][0], 0.7102852087270047, eps));
    TEST_CHECK(float_equals(m.m[0][1], -0.7026225180689177, eps));
    TEST_CHECK(float_equals(m.m[0][2], 0.0426206448347375, eps));
    TEST_CHECK(float_equals(m.m[0][3], 0.0));

    TEST_CHECK(float_equals(m.m[1][0], 0.6670022079522818, eps));
    TEST_CHECK(float_equals(m.m[1][1], 0.6911539437437854, eps));
    TEST_CHECK(float_equals(m.m[1][2], 0.2782342190209419, eps));
    TEST_CHECK(float_equals(m.m[1][3], 0.0));

    TEST_CHECK(float_equals(m.m[2][0], -0.224951054343865, eps));
    TEST_CHECK(float_equals(m.m[2][1], -0.16919758612316493, eps));
    TEST_CHECK(float_equals(m.m[2][2], 0.9595671941035071, eps));
    TEST_CHECK(float_equals(m.m[2][3], 0.0));

    TEST_CHECK(float_equals(m.m[3][0], 0.0));
    TEST_CHECK(float_equals(m.m[3][1], 0.0));
    TEST_CHECK(float_equals(m.m[3][2], 0.0));
    TEST_CHECK(float_equals(m.m[3][3], 1.0));
  }

  // trans x scale
  // scale firstly applied, then translation.
  {
    value::double3 trans = {1.0, 1.0, 1.0};
    value::double3 scale = {1.5, 0.5, 2.5};

    Xformable x;
    {
      XformOp op;
      op.op_type = XformOp::OpType::Translate;
      op.inverted = false;
      op.set_value(trans);

      x.xformOps.push_back(op);
    }

    {
      XformOp op;
      op.op_type = XformOp::OpType::Scale;
      op.inverted = false;
      op.set_value(scale);

      x.xformOps.push_back(op);
    }

    value::matrix4d m;
    bool resetXformStack;
    std::string err;
    double t = value::TimeCode::Default();
    value::TimeSampleInterpolationType tinterp = value::TimeSampleInterpolationType::Held;

    bool ret = x.EvaluateXformOps(t, tinterp, &m, &resetXformStack, &err);

    TEST_CHECK(ret);
    
    std::cout << "trans x scale = " << m << "\n";

    // 1.5 0 0 0, 0 1.5 0 0, 0 0 1.5 0, 1 0 0 1
    TEST_CHECK(float_equals(m.m[0][0], 1.5));
    TEST_CHECK(float_equals(m.m[0][1], 0.0));
    TEST_CHECK(float_equals(m.m[0][2], 0.0));
    TEST_CHECK(float_equals(m.m[0][3], 0.0));

    TEST_CHECK(float_equals(m.m[1][0], 0.0));
    TEST_CHECK(float_equals(m.m[1][1], 0.5));
    TEST_CHECK(float_equals(m.m[1][2], 0.0));
    TEST_CHECK(float_equals(m.m[1][3], 0.0));

    TEST_CHECK(float_equals(m.m[2][0], 0.0));
    TEST_CHECK(float_equals(m.m[2][1], 0.0));
    TEST_CHECK(float_equals(m.m[2][2], 2.5));
    TEST_CHECK(float_equals(m.m[2][3], 0.0));

    TEST_CHECK(float_equals(m.m[3][0], 1.0));
    TEST_CHECK(float_equals(m.m[3][1], 1.0));
    TEST_CHECK(float_equals(m.m[3][2], 1.0));
    TEST_CHECK(float_equals(m.m[3][3], 1.0));

  }


}
