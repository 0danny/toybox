#pragma once
#include "common.h"
#include <vector>
#include <filesystem>

struct rawTexture
{
	GLuint image;
	uint32_t width = 0;
	uint32_t height = 0;
	std::vector<uint8_t> rgba;
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
	std::vector<rawTexture> texPackets;
	std::vector<RawPacket> anmPackets;
	std::vector<RawPacket> allPackets;
	std::vector<CreatureRam> creatures;
};