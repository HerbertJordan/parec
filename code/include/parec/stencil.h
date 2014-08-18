#pragma once

#include <iostream>
#include <utility>

#include "parec/core.h"
#include "parec/utils/sequence.h"

namespace parec {

	namespace detail {

		template<typename Container, typename Stencil>
		void stencil_seq(Container& a, int steps, const Stencil& update);

		template<typename Container, typename Stencil>
		void stencil_iter(Container& a, int steps, const Stencil& update);

		template<typename Container, typename Stencil>
		void stencil_rec(Container& a, int steps, const Stencil& update);

	}

	template<
		typename Container,
		typename Stencil
	>
	void stencil(Container& a, int steps, const Stencil& update) {
//		detail::stencil_seq(a,steps,update);
		detail::stencil_iter(a,steps,update);
//		detail::stencil_rec(a,steps,update);
	}


	namespace detail {

		template<
			typename Container,
			typename Stencil
		>
		void stencil_seq(Container& a, int steps, const Stencil& update) {

			Container b = a;

			Container* x = &a;
			Container* y = &b;

			for(int t=0; t!=steps; t++) {

				for(int i=0; i<a.size(); i++) {
					update(t,i,*x,*y);
				}

				std::cout << *y << "\n";

				// switch buffers
				auto h = x;
				x = y;
				y = h;
			}

			// fix result in a
			if (x != &a) {
				a = *x;
			}
		}

		template<
			typename Container,
			typename Stencil
		>
		void stencil_iter(Container& a, int steps, const Stencil& update) {

			Container b = a;

			Container* x = &a;
			Container* y = &b;

			for(int t=0; t!=steps; t++) {

				// conduct update step in parallel
				pfor(utils::seq(0ul,a.size()),[&](int i){
					update(t,i,*x,*y);
				});

				std::cout << *y << "\n";

				// switch buffers
				auto h = x;
				x = y;
				y = h;
			}

			// fix result in a
			if (x != &a) {
				a = *x;
			}
		}

	} // end namespace detail

} // end namespace parec
