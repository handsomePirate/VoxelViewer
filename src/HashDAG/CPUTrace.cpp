#include "CPUTrace.hpp"

void MakeImage(const TracingParameters& tp, int imageWidth, int imageHeight,
	const HashDAG& hd, uint8_t* imageData)
{
	for (int x = 0; x < imageWidth; ++x)
	{
		for (int y = 0; y < imageHeight; ++y)
		{
			Eigen::Vector3i voxel;
			const int samples = 1;

			uint32_t solution = 0;
			for (int s = 0; s < samples; ++s)
			{
				bool rayCastResult = hd.CastRay(tp.CameraPosition,
					tp.RayMin + x * tp.RayDDx + y * tp.RayDDy - tp.CameraPosition, voxel);
				if (rayCastResult)
				{
					solution += 255;
				}
			}

			imageData[(y * imageWidth + x) * 4 + 0] = solution / samples;
			imageData[(y * imageWidth + x) * 4 + 1] = solution / samples;
			imageData[(y * imageWidth + x) * 4 + 2] = solution / samples;
			imageData[(y * imageWidth + x) * 4 + 3] = 1;
		}
	}
}
