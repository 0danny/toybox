#pragma once
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <filesystem>

struct Texture {
	std::string name;
    GLuint image;
	int x;
	int y;
};

struct Vertex {
	float x, y, z;       // Position
	float nx, ny, nz;    // Normal
	float u, v;          // UV
	float r, g, b, a;    // Color
};

struct Prim {
	std::vector<uint16_t> indices;
	GLuint vao, vbo, ebo;
	GLuint textureId; // set
};

struct Shape {
	std::string shapeName; // set
	std::vector<std::string> textureIndex; // set
	std::vector<int> materialIndex; // set
	std::vector<Vertex> vertices; // set
	std::vector<Prim> primitives; // "set"
};

struct Vector3I
{
	int32_t x;
	int32_t y;
	int32_t z;

	void print()
	{
		printf("[%d, %d, %d]\n", x, y, z);
	}
};

struct EntityControl
{
	uint8_t actorPhase;
	uint8_t unkByte;
	uint16_t actorFlags;
};

struct CreatureRam
{
	Vector3I pos;
	uint8_t creatureId;
	uint8_t movCtrl;
	uint8_t rotSpeed;
	uint8_t initialFacingAngle;
	EntityControl entCtrl;
	int16_t boundHalfX;
	int16_t boundHalfZ;
	int16_t boundAngle;
	int16_t defenseMode;
	uint8_t latSpeedNoTarget;
	uint8_t latSpeedTarget;
	uint8_t speedNoTarget;
	uint8_t speedTarget;
};

struct RawPacket
{
	std::filesystem::path sourcePath;
	uint32_t packetIndex = 0;
	uint32_t compressedSize = 0;
	uint32_t uncompressedSize = 0;
	std::vector<uint8_t> data;
};

struct RawReadResult
{
	bool success = true;
	std::vector<RawPacket> rawPackets;
	std::vector<RawPacket> anmPackets;
	std::vector<RawPacket> allPackets;
	std::vector<CreatureRam> creatures;
};