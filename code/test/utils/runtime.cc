#include <gtest/gtest.h>

#include <vector>
#include "parec/utils/runtime/runtime.h"

namespace parec {
namespace util {

	TEST(Runtime, Basic) {

		int i = 0;

		runtime::spawn([&](){ i++; }).get();
		EXPECT_EQ(1,i);

		runtime::spawn([&](){ i++; }).get();
		EXPECT_EQ(2,i);

		auto f = runtime::spawn([&]() { return i; });
		EXPECT_EQ(2, f.get());

	}

	TEST(Runtime, Stress) {

		int c = 0;
		int N = 1000;

		std::vector<runtime::Future<void>> list;
		for(int i=0; i<N; i++) {
			list.push_back(runtime::spawn([&]() {
				c++;
			}));
		}

		for(const auto& cur : list) {
			cur.get();
		}

		EXPECT_EQ(N,c);

	}


} // end namespace util
} // end namespace parec
