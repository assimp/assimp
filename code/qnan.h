

#if (!defined AI_QNAN_H_INCLUDED)
#define AI_QNAN_H_INCLUDED


// ---------------------------------------------------------------------------
// check whether a float is NaN
AI_FORCE_INLINE bool is_qnan(float in)
{
	return (in != in);
}

// ---------------------------------------------------------------------------
// check whether a float is NOT NaN. 
AI_FORCE_INLINE bool is_not_qnan(float in)
{
	return (in == in);
}

// ---------------------------------------------------------------------------
// check whether a float is either NaN or (+/-) INF. Denorms return false,
// they're treated like normal values.
AI_FORCE_INLINE bool is_special_float(float in)
{
	union IEEESingle
	{
		float Float;
		struct
		{
			uint32_t Frac : 23;
			uint32_t Exp  : 8;
			uint32_t Sign : 1;
		} IEEE;
	} f;

	f.Float = in;
	return (f.IEEE.Exp == (1u << 8)-1);
}

#endif // !! AI_QNAN_H_INCLUDED
