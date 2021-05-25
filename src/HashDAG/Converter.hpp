#pragma once
#include "Core/Common.hpp"
#include "HashDAG.hpp"

#include <openvdb/openvdb.h>
#include <Eigen/Dense>

static int RoundUpToPowerOf2(int val)
{
	--val;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	return ++val;
}

struct AxisAlignedCubeI
{
	/// The position of the box's corner that has the least coordinate value in all axes.
	Eigen::Vector3i pos;
	/// The extents of the cube.
	Eigen::Vector3i span;

	static bool CubesIntersect(const AxisAlignedCubeI& cube1, const AxisAlignedCubeI& cube2)
	{
		bool xIntersects = !(cube2.pos.x() + cube2.span.x() <= cube1.pos.x() || cube1.pos.x() + cube1.span.x() <= cube2.pos.x());
		bool yIntersects = !(cube2.pos.y() + cube2.span.y() <= cube1.pos.y() || cube1.pos.y() + cube1.span.y() <= cube2.pos.y());
		bool zIntersects = !(cube2.pos.z() + cube2.span.z() <= cube1.pos.z() || cube1.pos.z() + cube1.span.z() <= cube2.pos.z());
		return xIntersects && yIntersects && zIntersects;
	}

	static void SplitCube(const AxisAlignedCubeI& cube, AxisAlignedCubeI children[8])
	{
		Eigen::Vector3i newSpan = cube.span / 2;

		children[0].pos = { cube.pos.x(), cube.pos.y(), cube.pos.z() };
		children[0].span = newSpan;

		children[1].pos = { cube.pos.x(), cube.pos.y(), cube.pos.z() + newSpan.z() };
		children[1].span = newSpan;

		children[2].pos = { cube.pos.x(), cube.pos.y() + newSpan.y(), cube.pos.z() };
		children[2].span = newSpan;

		children[3].pos = { cube.pos.x(), cube.pos.y() + newSpan.y(), cube.pos.z() + newSpan.z() };
		children[3].span = newSpan;

		children[4].pos = { cube.pos.x() + newSpan.x(), cube.pos.y(), cube.pos.z() };
		children[4].span = newSpan;

		children[5].pos = { cube.pos.x() + newSpan.x(), cube.pos.y(), cube.pos.z() + newSpan.z() };
		children[5].span = newSpan;

		children[6].pos = { cube.pos.x() + newSpan.x(), cube.pos.y() + newSpan.y(), cube.pos.z() };
		children[6].span = newSpan;

		children[7].pos = { cube.pos.x() + newSpan.x(), cube.pos.y() + newSpan.y(), cube.pos.z() + newSpan.z() };
		children[7].span = newSpan;
	}

	friend std::ostream& operator<<(std::ostream& os, const AxisAlignedCubeI& cube)
	{
		os << '[' << cube.pos.x() << ", " << cube.pos.y() << ", " << cube.pos.z() << "] -> "
			<< '[' << cube.pos.x() + cube.span.x() - 1 << ", " << cube.pos.y() + cube.span.y() - 1 << 
			", " << cube.pos.z() + cube.span.z() - 1 << ']';
		return os;
	}
};

//#define DEBUG_CONVERTER

class Converter
{
public:
	static void OpenVDBToDAG(openvdb::SharedPtr<openvdb::Vec3SGrid> grid, HashDAG& hd);

private:
#ifdef DEBUG_CONVERTER
	static uint32_t ConstructHashDAG(const AxisAlignedCubeI& openvdbTrackingCube,
		void* nodeIn, HashDAG& hd,
		int level, bool full, int depth, const AxisAlignedCubeI& treebbox);
#else
	static uint32_t ConstructHashDAG(const AxisAlignedCubeI& openvdbTrackingCube,
		void* nodeIn, HashDAG& hd,
		int level, bool full, int depth);
#endif

#ifdef DEBUG_CONVERTER
	template <typename NodeType, int NODE_SIZE>
	static uint32_t HandleOpenvdbLevel(const AxisAlignedCubeI& openvdbTrackingCube,
		HashDAG& hd,
		int level, bool full, NodeType* node, int depth, const AxisAlignedCubeI& treebbox)
#else
	template <typename NodeType, int NODE_SIZE>
	static uint32_t HandleOpenvdbLevel(const AxisAlignedCubeI& openvdbTrackingCube,
		HashDAG& hd,
		int level, bool full, NodeType* node, int depth)
#endif
	{
		// Split the gridCube into children, recurse with the correct (according to the openvdb recursion rules) one.
		AxisAlignedCubeI trackingChildren[8];
		AxisAlignedCubeI::SplitCube(openvdbTrackingCube, trackingChildren);

#ifdef DEBUG_CONVERTER
		AxisAlignedCubeI treebboxChildren[8];
		AxisAlignedCubeI::SplitCube(treebbox, treebboxChildren);
#endif

		// When the cube has the correct extents, advance to the next gridIterator level before recursing.
		bool childOn[8];
		for (int i = 0; i < 8; ++i)
		{
			childOn[i] = true;
		}
		constexpr int NODE_SIZE_2 = NODE_SIZE * NODE_SIZE;
		if (openvdbTrackingCube.span.x() == 2 && !full)
		{
			int childCountOn = 8;
			for (int i = 0; i < 8; ++i)
			{
				childOn[i] = node->isChildMaskOn(
					NODE_SIZE_2 * trackingChildren[i].pos.x() +
					NODE_SIZE * trackingChildren[i].pos.y() +
					trackingChildren[i].pos.z());

				childCountOn -= childOn[i] ? 0 : 1;
			}
			if (childCountOn == 0)
			{
				return HTConstants::INVALID_POINTER;
			}
		}

		std::vector<uint32_t> nodeBytes;
		nodeBytes.push_back(0);
		int finalChildCount = 0;
		for (int i = 0; i < 8; ++i)
		{
			if (childOn[i])
			{
				uint32_t result = HTConstants::INVALID_POINTER;
				if (openvdbTrackingCube.span.x() == 2)
				{
					AxisAlignedCubeI trackingCube;
					trackingCube.pos = { 0, 0, 0 };
					constexpr int NODE_SIZE_HALF = NODE_SIZE / 2;
					trackingCube.span = { NODE_SIZE_HALF, NODE_SIZE_HALF, NODE_SIZE_HALF };

					typename NodeType::ChildNodeType* nextNode;
					openvdb::Vec3f dummy;
					int nodeIndex = NODE_SIZE_2 * trackingChildren[i].pos.x() +
						NODE_SIZE * trackingChildren[i].pos.y() +
						trackingChildren[i].pos.z();
					bool itemIn = node->beginChildAll().getItem(nodeIndex, nextNode, dummy);
					assert(itemIn);
#ifdef DEBUG_CONVERTER
					result = ConstructHashDAG(trackingCube, nextNode, hd, level + 1, full, depth + 1, treebboxChildren[i]);
#else
					result = ConstructHashDAG(trackingCube, nextNode, hd, level + 1, full, depth + 1);
#endif
				}
				else
				{
#ifdef DEBUG_CONVERTER
					result = ConstructHashDAG(trackingChildren[i], node, hd, level + 1, full, depth, treebboxChildren[i]);
#else
					result = ConstructHashDAG(trackingChildren[i], node, hd, level + 1, full, depth);
#endif
				}
				if (result != HTConstants::INVALID_POINTER)
				{
					nodeBytes[0] |= 1 << i;
					nodeBytes.push_back(result);
					++finalChildCount;
				}
			}
		}

		if (finalChildCount == 0)
		{
			return HTConstants::INVALID_POINTER;
		}

		// Insert child results along with newly created node into the HashDAG.
		uint32_t nodePtr = hd.FindOrAddNode(level, nodeBytes.size(), nodeBytes.data());

		return nodePtr;
	}
};