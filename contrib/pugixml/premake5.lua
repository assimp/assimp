project "pugixml"
    kind "StaticLib"
    language "C++"
    cppdialect "C++11"
    staticruntime "off"
    warnings "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/*.hpp",
        "src/*.cpp",
    }

    filter "system:linux"
        pic "On"
		systemversion "latest"

    filter "system:macosx"
		pic "On"

    filter "system:windows"
		systemversion "latest"