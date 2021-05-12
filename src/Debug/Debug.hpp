#pragma once
#include "Core/Common.hpp"

namespace Debug
{
	namespace ValidationLayers
	{
		bool CheckLayerPresent(const char* name);
		void Start(VkInstance instance);
		void Shutdown(VkInstance instance);
	}
}
