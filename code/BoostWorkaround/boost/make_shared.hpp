
// please note that this replacement implementation does not
// provide the performance benefit of the original, which
// makes only one allocation as opposed to two allocations
// (smart pointer counter and payload) which are usually
// required if object and smart pointer are constructed
// independently.

#ifndef INCLUDED_AI_BOOST_MAKE_SHARED
#define INCLUDED_AI_BOOST_MAKE_SHARED


namespace boost {

	template <typename T>
	shared_ptr<T> make_shared() {
		return shared_ptr<T>(new T());
	}

	template <typename T, typename T0>
	shared_ptr<T> make_shared(const T0& t0) {
		return shared_ptr<T>(new T(t0));
	}

	template <typename T, typename T0,typename T1>
	shared_ptr<T> make_shared(const T0& t0, const T1& t1) {
		return shared_ptr<T>(new T(t0,t1));
	}

	template <typename T, typename T0,typename T1,typename T2>
	shared_ptr<T> make_shared(const T0& t0, const T1& t1, const T2& t2) {
		return shared_ptr<T>(new T(t0,t1,t2));
	}

	template <typename T, typename T0,typename T1,typename T2,typename T3>
	shared_ptr<T> make_shared(const T0& t0, const T1& t1, const T2& t2, const T3& t3) {
		return shared_ptr<T>(new T(t0,t1,t2,t3));
	}

	template <typename T, typename T0,typename T1,typename T2,typename T3, typename T4>
	shared_ptr<T> make_shared(const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4) {
		return shared_ptr<T>(new T(t0,t1,t2,t3,t4));
	}

	template <typename T, typename T0,typename T1,typename T2,typename T3, typename T4, typename T5>
	shared_ptr<T> make_shared(const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) {
		return shared_ptr<T>(new T(t0,t1,t2,t3,t4,t5));
	}

	template <typename T, typename T0,typename T1,typename T2,typename T3, typename T4, typename T5, typename T6>
	shared_ptr<T> make_shared(const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6) {
		return shared_ptr<T>(new T(t0,t1,t2,t3,t4,t5,t6));
	}
}


#endif 
