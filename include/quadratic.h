#ifndef QUADRATIC_H
#define QUADRATIC_H

#include <ostream>
#include <iostream>

//====================================================
//  Floating point constants and utility functions
//====================================================
template <typename T>
const T NaN = std::numeric_limits<T>::quiet_NaN();

template <typename T>
const T inf = std::numeric_limits<T>::infinity();

template <typename T>
const T eps = std::numeric_limits<T>::epsilon();

// Maximum exponent
template <typename T>
const int E_MIN = std::numeric_limits<T>::min_exponent - 1;

// Minimum exponent
template <typename T>
const int E_MAX = std::numeric_limits<T>::max_exponent - 1;

// Number of bits in mantissa
template <typename T>
const int NUM_DIGITS = std::numeric_limits<T>::digits;

// max val of Ea + Ea - 2 Eb to avoid overflow
template<typename T>
const int ECP_MAX = E_MAX<T> - 2 - NUM_DIGITS<T> / 2;

// min val of Ea + Ea - 2 Eb to avoid underflow
template<typename T>
const int ECP_MIN = E_MIN<T> + 2 * NUM_DIGITS<T> - 4;

// Utility functions
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

// Return a pair (Kout, K2) of exponents such that neither is outside [E_MIN, E_MAX]
// with Kout = K2 - Kin
// float32:  E_MIN = -126,   E_MAX = 127
// float64:  E_MIN = -1022,  E_MAX = 1023
// float128: E_MIN = -16382, E_MAX = 16383
template <typename T>
inline int keep_exponent_in_check(int Kin) { return std::clamp(Kin, E_MIN<T>, E_MAX<T>); }

// Accurate computation of the discriminant with Kahan's method, with an fma instruction
template <typename T>
T kahan_discriminant_fma(T a, T b, T c) {
    auto p = b*b;
    auto q = 4*a*c;
    auto d = p - q;
    if (3 * std::abs(d) >= p + 4*q) {
        // If b^2 and 4ac are different enough
        return d;
    }
    auto dp = std::fma(b,b,-p);
    auto dq = std::fma(4*a,c,-q);
    d = (p - q) + (dp - dq);
    return d;
}

// Algorithm based on "The Ins and Outs of Solving Quadratic Equations with Floating-Point Arithmetic (Goualard 2023)"
template <typename T>
std::pair<T, T> solve_quadratic(T a, T b, T c) {

    const std::pair<T,T> NO_SOLUTIONS = std::make_pair(NaN<T>, NaN<T>);

    // Degenerate case 1: any of the parameters are nan or inf
    if (std::isnan(a) || std::isnan(b) || std::isnan(c) || 
        std::isinf(a) || std::isinf(b) || std::isinf(c)) {
        return NO_SOLUTIONS;
    }

    // More degenerate cases
    if (a == 0) {
        if (b == 0) {
            // if a and b are zero, then the quadratic is invalid
            // if c == 0, then all real numbers are solutions, i.e. 0 == 0,
            // and if c /= 0, then no real numbers are solutions, i.e. c == 0
            return NO_SOLUTIONS;
        } else {
            if (c == 0) {
                // If a and c are zero, but b is nonzero, then we have bx = 0, so only
                // one real solution (i.e. x = 0) exists
                return std::make_pair(T(0.), NaN<T>);
            } else {
                // If a == 0 but b and c are nonzero, then we have bx + c = 0, which is
                // a linear equation with solution x = -c / b
                return std::make_pair(-c / b, NaN<T>);
            }
        }
    } else {
        if (b == 0) {
            if (c == 0) {
                // If a /= 0, but b and c == 0, then we have ax^2 = 0, implying x = 0;
                return std::make_pair(T(0), NaN<T>);
            } else {
                // If a and c /= 0, but b == 0. then we have x^2 = -c/a, which has either no
                // real solutions if sign(a) = sign(c), or two real solutions otherwise
                if (sgn(a) == sgn(c)) {
                    return NO_SOLUTIONS;
                } else {
                    // Split a and c into significant and exponent parts
                    int exp_a, exp_c;
                    auto signif_a = frexp(a, &exp_a);
                    auto signif_c = frexp(c, &exp_c);

                    int ecp = exp_c - exp_a;

                    int dM = ecp & ~1;  // dM = floor(ecp/2) * 2
                    int M = dM >> 1;    // M = dM / 2
                    int E = ecp & 1;    // E = odd(ecp) ? 1 : 0
                    auto S = sqrt(-signif_c * exp2(E) / signif_a);

                    auto M1 = keep_exponent_in_check<T>(M);
                    auto M2 = M - M1;
                    auto x = S * exp2(M1) * exp2(M2);
                    auto x1 = -x;
                    auto x2 = x;
                    return std::make_pair(x1, x2);
                }   
            }
        } else {
            if (c == 0) {
                T x1, x2;
                // a /= 0, b /= 0, c /= 0
                if (sgn(a) == sgn(b)) {
                    x1 = -b / a; x2 = 0.0;
                } else {
                    x1 = 0.0; x2 = -b / a;
                }
                return std::make_pair(x1, x2);
            } else {
                // a, b, and c all nonzero

                // Split a, b, and c into significand and exponent
                int exp_a, exp_b, exp_c;
                auto signif_a = frexp(a, &exp_a);
                auto signif_b = frexp(b, &exp_b);
                auto signif_c = frexp(c, &exp_c);
                
                auto K = exp_b - exp_a;
                auto ecp = exp_c + exp_a - 2 * exp_b;

                if (ecp >= ECP_MIN<T> && ecp < ECP_MAX<T>) {
                    auto c2 = signif_c * exp2(ecp);
                    auto delta = kahan_discriminant_fma(signif_a, signif_b, c2);
                    if (delta < 0) {
                        return NO_SOLUTIONS;
                    }
                    auto K1 = keep_exponent_in_check<T>(K);
                    auto K2 = K - K1;
                    if (delta > 0) {
                        auto B = signif_b + sgn(b) * sqrt(delta);
                        auto y1 = -(2 * c2) / B;
                        auto y2 = -B / (2 * signif_a);
                        auto _x1 = y1 * exp2(K1) * exp2(K2);
                        auto _x2 = y2 * exp2(K1) * exp2(K2);
                        auto x1 = std::min(_x1, _x2);
                        auto x2 = std::max(_x1, _x2);
                        return std::make_pair(x1, x2);
                    }
                    // delta == 0
                    auto x1 = -(signif_b / (2 * signif_a)) * exp2(K1) * exp2(K2);
                    return std::make_pair(x1, NaN<T>);
                }

                int dM = ecp & ~1;  // dM = floor(ecp/2) * 2
                int M = dM >> 1;    // M = dM / 2
                int E = ecp & 1;    // E = odd(ecp) ? 1 : 0
                auto c3 = signif_c * exp2(E);
                auto S = sqrt(abs(c3 / signif_a));

                if (ecp < ECP_MIN<T>) {
                    auto y1 = -signif_b / signif_a;
                    auto y2 = c3 / (signif_a * y1);
                    auto dMK = dM + K;
                    auto dMK1 = keep_exponent_in_check<T>(dMK);
                    auto dMK2 = dMK - dMK1;
                    auto K1 = keep_exponent_in_check<T>(K);
                    auto K2 = K - K1;

                    auto _x1 = y1 * exp2(K1) * exp2(K2);
                    auto _x2 = y2 * exp2(dMK1) * exp2(dMK2);
                    auto x1 = std::min(_x1, _x2);
                    auto x2 = std::max(_x1, _x2);
                    return std::make_pair(x1, x2);
                }

                // ecp >= ECP_MAX<T>
                if (sgn(a) == sgn(c)) {
                    return NO_SOLUTIONS;
                } else {
                    auto MK = M + K;
                    auto MK1 = keep_exponent_in_check<T>(MK);
                    auto MK2 = MK - MK1;
                    auto x2 = S * exp2(MK1) * exp2(MK2);
                    auto x1 = -x2;
                    return std::make_pair(x1, x2);
                }
            }                    
        }
    }
}

#endif