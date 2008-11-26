// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine" and the "irrXML" project.
// For conditions of distribution and use, see copyright notice in irrlicht.h and irrXML.h

// ------------------------------------------------------------------------------------
// Original description: (Schrompf)
// Adapted to the ASSIMP library because the builtin atof indeed takes AGES to parse a
// float inside a large string. Before parsing, it does a strlen on the given point.
// Changes:
//  22nd October 08 (Aramis_acg): Added temporary cast to double, added strtol10_64
//     to ensure long numbers are handled correctly
// ------------------------------------------------------------------------------------


#ifndef __FAST_A_TO_F_H_INCLUDED__
#define __FAST_A_TO_F_H_INCLUDED__

#include <math.h>

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
// convert a string in decimal format to a number
// ------------------------------------------------------------------------------------
inline unsigned int strtol10( const char* in, const char** out=0)
{
	unsigned int value = 0;

	while ( 1 )
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
// convert a string in octal format to a number
// ------------------------------------------------------------------------------------
inline unsigned int strtol8( const char* in, const char** out=0)
{
	unsigned int value = 0;

	while ( 1 )
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
// convert a string in hex format to a number
// ------------------------------------------------------------------------------------
inline unsigned int strtol16( const char* in, const char** out=0)
{
	unsigned int value = 0;

	while ( 1 )
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
// convert just one hex digit
// ------------------------------------------------------------------------------------
inline unsigned int HexDigitToDecimal(char in)
{
	unsigned int out = 0xffffffff;
	if (in >= '0' && in <= '9')
		out = in - '0';

	else if (in >= 'a' && in <= 'f')
		out = 10u + in - 'a';

	else if (in >= 'A' && in <= 'F')
		out = 10u + in - 'A';

	// return value is 0xffffffff if the input is not a hex digit
	return out;
}

// ------------------------------------------------------------------------------------
// signed variant of strtol10
// ------------------------------------------------------------------------------------
inline int strtol10s( const char* in, const char** out=0)
{
	bool bNeg = false;
	if ('-' == *in){++in;bNeg = true;}
	if ('+' == *in)++in;
	int value = strtol10(in,out);
	if (bNeg)value = -value;
	return value;
}

// ------------------------------------------------------------------------------------
// Parse a C++-like integer literal - hex and oct prefixes.
// 0xNNNN - hex
// 0NNN   - oct
// NNN    - dec
// ------------------------------------------------------------------------------------
inline unsigned int strtol_cppstyle( const char* in, const char** out=0)
{
	if ('0' == in[0])
	{
		return 'x' == in[1] ? strtol16(in+2,out) : strtol8(in+1,out);
	}
	return strtol10(in, out);
}

// ------------------------------------------------------------------------------------
// Special version of the function, providing higher accuracy
// It is mainly used bx fast_atof to prevent ugly integer overflows.
// ------------------------------------------------------------------------------------
inline uint64_t strtol10_64( const char* in, const char** out=0)
{
	uint64_t value = 0;

	while ( 1 )
	{
		if ( *in < '0' || *in > '9' )
			break;

		value = ( value * 10 ) + ( *in - '0' );
		++in;
	}
	if (out)
		*out = in;
	return value;
}


// ------------------------------------------------------------------------------------
//! Provides a fast function for converting a string into a float,
//! about 6 times faster than atof in win32.
// If you find any bugs, please send them to me, niko (at) irrlicht3d.org.
// ------------------------------------------------------------------------------------
inline const char* fast_atof_move( const char* c, float& out)
{
	bool inv = false;
	const char *t;
	float f;

	if (*c=='-')
	{
		++c;
		inv = true;
	}
	else if (*c=='+')++c;

	f = (float) strtol10_64 ( c, &c );
	if (*c == '.' || (c[0] == ',' && isdigit( c[1])))
	{
		++c;

		// NOTE: The original implementation is highly unaccurate here
		// The precision of a single IEEE 754 float is not high enough
		// everything behind the 6th digit tends to be more inaccurate
		// than it would need  to be.
		// Casting to double seems to solve the problem.
		// strtol_64 is used to prevent integer overflow.

		//float pl = (float)strtol(c, &t, 10);
		double pl = (double) strtol10_64 ( c, &t );
		pl *= fast_atof_table[t-c];

		f += (float)pl;

		c = t;

		// FIX: a large 'E' should be allowed, too
		if (*c == 'e' || *c == 'E')
		{
			++c;
			bool einv = (*c=='-');
			if (einv)
				++c;

			float exp = (float)strtol10(c, &c);
			if (einv)
				exp *= -1.0f;

			f *= pow(10.0f, exp);
		}
	}

	if (inv)
		f *= -1.0f;
	
	out = f;
	return c;
}


// ------------------------------------------------------------------------------------
inline float fast_atof(const char* c)
{
	float ret;
	fast_atof_move(c, ret);
	return ret;
}

} // end of namespace Assimp

#endif

