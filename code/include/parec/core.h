#pragma once

#include <array>
#include <cstdlib>
#include <functional>
#include <tuple>
#include <type_traits>

#include "parec/utils/runtime/runtime.h"
#include "parec/utils/functional_utils.h"
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

		template<typename E>
		E pickRandom(const std::vector<E>& list) {
			return list[rand(list.size())];
		}

	} // end namespace detail

	// ----- function handling ----------

	namespace detail {

		namespace detail {

			template<typename Function> struct fun_type_of_helper { };

			template<typename R, typename ... A>
			struct fun_type_of_helper<R(A...)> {
			  typedef R type(A...);
			};

			// get rid of const modifier
			template<typename T>
			struct fun_type_of_helper<const T> : public fun_type_of_helper<T> {};

			// get rid of pointers
			template<typename T>
			struct fun_type_of_helper<T*> : public fun_type_of_helper<T> {};

			// handle class of member function pointers
			template<typename R, typename C, typename ... A>
			struct fun_type_of_helper<R(C::*)(A...)> : public fun_type_of_helper<R(A...)> {};

			// get rid of const modifier in member function pointer
			template<typename R, typename C, typename ... A>
			struct fun_type_of_helper<R(C::*)(A...) const> : public fun_type_of_helper<R(A...)> {};

		} // end namespace detail


		template <typename Lambda>
		struct fun_type_of : public detail::fun_type_of_helper<decltype(&Lambda::operator())> { };

		template<typename R, typename ... P>
		struct fun_type_of<R(P...)> : public detail::fun_type_of_helper<R(P...)> { };

		template<typename R, typename ... P>
		struct fun_type_of<R(*)(P...)> : public fun_type_of<R(P...)> { };

		template<typename R, typename ... P>
		struct fun_type_of<R(* const)(P...)> : public fun_type_of<R(P...)> { };

		template<typename R, typename C, typename ... P>
		struct fun_type_of<R(C::*)(P...)> : public detail::fun_type_of_helper<R(C::*)(P...)> { };

		template<typename R, typename C, typename ... P>
		struct fun_type_of<R(C::* const)(P...)> : public fun_type_of<R(C::*)(P...)> { };


	} // end namespace detail

	// --- function definitions ---

	template<
		typename FunctorType,
		typename FunctionType = typename detail::fun_type_of<FunctorType>::type
	>
	std::function<FunctionType> toFunction(const FunctorType& f) {
		return std::function<FunctionType>(f);
	}

	template<typename O, typename I, typename ... Funs>
	struct fun_def {
		typedef I in_type;
		typedef O out_type;

		std::function<bool(I)> bc_test;
		std::vector<std::function<O(I)>> base;
		std::vector<std::function<O(I,Funs...)>> step;

		fun_def(
			const std::function<bool(I)>& test,
			const std::vector<std::function<O(I)>>& base,
			const std::vector<std::function<O(I,Funs...)>>& step
		) : bc_test(test), base(base), step(step) {}

		O operator()(const I& in, const Funs& ... funs) const {
			if (bc_test(in)) return detail::pickRandom(base)(in);
			return detail::pickRandom(step)(in, funs...);
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

		template<
			typename O,
			typename I,
			typename ... Funs
		>
		fun_def<O,I,Funs...> fun_intern(
				const std::function<bool(I)>& test,
				const std::vector<std::function<O(I)>>& base,
				const std::vector<std::function<O(I,Funs...)>>& step
		) {
			return fun_def<O,I,Funs...>(test, base, step);
		}

		template<
			typename O,
			typename I,
			typename ... Funs
		>
		fun_def<O,I,Funs...> fun_intern(
				const std::function<bool(I)>& test,
				const std::function<O(I)>& base,
				const std::vector<std::function<O(I,Funs...)>>& step
		) {
			return fun_intern<O,I,Funs...>(test, toVector(base), step);
		}

		template<
			typename O,
			typename I,
			typename ... Funs
		>
		fun_def<O,I,Funs...> fun_intern(
				const std::function<bool(I)>& test,
				const std::function<O(I)>& base,
				const std::function<O(I,Funs...)>& step
		) {
			return fun_intern<O,I,Funs...>(test, base, toVector(step));
		}

	}


	template<
		typename BT, typename BC, typename SC,
		typename dummy = typename std::enable_if<
				!is_vector<BT>::value && !is_vector<BC>::value && !is_vector<SC>::value &&
				!is_std_function<BT>::value && !is_std_function<BC>::value && !is_std_function<SC>::value
			,int>::type
	>
	auto fun(const BT& a, const BC& b, const SC& c)->decltype(detail::fun_intern(toFunction(a), toFunction(b), toFunction(c))) {
		return detail::fun_intern(toFunction(a), toFunction(b), toFunction(c));
	}

	template<
		typename BT, typename BC, typename SC,
		typename dummy = typename std::enable_if<
				!is_vector<BT>::value && !is_vector<BC>::value &&
				!is_std_function<BT>::value && !is_std_function<BC>::value
			,int>::type
	>
	auto fun(const BT& a, const BC& b, const std::vector<SC>& c)->decltype(detail::fun_intern(toFunction(a), toFunction(b),c)) {
		return detail::fun_intern(toFunction(a), toFunction(b),c);
	}

	template<
		typename BT, typename BC, typename SC,
		typename dummy = typename std::enable_if<
				!is_vector<BT>::value && !is_std_function<BT>::value
			,int>::type
	>
	auto fun(const BT& a, const std::vector<BC>& b, const std::vector<SC>& c)->decltype(detail::fun_intern(toFunction(a), b,c)) {
		return detail::fun_intern(toFunction(a), b,c);
	}


	// --- add pick wrapper support ---

	template<typename F, typename ... Fs>
	std::vector<std::function<typename detail::fun_type_of<F>::type>> pick(const F& f, const Fs& ... fs) {
		typedef typename std::function<typename detail::fun_type_of<F>::type> fun_type;
		return toVector<fun_type>(f,fs...);
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
				return caller<n-1>().call<O>(f,i,d,args...,parec<n>(d));
			}
		};

		template<>
		struct caller<0> {
			template<typename O, typename F, typename I, typename D, typename ... Args>
			utils::runtime::Future<O> call(const F& f, const I& i, const D& d, const Args& ... args) const {
				return utils::runtime::spawn([=]() { return f(i,parec<0>(d),args...); });
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
		unsigned i = 0,
		typename ... Defs,
		typename I = typename type_at<i,type_list<Defs...>>::type::in_type,
		typename O = typename type_at<i,type_list<Defs...>>::type::out_type
	>
	std::function<utils::runtime::Future<O>(I)> parec(const rec_defs<Defs...>& defs) {
		auto x = std::get<i>(defs);
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
