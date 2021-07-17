#include "Color.hpp"

Color::Color(int voxelCount)
{
	voxelMap_.resize(voxelCount);
	history_.resize(historySize_);
}

void Color::Set(uint64_t index, const openvdb::Vec3s& color, bool init)
{
	openvdb::Vec4s color4 = { color.x(), color.y(), color.z(), 0 };
	if (!init)
	{
		history_[historyIndex_].emplace(voxelMap_[index], color4, index);
	}
	voxelMap_[index] = color4;
}

openvdb::Vec3s Color::Get(uint64_t index) const
{
	return openvdb::Vec3s(voxelMap_[index].x(), voxelMap_[index].y(), voxelMap_[index].z());
}

void Color::StartOperation()
{
	while (historyEnd_ != historyIndex_)
	{
		historyEnd_ = (historyEnd_ == 0) ? (historySize_ - 1) : (historyEnd_ - 1);
		history_[historyEnd_].clear();
	}
}

void Color::EndOperation()
{
	historyIndex_ = (historyIndex_ + 1) % historySize_;
	historyEnd_ = (historyEnd_ + 1) % historySize_;
	if (historyEnd_ == historyStart_)
	{
		history_[historyStart_].clear();
		historyStart_ = (historyStart_ + 1) % historySize_;
	}
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
	if (Compressed())
	{
		return (uint32_t)(compressed_.size() * sizeof(openvdb::Vec3s)) + (uint32_t)(indices_.size() * sizeof(index_t));
	}
	else
	{
		return (uint32_t)(voxelMap_.size() * sizeof(openvdb::Vec3s));
	}
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
	return (VkDeviceSize)(indices_.size() * sizeof(index_t));
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

bool Color::Undo(uint64_t& rangeStart, uint64_t& rangeEnd)
{
	if (historyIndex_ != historyStart_)
	{
		historyIndex_ = (historyIndex_ == 0) ? (historySize_ - 1) : (historyIndex_ - 1);

		uint64_t minIndex = 0xFFFFFFFFFFFFFFFF;
		uint64_t maxIndex = 0;
		for (const auto& operation : history_[historyIndex_])
		{
			voxelMap_[operation.VoxelIndex] = operation.Original;
			minIndex = (std::min)(minIndex, operation.VoxelIndex);
			maxIndex = (std::max)(maxIndex, operation.VoxelIndex);
		}

		rangeStart = minIndex;
		rangeEnd = maxIndex;

		return true;
	}

	return false;
}

bool Color::Redo(uint64_t& rangeStart, uint64_t& rangeEnd)
{
	if (historyIndex_ != historyEnd_)
	{
		uint64_t minIndex = 0xFFFFFFFFFFFFFFFF;
		uint64_t maxIndex = 0;
		for (const auto& operation : history_[historyIndex_])
		{
			voxelMap_[operation.VoxelIndex] = operation.New;
			minIndex = (std::min)(minIndex, operation.VoxelIndex);
			maxIndex = (std::max)(maxIndex, operation.VoxelIndex);
		}

		historyIndex_ = (historyIndex_ + 1) % historySize_;

		rangeStart = minIndex;
		rangeEnd = maxIndex;

		return true;
	}

	return false;
}
