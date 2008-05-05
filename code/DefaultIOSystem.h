/** @file Default implementation of IOSystem using the standard C file functions */
#ifndef AI_DEFAULTIOSYSTEM_H_INC
#define AI_DEFAULTIOSYSTEM_H_INC

#include "IOSystem.h"

namespace Assimp
{

// ---------------------------------------------------------------------------
/** Default implementation of IOSystem using the standard C file functions */
class DefaultIOSystem : public IOSystem
{
public:
	/** Constructor. */
    DefaultIOSystem();

	/** Destructor. */
	~DefaultIOSystem();

	// -------------------------------------------------------------------
	/** Tests for the existence of a file at the given path. */
	bool Exists( const std::string& pFile) const;

	// -------------------------------------------------------------------
	/** Returns the directory separator. */
	std::string getOsSeparator() const;

	// -------------------------------------------------------------------
	/** Open a new file with a given path. */
	IOStream* Open( const std::string& pFile, const std::string& pMode = std::string("rb"));

	// -------------------------------------------------------------------
	/** Closes the given file and releases all resources associated with it. */
	void Close( IOStream* pFile);
};

} //!ns Assimp

#endif //AI_DEFAULTIOSYSTEM_H_INC
