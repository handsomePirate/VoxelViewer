#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_float64 : enable

#define MAX_TREES 128
#define MAX_LEVELS 12

layout(local_size_x = 16, local_size_y = 16) in;
layout(binding = 0, rgba8) uniform writeonly image2D resultImage;
layout(binding = 1, rgba32ui) uniform writeonly uimage2D idImage;

layout(std140, binding = 2) buffer PageTable
{
	// The individual data entries need to be aligned by vec4, ivec4 or uvec4 as required by std140.
	// Other alignment options are not available for arrays (as far as I know).
	// Individual array elements should be querried as follows: pointers[i / 4][i % 4].
	uvec4 pointers[];
};

layout(std140, binding = 3) buffer Pages
{
	uvec4 entries[];
};

struct TreeRoot
{
	// The order matters here, because of alignment.
	ivec3 rootOffset;
	uint rootNode;
};

layout(std140, binding = 4) buffer TreeRoots
{
	TreeRoot treeRoots[];
};

layout(std140, binding = 5) buffer SortedTrees
{
	ivec4 treeIndices[];
};

layout(std140, binding = 6) buffer Colors
{
	vec4 colorArrays[];
};

layout(std140, binding = 7) buffer ColorOffsets
{
	u64vec2 colorOffsets[];
};

layout(std140, binding = 8) buffer ColorIndices
{
	uvec4 colorIndices[];
};

layout(std140, binding = 9) buffer ColorIndexOffsets
{
	u64vec2 colorIndexOffsets[];
};

layout(std140, binding = 10) uniform CuttingPlanes
{
	float xMin;
	float xMax;
	float yMin;
	float yMax;
	float zMin;
	float zMax;
} cuttingPlanes;

layout(binding = 11) uniform TracingParameters 
{
	vec3 cameraPosition;
	vec3 rayMin;
	vec3 rayDDx;
	vec3 rayDDy;
	vec3 mousePosition;
	int selectionDiameter;
} parameters;

layout(push_constant) uniform PushConstants
{
	uint maxLevels;
	uint leafLevel;
	uint pageSize;
	uint pageCount;
	uint treeCount;
} pushConstants;

struct NodeInfo
{
	uint64_t voxelIndex;
	uint nodePtr;
	uint childMask;
	uint visitMask;
};

uint PageTableElement(uint e)
{
	return pointers[e / 4][e % 4];
}

uint PagePoolElement(uint e)
{
	return entries[e / 4][e % 4];
}

uint64_t PagePoolElement64(uint e)
{
	return (uint64_t(entries[e / 4][e % 4])) | (uint64_t(entries[(e + 1) / 4][(e + 1) % 4]) << 32);
}

uint Translate(uint vptr)
{
	const uint page = vptr / pushConstants.pageSize;
	const uint offset = vptr % pushConstants.pageSize;
	return PageTableElement(page) * pushConstants.pageSize + offset;
}

uint GetNodeChildMask(uint vptr)
{
	return PagePoolElement(Translate(vptr)) & 0xFFu;
}

uint GetChildNode(uint node, uint nextChild, uint childMask)
{
	uint offset = bitCount(childMask & ((1u << nextChild) - 1u)) + 1;
	return PagePoolElement(Translate(node) + ((offset - 1) << 1u) + 1);
}

uint GetChildOffset(uint node, uint nextChild, uint childMask)
{
	uint offset = bitCount(childMask & ((1u << nextChild) - 1u)) + 1;
	return PagePoolElement(Translate(node) + (offset << 1u));
}

uint GetFirstLeafMask(uint64_t leaf)
{
	return uint(
		((leaf & 0x00000000000000FFul) == 0 ? 0 : 1 << 0) |
		((leaf & 0x000000000000FF00ul) == 0 ? 0 : 1 << 1) |
		((leaf & 0x0000000000FF0000ul) == 0 ? 0 : 1 << 2) |
		((leaf & 0x00000000FF000000ul) == 0 ? 0 : 1 << 3) |
		((leaf & 0x000000FF00000000ul) == 0 ? 0 : 1 << 4) |
		((leaf & 0x0000FF0000000000ul) == 0 ? 0 : 1 << 5) |
		((leaf & 0x00FF000000000000ul) == 0 ? 0 : 1 << 6) |
		((leaf & 0xFF00000000000000ul) == 0 ? 0 : 1 << 7));
}

uint GetSecondLeafMask(uint64_t leaf, uint firstMask)
{
	const uint shift = uint(firstMask * 8);
	return uint(leaf >> shift) & 0x000000FF;
}

uint GetFirstVoxelCount(uint64_t leaf, uint nextChild)
{
	uint leaf1 = uint(leaf & 0x00000000FFFFFFFFul);
	uint leaf2 = uint((leaf & 0xFFFFFFFF00000000ul) >> 32);

	uint mask1 = uint(
		((nextChild == 0) ? 0x00000000 : 0) |
		((nextChild == 1) ? 0x000000FF : 0) |
		((nextChild == 2) ? 0x0000FFFF : 0) |
		((nextChild == 3) ? 0x00FFFFFF : 0) |
		((nextChild == 4) ? 0xFFFFFFFF : 0) |
		((nextChild == 5) ? 0xFFFFFFFF : 0) |
		((nextChild == 6) ? 0xFFFFFFFF : 0) |
		((nextChild == 7) ? 0xFFFFFFFF : 0));

	uint mask2 = uint(
		((nextChild == 0) ? 0x00000000 : 0) |
		((nextChild == 1) ? 0x00000000 : 0) |
		((nextChild == 2) ? 0x00000000 : 0) |
		((nextChild == 3) ? 0x00000000 : 0) |
		((nextChild == 4) ? 0x00000000 : 0) |
		((nextChild == 5) ? 0x000000FF : 0) |
		((nextChild == 6) ? 0x0000FFFF : 0) |
		((nextChild == 7) ? 0x00FFFFFF : 0));

	return bitCount(leaf1 & mask1) + bitCount(leaf2 & mask2);
}

uint GetSecondVoxelCount(uint mask, uint nextChild)
{
	return bitCount(mask & ((1u << nextChild) - 1));
}

uint ComputeChildIntersectionMask(uint level, uvec3 traversalPath, vec3 rayPos, vec3 rayDir, vec3 rayInv,
	bool isRoot);

vec3 GetVoxelColor(int tree, uint64_t voxelIndex)
{
	if (colorIndexOffsets.length() == 1)
	{
		uint64_t colorIndex = colorOffsets[tree / 2][tree % 2] + voxelIndex;
		return colorArrays[uint(colorIndex)].rgb;
	}
	else
	{
		uint64_t colorIndex = colorIndexOffsets[tree / 2][tree % 2] + voxelIndex;
		uint compressedColorIndex = colorIndices[uint(colorIndex) / 4][uint(colorIndex) % 4];
		uint64_t finalColorIndex = colorOffsets[tree / 2][tree % 2] + compressedColorIndex;
		return colorArrays[uint(finalColorIndex)].rgb;
	}
}

uvec3 PathAscend(uvec3 path, uint levels)
{
	path.x >>= levels;
	path.y >>= levels;
	path.z >>= levels;
	return path;
}

uvec3 PathDescend(uvec3 path, uint child)
{
	path.x <<= 1;
	path.y <<= 1;
	path.z <<= 1;
	path.x |= (child & 0x4u) >> 2;
	path.y |= (child & 0x2u) >> 1;
	path.z |= (child & 0x1u) >> 0;
	return path;
}

vec3 PathAsPosition(uvec3 path, uint levelRank)
{
	return vec3(float(path.x << levelRank), float(path.y << levelRank), float(path.z << levelRank));
}

float MaxCoeff(vec3 v)
{
	return max(v.x, max(v.y, v.z));
}

float MinCoeff(vec3 v)
{
	return min(v.x, min(v.y, v.z));
}

float rayTMin = 0;
float rayTMax = 100000000;

void CutRay(vec3 rayPos, vec3 rayInv)
{
	vec3 cutMax = vec3(cuttingPlanes.xMax, cuttingPlanes.yMax, cuttingPlanes.zMax);
	vec3 cutMin = vec3(cuttingPlanes.xMin, cuttingPlanes.yMin, cuttingPlanes.zMin);
	vec3 maxDiff = cutMax - rayPos;
	vec3 minDiff = cutMin - rayPos;

	vec3 diff1 = vec3(
		((rayInv.x > 0) ? maxDiff.x : minDiff.x),
		((rayInv.y > 0) ? maxDiff.y : minDiff.y),
		((rayInv.z > 0) ? maxDiff.z : minDiff.z));

	vec3 tMax = diff1 * rayInv;
	rayTMax = min(MinCoeff(tMax), rayTMax);

	vec3 diff2 = vec3(
		((maxDiff.x < 0) ? maxDiff.x : 0),
		((maxDiff.y < 0) ? maxDiff.y : 0),
		((maxDiff.z < 0) ? maxDiff.z : 0));

	vec3 tMin = diff2 * rayInv;
	rayTMin = max(MaxCoeff(tMin), rayTMin);

	vec3 diff3 = vec3(
		((minDiff.x > 0) ? minDiff.x : 0),
		((minDiff.y > 0) ? minDiff.y : 0),
		((minDiff.z > 0) ? minDiff.z : 0));

	tMin = diff3 * rayInv;
	rayTMin = max(MaxCoeff(tMin), rayTMin);

	return;
}

int TreeIndex(int tree)
{
	return treeIndices[tree / 4][tree % 4];
}

void main()
{
	uint pixX = gl_GlobalInvocationID.x;
	uint pixY = gl_GlobalInvocationID.y;

	vec3 rayDir = normalize(parameters.rayMin + pixX * parameters.rayDDx + pixY * parameters.rayDDy - parameters.cameraPosition);
	const float epsilon = 0.0001;
	rayDir.x = (rayDir.x >= 0 && rayDir.x < epsilon) ? epsilon : rayDir.x;
	rayDir.x = (rayDir.x < 0 && rayDir.x > -epsilon) ? -epsilon : rayDir.x;
	rayDir.y = (rayDir.y >= 0 && rayDir.y < epsilon) ? epsilon : rayDir.y;
	rayDir.y = (rayDir.y < 0 && rayDir.y > -epsilon) ? -epsilon : rayDir.y;
	rayDir.z = (rayDir.z >= 0 && rayDir.z < epsilon) ? epsilon : rayDir.z;
	rayDir.z = (rayDir.z < 0 && rayDir.z > -epsilon) ? -epsilon : rayDir.z;

	vec3 rayInv = vec3(1.f / rayDir.x, 1.f / rayDir.y, 1.f / rayDir.z);

	CutRay(parameters.cameraPosition, rayInv);

	uint rayChildOrder =
		(rayDir.x < 0.f ? 4 : 0) +
		(rayDir.y < 0.f ? 2 : 0) +
		(rayDir.z < 0.f ? 1 : 0);

	vec3 resultColor = vec3(0.3);
	uvec4 idColor = uvec4(0, 0, 0, 0xFFFFFFFFu);
	bool hit = false;
	vec3 voxelPosition;
	for (int tree = 0; !hit && tree < pushConstants.treeCount; ++tree)
	{
		int treeIndex = TreeIndex(tree);
		vec3 rayPos = parameters.cameraPosition - vec3(treeRoots[treeIndex].rootOffset);

		uint level = 0;
		NodeInfo stack[MAX_LEVELS];

		//uint cachedLeafPart1;
		//uint cachedLeafPart2;
		uint64_t cachedLeaf;

		uvec3 traversalPath = uvec3(0);

		stack[level].nodePtr = treeRoots[treeIndex].rootNode;
		stack[level].childMask = GetNodeChildMask(stack[level].nodePtr);
		stack[level].visitMask = stack[level].childMask & ComputeChildIntersectionMask(level, traversalPath,
			rayPos, rayDir, rayInv, true);
		stack[level].voxelIndex = 0;

		for (;;)
		{
			uint formerLevel = level;
			// Ascend when there are no children to be visited.
			while (level > 0 && stack[level].visitMask == 0)
			{
				--level;
			}

			// Handle the case where we there are no voxels intersected by the ray inside this tree.
			if (level == 0 && stack[level].visitMask == 0)
			{
				// Empty path signifies that we found no intersections.
				break;
			}

			traversalPath = PathAscend(traversalPath, formerLevel - level);

			uint nextChild = 255;
			for (uint child = 0; child < 8; ++child)
			{
				uint childInOrder = child ^ rayChildOrder;
				if ((stack[level].visitMask & (1u << childInOrder)) != 0)
				{
					nextChild = childInOrder;
					break;
				}
			}

			stack[level].visitMask &= ~(1u << nextChild);

			// We descend into the selected child.
			traversalPath = PathDescend(traversalPath, nextChild);
			++level;

			if (level == pushConstants.maxLevels)
			{
				stack[level - 1].voxelIndex = stack[level - 1].voxelIndex + GetSecondVoxelCount(stack[level - 1].childMask, nextChild);
			}

			if (level == pushConstants.maxLevels)
			{
				// We have intersected a voxel and we have the path to it.
				resultColor = GetVoxelColor(treeIndex, stack[level - 1].voxelIndex);
				idColor = uvec4(traversalPath, treeIndex);
				hit = true;
				voxelPosition = vec3(treeRoots[treeIndex].rootOffset) + PathAsPosition(traversalPath, 0);
				break;
			}

			if (level < pushConstants.leafLevel)
			{
				// We are in an internal node.
				stack[level].nodePtr = GetChildNode(stack[level - 1].nodePtr, nextChild, stack[level - 1].childMask);
				stack[level].childMask = GetNodeChildMask(stack[level].nodePtr);
				stack[level].visitMask = stack[level].childMask & ComputeChildIntersectionMask(level, traversalPath, rayPos, rayDir,
					rayInv, false);
				stack[level].voxelIndex = stack[level - 1].voxelIndex + 
					GetChildOffset(stack[level - 1].nodePtr, nextChild, stack[level - 1].childMask);
			}
			else
			{
				uint childMask;
				uint64_t voxelIndex;
				if (level == pushConstants.leafLevel)
				{
					uint leafPtr = GetChildNode(stack[level - 1].nodePtr, nextChild, stack[level - 1].childMask);
					cachedLeaf = PagePoolElement64(Translate(leafPtr));
					childMask = GetFirstLeafMask(cachedLeaf);

					voxelIndex = stack[level - 1].voxelIndex +
						GetChildOffset(stack[level - 1].nodePtr, nextChild, stack[level - 1].childMask);
				}
				else
				{
					childMask = GetSecondLeafMask(cachedLeaf, nextChild);
					voxelIndex = stack[level - 1].voxelIndex + GetFirstVoxelCount(cachedLeaf, nextChild);
				}

				stack[level].childMask = childMask;
				stack[level].visitMask = stack[level].childMask & ComputeChildIntersectionMask(level, traversalPath, rayPos, rayDir,
					rayInv, false);
				stack[level].voxelIndex = voxelIndex;
			}
		}
	}

	if (hit)
	{
		//resultColor = voxelPosition * length(voxelPosition) * 0.000007f * vec3(1, 0.1, 1);
		float overlayAlpha = .6f;
		vec3 overlayColor = vec3(.1f, .1f, .1f);
		float dist = length(parameters.mousePosition - voxelPosition);
		resultColor = (dist < parameters.selectionDiameter * .5f) ?
			overlayColor * overlayAlpha + resultColor * (1 - overlayAlpha) :
			resultColor;
	}

	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), vec4(resultColor, 1));
	imageStore(idImage, ivec2(gl_GlobalInvocationID.xy), idColor);
}

uint ComputeChildIntersectionMask(uint level, uvec3 traversalPath, vec3 rayPos, vec3 rayDir, vec3 rayInv,
	bool isRoot)
{
	const uint levelRank = pushConstants.maxLevels - level;
	const float nodeRadius = float(1u << (levelRank - 1));
	const vec3 nodeCenter = vec3(nodeRadius) + PathAsPosition(traversalPath, levelRank);

	const vec3 rayToNodeCenter = nodeCenter - rayPos;

	vec3 tMid = rayToNodeCenter * rayInv;

	float tMin, tMax;
	{
		const vec3 slabRadius = nodeRadius * abs(rayInv);
		const vec3 pMin = tMid - slabRadius;
		tMin = max(MaxCoeff(pMin), .0f);

		const vec3 pMax = tMid + slabRadius;
		tMax = MinCoeff(pMax);
	}

	tMin = max(tMin, rayTMin);
	tMax = min(tMax, rayTMax);

	if (tMin >= tMax)
	{
		return 0;
	}

	uint intersectionMask = 0;
	{
		const vec3 pointOnRay = (0.5f * (tMin + tMax)) * rayDir;

		const uint firstChild =
			((pointOnRay.x >=rayToNodeCenter.x) ? 4 : 0) +
			((pointOnRay.y >=rayToNodeCenter.y) ? 2 : 0) +
			((pointOnRay.z >=rayToNodeCenter.z) ? 1 : 0);

		intersectionMask |= (1u << firstChild);
	}

	const float epsilon = 1e-4f;

	if (tMin <= tMid.x && tMid.x <= tMax)
	{
		const vec3 pointOnRay = tMid.x * rayDir;

		uint A = 0;
		if (pointOnRay.y >= rayToNodeCenter.y - epsilon)
		{
			A |= 0xCC;
		}
		if (pointOnRay.y <= rayToNodeCenter.y + epsilon)
		{
			A |= 0x33;
		}

		uint B = 0;
		if (pointOnRay.z >= rayToNodeCenter.z - epsilon)
		{
			B |= 0xAA;
		}
		if (pointOnRay.z <= rayToNodeCenter.z + epsilon)
		{
			B |= 0x55;
		}

		intersectionMask |= A & B;
	}
	if (tMin <= tMid.y && tMid.y <= tMax)
	{
		const vec3 pointOnRay = tMid.y * rayDir;

		uint C = 0;
		if (pointOnRay.x >= rayToNodeCenter.x - epsilon)
		{
			C |= 0xF0;
		}
		if (pointOnRay.x <= rayToNodeCenter.x + epsilon)
		{
			C |= 0x0F;
		}

		uint D = 0;
		if (pointOnRay.z >= rayToNodeCenter.z - epsilon)
		{
			D |= 0xAA;
		}
		if (pointOnRay.z <= rayToNodeCenter.z + epsilon)
		{
			D |= 0x55;
		}

		intersectionMask |= C & D;
	}
	if (tMin <= tMid.z && tMid.z <= tMax)
	{
		const vec3 pointOnRay = tMid.z * rayDir;

		uint E = 0;
		if (pointOnRay.x >= rayToNodeCenter.x - epsilon)
		{
			E |= 0xF0;
		}
		if (pointOnRay.x <= rayToNodeCenter.x + epsilon)
		{
			E |= 0x0F;
		}


		uint F = 0;
		if (pointOnRay.y >= rayToNodeCenter.y - epsilon)
		{
			F |= 0xCC;
		}
		if (pointOnRay.y <= rayToNodeCenter.y + epsilon)
		{
			F |= 0x33;
		}

		intersectionMask |= E & F;
	}

	return intersectionMask;
}
