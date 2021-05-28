#include "HashDAG.hpp"
#include "Core/Logger/Logger.hpp"

void HTStats::Print(std::ostream& os) const
{
	os << "Hash table stats.." << std::endl;
	os << "\tTotal memory allocated (MB): " << memoryAllocatedBytes / (1024.f * 1024.f) << std::endl;
	os << "\tPercentage of memory used to memory allocated (%): " << percentageOfMemoryUsed << std::endl;
	os << "\tNumber of empty buckets: " << emptyBuckets << std::endl;
	os << "\tEmpty to total bucket count ratio: " << emptyToTotalRatio << std::endl;
	os << std::endl;
	os << "\tAverage fullness percentage of top level buckets (%): " << avgTopBucketFullness << std::endl;
	os << "\tAverage fullness percentage of bottom level buckets (%): " << avgBottomBucketFullness << std::endl;
	os << "\tLevel node count (per level):" << std::endl;
	for (int i = 0; i <= HTConstants::LEAF_LEVEL; ++i)
	{
		os << "\t\tl" << i << ": " << levelNodeCount[i] << std::endl;
	}
}

HashTable::~HashTable()
{
	Destroy();
}

void HashTable::Init(uint32_t pagePoolSize)
{
	poolSize_ = pagePoolSize;
	pagePool_ = new uint32_t[poolSize_ * HTConstants::PAGE_SIZE];
	pageTable_ = new uint32_t[HTConstants::TOTAL_PAGE_COUNT];
	memset(pageTable_, 0, sizeof(uint32_t) * HTConstants::TOTAL_PAGE_COUNT);
	bucketsSizes_ = new uint32_t[HTConstants::TOTAL_BUCKET_COUNT];
	memset(bucketsSizes_, 0, sizeof(uint32_t) * HTConstants::TOTAL_BUCKET_COUNT);
}

void HashTable::Destroy()
{
	if (pagePool_)
	{
		delete[] pagePool_;
	}
	if (pageTable_)
	{
		delete[] pageTable_;
	}
	if (bucketsSizes_)
	{
		delete[] bucketsSizes_;
	}
}

HashTable::ptr_t HashTable::Translate(vptr_t ptr) const
{
	const uint32_t page = ptr / HTConstants::PAGE_SIZE;
	const uint32_t offset = ptr % HTConstants::PAGE_SIZE;
	return pagePool_ + pageTable_[page] * uint64_t(HTConstants::PAGE_SIZE) + offset;
}

HashTable::vptr_t HashTable::AddLeaf(const uint64_t leaf, const uint32_t hash)
{
	const bucket_t bucket = GetBucket(HTConstants::LEAF_LEVEL, hash);
	const uint32_t bucketSize = bucketsSizes_[bucket];

	// The bucket size contains the growing count of 32-bit entries in the bucket.
	const vptr_t bucketPtr = GetBucketPtr(bucket) + bucketSize;
	const uint32_t page = bucketPtr / HTConstants::PAGE_SIZE;

	if (0 == (bucketSize % HTConstants::PAGE_SIZE) && !IsPageAllocated(page))
	{
		AllocatePage(page);
	}

	assert(IsPageAllocated(page));

	uint64_t* entryPtr = (uint64_t*)Translate(bucketPtr);
	*entryPtr = leaf;
	bucketsSizes_[bucket] += 2;

	// TODO: Page Bloom filter here.

	return bucketPtr;
}

HashTable::vptr_t HashTable::AddNode(const uint32_t level, const uint32_t nodeSize, const uint32_t* node, const uint32_t hash)
{
	const bucket_t bucket = GetBucket(level, hash);
	uint32_t bucketSize = bucketsSizes_[bucket];

	// Make sure the node is in a single page.
	const uint32_t pageSpaceLeft = HTConstants::PAGE_SIZE - (bucketSize % HTConstants::PAGE_SIZE);
	vptr_t bucketPtr;
	uint32_t page;
	if (HTConstants::PAGE_SIZE == pageSpaceLeft || pageSpaceLeft < nodeSize)
	{
		// Update bucket size to include the page boundary
		if (HTConstants::PAGE_SIZE != pageSpaceLeft)
		{
			bucketSize += pageSpaceLeft;
		}

		// TODO: Track padding (pageSpaceLeft wasted).

		bucketPtr = GetBucketPtr(bucket) + bucketSize;
		page = bucketPtr / HTConstants::PAGE_SIZE;

		// New page, but still check if it's not allocated in case GC ran (TODO).
		if (!IsPageAllocated(page))
		{
			AllocatePage(page);
		}
	}
	else
	{
		bucketPtr = GetBucketPtr(bucket) + bucketSize;
		page = bucketPtr / HTConstants::PAGE_SIZE;
	}
	assert(IsPageAllocated(page));

	// TODO: There is a version without the page table if this causes trouble.

	ptr_t physicalBucketPtr = Translate(bucketPtr);
	for (uint32_t i = 0; i < nodeSize; i++)
	{
		physicalBucketPtr[i] = node[i];
	}

	bucketsSizes_[bucket] = bucketSize + nodeSize;

	// TODO: Page Bloom filter here.

	return bucketPtr;
}

HashTable::vptr_t HashTable::FindLeafInBucket(bucket_t bucket, uint64_t leaf) const
{
	vptr_t basePtr = GetBucketPtr(bucket);
	uint32_t bucketSize = bucketsSizes_[bucket];

	// The bucket could span across multiple pages, we need to iterate through them.
	for (uint32_t p = 0; p < bucketSize; p += HTConstants::PAGE_SIZE)
	{
		vptr_t pagePtr = basePtr + p;
		uint64_t* const physicalPtr = (uint64_t*)Translate(pagePtr);
		// Here, we go through all 32-bit pairs (tree leafs) and look for the right one.
		uint32_t leafCountInPage = (std::min)(bucketSize - p, HTConstants::PAGE_SIZE) / 2;
		for (uint32_t i = 0; i < leafCountInPage; ++i)
		{
			if (physicalPtr[i] == leaf)
			{
				// 'i' is the index of the leaf, its address is twice as big (2 32-bit entries).
				return pagePtr + i * 2;
			}
		}
	}

	// Leaf not present in this bucket.
	return HTConstants::INVALID_POINTER;
}

HashTable::vptr_t HashTable::FindNodeInBucket(bucket_t bucket, uint32_t nodeSize, uint32_t* const node) const
{
	vptr_t basePtr = GetBucketPtr(bucket);
	uint32_t bucketSize = bucketsSizes_[bucket];

	// The bucket could span across multiple pages, we need to iterate through them.
	for (uint32_t p = 0; p < bucketSize; p += HTConstants::PAGE_SIZE)
	{
		vptr_t pagePtr = basePtr + p;
		ptr_t const physicalPtr = Translate(pagePtr);
		// Here, we go through all 32-bit pairs (tree leafs) and look for the right one.
		uint32_t entryCount = (std::min)(bucketSize - p, HTConstants::PAGE_SIZE);

		if (p + nodeSize >= bucketSize)
		{
			return HTConstants::INVALID_POINTER;
		}

		uint32_t nodeLength = 0;
		for (uint32_t i = 0; i < entryCount; i += nodeLength)
		{
			if (std::memcmp(node, physicalPtr + i, nodeSize * sizeof(uint32_t)) == 0)
			{
				return pagePtr + i;
			}
			
			// The next cycle of the for loop will be moved by the size of the current node.
			nodeLength = GetNodeSize(physicalPtr + i);
		}
	}

	// Leaf not present in this bucket.
	return HTConstants::INVALID_POINTER;
}

HashTable::vptr_t HashTable::FindOrAddLeaf(uint64_t leaf)
{
	const uint32_t hash = HashLeaf(leaf);
	const bucket_t bucket = GetBucket(HTConstants::LEAF_LEVEL, hash);

	vptr_t ptr = FindLeafInBucket(bucket, leaf);

	if (ptr == HTConstants::INVALID_POINTER)
	{
		// TODO: Thread-safe part.

		ptr = AddLeaf(leaf, hash);
	}

	return ptr;
}

uint32_t HashTable::FindOrAddNode(uint32_t level, uint32_t nodeSize, uint32_t* const node)
{
	const uint32_t hash = HashNode(nodeSize, node);
	const bucket_t bucket = GetBucket(level, hash);

	vptr_t ptr = FindNodeInBucket(bucket, nodeSize, node);

	if (ptr == HTConstants::INVALID_POINTER)
	{
		// TODO: Thread-safe part.

		ptr = AddNode(level, nodeSize, node, hash);
	}

	return ptr;
}

uint32_t HashTable::GetMemAllocatedBytes() const
{
	return (poolSize_ * HTConstants::PAGE_SIZE + HTConstants::TOTAL_PAGE_COUNT + HTConstants::TOTAL_BUCKET_COUNT) *
		sizeof(uint32_t);
}

HTStats HashTable::GetStats() const
{
	HTStats result;
	result.memoryAllocatedBytes = GetMemAllocatedBytes();
	result.emptyBuckets = 0;
	uint32_t emptyTopBuckets = 0;
	uint32_t emptyBottomBuckets = 0;
	result.avgTopBucketFullness = 0;
	result.avgBottomBucketFullness = 0;
	for (int i = 0; i < HTConstants::TOTAL_BUCKET_COUNT; ++i)
	{
		result.emptyBuckets += bucketsSizes_[i] ? 0 : 1;
		if (i < HTConstants::TOTAL_TOP_BUCKET_COUNT)
		{
			emptyTopBuckets += bucketsSizes_[i] ? 0 : 1;
			result.avgTopBucketFullness += bucketsSizes_[i];
		}
		else
		{
			emptyBottomBuckets += bucketsSizes_[i] ? 0 : 1;
			result.avgBottomBucketFullness += bucketsSizes_[i];
		}
	}
	result.emptyToTotalRatio = result.emptyBuckets / (float)HTConstants::TOTAL_BUCKET_COUNT;
	result.emptyTopToTotalRatio = emptyTopBuckets / (float)HTConstants::TOTAL_TOP_BUCKET_COUNT;
	result.emptyBottomToTotalRatio = emptyBottomBuckets / (float)HTConstants::TOTAL_BOTTOM_BUCKET_COUNT;

	result.avgTopBucketFullness /= HTConstants::TOP_LEVEL_BUCKET_SIZE * HTConstants::TOTAL_TOP_BUCKET_COUNT;
	result.avgTopBucketFullness *= 100;

	result.avgBottomBucketFullness /= HTConstants::BOTTOM_LEVEL_BUCKET_SIZE * HTConstants::TOTAL_BOTTOM_BUCKET_COUNT;
	result.avgBottomBucketFullness *= 100;

	result.percentageOfMemoryUsed = poolTop_ / (float)poolSize_ * 100;

	for (int i = 0; i <= HTConstants::LEAF_LEVEL; ++i)
	{
		result.levelNodeCount[i] = CountLevelNodes(i);
	}

	return result;
}

void HashTable::UploadToGPU(const VulkanFactory::Device::DeviceInfo& deviceInfo, VkCommandPool commandPool,
	VkQueue queue, HashDAGGPUInfo& uploadInfo)
{
	VkDeviceSize pageTableBufferSize = HTConstants::TOTAL_PAGE_COUNT * sizeof(uint32_t);

	VulkanFactory::Buffer::Create("Voxel Page Table Storage Buffer", deviceInfo,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		pageTableBufferSize, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uploadInfo.PageTableStorageBuffer);
	CoreLogInfo("Page table size (MB): %f", uploadInfo.PageTableStorageBuffer.Size / 1048576.f);

	VulkanFactory::Buffer::BufferInfo pageTableStagingBufferInfo;
	VulkanFactory::Buffer::Create("Voxel Page Table Staging Buffer", deviceInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, pageTableBufferSize,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pageTableStagingBufferInfo);

	VulkanUtils::Buffer::Copy(deviceInfo.Handle, pageTableStagingBufferInfo.Memory, pageTableBufferSize, pageTable_);
	VulkanUtils::Buffer::Copy(deviceInfo.Handle, pageTableStagingBufferInfo.DescriptorBufferInfo.buffer,
		uploadInfo.PageTableStorageBuffer.DescriptorBufferInfo.buffer, pageTableBufferSize, commandPool, queue);

	VulkanFactory::Buffer::Destroy(deviceInfo, pageTableStagingBufferInfo);

	// Pages copy.
	VkDeviceSize pagesBufferSize = poolTop_ * HTConstants::PAGE_SIZE * sizeof(uint32_t);

	VulkanFactory::Buffer::Create("Voxel Pages Storage Buffer", deviceInfo,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		pagesBufferSize, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uploadInfo.PagesStorageBuffer);
	CoreLogInfo("Page table size (MB): %f", uploadInfo.PagesStorageBuffer.Size / 1048576.f);

	// TODO: Staging buffer.
	VulkanFactory::Buffer::BufferInfo pagesStagingBufferInfo;
	VulkanFactory::Buffer::Create("Voxel Pages Staging Buffer", deviceInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, pagesBufferSize,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pagesStagingBufferInfo);

	VulkanUtils::Buffer::Copy(deviceInfo.Handle, pagesStagingBufferInfo.Memory, pagesBufferSize, pagePool_);

	/*
	const auto GPUCopyPage = [&](uint32_t page, VkDeviceSize offset, VkDeviceSize size)
	{
		const uint32_t pageAddress = pageTable_[page];
		const uint32_t gpuPage = page;

		VulkanUtils::Buffer::Copy(deviceInfo.Handle, pagesStagingBufferInfo.Memory, size * sizeof(uint32_t),
			pagePool_ + (HTConstants::PAGE_SIZE * pageAddress + offset),
			(HTConstants::PAGE_SIZE * gpuPage + offset) * sizeof(uint32_t));
	};

	// TODO: for() -> transform pages and use offset and size to copy them to the GPU.
	for (uint32_t level = 0; level < HTConstants::MAX_LEVEL_COUNT; ++level)
	{
		uint32_t levelBucketCount = level < HTConstants::TOP_LEVEL_RANK ?
			HTConstants::TOP_LEVEL_BUCKET_COUNT : HTConstants::BOTTOM_LEVEL_BUCKET_COUNT;
		for (uint32_t bucketHash = 0; bucketHash < levelBucketCount; ++bucketHash)
		{
			const bucket_t bucket = GetBucket(level, bucketHash);
			const uint32_t bucketSize = bucketsSizes_[bucket];
			// TODO: If we ever want to just update geometric values instead of complete upload, we need to
			// add the notion of last bucket sizes and compare it to the current.
			const vptr_t bucketPtr = GetBucketPtr(bucket);
			uint32_t position = 0;
			while (position != bucketSize)
			{
				const vptr_t currentPtr = bucketPtr + position;
				const uint32_t spaceLeftInPage = HTConstants::PAGE_SIZE - (currentPtr % HTConstants::PAGE_SIZE);
				const uint32_t size = std::min(spaceLeftInPage, bucketSize - position);
				GPUCopyPage(currentPtr / HTConstants::PAGE_SIZE, currentPtr % HTConstants::PAGE_SIZE, size);
				position += size;
			}
			assert(position == bucketSize);
		}
	}
	*/

	// TODO: Copy data to storage buffer.
	VulkanUtils::Buffer::Copy(deviceInfo.Handle, pagesStagingBufferInfo.DescriptorBufferInfo.buffer,
		uploadInfo.PagesStorageBuffer.DescriptorBufferInfo.buffer, pagesBufferSize, commandPool, queue);

	// TODO: Think about uniforms/push constants for the rest of the data.
	// -Page count - uniform
	uploadInfo.PageCount = poolTop_;

	// TODO: Destroy staging buffers.
	VulkanFactory::Buffer::Destroy(deviceInfo, pagesStagingBufferInfo);
}

void HashTable::AllocatePage(uint32_t page)
{
	if (poolTop_ == poolSize_)
	{
		throw std::runtime_error("Out of DAG hash table memory.");
	}
	pageTable_[page] = poolTop_++;
}

bool HashTable::IsPageAllocated(uint32_t page)
{
	return pageTable_[page] != 0;
}

uint32_t HashTable::HashLeaf(uint64_t leaf)
{
	leaf ^= leaf >> 33;
	leaf *= 0xff51afd7ed558ccd;
	leaf ^= leaf >> 33;
	leaf *= 0xc4ceb9fe1a85ec53;
	leaf ^= leaf >> 33;
	return uint32_t(leaf);
}

uint32_t HashTable::CountLevelNodes(uint32_t level) const
{
	bucket_t bucket = 0;
	bool isTopLevel = level < HTConstants::TOP_LEVEL_RANK;

	if (isTopLevel)
	{
		bucket = level * HTConstants::TOP_LEVEL_BUCKET_COUNT;
	}
	else
	{
		bucket = HTConstants::TOTAL_TOP_BUCKET_COUNT + (level - HTConstants::TOP_LEVEL_RANK) * HTConstants::BOTTOM_LEVEL_BUCKET_COUNT;
	}

	uint32_t result = 0;
	uint32_t endBucket = bucket + (isTopLevel ? HTConstants::TOP_LEVEL_BUCKET_COUNT : HTConstants::BOTTOM_LEVEL_BUCKET_COUNT);
	for (; bucket < endBucket; ++bucket)
	{
		vptr_t basePtr = GetBucketPtr(bucket);
		uint32_t bucketSize = bucketsSizes_[bucket];
		if (level == HTConstants::LEAF_LEVEL)
		{
			result += bucketSize / 2;
			continue;
		}

		// The bucket could span across multiple pages, we need to iterate through them.
		for (uint32_t p = 0; p < bucketSize; p += HTConstants::PAGE_SIZE)
		{
			vptr_t pagePtr = basePtr + p;
			ptr_t const physicalPtr = Translate(pagePtr);
			// Here, we go through all 32-bit pairs (tree leafs) and look for the right one.
			uint32_t entryCount = (std::min)(bucketSize - p, HTConstants::PAGE_SIZE);

			uint32_t nodeSize = 0;
			for (uint32_t i = 0; i < entryCount; i += nodeSize)
			{
				// The next cycle of the for loop will be moved by the size of the current node.
				nodeSize = GetNodeSize(physicalPtr + i);
				++result;
			}
		}
	}

	return result;
}

uint32_t HashTable::HashNode(const uint32_t nodeSize, const uint32_t* node)
{
	constexpr uint32_t seed = 0;
	uint32_t h = seed;
	for (std::size_t i = 0; i < nodeSize; ++i)
	{
		uint32_t k = node[i];
		k *= 0xcc9e2d51;
		k = (k << 15) | (k >> 17);
		k *= 0x1b873593;
		h ^= k;
		h = (h << 13) | (h >> 19);
		h = h * 5 + 0xe6546b64;
	}

	h ^= nodeSize;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

HashTable::bucket_t HashTable::GetBucket(uint32_t level, uint32_t hash)
{
	const bool isTopLevel = level < HTConstants::TOP_LEVEL_RANK;

	const uint32_t bucketsPerLevel = isTopLevel ? HTConstants::TOP_LEVEL_BUCKET_COUNT : 
		HTConstants::BOTTOM_LEVEL_BUCKET_COUNT;

	// TODO: Check what this does.
	bucket_t bucket = hash & (bucketsPerLevel - 1);
	if (isTopLevel)
	{
		bucket += level * HTConstants::TOP_LEVEL_BUCKET_COUNT;
	}
	else
	{
		bucket += HTConstants::TOTAL_TOP_BUCKET_COUNT + 
			(level - HTConstants::TOP_LEVEL_RANK) * HTConstants::BOTTOM_LEVEL_BUCKET_COUNT;
	}
	return bucket;
}

HashTable::vptr_t HashTable::GetBucketPtr(const bucket_t bucket)
{
	if (bucket < HTConstants::TOTAL_TOP_BUCKET_COUNT)
	{
		// We are in the top level.
		return bucket * HTConstants::TOP_LEVEL_BUCKET_SIZE;
	}
	// We are in the bottom level.
	return HTConstants::TOTAL_TOP_BUCKET_COUNT * HTConstants::TOP_LEVEL_BUCKET_SIZE +
		(bucket - HTConstants::TOTAL_TOP_BUCKET_COUNT) * HTConstants::BOTTOM_LEVEL_BUCKET_SIZE;
}

uint32_t HashTable::GetNodeSize(uint32_t* const ptr)
{
	// The child mask are only the lower 8 bits of the 32-bit entry. '__popcnt' counts the set bits in
	// its argument. each child occupies 1 entry (node hash) +1 is for the child mask entry.
	return __popcnt(*ptr & 0xFF) + 1;
}

void HashDAG::Init(uint32_t pagePoolSize)
{
	ht_.Init(pagePoolSize);
}

HashTable::vptr_t HashDAG::FindOrAddLeaf(uint64_t leaf)
{
	return ht_.FindOrAddLeaf(leaf);
}

HashTable::vptr_t HashDAG::FindOrAddNode(uint32_t level, uint32_t nodeSize, uint32_t* const node)
{
	return ht_.FindOrAddNode(level, nodeSize, node);
}

void HashDAG::AddRoot(HashTable::vptr_t node, const Eigen::Vector3i& offset)
{
	trees_.push_back({ offset, node });
}

bool HashDAG::IsActive(const Eigen::Vector3i& voxel) const
{
	for (int t = 0; t < trees_.size(); ++t)
	{
		BoundingBox bbox;
		bbox.pos = trees_[t].rootOffset;
		bbox.span = { HTConstants::TREE_SPAN, HTConstants::TREE_SPAN, HTConstants::TREE_SPAN };

		if (BoundingBox::IsPointInCube(bbox, voxel))
		{
			return Traverse(voxel, trees_[t].rootNode, 0, bbox);
		}
	}

	return false;
}

bool HashDAG::CastRay(const Eigen::Vector3f& position, const Eigen::Vector3f& direction, Eigen::Vector3i& voxel,
	const Eigen::Vector3f& perturbationEpsilon) const
{
	const float altX = (((float)rand() / (RAND_MAX)) + 0.5f) * perturbationEpsilon.x();
	const float altY = (((float)rand() / (RAND_MAX)) + 0.5f) * perturbationEpsilon.y();
	const float altZ = (((float)rand() / (RAND_MAX)) + 0.5f) * perturbationEpsilon.z();
	Eigen::Vector3f altDirection(direction.x() + altX, direction.y() + altY, direction.z() + altZ);
	altDirection.normalize();
	// (1, 1, 1) / direction to later use to work out how many times the ray fits in a given distance.
	// As in the original paper code, the division of individual components is done in double and then cast into a float.
	float invDirX = float(1. / altDirection.x());
	float invDirY = float(1. / altDirection.y());
	float invDirZ = float(1. / altDirection.z());
	const Eigen::Vector3f invDirection(invDirX, invDirY, invDirZ);
	// Sets a bit for negative ray components, signifying that the ray would first hit the four children that have a 
	// higher value of this component than the other half. This is used to determine the order of children that we descend into
	// and is masked by the particular node's child mask.
	uint8_t rayChildOrder = 
		(altDirection.x() < 0.f ? 4 : 0) +
		(altDirection.y() < 0.f ? 2 : 0) +
		(altDirection.z() < 0.f ? 1 : 0);

	std::vector<TraversalPath> paths;

	for (const auto& tree : trees_)
	{
		const Eigen::Vector3f treePosition = Eigen::Vector3f(
			float(tree.rootOffset.x()),
			float(tree.rootOffset.y()),
			float(tree.rootOffset.z()));
		const Eigen::Vector3f rayPosition = position - treePosition;
		// TODO: Test if the ray intersects the tree's bbox.
		
		// Tracks the level that we are in during the traversal.
		uint32_t level = 0;
		// Tracks the path to take to get to the node currently being examined. At the end this will contain the path to the voxel
		// that has been first hit by the ray, if there is such a voxel in the tree.
		TraversalPath path{0, 0, 0};
		// The stack keeps track of all the nodes on the path to the current one so that the information does not need to be computed
		// again when ascending from a subtree where we did not find a hit.
		NodeInfo stack[HTConstants::MAX_LEVEL_COUNT];
		// A leaf is two levels therefore we need to store it in the first of those levels to be able to use it in the second one.
		uint64_t cachedLeaf;

		// Initialize the node info stack.
		stack[level].nodePtr = tree.rootNode;
		stack[level].childMask = GetNodeChildMask(stack[level].nodePtr);
		stack[level].visitMask = stack[level].childMask & ComputeChildIntersectionMask(level, path, rayPosition, altDirection,
			invDirection, 1 /*TODO: the scale will depend on the size of the voxels*/, true);

		// Traversal.
		while (true)
		{
			uint32_t formerLevel = level;
			// Ascend when there are no children to be visited.
			while (level > 0 && !stack[level].visitMask)
			{
				--level;
			}

			// Handle the case where we there are no voxels intersected by the ray inside this tree.
			if (level == 0 && !stack[level].visitMask)
			{
				// Empty path signifies that we found no intersections.
				path = TraversalPath{};
				break;
			}

			path.Ascend(formerLevel - level);

			// The following procedure uses XOR to permutate the range 0-7 depending on the ray direction.
			// Depending on where the ray is coming from the first four children will be the left/right (X),
			// the first two of both of the four will be the bottom/top (Y), and finally each of the four pairs
			// will have the back/forward (Z) child first.
			// All of these flips are the result of the XOR in the for loop and those are the permutations that
			// we get for each rayChildOrder value.
			// rayChildOrder == 0 -> [0, 1, 2, 3, 4, 5, 6, 7]
			// rayChildOrder == 1 -> [1, 0, 3, 2, 5, 4, 7, 6]
			// rayChildOrder == 2 -> [2, 3, 0, 1, 6, 7, 4, 5]
			// rayChildOrder == 3 -> [3, 2, 1, 0, 7, 6, 5, 4]
			// rayChildOrder == 4 -> [4, 5, 6, 7, 0, 1, 2, 3]
			// rayChildOrder == 5 -> [5, 4, 7, 6, 1, 0, 3, 2]
			// rayChildOrder == 6 -> [6, 7, 4, 5, 2, 3, 0, 1]
			// rayChildOrder == 7 -> [7, 6, 5, 4, 3, 2, 1, 0]
			uint8_t nextChild = 255;
			for (uint8_t child = 0; child < 8; ++child)
			{
				uint8_t childInOrder = child ^ rayChildOrder;
				if (stack[level].visitMask & (1u << childInOrder))
				{
					nextChild = childInOrder;
					break;
				}
			}
			assert(nextChild != 255);
			
			// We mark the currently picked child as solved.
			stack[level].visitMask &= ~(1u << nextChild);

			// We descend into the selected child.
			path.Descend(nextChild);
			++level;

			if (level == HTConstants::MAX_LEVEL_COUNT)
			{
				// We have intersected a voxel and we have the path to it.
				break;
			}

			if (level < HTConstants::LEAF_LEVEL)
			{
				// We are in an internal node.
				stack[level].nodePtr = GetChildNode(stack[level - 1].nodePtr, nextChild, stack[level - 1].childMask);
				stack[level].childMask = GetNodeChildMask(stack[level].nodePtr);
				stack[level].visitMask = stack[level].childMask & ComputeChildIntersectionMask(level, path, rayPosition, altDirection,
					invDirection, 1 /*TODO: the scale will depend on the size of the voxels*/);
			}
			else
			{
				uint8_t childMask;

				if (level == HTConstants::LEAF_LEVEL)
				{
					HashTable::vptr_t leafPtr = GetChildNode(stack[level - 1].nodePtr, nextChild, stack[level - 1].childMask);
					cachedLeaf = GetLeaf(leafPtr);
					childMask = GetFirstLeafMask(cachedLeaf);
				}
				else
				{
					childMask = GetSecondLeafMask(cachedLeaf, nextChild);
				}

				stack[level].childMask = childMask;
				stack[level].visitMask = stack[level].childMask & ComputeChildIntersectionMask(level, path, rayPosition, altDirection,
					invDirection, 1 /*TODO: the scale will depend on the size of the voxels*/);
			}
		}

		paths.push_back(path);
	}

	for (const auto& path : paths)
	{
		if (!path.IsNull())
		{
			return true;
		}
	}

	return false;
}

bool HashDAG::Traverse(const Eigen::Vector3i& voxel, uint32_t node, uint32_t level, const BoundingBox& bbox) const
{
	auto nodePtr = ht_.Translate(node);

	if (level == HTConstants::LEAF_LEVEL)
	{
		assert(bbox.span.x() == 4 && bbox.span.y() == 4 && bbox.span.z() == 4);

		auto maskOffset = voxel - bbox.pos;

		int bitIndex = maskOffset.z() + maskOffset.y() * 4 + maskOffset.x() * 16;

		uint64_t leaf = *((uint64_t*)nodePtr);

		return leaf & (1ull << bitIndex);
	}

	BoundingBox bboxChildren[8];
	BoundingBox::SplitCube(bbox, bboxChildren);

	uint32_t childMask = *nodePtr;

	uint32_t childNumber = 0;
	for (int b = 0; b < 8; ++b)
	{
		if ((1 << b) & childMask)
		{
			++childNumber;
			if (BoundingBox::IsPointInCube(bboxChildren[b], voxel))
			{
				return Traverse(voxel, nodePtr[childNumber], level + 1, bboxChildren[b]);
			}
		}
	}

	return false;
}

uint8_t HashDAG::GetNodeChildMask(HashTable::vptr_t node) const
{
	return (*ht_.Translate(node)) & 0xFFu;
}

uint64_t HashDAG::GetLeaf(HashTable::vptr_t node) const
{
	return *(uint64_t*)ht_.Translate(node);
}

HashTable::vptr_t HashDAG::GetChildNode(HashTable::vptr_t node, uint8_t child, uint8_t childMask) const
{
	assert(GetNodeChildMask(node) == childMask);
	assert(childMask & (1u << child));
	// Right side of the &: Get all of the bits on the right of the bit of the current child.
	// ANDing this with the 'childMask' and counting the set bits gets the number of 32-bit child entries
	// before the one we want.
	uint32_t offset = __popcnt(childMask & ((1u << child) - 1u)) + 1;
	return *(ht_.Translate(node) + offset);
}

uint8_t HashDAG::ComputeChildIntersectionMask(uint32_t level, const TraversalPath& path, const Eigen::Vector3f& rayOrigin,
	const Eigen::Vector3f& rayDirection, const Eigen::Vector3f& rayDirectionInverted, float treeScale, bool isRoot) const
{
	const uint32_t levelRank = HTConstants::MAX_LEVEL_COUNT - level;

	// Figuring out the node center.
	const float nodeRadius = float(1u << (levelRank - 1));
	const Eigen::Vector3f nodeCenter = (Eigen::Vector3f(nodeRadius, nodeRadius, nodeRadius) +
		path.AsPosition(levelRank)) * treeScale;

	// A vector connecting the ray origin to the center of the node
	const Eigen::Vector3f rayToNodeCenter = nodeCenter - rayOrigin;

	// Each component is the t in rayOrigin + t * rayDirection that intersects an axis-aligned plane that goes through
	// the node's center.
	Eigen::Vector3f tMid = { 
		rayToNodeCenter.x() * rayDirectionInverted.x(), 
		rayToNodeCenter.y() * rayDirectionInverted.y(),
		rayToNodeCenter.z() * rayDirectionInverted.z() };

	tMid.x() = isinf(tMid.x()) ? (tMid.x() > 0 ? FLT_MAX : -FLT_MAX) : tMid.x();
	tMid.y() = isinf(tMid.y()) ? (tMid.y() > 0 ? FLT_MAX : -FLT_MAX) : tMid.y();
	tMid.z() = isinf(tMid.z()) ? (tMid.z() > 0 ? FLT_MAX : -FLT_MAX) : tMid.z();

	// Defining max and min functions with extra handling for nans.
	auto MyMax = [] (float a, float b)
	{
		return a > b ? a : (isnan(b) ? a : b);
	};

	auto MyMin = [](float a, float b)
	{
		return a < b ? a : (isnan(b) ? a : b);
	};

	auto MyMaxCoeff = [&](const Eigen::Vector3f& v)
	{
		return MyMax(v.x(), MyMax(v.y(), v.z()));
	};

	auto MyMinCoeff = [&](const Eigen::Vector3f& v)
	{
		return MyMin(v.x(), MyMin(v.y(), v.z()));
	};

	float tMin, tMax;
	{
		const Eigen::Vector3f slabRadius = nodeRadius * 
			Eigen::Vector3f(fabs(rayDirectionInverted.x()), fabs(rayDirectionInverted.y()), fabs(rayDirectionInverted.z()));


		const Eigen::Vector3f tMinV = tMid - slabRadius;
		tMin = MyMax(MyMaxCoeff(tMinV), .0f);

		const Eigen::Vector3f tMaxV = tMid + slabRadius;
		tMax = MyMinCoeff(tMaxV);
	}

	// TODO: In the original code, this is where there is a check for the hit of the root bbox itself.
	// It might be better to check that at the start of each tree though.
	if (isRoot && (tMin >= tMax))
	{
		return 0;
	}

	// Each if here corresponds to the ray intersecting through an axis aligned plane inside the box, where
	// the plane goes through the node's center. For robustness we also allow an epsilon deviation.
	uint8_t intersectionMask = 0;
	const float epsilon = 1e-4f;

	if (tMin <= tMid.x() && tMid.x() <= tMax)
	{
		const Eigen::Vector3f pointOnRay = tMid.x() * rayDirection;

		uint8_t Y = 0;
		if (pointOnRay.y() >= rayToNodeCenter.y() - epsilon)
		{
			Y |= 0xCC;
		}
		if (pointOnRay.y() <= rayToNodeCenter.y() + epsilon)
		{
			Y |= 0x33;
		}

		uint8_t Z = 0;
		if (pointOnRay.z() >= rayToNodeCenter.z() - epsilon)
		{
			Z |= 0xAA;
		}
		if (pointOnRay.z() <= rayToNodeCenter.z() + epsilon)
		{
			Z |= 0x55;
		}

		intersectionMask |= Y & Z;
	}

	if (tMin <= tMid.y() && tMid.y() <= tMax)
	{
		const Eigen::Vector3f pointOnRay = tMid.y() * rayDirection;

		uint8_t X = 0;
		if (pointOnRay.x() >= rayToNodeCenter.x() - epsilon)
		{
			X |= 0xF0;
		}
		if (pointOnRay.x() <= rayToNodeCenter.x() + epsilon)
		{
			X |= 0x0F;
		}

		uint8_t Z = 0;
		if (pointOnRay.z() >= rayToNodeCenter.z() - epsilon)
		{
			Z |= 0xAA;
		}
		if (pointOnRay.z() <= rayToNodeCenter.z() + epsilon)
		{
			Z |= 0x55;
		}

		intersectionMask |= X & Z;
	}

	if (tMin <= tMid.z() && tMid.z() <= tMax)
	{
		const Eigen::Vector3f pointOnRay = tMid.z() * rayDirection;

		uint8_t X = 0;
		if (pointOnRay.x() >= rayToNodeCenter.x() - epsilon)
		{
			X |= 0xF0;
		}
		if (pointOnRay.x() <= rayToNodeCenter.x() + epsilon)
		{
			X |= 0x0F;
		}

		uint8_t Y = 0;
		if (pointOnRay.y() >= rayToNodeCenter.y() - epsilon)
		{
			Y |= 0xCC;
		}
		if (pointOnRay.y() <= rayToNodeCenter.y() + epsilon)
		{
			Y |= 0x33;
		}

		intersectionMask |= X & Y;
	}


	// In case we don't intersect any planes from the ifs, we must have intersected a single node (we
	// assume we hit the node's bbox so at least one child must get intersected.
	if (!intersectionMask)
	{
		const Eigen::Vector3f pointOnRay = (0.5f * (tMin + tMax)) * rayDirection;

		const uint8_t firstChild =
			((pointOnRay.x() >= rayToNodeCenter.x()) ? 4 : 0) +
			((pointOnRay.y() >= rayToNodeCenter.y()) ? 2 : 0) +
			((pointOnRay.z() >= rayToNodeCenter.z()) ? 1 : 0);

		return (1u << firstChild);
	}
	else
	{
		return intersectionMask;
	}
}

uint8_t HashDAG::GetFirstLeafMask(uint64_t leaf) const
{
	return uint8_t(
		((leaf & 0x00000000000000FF) == 0 ? 0 : 1 << 0) |
		((leaf & 0x000000000000FF00) == 0 ? 0 : 1 << 1) |
		((leaf & 0x0000000000FF0000) == 0 ? 0 : 1 << 2) |
		((leaf & 0x00000000FF000000) == 0 ? 0 : 1 << 3) |
		((leaf & 0x000000FF00000000) == 0 ? 0 : 1 << 4) |
		((leaf & 0x0000FF0000000000) == 0 ? 0 : 1 << 5) |
		((leaf & 0x00FF000000000000) == 0 ? 0 : 1 << 6) |
		((leaf & 0xFF00000000000000) == 0 ? 0 : 1 << 7));
}

uint8_t HashDAG::GetSecondLeafMask(uint64_t leaf, uint8_t firstMask) const
{
	const uint8_t shift = uint8_t(firstMask * 8);
	return uint8_t(leaf >> shift);
}

void HashDAG::UploadToGPU(const VulkanFactory::Device::DeviceInfo& deviceInfo, VkCommandPool commandPool,
	VkQueue queue, HashDAGGPUInfo& uploadInfo)
{
	ht_.UploadToGPU(deviceInfo, commandPool, queue, uploadInfo);

	VkDeviceSize treesBufferSize = (uint32_t)trees_.size() * sizeof(HashTree);

	// TODO: Check storage buffer construction against Sascha Willems' examples.
	VulkanFactory::Buffer::Create("Voxel Trees Storage Buffer", deviceInfo,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		treesBufferSize, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uploadInfo.TreeRootsStorageBuffer);

	VulkanFactory::Buffer::BufferInfo treesStagingBufferInfo;
	VulkanFactory::Buffer::Create("Voxel Trees Staging Buffer", deviceInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, treesBufferSize,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, treesStagingBufferInfo);

	VulkanUtils::Buffer::Copy(deviceInfo.Handle, treesStagingBufferInfo.Memory, treesBufferSize, trees_.data());
	VulkanUtils::Buffer::Copy(deviceInfo.Handle, treesStagingBufferInfo.DescriptorBufferInfo.buffer,
		uploadInfo.TreeRootsStorageBuffer.DescriptorBufferInfo.buffer, treesBufferSize, commandPool, queue);

	uploadInfo.TreeCount = (uint32_t)trees_.size();

	VulkanFactory::Buffer::Destroy(deviceInfo, treesStagingBufferInfo);
}

TraversalPath::TraversalPath()
	: x_(UINT32_MAX), y_(UINT32_MAX), z_(UINT32_MAX) {}

TraversalPath::TraversalPath(uint32_t x, uint32_t y, uint32_t z)
	: x_(x), y_(y), z_(z) {}

void TraversalPath::Ascend(uint32_t levels)
{
	x_ >>= levels;
	y_ >>= levels;
	z_ >>= levels;
}

void TraversalPath::Descend(uint8_t child)
{
	x_ <<= 1;
	y_ <<= 1;
	z_ <<= 1;
	x_ |= (child & 0x4u) >> 2;
	y_ |= (child & 0x2u) >> 1;
	z_ |= (child & 0x1u) >> 0;
}

Eigen::Vector3f TraversalPath::AsPosition(uint32_t levelRank) const
{
	return Eigen::Vector3f(float(x_ << levelRank), float(y_ << levelRank), float(z_ << levelRank));
}

bool TraversalPath::IsNull() const
{
	return x_ == UINT32_MAX && y_ == UINT32_MAX && z_ == UINT32_MAX;
}
