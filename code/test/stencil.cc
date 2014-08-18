#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "parec/ops.h"
#include "parec/stencil.h"
#include "parec/utils/print_utils.h"

namespace parec {

	using std::array;
	using std::vector;

	TEST(Sets, Basic) {

		typedef array<float,21> Field;

		// create an array
		Field A;

		// init the array
		pfor(A.begin(), A.end(), [](float& a) { a = 0; });
		A[10] = 5;

		std::cout << A << "\n";

		// run a stencil
		stencil(A, 200, [](int , int i, const Field& A, Field& B) {
			// get size of field
			int N = std::tuple_size<Field>::value;

			// handle boundaries (alternative)
//			if (i == 0 || i == N-1) return;

			// handle rest
			B[i] = (A[(i+N-1)%N] + A[i] + A[(i+1) % N]) / 3;
		});

		std::cout << A << "\n";


	}

	TEST(Sets, Comparison) {

		typedef array<float,10> Field;

		// create an array
		Field A;

		// init the array
		pfor(A.begin(), A.end(), [](float& a) { a = 0; });
		A[10] = 5;

		// create 3 copies
		Field As = A;
		Field Ai = A;
		Field Ar = A;

//		auto update = [](int , int i, const Field& A, Field& B) {
//			// get size of field
//			int N = std::tuple_size<Field>::value;
//
//			// handle rest
//			B[i] = (A[(i+N-1)%N] + A[i] + A[(i+1) % N]) / 3;
//		};

//		auto update = [](int t, int i, const Field& , Field& B) {
//			B[i] = t;
//		};

		auto update = [](int, int i, const Field& A, Field& B) {
			B[i] = A[i]+1;
		};

		auto t = std::tuple_size<Field>::value/2;

		// apply different stencils (todo: in parallel)
		detail::stencil_seq(As, t, update);
		detail::stencil_iter(Ai, t, update);
		detail::stencil_rec(Ar, t, update);

		EXPECT_EQ(As, Ai);
		EXPECT_EQ(As, Ar);
	}

} // end namespace parec
