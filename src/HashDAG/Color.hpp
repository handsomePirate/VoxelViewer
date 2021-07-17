#pragma once
#include "Core/Common.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include <openvdb/openvdb.h>
#include <unordered_set>

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
	void Set(uint64_t index, const openvdb::Vec3s& color, bool init = false);
	openvdb::Vec3s Get(uint64_t index) const;

	void StartOperation();
	void EndOperation();

	bool Compressed() const;

	VkDeviceSize GetBufferSize() const;
	uint32_t GetMemoryUsed() const;
	void* GetDataPointer();

	void CompressSimilar(float epsilonMargin);

	VkDeviceSize GetBufferSizeIndices() const;
	void* GetDataPointerIndices();
	VkDeviceSize GetBufferSizeCompressed() const;
	void* GetDataPointerCompressed();

	bool Undo(uint64_t& rangeStart, uint64_t& rangeEnd);
	bool Redo(uint64_t& rangeStart, uint64_t& rangeEnd);

private:
	std::vector<openvdb::Vec4s> voxelMap_;
	std::vector<openvdb::Vec4s> compressed_;
	std::vector<index_t> indices_;

	struct ColorOperation
	{
		openvdb::Vec4s Original;
		openvdb::Vec4s New;
		uint64_t VoxelIndex;

		ColorOperation(openvdb::Vec4s originalColor, openvdb::Vec4s newColor, uint64_t voxelIndex)
			: Original(originalColor), New(newColor), VoxelIndex(voxelIndex) {}

		bool operator==(const ColorOperation& other) const
		{
			return VoxelIndex == other.VoxelIndex;
		}
	};

	struct OperationHash
	{
		size_t operator()(const ColorOperation& operation) const
		{
			return operation.VoxelIndex;
		}
	};

	typedef std::unordered_set<ColorOperation, OperationHash> OperationSet;
	int historyIndex_ = 0;
	int historyStart_ = 0;
	const int historySize_ = 16;
	int historyEnd_ =  0;
	std::vector<OperationSet> history_;
};
