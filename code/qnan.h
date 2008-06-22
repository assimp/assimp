

#if (!defined AI_QNAN_H_INCLUDED)
#define AI_QNAN_H_INCLUDED

#if (!defined ASSIMP_BUILD_CPP_09)
#	include <boost/static_assert.hpp>
#endif

inline bool is_qnan(const float in)
{
	// _isnan() takes a double as argument and would
	// require a cast. Therefore we must do it on our own ...
	// Another method would be to check whether in != in.
	// This should also wor since nan compares to inequal, 
	// even when compared with itself. However, this could
	// case problems with other special floats like snan or inf
	union _tagFPUNION
	{
		float f;
		int32_t i;
	} FPUNION1,FPUNION2;

	// use a compile-time asertion if possible
#if (defined ASSIMP_BUILD_CPP_09)
	static_assert(sizeof(float)==sizeof(int32_t),
		"A float seems not to be 4 bytes on this platform");
#else
	BOOST_STATIC_ASSERT(sizeof(float)==sizeof(int32_t));
#endif

	FPUNION1.f = in;
	FPUNION2.f = std::numeric_limits<float>::quiet_NaN();
	return FPUNION1.i == FPUNION2.i;
}

inline bool is_not_qnan(const float in)
{
	return !is_qnan(in);
}

#endif // !! AI_QNAN_H_INCLUDED