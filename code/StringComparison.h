/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

/** @file Definition of platform independent string comparison functions */
#ifndef AI_STRINGCOMPARISON_H_INC
#define AI_STRINGCOMPARISON_H_INC


namespace Assimp
{

// ---------------------------------------------------------------------------
// itoa is not consistently available on all platforms so it is quite useful
// to have a small replacement function here. No need to use a full sprintf()
// if we just want to print a number ...
// @param out Output buffer
// @param max Maximum number of characters to be written, including '\0'
// @param number Number to be written
// @return Number of bytes written. Including '\0'.
inline unsigned int itoa10( char* out, unsigned int max, int32_t number)
{
	ai_assert(NULL != out);

	static const char lookup[] = {'0','1','2','3','4','5','6','7','8','9'};

	// write the unary minus to indicate we have a negative number
	unsigned int written = 1u;
	if (number < 0 && written < max)
	{
		*out++ = '-';
		++written;
	}

	// We begin with the largest number that is not zero. 
	int32_t cur = 1000000000; // 2147483648
	bool mustPrint = false;
	while (cur > 0 && written <= max)
	{
		unsigned int digit = number / cur;
		if (digit > 0 || mustPrint || 1 == cur)
		{
			// print all future zero's from now
			mustPrint = true;	
			*out++ = lookup[digit];

			++written;
			number -= digit*10;
		}
		cur /= 10;
	}

	// append a terminal zero
	*out++ = '\0';
	return written;
}

// ---------------------------------------------------------------------------
// Secure template overload
// The compiler should choose this function if he is able to determine the
// size of the array automatically.
template <unsigned int length>
inline unsigned int itoa10( char(& out)[length], int32_t number)
{
	return itoa10(out,length,number);
}

// ---------------------------------------------------------------------------
/** \brief Helper function to do platform independent string comparison.
 *
 *  This is required since stricmp() is not consistently available on
 *  all platforms. Some platforms use the '_' prefix, others don't even
 *  have such a function. 
 *
 *  \param s1 First input string
 *  \param s2 Second input string
 */
inline int ASSIMP_stricmp(const char *s1, const char *s2)
{
#if (defined _MSC_VER)

	return ::_stricmp(s1,s2);

#elif defined( __GNUC__ )

	return ::strcasecmp(s1,s2);

#else
	register char c1, c2;
	do 
	{
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
	} 
	while ( c1 && (c1 == c2) );

	return c1 - c2;
#endif
}

// ---------------------------------------------------------------------------
/** \brief Case independent comparison of two std::strings
 */
inline int ASSIMP_stricmp(const std::string& a, const std::string& b)
{
	register int i = (int)b.length()-(int)a.length();
	return (i ? i : ASSIMP_stricmp(a.c_str(),b.c_str()));
}

// ---------------------------------------------------------------------------
/** \brief Helper function to do platform independent string comparison.
 *
 *  This is required since strincmp() is not consistently available on
 *  all platforms. Some platforms use the '_' prefix, others don't even
 *  have such a function. 
 *
 *  \param s1 First input string
 *  \param s2 Second input string
 *  \param n Macimum number of characters to compare
 */
inline int ASSIMP_strincmp(const char *s1, const char *s2, unsigned int n)
{
#if (defined _MSC_VER)

	return ::_strnicmp(s1,s2,n);

#elif defined( __GNUC__ )

	return ::strncasecmp(s1,s2, n);

#else
	register char c1, c2;
	unsigned int p = 0;
	do 
	{
		if (p++ >= n)return 0;
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
	} 
	while ( c1 && (c1 == c2) );

	return c1 - c2;
#endif
}


// ---------------------------------------------------------------------------
// Evaluates an integer power. 
inline unsigned int integer_pow (unsigned int base, unsigned int power)
{
	unsigned int res = 1;
	for (unsigned int i = 0; i < power;++i)
		res *= base;

	return res;
}
} // end of namespace

#endif // !  AI_STRINGCOMPARISON_H_INC
