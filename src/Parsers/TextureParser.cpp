#include "Parsers/BinaryReader.hpp"
#include "Parsers/TextureParser.hpp"

#include "Clock.hpp"

#include <fstream>
#include <print>

#include <GLFW/glfw3.h>

namespace TextureParser
{
	static std::vector<NGNParser::Texture> textures;

	std::vector<NGNParser::Texture> ReturnImages()
	{ return textures; }

	void ReadTextureBlock(std::ifstream& file)
	{
		BinaryReader binaryReader(file);

		Timer timer;

		textures.clear();

		uint32_t textureCount = binaryReader.readU32();

		textures.resize(textureCount);

		std::vector<GLuint> textureIDs(textureCount);
		glGenTextures(textureCount, textureIDs.data());

		for (uint32_t i = 0; i < textureCount; i++)
		{
			file.seekg(4, std::ios::cur); // Seek textureByteLength
			uint32_t textureNameByteLength = binaryReader.readU32();
			std::string textureName = binaryReader.readStr(textureNameByteLength);

			file.seekg(18, std::ios::cur); // Seek unnecessary bytes

			uint32_t textureX = binaryReader.readU32();
			uint32_t textureY = binaryReader.readU32();

			file.seekg(28, std::ios::cur); // Seek unnecessary bytes

			std::vector<uint8_t> bgr(textureX * textureY * 3);
			file.read(reinterpret_cast<char*>(bgr.data()), bgr.size());

			std::vector<uint8_t> rgb(textureX * textureY * 3);
			for (uint32_t y = 0; y < textureY; y++)
			{
				memcpy(&rgb[y * textureX * 3], &bgr[(textureY - 1 - y) * textureX * 3], textureX * 3);
				// Also swap B<->R if needed
				for (uint32_t x = 0; x < textureX; x++)
				{
					std::swap(rgb[(y * textureX + x) * 3 + 0], rgb[(y * textureX + x) * 3 + 2]);
				}
			}

			GLuint texID = textureIDs[i];
			glBindTexture(GL_TEXTURE_2D, texID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureX, textureY, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
			textures[i].name = textureName;
			textures[i].image = texID;
			textures[i].x = textureX;
			textures[i].y = textureY;
		}

		std::println("~Extracted {} textures in {}ms", textures.size(), timer.returnTime());
	}
}
