#include "BinaryReader.hpp"
#include "Parsers/ALLParser.hpp"

#include "Clock.hpp"

#include <iostream>
#include <print>

namespace ALLParser
{
	// 0x01    graphics mesh
	// 0x06    collision mesh
	// 0x08    dynamic collision mesh
	// 0x011F  graphics mesh joint data
	// 0x09    target position data
	// 0x0101  infinite wall collision
	// 0x0104  collision footer

	DataGroup decodeGroupType(uint32_t type)
	{
		switch (type)
		{
			case 0x01:
				return DataGroup::GFX;
			case 0x06:
				return DataGroup::Collision;
			case 0x08:
				return DataGroup::DynamicCollision;
			case 0x011F:
				return DataGroup::GraphicsJointData;
			case 0x09:
				return DataGroup::TargetPosition;
			case 0x0101:
				return DataGroup::InfiniteWallCollision;
			case 0x0104:
				return DataGroup::CollisionFooter;
			default:
				return DataGroup::Unknown;
		}
	}

	Vector2F normaliseUV(Vector2u8 rawUV, uint8_t texturePageByte)
	{
		uint8_t texturePage = texturePageByte & 0x1F;

		// Juan's HTML:
		// u = rawU / 8192 + texturePage * 0.03125
		// v = 1 - rawV / 256
		return Vector2F { static_cast<float>(rawUV.u) / 8192.0f + static_cast<float>(texturePage) * 0.03125f, 1.0f - static_cast<float>(rawUV.v) / 256.0f };
	}

	MeshVert makeVertMesh(Vector3S pos, Vector2u8 uv, RGBu8 colour, uint8_t texPageByte)
	{
		MeshVert v;
		v.pos = pos;
		v.UV = uv;
		v.UVN = normaliseUV(uv, texPageByte);
		v.colour = colour;
		return v;
	}

	GFXMesh parseGFX(std::ifstream& file, const DataGroupHeader& header, size_t fileSize)
	{
		BinaryReader binaryReader(file);
		GFXMesh mesh;
		mesh.header = header;

		size_t pos = header.payloadOffset;
		const size_t payloadEnd = header.payloadOffset + header.payloadSize;

		while (pos + 4 <= fileSize && pos + 4 <= payloadEnd)
		{
			uint8_t flagPeek = binaryReader.readU8At(pos + 3, fileSize);

			if ((flagPeek & 0xF0) != 0x30)
				break;

			RGBu8 firstColour { binaryReader.readU8At(pos + 0, fileSize), binaryReader.readU8At(pos + 1, fileSize), binaryReader.readU8At(pos + 2, fileSize) };

			uint8_t flag = binaryReader.readU8At(pos + 3, fileSize);
			pos += 4; // seek

			int vertexCount = (flag & 0x08) ? 4 : 3;

			std::vector<Vector3S> positions;
			std::vector<Vector2u8> uvs;
			std::vector<RGBu8> colours;

			positions.reserve(vertexCount); // resize vectors to vertexcount
			uvs.reserve(vertexCount);
			colours.reserve(vertexCount);

			// The JS uses unshift(), so it reverses vertex order as it reads.
			for (int i = 0; i < vertexCount; i++)
			{
				Vector3S p = binaryReader.readVec3sAt(pos, fileSize);
				pos += 6;

				Vector2u8 uv { binaryReader.readU8At(pos + 0, fileSize), binaryReader.readU8At(pos + 1, fileSize) };
				pos += 2;

				positions.insert(positions.begin(), p);
				uvs.insert(uvs.begin(), uv);
			}

			// First colour was before the flag. Extra colours follow.
			colours.insert(colours.begin(), firstColour);

			uint8_t texPageByte = 0;

			for (int i = 1; i < vertexCount; i++)
			{
				RGBu8 c { binaryReader.readU8At(pos + 0, fileSize), binaryReader.readU8At(pos + 1, fileSize), binaryReader.readU8At(pos + 2, fileSize) };

				uint8_t fourthByte = binaryReader.readU8At(pos + 3, fileSize);
				pos += 4;

				texPageByte = fourthByte;

				// JS also unshifts these colours.
				colours.insert(colours.begin(), c);
			}

			uint8_t texturePage = texPageByte & 0x1F;
			uint8_t matIndex = static_cast<uint8_t>((flag & 0x07) | ((texPageByte >> 4) & 0x08));

			auto makeTri = [&](int a, int b, int c) {
				MeshTri tri;
				tri.flag = flag;
				tri.texPageByte = texPageByte;
				tri.texPage = texturePage;
				tri.matIndex = matIndex;

				tri.v0 = makeVertMesh(positions[a], uvs[a], colours[a], texPageByte);
				tri.v1 = makeVertMesh(positions[b], uvs[b], colours[b], texPageByte);
				tri.v2 = makeVertMesh(positions[c], uvs[c], colours[c], texPageByte);

				mesh.triangles.push_back(tri);
			};

			if (vertexCount == 4)
			{
				// Same split as the Three.js viewer:
				// 0,1,2 and 0,2,3
				makeTri(0, 1, 2);
				makeTri(0, 2, 3);
			}
			else
			{
				makeTri(0, 1, 2);
			}
		}

		// HTML checks for uint32 == 2 after primitive stream as an LOD marker.
		if (pos + 4 <= fileSize && binaryReader.readU32At(pos, fileSize) == 2)
		{
			mesh.LOD = true;
			mesh.lodOffset = static_cast<uint32_t>(pos);
		}

		return mesh;
	}

	CollisionPoly parseCollisionPoly(std::ifstream& file, size_t off, size_t fileSize)
	{
		BinaryReader binaryReader(file);
		CollisionPoly p;

		p.activeArea = { binaryReader.readS16At(off + 0, fileSize),
			binaryReader.readS16At(off + 2, fileSize),
			binaryReader.readS16At(off + 4, fileSize),
			binaryReader.readS16At(off + 6, fileSize) };

		p.origin =
			Vector3S { binaryReader.readS16At(off + 8, fileSize), binaryReader.readS16At(off + 10, fileSize), binaryReader.readS16At(off + 12, fileSize) };

		Vector3S p2off { binaryReader.readS16At(off + 14, fileSize), binaryReader.readS16At(off + 16, fileSize), binaryReader.readS16At(off + 18, fileSize) };

		Vector3S p3off { binaryReader.readS16At(off + 20, fileSize), binaryReader.readS16At(off + 22, fileSize), binaryReader.readS16At(off + 24, fileSize) };

		Vector3S p4off { binaryReader.readS16At(off + 26, fileSize), binaryReader.readS16At(off + 28, fileSize), binaryReader.readS16At(off + 30, fileSize) };

		p.p2 = binaryReader.addVec3s(p.origin, p2off);
		p.p3 = binaryReader.addVec3s(p.origin, p3off);
		p.p4 = binaryReader.addVec3s(p.origin, p4off);

		p.inclination1 = { binaryReader.readS16At(off + 32, fileSize), binaryReader.readS16At(off + 36, fileSize) };

		p.bouncing1 = binaryReader.readS16At(off + 34, fileSize);

		p.inclination2 = { binaryReader.readS16At(off + 38, fileSize), binaryReader.readS16At(off + 42, fileSize) };

		p.bouncing2 = binaryReader.readS16At(off + 40, fileSize);

		return p;
	}

	CollisionMesh parseCollision(std::ifstream& file, const DataGroupHeader& header, size_t fileSize)
	{
		BinaryReader binaryReader(file);
		CollisionMesh mesh;
		mesh.header = header;
		mesh.collisionType = header.collisionType;

		size_t pos = header.payloadOffset;
		const size_t payloadEnd = header.payloadOffset + header.payloadSize;

		while (pos + 12 <= fileSize && pos + 12 <= payloadEnd)
		{
			CollisionBlock block;
			block.enabled = binaryReader.readS16At(pos, fileSize);
			pos += 2;

			if (block.enabled != 1)
			{
				break;
			}

			block.polyCount = binaryReader.readS16At(pos, fileSize);
			pos += 2;

			for (int i = 0; i < 4; i++)
			{
				block.unk[i] = binaryReader.readS16At(pos, fileSize);
				pos += 2;
			}

			if (block.polyCount < 0)
			{
				throw std::runtime_error("Negative collision polygon count");
			}

			block.polygons.reserve(static_cast<size_t>(block.polyCount));

			for (int i = 0; i < block.polyCount; i++)
			{
				constexpr size_t Ts2CollisionPolygonBytes = 22 * 2;

				if (pos + Ts2CollisionPolygonBytes > fileSize || pos + Ts2CollisionPolygonBytes > payloadEnd)
				{
					throw std::runtime_error("Collision polygon exceeds file size");
				}

				block.polygons.push_back(parseCollisionPoly(file, pos, fileSize));
				pos += Ts2CollisionPolygonBytes;
			}

			mesh.blocks.push_back(std::move(block));
		}

		return mesh;
	}

	const char* typeName(DataGroup type)
	{ // for debug
		switch (type)
		{
			case DataGroup::GFX:
				return "GFX";
			case DataGroup::Collision:
				return "Collision";
			case DataGroup::DynamicCollision:
				return "DynamicCollision";
			case DataGroup::GraphicsJointData:
				return "GraphicsJointData";
			case DataGroup::TargetPosition:
				return "TargetPosition";
			case DataGroup::InfiniteWallCollision:
				return "InfiniteWallCollision";
			case DataGroup::CollisionFooter:
				return "CollisionFooter";
			default:
				return "Unknown";
		}
	}

	AllFile ReadALL(std::ifstream& file)
	{
		BinaryReader binaryReader(file);
		Timer timer;

		const size_t fileSize = binaryReader.getFileSize();

		if (fileSize < 8)
			throw std::runtime_error("too tiny to be an .all file");

		AllFile all;

		all.metadataOffset = binaryReader.readU32At(0, fileSize) * 2;

		if (all.metadataOffset + 4 > fileSize)
			throw std::runtime_error("metadata offset is EOF");

		all.dataGroupCount = binaryReader.readU32At(all.metadataOffset, fileSize);

		size_t metadataEntryOffset = all.metadataOffset + 4;
		size_t currentPayloadOffset = 4;
		size_t lastPayloadOffset = 4;

		for (uint32_t i = 0; i < all.dataGroupCount; i++)
		{
			if (metadataEntryOffset + 0x4C > fileSize)
			{
				throw std::runtime_error("Metadata entry exceeds file size");
			}

			DataGroupHeader h;
			h.metadataOffset = static_cast<uint32_t>(metadataEntryOffset);

			uint32_t sizeWords = binaryReader.readU32At(metadataEntryOffset + 0x00, fileSize);
			h.payloadSize = sizeWords * 2;

			// The JS does:
			// pos = sizeWords ? currpos : lastpos
			if (sizeWords != 0)
			{
				h.payloadOffset = static_cast<uint32_t>(currentPayloadOffset);
			}
			else
			{
				h.payloadOffset = static_cast<uint32_t>(lastPayloadOffset);
			}

			h.pos = Vector3I { binaryReader.readS32At(metadataEntryOffset + 0x04, fileSize),
				binaryReader.readS32At(metadataEntryOffset + 0x08, fileSize),
				binaryReader.readS32At(metadataEntryOffset + 0x0C, fileSize) };

			h.typeRaw = binaryReader.readU32At(metadataEntryOffset + 0x10, fileSize);
			h.type = decodeGroupType(h.typeRaw);

			h.collisionType = binaryReader.readU8At(metadataEntryOffset + 0x28, fileSize);
			h.flags = binaryReader.readU8At(metadataEntryOffset + 0x29, fileSize);

			all.dataGroups.push_back(h);

			switch (h.type)
			{
				case DataGroup::GFX:
					all.gfxMeshes.push_back(parseGFX(file, h, fileSize));
					break;

				case DataGroup::Collision:
				case DataGroup::DynamicCollision:
					all.collisionMeshes.push_back(parseCollision(file, h, fileSize));
					break;

				default:
					// Known ignored types and unknown types are preserved in all.dataGroups.
					break;
			}

			if (sizeWords != 0)
			{
				lastPayloadOffset = currentPayloadOffset;
				currentPayloadOffset += h.payloadSize;
			}

			metadataEntryOffset += 0x4C;
		}

		bool debug = true;
		if (debug == true)
		{
			std::cout << "Metadata offset: 0x" << std::hex << all.metadataOffset << std::dec << "\n";

			std::cout << "Datagroups: " << all.dataGroupCount << "\n";
			std::cout << "Graphics meshes: " << all.gfxMeshes.size() << "\n";
			std::cout << "Collision meshes: " << all.collisionMeshes.size() << "\n\n";

			for (const DataGroupHeader& h : all.dataGroups)
			{
				std::cout
					<< "Datagroup @ metadata 0x" << std::hex << h.metadataOffset << ", payload 0x" << h.payloadOffset << std::dec << ", size " << h.payloadSize
					<< ", type " << typeName(h.type) << " / raw 0x" << std::hex << h.typeRaw << std::dec << ", pos(" << h.pos.x << ", " << h.pos.y << ", "
					<< h.pos.z << ")";

				if (h.type == DataGroup::Collision || h.type == DataGroup::DynamicCollision)
				{
					std::cout << ", collisionType " << static_cast<int>(h.collisionType) << ", lodFlag " << h.hasLodFlag();
				}

				std::cout << "\n";
			}

			std::cout << "\nGraphics mesh details:\n";
			for (size_t i = 0; i < all.gfxMeshes.size(); i++)
			{
				const GFXMesh& m = all.gfxMeshes[i];
				std::cout << "  Mesh " << i << ": triangles=" << m.triangles.size() << ", LOD marker=" << (m.LOD ? "yes" : "no") << "\n";
			}

			std::cout << "\nCollision mesh details:\n";
			for (size_t i = 0; i < all.collisionMeshes.size(); i++)
			{
				const CollisionMesh& m = all.collisionMeshes[i];

				size_t polygonCount = 0;
				for (const CollisionBlock& b : m.blocks)
				{
					polygonCount += b.polygons.size();
				}

				std::cout
					<< "  Collision " << i << ": type=" << static_cast<int>(m.collisionType) << ", blocks=" << m.blocks.size() << ", polygons=" << polygonCount
					<< "\n";
			}
		}

		std::println("~Took {}ms to parse .all file", timer.returnTime());
		return all;
	}
}
