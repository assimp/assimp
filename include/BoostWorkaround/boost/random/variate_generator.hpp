

#ifndef BOOST_VG_INCLUDED
#define BOOST_VG_INCLUDED

namespace boost
{

template <typename Random, typename Distribution>
class variate_generator
{
public:

	variate_generator (Random _rnd, Distribution _dist)
		: rnd	(_rnd)
		, dist	(_dist)
	{}

	typename Distribution::type operator () ()
	{
		return dist ( rnd () );
	}

private:

	Random rnd;
	Distribution dist;
};
} // end namespace boost

#endif
