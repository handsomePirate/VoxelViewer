#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(local_size_x = 16, local_size_y = 16) in;
layout(binding = 0, rgba8) uniform writeonly image2D resultImage;

layout(std140, binding = 1) buffer PageTable
{
	uint pointers[];
};

layout(std140, binding = 2) buffer Pages
{
	uint entries[];
};

struct TreeRoot
{
	uint rootNode;
	int rootOffsetX;
	int rootOffsetY;
	int rootOffsetZ;
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

uint Translate(uint vptr)
{
	const uint page = vptr / 512;
	const uint offset = vptr % 512;
	return pointers[page] * 512 + offset;
}

void main()
{
	float green = gl_GlobalInvocationID.x / 2048.f;
	float blue = gl_GlobalInvocationID.y / 2048.f;
	uint sum = 0;
	for (int i = 0; i < pointers.length(); ++i)
	{
		sum += pointers[i];
	}
	vec4 testColor = (sum == 65 * 2) ? vec4(1, 0, 0, 1) : vec4(0, 0, 0, 1);
	//imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(blue, 0.f, green), 1.f));
	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), testColor);
}