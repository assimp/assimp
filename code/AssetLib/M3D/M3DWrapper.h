#pragma once
/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team
Copyright (c) 2019 bzt

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

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

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

/** @file M3DWrapper.h
*   @brief Declares a class to wrap the C m3d SDK
*/
#ifndef AI_M3DWRAPPER_H_INC
#define AI_M3DWRAPPER_H_INC

#if !defined ASSIMP_BUILD_NO_M3D_IMPORTER || !(defined ASSIMP_BUILD_NO_EXPORT || defined ASSIMP_BUILD_NO_M3D_EXPORTER)

#include <memory>
#include <vector>
#include <string>

// Assimp specific M3D configuration. Comment out these defines to remove functionality
//#define ASSIMP_USE_M3D_READFILECB

#include "Common/StbCommon.h"

#include "m3d.h"

namespace Assimp {

class IOSystem;

/// brief   The M3D-Wrapper, provudes c++ access to the data.
class M3DWrapper {
public:
	/// Construct an empty M3D model
	explicit M3DWrapper();

	/// Construct an M3D model from provided buffer
	/// @note The m3d.h SDK function does not mark the data as const. Have assumed it does not write.
	/// BUG: SECURITY: The m3d.h SDK cannot be informed of the buffer size. BUFFER OVERFLOW IS CERTAIN
	explicit M3DWrapper(IOSystem *pIOHandler, const std::vector<unsigned char> &buffer);

	/// Theclasss destructor.
    ~M3DWrapper();

	/// Will reset the wrapper, all data will become nullptr.
    void reset();

	// The Name access, empty string returned when no m3d instance.
	std::string Name() const;

	/// Executes a save.
	unsigned char *Save(int quality, int flags, unsigned int &size);

    /// Clearer
	void ClearSave();

    /// True for m3d instance exists.
	explicit operator bool() const;

	// Allow direct access to M3D API
	m3d_t *operator->() const;
	m3d_t *M3D() const;

private:
	m3d_t *m3d_ = nullptr;
	unsigned char *saved_output_ = nullptr;
};

inline std::string M3DWrapper::Name() const {
    if (nullptr != m3d_) {
        if (nullptr != m3d_->name) {
            return std::string(m3d_->name);
        }
    }
    return std::string();
}

inline M3DWrapper::operator bool() const {
    return m3d_ != nullptr;
}

inline m3d_t *M3DWrapper::operator->() const {
    return m3d_;
}

inline m3d_t *M3DWrapper::M3D() const {
    return m3d_;
}

} // namespace Assimp

#endif // ASSIMP_BUILD_NO_M3D_IMPORTER

#endif // AI_M3DWRAPPER_H_INC
