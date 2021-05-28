#pragma once
#include "Core/Common.hpp"
#include "Vulkan/Utils.hpp"
#include <Eigen/Dense>

#define PI_CONST 3.141592653589f

class Camera
{
public:
	Camera(const Eigen::Vector3f& position, const Eigen::Vector3f& forward, const Eigen::Vector3f& right, float fov);
	Camera() = default;
	~Camera() = default;
private:
	SHADER_ALIGN Eigen::Vector3f position_;
	SHADER_ALIGN Eigen::Vector3f forward_;
	SHADER_ALIGN Eigen::Vector3f right_;
	SHADER_ALIGN Eigen::Vector3f up_;
	SHADER_ALIGN float fov_;
};
