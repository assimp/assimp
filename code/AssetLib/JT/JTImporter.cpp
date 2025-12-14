//

#include "JT/JTImporter.h"
#include <assimp/StreamReader.h>

#include <cstddef>

namespace Assimp::JT {

enum class SegmentType {
	Invalid = -1, //< Predefined class: Invalid
	LOGICAL_SCENE_GRAPH,//< Predefined class: Logical_Scene_Graph
	JT_BREP,//< Predefined class: JT_BRep
	PMI_DATA, //<Predefined class: PMI_Data
	META_DATA, //< Predefined class: Meta_Data
	SHAPE, //< Predefined class: Shape
	SHAPE_LOD0, //< Predefined class: Shape_LOD0
	SHAPE_LOD1, //< Predefined class: Logical_Scene_Graph
	SHAPE_LOD2, //< Predefined class: Shape_LOD2
	SHAPE_LOD3, //< Predefined class: Shape_LOD3
	SHAPE_LOD4, //< Predefined class: Shape_LOD4
	SHAPE_LOD5, //< Predefined class: Shape_LOD5
	SHAPE_LOD6, //< Predefined class: Shape_LOD6
	SHAPE_LOD7, //< Predefined class: Shape_LOD7
	SHAPE_LOD8, //< Predefined class: Shape_LOD8
	SHAPE_LOD9, ///< Predefined class: Shape_LOD9
	XT_BREP, //< Predefined class: XT_BRep
    Count //< Number of segment types
};

struct Guid {
    uint32_t Data0{};
    uint32_t Data1{};
    uint32_t Data2{}
    uint32_t Data3{};
};

struct Version {
    constexpr static size_t VersionSize = 80;
    uint8_t Version[VersionSize]{};
    int32_t Empty{0};
    uint64_t TOCOffset{0};

}
struct SegmentHeader {
    Guid          SegmentGuid{};
    SegmentType   Type{SegmentType::Invalid};
};

namespace {
    void readVersion(StreamReaderLE& reader, Version& version) {
        reader.ReadBytes(version.Version, Version::VersionSize);
        version.Empty = reader.Read<int32_t>();
        version.TOCOffset = reader.Read<uint64_t>();
    }
} // Anonymous namespace

JTImporter::JTImporter() : BaseImporter() {
}

JTImporter::~JTImporter() {

}

bool JTImporter::CanRead(const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const {

}
    
void JTImporter::setupProperties(const Importer* pImp) {

}

void InternReadFile(const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler) {
    auto stream = std::shared_ptr<IOStream>(pIOHandler->Open(pFile, "rb"));
    if (!stream) {
        throw DeadlyImportError("Failed to open JT file " + pFile + " for reading.");
    }   
    StreamReaderLE reader(stream);
    Version version;
    readVersion(reader, version);
}

} // Namespace Assimp
