project "zip"
    kind "StaticLib"
    language "C"
    staticruntime "off"
    warnings "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.location}/src/*.h",
        "%{prj.location}/src/*.c"
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

    filter "configurations:Dist"
		    runtime "Release"
		    symbols "off"
		    optimize "Speed"
