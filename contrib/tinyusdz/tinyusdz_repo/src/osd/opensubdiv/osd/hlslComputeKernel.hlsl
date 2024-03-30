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

cbuffer KernelUniformArgs : register( b0 ) {
    int batchStart;
    int batchEnd;
    int srcOffset;
    int dstOffset;
};

RWBuffer<float> vertexBuffer  : register( u0 );
RWBuffer<float> dstVertexBuffer  : register( u1 );
Buffer<int>    sizes   : register( t1 );
Buffer<int>    offsets : register( t2 );
Buffer<int>    indices : register( t3 );
Buffer<float>  weights : register( t4 );

//----------------------------------------------------------------------------

struct Vertex {
    float vertexData[LENGTH];
};

void clear(out Vertex v) {
    for (int i = 0; i < LENGTH; ++i) {
        v.vertexData[i] = 0;
    }
}

Vertex readVertex(int index) {
    Vertex v;
    int vertexIndex = srcOffset + index * SRC_STRIDE;
    for (int i = 0; i < LENGTH; ++i) {
        v.vertexData[i] = vertexBuffer[vertexIndex + i];
    }
    return v;
}

void writeVertex(int index, Vertex v) {
    int vertexIndex = dstOffset + index * DST_STRIDE;
    for (int i = 0; i < LENGTH; ++i) {
        vertexBuffer[vertexIndex + i] = v.vertexData[i];
    }
}

void writeVertexSeparate(int index, Vertex v) {
    int vertexIndex = dstOffset + index * DST_STRIDE;
    for (int i = 0; i < LENGTH; ++i) {
        dstVertexBuffer[vertexIndex + i] = v.vertexData[i];
    }
}

void addWithWeight(inout Vertex v, const Vertex src, float weight) {
    for (int i = 0; i < LENGTH; ++i) {
        v.vertexData[i] += weight * src.vertexData[i];
    }
}

// ---------------------------------------------------------------------------

interface IComputeKernel {
    void runKernel( uint3 ID );
};
IComputeKernel kernel;

class SingleBufferCompute : IComputeKernel {

    int placeholder;
    void runKernel(uint3 ID) {
        int current = int(ID.x) + batchStart;

        if (current>=batchEnd) {
            return;
        }

        Vertex dst;
        clear(dst);

        int offset = offsets[current],
            size = sizes[current];

        for (int i=0; i<size; ++i) {
            addWithWeight(dst, readVertex( indices[offset+i] ), weights[offset+i]);
        }

        writeVertex(current, dst);
    }
};
class SeparateBufferCompute : IComputeKernel {

    int placeholder;
    void runKernel(uint3 ID) {
        int current = int(ID.x) + batchStart;

        if (current>=batchEnd) {
            return;
        }

        Vertex dst;
        clear(dst);

        int offset = offsets[current],
            size = sizes[current];

        for (int i=0; i<size; ++i) {
            addWithWeight(dst, readVertex( indices[offset+i] ), weights[offset+i]);
        }

        writeVertexSeparate(current, dst);
    }
};

SingleBufferCompute singleBufferCompute;
SeparateBufferCompute separateBufferCompute;

// Add place-holder stencil kernel or D3D11ShaderReflection::GetInterfaceSlots()
// returns 0
class PlaceHolder : IComputeKernel {
    int placeholder;
    void runKernel( uint3 ID ) {
    }
};

[numthreads(WORK_GROUP_SIZE, 1, 1)]
void cs_main( uint3 ID : SV_DispatchThreadID )
{
    // call kernel
    kernel.runKernel(ID);
}

