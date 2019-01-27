#pragma once

#include <complex>

typedef std::complex<float>                     gr_complex;
typedef std::complex<double>                    gr_complexd;

inline bool is_complex(gr_complex x) { (void)x; return true; }
inline bool is_complex(gr_complexd x) { (void)x; return true; }
inline bool is_complex(float x) { (void)x; return false; }
inline bool is_complex(double x) { (void)x; return false; }
inline bool is_complex(int x) { (void)x; return false; }
inline bool is_complex(char x) { (void)x; return false; }
inline bool is_complex(short x) { (void)x; return false; }

// this doesn't really belong here, but there are worse places for it...

#define CPPUNIT_ASSERT_COMPLEXES_EQUAL(expected, actual, delta)                   \
    CPPUNIT_ASSERT_DOUBLES_EQUAL (expected.real(), actual.real(), delta); \
    CPPUNIT_ASSERT_DOUBLES_EQUAL (expected.imag(), actual.imag(), delta);