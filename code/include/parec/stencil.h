#pragma once

#include <cassert>
#include <iostream>
#include <utility>

#include "parec/core.h"
#include "parec/futures.h"

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

			for(int t=0; t<steps; t++) {

				for(int i=0; i<a.size(); i++) {
					update(t,i,*x,*y);
				}

//				std::cout << *y << "\n";

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

			for(int t=0; t<steps; t++) {

				// conduct update step in parallel
				pfor(utils::seq(0ul,a.size()),[&](int i){
					update(t,i,*x,*y);
				});

//				std::cout << *y << "\n";

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
		void stencil_rec(Container& a, int steps, const Stencil& update) {

			struct param_type {
				Container* a;		// source field
				Container* b;		// target field
				int l;				// left boundary
				int r;				// right boundary
				int t;				// base time

				param_type(Container* a, Container* b, int l, int r, int t)
					: a(a), b(b), l(l), r(r), t(t) { }
			};

			typedef typename prec_fun<void(const param_type&)>::type solver;

			auto test = [](const param_type& param)->bool {
				return param.r - param.l < 2;
			};

			auto base = [&](const param_type& param)->void {
				auto N = param.a->size();

				auto t = param.t;

				if (t > steps) return;

				auto a = (t%2) ? param.a : param.b;
				auto b = (t%2) ? param.b : param.a;

				for(int i=param.l; i<= param.r; i++) {
					update(param.t,i%N,*a,*b);
//					std::cout << "  - Update: " << i%N << " @ " << param.t << "  from " << a << "=>" << b << " ... "<< (*a)[i%N] << " to " << (*b)[i%N] << "\n";
				}
			};

			auto def = group(
					fun(
						test, base,
						[](const param_type& param, const solver& up, const solver& down)->void {

							auto l = param.l;
							auto r = param.r;

							auto d = r - l + 1;
							auto hl = l + d/2 - 1;
							auto hr = r - d/2 + 1;

							auto al = l + d/4 + 1;
							auto ar = r - d/4 - 1;

							auto bl = l + d/4 + ((d/2)%2);
							auto br = r - d/4 - ((d/2)%2);

							auto t = param.t;
							auto ta = t + d/4 - 1;
							auto tb = ta + 1;

//							std::cout << "Step-A: " << param.l << "-" << param.r << " @ " << param.t << "\n";
//							std::cout << "     - A: " << l << "-" << hl << " / " << t <<  "\n";
//							std::cout << "     - A: " << hr << "-" << r << " / " << t << "\n";
//							std::cout << "     - V: " << al << "-" << ar << " / " << ta << "\n";
//							std::cout << "     - A: " << bl << "-" << br << " / " << tb << "\n";

//							std::cout << "Step-Up: " << l << "/" << a << "/" << h << "/" << b << "/" << r << " - " << param.t << "/" << th << "\n";

							Container* A = param.a;
							Container* B = param.b;

							parallel(
									up(param_type(A,B,l,hl,t)),
									up(param_type(A,B,hr,r,t))
							);
							down(param_type(A,B,al,ar,ta)).get();
							up(param_type(A,B,bl,br,tb)).get();
						}
					),
					fun(
						test, base,
						[](const param_type& param, const solver& up, const solver& down)->void {

							auto l = param.l;
							auto r = param.r;

							auto d = r - l + 1;
							auto hl = l + d/2 - 1;
							auto hr = r - d/2 + 1;

							auto al = l + d/4 + 1;
							auto ar = r - d/4 - 1;

							auto bl = l + d/4 + ((d/2)%2);
							auto br = r - d/4 - ((d/2)%2);

							auto t = param.t;
							auto ta = t - d/4 ;
							auto tb = ta + 1;

//							std::cout << "Step-V: " << param.l << "-" << param.r << " @ " << param.t << "\n";
//							std::cout << "     - V: " << bl << "-" << br << " / " << ta <<  "\n";
//							std::cout << "     - A: " << al << "-" << ar << " / " << tb <<  "\n";
//							std::cout << "     - V: " << l << "-" << hl << " / " << t <<  "\n";
//							std::cout << "     - V: " << hr << "-" << r << " / " << t <<  "\n";


							Container* A = param.a;
							Container* B = param.b;

							down(param_type(A,B,bl,br,ta)).get();
							up(param_type(A,B,al,ar,tb)).get();
							parallel(
									down(param_type(A,B,l,hl,t)),
									down(param_type(A,B,hr,r,t))
							);
						}
					)
			);

			auto s_u = parec<0>(def);
			auto s_d = parec<1>(def);

			// process layer by layer
			auto N = a.size();
			auto h = N/2 - 1;
			auto b = a;
			for(int i=0; i<=steps; i+=h) {

//				std::cout << " -- step: " << i << "\n";
//				std::cout << " - up - \n";
				s_u(param_type(&a,&b,1,N-2,i+1)).get();
//				std::cout << " - down - \n";
				s_d(param_type(&a,&b,h+2,h+N-1,i+h)).get();

			}

			// fix result
			if (steps%2) a = b;
		}

	} // end namespace detail

} // end namespace parec
