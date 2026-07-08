#include "Parsers/BinaryReader.hpp"
#include "Parsers/GeomParser.hpp"
#include "Parsers/NGNParser.hpp"
#include "Parsers/TextureParser.hpp"

#include "Clock.hpp"

#include <iostream>
#include <print>
#include <cstdlib>

namespace GeomParser
{
	std::vector<NGNParser::Shape> Shapes;

	std::vector<NGNParser::Shape> returnShapes()
	{ return Shapes; }

	std::string getShapeName(std::ifstream& file)
	{
		BinaryReader binaryReader(file);

		uint32_t subBlock = binaryReader.readU32();

		if (subBlock != uint32_t(64))
		{
			std::println("SHAPENAME SUBBLOCK MISALIGNED");
			exit(EXIT_FAILURE);
		}

		file.seekg(4, std::ios::cur); // skips subBlockLength

		uint8_t shapeNameByteCount = binaryReader.readU8();

		std::string shapeName = binaryReader.readStr(shapeNameByteCount);

		return shapeName;
	}

	std::vector<std::string> getTextureNames(std::ifstream& file)
	{
		BinaryReader binaryReader(file);

		uint32_t subBlock = binaryReader.readU32();

		if (subBlock != uint32_t(65))
		{
			std::println("TEX SUBBLOCK MISALIGNED");
			exit(EXIT_FAILURE);
		}

		std::vector<std::string> textureNames;
		uint32_t subBlockLength = binaryReader.readU32();

		std::streampos offset = file.tellg();
		uint16_t textureCount = binaryReader.readU16();

		textureNames.resize(textureCount);

		file.seekg(4, std::ios::cur); // skips unknownBytes
		for (uint16_t i = 0; i < textureCount; i++)
		{
			uint8_t texNameByteCount = binaryReader.readU8();
			std::string texName = binaryReader.readStr(texNameByteCount);
			textureNames[i] = texName;
		}

		file.seekg(offset, std::ios::beg); // seek to offset
		file.seekg(subBlockLength, std::ios::cur); // seek past subBlock (we do this to skip any amount of padding)

		return textureNames;
	}

	std::vector<int> getMaterialIndexes(std::ifstream& file)
	{
		BinaryReader binaryReader(file);

		uint32_t subBlock = binaryReader.readU32();

		if (subBlock != uint32_t(66))
		{
			std::println("MAT SUBBLOCK MISALIGNED");
			exit(EXIT_FAILURE);
		}

		file.seekg(4, std::ios::cur); // skips subBlockLength
		uint32_t materialCount = binaryReader.readU32();
		std::vector<int> materialIndexes(materialCount);

		for (uint32_t i = 0; i < materialCount; i++)
		{
			file.seekg(4, std::ios::cur); // skips materialId
			uint32_t materialSize = binaryReader.readU32();

			if (materialSize == uint32_t(9))
				file.seekg(4, std::ios::cur); // skips materialId again if size = 9

			file.seekg(3, std::ios::cur); // skips materialRGB
			uint16_t textureIndex = binaryReader.readU16();

			if (textureIndex == uint16_t(65536)) // no material if = 65536
				materialIndexes[i] = textureIndex;
			else
				materialIndexes[i] = -1;
		}

		return materialIndexes;
	}

	std::vector<NGNParser::Vertex> getRawVertexData(std::ifstream& file)
	{
		BinaryReader binaryReader(file);

		uint32_t subBlock = binaryReader.readU32();

		if (subBlock != uint32_t(67))
		{
			std::println("VERT SUBBLOCK MISALIGNED");
			exit(EXIT_FAILURE);
		}

		file.seekg(12, std::ios::cur); // skips subBlockLength, v17 and vertexDataLength

		uint32_t vertexCount = binaryReader.readU32();
		std::vector<NGNParser::Vertex> rawVertexData(vertexCount);

		for (uint32_t i = 0; i < vertexCount; i++)
		{
			NGNParser::Vertex currentVertex = rawVertexData[i];
			currentVertex.x = binaryReader.readU32();
			currentVertex.y = binaryReader.readU32();
			currentVertex.z = binaryReader.readU32();
			currentVertex.nx = binaryReader.readU32();
			currentVertex.nx = binaryReader.readU32();
			currentVertex.nx = binaryReader.readU32();
			currentVertex.a = binaryReader.readU8();
			currentVertex.r = binaryReader.readU8();
			currentVertex.g = binaryReader.readU8();
			currentVertex.b = binaryReader.readU8();
			currentVertex.u = binaryReader.readU32();
			currentVertex.v = binaryReader.readU32();
		}
		return rawVertexData;
	}

	std::vector<NGNParser::Prim> getPrimitiveData(std::ifstream& file, NGNParser::Shape currentShape)
	{
		BinaryReader binaryReader(file);

		uint32_t subBlock = binaryReader.readU32();

		if (subBlock != uint32_t(68))
		{
			std::println("PRIM SUBBLOCK MISALIGNED");
			exit(EXIT_FAILURE);
		}

		file.seekg(4, std::ios::cur); // skips subBlockLength
		uint32_t primitiveCount = binaryReader.readU32();

		std::vector<NGNParser::Prim> primitives(primitiveCount);

		for (uint32_t i = 0; i < primitiveCount; i++)
		{
			NGNParser::Prim currentPrimitive = primitives[i];

			uint32_t primitiveType = binaryReader.readU32();
			uint16_t materialIndex = binaryReader.readU16();

			if (currentShape.materialIndex[materialIndex] != -1)
			{ // if a texture is to be found, link it to the prim, else set it to NULL
				std::string target = currentShape.textureIndex[currentShape.materialIndex[materialIndex]];
				std::vector<NGNParser::Texture> images = TextureParser::ReturnImages();
				auto it = std::find_if(images.begin(), images.end(), [target](const NGNParser::Texture& s) { return s.name == target; });
				currentPrimitive.textureId = it->image;
			}
			else
				currentPrimitive.textureId = NULL;

			uint16_t vertexCount = binaryReader.readU16();

			std::vector<uint16_t> indices(vertexCount);

			for (uint16_t n = 0; n < vertexCount; n++)
			{
				uint16_t vertexNum = binaryReader.readU16();
				indices[n] = vertexNum;
			}

			if (primitiveType != 4)
			{
				currentPrimitive.indices = indices;
			}
			else if (primitiveType == 4)
			{
				// if (vertexCount % 4 == 0)
				// std::println("yeaaah idk if this is a quad lil bro");
				std::vector<uint16_t> triangleIndices;
				for (size_t q = 0; q + 3 < indices.size(); q += 4)
				{
					uint16_t a = indices[q];
					uint16_t b = indices[q + 1];
					uint16_t c = indices[q + 2];
					uint16_t d = indices[q + 3];

					// First triangle: a, b, c
					triangleIndices.push_back(a);
					triangleIndices.push_back(b);
					triangleIndices.push_back(c);

					// Second triangle: a, c, d
					triangleIndices.push_back(a);
					triangleIndices.push_back(c);
					triangleIndices.push_back(d);
				}
				currentPrimitive.indices = triangleIndices;
			}
		}
		file.seekg(8, std::ios::cur); // skips end of block padding
		return primitives;
	}

	void ReadGeomBlock(std::ifstream& file, bool LOD)
	{
		if (LOD == false)
		{
			BinaryReader binaryReader(file);

			Timer timer;

			uint32_t shapeCount = binaryReader.readU32();

			Shapes.resize(shapeCount);

			std::println("~Shapes: {}", shapeCount);

			for (uint32_t i = 0; i < shapeCount; i++)
			{
				NGNParser::Shape currentShape = Shapes[i]; // Set currentShape
				// std::println("currentShape set (Shape {})",i);
				currentShape.shapeName = getShapeName(file); // Get shapeName
				// std::println("Shape Passed ({})",currentShape.shapeName);
				currentShape.textureIndex = getTextureNames(file); // Get textureNames
				// std::println("Textures Passed ({} Texture Names)",currentShape.textureIndex.size());
				currentShape.materialIndex = getMaterialIndexes(file); // Get materialIndexes
				// std::println("Materials Passed ({} Material Indexes)",currentShape.materialIndex.size());
				currentShape.vertices = getRawVertexData(file); // Get rawVertexData
				// std::println("Vertex Passed ({} Verticies)", currentShape.vertices.size());
				currentShape.primitives = getPrimitiveData(file, currentShape); // Get primData
				// std::println("Primitives Passed ({} Prims)",currentShape.primitives.size());
			}
			std::println("~Shapes Indexed: {}", Shapes.size());

			std::string txt = "";
			if (LOD == true)
				txt = " (LOD)";
			std::println("~Took {}ms to parse geometry{}", timer.returnTime(), txt);
		}
		else
		{
			std::println("~LOD Geometry Skipped!");
		}
	}
}
