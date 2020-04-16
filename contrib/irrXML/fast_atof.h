// Copyright (C) 2002-2005 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine" and the "irrXML" project.
// For conditions of distribution and use, see copyright notice in irrlicht.h and irrXML.h

#ifndef __FAST_A_TO_F_H_INCLUDED__
#define __FAST_A_TO_F_H_INCLUDED__

#include <stdlib.h>
#include <math.h>

namespace irr
{
namespace core
{

const float fast_atof_table[] =	{
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

//! Provides a fast function for converting a string into a float,
//! about 6 times faster than atof in win32.
// If you find any bugs, please send them to me, niko (at) irrlicht3d.org.
inline char* fast_atof_move(char* c, float& out)
{
	bool inv = false;
	char *t;
	float f;

	if (*c=='-')
	{
		c++;
		inv = true;
	}

	f = (float)strtol(c, &t, 10);

	c = t;

	if (*c == '.')
	{
		c++;

		float pl = (float)strtol(c, &t, 10);
		pl *= fast_atof_table[t-c];

		f += pl;

		c = t;

		if (*c == 'e')
		{
			++c;
			float exp = (float)strtol(c, &t, 10);
			f *= (float)pow(10.0f, exp);
			c = t;
		}
	}

	if (inv)
		f *= -1.0f;
	
	out = f;
	return c;
}

//! Provides a fast function for converting a string into a float,
//! about 6 times faster than atof in win32.
// If you find any bugs, please send them to me, niko (at) irrlicht3d.org.
inline const char* fast_atof_move_const(const char* c, float& out)
{
	bool inv = false;
	char *t;
	float f;

	if (*c=='-')
	{
		c++;
		inv = true;
	}

	f = (float)strtol(c, &t, 10);

	c = t;

	if (*c == '.')
	{
		c++;

		float pl = (float)strtol(c, &t, 10);
		pl *= fast_atof_table[t-c];

		f += pl;

		c = t;

		if (*c == 'e') 
		{ 
			++c; 
			f32 exp = (f32)strtol(c, &t, 10); 
			f *= (f32)powf(10.0f, exp); 
			c = t; 
		}
	}

	if (inv)
		f *= -1.0f;
	
	out = f;
	return c;
}


inline float fast_atof(const char* c)
{
	float ret;
	fast_atof_move_const(c, ret);
	return ret;
}

} // end namespace core
}// end namespace irr

#endif

