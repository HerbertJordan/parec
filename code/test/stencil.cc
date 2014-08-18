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

} // end namespace parec
