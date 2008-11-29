
#ifndef BOOST_MT_INCLUDED
#define BOOST_MT_INCLUDED

#if _MSC_VER >= 1400
#	pragma message( "AssimpBuild: Using CRT's rand() as replacement for mt19937" )
#endif

namespace boost
{

	// A very minimal implementation. No mersenne_twister at all,
	// but it should generate some randomness, though
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