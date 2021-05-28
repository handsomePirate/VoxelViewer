#include "Camera.hpp"

Camera::Camera(const Eigen::Vector3f& position, const Eigen::Vector3f& forward, const Eigen::Vector3f& right, float fov)
	: position_(position),
	forward_(forward.normalized()),
	right_(right.normalized()),
	up_(right_.cross(forward_)),
	fov_(DegToRad(fov))
{}

void Camera::Rotate(const Eigen::Vector3f& axis, const float angle)
{
	if (axis.isZero() || angle == 0) { return; }

	const auto axisRotation = Eigen::AngleAxisf(angle, axis);
	forward_ = axisRotation * forward_;
	right_ = axisRotation * right_;
	up_ = right_.cross(forward_);
}

void Camera::RotateLocal(const Eigen::Vector3f& axis, const float angle)
{
	Rotate(GetLocalToGlobalMatrix() * axis, angle);
}

Eigen::Vector3f& Camera::Position()
{
	return position_;
}

Eigen::Vector3f& Camera::Forward()
{
	return forward_;
}

Eigen::Vector3f& Camera::Right()
{
	return right_;
}

Eigen::Vector3f& Camera::Up()
{
	return up_;
}

float& Camera::Fov()
{
	return fov_;
}

float Camera::DegToRad(float value)
{
	return value / 2.f * (PI_CONST / 180.f);
}

TracingParameters Camera::GetTracingParameters(uint32_t imageWidth, uint32_t imageHeight) const
{
	TracingParameters tracingParameters{};

	const float aspect = imageWidth / float(imageHeight);
	const Eigen::Vector3f X = right_ * sin(fov_) * aspect;
	const Eigen::Vector3f Y = up_ * sin(fov_);
	const Eigen::Vector3f Z = forward_ * cos(fov_);

	const Eigen::Vector3f bottomLeft = position_ + Z - Y - X;
	const Eigen::Vector3f bottomRight = position_ + Z - Y + X;
	const Eigen::Vector3f topLeft = position_ + Z + Y - X;

	tracingParameters.CameraPosition = position_;

	tracingParameters.RayDDx = (bottomRight - bottomLeft) / float(imageWidth);
	tracingParameters.RayDDy = (topLeft - bottomLeft) / float(imageHeight);

	tracingParameters.RayMin = bottomLeft;

	return tracingParameters;
}

Eigen::Matrix3f Camera::GetLocalToGlobalMatrix() const
{
	Eigen::Matrix3f m;
	m << right_, up_, forward_;

	return m;
}
