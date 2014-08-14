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

	TEST(RecOps, Reduce) {

		auto plus = [](int a, int b) { return a + b; };

		std::vector<int> v = { 1, 2, 3, 4, 5 };
		EXPECT_EQ(15, preduce(v, plus));

		std::vector<int> e = { };
		EXPECT_EQ(0, preduce(e, plus));
	}

} // end namespace parec
