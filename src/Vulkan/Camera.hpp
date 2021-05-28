#pragma once
#include "Core/Common.hpp"
#include "Vulkan/Utils.hpp"
#include <Eigen/Dense>

#define PI_CONST 3.141592653589f

struct TracingParameters
{
	SHADER_ALIGN Eigen::Vector3f CameraPosition;
	SHADER_ALIGN Eigen::Vector3f RayMin;
	SHADER_ALIGN Eigen::Vector3f RayDDx;
	SHADER_ALIGN Eigen::Vector3f RayDDy;
};

class Camera
{
public:
	Camera(const Eigen::Vector3f& position, const Eigen::Vector3f& forward, const Eigen::Vector3f& right, float fov);
	Camera() = default;
	~Camera() = default;

	TracingParameters GetTracingParameters(uint32_t imageWidth, uint32_t imageHeight) const;

private:
	Eigen::Vector3f position_;
	Eigen::Vector3f forward_;
	Eigen::Vector3f right_;
	Eigen::Vector3f up_;
	float fov_;
};
