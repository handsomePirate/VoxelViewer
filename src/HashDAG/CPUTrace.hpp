#pragma once
#include "Core/Common.hpp"
#include "HashDAG.hpp"
#include "Vulkan/Camera.hpp"

void MakeImage(const TracingParameters& tracingParameters, int imageWidth, int imageHeight,
	const HashDAG& hd, uint8_t* imageData);
