workspace "VoxelViewer"
	architecture "x64"
	configurations { "Debug", "Release" }
	location "proj"

project "VoxelViewer"
	kind "ConsoleApp"
	language "C++"
	location "proj"
	targetdir "build/%{cfg.buildcfg}"
	objdir "proj/obj/%{cfg.buildcfg}"
	files { "src/**.hpp", "src/**.cpp" }
	--include { "ext" }
	
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
	
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
	
	filter "configurations:Release"
		defines { "RELEASE" }
		optimize "On"