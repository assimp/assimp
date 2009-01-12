
#ifndef BOOST_MT_INCLUDED
#define BOOST_MT_INCLUDED

namespace boost
{

	// A very minimal implementation. No mersenne_twister at all,
	// but it should generate some randomness though
	class mt19937
	{
	public:

		mt19937(unsigned int seed)
		{
			::srand(seed);
		}

		unsigned int operator () (void)
		{
			return ::rand();
		}
	};
};

#endif
