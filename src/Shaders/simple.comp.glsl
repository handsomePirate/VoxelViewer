#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAX_TREES 128
#define MAX_LEVELS 12

layout(local_size_x = 16, local_size_y = 16) in;
layout(binding = 0, rgba8) uniform writeonly image2D resultImage;

layout(std140, binding = 1) buffer PageTable
{
	// The individual data entries need to be aligned by vec4, ivec4 or uvec4 as required by std140.
	// Other alignment options are not available for arrays (as far as I know).
	// Individual array elements should be querried as follows: pointers[i / 4][i % 4].
	uvec4 pointers[];
};

layout(std140, binding = 2) buffer Pages
{
	uvec4 entries[];
};

struct TreeRoot
{
	// The order matters here, because of alignment.
	ivec3 rootOffset;
	uint rootNode;
};

layout(std140, binding = 3) buffer TreeRoots
{
	TreeRoot treeRoots[];
};

layout(binding = 4) uniform TracingParameters 
{
	vec3 cameraPosition;
	vec3 rayMin;
	vec3 rayDDx;
	vec3 rayDDy;
	int voxelDetail;
	float colorScale;
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
	return PagePoolElement(Translate(node) + offset);
}

uint GetFirstLeafMask(uint leafPart1, uint leafPart2)
{
	return uint(
		((leafPart1 & 0x000000FF) == 0 ? 0 : 1 << 0) |
		((leafPart1 & 0x0000FF00) == 0 ? 0 : 1 << 1) |
		((leafPart1 & 0x00FF0000) == 0 ? 0 : 1 << 2) |
		((leafPart1 & 0xFF000000) == 0 ? 0 : 1 << 3) |
		((leafPart2 & 0x000000FF) == 0 ? 0 : 1 << 4) |
		((leafPart2 & 0x0000FF00) == 0 ? 0 : 1 << 5) |
		((leafPart2 & 0x00FF0000) == 0 ? 0 : 1 << 6) |
		((leafPart2 & 0xFF000000) == 0 ? 0 : 1 << 7));
}

uint GetSecondLeafMask(uint leafPart1, uint leafPart2, uint firstMask)
{
	const uint shift = uint(firstMask * 8);
	return (shift < 32) ? uint(leafPart1 >> shift) : uint(leafPart2 >> (shift - 32));
}

uint ComputeChildIntersectionMask(uint level, uvec3 traversalPath, vec3 rayPos, vec3 rayDir, vec3 rayInv, float treeScale,
	bool isRoot);

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

void main()
{
	uint pixX = gl_GlobalInvocationID.x;
	uint pixY = gl_GlobalInvocationID.y;

	uint voxelDetail = uint(parameters.voxelDetail);

	// TODO: Here would probably be clipping of the ray.

	vec3 rayDir = normalize(parameters.rayMin + pixX * parameters.rayDDx + pixY * parameters.rayDDy - parameters.cameraPosition);
	vec3 rayInv = vec3(1.f / rayDir.x, 1.f / rayDir.y, 1.f / rayDir.z);

	uint rayChildOrder =
		(rayDir.x < 0.f ? 4 : 0) +
		(rayDir.y < 0.f ? 2 : 0) +
		(rayDir.z < 0.f ? 1 : 0);

	uvec3 traversalPaths[MAX_TREES];

	vec3 resultColor = vec3(0);
	//int tree = 6;
	for (int tree = 0; tree < pushConstants.treeCount; ++tree)
	{
		vec3 rayPos = parameters.cameraPosition - vec3(treeRoots[tree].rootOffset);

		uint level = 0;
		NodeInfo stack[MAX_LEVELS];

		uint cachedLeafPart1;
		uint cachedLeafPart2;

		traversalPaths[tree] = uvec3(0);

		stack[level].nodePtr = treeRoots[tree].rootNode;
		stack[level].childMask = GetNodeChildMask(stack[level].nodePtr);
		stack[level].visitMask = stack[level].childMask & ComputeChildIntersectionMask(level, traversalPaths[tree],
			rayPos, rayDir, rayInv, 1, true);

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
				traversalPaths[tree] = uvec3(0xFFFFFFFF);
				break;
			}

			traversalPaths[tree] = PathAscend(traversalPaths[tree], formerLevel - level);

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
			traversalPaths[tree] = PathDescend(traversalPaths[tree], nextChild);
			++level;

			if (level == min(pushConstants.maxLevels, voxelDetail))
			{
				// We have intersected a voxel and we have the path to it.
				resultColor = vec3(1);
				break;
			}

			if (level < pushConstants.leafLevel)
			{
				// We are in an internal node.
				stack[level].nodePtr = GetChildNode(stack[level - 1].nodePtr, nextChild, stack[level - 1].childMask);
				stack[level].childMask = GetNodeChildMask(stack[level].nodePtr);
				stack[level].visitMask = stack[level].childMask & ComputeChildIntersectionMask(level, traversalPaths[tree], rayPos, rayDir,
					rayInv, 1, false);
			}
			else
			{
				uint childMask;
				if (level == pushConstants.leafLevel)
				{
					uint leafPtr = GetChildNode(stack[level - 1].nodePtr, nextChild, stack[level - 1].childMask);
					cachedLeafPart1 = PagePoolElement(Translate(leafPtr));
					cachedLeafPart2 = PagePoolElement(Translate(leafPtr) + 1);
					childMask = GetFirstLeafMask(cachedLeafPart1, cachedLeafPart2);
				}
				else
				{
					childMask = GetSecondLeafMask(cachedLeafPart1, cachedLeafPart2, nextChild);
				}

				stack[level].childMask = childMask;
				stack[level].visitMask = stack[level].childMask & ComputeChildIntersectionMask(level, traversalPaths[tree], rayPos, rayDir,
					rayInv, 1, false);
			}
		}
	}

	float hitDist = 3.40282e+038;
	for (int tree = 0; tree < pushConstants.treeCount; ++tree)
	{
		if (traversalPaths[tree].x != 0xFFFFFFFF || traversalPaths[tree].y != 0xFFFFFFFF || traversalPaths[tree].z != 0xFFFFFFFF)
		{
			vec3 voxelPos = vec3(treeRoots[tree].rootOffset) + PathAsPosition(traversalPaths[tree], 0);
			float dist = distance(parameters.cameraPosition, voxelPos);
			if (dist < hitDist)
			{
				resultColor = abs(voxelPos) * parameters.colorScale;
				hitDist = dist;
			}
		}
	}

	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), vec4(resultColor, 1));
}

float MaxCoeff(vec3 v)
{
	return max(v.x, max(v.y, v.z));
}

float MinCoeff(vec3 v)
{
	return min(v.x, min(v.y, v.z));
}

uint ComputeChildIntersectionMask(uint level, uvec3 traversalPath, vec3 rayPos, vec3 rayDir, vec3 rayInv, float treeScale,
	bool isRoot)
{
	const uint levelRank = pushConstants.maxLevels - level;
	const float nodeRadius = float(1u << (levelRank - 1));
	const vec3 nodeCenter = (vec3(nodeRadius) + PathAsPosition(traversalPath, levelRank)) * treeScale;

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

	if (isRoot && (tMin >= tMax))
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
