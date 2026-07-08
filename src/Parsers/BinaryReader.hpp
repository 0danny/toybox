#pragma once

#include <fstream>
#include "Common.hpp"

class BinaryReader
{
public:
	BinaryReader(std::ifstream& file);

	template <typename T>
	T readValue(std::streamsize bytes);

	size_t getFileSize();
	void requireRange(size_t fileSize, size_t off, size_t len);
	void seekTo(std::ifstream& file, size_t off, size_t fileSize);

	std::string readStr(uint8_t byteCount);

	uint8_t readU8();
	int16_t readS16();
	uint16_t readU16();
	uint32_t readU32();
	int32_t readS32();

	uint8_t readU8At(size_t off, size_t fileSize);
	int16_t readS16At(size_t off, size_t fileSize);
	uint32_t readU32At(size_t off, size_t fileSize);
	int32_t readS32At(size_t off, size_t fileSize);

	Vector3S readVec3sAt(size_t off, size_t fileSize);
	Vector3S addVec3s(Vector3S a, Vector3S b);

private:
	std::ifstream& file;
	const std::streamsize readUInt32 = 4;
	const std::streamsize readInt32 = 4;
	const std::streamsize readUInt16 = 2;
	const std::streamsize readInt16 = 2;
	const std::streamsize readUInt8 = 1;
};