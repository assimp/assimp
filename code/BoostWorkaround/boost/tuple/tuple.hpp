// A very small replacement for boost::tuple
// (c) Alexander Gessler, 2008 [alexander.gessler@gmx.net]

#ifndef BOOST_TUPLE_INCLUDED
#define BOOST_TUPLE_INCLUDED

namespace boost	{
	namespace detail	{

		// Represents an empty tuple slot (up to 5 supported)
		struct nulltype {};

		// For readable error messages
		struct tuple_component_idx_out_of_bounds;

		// To share some code for the const/nonconst versions of the getters
		template <bool b, typename T>
		struct ConstIf {
			typedef T t;
		};

		template <typename T>
		struct ConstIf<true,T> {
			typedef const T t;
		};

		// Predeclare some stuff
		template <typename, unsigned, typename, bool, unsigned> struct value_getter;

		// Helper to obtain the type of a tuple element
		template <typename T, unsigned NIDX, typename TNEXT, unsigned N /*= 0*/>
		struct type_getter	{
			typedef type_getter<typename TNEXT::type,NIDX+1,typename TNEXT::next_type,N> next_elem_getter;
			typedef typename next_elem_getter::type type;
		};

		template <typename T, unsigned NIDX, typename TNEXT >
		struct type_getter <T,NIDX,TNEXT,NIDX>	{
			typedef T type;
		};

		// Base class for all explicit specializations of list_elem
		template <typename T, unsigned NIDX, typename TNEXT >
		struct list_elem_base {

			// Store template parameters
			typedef TNEXT next_type;
			typedef T type;

			static const unsigned nidx = NIDX;
		};

		// Represents an element in the tuple component list
		template <typename T, unsigned NIDX, typename TNEXT >
		struct list_elem : list_elem_base<T,NIDX,TNEXT>{

			// Real members
			T me;
			TNEXT next;

			// Get the value of a specific tuple element
			template <unsigned N>
			typename type_getter<T,NIDX,TNEXT,N>::type& get () {
				value_getter <T,NIDX,TNEXT,false,N> s;
				return s(*this);
			}

			// Get the value of a specific tuple element
			template <unsigned N>
			const typename type_getter<T,NIDX,TNEXT,N>::type& get () const {
				value_getter <T,NIDX,TNEXT,true,N> s;
				return s(*this);
			}

			// Explicit cast
			template <typename T2, typename TNEXT2 >
			operator list_elem<T2,NIDX,TNEXT2> () const	{
				list_elem<T2,NIDX,TNEXT2> ret;
				ret.me   = (T2)me;
				ret.next = next;
				return ret;
			}

			// Recursively compare two elements (last element returns always true)
			bool operator == (const list_elem& s) const	{
				return (me == s.me && next == s.next);
			}
		};

		// Represents a non-used tuple element - the very last element processed
		template <typename TNEXT, unsigned NIDX  >
		struct list_elem<nulltype,NIDX,TNEXT> : list_elem_base<nulltype,NIDX,TNEXT> {
			template <unsigned N, bool IS_CONST = true> struct value_getter		{
				/* just dummy members to produce readable error messages */
				tuple_component_idx_out_of_bounds operator () (typename ConstIf<IS_CONST,list_elem>::t& me);
			};
			template <unsigned N> struct type_getter  {
				/* just dummy members to produce readable error messages */
				typedef tuple_component_idx_out_of_bounds type;
			};

			// dummy
			list_elem& operator = (const list_elem& /*other*/)	{
				return *this;
			}

			// dummy
			bool operator == (const list_elem& other)	{
				return true;
			}
		};

		// Represents the absolute end of the list
		typedef list_elem<nulltype,0,int> list_end;

		// Helper obtain to query the value of a tuple element
		// NOTE: This can't be a nested class as the compiler won't accept a full or
		// partial specialization of a nested class of a non-specialized template
		template <typename T, unsigned NIDX, typename TNEXT, bool IS_CONST, unsigned N>
		struct value_getter	 {

			// calling list_elem
			typedef list_elem<T,NIDX,TNEXT> outer_elem;

			// typedef for the getter for next element
			typedef value_getter<typename TNEXT::type,NIDX+1,typename TNEXT::next_type,
				IS_CONST, N> next_value_getter;

			typename ConstIf<IS_CONST,typename type_getter<T,NIDX,TNEXT,N>::type>::t&
				operator () (typename ConstIf<IS_CONST,outer_elem >::t& me) {

				next_value_getter s;
				return s(me.next);
			}
		};

		template <typename T, unsigned NIDX, typename TNEXT, bool IS_CONST>
		struct value_getter <T,NIDX,TNEXT,IS_CONST,NIDX>	{
			typedef list_elem<T,NIDX,TNEXT> outer_elem;

			typename ConstIf<IS_CONST,T>::t& operator () (typename ConstIf<IS_CONST,outer_elem >::t& me) {
				return me.me;
			}
		};
	}

	// A very minimal implementation for up to 5 elements
	template <typename T0  = detail::nulltype,
		      typename T1  = detail::nulltype,
			  typename T2  = detail::nulltype,
			  typename T3  = detail::nulltype,
			  typename T4  = detail::nulltype>
	class tuple	{

		template <typename T0b,
		      typename T1b,
			  typename T2b,
			  typename T3b,
			  typename T4b >
		friend class tuple;

	private:

		typedef detail::list_elem<T0,0,
					detail::list_elem<T1,1,
						detail::list_elem<T2,2,
							detail::list_elem<T3,3,
								detail::list_elem<T4,4,
									detail::list_end > > > > > very_long;

		very_long m;

	public:

		// Get a specific tuple element
		template <unsigned N>
		typename detail::type_getter<T0,0,typename very_long::next_type, N>::type& get ()	{
			return m.template get<N>();
		}

		// ... and the const version
		template <unsigned N>
		const typename detail::type_getter<T0,0,typename very_long::next_type, N>::type& get () const	{
			return m.template get<N>();
		}


		// comparison operators
		bool operator== (const tuple& other) const	{
			return m == other.m;
		}

		// ... and the other way round
		bool operator!= (const tuple& other) const	{
			return !(m == other.m);
		}

		// cast to another tuple - all single elements must be convertible
		template <typename T0b, typename T1b,typename T2b,typename T3b, typename T4b>
		operator tuple <T0b,T1b,T2b,T3b,T4b> () const {
			tuple <T0b,T1b,T2b,T3b,T4b> s;
			s.m = (typename tuple <T0b,T1b,T2b,T3b,T4b>::very_long)m;
			return s;
		}
	};

	// Another way to access an element ...
	template <unsigned N,typename T0,typename T1,typename T2,typename T3,typename T4>
	inline typename tuple<T0,T1,T2,T3,T4>::very_long::template type_getter<N>::type& get (
			tuple<T0,T1,T2,T3,T4>& m)	{
			return m.template get<N>();
		}

	// ... and the const version
	template <unsigned N,typename T0,typename T1,typename T2,typename T3,typename T4>
	inline const typename tuple<T0,T1,T2,T3,T4>::very_long::template type_getter<N>::type& get (
			const tuple<T0,T1,T2,T3,T4>& m)	{
			return m.template get<N>();
		}

	// Constructs a tuple with 5 elements
	template <typename T0,typename T1,typename T2,typename T3,typename T4>
	inline tuple <T0,T1,T2,T3,T4> make_tuple (const T0& t0,
		const T1& t1,const T2& t2,const T3& t3,const T4& t4) {

		tuple <T0,T1,T2,T3,T4> t;
		t.template get<0>() = t0;
		t.template get<1>() = t1;
		t.template get<2>() = t2;
		t.template get<3>() = t3;
		t.template get<4>() = t4;
		return t;
	}

	// Constructs a tuple with 4 elements
	template <typename T0,typename T1,typename T2,typename T3>
	inline tuple <T0,T1,T2,T3> make_tuple (const T0& t0,
		const T1& t1,const T2& t2,const T3& t3) {
		tuple <T0,T1,T2,T3> t;
		t.template get<0>() = t0;
		t.template get<1>() = t1;
		t.template get<2>() = t2;
		t.template get<3>() = t3;
		return t;
	}

	// Constructs a tuple with 3 elements
	template <typename T0,typename T1,typename T2>
	inline tuple <T0,T1,T2> make_tuple (const T0& t0,
		const T1& t1,const T2& t2) {
		tuple <T0,T1,T2> t;
		t.template get<0>() = t0;
		t.template get<1>() = t1;
		t.template get<2>() = t2;
		return t;
	}

	// Constructs a tuple with 2 elements 
	template <typename T0,typename T1>
	inline tuple <T0,T1> make_tuple (const T0& t0,
		const T1& t1) {
		tuple <T0,T1> t;
		t.template get<0>() = t0;
		t.template get<1>() = t1;
		return t;
	}

	// Constructs a tuple with 1 elements (well ...)
	template <typename T0>
	inline tuple <T0> make_tuple (const T0& t0) {
		tuple <T0> t;
		t.template get<0>() = t0;
		return t;
	}

	// Constructs a tuple with 0 elements (well ...)
	inline tuple <> make_tuple () {
		tuple <> t;
		return t;
	}
}

#endif // !! BOOST_TUPLE_INCLUDED
