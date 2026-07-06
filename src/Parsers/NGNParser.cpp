#include "Parsers/NGNParser.hpp"
#include "Parsers/TextureParser.hpp"
#include "Parsers/GeomParser.hpp"

#include <print>
#include <chrono>

namespace NGNParser
{
	const std::streamsize readUInt32 = 4;
	const std::streamsize readUInt8 = 1;

	void ReadNGN(std::ifstream& file)
	{
		auto start_time = std::chrono::high_resolution_clock::now();
		std::println("\n-- Parsing NGN... --");

		uint32_t BlockNum, BlockSize = 0;
		bool LOD = false;
		while (file.read(reinterpret_cast<char*>(&BlockNum), readUInt32))
		{
			if (! file.read(reinterpret_cast<char*>(&BlockSize), readUInt32))
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
			{
				break;
			}
			else
			{
				file.seekg(BlockSize, std::ios::cur);
			}
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		std::println("-- Took {}ms to parse NGN --\n", std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());
	}
}
