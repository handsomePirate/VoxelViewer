#pragma once
#include "Core/Common.hpp"

#include <openvdb/openvdb.h>

namespace OpenVDBUtils
{
	openvdb::Vec3SGrid::Ptr LoadGrid(const std::string& path)
	{
		openvdb::io::File loadFile(path);
		loadFile.open();

		// more simply by calling file.readGrid("LevelSetSphere").)
		openvdb::GridBase::Ptr baseGrid;
		for (openvdb::io::File::NameIterator nameIter = loadFile.beginName(); nameIter != loadFile.endName(); ++nameIter)
		{
			// Read in only the grid we are interested in.
			baseGrid = loadFile.readGrid(nameIter.gridName());
		}
		loadFile.close();

		openvdb::Vec3SGrid::Ptr gridptr = openvdb::gridPtrCast<openvdb::Vec3SGrid>(baseGrid);

		return gridptr;
	}
}
