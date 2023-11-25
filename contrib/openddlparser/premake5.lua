project "openddlparser"
    kind "StaticLib"
    language "C++"
    cppdialect "C++11"
    staticruntime "off"
    warnings "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")


    files
    {
        "%{prj.location}/code/*.cpp",
        "%{prj.location}/include/*.h"
    }

    includedirs
    {
        "%{prj.location}/include"
    }

    defines
    {
        "OPENDDL_STATIC_LIBARY"
    }

    filter "system:linux"
        pic "On"
		systemversion "latest"

    filter "system:macosx"
		pic "On"

    filter "system:windows"
		systemversion "latest"