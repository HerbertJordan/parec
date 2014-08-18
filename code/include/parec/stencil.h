#pragma once

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
					: a(a), b(b), l(l), r(r), t(t) {}
			};

			typedef typename prec_fun<void(const param_type&)>::type solver;

			auto def = group(
					fun(
						[](const param_type& param)->bool {
							return param.r - param.l < 4;
						},
						[&](const param_type& param)->void {

							std::cout << "Base-A: " << param.l << "-" << param.r << " @ " << param.t << "\n";

							if (param.t >= steps) return;

							auto N = param.a->size();
							for(int i=param.l+1; i < param.r; i++) {
								std::cout << "      - Updating: " << i%N  << " - " << param.t << "\n";
								update(param.t,i%N,*param.a,*param.b);
							}
						},
						[](const param_type& param, const solver& up, const solver& down)->void {

							auto l = param.l;
							auto r = param.r;

							auto d = r - l + 1;
							auto a = l + d/4;
							auto hl = l + d/2 - (1-d%2);
							auto hr = l + d/2;
							auto b = l + (3*d)/4;

							auto t = param.t;
							auto th = t + d/4;

							auto odd = (d/2)%2;

							auto vl = a+odd;
							auto vr = b-odd;

							std::cout << "Step-A: " << param.l << "-" << param.r << " @ " << param.t << "\n";
							std::cout << "   - Split: " << l << "-" << hl << " vs. " << hr << "-" << r << "\n";
							std::cout << "   - Cut:   " << a << "-" << b << "\n";

							std::cout << "     - A: " << l << "-" << hl << " / " << t <<  "\n";
							std::cout << "     - A: " << hr << "-" << r << " / " << t << "\n";
							std::cout << "     - V: " << vl << "-" << vr << " / " << t << "\n";
							std::cout << "     - A: " << a << "-" << b << " / " << th << "\n";

//							std::cout << "Step-Up: " << l << "/" << a << "/" << h << "/" << b << "/" << r << " - " << param.t << "/" << th << "\n";

							Container* A = param.a;
							Container* B = param.b;
							Container* M = (d%2) ? B : A;

							parallel(
									up(param_type(A,M,l,hl,t)),
									up(param_type(A,M,hr,r,t))
							);
							down(param_type(A,M,vl,vr,t)).get();
							up(param_type(M,B,a,b,th)).get();
						}
					),
					fun(
						[](const param_type& param)->bool {
							return param.r - param.l < 4;
						},
						[&](const param_type& param)->void {

							std::cout << "Base-V: " << param.l << "-" << param.r << " @ " << param.t << "\n";


							if (param.t >= steps) return;

							auto N = param.a->size();
							for(int i=param.l+1; i < param.r; i++) {
								std::cout << "      - Updating: " << i%N  << " - " << param.t << "\n";
								update(param.t,i%N,*param.a,*param.b);
							}
						},
						[](const param_type& param, const solver& up, const solver& down)->void {
							auto l = param.l;
							auto r = param.r;

							auto d = r - l + 1;
							auto a = l + d/4 + (1-d%2);
							auto hl = l + d/2;
							auto hr = l + d/2 + (1-d%2);
							auto b = l + (3*d)/4;

							auto t = param.t;
							auto th = t + d/2 -1;

							auto odd = (d/2)%2;
							auto al = a+odd;
							auto ar = b-odd;

							std::cout << "Step-V: " << param.l << "-" << param.r << " @ " << param.t << "\n";
							std::cout << "   - Split: " << l << "-" << hl << " vs. " << hr << "-" << r << "\n";
							std::cout << "   - Cut:   " << a << "-" << b << "\n";
//							std::cout << "Step-Do: " << l << "/" << a << "/" << h << "/" << b << "/" << r << " - " << param.t << "/" << th << "\n";


							std::cout << "     - V: " << a << "-" << b << " / " << t <<  "\n";
							std::cout << "     - A: " << al << "-" << ar << " / " << th <<  "\n";
							std::cout << "     - V: " << l << "-" << hl << " / " << th <<  "\n";
							std::cout << "     - V: " << hr << "-" << r << " / " << th <<  "\n";


							Container* A = param.a;
							Container* B = param.b;
							Container* M = (d%2) ? B : A;

							down(param_type(A,M,a,b,t)).get();
							up(param_type(M,B,al,ar,th)).get();
							parallel(
									down(param_type(M,B,l,hl,th)),
									down(param_type(M,B,hr,r,th))
							);
						}
					)
			);

			auto s_u = parec<0>(def);
			auto s_d = parec<1>(def);

			// process layer by layer
			auto N = a.size();
			auto h = N/2;
			auto b = a;
			for(int i=0; i<steps; i+=h) {

				std::cout << " -- step: " << i << "\n";

//				std::cout << " - up - \n";
				s_u(param_type(&a,&b,0,N-1,i)).get();
				std::cout << " - down - \n";
//				s_d(param_type(&a,&b,0,N-1,i)).get();
//				s_d(param_type(&a,&b,h+1,h+N,i)).get();

				// fix result
				if (h%2) a = b;
			}
		}

	} // end namespace detail

} // end namespace parec
