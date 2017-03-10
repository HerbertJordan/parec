#include <gtest/gtest.h>

#include "parec/ops.h"

#define size 300

TEST(Pfor, Simple) {

	std::array<std::vector<float>, size> a, b, c;

	for(unsigned i = 0; i < size; ++i) {
		a[i] = std::vector<float>(size);
		b[i] = std::vector<float>(size);
		c[i] = std::vector<float>(size);
		for(unsigned j = 0; j < size; ++j) {
			a[i][j] = i;
			b[i][j] = size * size / (j+0.3);
			c[i][j] = i;
		}
	}

	for(unsigned i = 0; i < size; ++i) {
		std::vector<float> sum;
		for(unsigned k = 0; k < size; ++k) {
			sum.push_back(0.0f);
			for(unsigned j = 0; j < size; ++j) {
				sum.back() += c[i][j] * b[j][k];
			}
		}
		c[i] = sum;
	}


	parec::pfor(a, [&](std::vector<float>& row){
		std::vector<float> sum;
		for(unsigned k = 0; k < size; ++k) {
			sum.push_back(0.0f);
			for(unsigned j = 0; j < size; ++j) {
				sum.back() += row[j] * b[j][k];
			}
		}
		row = sum;
	});

	for(unsigned i = 0; i < size; ++i) {
		for(unsigned j = 0; j < size; ++j) {
			ASSERT_EQ(a[i][j], c[i][j]);
		}
	}
}
