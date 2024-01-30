/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
*/

/** @file  MD5Parser.cpp
 *  @brief Implementation of the MD5 parser class
 */

// internal headers
#include "AssetLib/MD5/MD5Loader.h"
#include "Material/MaterialSystem.h"

#include <assimp/ParsingUtils.h>
#include <assimp/StringComparison.h>
#include <assimp/fast_atof.h>
#include <assimp/mesh.h>
#include <assimp/DefaultLogger.hpp>

using namespace Assimp;
using namespace Assimp::MD5;

// ------------------------------------------------------------------------------------------------
// Parse the segment structure for an MD5 file
MD5Parser::MD5Parser(char *_buffer, unsigned int _fileSize) : buffer(_buffer), bufferEnd(nullptr), fileSize(_fileSize), lineNumber(0) {
    ai_assert(nullptr != _buffer);
    ai_assert(0 != _fileSize);

    bufferEnd = buffer + fileSize;
    ASSIMP_LOG_DEBUG("MD5Parser begin");

    // parse the file header
    ParseHeader();

    // and read all sections until we're finished
    bool running = true;
    while (running) {
        mSections.emplace_back();
        Section &sec = mSections.back();
        if (!ParseSection(sec)) {
            break;
        }
    }

    if (!DefaultLogger::isNullLogger()) {
        char szBuffer[128]; // should be sufficiently large
        ::ai_snprintf(szBuffer, 128, "MD5Parser end. Parsed %i sections", (int)mSections.size());
        ASSIMP_LOG_DEBUG(szBuffer);
    }
}

// ------------------------------------------------------------------------------------------------
// Report error to the log stream
AI_WONT_RETURN void MD5Parser::ReportError(const char *error, unsigned int line) {
    char szBuffer[1024];
    ::ai_snprintf(szBuffer, 1024, "[MD5] Line %u: %s", line, error);
    throw DeadlyImportError(szBuffer);
}

// ------------------------------------------------------------------------------------------------
// Report warning to the log stream
void MD5Parser::ReportWarning(const char *warn, unsigned int line) {
    char szBuffer[1024];
    ::snprintf(szBuffer, sizeof(szBuffer), "[MD5] Line %u: %s", line, warn);
    ASSIMP_LOG_WARN(szBuffer);
}

// ------------------------------------------------------------------------------------------------
// Parse and validate the MD5 header
void MD5Parser::ParseHeader() {
    // parse and validate the file version
    SkipSpaces();
    if (!TokenMatch(buffer, "MD5Version", 10)) {
        ReportError("Invalid MD5 file: MD5Version tag has not been found");
    }
    SkipSpaces();
    unsigned int iVer = ::strtoul10(buffer, (const char **)&buffer);
    if (10 != iVer) {
        ReportError("MD5 version tag is unknown (10 is expected)");
    }
    SkipLine();
    if (buffer == bufferEnd) {
        return;
    }

    // print the command line options to the console
    // FIX: can break the log length limit, so we need to be careful
    char *sz = buffer;
    while (!IsLineEnd(*buffer++));
    
    ASSIMP_LOG_INFO(std::string(sz, std::min((uintptr_t)MAX_LOG_MESSAGE_LENGTH, (uintptr_t)(buffer - sz))));
    SkipSpacesAndLineEnd();
}

// ------------------------------------------------------------------------------------------------
// Recursive MD5 parsing function
bool MD5Parser::ParseSection(Section &out) {
    // store the current line number for use in error messages
    out.iLineNumber = lineNumber;

    // first parse the name of the section
    char *sz = buffer;
    while (!IsSpaceOrNewLine(*buffer)) {
        ++buffer;
        if (buffer == bufferEnd) {
            return false;
	    }
    }
    out.mName = std::string(sz, (uintptr_t)(buffer - sz));
    while (IsSpace(*buffer)) {
        ++buffer;
        if (buffer == bufferEnd) {
            return false;
	    }
    }

    bool running = true;
    while (running) {
        if ('{' == *buffer) {
            // it is a normal section so read all lines
            ++buffer;
            if (buffer == bufferEnd) {
                return false;
	        }
            bool run = true;
            while (run) {
                while (IsSpaceOrNewLine(*buffer)) {
                    ++buffer;
                    if (buffer == bufferEnd) {
                        return false;
		            }
                }
                if ('\0' == *buffer) {
                    return false; // seems this was the last section
                }
                if ('}' == *buffer) {
                    ++buffer;
                    break;
                }

                out.mElements.emplace_back();
                Element &elem = out.mElements.back();

                elem.iLineNumber = lineNumber;
                elem.szStart = buffer;
                elem.end = bufferEnd;

                // terminate the line with zero
                while (!IsLineEnd(*buffer)) {
                    ++buffer;
                    if (buffer == bufferEnd) {
                        return false;
		            }
                }
                if (*buffer) {
                    ++lineNumber;
                    *buffer++ = '\0';
                    if (buffer == bufferEnd) {
                        return false;
		            }
                }
            }
            break;
        } else if (!IsSpaceOrNewLine(*buffer)) {
            // it is an element at global scope. Parse its value and go on
            sz = buffer;
            while (!IsSpaceOrNewLine(*buffer++)) {
                if (buffer == bufferEnd) {
                    return false;
		        }
            }
            out.mGlobalValue = std::string(sz, (uintptr_t)(buffer - sz));
            continue;
        }
        break;
    }
    if (buffer == bufferEnd) {
        return false;
    }
    while (IsSpaceOrNewLine(*buffer)) {
        if (buffer == bufferEnd) {
            break;
	    }
        ++buffer;
    }
    return '\0' != *buffer;
}

// skip all spaces ... handle EOL correctly
inline void AI_MD5_SKIP_SPACES(const char **sz, const char *bufferEnd, int linenumber) {
    if (!SkipSpaces(sz, bufferEnd)) {
        MD5Parser::ReportWarning("Unexpected end of line", linenumber);
    }
}

// read a triple float in brackets: (1.0 1.0 1.0)
inline void AI_MD5_READ_TRIPLE(aiVector3D &vec, const char **sz, const char *bufferEnd, int linenumber) {
    AI_MD5_SKIP_SPACES(sz, bufferEnd, linenumber);
    if ('(' != **sz) {
        MD5Parser::ReportWarning("Unexpected token: ( was expected", linenumber);
        ++*sz;
    }
    ++*sz;
    AI_MD5_SKIP_SPACES(sz, bufferEnd, linenumber);
    *sz = fast_atoreal_move<float>(*sz, (float &)vec.x);
    AI_MD5_SKIP_SPACES(sz, bufferEnd, linenumber);
    *sz = fast_atoreal_move<float>(*sz, (float &)vec.y);
    AI_MD5_SKIP_SPACES(sz, bufferEnd, linenumber);
    *sz = fast_atoreal_move<float>(*sz, (float &)vec.z);
    AI_MD5_SKIP_SPACES(sz, bufferEnd, linenumber);
    if (')' != **sz) {
        MD5Parser::ReportWarning("Unexpected token: ) was expected", linenumber);
    }
    ++*sz;
}

// parse a string, enclosed in quotation marks or not
inline bool AI_MD5_PARSE_STRING(const char **sz, const char *bufferEnd, aiString &out, int linenumber) {
    bool bQuota = (**sz == '\"');
    const char *szStart = *sz;
    while (!IsSpaceOrNewLine(**sz)) {
        ++*sz;
        if (*sz == bufferEnd) break;
    }
    const char *szEnd = *sz;
    if (bQuota) {
        szStart++;
        if ('\"' != *(szEnd -= 1)) {
            MD5Parser::ReportWarning("Expected closing quotation marks in string", linenumber);
            ++*sz;
        }
    }
    out.length = (ai_uint32)(szEnd - szStart);
    ::memcpy(out.data, szStart, out.length);
    out.data[out.length] = '\0';

    return true;
}

// parse a string, enclosed in quotation marks
inline void AI_MD5_PARSE_STRING_IN_QUOTATION(const char **sz, const char *bufferEnd, aiString &out) {
    out.length = 0u;
    while (('\"' != **sz && '\0' != **sz) && *sz != bufferEnd) {
        ++*sz;
    }
    if ('\0' != **sz) {
        const char *szStart = ++(*sz);
        
        while (('\"' != **sz && '\0' != **sz) && *sz != bufferEnd) {
            ++*sz;
        }
        if ('\0' != **sz) {
            const char *szEnd = *sz;
            ++*sz;
            out.length = (ai_uint32)(szEnd - szStart);
            ::memcpy(out.data, szStart, out.length);
        }
    }
    out.data[out.length] = '\0';
}

// ------------------------------------------------------------------------------------------------
// .MD5MESH parsing function
MD5MeshParser::MD5MeshParser(SectionArray &mSections) {
    ASSIMP_LOG_DEBUG("MD5MeshParser begin");

    // now parse all sections
    for (SectionArray::const_iterator iter = mSections.begin(), iterEnd = mSections.end(); iter != iterEnd; ++iter) {
        if ((*iter).mName == "numMeshes") {
            mMeshes.reserve(::strtoul10((*iter).mGlobalValue.c_str()));
        } else if ((*iter).mName == "numJoints") {
            mJoints.reserve(::strtoul10((*iter).mGlobalValue.c_str()));
        } else if ((*iter).mName == "joints") {
            // "origin" -1 ( -0.000000 0.016430 -0.006044 ) ( 0.707107 0.000000 0.707107 )
            for (const auto &elem : (*iter).mElements) {
                mJoints.emplace_back();
                BoneDesc &desc = mJoints.back();

                const char *sz = elem.szStart;
                AI_MD5_PARSE_STRING_IN_QUOTATION(&sz, elem.end, desc.mName);
                
                AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);

                // negative values, at least -1, is allowed here
                desc.mParentIndex = (int)strtol10(sz, &sz);

                AI_MD5_READ_TRIPLE(desc.mPositionXYZ, &sz, elem.end, elem.iLineNumber);
                AI_MD5_READ_TRIPLE(desc.mRotationQuat, &sz, elem.end, elem.iLineNumber); // normalized quaternion, so w is not there
            }
        } else if ((*iter).mName == "mesh") {
            mMeshes.emplace_back();
            MeshDesc &desc = mMeshes.back();

            for (const auto &elem : (*iter).mElements) {
                const char *sz = elem.szStart;

                // shader attribute
                if (TokenMatch(sz, "shader", 6)) {
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    AI_MD5_PARSE_STRING_IN_QUOTATION(&sz, elem.end, desc.mShader);
                }
                // numverts attribute
                else if (TokenMatch(sz, "numverts", 8)) {
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    desc.mVertices.resize(strtoul10(sz));
                }
                // numtris attribute
                else if (TokenMatch(sz, "numtris", 7)) {
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    desc.mFaces.resize(strtoul10(sz));
                }
                // numweights attribute
                else if (TokenMatch(sz, "numweights", 10)) {
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    desc.mWeights.resize(strtoul10(sz));
                }
                // vert attribute
                // "vert 0 ( 0.394531 0.513672 ) 0 1"
                else if (TokenMatch(sz, "vert", 4)) {
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    const unsigned int idx = ::strtoul10(sz, &sz);
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    if (idx >= desc.mVertices.size())
                        desc.mVertices.resize(idx + 1);

                    VertexDesc &vert = desc.mVertices[idx];
                    if ('(' != *sz++)
                        MD5Parser::ReportWarning("Unexpected token: ( was expected", elem.iLineNumber);
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    sz = fast_atoreal_move<float>(sz, (float &)vert.mUV.x);
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    sz = fast_atoreal_move<float>(sz, (float &)vert.mUV.y);
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    if (')' != *sz++)
                        MD5Parser::ReportWarning("Unexpected token: ) was expected", elem.iLineNumber);
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    vert.mFirstWeight = ::strtoul10(sz, &sz);
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    vert.mNumWeights = ::strtoul10(sz, &sz);
                }
                // tri attribute
                // "tri 0 15 13 12"
                else if (TokenMatch(sz, "tri", 3)) {
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    const unsigned int idx = strtoul10(sz, &sz);
                    if (idx >= desc.mFaces.size())
                        desc.mFaces.resize(idx + 1);

                    aiFace &face = desc.mFaces[idx];
                    face.mIndices = new unsigned int[face.mNumIndices = 3];
                    for (unsigned int i = 0; i < 3; ++i) {
                        AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                        face.mIndices[i] = strtoul10(sz, &sz);
                    }
                }
                // weight attribute
                // "weight 362 5 0.500000 ( -3.553583 11.893474 9.719339 )"
                else if (TokenMatch(sz, "weight", 6)) {
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    const unsigned int idx = strtoul10(sz, &sz);
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    if (idx >= desc.mWeights.size())
                        desc.mWeights.resize(idx + 1);

                    WeightDesc &weight = desc.mWeights[idx];
                    weight.mBone = strtoul10(sz, &sz);
                    AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                    sz = fast_atoreal_move<float>(sz, weight.mWeight);
                    AI_MD5_READ_TRIPLE(weight.vOffsetPosition, &sz, elem.end, elem.iLineNumber);
                }
            }
        }
    }
    ASSIMP_LOG_DEBUG("MD5MeshParser end");
}

// ------------------------------------------------------------------------------------------------
// .MD5ANIM parsing function
MD5AnimParser::MD5AnimParser(SectionArray &mSections) {
    ASSIMP_LOG_DEBUG("MD5AnimParser begin");

    fFrameRate = 24.0f;
    mNumAnimatedComponents = UINT_MAX;
    for (SectionArray::const_iterator iter = mSections.begin(), iterEnd = mSections.end(); iter != iterEnd; ++iter) {
        if ((*iter).mName == "hierarchy") {
            // "sheath" 0 63 6
            for (const auto &elem : (*iter).mElements) {
                mAnimatedBones.emplace_back();
                AnimBoneDesc &desc = mAnimatedBones.back();

                const char *sz = elem.szStart;
                AI_MD5_PARSE_STRING_IN_QUOTATION(&sz, elem.end, desc.mName);
                AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);

                // parent index - negative values are allowed (at least -1)
                desc.mParentIndex = ::strtol10(sz, &sz);

                // flags (highest is 2^6-1)
                AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                if (63 < (desc.iFlags = ::strtoul10(sz, &sz))) {
                    MD5Parser::ReportWarning("Invalid flag combination in hierarchy section", elem.iLineNumber);
                }
                AI_MD5_SKIP_SPACES(&  sz, elem.end, elem.iLineNumber);

                // index of the first animation keyframe component for this joint
                desc.iFirstKeyIndex = ::strtoul10(sz, &sz);
            }
        } else if ((*iter).mName == "baseframe") {
            // ( -0.000000 0.016430 -0.006044 ) ( 0.707107 0.000242 0.707107 )
            for (const auto &elem : (*iter).mElements) {
                const char *sz = elem.szStart;

                mBaseFrames.emplace_back();
                BaseFrameDesc &desc = mBaseFrames.back();

                AI_MD5_READ_TRIPLE(desc.vPositionXYZ, &sz, elem.end, elem.iLineNumber);
                AI_MD5_READ_TRIPLE(desc.vRotationQuat, &sz, elem.end, elem.iLineNumber);
            }
        } else if ((*iter).mName == "frame") {
            if (!(*iter).mGlobalValue.length()) {
                MD5Parser::ReportWarning("A frame section must have a frame index", (*iter).iLineNumber);
                continue;
            }

            mFrames.emplace_back();
            FrameDesc &desc = mFrames.back();
            desc.iIndex = strtoul10((*iter).mGlobalValue.c_str());

            // we do already know how much storage we will presumably need
            if (UINT_MAX != mNumAnimatedComponents) {
                desc.mValues.reserve(mNumAnimatedComponents);
            }

            // now read all elements (continuous list of floats)
            for (const auto &elem : (*iter).mElements) {
                const char *sz = elem.szStart;
                while (SkipSpacesAndLineEnd(&sz, elem.end)) {
                    float f;
                    sz = fast_atoreal_move<float>(sz, f);
                    desc.mValues.push_back(f);
                }
            }
        } else if ((*iter).mName == "numFrames") {
            mFrames.reserve(strtoul10((*iter).mGlobalValue.c_str()));
        } else if ((*iter).mName == "numJoints") {
            const unsigned int num = strtoul10((*iter).mGlobalValue.c_str());
            mAnimatedBones.reserve(num);

            // try to guess the number of animated components if that element is not given
            if (UINT_MAX == mNumAnimatedComponents) {
                mNumAnimatedComponents = num * 6;
            }
        } else if ((*iter).mName == "numAnimatedComponents") {
            mAnimatedBones.reserve(strtoul10((*iter).mGlobalValue.c_str()));
        } else if ((*iter).mName == "frameRate") {
            fast_atoreal_move<float>((*iter).mGlobalValue.c_str(), fFrameRate);
        }
    }
    ASSIMP_LOG_DEBUG("MD5AnimParser end");
}

// ------------------------------------------------------------------------------------------------
// .MD5CAMERA parsing function
MD5CameraParser::MD5CameraParser(SectionArray &mSections) {
    ASSIMP_LOG_DEBUG("MD5CameraParser begin");
    fFrameRate = 24.0f;

    for (SectionArray::const_iterator iter = mSections.begin(), iterEnd = mSections.end(); iter != iterEnd; ++iter) {
        if ((*iter).mName == "numFrames") {
            frames.reserve(strtoul10((*iter).mGlobalValue.c_str()));
        } else if ((*iter).mName == "frameRate") {
            fFrameRate = fast_atof((*iter).mGlobalValue.c_str());
        } else if ((*iter).mName == "numCuts") {
            cuts.reserve(strtoul10((*iter).mGlobalValue.c_str()));
        } else if ((*iter).mName == "cuts") {
            for (const auto &elem : (*iter).mElements) {
                cuts.push_back(strtoul10(elem.szStart) + 1);
            }
        } else if ((*iter).mName == "camera") {
            for (const auto &elem : (*iter).mElements) {
                const char *sz = elem.szStart;

                frames.emplace_back();
                CameraAnimFrameDesc &cur = frames.back();
                AI_MD5_READ_TRIPLE(cur.vPositionXYZ, &sz, elem.end, elem.iLineNumber);
                AI_MD5_READ_TRIPLE(cur.vRotationQuat, &sz, elem.end, elem.iLineNumber);
                AI_MD5_SKIP_SPACES(&sz, elem.end, elem.iLineNumber);
                cur.fFOV = fast_atof(sz);
            }
        }
    }
    ASSIMP_LOG_DEBUG("MD5CameraParser end");
}
