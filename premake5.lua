project "Assimp"
    kind "StaticLib"
    language "C++"
    cppdialect "C++11"
    staticruntime "off"
    warnings "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files 
    {
        "code/AssetLib/**.h",
        "code/AssetLib/**.cpp",

        "code/Common/*.h",
        "code/Common/*.cpp",

        "code/Geometry/*.h",
        "code/Geometry/*.cpp",
        
        "code/Material/*.h",
        "code/Material/*.cpp",

        "code/Pbrt/*.h",
        "code/Pbrt/*.cpp",

        "code/PostProcessing/*.h",
        "code/PostProcessing/*.cpp",

        "contrib/pugixml/src/*.h",
        "contrib/pugixml/src/*.cpp",
    }

    defines
    {
        #"ASSIMP_DOUBLE_PRECISION"
    }
    
    includedirs
    {
        "%{prj.location}/code",
        "%{prj.location}/include"
    }

    filter "system:linux"
        pic "On"
		    systemversion "latest"

    filter "system:macosx"
		    pic "On"

    filter "system:windows"
		    systemversion "latest"
