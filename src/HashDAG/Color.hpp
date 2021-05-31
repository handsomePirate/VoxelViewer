#pragma once
#include "Core/Common.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include <openvdb/openvdb.h>

struct ColorGPUInfo
{
	VulkanFactory::Buffer::BufferInfo ColorsStorageBuffer;
	VulkanFactory::Buffer::BufferInfo ColorOffsetsStorageBuffer;
};

class Color
{
public:
	Color(int voxelCount);
	void Set(uint64_t index, const openvdb::Vec3s& color);

	VkDeviceSize GetBufferSize() const;
	void* GetDataPointer();

private:
	std::vector<openvdb::Vec4s> voxelMap_;
};
