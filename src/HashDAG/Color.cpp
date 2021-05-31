#include "Color.hpp"

Color::Color(int voxelCount)
{
	voxelMap_.resize(voxelCount);
}

void Color::Set(uint64_t index, const openvdb::Vec3s& color)
{
	voxelMap_[index] = { color.x(), color.y(), color.z(), 0 };
}

VkDeviceSize Color::GetBufferSize() const
{
	return (VkDeviceSize)(voxelMap_.size() * sizeof(openvdb::Vec4s));
}

void* Color::GetDataPointer()
{
	return voxelMap_.data();
}
