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
	File:		matrixlib.h
	
	Contains:	ALAC mixing/matrixing routines to/from 32-bit predictor buffers.

	Copyright:	Copyright (C) 2004 to 2011 Apple, Inc.
*/

#ifndef __MATRIXLIB_H
#define __MATRIXLIB_H

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 16-bit routines
void	mix16( int16_t * in, uint32_t stride, int32_t * u, int32_t * v, int32_t numSamples, int32_t mixbits, int32_t mixres );
void	unmix16( int32_t * u, int32_t * v, int16_t * out, uint32_t stride, int32_t numSamples, int32_t mixbits, int32_t mixres );

// 20-bit routines
void	mix20( uint8_t * in, uint32_t stride, int32_t * u, int32_t * v, int32_t numSamples, int32_t mixbits, int32_t mixres );
void	unmix20( int32_t * u, int32_t * v, uint8_t * out, uint32_t stride, int32_t numSamples, int32_t mixbits, int32_t mixres );

// 24-bit routines
// - 24-bit data sometimes compresses better by shifting off the bottom byte so these routines deal with
//	 the specified "unused lower bytes" in the combined "shift" buffer
void	mix24( uint8_t * in, uint32_t stride, int32_t * u, int32_t * v, int32_t numSamples,
				int32_t mixbits, int32_t mixres, uint16_t * shiftUV, int32_t bytesShifted );
void	unmix24( int32_t * u, int32_t * v, uint8_t * out, uint32_t stride, int32_t numSamples,
				 int32_t mixbits, int32_t mixres, uint16_t * shiftUV, int32_t bytesShifted );

// 32-bit routines
// - note that these really expect the internal data width to be < 32-bit but the arrays are 32-bit
// - otherwise, the calculations might overflow into the 33rd bit and be lost
// - therefore, these routines deal with the specified "unused lower" bytes in the combined "shift" buffer
void	mix32( int32_t * in, uint32_t stride, int32_t * u, int32_t * v, int32_t numSamples,
				int32_t mixbits, int32_t mixres, uint16_t * shiftUV, int32_t bytesShifted );
void	unmix32( int32_t * u, int32_t * v, int32_t * out, uint32_t stride, int32_t numSamples,
				 int32_t mixbits, int32_t mixres, uint16_t * shiftUV, int32_t bytesShifted );

// 20/24/32-bit <-> 32-bit helper routines (not really matrixing but convenient to put here)
void	copy20ToPredictor( uint8_t * in, uint32_t stride, int32_t * out, int32_t numSamples );
void	copy24ToPredictor( uint8_t * in, uint32_t stride, int32_t * out, int32_t numSamples );

void	copyPredictorTo24( int32_t * in, uint8_t * out, uint32_t stride, int32_t numSamples );
void	copyPredictorTo24Shift( int32_t * in, uint16_t * shift, uint8_t * out, uint32_t stride, int32_t numSamples, int32_t bytesShifted );
void	copyPredictorTo20( int32_t * in, uint8_t * out, uint32_t stride, int32_t numSamples );

void	copyPredictorTo32( int32_t * in, int32_t * out, uint32_t stride, int32_t numSamples );
void	copyPredictorTo32Shift( int32_t * in, uint16_t * shift, int32_t * out, uint32_t stride, int32_t numSamples, int32_t bytesShifted );

#ifdef __cplusplus
}
#endif

#endif	/* __MATRIXLIB_H */
