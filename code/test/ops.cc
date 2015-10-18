#include <gtest/gtest.h>

#include "parec/ops.h"

namespace parec {

	TEST(RecOps, Pfor) {

		std::vector<int> v = { 1, 2, 3, 4, 5 };

		pfor(v, [](int x) {
			std::cout << x << "\n";
		});

		std::vector<int> e = { };
		pfor(e, [](int x) {
			std::cout << x << "\n";
		});
	}

	TEST(RecOps, PforPerformance) {

//		static const int N = 1000000;
		static const int N = 100;
		using Vector = std::array<int,N>;

		Vector r;

		pfor(r, [](int& a) {
			int s = 0;
			for(int i=0; i<32; i++) {
				if (a>>i % 2) s++;
				if (a>>i % 3) s++;
			}
			a = s;
		});

	}

	TEST(RecOps, Reduce) {

		auto plus = [](int a, int b) { return a + b; };

		std::vector<int> v = { 1, 2, 3, 4, 5 };
		EXPECT_EQ(15, preduce(v, plus));

		std::vector<int> e = { };
		EXPECT_EQ(0, preduce(e, plus));
	}


} // end namespace parec
