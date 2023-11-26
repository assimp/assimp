ContribPath = "%{prj.location}/contrib"

ContribIncludeDirs = {
    "%{ContribPath}",
    "%{ContribPath}/zlib",
    "%{ContribPath}/zip/src",
    "%{ContribPath}/unzip",
    "%{ContribPath}/utf8cpp/source",
    "%{ContribPath}/stb",
    "%{ContribPath}/rapidjson/include",
    "%{ContribPath}/pugixml/src",
    "%{ContribPath}/plo2tri/plo2tri",
    "%{ContribPath}/openddlparser/include",
    "%{ContribPath}/Open3DGC",
    "%{ContribPath}/clipper"
}

ContribSourceFiles = {}
ContribSourceFilesIndex = 0

function AddSourceFilesContrib(sourceFiles)
    for _, file in ipairs(sourceFiles) do 
        ContribSourceFiles[ContribSourceFilesIndex] = file
        ContribSourceFilesIndex = ContribSourceFilesIndex + 1
    end
end

ClipperSourceFiles = {
    "%{ContribPath}/clipper/clipper.hpp",
    "%{ContribPath}/clipper/clipper.cpp"
}
AddSourceFilesContrib(ClipperSourceFiles)

Poly2triSourceFiles = {
    "%{ContribPath}/poly2tri/poly2tri/common/shapes.cc",
    "%{ContribPath}/poly2tri/poly2tri/common/shapes.h",
    "%{ContribPath}/poly2tri/poly2tri/common/utils.h",
    "%{ContribPath}/poly2tri/poly2tri/sweep/advancing_front.h",
    "%{ContribPath}/poly2tri/poly2tri/sweep/advancing_front.cc",
    "%{ContribPath}/poly2tri/poly2tri/sweep/cdt.cc",
    "%{ContribPath}/poly2tri/poly2tri/sweep/cdt.h",
    "%{ContribPath}/poly2tri/poly2tri/sweep/sweep.cc",
    "%{ContribPath}/poly2tri/poly2tri/sweep/sweep.h",
    "%{ContribPath}/poly2tri/poly2tri/sweep/sweep_context.cc",
    "%{ContribPath}/poly2tri/poly2tri/sweep/sweep_context.h"
}
AddSourceFilesContrib(Poly2triSourceFiles)

UnzipSourceFiles = {
    "%{ContribPath}/unzip/crypt.h",
    "%{ContribPath}/unzip/ioapi.c",
    "%{ContribPath}/unzip/ioapi.h",
    "%{ContribPath}/unzip/unzip.c",
    "%{ContribPath}/unzip/unzip.h"
}
AddSourceFilesContrib(UnzipSourceFiles)

ZiplibSourceFiles = {
    "%{ContribPath}/zip/src/miniz.h",
    "%{ContribPath}/zip/src/zip.c",
    "%{ContribPath}/zip/src/zip.h"
}
AddSourceFilesContrib(ZiplibSourceFiles)

OppenddlparserSourceFiles = {
    "%{ContribPath}/openddlparser/code/OpenDDLParser.cpp",
    "%{ContribPath}/openddlparser/code/DDLNode.cpp",
    "%{ContribPath}/openddlparser/code/OpenDDLCommon.cpp",
    "%{ContribPath}/openddlparser/code/OpenDDLExport.cpp",
    "%{ContribPath}/openddlparser/code/Value.cpp",
    "%{ContribPath}/openddlparser/code/OpenDDLStream.cpp",
    "%{ContribPath}/openddlparser/include/openddlparser/OpenDDLParser.h",
    "%{ContribPath}/openddlparser/include/openddlparser/OpenDDLParserUtils.h",
    "%{ContribPath}/openddlparser/include/openddlparser/OpenDDLCommon.h",
    "%{ContribPath}/openddlparser/include/openddlparser/OpenDDLExport.h",
    "%{ContribPath}/openddlparser/include/openddlparser/OpenDDLStream.h",
    "%{ContribPath}/openddlparser/include/openddlparser/DDLNode.h",
    "%{ContribPath}/openddlparser/include/openddlparser/Value.h"
}
AddSourceFilesContrib(OppenddlparserSourceFiles)


Open3dgcSourceFiles = {
    "%{ContribPath}/Open3DGC/o3dgcAdjacencyInfo.h",
    "%{ContribPath}/Open3DGC/o3dgcArithmeticCodec.cpp",
    "%{ContribPath}/Open3DGC/o3dgcArithmeticCodec.h",
    "%{ContribPath}/Open3DGC/o3dgcBinaryStream.h",
    "%{ContribPath}/Open3DGC/o3dgcCommon.h",
    "%{ContribPath}/Open3DGC/o3dgcDVEncodeParams.h",
    "%{ContribPath}/Open3DGC/o3dgcDynamicVectorDecoder.cpp",
    "%{ContribPath}/Open3DGC/o3dgcDynamicVectorDecoder.h",
    "%{ContribPath}/Open3DGC/o3dgcDynamicVectorEncoder.cpp",
    "%{ContribPath}/Open3DGC/o3dgcDynamicVectorEncoder.h",
    "%{ContribPath}/Open3DGC/o3dgcDynamicVector.h",
    "%{ContribPath}/Open3DGC/o3dgcFIFO.h",
    "%{ContribPath}/Open3DGC/o3dgcIndexedFaceSet.h",
    "%{ContribPath}/Open3DGC/o3dgcIndexedFaceSet.inl",
    "%{ContribPath}/Open3DGC/o3dgcSC3DMCDecoder.h",
    "%{ContribPath}/Open3DGC/o3dgcSC3DMCDecoder.inl",
    "%{ContribPath}/Open3DGC/o3dgcSC3DMCEncodeParams.h",
    "%{ContribPath}/Open3DGC/o3dgcSC3DMCEncoder.h",
    "%{ContribPath}/Open3DGC/o3dgcSC3DMCEncoder.inl",
    "%{ContribPath}/Open3DGC/o3dgcTimer.h",
    "%{ContribPath}/Open3DGC/o3dgcTools.cpp",
    "%{ContribPath}/Open3DGC/o3dgcTriangleFans.cpp",
    "%{ContribPath}/Open3DGC/o3dgcTriangleFans.h",
    "%{ContribPath}/Open3DGC/o3dgcTriangleListDecoder.h",
    "%{ContribPath}/Open3DGC/o3dgcTriangleListDecoder.inl",
    "%{ContribPath}/Open3DGC/o3dgcTriangleListEncoder.h",
    "%{ContribPath}/Open3DGC/o3dgcTriangleListEncoder.inl",
    "%{ContribPath}/Open3DGC/o3dgcVector.h",
    "%{ContribPath}/Open3DGC/o3dgcVector.inl"
}
AddSourceFilesContrib(Open3dgcSourceFiles)

