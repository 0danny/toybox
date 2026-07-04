#include <fstream>
#include <chrono>
#include <GLFW/glfw3.h>
#include <print>
#include <cstdlib>
#include "NGN.hpp"

const std::streamsize readUInt32 = 4;
const std::streamsize readUInt16 = 2;
const std::streamsize readUInt8 = 1;

std::vector<Texture> returnImages();

std::vector<Shape> Shapes;

std::vector<Shape> returnShapes() {
	return Shapes;
}

std::string getShapeName(std::ifstream& file) {
	uint32_t subBlock;
	file.read(reinterpret_cast<char*>(&subBlock), readUInt32);
	if (subBlock != uint32_t(64)) {
		std::println("SUBBLOCK MISALIGNED");
		exit(EXIT_FAILURE);
	}
	file.seekg(4, std::ios::cur); // skips subBlockLength
	uint8_t shapeNameByteCount;
	file.read(reinterpret_cast<char*>(&shapeNameByteCount), readUInt8);
	std::string shapeName(shapeNameByteCount, '\0');
	file.read(&shapeName[0], shapeNameByteCount);
	return shapeName;
}

std::vector<std::string> getTextureNames(std::ifstream& file) {
	uint32_t subBlock;
	file.read(reinterpret_cast<char*>(&subBlock), readUInt32);
	if (subBlock != uint32_t(65)) {
		std::println("SUBBLOCK MISALIGNED");
		exit(EXIT_FAILURE);
	}
	std::vector<std::string> textureNames;
	uint32_t subBlockLength;
	file.read(reinterpret_cast<char*>(&subBlockLength), readUInt32);
	std::streampos offset = file.tellg();
	uint16_t textureCount;
	file.read(reinterpret_cast<char*>(&textureCount), readUInt16);
	textureNames.resize(textureCount);

	file.seekg(4, std::ios::cur); // skips unknownBytes
	for (uint16_t i = 0; i < textureCount; i++) {
		uint8_t texNameByteCount;
		file.read(reinterpret_cast<char*>(&texNameByteCount), readUInt8);
		std::string texName(texNameByteCount, '\0');
		file.read(&texName[0], texNameByteCount);
		textureNames[i] = texName;
	}
	file.seekg(offset, std::ios::beg); // seek to offset
	file.seekg(subBlockLength, std::ios::cur); // seek past subBlock (we do this to skip any amount of padding)
	return textureNames;
}

std::vector<int> getMaterialIndexes(std::ifstream& file) {
	uint32_t subBlock;
	file.read(reinterpret_cast<char*>(&subBlock), readUInt32);
	if (subBlock != uint32_t(66)) {
		std::println("SUBBLOCK MISALIGNED");
		exit(EXIT_FAILURE);
	}
	std::vector<int> materialIndexes;
	file.seekg(4, std::ios::cur); // skips subBlockLength
	uint32_t materialCount;
	file.read(reinterpret_cast<char*>(&materialCount), readUInt32);
	materialIndexes.resize(materialCount);

	for (uint32_t i = 0; i < materialCount; i++) {
		file.seekg(4, std::ios::cur); // skips materialId
		uint32_t materialSize;
		file.read(reinterpret_cast<char*>(&materialSize), readUInt32);
		file.seekg(3, std::ios::cur); // skips materialRGB
		if (materialSize == uint32_t(9))
			file.seekg(4, std::ios::cur); // skips materialId if size = 9
		uint16_t textureIndex;
		file.read(reinterpret_cast<char*>(&textureIndex), readUInt16);
		if (textureIndex == uint16_t(65536))
			materialIndexes[i] = textureIndex;
		else
			materialIndexes[i] = -1;
	}

	return materialIndexes;
}

std::vector<Vertex> getRawVertexData(std::ifstream& file) {
	uint32_t subBlock;
	file.read(reinterpret_cast<char*>(&subBlock), readUInt32);
	if (subBlock != uint32_t(67)) {
		std::println("SUBBLOCK MISALIGNED");
		exit(EXIT_FAILURE);
	}
	std::vector<Vertex> rawVertexData;
	file.seekg(12, std::ios::cur); // skips subBlockLength, v17 and vertexDataLength
	uint32_t vertexCount;
	file.read(reinterpret_cast<char*>(&vertexCount), readUInt32);
	rawVertexData.resize(vertexCount);
	for (uint32_t i = 0; i < vertexCount; i++) {
		Vertex currentVertex = rawVertexData[i];
		file.read(reinterpret_cast<char*>(&currentVertex.x), readUInt32);
		file.read(reinterpret_cast<char*>(&currentVertex.y), readUInt32);
		file.read(reinterpret_cast<char*>(&currentVertex.z), readUInt32);
		file.read(reinterpret_cast<char*>(&currentVertex.nx), readUInt32);
		file.read(reinterpret_cast<char*>(&currentVertex.ny), readUInt32);
		file.read(reinterpret_cast<char*>(&currentVertex.nz), readUInt32);
		file.read(reinterpret_cast<char*>(&currentVertex.a), readUInt8);
		file.read(reinterpret_cast<char*>(&currentVertex.r), readUInt8);
		file.read(reinterpret_cast<char*>(&currentVertex.g), readUInt8);
		file.read(reinterpret_cast<char*>(&currentVertex.b), readUInt8);
		file.read(reinterpret_cast<char*>(&currentVertex.u), readUInt32);
		file.read(reinterpret_cast<char*>(&currentVertex.v), readUInt32);
	}
	return rawVertexData;
}

std::vector<Prim> getPrimitiveData(std::ifstream& file, Shape currentShape) {
	uint32_t subBlock;
	file.read(reinterpret_cast<char*>(&subBlock), readUInt32);
	if (subBlock != uint32_t(68)) {
		std::println("SUBBLOCK MISALIGNED");
		exit(EXIT_FAILURE);
	}
	file.seekg(4, std::ios::cur); // skips subBlockLength
	uint32_t primitiveCount;
	file.read(reinterpret_cast<char*>(&primitiveCount), readUInt32);

	std::vector<Prim> primitives;
	primitives.resize(primitiveCount);

	for (uint32_t i = 0; i < primitiveCount; i++) {
		Prim currentPrimitive = primitives[i];

		uint32_t primitiveType;
		file.read(reinterpret_cast<char*>(&primitiveType), readUInt32);
		uint16_t materialIndex;
		file.read(reinterpret_cast<char*>(&materialIndex), readUInt16);

		if (currentShape.materialIndex[materialIndex] != -1) { // if a texture is to be found, link it to the prim, else set it to NULL
			std::string target = currentShape.textureIndex[currentShape.materialIndex[materialIndex]];
			std::vector<Texture> images = returnImages();
			auto it = std::find_if(images.begin(), images.end(),
				[target](const Texture& s) {
					return s.name == target;
				});
			currentPrimitive.textureId = it->image;
		}
		else
			currentPrimitive.textureId = NULL;

		uint16_t vertexCount;
		file.read(reinterpret_cast<char*>(&vertexCount), readUInt16);

		std::vector<uint16_t> indices(vertexCount);

		for (uint16_t n = 0; n < vertexCount; n++) {
			uint16_t vertexNum;
			file.read(reinterpret_cast<char*>(&vertexNum), readUInt16);
			indices[n] = vertexNum;
		}

		if (primitiveType != 4) {
			currentPrimitive.indices = indices;
		}
		else if (primitiveType == 4){
			//if (vertexCount % 4 == 0)
				//std::println("yeaaah idk if this is a quad lil bro");
			std::vector<uint16_t> triangleIndices;
			for (size_t q = 0; q + 3 < indices.size(); q += 4) {
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



void readGeomBlock(std::ifstream& file, bool LOD) {
	if (LOD == false) {
		auto start_time = std::chrono::high_resolution_clock::now();

		uint32_t shapeCount;
		file.read(reinterpret_cast<char*>(&shapeCount), readUInt32);

		Shapes.resize(shapeCount);

		std::println("~Shapes: {}", shapeCount);

		for (uint32_t i = 0; i < shapeCount; i++) {
			Shape currentShape = Shapes[i];															// Set currentShape
			//std::println("currentShape set (Shape {})",i);
			currentShape.shapeName = getShapeName(file);											// Get shapeName
			//std::println("Shape Passed ({})",currentShape.shapeName);
			currentShape.textureIndex = getTextureNames(file);										// Get textureNames
			//std::println("Textures Passed ({} Texture Names)",currentShape.textureIndex.size());
			currentShape.materialIndex = getMaterialIndexes(file);									// Get materialIndexes
			//std::println("Materials Passed ({} Material Indexes)",currentShape.materialIndex.size());
			currentShape.vertices = getRawVertexData(file);											// Get rawVertexData
			//std::println("Vertex Passed ({} Verticies)", currentShape.vertices.size());
			currentShape.primitives = getPrimitiveData(file, currentShape);							// Get primData
			//std::println("Primitives Passed ({} Prims)",currentShape.primitives.size());
		}
		std::println("~Shapes Indexed: {}", Shapes.size());

		auto end_time = std::chrono::high_resolution_clock::now();
		std::string txt = "";
		if (LOD == true)
			txt = " (LOD)";
		std::println("~Took {}ms to parse geometry{}", std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count(), txt);
	}
	else {
		std::println("~LOD Geometry Skipped!");
	}
}