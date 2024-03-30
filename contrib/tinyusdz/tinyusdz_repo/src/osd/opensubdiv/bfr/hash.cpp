//
//   Copyright 2021 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

//
// Functions for hashing to 32- and 64-bit integers. These are the same
// functions used in USD (pxr/base/arch/hash.*) under similar conditions.
//

#include "hash.h"

//
// SpookyHash: a 128-bit noncryptographic hash function
// By Bob Jenkins, public domain
//   Oct 31 2010: alpha, framework + SpookyHash::Mix appears right
//   Oct 31 2011: alpha again, Mix only good to 2^^69 but rest appears right
//   Dec 31 2011: beta, improved Mix, tested it for 2-bit deltas
//   Feb  2 2012: production, same bits as beta
//   Feb  5 2012: adjusted definitions of uint* to be more portable
//   Mar 30 2012: 3 bytes/cycle, not 4.  Alpha was 4 but wasn't thorough enough.
//   August 5 2012: SpookyV2 (different results)
// 
// Up to 3 bytes/cycle for long messages.  Reasonably fast for short messages.
// All 1 or 2 bit deltas achieve avalanche within 1% bias per output bit.
//
// This was developed for and tested on 64-bit x86-compatible processors.
// It assumes the processor is little-endian.  There is a macro
// controlling whether unaligned reads are allowed (by default they are).
// This should be an equally good hash on big-endian machines, but it will
// compute different results on them than on little-endian machines.
//
// Google's CityHash has similar specs to SpookyHash, and CityHash is faster
// on new Intel boxes.  MD4 and MD5 also have similar specs, but they are orders
// of magnitude slower.  CRCs are two or more times slower, but unlike 
// SpookyHash, they have nice math for combining the CRCs of pieces to form 
// the CRCs of wholes.  There are also cryptographic hashes, but those are even 
// slower than MD5.
//

#include <cstring>
#include <cstddef>

//  Local typedefs to match the original code:
typedef  uint64_t  uint64;
typedef  uint32_t  uint32;
typedef  uint16_t  uint16;
typedef  uint8_t   uint8;

namespace {

class SpookyHash
{
public:
    //
    // SpookyHash: hash a single message in one call, produce 128-bit output
    //
    static void Hash128(
        const void *message,  // message to hash
        size_t length,        // length of message in bytes
        uint64 *hash1,        // in/out: in seed 1, out hash value 1
        uint64 *hash2);       // in/out: in seed 2, out hash value 2

    //
    // Hash64: hash a single message in one call, return 64-bit output
    //
    static uint64 Hash64(
        const void *message,  // message to hash
        size_t length,        // length of message in bytes
        uint64 seed)          // seed
    {
        uint64 hash1 = seed;
        Hash128(message, length, &hash1, &seed);
        return hash1;
    }

    //
    // Hash32: hash a single message in one call, produce 32-bit output
    //
    static uint32 Hash32(
        const void *message,  // message to hash
        size_t length,        // length of message in bytes
        uint32 seed)          // seed
    {
        uint64 hash1 = seed, hash2 = seed;
        Hash128(message, length, &hash1, &hash2);
        return (uint32)hash1;
    }

#ifdef OPENSUBDIV3_BFR_HASH_INCLUDE_UNUSED_FUNCTIONS
    //
    // Init: initialize the context of a SpookyHash
    //
    void Init(
        uint64 seed1,       // any 64-bit value will do, including 0
        uint64 seed2);      // different seeds produce independent hashes
    
    //
    // Update: add a piece of a message to a SpookyHash state
    //
    void Update(
        const void *message,  // message fragment
        size_t length);       // length of message fragment in bytes


    //
    // Final: compute the hash for the current SpookyHash state
    //
    // This does not modify the state; you can keep updating it afterward
    //
    // The result is the same as if SpookyHash() had been called with
    // all the pieces concatenated into one message.
    //
    void Final(
        uint64 *hash1,    // out only: first 64 bits of hash value.
        uint64 *hash2);   // out only: second 64 bits of hash value.
#endif

    //
    // left rotate a 64-bit value by k bytes
    //
    static inline uint64 Rot64(uint64 x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }

    //
    // This is used if the input is 96 bytes long or longer.
    //
    // The internal state is fully overwritten every 96 bytes.
    // Every input bit appears to cause at least 128 bits of entropy
    // before 96 other bytes are combined, when run forward or backward
    //   For every input bit,
    //   Two inputs differing in just that input bit
    //   Where "differ" means xor or subtraction
    //   And the base value is random
    //   When run forward or backwards one Mix
    // I tried 3 pairs of each; they all differed by at least 212 bits.
    //
    static inline void Mix(
        const uint64 *data, 
        uint64 &s0, uint64 &s1, uint64 &s2, uint64 &s3,
        uint64 &s4, uint64 &s5, uint64 &s6, uint64 &s7,
        uint64 &s8, uint64 &s9, uint64 &s10,uint64 &s11)
    {
      s0 += data[0];   s2 ^= s10; s11 ^= s0;  s0 = Rot64(s0,11);   s11 += s1;
      s1 += data[1];   s3 ^= s11; s0 ^= s1;   s1 = Rot64(s1,32);   s0 += s2;
      s2 += data[2];   s4 ^= s0;  s1 ^= s2;   s2 = Rot64(s2,43);   s1 += s3;
      s3 += data[3];   s5 ^= s1;  s2 ^= s3;   s3 = Rot64(s3,31);   s2 += s4;
      s4 += data[4];   s6 ^= s2;  s3 ^= s4;   s4 = Rot64(s4,17);   s3 += s5;
      s5 += data[5];   s7 ^= s3;  s4 ^= s5;   s5 = Rot64(s5,28);   s4 += s6;
      s6 += data[6];   s8 ^= s4;  s5 ^= s6;   s6 = Rot64(s6,39);   s5 += s7;
      s7 += data[7];   s9 ^= s5;  s6 ^= s7;   s7 = Rot64(s7,57);   s6 += s8;
      s8 += data[8];   s10 ^= s6; s7 ^= s8;   s8 = Rot64(s8,55);   s7 += s9;
      s9 += data[9];   s11 ^= s7; s8 ^= s9;   s9 = Rot64(s9,54);   s8 += s10;
      s10 += data[10]; s0 ^= s8;  s9 ^= s10;  s10 = Rot64(s10,22); s9 += s11;
      s11 += data[11]; s1 ^= s9;  s10 ^= s11; s11 = Rot64(s11,46); s10 += s0;
    }

    //
    // Mix all 12 inputs together so that h0, h1 are a hash of them all.
    //
    // For two inputs differing in just the input bits
    // Where "differ" means xor or subtraction
    // And the base value is random, or a counting value starting at that bit
    // The final result will have each bit of h0, h1 flip
    // For every input bit,
    // with probability 50 +- .3%
    // For every pair of input bits,
    // with probability 50 +- 3%
    //
    // This does not rely on the last Mix() call having already mixed some.
    // Two iterations was almost good enough for a 64-bit result, but a
    // 128-bit result is reported, so End() does three iterations.
    //
    static inline void EndPartial(
        uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3,
        uint64 &h4, uint64 &h5, uint64 &h6, uint64 &h7, 
        uint64 &h8, uint64 &h9, uint64 &h10,uint64 &h11)
    {
        h11+= h1;    h2 ^= h11;   h1 = Rot64(h1,44);
        h0 += h2;    h3 ^= h0;    h2 = Rot64(h2,15);
        h1 += h3;    h4 ^= h1;    h3 = Rot64(h3,34);
        h2 += h4;    h5 ^= h2;    h4 = Rot64(h4,21);
        h3 += h5;    h6 ^= h3;    h5 = Rot64(h5,38);
        h4 += h6;    h7 ^= h4;    h6 = Rot64(h6,33);
        h5 += h7;    h8 ^= h5;    h7 = Rot64(h7,10);
        h6 += h8;    h9 ^= h6;    h8 = Rot64(h8,13);
        h7 += h9;    h10^= h7;    h9 = Rot64(h9,38);
        h8 += h10;   h11^= h8;    h10= Rot64(h10,53);
        h9 += h11;   h0 ^= h9;    h11= Rot64(h11,42);
        h10+= h0;    h1 ^= h10;   h0 = Rot64(h0,54);
    }

    static inline void End(
        const uint64 *data, 
        uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3,
        uint64 &h4, uint64 &h5, uint64 &h6, uint64 &h7, 
        uint64 &h8, uint64 &h9, uint64 &h10,uint64 &h11)
    {
        h0 += data[0];   h1 += data[1];   h2 += data[2];   h3 += data[3];
        h4 += data[4];   h5 += data[5];   h6 += data[6];   h7 += data[7];
        h8 += data[8];   h9 += data[9];   h10 += data[10]; h11 += data[11];
        EndPartial(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
        EndPartial(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
        EndPartial(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    }

    //
    // The goal is for each bit of the input to expand into 128 bits of 
    //   apparent entropy before it is fully overwritten.
    // n trials both set and cleared at least m bits of h0 h1 h2 h3
    //   n: 2   m: 29
    //   n: 3   m: 46
    //   n: 4   m: 57
    //   n: 5   m: 107
    //   n: 6   m: 146
    //   n: 7   m: 152
    // when run forwards or backwards
    // for all 1-bit and 2-bit diffs
    // with diffs defined by either xor or subtraction
    // with a base of all zeros plus a counter, or plus another bit, or random
    //
    static inline void ShortMix(uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3)
    {
        h2 = Rot64(h2,50);  h2 += h3;  h0 ^= h2;
        h3 = Rot64(h3,52);  h3 += h0;  h1 ^= h3;
        h0 = Rot64(h0,30);  h0 += h1;  h2 ^= h0;
        h1 = Rot64(h1,41);  h1 += h2;  h3 ^= h1;
        h2 = Rot64(h2,54);  h2 += h3;  h0 ^= h2;
        h3 = Rot64(h3,48);  h3 += h0;  h1 ^= h3;
        h0 = Rot64(h0,38);  h0 += h1;  h2 ^= h0;
        h1 = Rot64(h1,37);  h1 += h2;  h3 ^= h1;
        h2 = Rot64(h2,62);  h2 += h3;  h0 ^= h2;
        h3 = Rot64(h3,34);  h3 += h0;  h1 ^= h3;
        h0 = Rot64(h0,5);   h0 += h1;  h2 ^= h0;
        h1 = Rot64(h1,36);  h1 += h2;  h3 ^= h1;
    }

    //
    // Mix all 4 inputs together so that h0, h1 are a hash of them all.
    //
    // For two inputs differing in just the input bits
    // Where "differ" means xor or subtraction
    // And the base value is random, or a counting value starting at that bit
    // The final result will have each bit of h0, h1 flip
    // For every input bit,
    // with probability 50 +- .3% (it is probably better than that)
    // For every pair of input bits,
    // with probability 50 +- .75% (the worst case is approximately that)
    //
    static inline void ShortEnd(uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3)
    {
        h3 ^= h2;  h2 = Rot64(h2,15);  h3 += h2;
        h0 ^= h3;  h3 = Rot64(h3,52);  h0 += h3;
        h1 ^= h0;  h0 = Rot64(h0,26);  h1 += h0;
        h2 ^= h1;  h1 = Rot64(h1,51);  h2 += h1;
        h3 ^= h2;  h2 = Rot64(h2,28);  h3 += h2;
        h0 ^= h3;  h3 = Rot64(h3,9);   h0 += h3;
        h1 ^= h0;  h0 = Rot64(h0,47);  h1 += h0;
        h2 ^= h1;  h1 = Rot64(h1,54);  h2 += h1;
        h3 ^= h2;  h2 = Rot64(h2,32);  h3 += h2;
        h0 ^= h3;  h3 = Rot64(h3,25);  h0 += h3;
        h1 ^= h0;  h0 = Rot64(h0,63);  h1 += h0;
    }
    
private:

    //
    // Short is used for messages under 192 bytes in length
    // Short has a low startup cost, the normal mode is good for long
    // keys, the cost crossover is at about 192 bytes.  The two modes were
    // held to the same quality bar.
    // 
    static void Short(
        const void *message, // message (array of bytes, not necessarily
                             // aligned)
        size_t length,       // length of message (in bytes)
        uint64 *hash1,       // in/out: in the seed, out the hash value
        uint64 *hash2);      // in/out: in the seed, out the hash value

    // number of uint64's in internal state
    static const size_t sc_numVars = 12;

    // size of the internal state
    static const size_t sc_blockSize = sc_numVars*8;

    // size of buffer of unhashed data, in bytes
    static const size_t sc_bufSize = 2*sc_blockSize;

    //
    // sc_const: a constant which:
    //  * is not zero
    //  * is odd
    //  * is a not-very-regular mix of 1's and 0's
    //  * does not need any other special mathematical properties
    //
    static const uint64 sc_const = 0xdeadbeefdeadbeefLL;

    uint64 m_data[2*sc_numVars];   // unhashed data, for partial messages
    uint64 m_state[sc_numVars];  // internal state of the hash
    size_t m_length;             // total length of the input so far
    uint8  m_remainder;          // length of unhashed data stashed in m_data
};

#define ALLOW_UNALIGNED_READS 1

//
// short hash ... it could be used on any message, 
// but it's used by Spooky just for short messages.
//
void SpookyHash::Short(
    const void *message,
    size_t length,
    uint64 *hash1,
    uint64 *hash2)
{
    uint64 buf[2*sc_numVars];
    union 
    { 
        const uint8 *p8; 
        uint32 *p32;
        uint64 *p64; 
        size_t i; 
    } u;

    u.p8 = (const uint8 *)message;
    
    if (!ALLOW_UNALIGNED_READS && (u.i & 0x7))
    {
        memcpy(buf, message, length);
        u.p64 = buf;
    }

    size_t remainder = length%32;
    uint64 a=*hash1;
    uint64 b=*hash2;
    uint64 c=sc_const;
    uint64 d=sc_const;

    if (length > 15)
    {
        const uint64 *end = u.p64 + (length/32)*4;
        
        // handle all complete sets of 32 bytes
        for (; u.p64 < end; u.p64 += 4)
        {
            c += u.p64[0];
            d += u.p64[1];
            ShortMix(a,b,c,d);
            a += u.p64[2];
            b += u.p64[3];
        }
        
        //Handle the case of 16+ remaining bytes.
        if (remainder >= 16)
        {
            c += u.p64[0];
            d += u.p64[1];
            ShortMix(a,b,c,d);
            u.p64 += 2;
            remainder -= 16;
        }
    }
    
    // Handle the last 0..15 bytes, and its length
    d += ((uint64)length) << 56;
    switch (remainder)
    {
    case 15:
    d += ((uint64)u.p8[14]) << 48;
        // FALLTHRU
    case 14:
        d += ((uint64)u.p8[13]) << 40;
        // FALLTHRU
    case 13:
        d += ((uint64)u.p8[12]) << 32;
        // FALLTHRU
    case 12:
        d += u.p32[2];
        c += u.p64[0];
        break;
    case 11:
        d += ((uint64)u.p8[10]) << 16;
        // FALLTHRU
    case 10:
        d += ((uint64)u.p8[9]) << 8;
        // FALLTHRU
    case 9:
        d += (uint64)u.p8[8];
        // FALLTHRU
    case 8:
        c += u.p64[0];
        break;
    case 7:
        c += ((uint64)u.p8[6]) << 48;
        // FALLTHRU
    case 6:
        c += ((uint64)u.p8[5]) << 40;
        // FALLTHRU
    case 5:
        c += ((uint64)u.p8[4]) << 32;
        // FALLTHRU
    case 4:
        c += u.p32[0];
        break;
    case 3:
        c += ((uint64)u.p8[2]) << 16;
        // FALLTHRU
    case 2:
        c += ((uint64)u.p8[1]) << 8;
        // FALLTHRU
    case 1:
        c += (uint64)u.p8[0];
        break;
    case 0:
        c += sc_const;
        d += sc_const;
    }
    ShortEnd(a,b,c,d);
    *hash1 = a;
    *hash2 = b;
}

// do the whole hash in one call
void SpookyHash::Hash128(
    const void *message, 
    size_t length, 
    uint64 *hash1, 
    uint64 *hash2)
{
    if (length < sc_bufSize)
    {
        Short(message, length, hash1, hash2);
        return;
    }

    uint64 h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11;
    uint64 buf[sc_numVars];
    uint64 *end;
    union 
    { 
        const uint8 *p8; 
        uint64 *p64; 
        size_t i; 
    } u;
    size_t remainder;
    
    h0=h3=h6=h9  = *hash1;
    h1=h4=h7=h10 = *hash2;
    h2=h5=h8=h11 = sc_const;
    
    u.p8 = (const uint8 *)message;
    end = u.p64 + (length/sc_blockSize)*sc_numVars;

    // handle all whole sc_blockSize blocks of bytes
    if (ALLOW_UNALIGNED_READS || ((u.i & 0x7) == 0))
    {
        while (u.p64 < end)
        { 
            Mix(u.p64, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
            u.p64 += sc_numVars;
        }
    }
    else
    {
        while (u.p64 < end)
        {
            memcpy(buf, u.p64, sc_blockSize);
            Mix(buf, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
            u.p64 += sc_numVars;
        }
    }

    // handle the last partial block of sc_blockSize bytes
    remainder = (length - ((const uint8 *)end-(const uint8 *)message));
    memcpy(buf, end, remainder);
    memset(((uint8 *)buf)+remainder, 0, sc_blockSize-remainder);
    ((uint8 *)buf)[sc_blockSize-1] = static_cast<uint8>(remainder);
    
    // do some final mixing 
    End(buf, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    *hash1 = h0;
    *hash2 = h1;
}

#ifdef OPENSUBDIV3_BFR_HASH_INCLUDE_UNUSED_FUNCTIONS
// init spooky state
void SpookyHash::Init(uint64 seed1, uint64 seed2)
{
    m_length = 0;
    m_remainder = 0;
    m_state[0] = seed1;
    m_state[1] = seed2;
}

// add a message fragment to the state
void SpookyHash::Update(const void *message, size_t length)
{
    uint64 h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11;
    size_t newLength = length + m_remainder;
    uint8  remainder;
    union 
    { 
        const uint8 *p8; 
        uint64 *p64; 
        size_t i; 
    } u;
    const uint64 *end;
    
    // Is this message fragment too short?  If it is, stuff it away.
    if (newLength < sc_bufSize)
    {
        memcpy(&((uint8 *)m_data)[m_remainder], message, length);
        m_length = length + m_length;
        m_remainder = (uint8)newLength;
        return;
    }
    
    // init the variables
    if (m_length < sc_bufSize)
    {
        h0=h3=h6=h9  = m_state[0];
        h1=h4=h7=h10 = m_state[1];
        h2=h5=h8=h11 = sc_const;
    }
    else
    {
        h0 = m_state[0];
        h1 = m_state[1];
        h2 = m_state[2];
        h3 = m_state[3];
        h4 = m_state[4];
        h5 = m_state[5];
        h6 = m_state[6];
        h7 = m_state[7];
        h8 = m_state[8];
        h9 = m_state[9];
        h10 = m_state[10];
        h11 = m_state[11];
    }
    m_length = length + m_length;
    
    // if we've got anything stuffed away, use it now
    if (m_remainder)
    {
        uint8 prefix = sc_bufSize-m_remainder;
        memcpy(&(((uint8 *)m_data)[m_remainder]), message, prefix);
        u.p64 = m_data;
        Mix(u.p64, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
        Mix(&u.p64[sc_numVars], h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
        u.p8 = ((const uint8 *)message) + prefix;
        length -= prefix;
    }
    else
    {
        u.p8 = (const uint8 *)message;
    }
    
    // handle all whole blocks of sc_blockSize bytes
    end = u.p64 + (length/sc_blockSize)*sc_numVars;
    remainder = (uint8)(length-((const uint8 *)end-u.p8));
    if (ALLOW_UNALIGNED_READS || (u.i & 0x7) == 0)
    {
        while (u.p64 < end)
        { 
            Mix(u.p64, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
            u.p64 += sc_numVars;
        }
    }
    else
    {
        while (u.p64 < end)
        { 
            memcpy(m_data, u.p8, sc_blockSize);
            Mix(m_data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
            u.p64 += sc_numVars;
        }
    }

    // stuff away the last few bytes
    m_remainder = remainder;
    memcpy(m_data, end, remainder);
    
    // stuff away the variables
    m_state[0] = h0;
    m_state[1] = h1;
    m_state[2] = h2;
    m_state[3] = h3;
    m_state[4] = h4;
    m_state[5] = h5;
    m_state[6] = h6;
    m_state[7] = h7;
    m_state[8] = h8;
    m_state[9] = h9;
    m_state[10] = h10;
    m_state[11] = h11;
}

// report the hash for the concatenation of all message fragments so far
void SpookyHash::Final(uint64 *hash1, uint64 *hash2)
{
    // init the variables
    if (m_length < sc_bufSize)
    {
        *hash1 = m_state[0];
        *hash2 = m_state[1];
        Short( m_data, m_length, hash1, hash2);
        return;
    }
    
    const uint64 *data = (const uint64 *)m_data;
    uint8 remainder = m_remainder;
    
    uint64 h0 = m_state[0];
    uint64 h1 = m_state[1];
    uint64 h2 = m_state[2];
    uint64 h3 = m_state[3];
    uint64 h4 = m_state[4];
    uint64 h5 = m_state[5];
    uint64 h6 = m_state[6];
    uint64 h7 = m_state[7];
    uint64 h8 = m_state[8];
    uint64 h9 = m_state[9];
    uint64 h10 = m_state[10];
    uint64 h11 = m_state[11];

    if (remainder >= sc_blockSize)
    {
        // m_data can contain two blocks; handle any whole first block
        Mix(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
        data += sc_numVars;
        remainder -= sc_blockSize;
    }

    // mix in the last partial block, and the length mod sc_blockSize
    memset(&((uint8 *)data)[remainder], 0, (sc_blockSize-remainder));

    ((uint8 *)data)[sc_blockSize-1] = remainder;
    
    // do some final mixing
    End(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);

    *hash1 = h0;
    *hash2 = h1;
}
#endif

} // anon


//
//  Public functions exposed for OpenSubdiv:
//
namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {
namespace Bfr {
namespace internal {

uint32_t
Hash32(const void *data, size_t len)
{
    return SpookyHash::Hash32(data, len, /*seed=*/0);
}

uint32_t
Hash32(const void *data, size_t len, uint32_t seed)
{
    return SpookyHash::Hash32(data, len, seed);
}

uint64_t
Hash64(const void *data, size_t len)
{
    return SpookyHash::Hash64(data, len, /*seed=*/0);
}

uint64_t
Hash64(const void *data, size_t len, uint64_t seed)
{
    return SpookyHash::Hash64(data, len, seed);
}

} // end namespace internal
} // end namespace Bfr
} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
