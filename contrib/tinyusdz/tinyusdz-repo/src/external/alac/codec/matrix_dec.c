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
	File:		matrix_dec.c
	
	Contains:	ALAC mixing/matrixing decode routines.

	Copyright:	(c) 2004-2011 Apple, Inc.
*/

#include "matrixlib.h"
#include "ALACAudioTypes.h"

// up to 24-bit "offset" macros for the individual bytes of a 20/24-bit word
#if TARGET_RT_BIG_ENDIAN
	#define LBYTE	2
	#define MBYTE	1
	#define HBYTE	0
#else
	#define LBYTE	0
	#define MBYTE	1
	#define HBYTE	2
#endif

/*
    There is no plain middle-side option; instead there are various mixing
    modes including middle-side, each lossless, as embodied in the mix()
    and unmix() functions.  These functions exploit a generalized middle-side
    transformation:
    
    u := [(rL + (m-r)R)/m];
    v := L - R;
    
    where [ ] denotes integer floor.  The (lossless) inverse is
    
    L = u + v - [rV/m];
    R = L - v;
*/

// 16-bit routines

void unmix16( int32_t * u, int32_t * v, int16_t * out, uint32_t stride, int32_t numSamples, int32_t mixbits, int32_t mixres )
{
	int16_t *	op = out;
	int32_t 		j;

	if ( mixres != 0 )
	{
		/* matrixed stereo */
		for ( j = 0; j < numSamples; j++ )
		{
			int32_t		l, r;

			l = u[j] + v[j] - ((mixres * v[j]) >> mixbits);
			r = l - v[j];

			op[0] = (int16_t) l;
			op[1] = (int16_t) r;
			op += stride;
		} 
	}
	else
	{
		/* Conventional separated stereo. */
		for ( j = 0; j < numSamples; j++ )
		{
			op[0] = (int16_t) u[j];
			op[1] = (int16_t) v[j];
			op += stride;
		}
	}
}

// 20-bit routines
// - the 20 bits of data are left-justified in 3 bytes of storage but right-aligned for input/output predictor buffers

void unmix20( int32_t * u, int32_t * v, uint8_t * out, uint32_t stride, int32_t numSamples, int32_t mixbits, int32_t mixres )
{
	uint8_t *	op = out;
	int32_t 		j;

	if ( mixres != 0 )
	{
		/* matrixed stereo */
		for ( j = 0; j < numSamples; j++ )
		{
			int32_t		l, r;

			l = u[j] + v[j] - ((mixres * v[j]) >> mixbits);
			r = l - v[j];

			l <<= 4;
			r <<= 4;

			op[HBYTE] = (uint8_t)((l >> 16) & 0xffu);
			op[MBYTE] = (uint8_t)((l >>  8) & 0xffu);
			op[LBYTE] = (uint8_t)((l >>  0) & 0xffu);
			op += 3;

			op[HBYTE] = (uint8_t)((r >> 16) & 0xffu);
			op[MBYTE] = (uint8_t)((r >>  8) & 0xffu);
			op[LBYTE] = (uint8_t)((r >>  0) & 0xffu);

			op += (stride - 1) * 3;
		}
	}
	else 
	{
		/* Conventional separated stereo. */
		for ( j = 0; j < numSamples; j++ )
		{
			int32_t		val;

			val = u[j] << 4;
			op[HBYTE] = (uint8_t)((val >> 16) & 0xffu);
			op[MBYTE] = (uint8_t)((val >>  8) & 0xffu);
			op[LBYTE] = (uint8_t)((val >>  0) & 0xffu);
			op += 3;

			val = v[j] << 4;
			op[HBYTE] = (uint8_t)((val >> 16) & 0xffu);
			op[MBYTE] = (uint8_t)((val >>  8) & 0xffu);
			op[LBYTE] = (uint8_t)((val >>  0) & 0xffu);

			op += (stride - 1) * 3;
		}
	}
}

// 24-bit routines
// - the 24 bits of data are right-justified in the input/output predictor buffers

void unmix24( int32_t * u, int32_t * v, uint8_t * out, uint32_t stride, int32_t numSamples,
				int32_t mixbits, int32_t mixres, uint16_t * shiftUV, int32_t bytesShifted )
{
	uint8_t *	op = out;
	int32_t			shift = bytesShifted * 8;
	int32_t		l, r;
	int32_t 		j, k;

	if ( mixres != 0 )
	{
		/* matrixed stereo */
		if ( bytesShifted != 0 )
		{
			for ( j = 0, k = 0; j < numSamples; j++, k += 2 )
			{
				l = u[j] + v[j] - ((mixres * v[j]) >> mixbits);
				r = l - v[j];

				l = (l << shift) | (uint32_t) shiftUV[k + 0];
				r = (r << shift) | (uint32_t) shiftUV[k + 1];

				op[HBYTE] = (uint8_t)((l >> 16) & 0xffu);
				op[MBYTE] = (uint8_t)((l >>  8) & 0xffu);
				op[LBYTE] = (uint8_t)((l >>  0) & 0xffu);
				op += 3;

				op[HBYTE] = (uint8_t)((r >> 16) & 0xffu);
				op[MBYTE] = (uint8_t)((r >>  8) & 0xffu);
				op[LBYTE] = (uint8_t)((r >>  0) & 0xffu);

				op += (stride - 1) * 3;
			}
		}
		else
		{
			for ( j = 0; j < numSamples; j++ )
			{
				l = u[j] + v[j] - ((mixres * v[j]) >> mixbits);
				r = l - v[j];

				op[HBYTE] = (uint8_t)((l >> 16) & 0xffu);
				op[MBYTE] = (uint8_t)((l >>  8) & 0xffu);
				op[LBYTE] = (uint8_t)((l >>  0) & 0xffu);
				op += 3;

				op[HBYTE] = (uint8_t)((r >> 16) & 0xffu);
				op[MBYTE] = (uint8_t)((r >>  8) & 0xffu);
				op[LBYTE] = (uint8_t)((r >>  0) & 0xffu);

				op += (stride - 1) * 3;
			}
		}
	}
	else 
	{
		/* Conventional separated stereo. */
		if ( bytesShifted != 0 )
		{
			for ( j = 0, k = 0; j < numSamples; j++, k += 2 )
			{
				l = u[j];
				r = v[j];

				l = (l << shift) | (uint32_t) shiftUV[k + 0];
				r = (r << shift) | (uint32_t) shiftUV[k + 1];

				op[HBYTE] = (uint8_t)((l >> 16) & 0xffu);
				op[MBYTE] = (uint8_t)((l >>  8) & 0xffu);
				op[LBYTE] = (uint8_t)((l >>  0) & 0xffu);
				op += 3;

				op[HBYTE] = (uint8_t)((r >> 16) & 0xffu);
				op[MBYTE] = (uint8_t)((r >>  8) & 0xffu);
				op[LBYTE] = (uint8_t)((r >>  0) & 0xffu);

				op += (stride - 1) * 3;
			}
		}
		else
		{
			for ( j = 0; j < numSamples; j++ )
			{
				int32_t		val;

				val = u[j];
				op[HBYTE] = (uint8_t)((val >> 16) & 0xffu);
				op[MBYTE] = (uint8_t)((val >>  8) & 0xffu);
				op[LBYTE] = (uint8_t)((val >>  0) & 0xffu);
				op += 3;

				val = v[j];
				op[HBYTE] = (uint8_t)((val >> 16) & 0xffu);
				op[MBYTE] = (uint8_t)((val >>  8) & 0xffu);
				op[LBYTE] = (uint8_t)((val >>  0) & 0xffu);

				op += (stride - 1) * 3;
			}
		}
	}
}

// 32-bit routines
// - note that these really expect the internal data width to be < 32 but the arrays are 32-bit
// - otherwise, the calculations might overflow into the 33rd bit and be lost
// - therefore, these routines deal with the specified "unused lower" bytes in the "shift" buffers

void unmix32( int32_t * u, int32_t * v, int32_t * out, uint32_t stride, int32_t numSamples,
				int32_t mixbits, int32_t mixres, uint16_t * shiftUV, int32_t bytesShifted )
{
	int32_t *	op = out;
	int32_t			shift = bytesShifted * 8;
	int32_t		l, r;
	int32_t 		j, k;

	if ( mixres != 0 )
	{
		//Assert( bytesShifted != 0 );

		/* matrixed stereo with shift */
		for ( j = 0, k = 0; j < numSamples; j++, k += 2 )
		{
			int32_t		lt, rt;

			lt = u[j];
			rt = v[j];
			
			l = lt + rt - ((mixres * rt) >> mixbits);
			r = l - rt;

			op[0] = (l << shift) | (uint32_t) shiftUV[k + 0];
			op[1] = (r << shift) | (uint32_t) shiftUV[k + 1];
			op += stride;
		} 
	}
	else
	{
		if ( bytesShifted == 0 )
		{
			/* interleaving w/o shift */
			for ( j = 0; j < numSamples; j++ )
			{
				op[0] = u[j];
				op[1] = v[j];
				op += stride;
			}
		}
		else
		{
			/* interleaving with shift */
			for ( j = 0, k = 0; j < numSamples; j++, k += 2 )
			{
				op[0] = (u[j] << shift) | (uint32_t) shiftUV[k + 0];
				op[1] = (v[j] << shift) | (uint32_t) shiftUV[k + 1];
				op += stride;
			}
		}
	}
}

// 20/24-bit <-> 32-bit helper routines (not really matrixing but convenient to put here)

void copyPredictorTo24( int32_t * in, uint8_t * out, uint32_t stride, int32_t numSamples )
{
	uint8_t *	op = out;
	int32_t			j;

	for ( j = 0; j < numSamples; j++ )
	{
		int32_t		val = in[j];

		op[HBYTE] = (uint8_t)((val >> 16) & 0xffu);
		op[MBYTE] = (uint8_t)((val >>  8) & 0xffu);
		op[LBYTE] = (uint8_t)((val >>  0) & 0xffu);
		op += (stride * 3);
	}
}

void copyPredictorTo24Shift( int32_t * in, uint16_t * shift, uint8_t * out, uint32_t stride, int32_t numSamples, int32_t bytesShifted )
{
	uint8_t *	op = out;
	int32_t			shiftVal = bytesShifted * 8;
	int32_t			j;

	//Assert( bytesShifted != 0 );

	for ( j = 0; j < numSamples; j++ )
	{
		int32_t		val = in[j];

		val = (val << shiftVal) | (uint32_t) shift[j];

		op[HBYTE] = (uint8_t)((val >> 16) & 0xffu);
		op[MBYTE] = (uint8_t)((val >>  8) & 0xffu);
		op[LBYTE] = (uint8_t)((val >>  0) & 0xffu);
		op += (stride * 3);
	}
}

void copyPredictorTo20( int32_t * in, uint8_t * out, uint32_t stride, int32_t numSamples )
{
	uint8_t *	op = out;
	int32_t			j;

	// 32-bit predictor values are right-aligned but 20-bit output values should be left-aligned
	// in the 24-bit output buffer
	for ( j = 0; j < numSamples; j++ )
	{
		int32_t		val = in[j];

		op[HBYTE] = (uint8_t)((val >> 12) & 0xffu);
		op[MBYTE] = (uint8_t)((val >>  4) & 0xffu);
		op[LBYTE] = (uint8_t)((val <<  4) & 0xffu);
		op += (stride * 3);
	}
}

void copyPredictorTo32( int32_t * in, int32_t * out, uint32_t stride, int32_t numSamples )
{
	int32_t			i, j;

	// this is only a subroutine to abstract the "iPod can only output 16-bit data" problem
	for ( i = 0, j = 0; i < numSamples; i++, j += stride )
		out[j] = in[i];
}

void copyPredictorTo32Shift( int32_t * in, uint16_t * shift, int32_t * out, uint32_t stride, int32_t numSamples, int32_t bytesShifted )
{
	int32_t *		op = out;
	uint32_t		shiftVal = bytesShifted * 8;
	int32_t				j;

	//Assert( bytesShifted != 0 );

	// this is only a subroutine to abstract the "iPod can only output 16-bit data" problem
	for ( j = 0; j < numSamples; j++ )
	{
		op[0] = (in[j] << shiftVal) | (uint32_t) shift[j];
		op += stride;
	}
}
