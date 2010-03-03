

#ifndef BOOST_MATH_COMMON_FACTOR_RT_HPP
#define BOOST_MATH_COMMON_FACTOR_RT_HPP


namespace boost	{
namespace math	{

// TODO: use binary GCD for unsigned integers ....
template < typename IntegerType >
IntegerType  gcd( IntegerType a, IntegerType b )
{
	const IntegerType zero = (IntegerType)0;
	while ( true )
	{
		if ( a == zero )
			return b;
		b %= a;

		if ( b == zero )
			return a;
		a %= b;
	}
}

template < typename IntegerType >
IntegerType  lcm( IntegerType a, IntegerType b )
{
	const IntegerType t = gcd (a,b);
	if (!t)return t;
	return a / t * b;
}

}}

#endif
