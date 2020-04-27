/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2018, Robotic Eyes GmbH

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

/** @file RexExporter.h
 * Declares the exporter class to write a scene to a the REX file
 */
#ifndef AI_REXEXPORTER_H_INC
#define AI_REXEXPORTER_H_INC

#include <assimp/types.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <map>
#include <sstream>
#include <rex.h>

#ifdef _WIN32
#define fopen_rex( file, filename, mode ) \
auto fopenerror = fopen_s( &file, filename, mode ); \
if (fopenerror) file = 0;
#else
#define fopen_rex( file, filename, mode ) \
file = fopen ( filename, mode )
#endif


struct aiScene;
struct aiNode;
struct aiMesh;

namespace rex
{
    class FileWrapper;

    class RexExporter
    {
    public:
        /// Constructor for a specific scene to export
        RexExporter (const char *fileName, const aiScene *pScene);
        ~RexExporter();

        void Start();

    private:
        struct Triangle {
            std::vector<uint32_t> indices;
        };

        struct VertexData {
            aiVector3D vp;
            aiColor3D vc;
            aiVector3D vt;
        };

        struct DataPtr{
            uint8_t* data;
            long size;
        };

        void WriteGeometryFile();
        void WriteMeshes(rex_header *header, int startId, int startMaterials, std::vector<DataPtr> &meshPtrs);
        void WriteImages(rex_header *header, int startId, std::vector<DataPtr> &meshPtrs);
        void WriteMaterials(rex_header *header, int startId, std::vector<DataPtr> &materialPtrs);
        void GetMaterialsAndTextures();

        void AddMesh(const aiString& name, const aiMesh* m, const aiMatrix4x4& mat);
        void AddNode(const aiNode* nd, const aiMatrix4x4& mParent);

        void GetTriangleArray(const std::vector<Triangle>& triangles, std::vector<uint32_t> &triangleArray );
        void GetVertexArray( const std::vector<aiVector3D> vector, std::vector<float>& vectorArray );
        void GetColorArray( const std::vector<aiColor3D> vector, std::vector<float>& colorArray );
        void GetTextureCoordArray( const std::vector<aiVector3D> vector, std::vector<float>& textureCoordArray );

    public:
        struct vertexDataCompare {
            bool operator() ( const VertexData& a, const VertexData& b ) const {
                // position
                if (a.vp.x < b.vp.x) return true;
                if (a.vp.x > b.vp.x) return false;
                if (a.vp.y < b.vp.y) return true;
                if (a.vp.y > b.vp.y) return false;
                if (a.vp.z < b.vp.z) return true;
                if (a.vp.z > b.vp.z) return false;

                // color
                if (a.vc.r < b.vc.r) return true;
                if (a.vc.r > b.vc.r) return false;
                if (a.vc.g < b.vc.g) return true;
                if (a.vc.g > b.vc.g) return false;
                if (a.vc.b < b.vc.b) return true;
                if (a.vc.b > b.vc.b) return false;

                // vertex textures
                if (a.vt.x < b.vt.x) return true;
                if (a.vt.x > b.vt.x) return false;
                if (a.vt.y < b.vt.y) return true;
                if (a.vt.y > b.vt.y) return false;
                if (a.vt.z < b.vt.z) return true;
                if (a.vt.z > b.vt.z) return false;
                return false;
            }
        };

        template <class T, class Compare = std::less<T>>
        class indexMap {
            int mNextIndex;
            typedef std::map<T, int, Compare> dataType;
            dataType vecMap;

        public:
            indexMap()
            : mNextIndex(0) {
                // empty
            }

            int getIndex(const T& key) {
                typename dataType::iterator vertIt = vecMap.find(key);

                // vertex already exists, so reference it
                if(vertIt != vecMap.end()){
                    return vertIt->second;
                }

                return vecMap[key] = mNextIndex++;
            }

            int add(const T& key, const int index) {
                return vecMap[key] = index;
            }

            void getKeys( std::vector<T>& keys ) {
                keys.resize(vecMap.size());
                for(typename dataType::iterator it = vecMap.begin(); it != vecMap.end(); ++it){
                    keys[it->second] = it->first;
                }
            }

            size_t size() const {
                return vecMap.size();
            }
        };

        struct MeshInstance {
            std::string                             name;
            bool                                    useColors;
            uint32_t                                materialId;
            std::vector<Triangle>                   triangles;
            indexMap<VertexData, vertexDataCompare> verticesWithColors;
        };

    private:
        const aiScene *const                m_Scene;
        std::shared_ptr<FileWrapper>        m_File;
        indexMap<std::string>               m_TextureMap;
        std::vector<rex_material_standard>  m_Materials;
        std::vector<MeshInstance>           m_Meshes;
    };

    /**
     * This file wrapper offers RAII (resource acquisition is initialization) for the
     * raw C FILE pointer. Whenever the FILE* is used, this wrapper ensures that the resource
     * is closed upon destruction. See Stroustrup 4th edition, page 356.
     */
    class FileWrapper
    {
    public:
        FileWrapper (const char *n, const char *a)
        {
            fopen_rex (m_file, n, a);
            if (m_file == nullptr)
            {
                throw std::runtime_error ("FileWrapper::FileWrapper: cannot open file");
            }

            //get path of file
            //TODO does not work on windows
            std::string fileName = std::string(n);
            auto pos = fileName.rfind("/");
            m_path = "";
            if (pos != fileName.npos) {
                m_path = fileName.substr(0, pos + 1);
            }
        }

        FileWrapper (const std::string &n, const char *a) :
            FileWrapper (n.c_str(), a)
        {
        }

        /**
         * This ctor assumes ownership of the file pointer
         */
        explicit FileWrapper (FILE *f) :
            m_file (f)
        {
            if (m_file == nullptr)
            {
                throw std::runtime_error ("FileWrapper::FileWrapper: file pointer is null");
            }
        }

        ~FileWrapper()
        {
            ::fclose (m_file);
        }

        operator FILE *()
        {
            return m_file;
        }

        FILE *ptr()
        {
            return m_file;
        }

        std::string getFilePath() {
            return m_path;
        }

        size_t write (const void *ptr, size_t size, size_t nitems, const std::string &debugInfo = "")
        {
            size_t ret = ::fwrite (ptr, size, nitems, m_file);
            if (ret != nitems)
            {
                std::cerr << debugInfo << "error writing";
                throw std::runtime_error ("FileWrapper::fwrite: error writing file");
            }
            return  ret;
        }

        size_t read (void *ptr, size_t size, size_t nitems, const std::string &debugInfo = "")
        {
            size_t ret = ::fread (ptr, size, nitems, m_file);
            if (ret != nitems)
            {
                std::cerr << debugInfo << "error reading";
                throw std::runtime_error ("FileWrapper::fread: error reading file");
            }
            return  ret;
        }

    private:
        FILE *m_file;
        std::string m_path;
    };

    /*!
     * Narrow cast proposed by Stroustrup's C++ bible (pg. 299, 11.5)
     */
    template<typename Target, typename Source>
    Target narrow_cast (Source v)
    {
        auto r = static_cast<Target> (v);
        if (static_cast<Source> (r) != v)
        {
            throw std::runtime_error ("narrow_cast<>() failed");
        }
        return r;
    };

}

#endif
