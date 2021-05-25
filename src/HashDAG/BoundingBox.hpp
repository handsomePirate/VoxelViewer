#pragma once
#include <Eigen/Dense>

struct BoundingBox
{
	/// The position of the box's corner that has the least coordinate value in all axes.
	Eigen::Vector3i pos;
	/// The extents of the cube.
	Eigen::Vector3i span;

	static bool IsPointInCube(const BoundingBox& cube, Eigen::Vector3i point)
	{
		bool xIntersects = cube.pos.x() <= point.x() && cube.pos.x() + cube.span.x() > point.x();
		bool yIntersects = cube.pos.y() <= point.y() && cube.pos.y() + cube.span.y() > point.y();
		bool zIntersects = cube.pos.z() <= point.z() && cube.pos.z() + cube.span.z() > point.z();
		return xIntersects && yIntersects && zIntersects;
	}

	static bool CubesIntersect(const BoundingBox& cube1, const BoundingBox& cube2)
	{
		bool xIntersects = !(cube2.pos.x() + cube2.span.x() <= cube1.pos.x() || cube1.pos.x() + cube1.span.x() <= cube2.pos.x());
		bool yIntersects = !(cube2.pos.y() + cube2.span.y() <= cube1.pos.y() || cube1.pos.y() + cube1.span.y() <= cube2.pos.y());
		bool zIntersects = !(cube2.pos.z() + cube2.span.z() <= cube1.pos.z() || cube1.pos.z() + cube1.span.z() <= cube2.pos.z());
		return xIntersects && yIntersects && zIntersects;
	}

	static void SplitCube(const BoundingBox& cube, BoundingBox children[8])
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

	friend std::ostream& operator<<(std::ostream& os, const BoundingBox& cube)
	{
		os << '[' << cube.pos.x() << ", " << cube.pos.y() << ", " << cube.pos.z() << "] -> "
			<< '[' << cube.pos.x() + cube.span.x() - 1 << ", " << cube.pos.y() + cube.span.y() - 1 <<
			", " << cube.pos.z() + cube.span.z() - 1 << ']';
		return os;
	}
};