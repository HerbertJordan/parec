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

//		std::cout << A << "\n";

		// run a stencil
		stencil(A, 200, [](int , int i, const Field& A, Field& B) {
			// get size of field
			int N = std::tuple_size<Field>::value;

			// handle boundaries (alternative)
//			if (i == 0 || i == N-1) return;

			// handle rest
			B[i] = (A[(i+N-1)%N] + A[i] + A[(i+1) % N]) / 3;
		});

//		std::cout << A << "\n";


	}


	namespace {

		template<unsigned size, unsigned steps = 3*size + 4>
		void run_rec_test() {

			typedef array<int,size> Field;

			// create an array
			Field A;

			// init the array
			pfor(A.begin(), A.end(), [](int& a) { a = 0; });

			// create 3 copies
			Field As = A;
			Field Ai = A;
			Field Ar = A;

			auto update = [](int t, int i, const Field& A, Field& B) {
				EXPECT_EQ(t,A[i]) << "Time step " << t << " field " << i;
				B[i] = A[i]+1;
			};

//			auto update = [](int, int i, const Field& A, Field& B) {
//				B[i] = A[i]+1;
//			};

			auto t = steps;

			// apply different stencils (todo: in parallel)
			detail::stencil_seq(As, t, update);
			detail::stencil_iter(Ai, t, update);
			detail::stencil_rec(Ar, t, update);

			EXPECT_EQ(As, Ai);
			EXPECT_EQ(As, Ar);
		}

	}

	TEST(Sets, Working) {
		run_rec_test<0>();
		run_rec_test<1>();
		run_rec_test<2>();
		run_rec_test<3>();
		run_rec_test<4>();
		run_rec_test<5>();
		run_rec_test<6>();
		run_rec_test<7>();
		run_rec_test<8>();
		run_rec_test<9>();
		run_rec_test<10>();
		run_rec_test<11>();
		run_rec_test<12>();
		run_rec_test<13>();
		run_rec_test<14>();
		run_rec_test<15>();
	}

	TEST(Sets, Large) {
		run_rec_test<500>();
	}

} // end namespace parec
