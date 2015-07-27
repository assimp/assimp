
/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2015, assimp team
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

/** @file  BlenderLoader.cpp
 *  @brief Implementation of the Blender3D importer class.
 */


//#define ASSIMP_BUILD_NO_COMPRESSED_BLEND
// Uncomment this to disable support for (gzip)compressed .BLEND files

#ifndef ASSIMP_BUILD_NO_BLEND_IMPORTER

#include "BlenderIntermediate.h"
#include "BlenderModifier.h"
#include "BlenderBMesh.h"
#include "../include/assimp/scene.h"
#include "StringComparison.h"

#include "StreamReader.h"
#include "MemoryIOWrapper.h"
#include <cctype>


// zlib is needed for compressed blend files
#ifndef ASSIMP_BUILD_NO_COMPRESSED_BLEND
#   ifdef ASSIMP_BUILD_NO_OWN_ZLIB
#       include <zlib.h>
#   else
#       include "../contrib/zlib/zlib.h"
#   endif
#endif

namespace Assimp {
    template<> const std::string LogFunctions<BlenderImporter>::log_prefix = "BLEND: ";
}

using namespace Assimp;
using namespace Assimp::Blender;
using namespace Assimp::Formatter;

static const aiImporterDesc blenderDesc = {
    "Blender 3D Importer \nhttp://www.blender3d.org",
    "",
    "",
    "No animation support yet",
    aiImporterFlags_SupportBinaryFlavour,
    0,
    0,
    2,
    50,
    "blend"
};


// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
BlenderImporter::BlenderImporter()
: modifier_cache(new BlenderModifierShowcase())
{}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
BlenderImporter::~BlenderImporter()
{
    delete modifier_cache;
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file.
bool BlenderImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
    const std::string& extension = GetExtension(pFile);
    if (extension == "blend") {
        return true;
    }

    else if ((!extension.length() || checkSig) && pIOHandler)   {
        // note: this won't catch compressed files
        const char* tokens[] = {"BLENDER"};
        return SearchFileHeaderForToken(pIOHandler,pFile,tokens,1);
    }
    return false;
}

// ------------------------------------------------------------------------------------------------
// List all extensions handled by this loader
void BlenderImporter::GetExtensionList(std::set<std::string>& app)
{
    app.insert("blend");
}

// ------------------------------------------------------------------------------------------------
// Loader registry entry
const aiImporterDesc* BlenderImporter::GetInfo () const
{
    return &blenderDesc;
}

// ------------------------------------------------------------------------------------------------
// Setup configuration properties for the loader
void BlenderImporter::SetupProperties(const Importer* /*pImp*/)
{
    // nothing to be done for the moment
}

struct free_it
{
    free_it(void* free) : free(free) {}
    ~free_it() {
        ::free(this->free);
    }

    void* free;
};

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure.
void BlenderImporter::InternReadFile( const std::string& pFile,
    aiScene* pScene, IOSystem* pIOHandler)
{
#ifndef ASSIMP_BUILD_NO_COMPRESSED_BLEND
    Bytef* dest = NULL;
    free_it free_it_really(dest);
#endif

    FileDatabase file;
    boost::shared_ptr<IOStream> stream(pIOHandler->Open(pFile,"rb"));
    if (!stream) {
        ThrowException("Could not open file for reading");
    }

    char magic[8] = {0};
    stream->Read(magic,7,1);
    if (strcmp(magic,"BLENDER")) {
        // Check for presence of the gzip header. If yes, assume it is a
        // compressed blend file and try uncompressing it, else fail. This is to
        // avoid uncompressing random files which our loader might end up with.
#ifdef ASSIMP_BUILD_NO_COMPRESSED_BLEND
        ThrowException("BLENDER magic bytes are missing, is this file compressed (Assimp was built without decompression support)?");
#else

        if (magic[0] != 0x1f || static_cast<uint8_t>(magic[1]) != 0x8b) {
            ThrowException("BLENDER magic bytes are missing, couldn't find GZIP header either");
        }

        LogDebug("Found no BLENDER magic word but a GZIP header, might be a compressed file");
        if (magic[2] != 8) {
            ThrowException("Unsupported GZIP compression method");
        }

        // http://www.gzip.org/zlib/rfc-gzip.html#header-trailer
        stream->Seek(0L,aiOrigin_SET);
        boost::shared_ptr<StreamReaderLE> reader = boost::shared_ptr<StreamReaderLE>(new StreamReaderLE(stream));

        // build a zlib stream
        z_stream zstream;
        zstream.opaque = Z_NULL;
        zstream.zalloc = Z_NULL;
        zstream.zfree  = Z_NULL;
        zstream.data_type = Z_BINARY;

        // http://hewgill.com/journal/entries/349-how-to-decompress-gzip-stream-with-zlib
        inflateInit2(&zstream, 16+MAX_WBITS);

        zstream.next_in   = reinterpret_cast<Bytef*>( reader->GetPtr() );
        zstream.avail_in  = reader->GetRemainingSize();

        size_t total = 0l;

        // and decompress the data .... do 1k chunks in the hope that we won't kill the stack
#define MYBLOCK 1024
        Bytef block[MYBLOCK];
        int ret;
        do {
            zstream.avail_out = MYBLOCK;
            zstream.next_out = block;
            ret = inflate(&zstream, Z_NO_FLUSH);

            if (ret != Z_STREAM_END && ret != Z_OK) {
                ThrowException("Failure decompressing this file using gzip, seemingly it is NOT a compressed .BLEND file");
            }
            const size_t have = MYBLOCK - zstream.avail_out;
            total += have;
            dest = reinterpret_cast<Bytef*>( realloc(dest,total) );
            memcpy(dest + total - have,block,have);
        }
        while (ret != Z_STREAM_END);

        // terminate zlib
        inflateEnd(&zstream);

        // replace the input stream with a memory stream
        stream.reset(new MemoryIOStream(reinterpret_cast<uint8_t*>(dest),total));

        // .. and retry
        stream->Read(magic,7,1);
        if (strcmp(magic,"BLENDER")) {
            ThrowException("Found no BLENDER magic word in decompressed GZIP file");
        }
#endif
    }

    file.i64bit = (stream->Read(magic,1,1),magic[0]=='-');
    file.little = (stream->Read(magic,1,1),magic[0]=='v');

    stream->Read(magic,3,1);
    magic[3] = '\0';

    LogInfo((format(),"Blender version is ",magic[0],".",magic+1,
        " (64bit: ",file.i64bit?"true":"false",
        ", little endian: ",file.little?"true":"false",")"
    ));

    ParseBlendFile(file,stream);

    Scene scene;
    ExtractScene(scene,file);

    ConvertBlendFile(pScene,scene,file);
}

// ------------------------------------------------------------------------------------------------
void BlenderImporter::ParseBlendFile(FileDatabase& out, boost::shared_ptr<IOStream> stream)
{
    out.reader = boost::shared_ptr<StreamReaderAny>(new StreamReaderAny(stream,out.little));

    DNAParser dna_reader(out);
    const DNA* dna = NULL;

    out.entries.reserve(128); { // even small BLEND files tend to consist of many file blocks
        SectionParser parser(*out.reader.get(),out.i64bit);

        // first parse the file in search for the DNA and insert all other sections into the database
        while ((parser.Next(),1)) {
            const FileBlockHead& head = parser.GetCurrent();

            if (head.id == "ENDB") {
                break; // only valid end of the file
            }
            else if (head.id == "DNA1") {
                dna_reader.Parse();
                dna = &dna_reader.GetDNA();
                continue;
            }

            out.entries.push_back(head);
        }
    }
    if (!dna) {
        ThrowException("SDNA not found");
    }

    std::sort(out.entries.begin(),out.entries.end());
}

// ------------------------------------------------------------------------------------------------
void BlenderImporter::ExtractScene(Scene& out, const FileDatabase& file)
{
    const FileBlockHead* block = NULL;
    std::map<std::string,size_t>::const_iterator it = file.dna.indices.find("Scene");
    if (it == file.dna.indices.end()) {
        ThrowException("There is no `Scene` structure record");
    }

    const Structure& ss = file.dna.structures[(*it).second];

    // we need a scene somewhere to start with.
    for_each(const FileBlockHead& bl,file.entries) {

        // Fix: using the DNA index is more reliable to locate scenes
        //if (bl.id == "SC") {

        if (bl.dna_index == (*it).second) {
            block = &bl;
            break;
        }
    }

    if (!block) {
        ThrowException("There is not a single `Scene` record to load");
    }

    file.reader->SetCurrentPos(block->start);
    ss.Convert(out,file);

#ifndef ASSIMP_BUILD_BLENDER_NO_STATS
    DefaultLogger::get()->info((format(),
        "(Stats) Fields read: " ,file.stats().fields_read,
        ", pointers resolved: " ,file.stats().pointers_resolved,
        ", cache hits: "        ,file.stats().cache_hits,
        ", cached objects: "    ,file.stats().cached_objects
    ));
#endif
}

// ------------------------------------------------------------------------------------------------
void BlenderImporter::ConvertBlendFile(aiScene* out, const Scene& in,const FileDatabase& file)
{
    ConversionData conv(file);

    // FIXME it must be possible to take the hierarchy directly from
    // the file. This is terrible. Here, we're first looking for
    // all objects which don't have parent objects at all -
    std::deque<const Object*> no_parents;
    for (boost::shared_ptr<Base> cur = boost::static_pointer_cast<Base> ( in.base.first ); cur; cur = cur->next) {
        if (cur->object) {
            if(!cur->object->parent) {
                no_parents.push_back(cur->object.get());
            }
            else conv.objects.insert(cur->object.get());
        }
    }
    for (boost::shared_ptr<Base> cur = in.basact; cur; cur = cur->next) {
        if (cur->object) {
            if(cur->object->parent) {
                conv.objects.insert(cur->object.get());
            }
        }
    }

    if (no_parents.empty()) {
        ThrowException("Expected at least one object with no parent");
    }

    aiNode* root = out->mRootNode = new aiNode("<BlenderRoot>");

    root->mNumChildren = static_cast<unsigned int>(no_parents.size());
    root->mChildren = new aiNode*[root->mNumChildren]();
    for (unsigned int i = 0; i < root->mNumChildren; ++i) {
        root->mChildren[i] = ConvertNode(in, no_parents[i], conv, aiMatrix4x4());
        root->mChildren[i]->mParent = root;
    }

    BuildMaterials(conv);

    if (conv.meshes->size()) {
        out->mMeshes = new aiMesh*[out->mNumMeshes = static_cast<unsigned int>( conv.meshes->size() )];
        std::copy(conv.meshes->begin(),conv.meshes->end(),out->mMeshes);
        conv.meshes.dismiss();
    }

    if (conv.lights->size()) {
        out->mLights = new aiLight*[out->mNumLights = static_cast<unsigned int>( conv.lights->size() )];
        std::copy(conv.lights->begin(),conv.lights->end(),out->mLights);
        conv.lights.dismiss();
    }

    if (conv.cameras->size()) {
        out->mCameras = new aiCamera*[out->mNumCameras = static_cast<unsigned int>( conv.cameras->size() )];
        std::copy(conv.cameras->begin(),conv.cameras->end(),out->mCameras);
        conv.cameras.dismiss();
    }

    if (conv.materials->size()) {
        out->mMaterials = new aiMaterial*[out->mNumMaterials = static_cast<unsigned int>( conv.materials->size() )];
        std::copy(conv.materials->begin(),conv.materials->end(),out->mMaterials);
        conv.materials.dismiss();
    }

    if (conv.textures->size()) {
        out->mTextures = new aiTexture*[out->mNumTextures = static_cast<unsigned int>( conv.textures->size() )];
        std::copy(conv.textures->begin(),conv.textures->end(),out->mTextures);
        conv.textures.dismiss();
    }

    // acknowledge that the scene might come out incomplete
    // by Assimps definition of `complete`: blender scenes
    // can consist of thousands of cameras or lights with
    // not a single mesh between them.
    if (!out->mNumMeshes) {
        out->mFlags |= AI_SCENE_FLAGS_INCOMPLETE;
    }
}

// ------------------------------------------------------------------------------------------------
void BlenderImporter::ResolveImage(aiMaterial* out, const Material* mat, const MTex* tex, const Image* img, ConversionData& conv_data)
{
    (void)mat; (void)tex; (void)conv_data;
    aiString name;

    // check if the file contents are bundled with the BLEND file
    if (img->packedfile) {
        name.data[0] = '*';
        name.length = 1+ ASSIMP_itoa10(name.data+1,MAXLEN-1,conv_data.textures->size());

        conv_data.textures->push_back(new aiTexture());
        aiTexture* tex = conv_data.textures->back();

        // usually 'img->name' will be the original file name of the embedded textures,
        // so we can extract the file extension from it.
        const size_t nlen = strlen( img->name );
        const char* s = img->name+nlen, *e = s;

        while (s >= img->name && *s != '.')--s;

        tex->achFormatHint[0] = s+1>e ? '\0' : ::tolower( s[1] );
        tex->achFormatHint[1] = s+2>e ? '\0' : ::tolower( s[2] );
        tex->achFormatHint[2] = s+3>e ? '\0' : ::tolower( s[3] );
        tex->achFormatHint[3] = '\0';

        // tex->mHeight = 0;
        tex->mWidth = img->packedfile->size;
        uint8_t* ch = new uint8_t[tex->mWidth];

        conv_data.db.reader->SetCurrentPos(static_cast<size_t>( img->packedfile->data->val));
        conv_data.db.reader->CopyAndAdvance(ch,tex->mWidth);

        tex->pcData = reinterpret_cast<aiTexel*>(ch);

        LogInfo("Reading embedded texture, original file was "+std::string(img->name));
    }
    else {
        name = aiString( img->name );
    }

    aiTextureType texture_type = aiTextureType_UNKNOWN;
    MTex::MapType map_type = tex->mapto;

    if (map_type & MTex::MapType_COL)
        texture_type = aiTextureType_DIFFUSE;
    else if (map_type & MTex::MapType_NORM) {
        if (tex->tex->imaflag & Tex::ImageFlags_NORMALMAP) {
            texture_type = aiTextureType_NORMALS;
        }
        else {
            texture_type = aiTextureType_HEIGHT;
        }
        out->AddProperty(&tex->norfac,1,AI_MATKEY_BUMPSCALING);
    }
    else if (map_type & MTex::MapType_COLSPEC)
        texture_type = aiTextureType_SPECULAR;
    else if (map_type & MTex::MapType_COLMIR)
        texture_type = aiTextureType_REFLECTION;
    //else if (map_type & MTex::MapType_REF)
    else if (map_type & MTex::MapType_SPEC)
        texture_type = aiTextureType_SHININESS;
    else if (map_type & MTex::MapType_EMIT)
        texture_type = aiTextureType_EMISSIVE;
    //else if (map_type & MTex::MapType_ALPHA)
    //else if (map_type & MTex::MapType_HAR)
    //else if (map_type & MTex::MapType_RAYMIRR)
    //else if (map_type & MTex::MapType_TRANSLU)
    else if (map_type & MTex::MapType_AMB)
        texture_type = aiTextureType_AMBIENT;
    else if (map_type & MTex::MapType_DISPLACE)
        texture_type = aiTextureType_DISPLACEMENT;
    //else if (map_type & MTex::MapType_WARP)

    out->AddProperty(&name,AI_MATKEY_TEXTURE(texture_type,
        conv_data.next_texture[texture_type]++));

}

// ------------------------------------------------------------------------------------------------
void BlenderImporter::AddSentinelTexture(aiMaterial* out, const Material* mat, const MTex* tex, ConversionData& conv_data)
{
    (void)mat; (void)tex; (void)conv_data;

    aiString name;
    name.length = sprintf(name.data, "Procedural,num=%i,type=%s",conv_data.sentinel_cnt++,
        GetTextureTypeDisplayString(tex->tex->type)
    );
    out->AddProperty(&name,AI_MATKEY_TEXTURE_DIFFUSE(
        conv_data.next_texture[aiTextureType_DIFFUSE]++)
    );
}

// ------------------------------------------------------------------------------------------------
void BlenderImporter::ResolveTexture(aiMaterial* out, const Material* mat, const MTex* tex, ConversionData& conv_data)
{
    const Tex* rtex = tex->tex.get();
    if(!rtex || !rtex->type) {
        return;
    }

    // We can't support most of the texture types because they're mostly procedural.
    // These are substituted by a dummy texture.
    const char* dispnam = "";
    switch( rtex->type )
    {
            // these are listed in blender's UI
        case Tex::Type_CLOUDS       :
        case Tex::Type_WOOD         :
        case Tex::Type_MARBLE       :
        case Tex::Type_MAGIC        :
        case Tex::Type_BLEND        :
        case Tex::Type_STUCCI       :
        case Tex::Type_NOISE        :
        case Tex::Type_PLUGIN       :
        case Tex::Type_MUSGRAVE     :
        case Tex::Type_VORONOI      :
        case Tex::Type_DISTNOISE    :
        case Tex::Type_ENVMAP       :

            // these do no appear in the UI, why?
        case Tex::Type_POINTDENSITY :
        case Tex::Type_VOXELDATA    :

            LogWarn(std::string("Encountered a texture with an unsupported type: ")+dispnam);
            AddSentinelTexture(out, mat, tex, conv_data);
            break;

        case Tex::Type_IMAGE        :
            if (!rtex->ima) {
                LogError("A texture claims to be an Image, but no image reference is given");
                break;
            }
            ResolveImage(out, mat, tex, rtex->ima.get(),conv_data);
            break;

        default:
            ai_assert(false);
    };
}

// ------------------------------------------------------------------------------------------------
void BlenderImporter::BuildMaterials(ConversionData& conv_data)
{
    conv_data.materials->reserve(conv_data.materials_raw.size());

    // add a default material if necessary
    unsigned int index = static_cast<unsigned int>( -1 );
    for_each( aiMesh* mesh, conv_data.meshes.get() ) {
        if (mesh->mMaterialIndex == static_cast<unsigned int>( -1 )) {

            if (index == static_cast<unsigned int>( -1 )) {
                // Setup a default material.
                boost::shared_ptr<Material> p(new Material());
                ai_assert(::strlen(AI_DEFAULT_MATERIAL_NAME) < sizeof(p->id.name)-2);
                strcpy( p->id.name+2, AI_DEFAULT_MATERIAL_NAME );

                // Note: MSVC11 does not zero-initialize Material here, although it should.
                // Thus all relevant fields should be explicitly initialized. We cannot add
                // a default constructor to Material since the DNA codegen does not support
                // parsing it.
                p->r = p->g = p->b = 0.6f;
                p->specr = p->specg = p->specb = 0.6f;
                p->ambr = p->ambg = p->ambb = 0.0f;
                p->mirr = p->mirg = p->mirb = 0.0f;
                p->emit = 0.f;
                p->alpha = 0.f;
                p->har = 0;

                index = static_cast<unsigned int>( conv_data.materials_raw.size() );
                conv_data.materials_raw.push_back(p);
                LogInfo("Adding default material");
            }
            mesh->mMaterialIndex = index;
        }
    }

    for_each(boost::shared_ptr<Material> mat, conv_data.materials_raw) {

        // reset per material global counters
        for (size_t i = 0; i < sizeof(conv_data.next_texture)/sizeof(conv_data.next_texture[0]);++i) {
            conv_data.next_texture[i] = 0 ;
        }

        aiMaterial* mout = new aiMaterial();
        conv_data.materials->push_back(mout);
        // For any new material field handled here, the default material above must be updated with an appropriate default value.

        // set material name
        aiString name = aiString(mat->id.name+2); // skip over the name prefix 'MA'
        mout->AddProperty(&name,AI_MATKEY_NAME);


        // basic material colors
        aiColor3D col(mat->r,mat->g,mat->b);
        if (mat->r || mat->g || mat->b ) {

            // Usually, zero diffuse color means no diffuse color at all in the equation.
            // So we omit this member to express this intent.
            mout->AddProperty(&col,1,AI_MATKEY_COLOR_DIFFUSE);

            if (mat->emit) {
                aiColor3D emit_col(mat->emit * mat->r, mat->emit * mat->g, mat->emit * mat->b) ;
                mout->AddProperty(&emit_col, 1, AI_MATKEY_COLOR_EMISSIVE) ;
            }
        }

        col = aiColor3D(mat->specr,mat->specg,mat->specb);
        mout->AddProperty(&col,1,AI_MATKEY_COLOR_SPECULAR);

        // is hardness/shininess set?
        if( mat->har ) {
            const float har = mat->har;
            mout->AddProperty(&har,1,AI_MATKEY_SHININESS);
        }

        col = aiColor3D(mat->ambr,mat->ambg,mat->ambb);
        mout->AddProperty(&col,1,AI_MATKEY_COLOR_AMBIENT);

        col = aiColor3D(mat->mirr,mat->mirg,mat->mirb);
        mout->AddProperty(&col,1,AI_MATKEY_COLOR_REFLECTIVE);

        for(size_t i = 0; i < sizeof(mat->mtex) / sizeof(mat->mtex[0]); ++i) {
            if (!mat->mtex[i]) {
                continue;
            }

            ResolveTexture(mout,mat.get(),mat->mtex[i].get(),conv_data);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void BlenderImporter::CheckActualType(const ElemBase* dt, const char* check)
{
    ai_assert(dt);
    if (strcmp(dt->dna_type,check)) {
        ThrowException((format(),
            "Expected object at ",std::hex,dt," to be of type `",check,
            "`, but it claims to be a `",dt->dna_type,"`instead"
        ));
    }
}

// ------------------------------------------------------------------------------------------------
void BlenderImporter::NotSupportedObjectType(const Object* obj, const char* type)
{
    LogWarn((format(), "Object `",obj->id.name,"` - type is unsupported: `",type, "`, skipping" ));
}

// ------------------------------------------------------------------------------------------------
void BlenderImporter::ConvertMesh(const Scene& /*in*/, const Object* /*obj*/, const Mesh* mesh,
    ConversionData& conv_data, TempArray<std::vector,aiMesh>&  temp
    )
{
    // TODO: Resolve various problems with BMesh triangluation before re-enabling.
    //       See issues #400, #373, #318  #315 and #132.
#if defined(TODO_FIX_BMESH_CONVERSION)
    BlenderBMeshConverter BMeshConverter( mesh );
    if ( BMeshConverter.ContainsBMesh( ) )
    {
        mesh = BMeshConverter.TriangulateBMesh( );
    }
#endif

    typedef std::pair<const int,size_t> MyPair;
    if ((!mesh->totface && !mesh->totloop) || !mesh->totvert) {
        return;
    }

    // some sanity checks
    if (static_cast<size_t> ( mesh->totface ) > mesh->mface.size() ){
        ThrowException("Number of faces is larger than the corresponding array");
    }

    if (static_cast<size_t> ( mesh->totvert ) > mesh->mvert.size()) {
        ThrowException("Number of vertices is larger than the corresponding array");
    }

    if (static_cast<size_t> ( mesh->totloop ) > mesh->mloop.size()) {
        ThrowException("Number of vertices is larger than the corresponding array");
    }

    // collect per-submesh numbers
    std::map<int,size_t> per_mat;
    std::map<int,size_t> per_mat_verts;
    for (int i = 0; i < mesh->totface; ++i) {

        const MFace& mf = mesh->mface[i];
        per_mat[ mf.mat_nr ]++;
        per_mat_verts[ mf.mat_nr ] += mf.v4?4:3;
    }

    for (int i = 0; i < mesh->totpoly; ++i) {
        const MPoly& mp = mesh->mpoly[i];
        per_mat[ mp.mat_nr ]++;
        per_mat_verts[ mp.mat_nr ] += mp.totloop;
    }

    // ... and allocate the corresponding meshes
    const size_t old = temp->size();
    temp->reserve(temp->size() + per_mat.size());

    std::map<size_t,size_t> mat_num_to_mesh_idx;
    for_each(MyPair& it, per_mat) {

        mat_num_to_mesh_idx[it.first] = temp->size();
        temp->push_back(new aiMesh());

        aiMesh* out = temp->back();
        out->mVertices = new aiVector3D[per_mat_verts[it.first]];
        out->mNormals  = new aiVector3D[per_mat_verts[it.first]];

        //out->mNumFaces = 0
        //out->mNumVertices = 0
        out->mFaces = new aiFace[it.second]();

        // all submeshes created from this mesh are named equally. this allows
        // curious users to recover the original adjacency.
        out->mName = aiString(mesh->id.name+2);
            // skip over the name prefix 'ME'

        // resolve the material reference and add this material to the set of
        // output materials. The (temporary) material index is the index
        // of the material entry within the list of resolved materials.
        if (mesh->mat) {

            if (static_cast<size_t> ( it.first ) >= mesh->mat.size() ) {
                ThrowException("Material index is out of range");
            }

            boost::shared_ptr<Material> mat = mesh->mat[it.first];
            const std::deque< boost::shared_ptr<Material> >::iterator has = std::find(
                    conv_data.materials_raw.begin(),
                    conv_data.materials_raw.end(),mat
            );

            if (has != conv_data.materials_raw.end()) {
                out->mMaterialIndex = static_cast<unsigned int>( std::distance(conv_data.materials_raw.begin(),has));
            }
            else {
                out->mMaterialIndex = static_cast<unsigned int>( conv_data.materials_raw.size() );
                conv_data.materials_raw.push_back(mat);
            }
        }
        else out->mMaterialIndex = static_cast<unsigned int>( -1 );
    }

    for (int i = 0; i < mesh->totface; ++i) {

        const MFace& mf = mesh->mface[i];

        aiMesh* const out = temp[ mat_num_to_mesh_idx[ mf.mat_nr ] ];
        aiFace& f = out->mFaces[out->mNumFaces++];

        f.mIndices = new unsigned int[ f.mNumIndices = mf.v4?4:3 ];
        aiVector3D* vo = out->mVertices + out->mNumVertices;
        aiVector3D* vn = out->mNormals + out->mNumVertices;

        // XXX we can't fold this easily, because we are restricted
        // to the member names from the BLEND file (v1,v2,v3,v4)
        // which are assigned by the genblenddna.py script and
        // cannot be changed without breaking the entire
        // import process.

        if (mf.v1 >= mesh->totvert) {
            ThrowException("Vertex index v1 out of range");
        }
        const MVert* v = &mesh->mvert[mf.v1];
        vo->x = v->co[0];
        vo->y = v->co[1];
        vo->z = v->co[2];
        vn->x = v->no[0];
        vn->y = v->no[1];
        vn->z = v->no[2];
        f.mIndices[0] = out->mNumVertices++;
        ++vo;
        ++vn;

        //  if (f.mNumIndices >= 2) {
        if (mf.v2 >= mesh->totvert) {
            ThrowException("Vertex index v2 out of range");
        }
        v = &mesh->mvert[mf.v2];
        vo->x = v->co[0];
        vo->y = v->co[1];
        vo->z = v->co[2];
        vn->x = v->no[0];
        vn->y = v->no[1];
        vn->z = v->no[2];
        f.mIndices[1] = out->mNumVertices++;
        ++vo;
        ++vn;

        if (mf.v3 >= mesh->totvert) {
            ThrowException("Vertex index v3 out of range");
        }
        //  if (f.mNumIndices >= 3) {
        v = &mesh->mvert[mf.v3];
        vo->x = v->co[0];
        vo->y = v->co[1];
        vo->z = v->co[2];
        vn->x = v->no[0];
        vn->y = v->no[1];
        vn->z = v->no[2];
        f.mIndices[2] = out->mNumVertices++;
        ++vo;
        ++vn;

        if (mf.v4 >= mesh->totvert) {
            ThrowException("Vertex index v4 out of range");
        }
        //  if (f.mNumIndices >= 4) {
        if (mf.v4) {
            v = &mesh->mvert[mf.v4];
            vo->x = v->co[0];
            vo->y = v->co[1];
            vo->z = v->co[2];
            vn->x = v->no[0];
            vn->y = v->no[1];
            vn->z = v->no[2];
            f.mIndices[3] = out->mNumVertices++;
            ++vo;
            ++vn;

            out->mPrimitiveTypes |= aiPrimitiveType_POLYGON;
        }
        else out->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;

        //  }
        //  }
        //  }
    }

    for (int i = 0; i < mesh->totpoly; ++i) {

        const MPoly& mf = mesh->mpoly[i];

        aiMesh* const out = temp[ mat_num_to_mesh_idx[ mf.mat_nr ] ];
        aiFace& f = out->mFaces[out->mNumFaces++];

        f.mIndices = new unsigned int[ f.mNumIndices = mf.totloop ];
        aiVector3D* vo = out->mVertices + out->mNumVertices;
        aiVector3D* vn = out->mNormals + out->mNumVertices;

        // XXX we can't fold this easily, because we are restricted
        // to the member names from the BLEND file (v1,v2,v3,v4)
        // which are assigned by the genblenddna.py script and
        // cannot be changed without breaking the entire
        // import process.
        for (int j = 0;j < mf.totloop; ++j)
        {
            const MLoop& loop = mesh->mloop[mf.loopstart + j];

            if (loop.v >= mesh->totvert) {
                ThrowException("Vertex index out of range");
            }

            const MVert& v = mesh->mvert[loop.v];

            vo->x = v.co[0];
            vo->y = v.co[1];
            vo->z = v.co[2];
            vn->x = v.no[0];
            vn->y = v.no[1];
            vn->z = v.no[2];
            f.mIndices[j] = out->mNumVertices++;

            ++vo;
            ++vn;

        }
        if (mf.totloop == 3)
        {
            out->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
        }
        else
        {
            out->mPrimitiveTypes |= aiPrimitiveType_POLYGON;
        }
    }

    // collect texture coordinates, they're stored in a separate per-face buffer
    if (mesh->mtface || mesh->mloopuv) {
        if (mesh->totface > static_cast<int> ( mesh->mtface.size())) {
            ThrowException("Number of UV faces is larger than the corresponding UV face array (#1)");
        }
        for (std::vector<aiMesh*>::iterator it = temp->begin()+old; it != temp->end(); ++it) {
            ai_assert((*it)->mNumVertices && (*it)->mNumFaces);

            (*it)->mTextureCoords[0] = new aiVector3D[(*it)->mNumVertices];
            (*it)->mNumFaces = (*it)->mNumVertices = 0;
        }

        for (int i = 0; i < mesh->totface; ++i) {
            const MTFace* v = &mesh->mtface[i];

            aiMesh* const out = temp[ mat_num_to_mesh_idx[ mesh->mface[i].mat_nr ] ];
            const aiFace& f = out->mFaces[out->mNumFaces++];

            aiVector3D* vo = &out->mTextureCoords[0][out->mNumVertices];
            for (unsigned int i = 0; i < f.mNumIndices; ++i,++vo,++out->mNumVertices) {
                vo->x = v->uv[i][0];
                vo->y = v->uv[i][1];
            }
        }

        for (int i = 0; i < mesh->totpoly; ++i) {
            const MPoly& v = mesh->mpoly[i];
            aiMesh* const out = temp[ mat_num_to_mesh_idx[ v.mat_nr ] ];
            const aiFace& f = out->mFaces[out->mNumFaces++];

            aiVector3D* vo = &out->mTextureCoords[0][out->mNumVertices];
            for (unsigned int j = 0; j < f.mNumIndices; ++j,++vo,++out->mNumVertices) {
                const MLoopUV& uv = mesh->mloopuv[v.loopstart + j];
                vo->x = uv.uv[0];
                vo->y = uv.uv[1];
            }

        }
    }

    // collect texture coordinates, old-style (marked as deprecated in current blender sources)
    if (mesh->tface) {
        if (mesh->totface > static_cast<int> ( mesh->tface.size())) {
            ThrowException("Number of faces is larger than the corresponding UV face array (#2)");
        }
        for (std::vector<aiMesh*>::iterator it = temp->begin()+old; it != temp->end(); ++it) {
            ai_assert((*it)->mNumVertices && (*it)->mNumFaces);

            (*it)->mTextureCoords[0] = new aiVector3D[(*it)->mNumVertices];
            (*it)->mNumFaces = (*it)->mNumVertices = 0;
        }

        for (int i = 0; i < mesh->totface; ++i) {
            const TFace* v = &mesh->tface[i];

            aiMesh* const out = temp[ mat_num_to_mesh_idx[ mesh->mface[i].mat_nr ] ];
            const aiFace& f = out->mFaces[out->mNumFaces++];

            aiVector3D* vo = &out->mTextureCoords[0][out->mNumVertices];
            for (unsigned int i = 0; i < f.mNumIndices; ++i,++vo,++out->mNumVertices) {
                vo->x = v->uv[i][0];
                vo->y = v->uv[i][1];
            }
        }
    }

    // collect vertex colors, stored separately as well
    if (mesh->mcol || mesh->mloopcol) {
        if (mesh->totface > static_cast<int> ( (mesh->mcol.size()/4)) ) {
            ThrowException("Number of faces is larger than the corresponding color face array");
        }
        for (std::vector<aiMesh*>::iterator it = temp->begin()+old; it != temp->end(); ++it) {
            ai_assert((*it)->mNumVertices && (*it)->mNumFaces);

            (*it)->mColors[0] = new aiColor4D[(*it)->mNumVertices];
            (*it)->mNumFaces = (*it)->mNumVertices = 0;
        }

        for (int i = 0; i < mesh->totface; ++i) {

            aiMesh* const out = temp[ mat_num_to_mesh_idx[ mesh->mface[i].mat_nr ] ];
            const aiFace& f = out->mFaces[out->mNumFaces++];

            aiColor4D* vo = &out->mColors[0][out->mNumVertices];
            for (unsigned int n = 0; n < f.mNumIndices; ++n, ++vo,++out->mNumVertices) {
                const MCol* col = &mesh->mcol[(i<<2)+n];

                vo->r = col->r;
                vo->g = col->g;
                vo->b = col->b;
                vo->a = col->a;
            }
            for (unsigned int n = f.mNumIndices; n < 4; ++n);
        }

        for (int i = 0; i < mesh->totpoly; ++i) {
            const MPoly& v = mesh->mpoly[i];
            aiMesh* const out = temp[ mat_num_to_mesh_idx[ v.mat_nr ] ];
            const aiFace& f = out->mFaces[out->mNumFaces++];

            aiColor4D* vo = &out->mColors[0][out->mNumVertices];
            for (unsigned int j = 0; j < f.mNumIndices; ++j,++vo,++out->mNumVertices) {
                const MLoopCol& col = mesh->mloopcol[v.loopstart + j];
                vo->r = col.r;
                vo->g = col.g;
                vo->b = col.b;
                vo->a = col.a;
            }

        }

    }

    return;
}

// ------------------------------------------------------------------------------------------------
aiCamera* BlenderImporter::ConvertCamera(const Scene& /*in*/, const Object* obj, const Camera* /*camera*/, ConversionData& /*conv_data*/)
{
    ScopeGuard<aiCamera> out(new aiCamera());
    out->mName = obj->id.name+2;
    out->mPosition = aiVector3D(0.f, 0.f, 0.f);
    out->mUp = aiVector3D(0.f, 1.f, 0.f);
    out->mLookAt = aiVector3D(0.f, 0.f, -1.f);
    return out.dismiss();
}

// ------------------------------------------------------------------------------------------------
aiLight* BlenderImporter::ConvertLight(const Scene& /*in*/, const Object* obj, const Lamp* lamp, ConversionData& /*conv_data*/)
{
    ScopeGuard<aiLight> out(new aiLight());
    out->mName = obj->id.name+2;

    switch (lamp->type)
    {
        case Lamp::Type_Local:
            out->mType = aiLightSource_POINT;
            break;
        case Lamp::Type_Sun:
            out->mType = aiLightSource_DIRECTIONAL;

            // blender orients directional lights as facing toward -z
            out->mDirection = aiVector3D(0.f, 0.f, -1.f);
            break;
        default:
            break;
    }

    out->mColorAmbient = aiColor3D(lamp->r, lamp->g, lamp->b) * lamp->energy;
    out->mColorSpecular = aiColor3D(lamp->r, lamp->g, lamp->b) * lamp->energy;
    out->mColorDiffuse = aiColor3D(lamp->r, lamp->g, lamp->b) * lamp->energy;
    return out.dismiss();
}

// ------------------------------------------------------------------------------------------------
aiNode* BlenderImporter::ConvertNode(const Scene& in, const Object* obj, ConversionData& conv_data, const aiMatrix4x4& parentTransform)
{
    std::deque<const Object*> children;
    for(ObjectSet::iterator it = conv_data.objects.begin(); it != conv_data.objects.end() ;) {
        const Object* object = *it;
        if (object->parent == obj) {
            children.push_back(object);

            conv_data.objects.erase(it++);
            continue;
        }
        ++it;
    }

    ScopeGuard<aiNode> node(new aiNode(obj->id.name+2)); // skip over the name prefix 'OB'
    if (obj->data) {
        switch (obj->type)
        {
        case Object :: Type_EMPTY:
            break; // do nothing


            // supported object types
        case Object :: Type_MESH: {
            const size_t old = conv_data.meshes->size();

            CheckActualType(obj->data.get(),"Mesh");
            ConvertMesh(in,obj,static_cast<const Mesh*>(obj->data.get()),conv_data,conv_data.meshes);

            if (conv_data.meshes->size() > old) {
                node->mMeshes = new unsigned int[node->mNumMeshes = static_cast<unsigned int>(conv_data.meshes->size()-old)];
                for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
                    node->mMeshes[i] = i + old;
                }
            }}
            break;
        case Object :: Type_LAMP: {
            CheckActualType(obj->data.get(),"Lamp");
            aiLight* mesh = ConvertLight(in,obj,static_cast<const Lamp*>(
                obj->data.get()),conv_data);

            if (mesh) {
                conv_data.lights->push_back(mesh);
            }}
            break;
        case Object :: Type_CAMERA: {
            CheckActualType(obj->data.get(),"Camera");
            aiCamera* mesh = ConvertCamera(in,obj,static_cast<const Camera*>(
                obj->data.get()),conv_data);

            if (mesh) {
                conv_data.cameras->push_back(mesh);
            }}
            break;


            // unsupported object types / log, but do not break
        case Object :: Type_CURVE:
            NotSupportedObjectType(obj,"Curve");
            break;
        case Object :: Type_SURF:
            NotSupportedObjectType(obj,"Surface");
            break;
        case Object :: Type_FONT:
            NotSupportedObjectType(obj,"Font");
            break;
        case Object :: Type_MBALL:
            NotSupportedObjectType(obj,"MetaBall");
            break;
        case Object :: Type_WAVE:
            NotSupportedObjectType(obj,"Wave");
            break;
        case Object :: Type_LATTICE:
            NotSupportedObjectType(obj,"Lattice");
            break;

            // invalid or unknown type
        default:
            break;
        }
    }

    for(unsigned int x = 0; x < 4; ++x) {
        for(unsigned int y = 0; y < 4; ++y) {
            node->mTransformation[y][x] = obj->obmat[x][y];
        }
    }

    aiMatrix4x4 m = parentTransform;
    m = m.Inverse();

    node->mTransformation = m*node->mTransformation;

    if (children.size()) {
        node->mNumChildren = static_cast<unsigned int>(children.size());
        aiNode** nd = node->mChildren = new aiNode*[node->mNumChildren]();
        for_each (const Object* nobj,children) {
            *nd = ConvertNode(in,nobj,conv_data,node->mTransformation * parentTransform);
            (*nd++)->mParent = node;
        }
    }

    // apply modifiers
    modifier_cache->ApplyModifiers(*node,conv_data,in,*obj);

    return node.dismiss();
}


#endif
