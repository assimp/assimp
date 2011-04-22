/// A quick replacement for boost::lexical_cast for all the Boost haters out there

#ifndef __AI_BOOST_WORKAROUND_LEXICAL_CAST
#define __AI_BOOST_WORKAROUND_LEXICAL_CAST

namespace boost
{

	/// A quick replacement for boost::lexical_cast - should work for all types a stringstream can handle
	template <typename TargetType, typename SourceType>
	TargetType lexical_cast( const SourceType& source)
	{
		std::stringstream stream;
		TargetType result;

		stream << source;
		stream >> result;
		return result;
	}

} // namespace boost

#endif // __AI_BOOST_WORKAROUND_LEXICAL_CAST

