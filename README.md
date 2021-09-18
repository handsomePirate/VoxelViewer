# VoxelViewer
A real-time program to view VDB voxel files. It offers visual color editing and inspecting.

## Building
The program is Windows-only and uses premake5 for building. For a Visual Studio 19 files build, use BootstrapVS19.bat. This will generate files that MSVC needs to build the binaries.

## Running
Voxel Viewer expects
```shell
VoxelViewer.exe <grid-filename> <grid-name> | VoxelViewer.exe -l <grid-filename> <grid-name> | VoxelViewer.exe
```

If the exe is run without arguments, it will launch with the default dragon model. Any VDB grid that is input as filename and grid name (inside the file) must be `openvdb::Vec3SGrid`. It is also possible to load level sets (`openvdb::FloatGrid`) by prepending -l, however they will be completely filled in with white.