include "code/assimp_code.lua"
include "contrib/assimp_contrib.lua"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
    include "contrib/zlib"
    include "contrib/zip"
    include "contrib/pugixml"
    include "contrib/openddlparser"
group ""

project "Assimp"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    warnings "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")


    files 
    {
        AssimpSourceFiles,
        AssimpImporterSourceFiles,

        ContribSourceFiles
    }

    links
    {
        "zlib",
        "zip",
        "pugixml",
        "openddlparser"
    }
    
    includedirs
    {
        "%{prj.location}/code",
        "%{prj.location}/include",
        "%{prj.location}",
        
        ContribIncludeDirs
    }

    defines
    {
        --"ASSIMP_DOUBLE_PRECISION"
        "RAPIDJSON_HAS_STDSTRING",
        "OPENDDLPARSER_BUILD"
    }

    if (AssimpEnableNoneFreeC4DImporter == false) then
        defines
        {
            "ASSIMP_BUILD_NO_C4D_IMPORTER"
        }
    end

    filter "system:linux"
        pic "On"
		systemversion "latest"

    filter "system:macosx"
		pic "On"

    filter "system:windows"
		systemversion "latest"

    filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"