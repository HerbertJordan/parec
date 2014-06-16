#pragma once

#include <cstdlib>
#include <functional>
#include <future>

#include "parec/functional_utils.h"

namespace parec {

	namespace detail {

		template<typename T>
		T pickRandom(const T& t) {
			return t;
		}

		template<typename T, typename ... Others>
		T pickRandom(const T& first, const Others& ... others) {
			if ((std::rand()/(float)RAND_MAX) < 1.0/(sizeof...(Others)+1)) return first;
			return pickRandom(others...);
		}

	}

	// ----- a sequential recursive operator ------

	template<typename T> struct rec_fun;

	template<typename O, typename I>
	struct rec_fun<O(I)> {
		typedef std::function<O(I)> type;
	};

	template<
		typename I,
		typename O,
		typename ... As
	>
	O rec(
		I in,
		const std::function<bool(I)>& bc_test,
		const std::function<O(I)>& base,
		const std::function<O(I,const typename rec_fun<O(I)>::type&)>& step,
		As ... as
	) {
		// check for the base case
		if (bc_test(in)) return base(in);

		// compute the step case
		return (detail::pickRandom(step,as...))(in, [&](I x) { return rec(x,bc_test,base,step /*,as...*/ ); });
	}


	// ----- a parallel recursive operator ------

	template<typename T> struct prec_fun;

	template<typename O, typename I>
	struct prec_fun<O(I)> {
		typedef std::function<std::future<O>(I)> type;
	};


	template<
		typename I,
		typename O,
		typename ... As
	>
	std::future<O> prec(
		I in,
		const std::function<bool(I)>& bc_test,
		const std::function<O(I)>& base,
		const std::function<O(I,const typename prec_fun<O(I)>::type&)>& step,
		As ... as
	) {
		// check for the base case
		if (bc_test(in)) return std::async(base, in);

		// compute the step case
		return std::async((detail::pickRandom(step,as...)), in, [&](I x) { return prec(x, bc_test, base, step /*,as...*/); });
	}


} // end namespace parec
