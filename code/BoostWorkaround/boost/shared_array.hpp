
#ifndef INCLUDED_AI_BOOST_SHARED_ARRAY
#define INCLUDED_AI_BOOST_SHARED_ARRAY

#ifndef BOOST_SHARED_ARRAY_HPP_INCLUDED

// ------------------------------
// Internal stub
namespace boost {
	namespace array_detail {
		class controller {
		public:

			controller()
				: cnt(1)
			{}
		
		public:

			template <typename T>
			controller* decref(T* pt) {
				if (--cnt <= 0) {
					delete this;
					delete[] pt;
				}
				return NULL;
			}
		
			controller* incref() {
				++cnt;
				return this;
			}

			long get() const {
				return cnt;
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
// Small replacement for boost::shared_array, not threadsafe because no
// atomic reference counter is in use.
// ------------------------------
template <class T>
class shared_array
{
	template <typename TT> friend class shared_array;

	template<class TT> friend bool operator== (const shared_array<TT>& a, const shared_array<TT>& b);
	template<class TT> friend bool operator!= (const shared_array<TT>& a, const shared_array<TT>& b);
	template<class TT> friend bool operator<  (const shared_array<TT>& a, const shared_array<TT>& b);

public:

	typedef T element_type;

public:

	// provide a default constructor
	shared_array()
		: ptr()
		, ctr(NULL)
	{
	}

	// construction from an existing object of type T
	explicit shared_array(T* ptr)
		: ptr(ptr)
		, ctr(ptr ? new array_detail::controller() : NULL)
	{
	}

	shared_array(const shared_array& r)
		: ptr(r.ptr)
		, ctr(r.ctr ? r.ctr->incref() : NULL)
	{
	}

	template <typename Y>
	shared_array(const shared_array<Y>& r,typename detail::is_convertible<T,Y>::result = detail::empty())
		: ptr(r.ptr)
		, ctr(r.ctr ? r.ctr->incref() : NULL)
	{
	}

	// automatic destruction of the wrapped object when all
	// references are freed.
	~shared_array()	{
		if (ctr) {
			ctr = ctr->decref(ptr);
		}
	}

	shared_array& operator=(const shared_array& r) {
		if (this == &r) {
			return *this;
		}
		if (ctr) {
			ctr->decref(ptr);
		}
		ptr = r.ptr;
		ctr = ptr?r.ctr->incref():NULL;
		return *this;
	}

	template <typename Y>
	shared_array& operator=(const shared_array<Y>& r) {
		if (this == &r) {
			return *this;
		}
		if (ctr) {
			ctr->decref(ptr);
		}
		ptr = r.ptr;
		ctr = ptr?r.ctr->incref():NULL;
		return *this;
	}

	// pointer access
	inline operator T*()	{
		return ptr;
	}

	inline T* operator-> () const	{
		return ptr;
	}

	// standard semantics
	inline T* get() {
		return ptr;
	}

	T& operator[] (std::ptrdiff_t index) const {
		return ptr[index];
	}

	inline const T* get() const	{
		return ptr;
	}

	inline operator bool () const {
		return ptr != NULL;
	}

	inline bool unique() const {
		return use_count() == 1;
	}

	inline long use_count() const {
		return ctr->get();
	}

	inline void reset (T* t = 0)	{
		if (ctr) {
			ctr->decref(ptr);
		}
		ptr = t;
		ctr = ptr?new array_detail::controller():NULL;
	}

	void swap(shared_array & b)	{
		std::swap(ptr, b.ptr);
		std::swap(ctr, b.ctr);
	}


private:

	// encapsulated object pointer
	T* ptr;

	// control block
	array_detail::controller* ctr;
};

template<class T>
inline void swap(shared_array<T> & a, shared_array<T> & b)
{
	a.swap(b);
}

template<class T>
bool operator== (const shared_array<T>& a, const shared_array<T>& b) {
	return a.ptr == b.ptr;
}
template<class T>
bool operator!= (const shared_array<T>& a, const shared_array<T>& b) {
	return a.ptr != b.ptr;
}
	
template<class T>
bool operator< (const shared_array<T>& a, const shared_array<T>& b) {
	return a.ptr < b.ptr;
}


} // end of namespace boost

#else
#	error "shared_array.h was already included"
#endif
#endif // INCLUDED_AI_BOOST_SHARED_ARRAY
