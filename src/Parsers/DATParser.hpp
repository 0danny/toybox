#pragma once

#include <fstream>
#include <vector>

#include "Common.hpp"

namespace DATParser
{
	struct Coin
	{
		Vector3I pos;
		int8_t spriteIndex;
		int8_t flags;
		int16_t floorPos;
	};

	struct CoinChunk
	{
		int16_t coinCount;
		std::vector<Coin> coins;
	};

	struct DefaultChunk
	{
		int16_t entryCount;
		int16_t entryType;
		std::vector<Vector3I> pos;
	};

	struct EntryChunks
	{
		CoinChunk coinChunk;
		std::vector<DefaultChunk> defaultChunks;
	};

	struct DatFile
	{
		EntryChunks entryChunks;
	};

	DatFile readDat(std::ifstream& file);
}