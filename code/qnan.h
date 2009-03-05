
/**  @file  qnan.h
 *   @brief Some utilities for our dealings with qnans.
 *
 *   @note Some loaders use qnans heavily to mark invalid values (and they're
 *     even returned by Gen(Smooth)Normals if normals are undefined for a 
 *     primitive). Their whole usage is wrapped here, so you can easily
 *     fix issues with platforms with a different qnan implementation.
 */

#if (!defined AI_QNAN_H_INCLUDED)
#define AI_QNAN_H_INCLUDED

// ---------------------------------------------------------------------------
/** @brief Data structure for the bit pattern of a 32 Bit 
 *         IEEE 754 floating-point number.
 */
union _IEEESingle
{
	float Float;
	struct
	{
		uint32_t Frac : 23;
		uint32_t Exp  : 8;
		uint32_t Sign : 1;
	} IEEE;
} ;

// ---------------------------------------------------------------------------
/** @brief check whether a float is qNaN. 
 *  @param in Input value
 */
AI_FORCE_INLINE bool is_qnan(float in)
{
	return (in != in);
}

// ---------------------------------------------------------------------------
/** @brief check whether a float is NOT qNaN. 
 *  @param in Input value
 */
AI_FORCE_INLINE bool is_not_qnan(float in)
{
	return (in == in);
}

// ---------------------------------------------------------------------------
/** @brief check whether a float is either NaN or (+/-) INF. 
 *
 *  Denorms return false, they're treated like normal values.
 *  @param in Input value
 */
AI_FORCE_INLINE bool is_special_float(float in)
{
	return (((_IEEESingle*)&in)->IEEE.Exp == (1u << 8)-1);
}

// ---------------------------------------------------------------------------
/** @brief Get a qnan
 */
AI_FORCE_INLINE float get_qnan()
{
	return std::numeric_limits<float>::quiet_NaN();
}

#endif // !! AI_QNAN_H_INCLUDED
