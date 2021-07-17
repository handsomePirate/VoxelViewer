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
	SHADER_ALIGN Eigen::Vector3f MousePosition;
	int SelectionDiameter;
};

struct CameraSetup
{
	Eigen::Vector3f Position;
	Eigen::Vector3f Forward;
	Eigen::Vector3f Right;
};

class Camera
{
public:
	Camera(const Eigen::Vector3f& position, const Eigen::Vector3f& forward, const Eigen::Vector3f& right, float fov);
	Camera() = default;
	~Camera() = default;

	void Set(const CameraSetup& setup);
	void Rotate(const Eigen::Vector3f& axis, const float angle);
	void RotateLocal(const Eigen::Vector3f& axis, const float angle);

	void Move(const Eigen::Vector3f& offset);
	void MoveLocal(const Eigen::Vector3f& offset);

	Eigen::Vector3f& Position();
	Eigen::Vector3f& Forward();
	Eigen::Vector3f& Right();
	Eigen::Vector3f& Up();
	float& Fov();

	static float DegToRad(float value);

	void GetTracingParameters(uint32_t imageWidth, uint32_t imageHeight, TracingParameters& tracingParameters) const;

private:
	Eigen::Matrix3f GetLocalToGlobalMatrix() const;

	Eigen::Vector3f position_;
	Eigen::Vector3f forward_;
	Eigen::Vector3f right_;
	Eigen::Vector3f up_;
	float fov_;
};
