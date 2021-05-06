workspace "VoxelViewer"
	architecture "x64"
	configurations { "Debug", "Release" }
	location "proj"

-- This takes care of shader compilation for Visual Studio, for other systems (aside from gmake2) a different
-- compilation pipeline must be devised.
-- Solution found here: https://stackoverflow.com/questions/39072485/compiling-glsl-to-spir-v-using-premake-5-and-visual-studio-2015
rule "GLSLShaderCompilationRule"
	display "GLSL Shader Compiler"
	fileextension ".glsl"

	propertydefinition {
	  name = "Configuration",
	  display = "Configuration",
	  values = {
	    [0] = "Debug",
	    [1] = "Release",
	  },
	  switch = {
	    [0] = "Debug",
	    [1] = "Release",
	  },
	  value = 1,
	}

	buildmessage 'Compiling shader...'
	buildcommands {
		'@echo off & "$(VULKAN_SDK)\\Bin\\glslangValidator.exe" -V "%{file.directory}%{file.name}" -o "%{file.directory}..\\..\\build\\[Configuration]\\%{file.name}.spv"'
	}
	buildoutputs {
		"shadersareneverdone"
	}

project "VoxelViewer"
	rules { "GLSLShaderCompilationRule" }
	kind "ConsoleApp"
	language "C++"
	location "proj"
	targetdir "./build/%{cfg.buildcfg}"
	objdir "proj/obj/%{cfg.buildcfg}"
	files { "src/**.hpp", "src/**.cpp", "src/**.glsl" }
	includedirs { "ext", "src/Core", "$(VULKAN_SDK)/include" }
	links { "$(VULKAN_SDK)/lib/vulkan-1.lib" }

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
	
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
		GLSLShaderCompilationRuleVars {
			Configuration = "Debug"
		}
	
	filter "configurations:Release"
		defines { "RELEASE" }
		optimize "On"