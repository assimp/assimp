/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team

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

#ifndef ASSIMP_BUILD_NO_JT_IMPORTER

#include "JTImporter.h"
#include <assimp/Compiler/pushpack1.h>
#include <assimp/Compiler/poppack1.h>

#include <assimp/ByteSwapper.h>
#include <assimp/DefaultIOSystem.h>

#include "JTData.h"

#include <memory>

namespace Assimp {

static const aiImporterDesc desc = {
	"JT File Format from Siemens",
	"",
	"",
	"",
	aiImporterFlags_SupportTextFlavour,
	0,
	0,
	0,
	0,
	"jt"
};

using namespace ::Assimp::JT;

JTImporter::JTImporter() :
		BaseImporter(),
		mModel(nullptr) {
	// empty
}

JTImporter::~JTImporter() {
	delete mModel;
	mModel = nullptr;
}

bool JTImporter::CanRead(const std::string &file, IOSystem *pIOHandler, bool checkSig) const {
	if (!checkSig) {
		//Check File Extension
		return SimpleExtensionCheck(file, "jt");
	}

	// Check file Header

	return false;
}

const aiImporterDesc *JTImporter::GetInfo() const {
	return &desc;
}

void JTImporter::InternReadFile(const std::string &file, aiScene *scene, IOSystem *ioHandler) {
	ai_assert(nullptr != scene);
	ai_assert(nullptr != ioHandler);

	static const std::string mode = "rb";
	std::unique_ptr<IOStream> fileStream(ioHandler->Open(file, mode));
	if (!fileStream.get()) {
		throw DeadlyImportError("Failed to open file " + file + ".");
	}

	// Get the file-size and validate it, throwing an exception when fails
	size_t fileSize = fileStream->FileSize();
	if (fileSize < sizeof(JT::JTHeader)) {
		throw DeadlyImportError("JT-file is too small.");
	}

	mModel = new JTModel;
	DataBuffer buffer;
	buffer.resize(fileSize);
	fileStream->Read(&buffer[0], sizeof(char), fileSize);

	size_t offset(0);
	readHeader(buffer, offset);
	readTOCSegment(mModel->header.toc_offset, buffer, offset);

	for (size_t i = 0; i < mModel->toc_segment.num_entries; ++i) {
	}
}

void JTImporter::readHeader(DataBuffer &buffer, size_t &offset) {
	::memcpy(&mModel->header, &buffer[offset], sizeof(JTHeader));
	offset += sizeof(JTHeader);
}

void JTImporter::readTOCSegment(size_t toc_offset, DataBuffer &buffer, size_t &offset) {
	i32 num_entries(0);
	::memcpy(&num_entries, &buffer[toc_offset], sizeof(i32));
	offset = toc_offset + sizeof(i32);
	mModel->toc_segment.allocEntries(num_entries);
	for (i32 i = 0; i < num_entries; ++i) {
		JTTOCEntry *entry = mModel->toc_segment.getEntryAt(i);
		if (nullptr == entry) {
			continue;
		}

		::memcpy(entry, &buffer[offset], sizeof(JTTOCEntry));
	}
}

void JTImporter::readDataSegment(JTTOCEntry *entry, DataBuffer &buffer, size_t &offset) {
	if (nullptr == entry) {
		return;
	}

	SegmentHeader header;
	::memcpy(&header, &buffer[entry->segment_offset], entry->segment_lenght);
	header.SegmentID;
	const bool compressed = isZLibCompressionEnabled((SegmentTypeEnum)header.SegmentType);
	switch ((SegmentTypeEnum)header.SegmentType) {
		case SegmentTypeEnum::LogicalScenegraph:
			readLSGSegment(header, compressed, buffer, offset);
			break;

		case SegmentTypeEnum::JT_BRep:
			break;
		case SegmentTypeEnum::PMIData:
			break;
		case SegmentTypeEnum::MetaData:
			break;
		case SegmentTypeEnum::Shape:
			break;
		case SegmentTypeEnum::ShapeLOD0:
			break;
		case SegmentTypeEnum::ShapeLOD1:
			break;
		case SegmentTypeEnum::ShapeLOD2:
			break;
		case SegmentTypeEnum::ShapeLOD3:
			break;
		case SegmentTypeEnum::ShapeLOD4:
			break;
		case SegmentTypeEnum::ShapeLOD5:
			break;
		case SegmentTypeEnum::ShapeLOD6:
			break;
		case SegmentTypeEnum::ShapeLOD7:
			break;
		case SegmentTypeEnum::ShapeLOD8:
			break;
		case SegmentTypeEnum::ShapeLOD9:
			break;
		case SegmentTypeEnum::XT_BRep:
			break;
		case SegmentTypeEnum::WireframeRepresentation:
			break;
		case SegmentTypeEnum::ULP:
			break;
		case SegmentTypeEnum::LWPA:
			break;
		default:
			break;
	}
}

void JTImporter::readLogicalElementHeaderZLib(LogicalElementHeaderZLib &headerZLib, DataBuffer &buffer, size_t &offset) {
	::memcpy(&headerZLib, &buffer[offset], sizeof(LogicalElementHeaderZLib));
}

void readBaseNodeData(BaseNodeData &baseNodeData, JTImporter::DataBuffer &buffer, size_t &offset) {

    /* 0x10dd1035, 0x2ac8, 0x11d1, 0x9b, 0x6b, 0x00, 0x80, 0xc7, 0xbb, 0x59, 0x97 */

    ::memcpy(&baseNodeData.Version, &buffer[offset], sizeof(i16));
	offset += sizeof(i16);
	::memcpy(&baseNodeData.NodeFlags, &buffer[offset], sizeof(u32));
	offset += sizeof(u32);
	::memcpy(&baseNodeData.AttributeCount, &buffer[offset], sizeof(i32));
	offset += sizeof(i32);
	if (baseNodeData.AttributeCount > 0) {
		baseNodeData.AttributeObjectIds = new i32[baseNodeData.AttributeCount];
		::memcpy(baseNodeData.AttributeObjectIds, &buffer[offset], sizeof(i32) * baseNodeData.AttributeCount);
	}
}

void readVertexCountRange(VertexCountRange &vcRange, JTImporter::DataBuffer &buffer, size_t &offset) {
	::memcpy(&vcRange.MinCount, &buffer[offset], sizeof(i32));
	offset += sizeof(i32);
	::memcpy(&vcRange.MaxCount, &buffer[offset], sizeof(i32));
	offset += sizeof(i32);
}

void readGroupNodeData( GroupNodeData &gnData, JTImporter::DataBuffer &buffer, size_t &offset ) {
	readBaseNodeData(gnData.BNData, buffer, offset);
	::memcpy(&gnData.VersionNumber, &buffer[offset], sizeof(i16));
	offset += sizeof(i16);
	::memcpy(&gnData.ChildCount, &buffer[offset], sizeof(i32));
	offset += sizeof(i32);
	if (gnData.ChildCount == 0) {
		return;
    }

    gnData.ChildNodeObjIds = new i32[gnData.ChildCount];
    for (i32 i = 0; i < gnData.ChildCount; ++i) {
		::memcpy(gnData.ChildNodeObjIds, &buffer[offset], sizeof(i32) * gnData.ChildCount);
    }
}

void readPartitionNodeData() {

    /* : 0x10dd103e, 0x2ac8, 0x11d1, 0x9b, 0x6b, 0x00, 0x80, 0xc7, 0xbb, 0x59, 0x97 */


}

void JTImporter::readLSGSegment(SegmentHeader header, bool isCompressed, DataBuffer &buffer, size_t &offset) {
	if (isCompressed) {
		LogicalElementHeaderZLib header;
		readLogicalElementHeaderZLib(header, buffer, offset);
	}

}

} // namespace Assimp

#endif // ASSIMP_BUILD_NO_JT_IMPORTER
