#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(local_size_x = 16, local_size_y = 16) in;
layout(binding = 0, rgba8) uniform writeonly image2D resultImage;

layout(std140, binding = 1) buffer PageTable
{
	// The individual data entries need to be aligned by vec4, ivec4 or uvec4 as required by std140.
	// Other alignment options are not available for arrays (as far as I know).
	// Individual array elements should be querried as follows: pointers[i / 4][i % 4].
	uvec4 pointers[];
};

layout(std140, binding = 2) buffer Pages
{
	uvec4 entries[];
};

struct TreeRoot
{
	// The order matters here, because of alignment.
	ivec3 rootOffset;
	uint rootNode;
};

layout(std140, binding = 3) buffer TreeRoots
{
	TreeRoot treeRoots[];
};

layout(binding = 4) uniform TreesData
{
	uint pageCount;
	uint treeCount;
} treesData;

uint PageTableElement(uint e)
{
	return pointers[e / 4][e % 4];
}

uint PagePoolElement(uint e)
{
	return entries[e / 4][e % 4];
}

uint Translate(uint vptr)
{
	const uint page = vptr / 512;
	const uint offset = vptr % 512;
	return PageTableElement(page) * 512 + offset;
}

void main()
{
	float green = gl_GlobalInvocationID.x / 2048.f;
	float blue = gl_GlobalInvocationID.y / 2048.f;
	
	const int pointerIndex = 4;
	vec4 testColor = (PagePoolElement(Translate(treeRoots[0].rootNode)) != 0) ? vec4(1, 0, 0, 1) : vec4(0, 0, 0, 1);
	//imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(blue, 0.f, green), 1.f));
	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), testColor);
}