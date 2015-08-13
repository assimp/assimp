
#ifndef __AI_BOOST_SCOPED_ARRAY_INCLUDED
#define __AI_BOOST_SCOPED_ARRAY_INCLUDED

#ifndef BOOST_SCOPED_ARRAY_HPP_INCLUDED

namespace boost {

// small replacement for boost::scoped_array
template <class T>
class scoped_array
{
public:

	// provide a default construtctor
	scoped_array()
		: ptr(0)
	{
	}

	// construction from an existing heap object of type T
	scoped_array(T* _ptr)
		: ptr(_ptr)
	{
	}

	// automatic destruction of the wrapped object at the
	// end of our lifetime
	~scoped_array()
	{
		delete[] ptr;
	}

	inline T* get()
	{
		return ptr;
	}

	inline T* operator-> ()
	{
		return ptr;
	}

	inline void reset (T* t = 0)
	{
		delete[] ptr;
		ptr = t;
	}

	T & operator[](std::ptrdiff_t i) const
	{
		return ptr[i];
	}

	void swap(scoped_array & b)
	{
		std::swap(ptr, b.ptr);
	}

private:

	// encapsulated object pointer
	T* ptr;

};

template<class T>
inline void swap(scoped_array<T> & a, scoped_array<T> & b)
{
	a.swap(b);
}

} // end of namespace boost

#else
#	error "scoped_array.h was already included"
#endif
#endif // __AI_BOOST_SCOPED_ARRAY_INCLUDED

