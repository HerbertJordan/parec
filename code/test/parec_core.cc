#include <gtest/gtest.h>

#include <sstream>

#include "parec/parec_core.h"

namespace parec {

	int fib(int x) {
		return rec<int,int>(
				x,
				[](int x) { return x < 2; },
				[](int x) { return x; },
				[](int x, const typename rec_fun<int(int)>::type& f) { return f(x-1) + f(x-2); },
				[](int x, const typename rec_fun<int(int)>::type& f) { return f(x-2) + f(x-1); }
		);
	}

	int fac(int x) {
		return rec<int,int>(
				x,
				[](int x) { return x < 2; },
				[](int) { return 1; },
				[](int x, const typename rec_fun<int(int)>::type& f) { return x * f(x-1); }
		);
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


	int pfib(int x) {
		return prec<int,int>(
				x,
				[](int x) { return x < 2; },
				[](int x) { return x; },
				[](int x, const typename prec_fun<int(int)>::type& f)->int {
					auto a = f(x-1);
					auto b = f(x-2);
					return a.get() + b.get();
				}
		).get();
	}


	TEST(RecOps, ParallelTest) {

		EXPECT_EQ(6765, pfib(20));

	}


	TEST(PickRandom, SimpleTest) {

		std::vector<int> data;
		std::srand(1);
		for(int i =0; i<20; i++) {
			data.push_back(detail::pickRandom(1,2,3,4,5));
		}

		EXPECT_EQ(std::vector<int>({5,2,3,4,5,2,2,2,2,2,1,2,4,5,5,3,4,3,1,4}), data);

	}

} // end namespace parec
