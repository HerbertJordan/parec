#pragma once

#include <array>
#include <cstdlib>
#include <functional>
#include <tuple>
#include <type_traits>

#include "parec/utils/runtime/runtime.h"
#include "parec/utils/functional_utils.h"
#include "parec/utils/tuple_utils.h"
#include "parec/utils/vector_utils.h"

namespace parec {

	template<typename T> struct prec_fun;

	template<typename O, typename I>
	struct prec_fun<O(I)> {
		typedef std::function<utils::runtime::Future<O>(I)> type;
	};

	namespace detail {

		int rand(int x) {
			return (std::rand()/(float)RAND_MAX) * x;
		}

		template<typename T>
		T pickRandom(const T& t) {
			return t;
		}

		template<typename T, typename ... Others>
		T pickRandom(const T& first, const Others& ... others) {
			if (rand(sizeof...(Others)+1) == 0) return first;
			return pickRandom(others...);
		}

		template<unsigned L>
		struct random_caller {

			template<
				typename Res,
				typename ... Versions,
				typename ... Args
			>
			Res callRandom(unsigned pos, const std::tuple<Versions...>& version, const Args& ... args) {
				if (pos == L) return std::get<L>(version)(args...);
				return random_caller<L-1>().template callRandom<Res>(pos,version,args...);
			}

			template<
				typename Res,
				typename ... Versions,
				typename ... Args
			>
			Res callRandom(const std::tuple<Versions...>& version, const Args& ... args) {
				int index = rand(sizeof...(Versions));
				return callRandom<Res>(index, version, args...);
			}

		};

		template<>
		struct random_caller<0> {
			template<
				typename Res,
				typename ... Versions,
				typename ... Args
			>
			Res callRandom(unsigned, const std::tuple<Versions...>& versions, const Args& ... args) {
				return std::get<0>(versions)(args...);
			}

			template<
				typename Res,
				typename ... Versions,
				typename ... Args
			>
			Res callRandom(const std::tuple<Versions...>& versions, const Args& ... args) {
				return std::get<0>(versions)(args...);
			}
		};

	} // end namespace detail

	// ----- function handling ----------

	template<
		typename O,
		typename I,
		typename BaseCaseTest,
		typename BaseCases,
		typename StepCases
	>
	struct fun_def;

	template<
		typename O,
		typename I,
		typename BaseCaseTest,
		typename ... BaseCases,
		typename ... StepCases
	>
	struct fun_def<O,I,BaseCaseTest,std::tuple<BaseCases...>,std::tuple<StepCases...>> {
		typedef I in_type;
		typedef O out_type;

		BaseCaseTest bc_test;
		std::tuple<BaseCases...> base;
		std::tuple<StepCases...> step;

		fun_def(
			const BaseCaseTest& test,
			const std::tuple<BaseCases...>& base,
			const std::tuple<StepCases...>& step
		) : bc_test(test), base(base), step(step) {}

		template<typename ... Funs>
		utils::runtime::Future<O> operator()(const I& in, const Funs& ... funs) const {
			// check for the base case
			const auto& base = this->base;
			if (bc_test(in)) return utils::runtime::spawn([=] {
				return detail::random_caller<sizeof...(BaseCases)-1>().template callRandom<O>(base, in);
			});

			// run step case
			const auto& step = this->step;
			return utils::runtime::spawn(
					// TODO: forward sequential alternative
					[=]() { return detail::random_caller<sizeof...(StepCases)-1>().template callRandom<O>(step, in, funs...); }
			);
		}

	};

	namespace detail {

		template<typename T>
		struct is_fun_def : public std::false_type {};

		template<typename O, typename I, typename ... T>
		struct is_fun_def<fun_def<O,I,T...>> : public std::true_type {};

		template<typename T>
		struct is_fun_def<const T> : public is_fun_def<T> {};

		template<typename T>
		struct is_fun_def<T&> : public is_fun_def<T> {};

	}

	template<
		typename BT, typename First_BC, typename ... BC, typename ... SC,
		typename O = typename parec::lambda_traits<First_BC>::result_type,
		typename I = typename parec::lambda_traits<First_BC>::arg1_type
	>
	fun_def<O,I,BT,std::tuple<First_BC,BC...>,std::tuple<SC...>>
	fun(const BT& a, const std::tuple<First_BC,BC...>& b, const std::tuple<SC...>& c) {
		return fun_def<O,I,BT,std::tuple<First_BC,BC...>,std::tuple<SC...>>(a,b,c);
	}

	template<
		typename BT, typename BC, typename SC,
		typename filter = typename std::enable_if<!is_tuple<BC>::value && !is_tuple<SC>::value,int>::type
	>
	auto fun(const BT& a, const BC& b, const SC& c) -> decltype(fun(a,std::make_tuple(b),std::make_tuple(c))) {
		return fun(a,std::make_tuple(b),std::make_tuple(c));
	}

	template<
		typename BT, typename BC, typename SC,
		typename filter = typename std::enable_if<!is_tuple<BC>::value && is_tuple<SC>::value,int>::type
	>
	auto fun(const BT& a, const BC& b, const SC& c) -> decltype(fun(a,std::make_tuple(b),c)) {
		return fun(a,std::make_tuple(b),c);
	}

	template<
		typename BT, typename BC, typename SC,
		typename filter = typename std::enable_if<is_tuple<BC>::value && !is_tuple<SC>::value,int>::type
	>
	auto fun(const BT& a, const BC& b, const SC& c) -> decltype(fun(a,b,std::make_tuple(c))) {
		return fun(a,b,std::make_tuple(c));
	}


	// --- add pick wrapper support ---

	template<typename F, typename ... Fs>
	std::tuple<F,Fs...> pick(const F& f, const Fs& ... fs) {
		return std::make_tuple(f,fs...);
	}


	// --- recursive definitions ---

	template<typename ... Defs> struct rec_defs;

	template<
		unsigned i = 0,
		typename ... Defs,
		typename I = typename type_at<i,type_list<Defs...>>::type::in_type,
		typename O = typename type_at<i,type_list<Defs...>>::type::out_type
	>
	std::function<utils::runtime::Future<O>(I)> parec(const rec_defs<Defs...>& );


	namespace detail {

		template<unsigned n>
		struct caller {
			template<typename O, typename F, typename I, typename D, typename ... Args>
			utils::runtime::Future<O> call(const F& f, const I& i, const D& d, const Args& ... args) const {
				return caller<n-1>().template call<O>(f,i,d,args...,parec<n>(d));
			}
		};

		template<>
		struct caller<0> {
			template<typename O, typename F, typename I, typename D, typename ... Args>
			utils::runtime::Future<O> call(const F& f, const I& i, const D& d, const Args& ... args) const {
				return f(i,parec<0>(d),args...);
			}
		};


		template<typename T>
		struct is_rec_def : public std::false_type {};

		template<typename ... Defs>
		struct is_rec_def<rec_defs<Defs...>> : public std::true_type {};

		template<typename T>
		struct is_rec_def<T&> : public is_rec_def<T> {};

		template<typename T>
		struct is_rec_def<const T> : public is_rec_def<T> {};

	}


	template<typename ... Defs>
	struct rec_defs : public std::tuple<Defs...> {
		template<typename ... Args>
		rec_defs(const Args& ... args) : std::tuple<Defs...>(args...) {}


		template<
			unsigned i,
			typename O,
			typename I
		>
		utils::runtime::Future<O> call(const I& in) const {
			// get targeted function
			auto x = std::get<i>(*this);

			// call target function with an async
			return detail::caller<sizeof...(Defs)-1>().template call<O>(x,in,*this);
		}

	};


	template<
		typename ... Defs
	>
	rec_defs<Defs...> group(const Defs& ... defs) {
		return rec_defs<Defs...>(defs...);
	}


	// --- parec operator ---


	template<
		unsigned i,
		typename ... Defs,
		typename I,
		typename O
	>
	std::function<utils::runtime::Future<O>(I)> parec(const rec_defs<Defs...>& defs) {
		return [=](const I& in)->utils::runtime::Future<O> {
			return defs.template call<i,O,I>(in);
		};
	}


	template<
		unsigned i = 0,
		typename First,
		typename ... Rest,
		typename dummy = typename std::enable_if<detail::is_fun_def<First>::value,int>::type
	>
	auto prec(const First& f, const Rest& ... r)->decltype(parec<i>(group(f,r...))) {
		return parec<i>(group(f,r...));
	}

	template<
		typename BT, typename BC, typename SC,
		typename dummy = typename std::enable_if<!detail::is_fun_def<BT>::value,int>::type
	>
	auto prec(const BT& t, const BC& b, const SC& s)->decltype(prec<0>(fun(t,b,s))) {
		return prec<0>(fun(t,b,s));
	}

} // end namespace parec
