#pragma once

#include "JTTypes.h"

namespace Assimp {
namespace JT {

enum class SegmentTypeEnum {
	LogicalScenegraph = 1,
	JT_BRep,
	PMIData,
	MetaData,
	Shape,
	ShapeLOD0,
	ShapeLOD1,
	ShapeLOD2,
	ShapeLOD3,
	ShapeLOD4,
	ShapeLOD5,
	ShapeLOD6,
	ShapeLOD7,
	ShapeLOD8,
	ShapeLOD9,
	XT_BRep,
	WireframeRepresentation,
	ULP,
	LWPA
};

static const char ZLibCompressionEnabled[] = {
	1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1
};

bool isZLibCompressionEnabled(SegmentTypeEnum type) {
	const size_t index = (size_t)type - 1;
	return 1 == ZLibCompressionEnabled[index];
}

#pragma pack(1)
struct JTHeader {
	uchar version[80];
	uchar byte_order;
	i32 reserved;
	i32 toc_offset;
	union {
		Guid LSG_segment_id;
		Guid reserved_field;
	};

	JTHeader() :
			byte_order(0), reserved(0), toc_offset(0) {
		::memset(version, ' ', sizeof(uchar) * 80);
	}
};

struct JTTOCEntry {
	Guid segment_id;
	i32 segment_offset;
	i32 segment_lenght;
	u32 segment_attributes;
};

struct JTTOCSegment {
	i32 num_entries;
	JTTOCEntry *entries;

	JTTOCSegment() :
			num_entries(0),
			entries(nullptr) {
		// empty
	}

	~JTTOCSegment() {
		delete[] entries;
	}

	void allocEntries(i32 numEntries) {
		num_entries = num_entries;
		entries = new JTTOCEntry[num_entries];
	}

	JTTOCEntry *getEntryAt(size_t index) {
		if (index > num_entries) {
			return nullptr;
		}

		return &entries[index];
	}
};

struct SegmentHeader {
	Guid SegmentID;
	i32 SegmentType;
	i32 SegmentLength;
};

struct LogicalElementHeader {
	i32 length;
};

struct LogicalElementHeaderZLib {
	i32 CompressionFlag;
	i32 CompressionDataLength;
	u8 CompressionAlgo;
};

struct ElementHeader {
	i32 ElementLength;
	Guid ObjectTypeID;
	uchar ObjectBaseType;
};

struct BaseNodeData {
	const char *Id = "0x10dd1035-0x2ac8-0x11d1-0x9b-0x6b-0x00-0x80-0xc7-0xbb-0x59-0x97";
					
    i16 Version;
	u32 NodeFlags;
	i32 AttributeCount;
	i32 *AttributeObjectIds;
};

struct VertexCountRange {
	i32 MinCount;
	i32 MaxCount;
};

struct JTModel {
	JTHeader header;
	JTTOCSegment toc_segment;

	JTModel() {
		// empty
	}
};

struct GroupNodeData {
	BaseNodeData BNData;
	i16 VersionNumber;
	i32 ChildCount;
	i32 *ChildNodeObjIds;

    GroupNodeData()
    : BNData()
    , VersionNumber(-1)
    , ChildCount(0)
    , ChildNodeObjIds(nullptr) {
        // empty
	}

    ~GroupNodeData() {
		delete[] ChildNodeObjIds;
	}
};

struct PartitionNodeElement {
	const char *Id = "0x10dd103e-0x2ac8-0x11d1-0x9b-0x6b-0x00-0x80-0xc7-0xbb-0x59-0x97";

	LogicalElementHeaderZLib LogicalHeaderZLib;
	GroupNodeData GNdata;
	i32 PartitionFlags;
	MbString Filename;
	BBoxF32 TransformedBBox;
	f32 Area;
	VertexCountRange VCRange;
	VertexCountRange NodeCountRange;
	VertexCountRange PolygonCountRange;
	BBoxF32 UntransformedBBox;
};

struct JTNode {
	virtual int getObjectId() const = 0;

	virtual ~JTNode() {
		// empty
	}
};

} // namespace JT
} // namespace Assimp
