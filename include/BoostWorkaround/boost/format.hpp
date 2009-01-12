

#ifndef AI_BOOST_FORMAT_DUMMY_INCLUDED
#define AI_BOOST_FORMAT_DUMMY_INCLUDED

#ifndef BOOST_FORMAT_HPP

#include <string>

namespace boost
{
	class str;

	class format
	{
		friend class str;
	public:
		format (const std::string& _d)
			: d(_d)
		{
		}

		template <typename T>
		const format& operator % (T in) const
		{
			return *this;
		}

	private:
		std::string d;
	};

	class str : public std::string
	{
	public:

		str(const format& f)
		{
			*((std::string* const)this) = std::string( f.d );
		}
	};
}

#else
#	error "format.h was already included"
#endif //
#endif // !! AI_BOOST_FORMAT_DUMMY_INCLUDED

