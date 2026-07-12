#include <print>

#include "BinaryReader.hpp"
#include "DATParser.hpp"
#include "Clock.hpp"

// 0x3F = coins
// 0x0 = Box Type #0 (Attic)
// 0xX = different boxes(?)

namespace DATParser
{
    void readEntries(std::ifstream& file, DatFile& dat, int32_t entryCount, BinaryReader& reader)
    {
        int16_t objectCount = reader.readS16();
		int16_t entryType = reader.readS16();

		switch (entryType)
		{
			case (63): // coinChunk
			{
				for (int i = 0; i < objectCount; i++)
				{
					dat.entryChunks.coinChunk.coinCount = objectCount;
					Coin coin;
					coin.pos = reader.readVec3iAt(file.tellg(), reader.getFileSize());
					file.seekg(12, std::ios::cur); // read past pos
					coin.spriteIndex = reader.readU8();
					coin.flags = reader.readU8();
					coin.floorPos = reader.readU16();
                    dat.entryChunks.coinChunk.coins.push_back(coin);
				}
				break;
			}
			default:
				break;
		}
    }

	DatFile readDat(std::ifstream& file)
	{
        Timer timer;

		DatFile dat;

		BinaryReader reader(file);

        int32_t entryCount = reader.readS32();

        readEntries(file, dat, entryCount, reader);

        std::println("Parsed .dat file in {}ms", timer.returnTime());
		return dat;
	}

}