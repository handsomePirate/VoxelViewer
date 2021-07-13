#include "Converter.hpp"
#include "Core/Logger/Logger.hpp"

#include <openvdb/openvdb.h>

int GetTreePageRequirement()
{
	int runningPhysicalPageCount = 0;
	for (int i = 0; i <= HTConstants::LEAF_LEVEL; ++i)
	{
		int nodeCount = (1 << i) * (1 << i);
		if (i < HTConstants::LEAF_LEVEL)
		{
			// A full node size is child mask & 8 child pointers(most nodes will be smaller though).
			constexpr int maxNodeSize = 9 * 32;
			// Every started page needs to be available.
			int physicalPageCount = (nodeCount * maxNodeSize + HTConstants::PAGE_SIZE - 1) / HTConstants::PAGE_SIZE;
			runningPhysicalPageCount += physicalPageCount;
		}
		else
		{
			// Each leaf is only two entries.
			constexpr int maxNodeSize = 64;
			// Every started page needs to be available.
			int physicalPageCount = (nodeCount * maxNodeSize + HTConstants::PAGE_SIZE - 1) / HTConstants::PAGE_SIZE;
			runningPhysicalPageCount += physicalPageCount;
		}
	}
	return runningPhysicalPageCount;
}

void Converter::OpenVDBToDAG(openvdb::SharedPtr<openvdb::Vec3SGrid> grid, HashDAG& hd)
{
	auto iter = grid->tree().beginRootChildren();

	std::vector<openvdb::Vec3SGrid::TreeType::RootNodeType::ChildNodeType*> roots;

	for (; iter; ++iter)
	{
		roots.push_back(iter.mIter->second.child);
	}

	if (roots.empty())
	{
		CoreLogError("Empty tree! Something went wrong.");
		return;
	}

	int treeCount = roots.size();
	// Initialize the HashDAG according to the height (and other parameters).
	const int requiredPagesPerTree = GetTreePageRequirement();
	hd.Init(/*requiredPagesPerTree * treeCount*/ 2621448);

	// Create an axis aligned box that represents the recursion into the tree.
	AxisAlignedCubeI trackingCube;
	trackingCube.pos = { 0, 0, 0 };
	trackingCube.span = { 32, 32, 32 };

	auto bbox = grid->evalActiveVoxelBoundingBox();
	InternalBoundingBox hdBoundingBox;
	auto bboxStart = bbox.getStart();
	auto bboxEnd = bbox.getEnd();
	auto bboxSpan = bboxEnd - bboxStart;
	hdBoundingBox.pos = { bboxStart.x(), bboxStart.y(), bboxStart.z() };
	hdBoundingBox.span = { bboxSpan.x(), bboxSpan.y(), bboxSpan.z() };
	hd.SetBoundingBox(hdBoundingBox);

	for (int i = 0; i < roots.size(); ++i)
	{
		uint64_t voxelIndex = 0;
		Color& colorArray = hd.AddColorArray(roots[i]->onVoxelCount());
		// TODO: Get the position of the L1 node so that the Hash tree is able to retain that information.
#ifdef DEBUG_CONVERTER
		AxisAlignedCubeI treebbox;
		uint32_t rootPtr = ConstructHashDAG(trackingCube, roots[i], hd, voxelIndex, 0, false, 1, treebbox);
#else
		uint32_t rootPtr = ConstructHashDAG(trackingCube, roots[i], hd, colorArray, voxelIndex, 0, false, 1);
#endif
		assert(rootPtr != HTConstants::INVALID_POINTER);
		assert(voxelIndex == roots[i]->onVoxelCount());
		auto bbox = roots[i]->getNodeBoundingBox();
		hd.AddRoot(rootPtr, { bbox.getStart().x(), bbox.getStart().y(), bbox.getStart().z() });
	}
}

#ifdef DEBUG_CONVERTER
uint32_t Converter::ConstructHashDAG(const AxisAlignedCubeI& openvdbTrackingCube,
	void* nodeIn, HashDAG& hd, Color& hdColors, uint64_t& voxelIndex,
	int level, bool full, int depth, const AxisAlignedCubeI& treebbox)
#else
uint32_t Converter::ConstructHashDAG(const AxisAlignedCubeI& openvdbTrackingCube,
	void* nodeIn, HashDAG& hd, Color& hdColors, uint64_t& voxelIndex,
	int level, bool full, int depth)
#endif
{
	using L1NodeType = openvdb::Vec3SGrid::TreeType::RootNodeType::ChildNodeType;
	using L2NodeType = openvdb::Vec3SGrid::TreeType::RootNodeType::ChildNodeType::ChildNodeType;

	switch (depth)
	{
	case 0:
	{
		CoreLogError("Looking for openvdb root in recursion. This should not happen.");
		break;
	}
	case 1:
	{
		L1NodeType* node = (L1NodeType*)nodeIn;
		if (node)
		{
#ifdef DEBUG_CONVERTER
			AxisAlignedCubeI newTreebbox = treebbox;
			auto bbox = node->getNodeBoundingBox();
			if (openvdbTrackingCube.span.x() == 32)
			{
				auto start = bbox.getStart();
				newTreebbox.pos = { start.x(), start.y(), start.z() };
				newTreebbox.span = { 4096, 4096, 4096 };
				std::stringstream ss;
				ss << bbox << " |X| " << newTreebbox << std::endl;
				LogDebug("l1 node: %s", ss.str().c_str());
			}
#endif

#ifdef DEBUG_CONVERTER
			return HandleOpenvdbLevel<L1NodeType, 32>(openvdbTrackingCube, hd, hdColors, voxelIndex, level, full, node, depth, newTreebbox);
#else
			return HandleOpenvdbLevel<L1NodeType, 32>(openvdbTrackingCube, hd, hdColors, voxelIndex, level, full, node, depth);
#endif
		}
		else
		{
			CoreLogError("Found empty l1 node in recursion. This should not happen.");
		}
		break;
	}
	case 2:
	{
		L2NodeType* node = (L2NodeType*)nodeIn;
		if (node)
		{
#ifdef DEBUG_CONVERTER
			auto bbox = node->getNodeBoundingBox();
			if (openvdbTrackingCube.span.x() == 16)
			{
				std::stringstream ss;
				ss << bbox << " |X| " << treebbox;
				LogDebug("l2 node: %s", ss.str().c_str());
			}

			return HandleOpenvdbLevel<L2NodeType, 16>(openvdbTrackingCube, hd, hdColors, voxelIndex, level, full, node, depth, treebbox);
#else
			return HandleOpenvdbLevel<L2NodeType, 16>(openvdbTrackingCube, hd, hdColors, voxelIndex, level, full, node, depth);
#endif
		}
		else
		{
			CoreLogError("Found empty l2 node in recursion. This should not happen.");
		}
		break;
	}
	case 3:
	{
		// TODO: Add colors.
		openvdb::Vec3SGrid::TreeType::LeafNodeType* leaf = (openvdb::Vec3SGrid::TreeType::LeafNodeType*)nodeIn;

		if (leaf)
		{
			std::vector<openvdb::Vec3s> leafColors;
			leafColors.resize(512);
			int index = 0;
			for (auto colorIt = leaf->beginValueAll(); colorIt != leaf->endValueAll(); ++colorIt)
			{
				//CoreLogInfo("%i: (%f, %f, %f)", index, colorIt->x(), colorIt->y(), colorIt->z());
				leafColors[index++] = *colorIt;
			}

#ifdef DEBUG_CONVERTER
			auto bbox = leaf->getNodeBoundingBox();
			std::stringstream ss;
			ss << bbox << " |X| " << treebbox;
			CoreLogInfo("leaf: %s", ss.str().c_str());
#endif
			std::vector<uint32_t> node;
			node.push_back(0);
			uint64_t parentIndex = voxelIndex;
			for (int i = 0; i < 8; ++i)
			{
				uint64_t leafMask = 0;
				uint64_t result = 0;
				if (full)
				{
					leafMask = 0xFFFFFFFFFFFFFFFF;
				}
				else
				{
					int maskWordStart;
					if (i < 4)
					{
						// Interested in the first half of the mask words.
						maskWordStart = 0;
					}
					else
					{
						// Interested in the second half of the mask words.
						maskWordStart = 32;
					}

					bool firstFour;
					if (i == 0 || i == 1 || i == 4 || i == 5)
					{
						// Interested in the first half of the mask word.
						firstFour = true;
					}
					else
					{
						// Interested in the second half of the mask word.
						firstFour = false;
					}

					bool firstHalf;
					if (i % 2 == 0)
					{
						// Interested in the first half of each byte.
						firstHalf = true;
					}
					else
					{
						// Interested in the second half of each byte.
						firstHalf = false;
					}

					int byteShift = -1;
					for (int maskByteid = maskWordStart; maskByteid < maskWordStart + 32; ++maskByteid)
					{
						uint8_t maskByte = leaf->getValueMask().getWord<uint8_t>(maskByteid);

						bool useByte = false;
						if ((firstFour && (maskByteid / 4) % 2 == 0) || (!firstFour && (maskByteid / 4) % 2 == 1))
						{
							useByte = true;
							++byteShift;
						}

						if (useByte)
						{
							if (firstHalf)
							{
								maskByte &= 0x0F;
							}
							else
							{
								maskByte &= 0xF0;
								maskByte >>= 4;
							}
							result |= ((uint64_t)maskByte) << (byteShift * 4);
						}
					}
					assert(byteShift == 15);
					const int8_t shiftsRight[7] = { 14, 12, 8, 6, 4, 2, 0 };
					const int8_t shiftsLeft[6] = { 2, 4, 6, 8, 12, 14 };
					const uint64_t masks[13] =
					{
						0x0030000000300000,
						0x0003000000030000,
						0x00C0000000C00000,
						0x300C0000300C0000,
						0x0300000003000000,
						0x0000003000000030,
						0xC0000003C0000003,
						0x0C0000000C000000,
						0x000000C0000000C0,
						0x0000300C0000300C,
						0x0000030000000300,
						0x0000C0000000C000,
						0x00000C0000000C00
					};
					for (int i = 0; i < 7; ++i)
					{
						leafMask |= (result & masks[i]) >> shiftsRight[i];
					}
					for (int i = 0; i < 6; ++i)
					{
						leafMask |= (result & masks[i + 7]) << shiftsLeft[i];
					}
					//leafMask = result;
				}
				if (leafMask != 0)
				{
					node[0] |= 1 << i;
					node.push_back(hd.FindOrAddLeaf(leafMask));
					node.push_back(uint32_t(voxelIndex - parentIndex));

					static uint32_t zOffset[8] =
					{
						0, 1, 0, 1, 0, 1, 0, 1
					};
					static uint32_t yOffset[8] =
					{
						0, 0, 1, 1, 0, 0, 1, 1
					};
					static uint32_t xOffset[8] =
					{
						0, 0, 0, 0, 1, 1, 1, 1
					};

					uint64_t voxelSum = 0;
					for (uint32_t leafPart = 0; leafPart < 8; ++leafPart)
					{
						for (uint32_t xLeaf = 0; xLeaf < 2; ++xLeaf)
						{
							for (uint32_t yLeaf = 0; yLeaf < 2; ++yLeaf)
							{
								for (uint32_t zLeaf = 0; zLeaf < 2; ++zLeaf)
								{
									const uint64_t leafBit = 1ull << (
										zLeaf + zOffset[leafPart] * 2 +
										4 * (yLeaf + yOffset[leafPart] * 2) +
										16 * (xLeaf + xOffset[leafPart] * 2));

									if (leafBit & result)
									{
										const uint32_t allValueIndex =
											zLeaf + zOffset[i] * 4 + zOffset[leafPart] * 2 +
											8 * (yLeaf + yOffset[i] * 4 + yOffset[leafPart] * 2) +
											64 * (xLeaf + xOffset[i] * 4 + xOffset[leafPart] * 2);

										uint64_t colorVoxelIndex = voxelIndex + voxelSum++;
										hdColors.Set(colorVoxelIndex, leafColors[allValueIndex]);
									}
								}
							}
						}
					}

					assert(voxelSum == __popcnt64(leafMask));
					voxelIndex += voxelSum;
				}
			}

			//if ((bbox.getStart().x() == -104 && bbox.getStart().y() == 32 && bbox.getStart().z() == -40) ||
			//	(bbox.getStart().x() == -104 && bbox.getStart().y() == 32 && bbox.getStart().z() == -48))
			//{
			//	__debugbreak();
			//}

			uint32_t nodePtr = hd.FindOrAddNode(level, node.size(), node.data());
			return nodePtr;
		}
		else
		{
			CoreLogError("Found empty leaf in recursion. This should not happen.");
		}
		break;
	}
	}

	CoreLogError("While traversing the openvdb tree, the code ran into unexpected depth. The resulting HashDAG will not be correct.");
	return HTConstants::INVALID_POINTER;
}
