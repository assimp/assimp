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
        "OPENDDL_STATIC_LIBARY",
        "OPENDDLPARSER_BUILD"
    }

    -- OS specific
    filter "system:linux"
        pic "On"
		systemversion "latest"

    filter "system:macosx"
		pic "On"

    filter "system:windows"
		systemversion "latest"

    -- Configuration stuff
    filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "Speed"

    filter "configurations:DIST"
		runtime "Release"
		symbols "off"
		optimize "Speed"