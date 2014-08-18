#pragma once

#include <utility>

namespace parec {

	/**
	 * NOTE: those are prototype implementations for checking the suitability of building
	 * algorithms on top of it. Later implementations will have to provide smarter solutions.
	 */

	// ----- an all operator ----

	bool all() { return true; }

	template<typename ... Rest>
	bool all(std::future<bool>&& a, Rest&& ... rest) {
		// TODO: test futures => check them as soon as they are done, not in order
		// TODO: return future instead of boolean
		return a.get() && all(std::forward<Rest>(rest)...);
	}


	// ----- an any operator ----

	bool any() { return false; }

	template<typename ... Rest>
	bool any(std::future<bool>&& a, Rest&& ... rest) {
		// TODO: test futures => check them as soon as they are done, not in order
		// TODO: return future instead of boolean
		return a.get() || any(std::forward<Rest>(rest)...);
	}


	// ---- a parallel operator ----

	void parallel() { }

	template<typename First, typename ... Rest>
	void parallel(std::future<First>&& f, Rest&& ... rest) {
		f.get();
		parallel(std::forward<Rest>(rest)...);
	}


} // end namespace parec
