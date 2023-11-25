project "zlib"
    kind "StaticLib"
    language "C"
    staticruntime "off"
    warnings "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.location}/*.h",
        "%{prj.location}/*.c"
    }

    filter "system:linux"
        pic "On"
		systemversion "latest"

    filter "system:macosx"
		pic "On"

    filter "system:windows"
        systemversion "latest"
