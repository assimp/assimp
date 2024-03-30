/*
 * Copyright (c) 2011 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

/*
	File:		dp_dec.c

	Contains:	Dynamic Predictor decode routines

	Copyright:	(c) 2001-2011 Apple, Inc.
*/


#include "dplib.h"
#include <string.h>

#if __GNUC__
#define ALWAYS_INLINE		__attribute__((always_inline))
#else
#define ALWAYS_INLINE
#endif

#if TARGET_CPU_PPC && (__MWERKS__ >= 0x3200)
// align loops to a 16 byte boundary to make the G5 happy
#pragma function_align 16
#define LOOP_ALIGN			asm { align 16 }
#else
#define LOOP_ALIGN
#endif

static inline int32_t ALWAYS_INLINE sign_of_int( int32_t i )
{
    int32_t negishift;
	
    negishift = ((uint32_t)-i) >> 31;
    return negishift | (i >> 31);
}

void unpc_block( int32_t * pc1, int32_t * out, int32_t num, int16_t * coefs, int32_t numactive, uint32_t chanbits, uint32_t denshift )
{
	register int16_t	a0, a1, a2, a3;
	register int32_t	b0, b1, b2, b3;
	int32_t					j, k, lim;
	int32_t				sum1, sg, sgn, top, dd;
	int32_t *			pout;
	int32_t				del, del0;
	uint32_t			chanshift = 32 - chanbits;
	int32_t				denhalf = 1<<(denshift-1);

	out[0] = pc1[0];
	if ( numactive == 0 )
	{
		// just copy if numactive == 0 (but don't bother if in/out pointers the same)
		if ( (num > 1)  && (pc1 != out) )
			memcpy( &out[1], &pc1[1], (num - 1) * sizeof(int32_t) );
		return;
	}
	if ( numactive == 31 )
	{
		// short-circuit if numactive == 31
		int32_t		prev;
		
		/*	this code is written such that the in/out buffers can be the same
			to conserve buffer space on embedded devices like the iPod
			
			(original code)
			for ( j = 1; j < num; j++ )
				del = pc1[j] + out[j-1];
				out[j] = (del << chanshift) >> chanshift;
		*/
		prev = out[0];
		for ( j = 1; j < num; j++ )
		{
			del = pc1[j] + prev;
			prev = (del << chanshift) >> chanshift;
			out[j] = prev;
		}
		return;
	}

	for ( j = 1; j <= numactive; j++ )
	{
		del = pc1[j] + out[j-1];
		out[j] = (del << chanshift) >> chanshift;
	}

	lim = numactive + 1;

	if ( numactive == 4 )
	{
		// optimization for numactive == 4
		register int16_t	a0, a1, a2, a3;
		register int32_t	b0, b1, b2, b3;

		a0 = coefs[0];
		a1 = coefs[1];
		a2 = coefs[2];
		a3 = coefs[3];

		for ( j = lim; j < num; j++ )
		{
			LOOP_ALIGN

			top = out[j - lim];
			pout = out + j - 1;

			b0 = top - pout[0];
			b1 = top - pout[-1];
			b2 = top - pout[-2];
			b3 = top - pout[-3];

			sum1 = (denhalf - a0 * b0 - a1 * b1 - a2 * b2 - a3 * b3) >> denshift;

			del = pc1[j];
			del0 = del;
			sg = sign_of_int(del);
			del += top + sum1;

			out[j] = (del << chanshift) >> chanshift;

			if ( sg > 0 )
			{
				sgn = sign_of_int( b3 );
				a3 -= sgn;
				del0 -= (4 - 3) * ((sgn * b3) >> denshift);
				if ( del0 <= 0 )
					continue;
				
				sgn = sign_of_int( b2 );
				a2 -= sgn;
				del0 -= (4 - 2) * ((sgn * b2) >> denshift);
				if ( del0 <= 0 )
					continue;
				
				sgn = sign_of_int( b1 );
				a1 -= sgn;
				del0 -= (4 - 1) * ((sgn * b1) >> denshift);
				if ( del0 <= 0 )
					continue;

				a0 -= sign_of_int( b0 );
			}
			else if ( sg < 0 )
			{
				// note: to avoid unnecessary negations, we flip the value of "sgn"
				sgn = -sign_of_int( b3 );
				a3 -= sgn;
				del0 -= (4 - 3) * ((sgn * b3) >> denshift);
				if ( del0 >= 0 )
					continue;
				
				sgn = -sign_of_int( b2 );
				a2 -= sgn;
				del0 -= (4 - 2) * ((sgn * b2) >> denshift);
				if ( del0 >= 0 )
					continue;
				
				sgn = -sign_of_int( b1 );
				a1 -= sgn;
				del0 -= (4 - 1) * ((sgn * b1) >> denshift);
				if ( del0 >= 0 )
					continue;

				a0 += sign_of_int( b0 );
			}
		}

		coefs[0] = a0;
		coefs[1] = a1;
		coefs[2] = a2;
		coefs[3] = a3;
	}
	else if ( numactive == 8 )
	{
		register int16_t	a4, a5, a6, a7;
		register int32_t	b4, b5, b6, b7;

		// optimization for numactive == 8
		a0 = coefs[0];
		a1 = coefs[1];
		a2 = coefs[2];
		a3 = coefs[3];
		a4 = coefs[4];
		a5 = coefs[5];
		a6 = coefs[6];
		a7 = coefs[7];

		for ( j = lim; j < num; j++ )
		{
			LOOP_ALIGN

			top = out[j - lim];
			pout = out + j - 1;

			b0 = top - (*pout--);
			b1 = top - (*pout--);
			b2 = top - (*pout--);
			b3 = top - (*pout--);
			b4 = top - (*pout--);
			b5 = top - (*pout--);
			b6 = top - (*pout--);
			b7 = top - (*pout);
			pout += 8;

			sum1 = (denhalf - a0 * b0 - a1 * b1 - a2 * b2 - a3 * b3
					- a4 * b4 - a5 * b5 - a6 * b6 - a7 * b7) >> denshift;

			del = pc1[j];
			del0 = del;
			sg = sign_of_int(del);
			del += top + sum1;

			out[j] = (del << chanshift) >> chanshift;

			if ( sg > 0 )
			{
				sgn = sign_of_int( b7 );
				a7 -= sgn;
				del0 -= 1 * ((sgn * b7) >> denshift);
				if ( del0 <= 0 )
					continue;
				
				sgn = sign_of_int( b6 );
				a6 -= sgn;
				del0 -= 2 * ((sgn * b6) >> denshift);
				if ( del0 <= 0 )
					continue;
				
				sgn = sign_of_int( b5 );
				a5 -= sgn;
				del0 -= 3 * ((sgn * b5) >> denshift);
				if ( del0 <= 0 )
					continue;

				sgn = sign_of_int( b4 );
				a4 -= sgn;
				del0 -= 4 * ((sgn * b4) >> denshift);
				if ( del0 <= 0 )
					continue;
				
				sgn = sign_of_int( b3 );
				a3 -= sgn;
				del0 -= 5 * ((sgn * b3) >> denshift);
				if ( del0 <= 0 )
					continue;
				
				sgn = sign_of_int( b2 );
				a2 -= sgn;
				del0 -= 6 * ((sgn * b2) >> denshift);
				if ( del0 <= 0 )
					continue;
				
				sgn = sign_of_int( b1 );
				a1 -= sgn;
				del0 -= 7 * ((sgn * b1) >> denshift);
				if ( del0 <= 0 )
					continue;

				a0 -= sign_of_int( b0 );
			}
			else if ( sg < 0 )
			{
				// note: to avoid unnecessary negations, we flip the value of "sgn"
				sgn = -sign_of_int( b7 );
				a7 -= sgn;
				del0 -= 1 * ((sgn * b7) >> denshift);
				if ( del0 >= 0 )
					continue;
				
				sgn = -sign_of_int( b6 );
				a6 -= sgn;
				del0 -= 2 * ((sgn * b6) >> denshift);
				if ( del0 >= 0 )
					continue;
				
				sgn = -sign_of_int( b5 );
				a5 -= sgn;
				del0 -= 3 * ((sgn * b5) >> denshift);
				if ( del0 >= 0 )
					continue;

				sgn = -sign_of_int( b4 );
				a4 -= sgn;
				del0 -= 4 * ((sgn * b4) >> denshift);
				if ( del0 >= 0 )
					continue;
				
				sgn = -sign_of_int( b3 );
				a3 -= sgn;
				del0 -= 5 * ((sgn * b3) >> denshift);
				if ( del0 >= 0 )
					continue;
				
				sgn = -sign_of_int( b2 );
				a2 -= sgn;
				del0 -= 6 * ((sgn * b2) >> denshift);
				if ( del0 >= 0 )
					continue;
				
				sgn = -sign_of_int( b1 );
				a1 -= sgn;
				del0 -= 7 * ((sgn * b1) >> denshift);
				if ( del0 >= 0 )
					continue;

				a0 += sign_of_int( b0 );
			}
		}

		coefs[0] = a0;
		coefs[1] = a1;
		coefs[2] = a2;
		coefs[3] = a3;
		coefs[4] = a4;
		coefs[5] = a5;
		coefs[6] = a6;
		coefs[7] = a7;
	}
	else
	{
		// general case
		for ( j = lim; j < num; j++ )
		{
			LOOP_ALIGN

			sum1 = 0;
			pout = out + j - 1;
			top = out[j-lim];

			for ( k = 0; k < numactive; k++ )
				sum1 += coefs[k] * (pout[-k] - top);

			del = pc1[j];
			del0 = del;
			sg = sign_of_int( del );
			del += top + ((sum1 + denhalf) >> denshift);
			out[j] = (del << chanshift) >> chanshift;

			if ( sg > 0 )
			{
				for ( k = (numactive - 1); k >= 0; k-- )
				{
					dd = top - pout[-k];
					sgn = sign_of_int( dd );
					coefs[k] -= sgn;
					del0 -= (numactive - k) * ((sgn * dd) >> denshift);
					if ( del0 <= 0 )
						break;
				}
			}
			else if ( sg < 0 )
			{
				for ( k = (numactive - 1); k >= 0; k-- )
				{
					dd = top - pout[-k];
					sgn = sign_of_int( dd );
					coefs[k] += sgn;
					del0 -= (numactive - k) * ((-sgn * dd) >> denshift);
					if ( del0 >= 0 )
						break;
				}
			}
		}
	}
}
