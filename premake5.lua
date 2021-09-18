workspace "VoxelViewer"
	architecture "x64"
	configurations { "Debug", "Release" }
	location "proj"

group "ext"
	include "ext/imgui"
group ""

-- This takes care of shader compilation for Visual Studio, for other systems (aside from gmake2) a different
-- compilation pipeline must be devised.
-- Solution found here: https://stackoverflow.com/questions/39072485/compiling-glsl-to-spir-v-using-premake-5-and-visual-studio-2015
--rule "GLSLShaderCompilationRule"
--	display "GLSL Shader Compiler"
--	fileextension ".glsl"
--
--	propertydefinition {
--	  name = "Configuration",
--	  display = "Configuration",
--	  values = {
--	    [0] = "Debug",
--	    [1] = "Release",
--	  },
--	  switch = {
--	    [0] = "Debug",
--	    [1] = "Release",
--	  },
--	  value = 1,
--	}
--
--	buildmessage 'Compiling shader...'
--	buildcommands {
--		'@echo off & "$(VULKAN_SDK)\\Bin\\glslangValidator.exe" -V "%{file.directory}%{file.name}" -o "%{file.directory}..\\..\\build\\[Configuration]\\%{file.name}.spv"'
--	}
--	buildoutputs {
--		"shadersareneverdone"
--	}

project "VoxelViewer"
	--rules { "GLSLShaderCompilationRule" }
	kind "ConsoleApp"
	staticruntime "off"
	language "C++"
	cppdialect "C++17"
	location "proj"
	callingconvention "FastCall"
	targetdir "./build/%{cfg.buildcfg}"
	objdir "proj/obj/%{cfg.buildcfg}"
	files { "src/**.hpp", "src/**.cpp", "src/**.glsl" }

	linkoptions { '/NODEFAULTLIB:"tbb"' }

	flags {
		"MultiProcessorCompile"
	}

	includedirs {
		"src",
		"$(VULKAN_SDK)/include",
		"ext/imGui",
		"ext/Eigen/include",
		"ext/openvdb/ext/OpenEXR/include",
		"ext/openvdb/ext/boost/include",
		"ext/openvdb/ext/tbb/include",
		"ext/openvdb/include",
		"ext/NanoVDB/include",
	}

	links {
		"$(VULKAN_SDK)/lib/vulkan-1.lib",
		"ext/shaderc/lib/shaderc_shared.lib",
		"ext/imgui/bin/%{cfg.buildcfg}/ImGui.lib",
		"ext/openvdb/lib/%{cfg.buildcfg}/openvdb.lib",
		"ext/openvdb/ext/blosc/lib/%{cfg.buildcfg}/blosc.lib"
	}

	filter "system:windows"
		systemversion "latest"
	filter{}
	
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
		links {
			"ext/openvdb/ext/tbb/lib/%{cfg.buildcfg}/tbb_debug.lib",
			"ext/openvdb/ext/tbb/lib/%{cfg.buildcfg}/tbbmalloc_debug.lib",
			"ext/openvdb/ext/OpenEXR/lib/%{cfg.buildcfg}/Half-2_5_d.lib",
			"ext/openvdb/ext/OpenEXR/lib/%{cfg.buildcfg}/lz4d.lib",
			"ext/openvdb/ext/OpenEXR/lib/%{cfg.buildcfg}/snappyd.lib",
			"ext/openvdb/ext/zlib/lib/%{cfg.buildcfg}/zlibd.lib",
			"ext/openvdb/ext/zstd/lib/%{cfg.buildcfg}/zstdd.lib"
		}
		--GLSLShaderCompilationRuleVars {
		--	Configuration = "Debug"
		--}

	filter "configurations:Release"
		defines { "RELEASE" }
		optimize "On"
		links {
			"ext/openvdb/ext/tbb/lib/%{cfg.buildcfg}/tbb.lib",
			"ext/openvdb/ext/tbb/lib/%{cfg.buildcfg}/tbbmalloc.lib",
			"ext/openvdb/ext/OpenEXR/lib/%{cfg.buildcfg}/Half-2_5.lib",
			"ext/openvdb/ext/OpenEXR/lib/%{cfg.buildcfg}/lz4.lib",
			"ext/openvdb/ext/OpenEXR/lib/%{cfg.buildcfg}/snappy.lib",
			"ext/openvdb/ext/zlib/lib/%{cfg.buildcfg}/zlib.lib",
			"ext/openvdb/ext/zstd/lib/%{cfg.buildcfg}/zstd.lib"
		}

	filter {}
