#include <gtest/gtest.h>

#include <cstdlib>
#include "parec/set.h"

namespace parec {

	TEST(Sets, Basic) {

		set<int> a;
		EXPECT_TRUE(a.empty());
		EXPECT_EQ(0, a.size());

		set<int> b(12);
		EXPECT_FALSE(b.empty());
		EXPECT_EQ(1, b.size());

		set<int> c = b;
		EXPECT_FALSE(c.empty());
		EXPECT_EQ(1, c.size());

		EXPECT_FALSE(a.contains(1));
		EXPECT_FALSE(a.contains(12));

		EXPECT_FALSE(b.contains(10));
		EXPECT_TRUE(b.contains(12));

		EXPECT_FALSE(c.contains(10));
		EXPECT_TRUE(c.contains(12));

	}

	TEST(Sets, Insert) {

		set<int> a;
		EXPECT_TRUE(a.empty());
		EXPECT_EQ(0, a.size());

		set<int> b(12);
		EXPECT_FALSE(b.empty());
		EXPECT_EQ(1, b.size());


		// test insertion
		EXPECT_EQ(0, a.size());
		EXPECT_FALSE(a.contains(12));
		EXPECT_FALSE(a.contains(14));

		a.insert(b);
		EXPECT_EQ(1, a.size());
		EXPECT_TRUE(a.contains(12));
		EXPECT_FALSE(a.contains(14));

		a.insert(14);
		EXPECT_EQ(2, a.size());
		EXPECT_TRUE(a.contains(12));
		EXPECT_TRUE(a.contains(14));


		set<int> c;
		c.insert(a);
		EXPECT_EQ(2, c.size());
		EXPECT_TRUE(c.contains(12));
		EXPECT_TRUE(c.contains(14));

	}

	TEST(Sets, Subsets) {

		set<int> a;
		a.insert(4,2,8,3,8,7,4);

		set<int> b;
		b.insert(4,3,7);

		set<int> c;
		c.insert(5,3,8);

		set<int> e;

		EXPECT_TRUE(b.isSubsetOf(a));
		EXPECT_FALSE(c.isSubsetOf(a));

		EXPECT_TRUE(e.isSubsetOf(a));
		EXPECT_TRUE(e.isSubsetOf(b));
		EXPECT_TRUE(e.isSubsetOf(c));

	}

	TEST(Sets, Equality) {

		set<int> a;
		a.insert(4,2,8,3,8,7,4);

		set<int> b;
		b.insert(4,4,4,4,4,8,7);

		set<int> c = b;
		c.insert(a);

		EXPECT_EQ(a,a);
		EXPECT_NE(a,b);
		EXPECT_EQ(a,c);

		EXPECT_EQ(b,b);
		EXPECT_NE(b,c);

		EXPECT_EQ(c,c);

	}


	TEST(Sets, LargerSets) {
		const int N = 100;

		set<int> a;
		for(int i=0; i<2*N; i++) {
			a.insert(rand() % N);
		}

		EXPECT_LE(a.size(), N);
//		std::cout << a.size() << "\n";

	}

} // end namespace parec
