
#ifndef INCLUDED_AI_BOOST_SHARED_PTR
#define INCLUDED_AI_BOOST_SHARED_PTR

#ifndef BOOST_SCOPED_PTR_HPP_INCLUDED

// ------------------------------
// Internal stub
namespace boost {
	namespace detail {
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
					delete pt;
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
// Small replacement for boost::shared_ptr, not threadsafe because no
// atomic reference counter is in use.
// ------------------------------
template <class T>
class shared_ptr
{
	template <typename TT> friend class shared_ptr;

	template<class TT, class U> friend shared_ptr<TT> static_pointer_cast   (shared_ptr<U> ptr);
	template<class TT, class U> friend shared_ptr<TT> dynamic_pointer_cast  (shared_ptr<U> ptr);
	template<class TT, class U> friend shared_ptr<TT> const_pointer_cast    (shared_ptr<U> ptr);

	template<class TT> friend bool operator== (const shared_ptr<TT>& a, const shared_ptr<TT>& b);
	template<class TT> friend bool operator!= (const shared_ptr<TT>& a, const shared_ptr<TT>& b);
	template<class TT> friend bool operator<  (const shared_ptr<TT>& a, const shared_ptr<TT>& b);

public:

	typedef T element_type;

public:

	// provide a default constructor
	shared_ptr()
		: ptr()
		, ctr(NULL)
	{
	}

	// construction from an existing object of type T
	explicit shared_ptr(T* ptr)
		: ptr(ptr)
		, ctr(ptr ? new detail::controller() : NULL)
	{
	}

	shared_ptr(const shared_ptr& r)
		: ptr(r.ptr)
		, ctr(r.ctr ? r.ctr->incref() : NULL)
	{
	}

	template <typename Y>
	shared_ptr(const shared_ptr<Y>& r,typename detail::is_convertible<T,Y>::result = detail::empty())
		: ptr(r.ptr)
		, ctr(r.ctr ? r.ctr->incref() : NULL)
	{
	}

	// automatic destruction of the wrapped object when all
	// references are freed.
	~shared_ptr()	{
		if (ctr) {
			ctr = ctr->decref(ptr);
		}
	}

	shared_ptr& operator=(const shared_ptr& r) {
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
	shared_ptr& operator=(const shared_ptr<Y>& r) {
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
	inline operator T*() const {
		return ptr;
	}

	inline T* operator-> () const	{
		return ptr;
	}

	// standard semantics
	inline T* get() {
		return ptr;
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
		ctr = ptr?new detail::controller():NULL;
	}

	void swap(shared_ptr & b)	{
		std::swap(ptr, b.ptr);
		std::swap(ctr, b.ctr);
	}

private:


	// for use by the various xxx_pointer_cast helper templates
	explicit shared_ptr(T* ptr, detail::controller* ctr)
		: ptr(ptr)
		, ctr(ctr->incref())
	{
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

template<class T>
bool operator== (const shared_ptr<T>& a, const shared_ptr<T>& b) {
	return a.ptr == b.ptr;
}
template<class T>
bool operator!= (const shared_ptr<T>& a, const shared_ptr<T>& b) {
	return a.ptr != b.ptr;
}
	
template<class T>
bool operator< (const shared_ptr<T>& a, const shared_ptr<T>& b) {
	return a.ptr < b.ptr;
}


template<class T, class U>
inline shared_ptr<T> static_pointer_cast( shared_ptr<U> ptr)
{  
   return shared_ptr<T>(static_cast<T*>(ptr.ptr),ptr.ctr);
}

template<class T, class U>
inline shared_ptr<T> dynamic_pointer_cast( shared_ptr<U> ptr)
{  
   return shared_ptr<T>(dynamic_cast<T*>(ptr.ptr),ptr.ctr);
}

template<class T, class U>
inline shared_ptr<T> const_pointer_cast( shared_ptr<U> ptr)
{  
   return shared_ptr<T>(const_cast<T*>(ptr.ptr),ptr.ctr);
}



} // end of namespace boost

#else
#	error "shared_ptr.h was already included"
#endif
#endif // INCLUDED_AI_BOOST_SCOPED_PTR
