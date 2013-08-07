// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine" and the "irrXML" project.
// For conditions of distribution and use, see copyright notice in irrlicht.h and irrXML.h

// ------------------------------------------------------------------------------------
// Original description: (Schrompf)
// Adapted to the ASSIMP library because the builtin atof indeed takes AGES to parse a
// float inside a large string. Before parsing, it does a strlen on the given point.
// Changes:
//  22nd October 08 (Aramis_acg): Added temporary cast to double, added strtoul10_64
//     to ensure long numbers are handled correctly
// ------------------------------------------------------------------------------------


#ifndef __FAST_A_TO_F_H_INCLUDED__
#define __FAST_A_TO_F_H_INCLUDED__

#include <math.h>
#include <limits.h>

namespace Assimp
{

const float fast_atof_table[16] =	{  // we write [16] here instead of [] to work around a swig bug
	0.f,
	0.1f,
	0.01f,
	0.001f,
	0.0001f,
	0.00001f,
	0.000001f,
	0.0000001f,
	0.00000001f,
	0.000000001f,
	0.0000000001f,
	0.00000000001f,
	0.000000000001f,
	0.0000000000001f,
	0.00000000000001f,
	0.000000000000001f
};


// ------------------------------------------------------------------------------------
// Convert a string in decimal format to a number
// ------------------------------------------------------------------------------------
inline unsigned int strtoul10( const char* in, const char** out=0)
{
	unsigned int value = 0;

	bool running = true;
	while ( running )
	{
		if ( *in < '0' || *in > '9' )
			break;

		value = ( value * 10 ) + ( *in - '0' );
		++in;
	}
	if (out)*out = in;
	return value;
}

// ------------------------------------------------------------------------------------
// Convert a string in octal format to a number
// ------------------------------------------------------------------------------------
inline unsigned int strtoul8( const char* in, const char** out=0)
{
	unsigned int value = 0;

	bool running = true;
	while ( running )
	{
		if ( *in < '0' || *in > '7' )
			break;

		value = ( value << 3 ) + ( *in - '0' );
		++in;
	}
	if (out)*out = in;
	return value;
}

// ------------------------------------------------------------------------------------
// Convert a string in hex format to a number
// ------------------------------------------------------------------------------------
inline unsigned int strtoul16( const char* in, const char** out=0)
{
	unsigned int value = 0;

	bool running = true;
	while ( running )
	{
		if ( *in >= '0' && *in <= '9' )
		{
			value = ( value << 4u ) + ( *in - '0' );
		}
		else if (*in >= 'A' && *in <= 'F')
		{
			value = ( value << 4u ) + ( *in - 'A' ) + 10;
		}
		else if (*in >= 'a' && *in <= 'f')
		{
			value = ( value << 4u ) + ( *in - 'a' ) + 10;
		}
		else break;
		++in;
	}
	if (out)*out = in;
	return value;
}

// ------------------------------------------------------------------------------------
// Convert just one hex digit
// Return value is UINT_MAX if the input character is not a hex digit.
// ------------------------------------------------------------------------------------
inline unsigned int HexDigitToDecimal(char in)
{
	unsigned int out = UINT_MAX;
	if (in >= '0' && in <= '9')
		out = in - '0';

	else if (in >= 'a' && in <= 'f')
		out = 10u + in - 'a';

	else if (in >= 'A' && in <= 'F')
		out = 10u + in - 'A';

	// return value is UINT_MAX if the input is not a hex digit
	return out;
}

// ------------------------------------------------------------------------------------
// Convert a hex-encoded octet (2 characters, i.e. df or 1a).
// ------------------------------------------------------------------------------------
inline uint8_t HexOctetToDecimal(const char* in)
{
	return ((uint8_t)HexDigitToDecimal(in[0])<<4)+(uint8_t)HexDigitToDecimal(in[1]);
}


// ------------------------------------------------------------------------------------
// signed variant of strtoul10
// ------------------------------------------------------------------------------------
inline int strtol10( const char* in, const char** out=0)
{
	bool inv = (*in=='-');
	if (inv || *in=='+')
		++in;

	int value = strtoul10(in,out);
	if (inv) {
		value = -value;
	}
	return value;
}

// ------------------------------------------------------------------------------------
// Parse a C++-like integer literal - hex and oct prefixes.
// 0xNNNN - hex
// 0NNN   - oct
// NNN    - dec
// ------------------------------------------------------------------------------------
inline unsigned int strtoul_cppstyle( const char* in, const char** out=0)
{
	if ('0' == in[0])
	{
		return 'x' == in[1] ? strtoul16(in+2,out) : strtoul8(in+1,out);
	}
	return strtoul10(in, out);
}

// ------------------------------------------------------------------------------------
// Special version of the function, providing higher accuracy and safety
// It is mainly used by fast_atof to prevent ugly and unwanted integer overflows.
// ------------------------------------------------------------------------------------
inline uint64_t strtoul10_64( const char* in, const char** out=0, unsigned int* max_inout=0)
{
	unsigned int cur = 0;
	uint64_t value = 0;

	bool running = true;
	while ( running )
	{
		if ( *in < '0' || *in > '9' )
			break;

		const uint64_t new_value = ( value * 10 ) + ( *in - '0' );
		
		if (new_value < value) /* numeric overflow, we rely on you */
			return value;

		value = new_value;

		++in;
		++cur;

		if (max_inout && *max_inout == cur) {
					
			if (out) { /* skip to end */
				while (*in >= '0' && *in <= '9')
					++in;
				*out = in;
			}

			return value;
		}
	}
	if (out)
		*out = in;

	if (max_inout)
		*max_inout = cur;

	return value;
}

// Number of relevant decimals for floating-point parsing.
#define AI_FAST_ATOF_RELAVANT_DECIMALS 15

// ------------------------------------------------------------------------------------
//! Provides a fast function for converting a string into a float,
//! about 6 times faster than atof in win32.
// If you find any bugs, please send them to me, niko (at) irrlicht3d.org.
// ------------------------------------------------------------------------------------
template <typename Real>
inline const char* fast_atoreal_move( const char* c, Real& out)
{
	Real f;

	bool inv = (*c=='-');
	if (inv || *c=='+') {
		++c;
	}

	f = static_cast<Real>( strtoul10_64 ( c, &c) );
	if (*c == '.' || (c[0] == ',' && c[1] >= '0' && c[1] <= '9')) // allow for commas, too
	{
		++c;

		// NOTE: The original implementation is highly inaccurate here. The precision of a single
		// IEEE 754 float is not high enough, everything behind the 6th digit tends to be more 
		// inaccurate than it would need to be. Casting to double seems to solve the problem.
		// strtol_64 is used to prevent integer overflow.

		// Another fix: this tends to become 0 for long numbers if we don't limit the maximum 
		// number of digits to be read. AI_FAST_ATOF_RELAVANT_DECIMALS can be a value between
		// 1 and 15.
		unsigned int diff = AI_FAST_ATOF_RELAVANT_DECIMALS;
		double pl = static_cast<double>( strtoul10_64 ( c, &c, &diff ));

		pl *= fast_atof_table[diff];
		f += static_cast<Real>( pl );
	}

	// A major 'E' must be allowed. Necessary for proper reading of some DXF files.
	// Thanks to Zhao Lei to point out that this if() must be outside the if (*c == '.' ..)
	if (*c == 'e' || *c == 'E')	{

		++c;
		const bool einv = (*c=='-');
		if (einv || *c=='+') {
			++c;
		}

		// The reason float constants are used here is that we've seen cases where compilers
		// would perform such casts on compile-time constants at runtime, which would be
		// bad considering how frequently fast_atoreal_move<float> is called in Assimp.
		Real exp = static_cast<Real>( strtoul10_64(c, &c) );
		if (einv) {
			exp = -exp;
		}
		f *= pow(static_cast<Real>(10.0f), exp);
	}

	if (inv) {
		f = -f;
	}
	out = f;
	return c;
}

// ------------------------------------------------------------------------------------
// The same but more human.
inline float fast_atof(const char* c)
{
	float ret;
	fast_atoreal_move<float>(c, ret);
	return ret;
}


inline float fast_atof( const char* c, const char** cout)
{
	float ret;
	*cout = fast_atoreal_move<float>(c, ret);

	return ret;
}

inline float fast_atof( const char** inout)
{
	float ret;
	*inout = fast_atoreal_move<float>(*inout, ret);

	return ret;
}


inline double fast_atod(const char* c)
{
	double ret;
	fast_atoreal_move<double>(c, ret);
	return ret;
}


inline double fast_atod( const char* c, const char** cout)
{
	double ret;
	*cout = fast_atoreal_move<double>(c, ret);

	return ret;
}

inline double fast_atod( const char** inout)
{
	double ret;
	*inout = fast_atoreal_move<double>(*inout, ret);

	return ret;
}

} // end of namespace Assimp

#endif

