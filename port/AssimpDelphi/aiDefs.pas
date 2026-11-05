unit aiDefs;

interface

// Build Assimp with -DLIBRARY_SUFFIX= to produce this name
const ASSIMP_DLL = 'assimp.dll';

// Define ASSIMP_DOUBLE_PRECISION when compiling these units if the DLL was built with it
{$IFDEF ASSIMP_DOUBLE_PRECISION}
type ai_real = Double;
type ai_int = Int64;
type ai_uint = UInt64;
{$ELSE}
type ai_real = Single;
type ai_int = Integer;
type ai_uint = Cardinal;
{$ENDIF}
type Pai_real = ^ai_real;

const AI_MATH_PI = 3.141592653589793238462643383279;
const AI_MATH_TWO_PI = AI_MATH_PI * 2.0;
const AI_MATH_HALF_PI = AI_MATH_PI * 0.5;
const AI_MATH_PI_F = 3.1415926538;
const AI_MATH_TWO_PI_F = AI_MATH_PI_F * 2.0;
const AI_MATH_HALF_PI_F = AI_MATH_PI_F * 0.5;

const ai_epsilon = 1e-6;

implementation

end.
