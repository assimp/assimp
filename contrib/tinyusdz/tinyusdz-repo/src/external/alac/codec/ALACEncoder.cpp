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
	File:		ALACEncoder.cpp
*/

// build stuff
#define VERBOSE_DEBUG		0

// headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ALACEncoder.h"

#include "aglib.h"
#include "dplib.h"
#include "matrixlib.h"

#include "ALACBitUtilities.h"
#include "ALACAudioTypes.h"
#include "EndianPortable.h"

// Note: in C you can't typecast to a 2-dimensional array pointer but that's what we need when
// picking which coefs to use so we declare this typedef b/c we *can* typecast to this type
typedef int16_t (*SearchCoefs)[kALACMaxCoefs];

// defines/constants
//const uint32_t kALACEncoderMagic	= 'dpge';
const uint32_t kMaxSampleSize		= 32;			// max allowed bit width is 32
const uint32_t kDefaultMixBits	= 2;
const uint32_t kDefaultMixRes		= 0;
const uint32_t kMaxRes			= 4;
const uint32_t kDefaultNumUV		= 8;
const uint32_t kMinUV				= 4;
const uint32_t kMaxUV				= 8;

// static functions
#if VERBOSE_DEBUG
static void AddFiller( BitBuffer * bits, int32_t numBytes );
#endif


/*
	Map Format: 3-bit field per channel which is the same as the "element tag" that should be placed
				at the beginning of the frame for that channel.  Indicates whether SCE, CPE, or LFE.
				Each particular field is accessed via the current channel index.  Note that the channel
				index increments by two for channel pairs.
				
	For example:
	
			C L R 3-channel input		= (ID_CPE << 3) | (ID_SCE)
				index 0 value = (map & (0x7ul << (0 * 3))) >> (0 * 3)
				index 1 value = (map & (0x7ul << (1 * 3))) >> (1 * 3)

			C L R Ls Rs LFE 5.1-channel input = (ID_LFE << 15) | (ID_CPE << 9) | (ID_CPE << 3) | (ID_SCE)
				index 0 value = (map & (0x7ul << (0 * 3))) >> (0 * 3)
				index 1 value = (map & (0x7ul << (1 * 3))) >> (1 * 3)
				index 3 value = (map & (0x7ul << (3 * 3))) >> (3 * 3)
				index 5 value = (map & (0x7ul << (5 * 3))) >> (5 * 3)
				index 7 value = (map & (0x7ul << (7 * 3))) >> (7 * 3)
*/
static const uint32_t	sChannelMaps[kALACMaxChannels] =
{
	ID_SCE,
	ID_CPE,
	(ID_CPE << 3) | (ID_SCE),
	(ID_SCE << 9) | (ID_CPE << 3) | (ID_SCE),
	(ID_CPE << 9) | (ID_CPE << 3) | (ID_SCE),
	(ID_SCE << 15) | (ID_CPE << 9) | (ID_CPE << 3) | (ID_SCE),
	(ID_SCE << 18) | (ID_SCE << 15) | (ID_CPE << 9) | (ID_CPE << 3) | (ID_SCE),
	(ID_SCE << 21) | (ID_CPE << 15) | (ID_CPE << 9) | (ID_CPE << 3) | (ID_SCE)
};

//static const uint32_t sSupportediPodSampleRates[] =
//{
//	8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000
//};

/*
	Constructor
*/
ALACEncoder::ALACEncoder() :
	mBitDepth( 0 ),
    mFastMode( 0 ),
	mMixBufferU( nil ),
	mMixBufferV( nil ),
	mPredictorU( nil ),
	mPredictorV( nil ),
	mShiftBufferUV( nil ),
	mWorkBuffer( nil ),


	mTotalBytesGenerated( 0 ),
	mAvgBitRate( 0 ),
	mMaxFrameBytes( 0 )
{
	// overrides
	mFrameSize = kALACDefaultFrameSize;
}

/*
	Destructor
*/
ALACEncoder::~ALACEncoder()
{
	// delete the matrix mixing buffers
	if ( mMixBufferU )
    {
		free(mMixBufferU);
        mMixBufferU = NULL;
    }
	if ( mMixBufferV )
    {
		free(mMixBufferV);
        mMixBufferV = NULL;
    }
	
	// delete the dynamic predictor's "corrector" buffers
	if ( mPredictorU )
    {
		free(mPredictorU);
        mPredictorU = NULL;
    }
	if ( mPredictorV )
    {
		free(mPredictorV);
        mPredictorV = NULL;
    }

	// delete the unused byte shift buffer
	if ( mShiftBufferUV )
    {
		free(mShiftBufferUV);
        mShiftBufferUV = NULL;
    }

	// delete the work buffer
	if ( mWorkBuffer )
    {
		free(mWorkBuffer);
        mWorkBuffer = NULL;
    }	
}

#if PRAGMA_MARK
#pragma mark -
#endif

/*
	HEADER SPECIFICATION  

        For every segment we adopt the following header:
        
			1 byte reserved			(always 0)
			1 byte flags			(see below)
			[4 byte frame length]	(optional, see below)
			     ---Next, the per-segment ALAC parameters---
			1 byte mixBits			(middle-side parameter)
			1 byte mixRes			(middle-side parameter, interpreted as signed char)

			1 byte shiftU			(4 bits modeU, 4 bits denShiftU)
			1 byte filterU			(3 bits pbFactorU, 5 bits numU)
			(numU) shorts			(signed DP coefficients for V channel)
			     ---Next, 2nd-channel ALAC parameters in case of stereo mode---
			1 byte shiftV			(4 bits modeV, 4 bits denShiftV)
			1 byte filterV			(3 bits pbFactorV, 5 bits numV)
			(numV) shorts			(signed DP coefficients for V channel)
			     ---After this come the shift-off bytes for (>= 24)-bit data (n-byte shift) if indicated---
			     ---Then comes the AG-compressor bitstream---


        FLAGS
        -----

		The presence of certain flag bits changes the header format such that the parameters might
		not even be sent.  The currently defined flags format is:

			0000psse
			
			where		0 	= reserved, must be 0
						p	= 1-bit field "partial frame" flag indicating 32-bit frame length follows this byte
						ss	= 2-bit field indicating "number of shift-off bytes ignored by compression"
						e	= 1-bit field indicating "escape"

		The "partial frame" flag means that the following segment is not equal to the frame length specified
		in the out-of-band decoder configuration.  This allows the decoder to deal with end-of-file partial
		segments without incurring the 32-bit overhead for each segment.

		The "shift-off" field indicates the number of bytes at the bottom of the word that were passed through
		uncompressed.  The reason for this is that the entropy inherent in the LS bytes of >= 24-bit words
		quite often means that the frame would have to be "escaped" b/c the compressed size would be >= the
		uncompressed size.  However, by shifting the input values down and running the remaining bits through
		the normal compression algorithm, a net win can be achieved.  If this field is non-zero, it means that
		the shifted-off bytes follow after the parameter section of the header and before the compressed
		bitstream.  Note that doing this also allows us to use matrixing on 32-bit inputs after one or more
		bytes are shifted off the bottom which helps the eventual compression ratio.  For stereo channels,
		the shifted off bytes are interleaved.

        The "escape" flag means that this segment was not compressed b/c the compressed size would be
        >= uncompressed size.  In that case, the audio data was passed through uncompressed after the header.
        The other header parameter bytes will not be sent.
        

		PARAMETERS
		----------

		If the segment is not a partial or escape segment, the total header size (in bytes) is given exactly by:

			4 + (2 + 2 * numU)                   (mono mode)
			4 + (2 + 2 * numV) + (2 + 2 * numV)  (stereo mode)

        where the ALAC filter-lengths numU, numV are bounded by a
        constant (in the current source, numU, numV <= NUMCOEPAIRS), and
        this forces an absolute upper bound on header size.

        Each segment-decode process loads up these bytes from the front of the
        local stream, in the above order, then follows with the entropy-encoded
        bits for the given segment.

        To generalize middle-side, there are various mixing modes including middle-side, each lossless,
        as embodied in the mix() and unmix() functions.  These functions exploit a generalized middle-side
        transformation:

        u := [(rL + (m-r)R)/m];
        v := L - R;

        where [ ] denotes integer floor.  The (lossless) inverse is

        L = u + v - [rV/m];
        R = L - v;

        In the segment header, m and r are encoded in mixBits and mixRes.
        Classical "middle-side" is obtained with m = 2, r = 1, but now
        we have more generalized mixes.

        NOTES
        -----
        The relevance of the ALAC coefficients is explained in detail
        in patent documents.
*/

/*
	EncodeStereo()
	- encode a channel pair
*/
int32_t ALACEncoder::EncodeStereo( BitBuffer * bitstream, void * inputBuffer, uint32_t stride, uint32_t channelIndex, uint32_t numSamples )
{
	BitBuffer		workBits;
	BitBuffer		startBits = *bitstream;			// squirrel away copy of current state in case we need to go back and do an escape packet
	AGParamRec		agParams;
	uint32_t          bits1, bits2;
	uint32_t			dilate;
	int32_t			mixBits, mixRes, maxRes;
	uint32_t			minBits, minBits1, minBits2;
	uint32_t			numU, numV;
	uint32_t			mode;
	uint32_t			pbFactor;
	uint32_t			chanBits;
	//uint32_t			denShift;
	uint8_t			bytesShifted;
	SearchCoefs		coefsU;
	SearchCoefs		coefsV;
	uint32_t			index;
	uint8_t			partialFrame;
	uint32_t			escapeBits;
	bool			doEscape;
	int32_t		status = ALAC_noErr;

	// make sure we handle this bit-depth before we get going
	RequireAction( (mBitDepth == 16) || (mBitDepth == 20) || (mBitDepth == 24) || (mBitDepth == 32), return kALAC_ParamError; );

	// reload coefs pointers for this channel pair
	// - note that, while you might think they should be re-initialized per block, retaining state across blocks
	//	 actually results in better overall compression
	// - strangely, re-using the same coefs for the different passes of the "mixRes" search loop instead of using
	//	 different coefs for the different passes of "mixRes" results in even better compression
	coefsU = (SearchCoefs) mCoefsU[channelIndex];
	coefsV = (SearchCoefs) mCoefsV[channelIndex];

	// matrix encoding adds an extra bit but 32-bit inputs cannot be matrixed b/c 33 is too many
	// so enable 16-bit "shift off" and encode in 17-bit mode
	// - in addition, 24-bit mode really improves with one byte shifted off
	if ( mBitDepth == 32 )
		bytesShifted = 2;
	else if ( mBitDepth >= 24 )
		bytesShifted = 1;
	else
		bytesShifted = 0;

	chanBits = mBitDepth - (bytesShifted * 8) + 1;
	
	// flag whether or not this is a partial frame
	partialFrame = (numSamples == mFrameSize) ? 0 : 1;

	// brute-force encode optimization loop
	// - run over variations of the encoding params to find the best choice
	mixBits		= kDefaultMixBits;
	maxRes		= kMaxRes;
	numU = numV = kDefaultNumUV;
	//denShift	= DENSHIFT_DEFAULT;
	mode		= 0;
	pbFactor	= 4;
	dilate		= 8;

	minBits	= minBits1 = minBits2 = 1ul << 31;
	
    int32_t		bestRes = mLastMixRes[channelIndex];

    for ( mixRes = 0; mixRes <= maxRes; mixRes++ )
    {
        // mix the stereo inputs
        switch ( mBitDepth )
        {
            case 16:
                mix16( (int16_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples/dilate, mixBits, mixRes );
                break;
            case 20:
                mix20( (uint8_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples/dilate, mixBits, mixRes );
                break;
            case 24:
                // includes extraction of shifted-off bytes
                mix24( (uint8_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples/dilate,
                        mixBits, mixRes, mShiftBufferUV, bytesShifted );
                break;
            case 32:
                // includes extraction of shifted-off bytes
                mix32( (int32_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples/dilate,
                        mixBits, mixRes, mShiftBufferUV, bytesShifted );
                break;
        }

        BitBufferInit( &workBits, mWorkBuffer, mMaxOutputBytes );
        
        // run the dynamic predictors
        pc_block( mMixBufferU, mPredictorU, numSamples/dilate, coefsU[numU - 1], numU, chanBits, DENSHIFT_DEFAULT );
        pc_block( mMixBufferV, mPredictorV, numSamples/dilate, coefsV[numV - 1], numV, chanBits, DENSHIFT_DEFAULT );

        // run the lossless compressor on each channel
        set_ag_params( &agParams, MB0, (pbFactor * PB0) / 4, KB0, numSamples/dilate, numSamples/dilate, MAX_RUN_DEFAULT );
        status = dyn_comp( &agParams, mPredictorU, &workBits, numSamples/dilate, chanBits, &bits1 );
        RequireNoErr( status, goto Exit; );

        set_ag_params( &agParams, MB0, (pbFactor * PB0) / 4, KB0, numSamples/dilate, numSamples/dilate, MAX_RUN_DEFAULT );
        status = dyn_comp( &agParams, mPredictorV, &workBits, numSamples/dilate, chanBits, &bits2 );
        RequireNoErr( status, goto Exit; );

        // look for best match
        if ( (bits1 + bits2) < minBits1 )
        {
            minBits1 = bits1 + bits2;
            bestRes = mixRes;
        }
    }
    
    mLastMixRes[channelIndex] = (int16_t)bestRes;

	// mix the stereo inputs with the current best mixRes
	mixRes = mLastMixRes[channelIndex];
	switch ( mBitDepth )
	{
		case 16:
			mix16( (int16_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples, mixBits, mixRes );
			break;
		case 20:
			mix20( (uint8_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples, mixBits, mixRes );
			break;
		case 24:
			// also extracts the shifted off bytes into the shift buffers
			mix24( (uint8_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples,
					mixBits, mixRes, mShiftBufferUV, bytesShifted );
			break;
		case 32:
			// also extracts the shifted off bytes into the shift buffers
			mix32( (int32_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples,
					mixBits, mixRes, mShiftBufferUV, bytesShifted );
			break;
	}

	// now it's time for the predictor coefficient search loop
	numU = numV = kMinUV;
	minBits1 = minBits2 = 1ul << 31;

	for ( uint32_t numUV = kMinUV; numUV <= kMaxUV; numUV += 4 )
	{
		BitBufferInit( &workBits, mWorkBuffer, mMaxOutputBytes );		

		dilate = 32;

		// run the predictor over the same data multiple times to help it converge
		for ( uint32_t converge = 0; converge < 8; converge++ )
		{
		    pc_block( mMixBufferU, mPredictorU, numSamples/dilate, coefsU[numUV-1], numUV, chanBits, DENSHIFT_DEFAULT );
		    pc_block( mMixBufferV, mPredictorV, numSamples/dilate, coefsV[numUV-1], numUV, chanBits, DENSHIFT_DEFAULT );
		}

		dilate = 8;

		set_ag_params( &agParams, MB0, (pbFactor * PB0)/4, KB0, numSamples/dilate, numSamples/dilate, MAX_RUN_DEFAULT );
		status = dyn_comp( &agParams, mPredictorU, &workBits, numSamples/dilate, chanBits, &bits1 );

		if ( (bits1 * dilate + 16 * numUV) < minBits1 )
		{
			minBits1 = bits1 * dilate + 16 * numUV;
			numU = numUV;
		}

		set_ag_params( &agParams, MB0, (pbFactor * PB0)/4, KB0, numSamples/dilate, numSamples/dilate, MAX_RUN_DEFAULT );
		status = dyn_comp( &agParams, mPredictorV, &workBits, numSamples/dilate, chanBits, &bits2 );

		if ( (bits2 * dilate + 16 * numUV) < minBits2 )
		{
			minBits2 = bits2 * dilate + 16 * numUV;
			numV = numUV;
		}
	}

	// test for escape hatch if best calculated compressed size turns out to be more than the input size
	minBits = minBits1 + minBits2 + (8 /* mixRes/maxRes/etc. */ * 8) + ((partialFrame == true) ? 32 : 0);
	if ( bytesShifted != 0 )
		minBits += (numSamples * (bytesShifted * 8) * 2);

	escapeBits = (numSamples * mBitDepth * 2) + ((partialFrame == true) ? 32 : 0) + (2 * 8);	/* 2 common header bytes */

	doEscape = (minBits >= escapeBits) ? true : false;

	if ( doEscape == false )
	{
		// write bitstream header and coefs
		BitBufferWrite( bitstream, 0, 12 );
		BitBufferWrite( bitstream, (partialFrame << 3) | (bytesShifted << 1), 4 );
		if ( partialFrame )
			BitBufferWrite( bitstream, numSamples, 32 );
		BitBufferWrite( bitstream, mixBits, 8 );
		BitBufferWrite( bitstream, mixRes, 8 );
		
		//Assert( (mode < 16) && (DENSHIFT_DEFAULT < 16) );
		//Assert( (pbFactor < 8) && (numU < 32) );
		//Assert( (pbFactor < 8) && (numV < 32) );

		BitBufferWrite( bitstream, (mode << 4) | DENSHIFT_DEFAULT, 8 );
		BitBufferWrite( bitstream, (pbFactor << 5) | numU, 8 );
		for ( index = 0; index < numU; index++ )
			BitBufferWrite( bitstream, coefsU[numU - 1][index], 16 );

		BitBufferWrite( bitstream, (mode << 4) | DENSHIFT_DEFAULT, 8 );
		BitBufferWrite( bitstream, (pbFactor << 5) | numV, 8 );
		for ( index = 0; index < numV; index++ )
			BitBufferWrite( bitstream, coefsV[numV - 1][index], 16 );

		// if shift active, write the interleaved shift buffers
		if ( bytesShifted != 0 )
		{
			uint32_t		bitShift = bytesShifted * 8;

			//Assert( bitShift <= 16 );

			for ( index = 0; index < (numSamples * 2); index += 2 )
			{
				uint32_t			shiftedVal;
				
				shiftedVal = ((uint32_t)mShiftBufferUV[index + 0] << bitShift) | (uint32_t)mShiftBufferUV[index + 1];
				BitBufferWrite( bitstream, shiftedVal, bitShift * 2 );
			}
		}

		// run the dynamic predictor and lossless compression for the "left" channel
		// - note: to avoid allocating more buffers, we're mixing and matching between the available buffers instead
		//		   of only using "U" buffers for the U-channel and "V" buffers for the V-channel
		if ( mode == 0 )
		{
			pc_block( mMixBufferU, mPredictorU, numSamples, coefsU[numU - 1], numU, chanBits, DENSHIFT_DEFAULT );
		}
		else
		{
			pc_block( mMixBufferU, mPredictorV, numSamples, coefsU[numU - 1], numU, chanBits, DENSHIFT_DEFAULT );
			pc_block( mPredictorV, mPredictorU, numSamples, nil, 31, chanBits, 0 );
		}

		set_ag_params( &agParams, MB0, (pbFactor * PB0) / 4, KB0, numSamples, numSamples, MAX_RUN_DEFAULT );
		status = dyn_comp( &agParams, mPredictorU, bitstream, numSamples, chanBits, &bits1 );
		RequireNoErr( status, goto Exit; );

		// run the dynamic predictor and lossless compression for the "right" channel
		if ( mode == 0 )
		{
			pc_block( mMixBufferV, mPredictorV, numSamples, coefsV[numV - 1], numV, chanBits, DENSHIFT_DEFAULT );
		}
		else
		{
			pc_block( mMixBufferV, mPredictorU, numSamples, coefsV[numV - 1], numV, chanBits, DENSHIFT_DEFAULT );
			pc_block( mPredictorU, mPredictorV, numSamples, nil, 31, chanBits, 0 );
		}

		set_ag_params( &agParams, MB0, (pbFactor * PB0) / 4, KB0, numSamples, numSamples, MAX_RUN_DEFAULT );
		status = dyn_comp( &agParams, mPredictorV, bitstream, numSamples, chanBits, &bits2 );
		RequireNoErr( status, goto Exit; );

		/*	if we happened to create a compressed packet that was actually bigger than an escape packet would be,
			chuck it and do an escape packet
		*/
		minBits = BitBufferGetPosition( bitstream ) - BitBufferGetPosition( &startBits );
		if ( minBits >= escapeBits )
		{
			*bitstream = startBits;		// reset bitstream state
			doEscape = true;
			printf( "compressed frame too big: %u vs. %u \n", minBits, escapeBits );
		}
	}

	if ( doEscape == true )
	{
		/* escape */
		status = this->EncodeStereoEscape( bitstream, inputBuffer, stride, numSamples );

#if VERBOSE_DEBUG		
		DebugMsg( "escape!: %lu vs %lu", minBits, escapeBits );
#endif
	}
	
Exit:
	return status;
}

/*
	EncodeStereoFast()
	- encode a channel pair without the search loop for maximum possible speed
*/
int32_t ALACEncoder::EncodeStereoFast( BitBuffer * bitstream, void * inputBuffer, uint32_t stride, uint32_t channelIndex, uint32_t numSamples )
{
	BitBuffer		startBits = *bitstream;			// squirrel away current bit position in case we decide to use escape hatch
	AGParamRec		agParams;
	uint32_t	bits1, bits2;
	int32_t			mixBits, mixRes;
	uint32_t			minBits, minBits1, minBits2;
	uint32_t			numU, numV;
	uint32_t			mode;
	uint32_t			pbFactor;
	uint32_t			chanBits;
	//uint32_t			denShift;
	uint8_t			bytesShifted;
	SearchCoefs		coefsU;
	SearchCoefs		coefsV;
	uint32_t			index;
	uint8_t			partialFrame;
	uint32_t			escapeBits;
	bool			doEscape;	
	int32_t		status;

	// make sure we handle this bit-depth before we get going
	RequireAction( (mBitDepth == 16) || (mBitDepth == 20) || (mBitDepth == 24) || (mBitDepth == 32), return kALAC_ParamError; );

	// reload coefs pointers for this channel pair
	// - note that, while you might think they should be re-initialized per block, retaining state across blocks
	//	 actually results in better overall compression
	// - strangely, re-using the same coefs for the different passes of the "mixRes" search loop instead of using
	//	 different coefs for the different passes of "mixRes" results in even better compression
	coefsU = (SearchCoefs) mCoefsU[channelIndex];
	coefsV = (SearchCoefs) mCoefsV[channelIndex];

	// matrix encoding adds an extra bit but 32-bit inputs cannot be matrixed b/c 33 is too many
	// so enable 16-bit "shift off" and encode in 17-bit mode
	// - in addition, 24-bit mode really improves with one byte shifted off
	if ( mBitDepth == 32 )
		bytesShifted = 2;
	else if ( mBitDepth >= 24 )
		bytesShifted = 1;
	else
		bytesShifted = 0;

	chanBits = mBitDepth - (bytesShifted * 8) + 1;
	
	// flag whether or not this is a partial frame
	partialFrame = (numSamples == mFrameSize) ? 0 : 1;

	// set up default encoding parameters for "fast" mode
	mixBits		= kDefaultMixBits;
	mixRes		= kDefaultMixRes;
	numU = numV = kDefaultNumUV;
	//denShift	= DENSHIFT_DEFAULT;
	mode		= 0;
	pbFactor	= 4;

	minBits	= minBits1 = minBits2 = 1ul << 31;
	
	// mix the stereo inputs with default mixBits/mixRes
	switch ( mBitDepth )
	{
		case 16:
			mix16( (int16_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples, mixBits, mixRes );
			break;
		case 20:
			mix20( (uint8_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples, mixBits, mixRes );
			break;
		case 24:
			// also extracts the shifted off bytes into the shift buffers
			mix24( (uint8_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples,
					mixBits, mixRes, mShiftBufferUV, bytesShifted );
			break;
		case 32:
			// also extracts the shifted off bytes into the shift buffers
			mix32( (int32_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples,
					mixBits, mixRes, mShiftBufferUV, bytesShifted );
			break;
	}

	/* speculatively write the bitstream assuming the compressed version will be smaller */

	// write bitstream header and coefs
	BitBufferWrite( bitstream, 0, 12 );
	BitBufferWrite( bitstream, (partialFrame << 3) | (bytesShifted << 1), 4 );
	if ( partialFrame )
		BitBufferWrite( bitstream, numSamples, 32 );
	BitBufferWrite( bitstream, mixBits, 8 );
	BitBufferWrite( bitstream, mixRes, 8 );
	
	//Assert( (mode < 16) && (DENSHIFT_DEFAULT < 16) );
	//Assert( (pbFactor < 8) && (numU < 32) );
	//Assert( (pbFactor < 8) && (numV < 32) );

	BitBufferWrite( bitstream, (mode << 4) | DENSHIFT_DEFAULT, 8 );
	BitBufferWrite( bitstream, (pbFactor << 5) | numU, 8 );
	for ( index = 0; index < numU; index++ )
		BitBufferWrite( bitstream, coefsU[numU - 1][index], 16 );

	BitBufferWrite( bitstream, (mode << 4) | DENSHIFT_DEFAULT, 8 );
	BitBufferWrite( bitstream, (pbFactor << 5) | numV, 8 );
	for ( index = 0; index < numV; index++ )
		BitBufferWrite( bitstream, coefsV[numV - 1][index], 16 );

	// if shift active, write the interleaved shift buffers
	if ( bytesShifted != 0 )
	{
		uint32_t		bitShift = bytesShifted * 8;

		//Assert( bitShift <= 16 );

		for ( index = 0; index < (numSamples * 2); index += 2 )
		{
			uint32_t			shiftedVal;
			
			shiftedVal = ((uint32_t)mShiftBufferUV[index + 0] << bitShift) | (uint32_t)mShiftBufferUV[index + 1];
			BitBufferWrite( bitstream, shiftedVal, bitShift * 2 );
		}
	}

	// run the dynamic predictor and lossless compression for the "left" channel
	// - note: we always use mode 0 in the "fast" path so we don't need the code for mode != 0
	pc_block( mMixBufferU, mPredictorU, numSamples, coefsU[numU - 1], numU, chanBits, DENSHIFT_DEFAULT );

	set_ag_params( &agParams, MB0, (pbFactor * PB0) / 4, KB0, numSamples, numSamples, MAX_RUN_DEFAULT );
	status = dyn_comp( &agParams, mPredictorU, bitstream, numSamples, chanBits, &bits1 );
	RequireNoErr( status, goto Exit; );

	// run the dynamic predictor and lossless compression for the "right" channel
	pc_block( mMixBufferV, mPredictorV, numSamples, coefsV[numV - 1], numV, chanBits, DENSHIFT_DEFAULT );

	set_ag_params( &agParams, MB0, (pbFactor * PB0) / 4, KB0, numSamples, numSamples, MAX_RUN_DEFAULT );
	status = dyn_comp( &agParams, mPredictorV, bitstream, numSamples, chanBits, &bits2 );
	RequireNoErr( status, goto Exit; );

	// do bit requirement calculations
	minBits1 = bits1 + (numU * sizeof(int16_t) * 8);
	minBits2 = bits2 + (numV * sizeof(int16_t) * 8);

	// test for escape hatch if best calculated compressed size turns out to be more than the input size
	minBits = minBits1 + minBits2 + (8 /* mixRes/maxRes/etc. */ * 8) + ((partialFrame == true) ? 32 : 0);
	if ( bytesShifted != 0 )
		minBits += (numSamples * (bytesShifted * 8) * 2);

	escapeBits = (numSamples * mBitDepth * 2) + ((partialFrame == true) ? 32 : 0) + (2 * 8);	/* 2 common header bytes */

	doEscape = (minBits >= escapeBits) ? true : false;

	if ( doEscape == false )
	{
		/*	if we happened to create a compressed packet that was actually bigger than an escape packet would be,
			chuck it and do an escape packet
		*/
		minBits = BitBufferGetPosition( bitstream ) - BitBufferGetPosition( &startBits );
		if ( minBits >= escapeBits )
		{
			doEscape = true;
			printf( "compressed frame too big: %u vs. %u\n", minBits, escapeBits );
		}

	}

	if ( doEscape == true )
	{
		/* escape */

		// reset bitstream position since we speculatively wrote the compressed version
		*bitstream = startBits;

		// write escape frame
		status = this->EncodeStereoEscape( bitstream, inputBuffer, stride, numSamples );

#if VERBOSE_DEBUG		
		DebugMsg( "escape!: %u vs %u", minBits, (numSamples * mBitDepth * 2) );
#endif
	}
	
Exit:
	return status;
}

/*
	EncodeStereoEscape()
	- encode stereo escape frame
*/
int32_t ALACEncoder::EncodeStereoEscape( BitBuffer * bitstream, void * inputBuffer, uint32_t stride, uint32_t numSamples )
{
	int16_t *		input16;
	int32_t *		input32;
	uint8_t			partialFrame;
	uint32_t			index;

	// flag whether or not this is a partial frame
	partialFrame = (numSamples == mFrameSize) ? 0 : 1;

	// write bitstream header
	BitBufferWrite( bitstream, 0, 12 );
	BitBufferWrite( bitstream, (partialFrame << 3) | 1, 4 );	// LSB = 1 means "frame not compressed"
	if ( partialFrame )
		BitBufferWrite( bitstream, numSamples, 32 );

	// just copy the input data to the output buffer
	switch ( mBitDepth )
	{
		case 16:
			input16 = (int16_t *) inputBuffer;
			
			for ( index = 0; index < (numSamples * stride); index += stride )
			{
				BitBufferWrite( bitstream, input16[index + 0], 16 );
				BitBufferWrite( bitstream, input16[index + 1], 16 );
			}
			break;
		case 20:
			// mix20() with mixres param = 0 means de-interleave so use it to simplify things
			mix20( (uint8_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples, 0, 0 );
			for ( index = 0; index < numSamples; index++ )
			{
				BitBufferWrite( bitstream, mMixBufferU[index], 20 );
				BitBufferWrite( bitstream, mMixBufferV[index], 20 );
			}				
			break;
		case 24:
			// mix24() with mixres param = 0 means de-interleave so use it to simplify things
			mix24( (uint8_t *) inputBuffer, stride, mMixBufferU, mMixBufferV, numSamples, 0, 0, mShiftBufferUV, 0 );
			for ( index = 0; index < numSamples; index++ )
			{
				BitBufferWrite( bitstream, mMixBufferU[index], 24 );
				BitBufferWrite( bitstream, mMixBufferV[index], 24 );
			}				
			break;
		case 32:
			input32 = (int32_t *) inputBuffer;

			for ( index = 0; index < (numSamples * stride); index += stride )
			{
				BitBufferWrite( bitstream, input32[index + 0], 32 );
				BitBufferWrite( bitstream, input32[index + 1], 32 );
			}				
			break;
	}
	
	return ALAC_noErr;
}

/*
	EncodeMono()
	- encode a mono input buffer
*/
int32_t ALACEncoder::EncodeMono( BitBuffer * bitstream, void * inputBuffer, uint32_t stride, uint32_t channelIndex, uint32_t numSamples )
{
	BitBuffer		startBits = *bitstream;			// squirrel away copy of current state in case we need to go back and do an escape packet
	AGParamRec		agParams;
	uint32_t	bits1;
	uint32_t			numU;
	SearchCoefs		coefsU;
	uint32_t			dilate;
	uint32_t			minBits, bestU;
	uint32_t			minU, maxU;
	uint32_t			index, index2;
	uint8_t			bytesShifted;
	uint32_t			shift;
	uint32_t			mask;
	uint32_t			chanBits;
	uint8_t			pbFactor;
	uint8_t			partialFrame;
	int16_t *		input16;
	int32_t *		input32;
	uint32_t			escapeBits;
	bool			doEscape;
	int32_t		status;

	// make sure we handle this bit-depth before we get going
	RequireAction( (mBitDepth == 16) || (mBitDepth == 20) || (mBitDepth == 24) || (mBitDepth == 32), return kALAC_ParamError; );

	status = ALAC_noErr;
	
	// reload coefs array from previous frame
	coefsU = (SearchCoefs) mCoefsU[channelIndex];

	// pick bit depth for actual encoding
	// - we lop off the lower byte(s) for 24-/32-bit encodings
	if ( mBitDepth == 32 )
		bytesShifted = 2;
	else if ( mBitDepth >= 24 )
		bytesShifted = 1;
	else
		bytesShifted = 0;

	shift = bytesShifted * 8;
	mask = (1ul << shift) - 1;
	chanBits = mBitDepth - (bytesShifted * 8);

	// flag whether or not this is a partial frame
	partialFrame = (numSamples == mFrameSize) ? 0 : 1;

	// convert N-bit data to 32-bit for predictor
	switch ( mBitDepth )
	{
		case 16:
		{
			// convert 16-bit data to 32-bit for predictor
			input16 = (int16_t *) inputBuffer;
			for ( index = 0, index2 = 0; index < numSamples; index++, index2 += stride )
				mMixBufferU[index] = (int32_t) input16[index2];
			break;
		}
		case 20:
			// convert 20-bit data to 32-bit for predictor
			copy20ToPredictor( (uint8_t *) inputBuffer, stride, mMixBufferU, numSamples );
			break;
		case 24:
			// convert 24-bit data to 32-bit for the predictor and extract the shifted off byte(s)
			copy24ToPredictor( (uint8_t *) inputBuffer, stride, mMixBufferU, numSamples );
			for ( index = 0; index < numSamples; index++ )
			{
				mShiftBufferUV[index] = (uint16_t)(mMixBufferU[index] & mask);
				mMixBufferU[index] >>= shift;
			}
			break;
		case 32:
		{
			// just copy the 32-bit input data for the predictor and extract the shifted off byte(s)
			input32 = (int32_t *) inputBuffer;

			for ( index = 0, index2 = 0; index < numSamples; index++, index2 += stride )
			{
				int32_t			val = input32[index2];
				
				mShiftBufferUV[index] = (uint16_t)(val & mask);
				mMixBufferU[index] = val >> shift;
			}
			break;
		}
	}

	// brute-force encode optimization loop (implied "encode depth" of 0 if comparing to cmd line tool)
	// - run over variations of the encoding params to find the best choice
	minU		= 4;
	maxU		= 8;
	minBits		= 1ul << 31;
	pbFactor	= 4;
	
	minBits	= 1ul << 31;
	bestU	= minU;

	for ( numU = minU; numU <= maxU; numU += 4 )
	{
		BitBuffer		workBits;
		uint32_t			numBits;

		BitBufferInit( &workBits, mWorkBuffer, mMaxOutputBytes );
	
		dilate = 32;
		for ( uint32_t converge = 0; converge < 7; converge++ )	
			pc_block( mMixBufferU, mPredictorU, numSamples/dilate, coefsU[numU-1], numU, chanBits, DENSHIFT_DEFAULT );

		dilate = 8;
		pc_block( mMixBufferU, mPredictorU, numSamples/dilate, coefsU[numU-1], numU, chanBits, DENSHIFT_DEFAULT );

		set_ag_params( &agParams, MB0, (pbFactor * PB0) / 4, KB0, numSamples/dilate, numSamples/dilate, MAX_RUN_DEFAULT );
		status = dyn_comp( &agParams, mPredictorU, &workBits, numSamples/dilate, chanBits, &bits1 );
		RequireNoErr( status, goto Exit; );

		numBits = (dilate * bits1) + (16 * numU);
		if ( numBits < minBits )
		{
			bestU	= numU;
			minBits = numBits;
		}
	}             

	// test for escape hatch if best calculated compressed size turns out to be more than the input size
	// - first, add bits for the header bytes mixRes/maxRes/shiftU/filterU
	minBits += (4 /* mixRes/maxRes/etc. */ * 8) + ((partialFrame == true) ? 32 : 0);
	if ( bytesShifted != 0 )
		minBits += (numSamples * (bytesShifted * 8));

	escapeBits = (numSamples * mBitDepth) + ((partialFrame == true) ? 32 : 0) + (2 * 8);	/* 2 common header bytes */

	doEscape = (minBits >= escapeBits) ? true : false;

	if ( doEscape == false )
	{
		// write bitstream header
		BitBufferWrite( bitstream, 0, 12 );
		BitBufferWrite( bitstream, (partialFrame << 3) | (bytesShifted << 1), 4 );
		if ( partialFrame )
			BitBufferWrite( bitstream, numSamples, 32 );
		BitBufferWrite( bitstream, 0, 16 );								// mixBits = mixRes = 0
		
		// write the params and predictor coefs
		numU = bestU;
		BitBufferWrite( bitstream, (0 << 4) | DENSHIFT_DEFAULT, 8 );	// modeU = 0
		BitBufferWrite( bitstream, (pbFactor << 5) | numU, 8 );
		for ( index = 0; index < numU; index++ )
			BitBufferWrite( bitstream, coefsU[numU-1][index], 16 );

		// if shift active, write the interleaved shift buffers
		if ( bytesShifted != 0 )
		{
			for ( index = 0; index < numSamples; index++ )
				BitBufferWrite( bitstream, mShiftBufferUV[index], shift );
		}

		// run the dynamic predictor with the best result
		pc_block( mMixBufferU, mPredictorU, numSamples, coefsU[numU-1], numU, chanBits, DENSHIFT_DEFAULT );

		// do lossless compression
		set_standard_ag_params( &agParams, numSamples, numSamples );
		status = dyn_comp( &agParams, mPredictorU, bitstream, numSamples, chanBits, &bits1 );
		//AssertNoErr( status );


		/*	if we happened to create a compressed packet that was actually bigger than an escape packet would be,
			chuck it and do an escape packet
		*/
		minBits = BitBufferGetPosition( bitstream ) - BitBufferGetPosition( &startBits );
		if ( minBits >= escapeBits )
		{
			*bitstream = startBits;		// reset bitstream state
			doEscape = true;
			printf( "compressed frame too big: %u vs. %u\n", minBits, escapeBits );
		}
	}

	if ( doEscape == true )
	{
		// write bitstream header and coefs
		BitBufferWrite( bitstream, 0, 12 );
		BitBufferWrite( bitstream, (partialFrame << 3) | 1, 4 );	// LSB = 1 means "frame not compressed"
		if ( partialFrame )
			BitBufferWrite( bitstream, numSamples, 32 );

		// just copy the input data to the output buffer
		switch ( mBitDepth )
		{
			case 16:
				input16 = (int16_t *) inputBuffer;
				for ( index = 0; index < (numSamples * stride); index += stride )
					BitBufferWrite( bitstream, input16[index], 16 );
				break;
			case 20:
				// convert 20-bit data to 32-bit for simplicity
				copy20ToPredictor( (uint8_t *) inputBuffer, stride, mMixBufferU, numSamples );
				for ( index = 0; index < numSamples; index++ )
					BitBufferWrite( bitstream, mMixBufferU[index], 20 );
				break;
			case 24:
				// convert 24-bit data to 32-bit for simplicity
				copy24ToPredictor( (uint8_t *) inputBuffer, stride, mMixBufferU, numSamples );
				for ( index = 0; index < numSamples; index++ )
					BitBufferWrite( bitstream, mMixBufferU[index], 24 );
				break;
			case 32:
				input32 = (int32_t *) inputBuffer;
				for ( index = 0; index < (numSamples * stride); index += stride )
					BitBufferWrite( bitstream, input32[index], 32 );
				break;
		}
#if VERBOSE_DEBUG		
		DebugMsg( "escape!: %lu vs %lu", minBits, (numSamples * mBitDepth) );
#endif
	}

Exit:
	return status;
}

#if PRAGMA_MARK
#pragma mark -
#endif

/*
	Encode()
	- encode the next block of samples
*/
int32_t ALACEncoder::Encode(AudioFormatDescription theInputFormat, AudioFormatDescription theOutputFormat,
                             unsigned char * theReadBuffer, unsigned char * theWriteBuffer, int32_t * ioNumBytes)
{
	uint32_t				numFrames;
	uint32_t				outputSize;
	BitBuffer			bitstream;
	int32_t			status;

	numFrames = *ioNumBytes/theInputFormat.mBytesPerPacket;

	// create a bit buffer structure pointing to our output buffer
	BitBufferInit( &bitstream, theWriteBuffer, mMaxOutputBytes );

	if ( theInputFormat.mChannelsPerFrame == 2 )
	{
		// add 3-bit frame start tag ID_CPE = channel pair & 4-bit element instance tag = 0
		BitBufferWrite( &bitstream, ID_CPE, 3 );
		BitBufferWrite( &bitstream, 0, 4 );

		// encode stereo input buffer
		if ( mFastMode == false )
			status = this->EncodeStereo( &bitstream, theReadBuffer, 2, 0, numFrames );
		else
			status = this->EncodeStereoFast( &bitstream, theReadBuffer, 2, 0, numFrames );
		RequireNoErr( status, goto Exit; );
	}
	else if ( theInputFormat.mChannelsPerFrame == 1 )
	{
		// add 3-bit frame start tag ID_SCE = mono channel & 4-bit element instance tag = 0
		BitBufferWrite( &bitstream, ID_SCE, 3 );
		BitBufferWrite( &bitstream, 0, 4 );

		// encode mono input buffer
		status = this->EncodeMono( &bitstream, theReadBuffer, 1, 0, numFrames );
		RequireNoErr( status, goto Exit; );
	}
	else
	{
		char *					inputBuffer;
		uint32_t				tag;
		uint32_t				channelIndex;
		uint32_t				inputIncrement;
		uint8_t				stereoElementTag;
		uint8_t				monoElementTag;
		uint8_t				lfeElementTag;
		
		inputBuffer		= (char *) theReadBuffer;
		inputIncrement	= ((mBitDepth + 7) / 8);
		
		stereoElementTag	= 0;
		monoElementTag		= 0;
		lfeElementTag		= 0;

		for ( channelIndex = 0; channelIndex < theInputFormat.mChannelsPerFrame; )
		{
			tag = (sChannelMaps[theInputFormat.mChannelsPerFrame - 1] & (0x7ul << (channelIndex * 3))) >> (channelIndex * 3);
	
			BitBufferWrite( &bitstream, tag, 3 );
			switch ( tag )
			{
				case ID_SCE:
					// mono
					BitBufferWrite( &bitstream, monoElementTag, 4 );

					status = this->EncodeMono( &bitstream, inputBuffer, theInputFormat.mChannelsPerFrame, channelIndex, numFrames );
					
					inputBuffer += inputIncrement;
					channelIndex++;
					monoElementTag++;
					break;

				case ID_CPE:
					// stereo
					BitBufferWrite( &bitstream, stereoElementTag, 4 );

					status = this->EncodeStereo( &bitstream, inputBuffer, theInputFormat.mChannelsPerFrame, channelIndex, numFrames );

					inputBuffer += (inputIncrement * 2);
					channelIndex += 2;
					stereoElementTag++;
					break;

				case ID_LFE:
					// LFE channel (subwoofer)
					BitBufferWrite( &bitstream, lfeElementTag, 4 );

					status = this->EncodeMono( &bitstream, inputBuffer, theInputFormat.mChannelsPerFrame, channelIndex, numFrames );

					inputBuffer += inputIncrement;
					channelIndex++;
					lfeElementTag++;
					break;

				default:
					printf( "That ain't right! (%u)\n", tag );
					status = kALAC_ParamError;
					goto Exit;
			}

			RequireNoErr( status, goto Exit; );
		}
	}

#if VERBOSE_DEBUG
{
	// if there is room left in the output buffer, add some random fill data to test decoder
	int32_t			bitsLeft;
	int32_t			bytesLeft;
	
	bitsLeft = BitBufferGetPosition( &bitstream ) - 3;	// - 3 for ID_END tag
	bytesLeft = bitstream.byteSize - ((bitsLeft + 7) / 8);
	
	if ( (bytesLeft > 20) && ((bytesLeft & 0x4u) != 0) )
		AddFiller( &bitstream, bytesLeft );
}
#endif

	// add 3-bit frame end tag: ID_END
	BitBufferWrite( &bitstream, ID_END, 3 );

	// byte-align the output data
	BitBufferByteAlign( &bitstream, true );

	outputSize = BitBufferGetPosition( &bitstream ) / 8;
	//Assert( outputSize <= mMaxOutputBytes );


	// all good, let iTunes know what happened and remember the total number of input sample frames
	*ioNumBytes = outputSize;
	//mEncodedFrames		   	   += encodeMsg->numInputSamples;

	// gather encoding stats
	mTotalBytesGenerated += outputSize;
	mMaxFrameBytes = MAX( mMaxFrameBytes, outputSize );

	status = ALAC_noErr;

Exit:
	return status;
}

/*
	Finish()
	- drain out any leftover samples
*/

int32_t ALACEncoder::Finish()
{
/*	// finalize bit rate statistics
	if ( mSampleSize.numEntries != 0 )
		mAvgBitRate = (uint32_t)( (((float)mTotalBytesGenerated * 8.0f) / (float)mSampleSize.numEntries) * ((float)mSampleRate / (float)mFrameSize) );
	else
		mAvgBitRate = 0;
*/
	return ALAC_noErr;
}

#if PRAGMA_MARK
#pragma mark -
#endif

/*
	GetConfig()
*/
void ALACEncoder::GetConfig( ALACSpecificConfig & config )
{
	config.frameLength			= Swap32NtoB(mFrameSize);
	config.compatibleVersion	= (uint8_t) kALACCompatibleVersion;
	config.bitDepth				= (uint8_t) mBitDepth;
	config.pb					= (uint8_t) PB0;
	config.kb					= (uint8_t) KB0;
	config.mb					= (uint8_t) MB0;
	config.numChannels			= (uint8_t) mNumChannels;
	config.maxRun				= Swap16NtoB((uint16_t) MAX_RUN_DEFAULT);
	config.maxFrameBytes		= Swap32NtoB(mMaxFrameBytes);
	config.avgBitRate			= Swap32NtoB(mAvgBitRate);
	config.sampleRate			= Swap32NtoB(mOutputSampleRate);
}

uint32_t ALACEncoder::GetMagicCookieSize(uint32_t inNumChannels)
{
    if (inNumChannels > 2)
    {
        return sizeof(ALACSpecificConfig) + kChannelAtomSize + sizeof(ALACAudioChannelLayout);
    }
    else
    {
        return sizeof(ALACSpecificConfig);
    }
}

void ALACEncoder::GetMagicCookie(void * outCookie, uint32_t * ioSize)
{
    ALACSpecificConfig theConfig = {0};
    ALACAudioChannelLayout theChannelLayout = {0};
    uint8_t theChannelAtom[kChannelAtomSize] = {0, 0, 0, 0, 'c', 'h', 'a', 'n', 0, 0, 0, 0};
    uint32_t theCookieSize = sizeof(ALACSpecificConfig);
    uint8_t * theCookiePointer = (uint8_t *)outCookie;
    
    GetConfig(theConfig);
    if (theConfig.numChannels > 2)
    {
        theChannelLayout.mChannelLayoutTag = ALACChannelLayoutTags[theConfig.numChannels - 1];
        theCookieSize += (sizeof(ALACAudioChannelLayout) + kChannelAtomSize);
    }
     if (*ioSize >= theCookieSize)
    {
        memcpy(theCookiePointer, &theConfig, sizeof(ALACSpecificConfig));
        theChannelAtom[3] = (sizeof(ALACAudioChannelLayout) + kChannelAtomSize);
        if (theConfig.numChannels > 2)
        {
            theCookiePointer += sizeof(ALACSpecificConfig);
            memcpy(theCookiePointer, theChannelAtom, kChannelAtomSize);
            theCookiePointer += kChannelAtomSize;
            memcpy(theCookiePointer, &theChannelLayout, sizeof(ALACAudioChannelLayout));
        }
        *ioSize = theCookieSize;
    }
    else
    {
        *ioSize = 0; // no incomplete cookies
    }
}

/*
	InitializeEncoder()
	- initialize the encoder component with the current config
*/
int32_t ALACEncoder::InitializeEncoder(AudioFormatDescription theOutputFormat)
{
	int32_t			status;
    
    mOutputSampleRate = theOutputFormat.mSampleRate;
    mNumChannels = theOutputFormat.mChannelsPerFrame;
    switch(theOutputFormat.mFormatFlags)
    {
        case 1:
            mBitDepth = 16;
            break;
        case 2:
            mBitDepth = 20;
            break;
        case 3:
            mBitDepth = 24;
            break;
        case 4:
            mBitDepth = 32;
            break;
        default:
            break;
    }

	// set up default encoding parameters and state
	// - note: mFrameSize is set in the constructor or via SetFrameSize() which must be called before this routine
	for ( uint32_t index = 0; index < kALACMaxChannels; index++ )
		mLastMixRes[index] = kDefaultMixRes;

	// the maximum output frame size can be no bigger than (samplesPerBlock * numChannels * ((10 + sampleSize)/8) + 1)
	// but note that this can be bigger than the input size!
	// - since we don't yet know what our input format will be, use our max allowed sample size in the calculation
	mMaxOutputBytes = mFrameSize * mNumChannels * ((10 + kMaxSampleSize) / 8)  + 1;

	// allocate mix buffers
	mMixBufferU = (int32_t *) calloc( mFrameSize * sizeof(int32_t), 1 );
	mMixBufferV = (int32_t *) calloc( mFrameSize * sizeof(int32_t), 1 );

	// allocate dynamic predictor buffers
	mPredictorU = (int32_t *) calloc( mFrameSize * sizeof(int32_t), 1 );
	mPredictorV = (int32_t *) calloc( mFrameSize * sizeof(int32_t), 1 );
	
	// allocate combined shift buffer
	mShiftBufferUV = (uint16_t *) calloc( mFrameSize * 2 * sizeof(uint16_t),1 );
	
	// allocate work buffer for search loop
	mWorkBuffer = (uint8_t *) calloc( mMaxOutputBytes, 1 );

	RequireAction( (mMixBufferU != nil) && (mMixBufferV != nil) &&
					(mPredictorU != nil) && (mPredictorV != nil) &&
					(mShiftBufferUV != nil) && (mWorkBuffer != nil ),
					status = kALAC_MemFullError; goto Exit; );

	status = ALAC_noErr;


	// initialize coefs arrays once b/c retaining state across blocks actually improves the encode ratio
	for ( int32_t channel = 0; channel < (int32_t)mNumChannels; channel++ )
	{
		for ( int32_t search = 0; search < kALACMaxSearches; search++ )
		{
			init_coefs( mCoefsU[channel][search], DENSHIFT_DEFAULT, kALACMaxCoefs );
			init_coefs( mCoefsV[channel][search], DENSHIFT_DEFAULT, kALACMaxCoefs );
		}
	}

Exit:
	return status;
}

/*
	GetSourceFormat()
	- given the input format, return one of our supported formats
*/
void ALACEncoder::GetSourceFormat( const AudioFormatDescription * source, AudioFormatDescription * output )
{
	// default is 16-bit native endian
	// - note: for float input we assume that's coming from one of our decoders (mp3, aac) so it only makes sense
	//		   to encode to 16-bit since the source was lossy in the first place
	// - note: if not a supported bit depth, find the closest supported bit depth to the input one
	if ( (source->mFormatID != kALACFormatLinearPCM) || ((source->mFormatFlags & kALACFormatFlagIsFloat) != 0) ||
		( source->mBitsPerChannel <= 16 ) )
		mBitDepth = 16;
	else if ( source->mBitsPerChannel <= 20 )
		mBitDepth = 20;
	else if ( source->mBitsPerChannel <= 24 )
		mBitDepth = 24;
	else
		mBitDepth = 32;
		
	// we support 16/20/24/32-bit integer data at any sample rate and our target number of channels
	// and sample rate were specified when we were configured
	/*
    MakeUncompressedAudioFormat( mNumChannels, (float) mOutputSampleRate, mBitDepth, kAudioFormatFlagsNativeIntegerPacked, output );
     */
}



#if VERBOSE_DEBUG

#if PRAGMA_MARK
#pragma mark -
#endif

/*
	AddFiller()
	- add fill and data stream elements to the bitstream to test the decoder
*/
static void AddFiller( BitBuffer * bits, int32_t numBytes )
{
	uint8_t		tag;
	uint32_t		index;

	// out of lameness, subtract 6 bytes to deal with header + alignment as required for fill/data elements
	numBytes -= 6;
	if ( numBytes <= 0 )
		return;
	
	// randomly pick Fill or Data Stream Element based on numBytes requested
	tag = (numBytes & 0x8) ? ID_FIL : ID_DSE;

	BitBufferWrite( bits, tag, 3 );
	if ( tag == ID_FIL )
	{
		// can't write more than 269 bytes in a fill element
		numBytes = (numBytes > 269) ? 269 : numBytes;

		// fill element = 4-bit size unless >= 15 then 4-bit size + 8-bit extension size
		if ( numBytes >= 15 )
		{
			uint16_t			extensionSize;

			BitBufferWrite( bits, 15, 4 );

			// 8-bit extension count field is "extra + 1" which is weird but I didn't define the syntax
			// - otherwise, there's no way to represent 15
			// - for example, to really mean 15 bytes you must encode extensionSize = 1
			// - why it's not like data stream elements I have no idea
			extensionSize = (numBytes - 15) + 1;
			Assert( extensionSize <= 255 );
			BitBufferWrite( bits, extensionSize, 8 );
		}
		else
			BitBufferWrite( bits, numBytes, 4 );

		BitBufferWrite( bits, 0x10, 8 );		// extension_type = FILL_DATA = b0001 or'ed with fill_nibble = b0000
		for ( index = 0; index < (numBytes - 1); index++ )
			BitBufferWrite( bits, 0xa5, 8 );	// fill_byte = b10100101 = 0xa5
	}
	else
	{
		// can't write more than 510 bytes in a data stream element
		numBytes = (numBytes > 510) ? 510 : numBytes;

		BitBufferWrite( bits, 0, 4 );			// element instance tag
		BitBufferWrite( bits, 1, 1 );			// byte-align flag = true

		// data stream element = 8-bit size unless >= 255 then 8-bit size + 8-bit size
		if ( numBytes >= 255 )
		{
			BitBufferWrite( bits, 255, 8 );
			BitBufferWrite( bits, numBytes - 255, 8 );
		}
		else
			BitBufferWrite( bits, numBytes, 8 );
		
		BitBufferByteAlign( bits, true );		// byte-align with zeros

		for ( index = 0; index < numBytes; index++ )
			BitBufferWrite( bits, 0x5a, 8 );
	}
}

#endif	/* VERBOSE_DEBUG */
