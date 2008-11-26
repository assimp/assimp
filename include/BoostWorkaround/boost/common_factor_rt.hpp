

#ifndef BOOST_MATH_COMMON_FACTOR_RT_HPP
#define BOOST_MATH_COMMON_FACTOR_RT_HPP


namespace boost	{
namespace math	{

// TODO: use binary GCD for unsigned integers ....
template < typename IntegerType >
IntegerType  gcd( IntegerType const &a, IntegerType const &b )
{
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
IntegerType  lcm( IntegerType const &a, IntegerType const &b )
{
	IntegerType t = gcd (a,b);
	if (!t)return t;
	return a / t * b;
}

}}

#endif