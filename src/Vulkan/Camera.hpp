#pragma once
#include "Core/Common.hpp"
#include <Eigen/Dense>

#define PI_CONST 3.141592653589f

class Camera
{
public:
	Camera(const Eigen::Vector3f& position, const Eigen::Vector3f& forward, const Eigen::Vector3f& right, float fov);
	Camera() = default;
	~Camera() = default;
private:
	alignas(16) Eigen::Vector3f position_;
	alignas(16) Eigen::Vector3f forward_;
	alignas(16) Eigen::Vector3f right_;
	alignas(16) Eigen::Vector3f up_;
	alignas(4) float fov_;
};
