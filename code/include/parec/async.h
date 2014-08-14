#pragma once

#include <type_traits>
#include <future>

#include "parec/core.h"

namespace parec {

	/**
	 * A simple version of an async operator based on the prec operator.
	 */
	template< class Function>
	std::future<typename std::result_of<Function()>::type>
	async(const Function& f ) {
		struct empty {};
		typedef typename std::result_of<Function()>::type res_type;
		typedef typename prec_fun<res_type(empty)>::type fun_type;
		// maps the operation to a recursion
		return prec(fun(
				[](empty)->bool { return true; },
				[=](empty)->res_type { return f(); },
				[](empty, const fun_type&)->res_type { return res_type(); }
		))(empty());
	}


	/**
	 * NOTE: those are prototype implementations for checking the suitability of building
	 * algorithms on top of it. Later implementations will have to provide smarter solutions.
	 */

	// ----- an all operator ----

	bool all() { return true; }

	bool all(std::future<bool>&& a) {
		return a.get();
	}

	bool all(std::future<bool>&& a, std::future<bool>&& b) {
		// TODO: test futures => check them as soon as they are done, not in order
		// TODO: return future instead of boolean
		return a.get() && b.get();
	}

	bool all(std::future<bool>&& a, std::future<bool>&& b, std::future<bool>&& c) {
		// TODO: test futures => check them as soon as they are done, not in order
		// TODO: return future instead of boolean
		return a.get() && b.get() && c.get();
	}

	// ----- an any operator ----

	bool any() { return false; }

	bool any(std::future<bool>&& a) {
		return a.get();
	}

	bool any(std::future<bool>&& a, std::future<bool>&& b) {
		// TODO: test futures => check them as soon as they are done, not in order
		// TODO: return future instead of boolean
		return a.get() || b.get();
	}


	bool any(std::future<bool>&& a, std::future<bool>&& b, std::future<bool>&& c) {
		// TODO: test futures => check them as soon as they are done, not in order
		// TODO: return future instead of boolean
		return a.get() || b.get() || c.get();
	}

} // end namespace parec
