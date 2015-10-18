#pragma once

#include <cstdlib>
#include <functional>
#include <future>
#include <mutex>

#include "parec/core.h"
#include "parec/utils/sequence.h"

namespace parec {

	// ----- parallel loops ------

	/**
	 * A parallel for-each implementation iterating over the given range of elements.
	 */
	template<typename Iter, typename Op>
	void pfor(Iter a, Iter b, const Op& op) {
		// implements a binary splitting policy for iterating over the given iterator range
		auto cut = std::distance(a,b) / 1000;
		cut = (cut < 1) ? 1 : cut;
		typedef std::pair<Iter,Iter> range;
		prec(
			[cut](const range& r) {
				return std::distance(r.first,r.second) <= cut;
			},
			[&](const range& r) {
				if (std::distance(r.first,r.second) < 1) return;
				for(auto it = r.first; it != r.second; ++it) op(*it);
			},
			[](const range& r, const typename prec_fun<void(range)>::type& f) {
				// here we have the binary splitting
				auto mid = r.first + (r.second - r.first)/2;
				auto a = f(range(r.first, mid));
				auto b = f(range(mid, r.second));
				a.get(); b.get();		// sync futures (also automated by destructor)
			}
		)(range(a,b)).get();
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the given container.
	 */
	template<typename Container, typename Op>
	void pfor(Container& c, const Op& op) {
		pfor(c.begin(), c.end(), op);
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the given container.
	 */
	template<typename Container, typename Op>
	void pfor(const Container& c, const Op& op) {
		pfor(c.begin(), c.end(), op);
	}


	// ----- reduction ------

	template<typename Iter, typename Op>
	typename lambda_traits<Op>::result_type
	preduce(const Iter& a, const Iter& b, Op& op) {
		typedef typename lambda_traits<Op>::result_type res_type;

		// implements a binary splitting policy for iterating over the given iterator range
		typedef std::pair<Iter,Iter> range;
		return prec(
			[](const range& r) {
				return std::distance(r.first,r.second) <= 1;
			},
			[&](const range& r)->res_type {
				if (r.first == r.second) return res_type();
				return op(*r.first,res_type());
			},
			[&](const range& r, const typename prec_fun<res_type(range)>::type& f)->res_type {
				// here we have the binary splitting
				auto mid = r.first + (r.second - r.first)/2;
				auto a = f(range(r.first, mid));
				auto b = f(range(mid, r.second));
				return op(a.get(), b.get());
			}
		)(range(a,b)).get();
	}

	/**
	 * A parallel reduce implementation over the elements of the given container.
	 */
	template<typename Container, typename Op>
	typename lambda_traits<Op>::result_type
	preduce(Container& c, Op& op) {
		return preduce(c.begin(), c.end(), op);
	}

	/**
	 * A parallel reduce implementation over the elements of the given container.
	 */
	template<typename Container, typename Op>
	typename lambda_traits<Op>::result_type
	preduce(const Container& c, const Op& op) {
		return preduce(c.begin(), c.end(), op);
	}

} // end namespace parec
