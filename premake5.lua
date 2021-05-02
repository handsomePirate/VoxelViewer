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
	files { "**.hpp", "**.cpp" }

filter "configurations:Debug"
	defines { "DEBUG" }
	symbols "On"

filter "configurations:Release"
	defines { "NDEBUG" }
	optimize "On"