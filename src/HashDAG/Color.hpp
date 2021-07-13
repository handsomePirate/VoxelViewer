#pragma once
#include "Core/Common.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include <openvdb/openvdb.h>

struct ColorGPUInfo
{
	VulkanFactory::Buffer::BufferInfo ColorsStorageBuffer;
	VulkanFactory::Buffer::BufferInfo ColorOffsetsStorageBuffer;
	VulkanFactory::Buffer::BufferInfo ColorIndicesStorageBuffer;
	VulkanFactory::Buffer::BufferInfo ColorIndexOffsetsStorageBuffer;
};

class Color
{
public:
	typedef uint32_t index_t;

	Color(int voxelCount);
	void Set(uint64_t index, const openvdb::Vec3s& color);

	bool Compressed() const;

	VkDeviceSize GetBufferSize() const;
	uint32_t GetMemoryUsed() const;
	void* GetDataPointer();

	void CompressSimilar(float epsilonMargin);

	VkDeviceSize GetBufferSizeIndices() const;
	void* GetDataPointerIndices();
	VkDeviceSize GetBufferSizeCompressed() const;
	void* GetDataPointerCompressed();

private:
	std::vector<openvdb::Vec4s> voxelMap_;
	std::vector<openvdb::Vec4s> compressed_;
	std::vector<index_t> indices_;
};
