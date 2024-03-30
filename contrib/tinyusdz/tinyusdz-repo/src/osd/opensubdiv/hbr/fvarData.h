//
//   Copyright 2013 Pixar
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

#ifndef OPENSUBDIV3_HBRFVARDATA_H
#define OPENSUBDIV3_HBRFVARDATA_H

#include <cstring>
#include <cmath>

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrFVarEdit;
template <class T> class HbrFace;
template <class T> class HbrVertex;

// This class implements a "face varying vector item". Really it's
// just a smart wrapper around face varying data (itself just a bunch
// of floats) stored on each vertex.
template <class T> class HbrFVarData {

private:
    HbrFVarData()
        : faceid(0), initialized(0) {
    }

    ~HbrFVarData() {
        Uninitialize();
    }

    HbrFVarData(const HbrFVarData &/* data */) {}

public:
    
    // Sets the face id
    void SetFaceID(int id) {
        faceid = id;
    }

    // Returns the id of the face to which this data is bound
    int GetFaceID() const {
        return faceid;
    }

    // Clears the initialized flag
    void Uninitialize() {
        initialized = 0;
        faceid = 0;
    }

    // Returns initialized flag
    bool IsInitialized() const {
        return initialized;
    }

    // Sets initialized flag
    void SetInitialized() {
        initialized = 1;
    }

    // Return the data from the NgpFVVector
    float* GetData(int item) { return data + item; }    

    // Clears the indicates value of this item
    void Clear(int startindex, int width) {
        memset(data + startindex, 0, width * sizeof(float));
    }

    // Clears all values of this item
    void ClearAll(int width) {
        initialized = 1;
        memset(data, 0, width * sizeof(float));
    }

    // Set values of the indicated item (with the indicated weighing)
    // on this item
    void SetWithWeight(const HbrFVarData& fvvi, int startindex, int width, float weight) {
        float *dst = data + startindex;
        const float *src = fvvi.data + startindex;
        for (int i = 0; i < width; ++i) {
            *dst++ = weight * *src++;
        }
    }

    // Add values of the indicated item (with the indicated weighing)
    // to this item
    void AddWithWeight(const HbrFVarData& fvvi, int startindex, int width, float weight) {
        float *dst = data + startindex;
        const float *src = fvvi.data + startindex;
        for (int i = 0; i < width; ++i) {
            *dst++ += weight * *src++;
        }
    }

    // Add all values of the indicated item (with the indicated
    // weighing) to this item
    void AddWithWeightAll(const HbrFVarData& fvvi, int width, float weight) {
        float *dst = data;
        const float *src = fvvi.data;
        for (int i = 0; i < width; ++i) {
            *dst++ += weight * *src++;
        }
    }

    // Compare all values item against a float buffer. Returns true
    // if all values match
    bool CompareAll(int width, const float *values, float tolerance=0.0f) const {
        if (!initialized) return false;
        for (int i = 0; i < width; ++i) {
            if (fabsf(values[i] - data[i]) > tolerance) return false;
        }
        return true;
    }

    // Initializes data
    void SetAllData(int width, const float *values) {
        initialized = 1;
        memcpy(data, values, width * sizeof(float));
    }

    // Compare this item against another item with tolerance.  Returns
    // true if it compares identical
    bool Compare(const HbrFVarData& fvvi, int startindex, int width, float tolerance=0.0f) const {
        for (int i = 0; i < width; ++i) {
            if (fabsf(data[startindex + i] - fvvi.data[startindex + i]) > tolerance) return false;
        }
        return true;
    }

    // Modify the data of the item with an edit
    void ApplyFVarEdit(const HbrFVarEdit<T>& edit);

    friend class HbrVertex<T>;
    
private:
    unsigned int faceid:31;
    unsigned int initialized:1;
    float data[1];
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#include "../hbr/fvarEdit.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T>
void
HbrFVarData<T>::ApplyFVarEdit(const HbrFVarEdit<T>& edit) {
        float *dst = data + edit.GetIndex() + edit.GetOffset();
        const float *src = edit.GetEdit();
        for (int i = 0; i < edit.GetWidth(); ++i) {
            switch(edit.GetOperation()) {
                case HbrVertexEdit<T>::Set:
                    *dst++ = *src++;
                    break;
                case HbrVertexEdit<T>::Add:
                    *dst++ += *src++;
                    break;
                case HbrVertexEdit<T>::Subtract:
                    *dst++ -= *src++;
            }
        }
        initialized = 1;
    }


} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRFVARDATA_H */
