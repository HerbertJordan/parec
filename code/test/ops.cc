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

	TEST(RecOps, PforInts) {

		std::atomic<int> counter;
		counter = 0;

		bool map[20];
		for(int i=0; i<20; i++) {
			map[i] = false;
		}

		pfor(5,10,[&](int a) {
			map[a] = true;
			++counter;
		});

		// check correct number of calls
		EXPECT_EQ(10-5, counter);

		// check correct arguments
		for(int i=0; i<20; i++) {
			EXPECT_EQ(5 <= i && i <10, map[i]);
		}

	}

	TEST(RecOps, PforArea) {

		using coord = std::array<int,3>;

		coord a = {{ 10, 20, 30 }};
		coord b = {{ 12, 23, 34 }};

		std::atomic<int> counter;
		counter = 0;

		bool map[50][50][50];
		for(int i=0; i<50; i++) {
			for(int j=0; j<50; j++) {
				for(int k=0; k<50; k++) {
					map[i][j][k] = false;
				}
			}
		}

		pfor(a,b,[&](const coord& a) {
			map[a[0]][a[1]][a[2]] = true;
			++counter;
		});

		// check correct number of calls
		EXPECT_EQ(2*3*4,counter);

		// check coordinates
		for(int i=0; i<50; i++) {
			for(int j=0; j<50; j++) {
				for(int k=0; k<50; k++) {
					bool in = true;
					in = in && 10 <= i && i < 12;
					in = in && 20 <= j && j < 23;
					in = in && 30 <= k && k < 34;
					EXPECT_EQ(in, map[i][j][k]);
				}
			}
		}

	}


	TEST(RecOps, PforPerformance) {

		static const int N = 1000000;
//		static const int N = 100;
		using Vector = std::array<int,N>;

		Vector r;

		for(int i=0; i<100; i++) {
			r[i] = 2*i+1;
		}

		pfor(r, [](int& a) {
			int s = 0;
			for(int i=0; i<32; i++) {
				if (a>>i % 2) s++;
				if (a>>i % 3) s++;
			}
			a = s;
		});

	}

	TEST(RecOps, PforPolicies1D) {

		const int N = 100;

		std::array<int,N> data;

		// init array
		pfor(0,N, [&](int i){
			data[i] = 0;
		});

		for(int i=0;i<N;i++) {
			EXPECT_EQ(0,data[i]) << "i=" << i;
		}

		pfor<loop_policy::binary_split>(0,N, [&](int i){
			data[i]++;
		});

		for(int i=0;i<N;i++) {
			EXPECT_EQ(1,data[i]) << "i=" << i;
		}

		pfor<loop_policy::queue>(0,N, [&](int i){
			data[i]++;
		});

		for(int i=0;i<N;i++) {
			EXPECT_EQ(2,data[i]) << "i=" << i;
		}
	}

	TEST(RecOps, PforPolicies2D) {

		const int N = 100;
		const int M = 100;

		std::array<std::array<int,M>,N> data;

		using coord = std::array<int,2>;

		coord a = {{ 0, 0 }};
		coord b = {{ N, M }};

		// init array
		pfor(a,b, [&](const coord& i){
			data[i[0]][i[1]] = 0;
		});

		for(int i=0;i<N;i++) {
			for(int j=0;j<M;j++) {
				EXPECT_EQ(0,data[i][j]) << "i=" << i << "\nj=" << j;
			}
		}

		pfor<loop_policy::binary_split>(a,b, [&](const coord& i){
			data[i[0]][i[1]]++;
		});

		for(int i=0;i<N;i++) {
			for(int j=0;j<M;j++) {
				EXPECT_EQ(1,data[i][j]) << "i=" << i << "\nj=" << j;
			}
		}

		pfor<loop_policy::queue>(a,b, [&](const coord& i){
			data[i[0]][i[1]]++;
		});

		for(int i=0;i<N;i++) {
			for(int j=0;j<M;j++) {
				EXPECT_EQ(2,data[i][j]) << "i=" << i << "\nj=" << j;
			}
		}
	}


	TEST(RecOps, Reduce) {

		auto plus = [](int a, int b) { return a + b; };

		std::vector<int> v = { 1, 2, 3, 4, 5 };
		EXPECT_EQ(15, preduce(v, plus));

		std::vector<int> e = { };
		EXPECT_EQ(0, preduce(e, plus));
	}


} // end namespace parec
