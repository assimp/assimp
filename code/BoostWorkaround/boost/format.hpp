


/* DEPRECATED! - use code/TinyFormatter.h instead.
 *
 *
 * */

#ifndef AI_BOOST_FORMAT_DUMMY_INCLUDED
#define AI_BOOST_FORMAT_DUMMY_INCLUDED

#if (!defined BOOST_FORMAT_HPP) || (defined ASSIMP_FORCE_NOBOOST)

#include <string>
#include <vector>
#include <sstream> 

namespace boost
{


	class format
	{
	public:
		format (const std::string& _d)
			: d(_d)
		{
		}

		template <typename T>
		format& operator % (T in) 
		{
			// XXX add replacement for boost::lexical_cast?
			
			std::ostringstream ss;
			ss << in; // note: ss cannot be an rvalue, or  the global operator << (const char*) is not called for T == const char*.
			chunks.push_back( ss.str());
			return *this;
		}


		operator std::string () const {
			std::string res; // pray for NRVO to kick in

			size_t start = 0, last = 0;

			std::vector<std::string>::const_iterator chunkin = chunks.begin();

			for ( start = d.find('%');start != std::string::npos;  start = d.find('%',last)) {
				res += d.substr(last,start-last);
				last = start+2;
				if (d[start+1] == '%') {
					res += "%";
					continue;
				}

				if (chunkin == chunks.end()) {
					break;
				}

				res += *chunkin++;
			}
			res += d.substr(last);
			return res;
		}

	private:
		std::string d;
		std::vector<std::string> chunks;
	};

	inline std::string str(const std::string& s) {
		return s;
	} 
}


#else
#	error "format.h was already included"
#endif //
#endif // !! AI_BOOST_FORMAT_DUMMY_INCLUDED

