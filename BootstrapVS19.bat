@ECHO off
REM Check that the premake EXE file exists.
IF NOT EXIST "ext\premake\bin\release\premake5.exe" (
	REM Check that the premake submodule is present.
	IF NOT EXIST "ext\premake\.git" (
		REM If the submodule was not present, we need to update our submodules.
		REM NOTE: We assume that our submodules contain the premake submodule.
		ECHO -- Premake cloning --
		git submodule update --init --recursive
	)	
	REM If the premake EXE file was not present, we need to build it.
	ECHO -- Premake building --
	cd ext\premake
	CALL Bootstrap.bat
	cd ..\..
)
REM Run the premake procedure to generate a VS 2019 solution.
REM NOTE: Uses the premake5.lua file to obtain the generation details.
ECHO -- Configuring solution --
CALL ext\premake\bin\release\premake5.exe vs2019
PAUSE