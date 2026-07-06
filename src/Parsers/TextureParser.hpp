#pragma once

#include "Parsers/NGNParser.hpp"

namespace TextureParser
{
    std::vector<NGNParser::Texture> ReturnImages();
	void ReadTextureBlock(std::ifstream& file);
}