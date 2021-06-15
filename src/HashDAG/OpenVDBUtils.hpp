#pragma once
#include "Core/Common.hpp"

#include <openvdb/openvdb.h>

namespace OpenVDBUtils
{
	openvdb::Vec3SGrid::Ptr LoadGrid(const std::string& path, const std::string& name = "")
	{
		openvdb::io::File loadFile(path);
		loadFile.open();

		openvdb::GridBase::Ptr baseGrid;
		bool foundGrid = false;
		for (openvdb::io::File::NameIterator nameIter = loadFile.beginName(); nameIter != loadFile.endName(); ++nameIter)
		{
			CoreLogInfo("Found grid %s.", nameIter.gridName().c_str());
			// Read in only the grid we are interested in.
			if (name == "" || nameIter.gridName() == name)
			{
				baseGrid = loadFile.readGrid(nameIter.gridName());
				foundGrid = true;
			}
		}
		loadFile.close();

		if (!foundGrid)
		{
			CoreLogError("Couldn't find the required grid name in the grid file.");
		}

		openvdb::Vec3SGrid::Ptr grid = openvdb::gridPtrCast<openvdb::Vec3SGrid>(baseGrid);

		return grid;
	}
}
