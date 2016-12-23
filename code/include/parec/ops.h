#pragma once

#include <cstdlib>
#include <functional>
#include <future>
#include <mutex>
#include <array>

#include "parec/core.h"
#include "parec/async.h"
#include "parec/utils/sequence.h"

namespace parec {

	// ----- parallel loops ------

	namespace detail {

		template<typename Iter>
		size_t distance(const Iter& a, const Iter& b) {
			return std::distance(a,b);
		}

		size_t distance(int a, int b) {
			return b-a;
		}

		template<typename Iter>
		size_t distance(const std::pair<Iter,Iter>& r) {
			return distance(r.first,r.second);
		}

		template<typename Iter>
		auto access(const Iter& iter) -> decltype(*iter) {
			return *iter;
		}

		int access(int a) {
			return a;
		}

		// TODO consider renaming this function to hypervolume
		template<typename Iter, size_t dims>
		size_t area(const std::array<std::pair<Iter,Iter>,dims>& range) {
			size_t res = 1;
			for(size_t i = 0; i<dims; i++) {
				res *= distance(range[i].first, range[i].second);
			}
			return res;
		}


		template<size_t idx>
		struct scanner {
			scanner<idx-1> nested;
			template<typename Iter, size_t dims, typename Op>
			void operator()(const std::array<std::pair<Iter,Iter>,dims>& range, std::array<Iter,dims>& cur, const Op& op) {
				auto& i = cur[dims-idx];
				for(i = range[dims-idx].first; i != range[dims-idx].second; ++i ) {
					nested(range, cur, op);
				}
			}
		};

		template<>
		struct scanner<0> {
			template<typename Iter, size_t dims, typename Op>
			void operator()(const std::array<std::pair<Iter,Iter>,dims>&, std::array<Iter,dims>& cur, const Op& op) {
				op(cur);
			}
		};

		template<typename Iter, size_t dims, typename Op>
		void for_each(const std::array<std::pair<Iter,Iter>,dims>& range, const Op& op) {

			// the current position
			std::array<Iter,dims> cur;

			// scan range
			scanner<dims>()(range, cur, op);

		}

	}

	namespace loop_policy {

		/**
		 * In the binary split policy, ranges of for-loops are split recursively into smaller ranges
		 * and distributed among the available threads.
		 */
		struct binary_split {

			/**
			 * The implementation for standard 1D iterators.
			 */
			template<typename Iter, typename Op>
			void operator()(Iter a, Iter b, const Op& op) {
				// implements a binary splitting policy for iterating over the given iterator range
				auto cut = detail::distance(a,b) / 1000; // hardcoded cutoff, to be removed when runtime is smart enough to find good value
				cut = (cut < 1) ? 1 : cut;
				typedef std::pair<Iter,Iter> range;
				prec(
					[cut](const range& r) {
						return detail::distance(r.first,r.second) <= cut;
					},
					[&](const range& r) {
						if (detail::distance(r.first,r.second) < 1) return;
						for(auto it = r.first; it != r.second; ++it) op(detail::access(it));
					},
					[](const range& r, const auto& f) {
						// here we have the binary splitting
						auto mid = r.first + (r.second - r.first)/2;
						auto a = f(range(r.first, mid));
						auto b = f(range(mid, r.second));
						a.get(); b.get();		// sync futures (also automated by destructor)
					}
				)(range(a,b)).get();
			}

			/**
			 * The implementation for higher-dimensions iterators.
			 */
			template<typename Iter, size_t dims, typename Op>
			void operator()(const std::array<Iter,dims>& a, const std::array<Iter,dims>& b, const Op& op) {
				// process 0-dimensional case
				if (dims == 0) return; // no iterations required

				// implements a recursive splitting policy for iterating over the given iterator range
				using range = std::array<std::pair<Iter,Iter>,dims>;
				range full;
				for(size_t i = 0; i<dims; i++) {
					full[i] = std::make_pair(a[i],b[i]);
				}
				auto cut = detail::area(full) / 1000;
				cut = (cut < 1) ? 1 : cut;
				prec(
					[cut](const range& r) {
						return detail::area(r) <= cut;
					},
					[&](const range& r) {
						if (detail::area(r) < 1) return;
						detail::for_each(r,op);
					},
					[](const range& r, const auto& f) {
						// here we have the binary splitting

						// TODO: think about splitting all dimensions

						// get the longest dimension
						size_t maxDim = 0;
						size_t maxDist = detail::distance(r[0]);
						for(size_t i = 1; i<dims;++i) {
							size_t curDist = detail::distance(r[i]);
							if (curDist > maxDist) {
								maxDim = i;
								maxDist = curDist;
							}
						}

						// split the longest dimension
						range a = r;
						range b = r;

						auto mid = r[maxDim].first + (maxDist / 2);
						a[maxDim].second = mid;
						b[maxDim].first = mid;

						// process branches
						auto x = f(a);
						auto y = f(b);
						x.get(); y.get();		// sync futures (also automated by destructor)
					}
				)(full).get();
			}

		};


		/**
		 * In the queued policy, the handed in tasks are placed in a queue and distributed among
		 * the available threads in a work-dispatcher style of parallelism.
		 */
		struct queue {

			/**
			 * The implementation for standard 1D iterators.
			 */
			template<typename Iter, typename Op>
			void operator()(Iter a, Iter b, const Op& op) {
				// implementation of a worker-queue for the range to be iterated over
				std::vector<utils::runtime::Future<void>> futures;
				futures.reserve(detail::distance(a,b));
				// spawn tasks
				for(Iter i = a; i < b; ++i) {
					futures.push_back(async([i,&op](){
						op(detail::access(i));
					}));
				}
				// wait for tasks
				for(const auto& cur : futures) {
					cur.get();
				}
			}

			/**
			 * The implementation for higher-dimensions iterators.
			 */
			template<typename Iter, size_t dims, typename Op>
			void operator()(const std::array<Iter,dims>& a, const std::array<Iter,dims>& b, const Op& op) {
				// process 0-dimensional case
				if (dims == 0) return; // no iterations required

				// implementation of a worker-queue for the range to be iterated over

				// create the full range
				using range = std::array<std::pair<Iter,Iter>,dims>;
				range full;
				for(size_t i = 0; i<dims; i++) {
					full[i] = std::make_pair(a[i],b[i]);
				}

				// spawn all tasks
				std::vector<utils::runtime::Future<void>> futures;
				futures.reserve(detail::area(full));
				detail::for_each(full,[&](const auto& pos) {
					futures.push_back(async([pos,&op](){
						op(pos);
					}));
				});

				// wait for all tasks to finish
				for(const auto& cur : futures) {
					cur.get();
				}
			}

		};

	}


	template<typename policy = loop_policy::binary_split, typename Iter, size_t dims, typename Op>
	void pfor(const std::array<Iter,dims>& a, const std::array<Iter,dims>& b, const Op& op) {
		// just forward execution to loop policy
		policy()(a,b,op);
	}

	/**
	 * A parallel for-each implementation iterating over the given range of elements.
	 */
	template<typename policy = typename loop_policy::binary_split, typename Iter, typename Op>
	void pfor(Iter a, Iter b, const Op& op) {
		// just forward execution to loop policy
		policy()(a,b,op);
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the given container.
	 */
	template<typename policy = loop_policy::binary_split, typename Container, typename Op>
	void pfor(Container& c, const Op& op) {
		pfor<policy>(c.begin(), c.end(), op);
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the given container.
	 */
	template<typename policy = loop_policy::binary_split, typename Container, typename Op>
	void pfor(const Container& c, const Op& op) {
		pfor<policy>(c.begin(), c.end(), op);
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
			[&](const range& r, const auto& f)->res_type {
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


	// ----- map / reduce ------


	template<
		typename Iter, size_t dims,
		typename MapOp,
		typename ReduceOp,
		typename InitLocalState,
		typename ReduceLocalState
	>
	typename lambda_traits<ReduceOp>::result_type
	map_reduce(
			const std::array<Iter,dims>& a,
			const std::array<Iter,dims>& b,
			const MapOp& map,
			const ReduceOp& reduce,
			const InitLocalState& init,
			const ReduceLocalState& exit = [](typename lambda_traits<ReduceOp>::result_type r) { return r; } -> lambda_traits<ReduceOp>::result_type
			) {

		using res_type = typename lambda_traits<ReduceOp>::result_type;

		using range = std::array<std::pair<Iter,Iter>,dims>;
		range full;
		for(size_t i = 0; i<dims; i++) {
			full[i] = std::make_pair(a[i],b[i]);
		}

		auto cut = detail::area(full) / 1000;
		cut = (cut < 1) ? 1 : cut;

		return prec(
			[&](const range& r) {
			return detail::area(r) <= cut;
			},
			[&](const range& r)->res_type {
				auto res = init();
				if (detail::area(r) < 1) return res;

				auto mapB = [map,&res](const std::array<Iter,dims>& cur) {
					return map(cur,res);
				};

				detail::for_each(r,mapB);

				return exit(res);
			},
			[&](const range& r, const auto& f)->res_type {
				// here we have the binary splitting

				// TODO: think about splitting all dimensions

				// get the longest dimension
				size_t maxDim = 0;
				size_t maxDist = detail::distance(r[0]);
				for(size_t i = 1; i<dims;++i) {
					size_t curDist = detail::distance(r[i]);
					if (curDist > maxDist) {
						maxDim = i;
						maxDist = curDist;
					}
				}

				// split the longest dimension
				range a = r;
				range b = r;

				auto mid = r[maxDim].first + (maxDist / 2);
				a[maxDim].second = mid;
				b[maxDim].first = mid;

				// process branches
				auto x = f(a);
				auto y = f(b);
				x.get(); y.get();		// sync futures (also automated by destructor)

				return reduce(std::move(x.extract()), std::move(y.extract()));
			}
		)(full).get();

		return res_type();

	}


	template<
		typename Iter,
		typename MapOp,
		typename ReduceOp,
		typename InitLocalState,
		typename ReduceLocalState
	>
	typename lambda_traits<ReduceOp>::result_type
	map_reduce(
			const Iter& a,
			const Iter& b,
			const MapOp& map,
			const ReduceOp& reduce,
			const InitLocalState& init,
			const ReduceLocalState& exit = [](typename lambda_traits<ReduceOp>::result_type r) { return r; } -> lambda_traits<ReduceOp>::result_type
		) {

		using res_type = typename lambda_traits<ReduceOp>::result_type;

		typedef std::pair<Iter, Iter> range;
		auto full = range(a, b);

		unsigned cut = detail::distance(full.first,full.second) / 1000;
		cut = (cut < 1) ? 1 : cut;

		return prec(
			[&](const range& r) {
				return std::distance(r.first,r.second) <= cut;
			},
			[&](const range& r)->res_type {
				auto res = init();
				for(auto it = r.first; it != r.second; ++it) {
					map(*it,res);
				}
				return exit(res);
			},
			[&](const range& r, const auto& f)->res_type {
				// here we have the binary splitting
				auto mid = r.first + (r.second - r.first)/2;
				auto a = f(range(r.first, mid));
				auto b = f(range(mid, r.second));
				return reduce(std::move(a.extract()), std::move(b.extract()));
			}
		)(full).get();


		return typename lambda_traits<ReduceOp>::result_type();

	}

	template<
		typename Container,
		typename MapOp,
		typename ReduceOp,
		typename InitLocalState,
		typename ReduceLocalState
	>
	typename lambda_traits<ReduceOp>::result_type
	map_reduce(
			const Container& c,
			const MapOp& map,
			const ReduceOp& reduce,
			const InitLocalState& init,
			const ReduceLocalState& exit = [](Container r) { return r; } -> Container
		) {

		return map_reduce(c.begin(), c.end(), map, reduce, init, exit);

	}

} // end namespace parec

