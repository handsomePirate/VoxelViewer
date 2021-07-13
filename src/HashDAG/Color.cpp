#include "Color.hpp"

Color::Color(int voxelCount)
{
	voxelMap_.resize(voxelCount);
}

void Color::Set(uint64_t index, const openvdb::Vec3s& color)
{
	voxelMap_[index] = { color.x(), color.y(), color.z(), 0 };
}

bool Color::Compressed() const
{
	return indices_.size() > 0;
}

VkDeviceSize Color::GetBufferSize() const
{
	return (VkDeviceSize)(voxelMap_.size() * sizeof(openvdb::Vec4s));
}

uint32_t Color::GetMemoryUsed() const
{
	return (uint32_t)(voxelMap_.size() * sizeof(openvdb::Vec3s));
}

void* Color::GetDataPointer()
{
	return voxelMap_.data();
}

void Color::CompressSimilar(float epsilonMargin)
{
	int current = 0;
	indices_.resize(voxelMap_.size());
	std::vector<int> weights;

	for (int i = 0; i < voxelMap_.size(); ++i)
	{
		int found = -1;
		for (int k = 0; k < current; ++k)
		{
			if ((voxelMap_[i] - compressed_[k]).lengthSqr() < epsilonMargin)
			{
				found = k;
				break;
			}
		}

		if (found > -1)
		{
			compressed_[found] = (compressed_[found] * weights[found] + voxelMap_[i]) / float(weights[found] + 1);
			++weights[found];
			indices_[i] = found;
		}
		else
		{
			compressed_.push_back(voxelMap_[i]);
			weights.push_back(1);
			indices_[i] = current;
			++current;
		}
	}
}

VkDeviceSize Color::GetBufferSizeIndices() const
{
	return (VkDeviceSize)(indices_.size() * sizeof(index_t));;
}

void* Color::GetDataPointerIndices()
{
	return indices_.data();
}

VkDeviceSize Color::GetBufferSizeCompressed() const
{
	return (VkDeviceSize)(compressed_.size() * sizeof(openvdb::Vec4s));
}

void* Color::GetDataPointerCompressed()
{
	return compressed_.data();
}
