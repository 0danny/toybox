#include <fstream>

#include "Parsers/BinaryReader.hpp"
#include "Common.hpp"
#include "FileUtils.hpp"

BinaryReader::BinaryReader(std::ifstream& file)
	: file(file)
{}

size_t BinaryReader::getFileSize()
{
	std::streampos oldPos = file.tellg();

	file.seekg(0, std::ios::end);
	std::streampos endPos = file.tellg();

	file.seekg(oldPos, std::ios::beg);

	if (endPos < 0)
		throw std::runtime_error("file size NULL");

	return static_cast<size_t>(endPos);
}

void BinaryReader::requireRange(size_t fileSize, size_t off, size_t len) // make sure that the file read isnt about to breach EOF/SOF
{
	if (off + len < off || off + len > fileSize)
		throw std::runtime_error("EOF");
}

void BinaryReader::seekTo(std::ifstream& file, size_t off, size_t fileSize) // move cur to specific byte, if we can
{
	requireRange(fileSize, off, 0);
	file.seekg(static_cast<std::streamoff>(off), std::ios::beg);

	if (! file)
		throw std::runtime_error("seek no work");
}

template <typename T>
T BinaryReader::readValue(std::streamsize bytes)
{
	T value {};
	file.read(reinterpret_cast<char*>(&value), bytes);

	if (! file)
		throw std::runtime_error("read no work");

	return value;
}

std::string BinaryReader::readStr(uint8_t byteCount)
{ return readValue<std::string>(byteCount); }

uint8_t BinaryReader::readU8()
{ return readValue<uint8_t>(readUInt8); }

int16_t BinaryReader::readS16()
{ return readValue<int16_t>(readInt16); }

uint16_t BinaryReader::readU16()
{ return readValue<uint32_t>(readUInt16); }

uint32_t BinaryReader::readU32()
{ return readValue<uint32_t>(readUInt32); }

int32_t BinaryReader::readS32()
{ return readValue<int32_t>(readInt32); }

uint8_t BinaryReader::readU8At(size_t off, size_t fileSize)
{
	requireRange(fileSize, off, sizeof(uint8_t));
	seekTo(file, off, fileSize);
	return BinaryReader::readU8();
}

int16_t BinaryReader::readS16At(size_t off, size_t fileSize)
{
	requireRange(fileSize, off, sizeof(int16_t));
	seekTo(file, off, fileSize);
	return readS16();
}

uint32_t BinaryReader::readU32At(size_t off, size_t fileSize)
{
	requireRange(fileSize, off, sizeof(uint32_t));
	seekTo(file, off, fileSize);
	return readU32();
}

int32_t BinaryReader::readS32At(size_t off, size_t fileSize)
{
	requireRange(fileSize, off, sizeof(int32_t));
	seekTo(file, off, fileSize);
	return readS32();
}

Vector3S BinaryReader::readVec3sAt(size_t off, size_t fileSize)
{ return Vector3S { readS16At(off + 0, fileSize), readS16At(off + 2, fileSize), readS16At(off + 4, fileSize) }; }

Vector3S BinaryReader::addVec3s(Vector3S a, Vector3S b)
{ return Vector3S { static_cast<int16_t>(a.x + b.x), static_cast<int16_t>(a.y + b.y), static_cast<int16_t>(a.z + b.z) }; }