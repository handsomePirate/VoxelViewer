#pragma once
#include "Core/Common.hpp"
#include "BoundingBox.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include "Vulkan/Camera.hpp"
#include "Color.hpp"
#include <Eigen/Dense>
#include <ostream>

struct HTConstants
{
	static constexpr uint32_t PAGE_SIZE = 512;
	// TODO: Probably should not be constant (should be computed from the actual input voxel grid - as its
	// size is not going to change). This change involves also removing other related values from constants.
	static constexpr uint32_t MAX_LEVEL_COUNT = 12; // To fit 4096^3
	static constexpr uint32_t TREE_SPAN = 4096; // NOTE: Needs to be changed with the tree size (if ever modified).
	static constexpr uint32_t LEAF_LEVEL = MAX_LEVEL_COUNT - 2;
	// TODO: Potentially change these.
	// in pages
	static constexpr uint32_t TOP_LEVEL_BUCKET_SIZE = 1024;
	static constexpr uint32_t BOTTOM_LEVEL_BUCKET_SIZE = 4096;
	static constexpr uint32_t TOP_LEVEL_BUCKET_COUNT = 1024;
	static constexpr uint32_t BOTTOM_LEVEL_BUCKET_COUNT = 65536;
	static constexpr uint32_t TOP_LEVEL_RANK = 6;
	static constexpr uint32_t BOTTOM_LEVEL_RANK = MAX_LEVEL_COUNT - TOP_LEVEL_RANK;
	static constexpr uint32_t TOTAL_TOP_BUCKET_COUNT = TOP_LEVEL_RANK * TOP_LEVEL_BUCKET_COUNT;
	static constexpr uint32_t TOTAL_BOTTOM_BUCKET_COUNT = BOTTOM_LEVEL_RANK * BOTTOM_LEVEL_BUCKET_COUNT;
	static constexpr uint32_t TOTAL_BUCKET_COUNT = TOTAL_TOP_BUCKET_COUNT + TOTAL_BOTTOM_BUCKET_COUNT;
	static constexpr uint32_t TOTAL_PAGE_COUNT =
		(TOTAL_TOP_BUCKET_COUNT * TOP_LEVEL_BUCKET_SIZE +
			TOTAL_BOTTOM_BUCKET_COUNT * BOTTOM_LEVEL_BUCKET_SIZE)
		/ PAGE_SIZE;
	static constexpr uint32_t INVALID_POINTER = 0xFFFFFFFF;
};

struct HTStats
{
	uint32_t memoryAllocatedBytes;
	uint32_t emptyBuckets;
	float emptyToTotalRatio;
	float emptyTopToTotalRatio;
	float emptyBottomToTotalRatio;
	float avgTopBucketFullness;
	float avgBottomBucketFullness;
	float percentageOfMemoryUsed;
	uint32_t levelNodeCount[HTConstants::LEAF_LEVEL + 1];

	void Print(std::ostream& os) const;
};

// TODO: We are only interested in color modifications, the infrastructure could be simplified when colors are in place.

struct HashDAGPushConstants
{
	uint32_t MaxLevels = HTConstants::MAX_LEVEL_COUNT;
	uint32_t LeafLevel = HTConstants::LEAF_LEVEL;
	uint32_t PageSize = HTConstants::PAGE_SIZE;
	uint32_t PageCount;
	uint32_t TreeCount;
};

struct HashDAGGPUInfo
{
	VulkanFactory::Buffer::BufferInfo PagesStorageBuffer;
	VulkanFactory::Buffer::BufferInfo PageTableStorageBuffer;
	VulkanFactory::Buffer::BufferInfo TreeRootsStorageBuffer;
	VulkanFactory::Buffer::BufferInfo SortedTreesStorageBuffer;
	uint32_t PageCount;
	uint32_t TreeCount;
};

class HashTable
{
public:
	/// Physical Pointer into the hash table.
	typedef uint32_t* ptr_t;
	/// Virtual memory index that can be translated into the physical pointer.
	typedef uint32_t vptr_t;
	/// Global bucket number that the 'bucketsSizes' can be indexed by.
	typedef uint32_t bucket_t;

	HashTable() = default;
	~HashTable();
	/// Pre-allocates the memory necessary for operating this HashDAG.
	void Init(uint32_t pagePoolSize);
	void Destroy();

	/// Returns a physical memory pointer to the node represented by the virtual pointer.
	ptr_t Translate(vptr_t ptr) const;

	// TODO: Check if this can be private.
	/// Remembers given leaf node under given hash.
	vptr_t AddLeaf(const uint64_t leaf, const uint32_t hash);
	/// Remembers given non-leaf node under given hash.
	vptr_t AddNode(const uint32_t level, const uint32_t nodeSize, const uint32_t* node, const uint32_t hash);

	// TODO: Check if this can be private.
	/// Finds this leaf node in the given bucket and returns a virtual pointer to it.
	vptr_t FindLeafInBucket(bucket_t bucket, uint64_t leaf) const;
	/// Finds this non-leaf node in the given bucket and returns a virtual pointer to it.
	vptr_t FindNodeInBucket(bucket_t bucket, uint32_t nodeSize, uint32_t* const node) const;

	/// Finds this leaf node in the structure or creates a new entry and returns a virtual pointer to it.
	vptr_t FindOrAddLeaf(uint64_t leaf);
	/// Finds this non-leaf node in the structure or creates a new entry and returns a virtual pointer to it.
	vptr_t FindOrAddNode(uint32_t level, uint32_t nodeSize, uint32_t* const node);

	/// Returns the number of MB that is allocated by this class.
	uint32_t GetMemAllocatedBytes() const;
	/// How many nodes there is in the level.
	uint32_t CountLevelNodes(uint32_t level) const;
	// TODO: Add void AddFullNode(uint32_t) function which lets us populate the hash dag if we know the nodes in advance.

	/// Returns some useful statistics.
	HTStats GetStats() const;

	void UploadToGPU(const VulkanFactory::Device::DeviceInfo& deviceInfo, VkCommandPool commandPool,
		VkQueue queue, HashDAGGPUInfo& uploadInfo);

private:
	/// Allocates a page from the pre-allocated page pool.
	void AllocatePage(uint32_t page);
	/// Check if the page has already been allocated from the page pool.
	bool IsPageAllocated(uint32_t page);

	/// Creates a 32-bit hash for the leaf node.
	static uint32_t HashLeaf(uint64_t leaf);
	/// Creates a 32-bit hash for the non-leaf node.
	static uint32_t HashNode(const uint32_t nodeSize, const uint32_t* node);

	/// Finds a bucket that corresponds to the given hash in the given level.
	static bucket_t GetBucket(uint32_t level, uint32_t hash);
	/// Retrieves a virtual pointer to the bucket.
	static uint32_t GetBucketPtr(bucket_t bucket);

	/// Determines the size of a non-leaf node.
	static uint32_t GetNodeSize(uint32_t* const ptr);

	/// The top of the already allocated pool pages. Starts at 1, 0 is reserved of un-allocated entries.
	uint32_t poolTop_ = 1;
	uint32_t poolSize_;
	/// Physical page memory with 32 bit pointers.
	uint32_t* pagePool_;
	/// The virtual page table (virtual -> physical address). The physical address is an index of the pagePool.
	uint32_t* pageTable_;
	/// The array holding the sizes of all buckets present in the hash table.
	uint32_t* bucketsSizes_;
};

struct HashTree
{
	Eigen::Vector3i rootOffset;
	HashTable::vptr_t rootNode = HTConstants::INVALID_POINTER;
};

/// This class determines the path that a traversal algorithm should take to get to a certain voxel (used in tracing).
/// Starting from the HTConstants::MAX_LEVEL_COUNT-th bit from the right for each component going to the right-most bit,
/// we are presented with the children we should pick for our descent.
/// In the ray intersection algorithm itself, the right-most bit represents the current tree level (not necessarily the
/// last one).
class TraversalPath
{
public:
	TraversalPath();
	TraversalPath(uint32_t x, uint32_t y, uint32_t z);

	/// Ascends in the tree path by the specified number of levels.
	void Ascend(uint32_t levels = 1);
	/// Descends one level into the child specified by the argument.
	void Descend(uint8_t child);

	uint8_t ChildAtLevel(uint32_t level);

	Eigen::Vector3f AsPosition(uint32_t levelRank) const;

	bool IsNull() const;
private:
	/// x component of the tree path.
	uint32_t x_;
	/// y component of the tree path.
	uint32_t y_;
	/// z component of the tree path.
	uint32_t z_;
};

/// Node information holding structure that allows for caching already computed nodes and reusing the data
/// when ascending back up to them.
struct NodeInfo
{
	/// A virtual pointer to the node that is inside the
	HashTable::vptr_t nodePtr;
	/// The child mask of the node.
	uint8_t childMask;
	/// A mask of the children that are to be visited. This is computed based on the traversal ray.
	uint8_t visitMask;
	uint64_t voxelIndex;
};

class HashDAG
{
public:
	HashDAG() = default;
	~HashDAG() = default;

	void Init(uint32_t pagePoolSize);

	/// Finds this leaf node in the structure or creates a new entry and returns a virtual pointer to it.
	HashTable::vptr_t FindOrAddLeaf(uint64_t leaf);
	/// Finds this non-leaf node in the structure or creates a new entry and returns a virtual pointer to it.
	HashTable::vptr_t FindOrAddNode(uint32_t level, uint32_t nodeSize, uint32_t* const node);
	/// Makes a node the root of a tree and assigns it an ofset from the origin.
	void AddRoot(HashTable::vptr_t node, const Eigen::Vector3i& offset);

	Color& AddColorArray(int voxelCount);

	/// Querries the trees stored in this structure. Returns true if the voxels is on, returns false otherwise.
	bool IsActive(const Eigen::Vector3i& voxel) const;

	/// Simulates a ray traversal through the structure. Returns true, if the ray hits a voxel. If returns true,
	/// voxel will contain the position of the voxel that has been hit first on the path of the ray.
	bool CastRay(const Eigen::Vector3f& position, const Eigen::Vector3f& direction, Eigen::Vector3i& voxel,
		const Eigen::Vector3f& perturbationEpsilon = { 1e-5f, 1e-5f, 1e-5f }) const;

	void UploadToGPU(const VulkanFactory::Device::DeviceInfo& deviceInfo, VkCommandPool commandPool,
		VkQueue queue, HashDAGGPUInfo& uploadInfo, ColorGPUInfo& colorInfo, float colorCompressionMargin = 0.f);

	void UploadColorRangeToGPU(const VulkanFactory::Device::DeviceInfo& deviceInfo, VkCommandPool commandPool,
		VkQueue queue, ColorGPUInfo& colorInfo, uint32_t tree, uint64_t offset, uint64_t size, float colorCompressionMargin = 0.f);

	void SetBoundingBox(const BoundingBox& boundingBox);

	int Bottom() const;
	int Top() const;

	int Left() const;
	int Right() const;

	int Back() const;
	int Front() const;

	uint64_t ComputeVoxelIndex(uint32_t tree, uint32_t x, uint32_t y, uint32_t z) const;

	void SetVoxelColor(uint32_t tree, uint64_t voxelIndex, const openvdb::Vec3s& color);

	void SortAndUploadTreeIndices(VulkanFactory::Device::DeviceInfo& deviceInfo, VkCommandPool commandPool,
		VkQueue queue, const Eigen::Vector3f& cameraPosition, VulkanFactory::Buffer::BufferInfo& sortedTreesBuffer);

private:
	/// Recurses through the tree and finds out if the specified voxel is on or off (internal implementation of IsActive).
	bool Traverse(const Eigen::Vector3i& voxel, uint32_t node, uint32_t level, const BoundingBox& bbox) const;

	/// Returns the child mask of the specified node.
	uint8_t GetNodeChildMask(HashTable::vptr_t node) const;
	/// Returns the node as a leaf (using it anywhere besides the leaf level will return undefined numbers).
	uint64_t GetLeaf(HashTable::vptr_t node) const;
	/// Returns the child node of the specified node. Selected 'child' must be included in the child mask of the
	/// specified node. The 'childMask' argument is included to avoid having to query it from the hash table.
	HashTable::vptr_t GetChildNode(HashTable::vptr_t node, uint8_t child, uint8_t childMask) const;
	uint32_t GetChildOffset(HashTable::vptr_t node, uint8_t child, uint8_t childMask) const;
	/// Computes the mask of all children of a node that would be intersected by the ray.
	uint8_t ComputeChildIntersectionMask(uint32_t level, const TraversalPath& path,const Eigen::Vector3f& rayOrigin,
		const Eigen::Vector3f& rayDirection, const Eigen::Vector3f& rayDirectionInverted, float treeScale, bool isRoot = false) const;

	/// Separates the first layer of the child mask of the 4x4x4 leaf and returns it.
	uint8_t GetFirstLeafMask(uint64_t leaf) const;
	uint32_t GetFirstVoxelCount(uint64_t leaf, uint32_t nextChild) const;
	/// Separates the second layer of the child mask of the 4x4x4 leaf using the mask of the first layer and returns it.
	uint8_t GetSecondLeafMask(uint64_t leaf, uint8_t firstMask) const;
	uint32_t GetSecondVoxelCount(uint32_t mask, uint32_t nextChild) const;

	HashTable ht_;
	std::vector<HashTree> trees_;
	std::vector<std::unique_ptr<Color>> treeColorArrays_;
	BoundingBox boundingBox_;
};
