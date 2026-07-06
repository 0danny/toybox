#pragma once

#include <fstream>

namespace GeomParser
{
	void ReadGeomBlock(std::ifstream& file, bool LOD);
}