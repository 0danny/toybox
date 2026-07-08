#include "Parsers/BinaryReader.hpp"
#include "Clock.hpp"

#include "Parsers/NGNParser.hpp"
#include "Parsers/TextureParser.hpp"
#include "Parsers/GeomParser.hpp"

#include <print>

namespace NGNParser
{
	void ReadNGN(std::ifstream& file)
	{
		BinaryReader binaryReader(file);
		Timer timer;

		std::println("\n-- Parsing NGN... --");

		uint32_t BlockNum, BlockSize = 0;
		bool LOD = false;

		while (file)
		{
			BlockNum = binaryReader.readU32();
			BlockSize = binaryReader.readU32();
			if (! BlockSize)
				break;

			std::println("Block Number: {}", BlockNum);

			if (BlockNum == 260)
			{ // 260 = Textures
				TextureParser::ReadTextureBlock(file);
			}
			else if (BlockNum == 256)
			{ // 256 = Geometry
				GeomParser::ReadGeomBlock(file, LOD);
				if (LOD == true)
					file.seekg(BlockSize, std::ios::cur);
				if (LOD == false)
					LOD = true;
			}
			else if (BlockNum == 0)
				break;
			else
				file.seekg(BlockSize, std::ios::cur);
		}

		std::println("-- Took {}ms to parse NGN --\n", timer.returnTime());
	}
}
