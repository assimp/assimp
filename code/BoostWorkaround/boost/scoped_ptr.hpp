
#ifndef __AI_BOOST_SCOPED_PTR_INCLUDED
#define __AI_BOOST_SCOPED_PTR_INCLUDED

#ifndef BOOST_SCOPED_PTR_HPP_INCLUDED

namespace boost {

// small replacement for boost::scoped_ptr
template <class T>
class scoped_ptr
{
public:

	// provide a default construtctor
	scoped_ptr()
		: ptr(0)
	{
	}

	// construction from an existing heap object of type T
	scoped_ptr(T* _ptr)
		: ptr(_ptr)
	{
	}

	// automatic destruction of the wrapped object at the
	// end of our lifetime
	~scoped_ptr()
	{
		delete ptr;
	}

	inline T* get() const
	{
		return ptr;
	}

	inline operator T*()
	{
		return ptr;
	}

	inline T* operator-> ()
	{
		return ptr;
	}

	inline void reset (T* t = 0)
	{
		delete ptr;
		ptr = t;
	}

	void swap(scoped_ptr & b)
	{
		std::swap(ptr, b.ptr);
	}

private:

	// encapsulated object pointer
	T* ptr;

};

template<class T>
inline void swap(scoped_ptr<T> & a, scoped_ptr<T> & b)
{
	a.swap(b);
}

} // end of namespace boost

#else
#	error "scoped_ptr.h was already included"
#endif
#endif // __AI_BOOST_SCOPED_PTR_INCLUDED

