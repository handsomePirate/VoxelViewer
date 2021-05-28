#include "Camera.hpp"

Camera::Camera(const Eigen::Vector3f& position, const Eigen::Vector3f& forward, const Eigen::Vector3f& right, float fov)
	: position_(position), forward_(forward), right_(right), up_(right.cross(forward)), fov_(fov)
{}
