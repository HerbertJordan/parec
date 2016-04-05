#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "parec/core.h"

namespace parec {

	using std::string;

	TEST(PickRandom, SimpleTest) {

		std::vector<int> data;
		std::srand(1);
		for(int i =0; i<20; i++) {
			data.push_back(detail::pickRandom(1,2,3,4,5));
		}

		EXPECT_EQ(std::vector<int>({5,2,3,4,5,2,2,2,2,2,1,2,4,5,5,3,4,3,1,4}), data);

	}

	TEST(RecOps, IsFunDef) {

		auto a = [](){return false;};
		EXPECT_FALSE(detail::is_fun_def<decltype(a)>::value);

		auto f = fun(
				[](int)->bool { return true; },
				[=](int)->float { return 0.0; },
				[](int, const auto&)->float { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(f)>::value);

		struct empty {};

		EXPECT_FALSE(is_vector<empty>::value);

		auto g = fun(
				[](empty)->bool { return true; },
				[=](empty)->float { return 0.0; },
				[](empty, const auto&)->float { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(g)>::value);
	}

	TEST(RecOps, IsFunDefGeneric) {

		auto a = [](){return false;};
		EXPECT_FALSE(detail::is_fun_def<decltype(a)>::value);

		auto f = fun(
				[](int)->bool { return true; },
				[=](int)->float { return 0.0; },
				[](int, const auto&)->float { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(f)>::value);

		struct empty {};

		EXPECT_FALSE(is_vector<empty>::value);

		auto g = fun(
				[](empty)->bool { return true; },
				[=](empty)->float { return 0.0; },
				[](empty, const auto&)->float { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(g)>::value);
	}

	TEST(RecOps, IsRecDef) {
		EXPECT_FALSE(detail::is_rec_def<int>::value);

		EXPECT_TRUE((detail::is_rec_def<rec_defs<int,int>>::value));
		EXPECT_TRUE((detail::is_rec_def<const rec_defs<int,int>>::value));
	}

	TEST(RecOps, Fib) {

		auto fib = prec(
				fun(
					[](int x)->bool { return x < 2; },
					[](int x)->int { return x; },
					[](int x, const auto& f)->int {
						auto a = f(x-1);
						auto b = f(x-2);
						return a.get() + b.get();
					}
				)
		);

		EXPECT_EQ( 0,fib(0).get());
		EXPECT_EQ( 1,fib(1).get());
		EXPECT_EQ( 1,fib(2).get());
		EXPECT_EQ( 2,fib(3).get());
		EXPECT_EQ( 3,fib(4).get());
		EXPECT_EQ( 5,fib(5).get());
		EXPECT_EQ( 8,fib(6).get());
		EXPECT_EQ(13,fib(7).get());
		EXPECT_EQ(21,fib(8).get());
		EXPECT_EQ(34,fib(9).get());

	}

	TEST(RecOps, FibShort) {

		auto fib = prec(
				[](int x)->bool { return x < 2; },
				[](int x)->int { return x; },
				[](int x, const auto& f)->int {
					auto a = f(x-1);
					auto b = f(x-2);
					return a.get() + b.get();
				}
		);

		EXPECT_EQ( 0,fib(0).get());
		EXPECT_EQ( 1,fib(1).get());
		EXPECT_EQ( 1,fib(2).get());
		EXPECT_EQ( 2,fib(3).get());
		EXPECT_EQ( 3,fib(4).get());
		EXPECT_EQ( 5,fib(5).get());
		EXPECT_EQ( 8,fib(6).get());
		EXPECT_EQ(13,fib(7).get());
		EXPECT_EQ(21,fib(8).get());
		EXPECT_EQ(34,fib(9).get());

	}

	TEST(RecOps, MultipleRecursion) {

		auto def = group(
				// function A
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->int { return 1; },
						[](int, const auto& A, const auto& B, const auto& C)->int {
							EXPECT_EQ(1,A(0).get());
							EXPECT_EQ(2,B(0).get());
							EXPECT_EQ(3,C(0).get());
							return 1;
						}
				),
				// function B
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->int { return 2; },
						[](int, const auto& A, const auto& B, const auto& C)->int {
							EXPECT_EQ(1,A(0).get());
							EXPECT_EQ(2,B(0).get());
							EXPECT_EQ(3,C(0).get());
							return 2;
						}
				),
				// function C
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->int { return 3; },
						[](int, const auto& A, const auto& B, const auto& C)->int {
							EXPECT_EQ(1,A(0).get());
							EXPECT_EQ(2,B(0).get());
							EXPECT_EQ(3,C(0).get());
							return 3;
						}
				)
		);

		auto A = parec<0>(def);
		auto B = parec<1>(def);
		auto C = parec<2>(def);

		EXPECT_EQ(1,A(1).get());
		EXPECT_EQ(2,B(1).get());
		EXPECT_EQ(3,C(1).get());
	}


	TEST(RecOps, EvenOdd) {

		auto def = group(
				// even
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return true; },
						[](int x, const auto& , const auto& odd)->bool {
							return odd(x-1).get();
						}
				),
				// odd
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return false; },
						[](int x, const auto& even, const auto& )->bool {
							return even(x-1).get();
						}
				)
		);

		auto even = parec<0>(def);
		auto odd = parec<1>(def);

		EXPECT_TRUE(even(0).get());
		EXPECT_TRUE(even(2).get());
		EXPECT_TRUE(even(4).get());
		EXPECT_TRUE(even(6).get());
		EXPECT_TRUE(even(8).get());

		EXPECT_FALSE(even(1).get());
		EXPECT_FALSE(even(3).get());
		EXPECT_FALSE(even(5).get());
		EXPECT_FALSE(even(7).get());
		EXPECT_FALSE(even(9).get());

		EXPECT_FALSE(odd(0).get());
		EXPECT_FALSE(odd(2).get());
		EXPECT_FALSE(odd(4).get());
		EXPECT_FALSE(odd(6).get());
		EXPECT_FALSE(odd(8).get());

		EXPECT_TRUE(odd(1).get());
		EXPECT_TRUE(odd(3).get());
		EXPECT_TRUE(odd(5).get());
		EXPECT_TRUE(odd(7).get());
		EXPECT_TRUE(odd(9).get());

	}

	TEST(RecOps, Even) {

		auto even = prec(
				// even
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return true; },
						[](int x, const auto& , const auto& odd)->bool {
							return odd(x-1).get();
						}
				),
				// odd
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return false; },
						[](int x, const auto& even, const auto& )->bool {
							return even(x-1).get();
						}
				)
		);

		EXPECT_TRUE(even(0).get());
		EXPECT_TRUE(even(2).get());
		EXPECT_TRUE(even(4).get());
		EXPECT_TRUE(even(6).get());
		EXPECT_TRUE(even(8).get());

		EXPECT_FALSE(even(1).get());
		EXPECT_FALSE(even(3).get());
		EXPECT_FALSE(even(5).get());
		EXPECT_FALSE(even(7).get());
		EXPECT_FALSE(even(9).get());

	}



	int fib(int x) {

		return prec(
				fun(
					[](int x) { return x < 2; },
					[](int x) { return x; },
					pick(
							[](int x, const auto& f) { return f(x-1).get() + f(x-2).get(); },
							[](int x, const auto& f) { return f(x-2).get() + f(x-1).get(); }
					)
				)
		)(x).get();
	}

	int fac(int x) {

		return prec(
				fun(
					[](int x) { return x < 2; },
					[](int) { return 1; },
					[](int x, const auto& f) { return x * f(x-1).get(); }
				)
		)(x).get();
	}

	TEST(RecOps, SimpleTest) {

		EXPECT_EQ(0, fib(0));
		EXPECT_EQ(1, fib(1));
		EXPECT_EQ(1, fib(2));
		EXPECT_EQ(2, fib(3));
		EXPECT_EQ(3, fib(4));
		EXPECT_EQ(5, fib(5));
		EXPECT_EQ(8, fib(6));

		EXPECT_EQ(1, fac(1));
		EXPECT_EQ(2, fac(2));
		EXPECT_EQ(6, fac(3));
		EXPECT_EQ(24, fac(4));

	}


	// ---- application tests --------

	int pfib(int x) {
		return prec(
				fun(
					[](int x) { return x < 2; },
					[](int x) { return x; },
					[](int x, const auto& f)->int {
						auto a = f(x-1);
						auto b = f(x-2);
						return a.get() + b.get();
					}
				)
		)(x).get();
	}


	TEST(RecOps, ParallelTest) {

		EXPECT_EQ(6765, pfib(20));
		EXPECT_EQ(46368, pfib(24));

	}


	template<unsigned N>
	struct static_fib {
		enum { value = static_fib<N-1>::value + static_fib<N-2>::value };
	};

	template<>
	struct static_fib<1> {
		enum { value = 1 };
	};

	template<>
	struct static_fib<0> {
		enum { value = 0 };
	};

	int sfib(int x) {
		return (x<2) ? x : sfib(x-1) + sfib(x-2);
	}

	static const int N = 40;

	TEST(ScalingTest, StaticFib) {
		// this should not take any time
		EXPECT_LT(0, static_fib<N>::value);
	}

	TEST(ScalingTest, SequentialFib) {
		EXPECT_EQ(static_fib<N>::value, sfib(N));
	}

	TEST(ScalingTest, ParallelFib) {
		EXPECT_EQ(static_fib<N>::value, pfib(N));
	}


} // end namespace parec
