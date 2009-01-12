
#ifndef BOOST_UNIFORM_INT_INCLUDED
#define BOOST_UNIFORM_INT_INCLUDED

namespace boost
{
	template <typename IntType = unsigned int>
	class uniform_int
	{
	public:

		typedef IntType type;

		uniform_int (IntType _first, IntType _last)
			:	first	(_first)
			,	last	(_last)
		{}

		IntType operator () (IntType in) 
		{
			return (IntType)((in * ((last-first)/RAND_MAX)) + first);
		}

	private:

		IntType first, last;
	};
};

#endif // BOOST_UNIFORM_INT_INCLUDED
