#version 450

layout(local_size_x = 16, local_size_y = 16) in;
layout(binding = 0, rgba8) uniform writeonly image2D resultImage;

void main()
{
	float green = gl_GlobalInvocationID.x / 2048.f;
	float blue = gl_GlobalInvocationID.y / 2048.f;
	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), vec4(blue, 0.f, green, 1.f));
}