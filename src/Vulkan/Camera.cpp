#include "Camera.hpp"

Camera::Camera(const Eigen::Vector3f& position, const Eigen::Vector3f& forward, const Eigen::Vector3f& right, float fov)
	: position_(position), forward_(forward), right_(right), up_(right.cross(forward)), fov_(fov / 2.f * (PI_CONST / 180.f))
{}

TracingParameters Camera::GetTracingParameters(uint32_t imageWidth, uint32_t imageHeight) const
{
	TracingParameters tracingParameters{};

	const float aspect = imageWidth / float(imageHeight);
	const Eigen::Vector3f X = right_.normalized() * sin(fov_) * aspect;
	const Eigen::Vector3f Y = up_.normalized() * sin(fov_);
	const Eigen::Vector3f Z = forward_.normalized() * cos(fov_);

	const Eigen::Vector3f bottomLeft = position_ + Z - Y - X;
	const Eigen::Vector3f bottomRight = position_ + Z - Y + X;
	const Eigen::Vector3f topLeft = position_ + Z + Y - X;

	tracingParameters.CameraPosition = position_;

	tracingParameters.RayDDx = (bottomRight - bottomLeft) / float(imageWidth);
	tracingParameters.RayDDy = (topLeft - bottomLeft) / float(imageHeight);

	tracingParameters.RayMin = bottomLeft;

	return tracingParameters;
}
