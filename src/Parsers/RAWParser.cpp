/*
	----------------------------------------------------
	Toy Story 2 .raw decompressor written by Danny
	3/05/2026
	Decompiled from original game source at 0x0047B170
	----------------------------------------------------
*/

#include "Parsers/RAWParser.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <vector>
#include <GLFW/glfw3.h>

namespace RawParser
{
	constexpr int32_t kMaxActors = 64;
	namespace fs = std::filesystem;

#define LOBYTE_RD(x) (*((uint8_t*)&(x)))
#define HIBYTE_RD(x) (*((uint8_t*)&(x) + 1))

#define LOWORD_RD(x) (*((uint16_t*)&(x)))
#define HIWORD_RD(x) (*((uint16_t*)&(x) + 1))

	// Original function -> Toy2.exe -> 0x0047B170
	void DecompressBuffer(uint8_t* p_inBuffer, uint8_t* p_outBuffer)
	{
		uint32_t copyLength;
		uint32_t exitFlag;
		uint32_t lengthCode;
		uint32_t distanceHighOrLiteralCount;
		uint32_t extendedLengthBit;
		uint32_t lengthFollowupBit;
		uint32_t hasDistanceHighBits;

		uint8_t* afterHeader = p_inBuffer + 15;
		uint32_t bitAccum = 4 * p_inBuffer[14] + 2;

		auto copyLiteral4 = [&]() {
			p_outBuffer[0] = afterHeader[0];
			p_outBuffer[1] = afterHeader[1];
			p_outBuffer[2] = afterHeader[2];
			p_outBuffer[3] = afterHeader[3];

			p_outBuffer += 4;
			afterHeader += 4;
		};

		do
		{
			while (true)
			{
				while (true)
				{
					while (true)
					{
						bitAccum *= 2;
						uint32_t bitTest = (bitAccum >> 8) & 1;

						if (! bitTest)
						{
							bitAccum *= 2;

							*p_outBuffer++ = *afterHeader++;

							bitTest = (bitAccum >> 8) & 1;
						}

						if (bitTest)
						{
							bitAccum &= 0xFF;

							if (bitAccum)
								break;

							bitAccum = bitTest + 2 * (*afterHeader++);

							if ((bitAccum & 0x100) != 0)
								break;
						}

						*p_outBuffer++ = *afterHeader++;
					}

					LOBYTE_RD(lengthCode) = 2 * bitAccum;
					distanceHighOrLiteralCount = 0;
					copyLength = 2;

					uint32_t lengthCodeBit = ((2 * bitAccum) >> 8) & 1;
					lengthCode &= 0xFF;

					if (! lengthCode)
					{
						lengthCode = lengthCodeBit + 2 * (*afterHeader++);
						lengthCodeBit = (lengthCode >> 8) & 1;
					}

					if (lengthCodeBit)
						break;

					uint32_t shiftedLengthCode = 2 * lengthCode;
					uint32_t shiftedLengthBit = (shiftedLengthCode >> 8) & 1;
					shiftedLengthCode &= 0xFF;

					if (! shiftedLengthCode)
					{
						LOBYTE_RD(lengthCodeBit) = (*afterHeader++);
						shiftedLengthCode = shiftedLengthBit + 2 * lengthCodeBit;
						shiftedLengthBit = (shiftedLengthCode >> 8) & 1;
					}

					LOBYTE_RD(bitAccum) = 2 * shiftedLengthCode;
					copyLength = shiftedLengthBit + 4;

					uint32_t extraLengthBit = ((2 * shiftedLengthCode) >> 8) & 1;

					bitAccum &= 0xFF;

					if (! bitAccum)
					{
						bitAccum = extraLengthBit + 2 * (*afterHeader++);
						extraLengthBit = (bitAccum >> 8) & 1;
					}

					if (! extraLengthBit)
						goto LBL_DECODE_DISTANCE;

					bitAccum *= 2;
					uint32_t finalLengthBit = (bitAccum >> 8) & 1;
					bitAccum &= 0xFF;

					if (! bitAccum)
					{
						bitAccum = finalLengthBit + 2 * (*afterHeader++);
						finalLengthBit = (bitAccum >> 8) & 1;
					}

					copyLength = finalLengthBit + 2 * (copyLength - 1);

					if (copyLength != 9)
						goto LBL_DECODE_DISTANCE;

					for (int32_t bitIndex = 3; bitIndex >= 0; --bitIndex)
					{
						bitAccum *= 2;
						uint32_t literalCountBit = (bitAccum >> 8) & 1;
						bitAccum &= 0xFF;

						if (! bitAccum)
						{
							bitAccum = literalCountBit + 2 * (*afterHeader++);
							literalCountBit = (bitAccum >> 8) & 1;
						}

						distanceHighOrLiteralCount = literalCountBit + 2 * distanceHighOrLiteralCount;
					}

					copyLiteral4();

					uint32_t literalBlockCount = distanceHighOrLiteralCount + 1;
					uint32_t literalBlocksRemaining = literalBlockCount + 1;

					do
					{
						copyLiteral4();
						--literalBlocksRemaining;
					} while (literalBlocksRemaining);
				}

				LOBYTE_RD(bitAccum) = 2 * lengthCode;
				lengthFollowupBit = ((2 * lengthCode) >> 8) & 1;
				bitAccum &= 0xFF;

				if (! bitAccum)
				{
					bitAccum = lengthFollowupBit + 2 * (*afterHeader++);
					lengthFollowupBit = (bitAccum >> 8) & 1;
				}

				if (! lengthFollowupBit)
					goto LBL_COPY_MATCH;

				bitAccum *= 2;
				copyLength = 3;

				extendedLengthBit = (bitAccum >> 8) & 1;
				bitAccum &= 0xFF;

				if (! bitAccum)
				{
					bitAccum = extendedLengthBit + 2 * (*afterHeader++);
					extendedLengthBit = (bitAccum >> 8) & 1;
				}

				if (extendedLengthBit)
					break;

			LBL_DECODE_DISTANCE:

				bitAccum *= 2;
				hasDistanceHighBits = (bitAccum >> 8) & 1;
				bitAccum &= 0xFF;

				if (! bitAccum)
				{
					bitAccum = hasDistanceHighBits + 2 * (*afterHeader++);
					hasDistanceHighBits = (bitAccum >> 8) & 1;
				}

				if (hasDistanceHighBits)
				{
					uint32_t distancePrefixAccum = 2 * bitAccum;
					uint32_t distancePrefixBit = (distancePrefixAccum >> 8) & 1;

					distancePrefixAccum &= 0xFF;

					if (! distancePrefixAccum)
					{
						distancePrefixAccum = distancePrefixBit + 2 * (*afterHeader++);
						distancePrefixBit = (distancePrefixAccum >> 8) & 1;
					}

					bitAccum = 2 * distancePrefixAccum;
					uint32_t distanceHighBits = distancePrefixBit;

					uint32_t hasMoreDistanceBits = (bitAccum >> 8) & 1;
					bitAccum &= 0xFF;

					if (! bitAccum)
					{
						bitAccum = hasMoreDistanceBits + 2 * (*afterHeader++);
						hasMoreDistanceBits = (bitAccum >> 8) & 1;
					}

					if (hasMoreDistanceBits)
					{
						uint32_t shifted = 2 * bitAccum;
						uint32_t distanceExtraBit = (shifted >> 8) & 1;
						uint32_t distanceExtraAccum = shifted & 0xFF;

						if (! distanceExtraAccum)
						{
							distanceExtraAccum = distanceExtraBit + 2 * (*afterHeader++);
							distanceExtraBit = (distanceExtraAccum >> 8) & 1;
						}

						distanceHighBits = (distanceExtraBit + 2 * distanceHighBits) | 4;

						shifted = 2 * distanceExtraAccum;
						uint32_t distanceTerminatorBit = (shifted >> 8) & 1;
						bitAccum = shifted & 0xFF;

						if (! bitAccum)
						{
							bitAccum = distanceTerminatorBit + 2 * (*afterHeader++);
							distanceTerminatorBit = (bitAccum >> 8) & 1;
						}

						if (distanceTerminatorBit)
							goto LBL_COMMIT_DISTANCE;
					}
					else
					{
						if (distanceHighBits)
						{
						LBL_COMMIT_DISTANCE:

							int16_t distanceHighWord = 0;
							HIBYTE_RD(distanceHighWord) = distanceHighBits;
							LOWORD_RD(distanceHighOrLiteralCount) = distanceHighWord | (distanceHighBits >> 8);

							goto LBL_COPY_MATCH;
						}

						distanceHighBits = 1;
					}

					bitAccum *= 2;
					uint32_t distanceTailBit = (bitAccum >> 8) & 1;
					bitAccum &= 0xFF;

					if (! bitAccum)
					{
						bitAccum = distanceTailBit + 2 * (*afterHeader++);
						distanceTailBit = (bitAccum >> 8) & 1;
					}

					distanceHighBits = distanceTailBit + 2 * distanceHighBits;
					goto LBL_COMMIT_DISTANCE;
				}

			LBL_COPY_MATCH:

				int32_t matchDistance = (distanceHighOrLiteralCount & 0xFF00) | (*afterHeader++);
				uint8_t* matchSource = &p_outBuffer[-matchDistance - 1];

				if (copyLength & 1)
				{
					*p_outBuffer++ = *matchSource++;
				}

				int32_t pairCopyCount = (copyLength >> 1) - 1;

				if (matchDistance)
				{
					p_outBuffer += 2;
					uint16_t* matchSourceWords = (uint16_t*)(matchSource + 2);

					*(p_outBuffer - 1) = *((uint8_t*)matchSourceWords - 1);
					*(p_outBuffer - 2) = *matchSource;

					int32_t wordCopyLoopCount = pairCopyCount - 1;

					if (wordCopyLoopCount >= 0)
					{
						uint32_t wordCopiesRemaining = wordCopyLoopCount + 1;

						do
						{
							*(uint16_t*)p_outBuffer = *matchSourceWords++;
							p_outBuffer += 2;
							--wordCopiesRemaining;
						} while (wordCopiesRemaining);
					}
				}
				else
				{
					p_outBuffer += 2;
					uint8_t repeatedByte = *matchSource;
					int32_t repeatPairLoopCount = pairCopyCount - 1;

					*(p_outBuffer - 2) = repeatedByte;
					*(p_outBuffer - 1) = repeatedByte;

					if (repeatPairLoopCount >= 0)
					{
						int32_t repeatPairsRemaining = repeatPairLoopCount + 1;

						do
						{
							*p_outBuffer = repeatedByte;
							p_outBuffer[1] = repeatedByte;
							p_outBuffer += 2;
							--repeatPairsRemaining;
						} while (repeatPairsRemaining);
					}
				}
			}

			int32_t extendedLengthByte = (*afterHeader++);

			if (extendedLengthByte)
			{
				copyLength = extendedLengthByte + 8;
				goto LBL_DECODE_DISTANCE;
			}

			bitAccum *= 2;
			exitFlag = (bitAccum >> 8) & 1;
			bitAccum &= 0xFF;

			if (! bitAccum)
			{
				bitAccum = exitFlag + 2 * (*afterHeader++);
				exitFlag = (bitAccum >> 8) & 1;
			}
		} while (exitFlag);
	}

	static uint32_t toBigEndian(uint32_t num)
	{
		return ((num & 0x000000ff) << 24) | ((num & 0x0000ff00) << 8) | ((num & 0x00ff0000) >> 8) | ((num & 0xff000000) >> 24);
	}

	struct GLTexture
	{
		GLuint id = 0;
		int width = 0;
		int height = 0;
	};

	static GLuint createGLTex(const rawTexture& image)
	{
		if (image.rgba.empty() || image.width == 0 || image.height == 0)
			return 0;

		GLuint textureID;
		glGenTextures(1, &textureID);

		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(image.width), static_cast<GLsizei>(image.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());

		glBindTexture(GL_TEXTURE_2D, 0);

		return textureID;
	}

	static uint32_t readU32LE(const std::vector<uint8_t>& data, size_t offset)
	{
		if (offset + 3 >= data.size())
			return 0;

		return static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1]) << 8) | (static_cast<uint32_t>(data[offset + 2]) << 16)
			| (static_cast<uint32_t>(data[offset + 3]) << 24);
	}

	static uint16_t readU16LE(const std::vector<uint8_t>& data, size_t offset)
	{
		if (offset + 1 >= data.size())
			return 0;

		return static_cast<uint16_t>(data[offset]) | (static_cast<uint16_t>(data[offset + 1]) << 8);
	}

	static uint8_t expand5To8(uint8_t value)
	{
		return static_cast<uint8_t>((value << 3) | (value >> 2));
	}

	static rawTexture convertRGB555PacketToImage(const RawPacket& packet)
	{
		rawTexture image;

		if (packet.data.size() < 16)
			return image;

		const uint32_t width = readU32LE(packet.data, 4);
		const uint32_t height = readU32LE(packet.data, 8);

		if (width == 0 || height == 0)
			return image;

		constexpr size_t headerSize = 16;
		const size_t pixelCount = static_cast<size_t>(width) * height;
		const size_t neededSize = headerSize + pixelCount * 2;

		if (packet.data.size() < neededSize)
			return image;

		const uint8_t* pixels = packet.data.data() + headerSize;

		image.width = width;
		image.height = height;
		image.rgba.resize(pixelCount * 4);

		for (size_t i = 0; i < pixelCount; i++)
		{
			const uint16_t color = static_cast<uint16_t>(pixels[i * 2]) | (static_cast<uint16_t>(pixels[i * 2 + 1]) << 8);

			const uint8_t r5 = color & 0x1F;
			const uint8_t g5 = (color >> 5) & 0x1F;
			const uint8_t b5 = (color >> 10) & 0x1F;

			const size_t out = i * 4;

			image.rgba[out + 0] = expand5To8(r5);
			image.rgba[out + 1] = expand5To8(g5);
			image.rgba[out + 2] = expand5To8(b5);
			image.rgba[out + 3] = 255;
		}

		return image;
	}

	static bool isRGB555Packet(const RawPacket& packet)
	{
		if (packet.data.size() < 16)
			return false;

		const uint32_t width = readU32LE(packet.data, 4);
		const uint32_t height = readU32LE(packet.data, 8);

		if (width == 0 || height == 0)
			return false;

		const size_t expectedSize = 16 + static_cast<size_t>(width) * height * 2;

		return packet.data.size() == expectedSize;
	}

	static rawTexture convertTexturePacketToImage(const RawPacket& packet)
	{
		constexpr size_t paletteSize = 256 * 3;

		rawTexture image;

		if (packet.data.size() < 12 + paletteSize)
			return image;

		size_t headerSize = 12;
		uint32_t width = 256;
		uint32_t height = 256;

		// Header layout:
		// 0x04 = width
		// 0x06 = height
		// 0x08 = palette size marker 0x0300
		if (readU16LE(packet.data, 8) == 0x0300)
		{
			width = readU16LE(packet.data, 4);
			height = readU16LE(packet.data, 6);
			headerSize = 12;
		}
		// Alternate header layout:
		// 0x08 = width
		// 0x0A = height
		// 0x0C = palette size marker 0x0300
		else if (readU16LE(packet.data, 12) == 0x0300)
		{
			width = readU16LE(packet.data, 8);
			height = readU16LE(packet.data, 10);
			headerSize = 16;
		}

		if (width == 0 || height == 0)
			return image;

		const size_t pixelOffset = headerSize + paletteSize;

		if (packet.data.size() <= pixelOffset)
			return image;

		const uint8_t* palette = packet.data.data() + headerSize;
		const uint8_t* pixels = packet.data.data() + pixelOffset;

		const size_t pixelDataSize = packet.data.size() - pixelOffset;
		const size_t pixelCount = static_cast<size_t>(width) * height;

		const bool is8bpp = pixelDataSize >= pixelCount;
		const bool is4bpp = ! is8bpp && pixelDataSize >= (pixelCount / 2);

		if (! is8bpp && ! is4bpp)
			return image;

		image.width = width;
		image.height = height;
		image.rgba.resize(pixelCount * 4);

		auto writePaletteColor = [&](size_t outPixelIndex, uint8_t paletteIndex) {
			const size_t palOff = static_cast<size_t>(paletteIndex) * 3;
			const size_t outOff = outPixelIndex * 4;

			image.rgba[outOff + 0] = palette[palOff + 0];
			image.rgba[outOff + 1] = palette[palOff + 1];
			image.rgba[outOff + 2] = palette[palOff + 2];
			image.rgba[outOff + 3] = 255;
		};

		if (is8bpp)
		{
			for (size_t i = 0; i < pixelCount; i++)
			{
				writePaletteColor(i, pixels[i]);
			}
		}
		else
		{
			for (uint32_t y = 0; y < height; y++)
			{
				for (uint32_t x = 0; x < width; x += 2)
				{
					const size_t byteIndex = (static_cast<size_t>(y) * width + x) / 2;

					if (byteIndex >= pixelDataSize)
						continue;

					const uint8_t packed = pixels[byteIndex];

					const uint8_t lowNibble = packed & 0x0F;
					const uint8_t highNibble = packed >> 4;

					const uint32_t paletteBlock = (x / 64) + ((y / 64) * 4);

					const uint8_t index0 = static_cast<uint8_t>(paletteBlock * 16 + lowNibble);
					const uint8_t index1 = static_cast<uint8_t>(paletteBlock * 16 + highNibble);

					const size_t outIndex0 = static_cast<size_t>(y) * width + x;
					const size_t outIndex1 = outIndex0 + 1;

					writePaletteColor(outIndex0, index0);

					if (x + 1 < width)
						writePaletteColor(outIndex1, index1);
				}
			}
		}

		return image;
	}

	static bool readRawFileToMemory(const fs::path& inputFilePath, RawReadResult& result)
	{
		std::ifstream inFile(inputFilePath, std::ios::binary);
		if (! inFile)
		{
			std::println("Failed to open {}", inputFilePath.string());
			result.success = false;
			return false;
		}

		uint32_t beUncompressedSize = 0;
		uint32_t beCompressedSize = 0;
		uint32_t packetIndex = 0;

		inFile.read(reinterpret_cast<char*>(&beUncompressedSize), sizeof(beUncompressedSize));

		while (inFile && beUncompressedSize != 0xFFFFFFFF)
		{
			uint32_t uncompressedSize = toBigEndian(beUncompressedSize);

			inFile.read(reinterpret_cast<char*>(&beCompressedSize), sizeof(beCompressedSize));
			if (! inFile)
				break;

			uint32_t compressedSize = toBigEndian(beCompressedSize);

			std::vector<uint8_t> compressedBuffer(compressedSize + 14);
			std::vector<uint8_t> decompressedBuffer(uncompressedSize);

			std::memcpy(compressedBuffer.data(), &beUncompressedSize, 4);
			std::memcpy(compressedBuffer.data() + 4, &beCompressedSize, 4);

			inFile.read(reinterpret_cast<char*>(compressedBuffer.data() + 8), 6 + compressedSize);

			if (! inFile)
			{
				std::println("Failed to read compressed packet {}", packetIndex);
				result.success = false;
				return false;
			}

			// std::println("Inflating {}->{}", compressedSize, uncompressedSize);

			DecompressBuffer(compressedBuffer.data(), decompressedBuffer.data());

			RawPacket packet;
			packet.sourcePath = inputFilePath;
			packet.packetIndex = packetIndex;
			packet.compressedSize = compressedSize;
			packet.uncompressedSize = uncompressedSize;
			packet.data = std::move(decompressedBuffer);

			if (! packet.data.empty() && packet.data[0] == 35)
			{
				const size_t creatureBytes = sizeof(CreatureRam) * kMaxActors;

				if (packet.data.size() >= 4 + creatureBytes)
				{
					const uint8_t* creatureBytesStart = packet.data.data() + 4;

					for (int32_t idx = 0; idx < kMaxActors; idx++)
					{
						CreatureRam creature {};
						std::memcpy(&creature, creatureBytesStart + sizeof(CreatureRam) * idx, sizeof(CreatureRam));

						result.creatures.push_back(creature);
					}
				}
				else
				{
					std::println("Type 35 packet {} in {} is too small for {} creatures", packetIndex, inputFilePath.string(), kMaxActors);
				}
				while (! result.creatures.empty() && result.creatures.back().creatureId == 0)
				{
					result.creatures.pop_back();
				}
			}

			if (packet.data[0] == 0x01 && packet.data[1] == 0x01) // anm
			{
				result.anmPackets.push_back(std::move(packet));
			}
			else if (packet.data[0] == 0x02 && packet.data[1] == 0x01 or packet.data[0] == 0x03 && packet.data[1] == 0x01) // all
			{
				result.allPackets.push_back(std::move(packet));
			}
			else if (packet.data[0] != 35) // infer texture if not all, anm or entdata
			{
				rawTexture convPacket;
				if (isRGB555Packet(packet))
				{
					convPacket = convertRGB555PacketToImage(packet);
				}
				else
				{
					convPacket = convertTexturePacketToImage(packet);
				}
				convPacket.image = createGLTex(convPacket);
				result.texPackets.push_back(std::move(convPacket));
			}

			packetIndex++;

			inFile.read(reinterpret_cast<char*>(&beUncompressedSize), sizeof(beUncompressedSize));
		}

		return true;
	}

	RawReadResult ReadRAW(const fs::path& inputFilePath)
	{
		auto start_time = std::chrono::high_resolution_clock::now();
		RawReadResult result;

		if (inputFilePath.empty())
		{
			std::println("No RAW path selected");
			result.success = false;
			return result;
		}

		if (fs::is_directory(inputFilePath))
		{
			// std::println("Running directory code!\n");

			for (const auto& entry : fs::recursive_directory_iterator(inputFilePath))
			{
				if (! entry.is_regular_file())
					continue;

				if (entry.path().extension() != ".raw")
					continue;

				std::println("Found: {}", entry.path().string());

				if (! readRawFileToMemory(entry.path(), result))
					break;
			}
		}
		else
		{
			// std::println("Running file code!\n");
			readRawFileToMemory(inputFilePath, result);
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		std::println("~Took {}ms to parse .raw file", std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());
		return result;
	}
}
