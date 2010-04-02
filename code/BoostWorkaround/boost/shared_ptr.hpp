
#ifndef INCLUDED_AI_BOOST_SHARED_PTR
#define INCLUDED_AI_BOOST_SHARED_PTR

#ifndef BOOST_SCOPED_PTR_HPP_INCLUDED

#include <assert.h>

// ------------------------------
// Internal stub
namespace boost {
	namespace detail {
		class controller {

		public:

			controller()
				: cnt()
			{}

			template <typename T>
			controller(T* ptr)
				: cnt(1)
			{}
		
		public:

			template <typename T>
			controller* decref(T* pt) {
				if (--cnt <= 0) {
					delete this;
					delete pt;
				}
				return NULL;
			}
		
			controller* incref() {
				++cnt;
				return this;
			}

		private:
			long cnt;
		};

		struct empty {};
		
		template <typename DEST, typename SRC>
		struct is_convertible_stub {
			
			struct yes {char s[1];};
			struct no  {char s[2];};

			static yes foo(DEST*);
			static no  foo(...);

			enum {result = (sizeof(foo((SRC*)0)) == sizeof(yes) ? 1 : 0)};	
		};

		template <bool> struct enable_if {};
		template <> struct enable_if<true> {
			typedef empty result;
		};

		template <typename DEST, typename SRC>
		struct is_convertible : public enable_if<is_convertible_stub<DEST,SRC>::result > {
		};
	}

// ------------------------------
// Small replacement for boost::shared_ptr, not threadsafe because no
// atomic reference counter is in use.
// ------------------------------
template <class T>
class shared_ptr
{
	template <typename TT> friend class shared_ptr;
public:

	// provide a default construtctor
	shared_ptr()
		: ptr()
		, ctr(new detail::controller())
	{
	}

	// construction from an existing object of type T
	shared_ptr(T* _ptr)
		: ptr(_ptr)
		, ctr(new detail::controller(ptr))
	{
	}

	shared_ptr(const shared_ptr& o)
		: ptr(o.ptr)
		, ctr(o.ctr->incref())
	{
	}

	template <typename Y>
	shared_ptr(const shared_ptr<Y>& o,typename detail::is_convertible<T,Y>::result = detail::empty())
		: ptr(o.ptr)
		, ctr(o.ctr->incref())
	{
	}

	// automatic destruction of the wrapped object when all
	// references are freed.
	~shared_ptr()	{
		ctr = ctr->decref(ptr);
	}

	// pointer access
	inline operator T*()	{
		return ptr;
	}

	inline T* operator-> ()	{
		return ptr;
	}

	// standard semantics
	inline T* get()	{
		return ptr;
	}

	inline void reset (T* t = 0)	{
		ctr = ctr->decref(ptr);
		ptr = t;
		if(ptr) {
			ctr = new detail::controller(ptr);
		}
	}

	void swap(shared_ptr & b)	{
		std::swap(ptr, b.ptr);
		std::swap(ctr, b.ctr);
	}

private:

	// encapsulated object pointer
	T* ptr;

	// control block
	detail::controller* ctr;
};

template<class T>
inline void swap(shared_ptr<T> & a, shared_ptr<T> & b)
{
	a.swap(b);
}

} // end of namespace boost

#else
#	error "shared_ptr.h was already included"
#endif
#endif // INCLUDED_AI_BOOST_SCOPED_PTR
