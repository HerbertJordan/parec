#include <gtest/gtest.h>

#include "parec/utils/sequence.h"
#include "parec/utils/string_utils.h"
#include "parec/utils/printer/arrays.h"
#include "parec/ops.h"

namespace parec {

	TEST(Sequence, Basic) {

		auto s = utils::seq(0,10);

		std::cout << s << "\n";

		EXPECT_EQ(10,s.size());

	}

	TEST(Sequence, Pfor) {

		std::array<bool, 10> b;

		pfor(b,[](bool& b) { b = false; });

		pfor(utils::seq(4,8), [&](int x) {
			b[x] = true;
		});

		EXPECT_EQ("[0,0,0,0,1,1,1,1,0,0]", toString(b));
	}

} // end namespace parec
