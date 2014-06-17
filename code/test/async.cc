#include <gtest/gtest.h>

#include <cstdlib>
#include "parec/async.h"

namespace parec {

	TEST(Async, Basic) {

		int x = 0;
		auto f = async([&]()->int {
			x = 1;
			return x+2;
		});

		EXPECT_EQ(3,f.get());
		EXPECT_EQ(1,x);
	}

	TEST(Async, AnyAll) {

		EXPECT_TRUE(all(
				async([]() { return true; }),
				async([]() { return true; }),
				async([]() { return true; })
		));

		EXPECT_FALSE(all(
				async([]() { return true; }),
				async([]() { return false; }),
				async([]() { return true; })
		));

		EXPECT_TRUE(any(
				async([]() { return true; }),
				async([]() { return false; }),
				async([]() { return true; })
		));

		EXPECT_FALSE(any(
				async([]() { return false; }),
				async([]() { return false; }),
				async([]() { return false; })
		));

	}

} // end namespace parec
