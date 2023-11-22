project "Assimp"
    kind "StaticLib"
    language "C++"
    cppdialect "C++11"
    staticruntime "off"
    warnings "off"

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
    }

    filter "system:linux"
        pic "On"
		systemversion "latest"

    filter "system:macosx"
		pic "On"

    filter "system:windows"
		systemversion "latest"
