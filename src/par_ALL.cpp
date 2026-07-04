//0x01    graphics mesh
//0x06    collision mesh
//0x08    dynamic collision mesh
//0x011F  graphics mesh joint data
//0x09    target position data
//0x0101  infinite wall collision
//0x0104  collision footer

#include <ALL.h>
#include <common.h>
#include <iostream>
#include <fstream>
#include <print>
#include <chrono>

const std::streamsize readUInt32 = 4;
const std::streamsize readInt32 = 4;
const std::streamsize readUInt16 = 2;
const std::streamsize readInt16 = 2;
const std::streamsize readUInt8 = 1;

static size_t getFileSize(std::ifstream& file){
    std::streampos oldPos = file.tellg();

    file.seekg(0, std::ios::end);
    std::streampos endPos = file.tellg();

    file.seekg(oldPos, std::ios::beg);

    if (endPos < 0) throw std::runtime_error("file size NULL");

    return static_cast<size_t>(endPos);
}

static void requireRange (size_t fileSize, size_t off, size_t len) {
    if (off + len < off || off + len > fileSize) throw std::runtime_error("EOF");
}

static void seekTo(std::ifstream& file, size_t off, size_t fileSize) {
    requireRange(fileSize, off, 0);
    file.seekg(static_cast<std::streamoff>(off), std::ios::beg);

    if (!file) throw std::runtime_error("seek no work");
}

template <typename T>
static T readValue(std::ifstream& file, std::streamsize bytes) {
    T value{};
    file.read(reinterpret_cast<char*>(&value), bytes);

    if (!file) throw std::runtime_error("read no work");

    return value;
}

// FILE READ HELPERS FOR COMMON TYPES

static uint8_t readU8(std::ifstream& file) {
    return readValue<uint8_t>(file, readUInt8);
}

static int16_t readS16(std::ifstream& file) {
    return readValue<int16_t>(file, readInt16);
}

static uint32_t readU32(std::ifstream& file) {
    return readValue<uint32_t>(file, readUInt32);
}

static int32_t readS32(std::ifstream& file) {
    return readValue<int32_t>(file, readInt32);
}

static uint8_t readU8At(std::ifstream& file, size_t off, size_t fileSize) {
    requireRange(fileSize, off, sizeof(uint8_t));
    seekTo(file, off, fileSize);
    return readU8(file);
}

static int16_t readS16At(std::ifstream& file, size_t off, size_t fileSize) {
    requireRange(fileSize, off, sizeof(int16_t));
    seekTo(file, off, fileSize);
    return readS16(file);
}

static uint32_t readU32At(std::ifstream& file, size_t off, size_t fileSize) {
    requireRange(fileSize, off, sizeof(uint32_t));
    seekTo(file, off, fileSize);
    return readU32(file);
}

static int32_t readS32At(std::ifstream& file, size_t off, size_t fileSize) {
    requireRange(fileSize, off, sizeof(int32_t));
    seekTo(file, off, fileSize);
    return readS32(file);
}

static Vector3S readVec3sAt(std::ifstream& file, size_t off, size_t fileSize) {
    return Vector3S{
        readS16At(file, off + 0, fileSize),
        readS16At(file, off + 2, fileSize),
        readS16At(file, off + 4, fileSize)
    };
}

static Vector3S addVec3s(Vector3S a, Vector3S b) {
    return Vector3S{
        static_cast<int16_t>(a.x + b.x),
        static_cast<int16_t>(a.y + b.y),
        static_cast<int16_t>(a.z + b.z)
    };
}

// END OF HELPERS

static DataGroup decodeGroupType(uint32_t type) {
    switch (type) {
        case 0x01:   return DataGroup::GFX;
        case 0x06:   return DataGroup::Collision;
        case 0x08:   return DataGroup::DynamicCollision;
        case 0x011F: return DataGroup::GraphicsJointData;
        case 0x09:   return DataGroup::TargetPosition;
        case 0x0101: return DataGroup::InfiniteWallCollision;
        case 0x0104: return DataGroup::CollisionFooter;
        default:     return DataGroup::Unknown;
    }
}

static Vector2F normaliseUV(Vector2u8 rawUV, uint8_t texturePageByte) {
    uint8_t texturePage = texturePageByte & 0x1F;

    // Juan's HTML:
    // u = rawU / 8192 + texturePage * 0.03125
    // v = 1 - rawV / 256
    return Vector2F{
        static_cast<float>(rawUV.u) / 8192.0f + static_cast<float>(texturePage) * 0.03125f,
        1.0f - static_cast<float>(rawUV.v) / 256.0f
    };
}

static MeshVert makeVertMesh(Vector3S pos, Vector2u8 uv, RGBu8 colour, uint8_t texPageByte) {
    MeshVert v;
    v.pos = pos;
    v.UV = uv;
    v.UVN = normaliseUV(uv, texPageByte);
    v.colour = colour;
    return v;
}

static GFXMesh parseGFX(std::ifstream& file, const DataGroupHeader& header, size_t fileSize) {
    GFXMesh mesh;
    mesh.header = header;

    size_t pos = header.payloadOffset;
    const size_t payloadEnd = header.payloadOffset + header.payloadSize;

    while (pos + 4 <= fileSize && pos + 4 <= payloadEnd) {
        uint8_t flagPeek = readU8At(file, pos + 3, fileSize);

        if ((flagPeek & 0xF0) != 0x30) break;

        RGBu8 firstColour{
            readU8At(file, pos + 0, fileSize),
            readU8At(file, pos + 1, fileSize),
            readU8At(file, pos + 2, fileSize)
        };

        uint8_t flag = readU8At(file, pos + 3, fileSize);
        pos += 4; //seek

        int vertexCount = (flag & 0x08) ? 4:3;

        std::vector<Vector3S> positions;
        std::vector<Vector2u8> uvs;
        std::vector<RGBu8> colours;

        positions.reserve(vertexCount); // resize vectors to vertexcount
        uvs.reserve(vertexCount);
        colours.reserve(vertexCount);

        // The JS uses unshift(), so it reverses vertex order as it reads.
        for (int i = 0; i < vertexCount; i++) {
            Vector3S p = readVec3sAt(file, pos, fileSize);
            pos += 6;

            Vector2u8 uv{ readU8At(file, pos + 0, fileSize), readU8At(file, pos + 1, fileSize) };
            pos += 2;

            positions.insert(positions.begin(), p);
            uvs.insert(uvs.begin(), uv);
        }

        // First colour was before the flag. Extra colours follow.
        colours.insert(colours.begin(), firstColour);

        uint8_t texPageByte = 0;

        for (int i = 1; i < vertexCount; i++) {
            RGBu8 c{
                readU8At(file, pos + 0, fileSize),
                readU8At(file, pos + 1, fileSize),
                readU8At(file, pos + 2, fileSize)
            };

            uint8_t fourthByte = readU8At(file, pos + 3, fileSize);
            pos += 4;

            texPageByte = fourthByte;

            // JS also unshifts these colours.
            colours.insert(colours.begin(), c);
        }

        uint8_t texturePage = texPageByte & 0x1F;
        uint8_t matIndex = static_cast<uint8_t>(
            (flag & 0x07) | ((texPageByte >> 4) & 0x08)
            );


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

        if (vertexCount == 4) {
            // Same split as the Three.js viewer:
            // 0,1,2 and 0,2,3
            makeTri(0, 1, 2);
            makeTri(0, 2, 3);
        }
        else {
            makeTri(0, 1, 2);
        }
    }

    // HTML checks for uint32 == 2 after primitive stream as an LOD marker.
    if (pos + 4 <= fileSize && readU32At(file, pos, fileSize) == 2) {
        mesh.LOD = true;
        mesh.lodOffset = static_cast<uint32_t>(pos);
    }

    return mesh;
}

static CollisionPoly parseCollisionPoly(
    std::ifstream& file,
    size_t off,
    size_t fileSize
) {
    CollisionPoly p;

    p.activeArea = {
        readS16At(file, off + 0, fileSize),
        readS16At(file, off + 2, fileSize),
        readS16At(file, off + 4, fileSize),
        readS16At(file, off + 6, fileSize)
    };

    p.origin = Vector3S{
        readS16At(file, off + 8, fileSize),
        readS16At(file, off + 10, fileSize),
        readS16At(file, off + 12, fileSize)
    };

    Vector3S p2off{
        readS16At(file, off + 14, fileSize),
        readS16At(file, off + 16, fileSize),
        readS16At(file, off + 18, fileSize)
    };

    Vector3S p3off{
        readS16At(file, off + 20, fileSize),
        readS16At(file, off + 22, fileSize),
        readS16At(file, off + 24, fileSize)
    };

    Vector3S p4off{
        readS16At(file, off + 26, fileSize),
        readS16At(file, off + 28, fileSize),
        readS16At(file, off + 30, fileSize)
    };

    p.p2 = addVec3s(p.origin, p2off);
    p.p3 = addVec3s(p.origin, p3off);
    p.p4 = addVec3s(p.origin, p4off);

    p.inclination1 = {
        readS16At(file, off + 32, fileSize),
        readS16At(file, off + 36, fileSize)
    };

    p.bouncing1 = readS16At(file, off + 34, fileSize);

    p.inclination2 = {
        readS16At(file, off + 38, fileSize),
        readS16At(file, off + 42, fileSize)
    };

    p.bouncing2 = readS16At(file, off + 40, fileSize);

    return p;
}

static CollisionMesh parseCollision(
    std::ifstream& file,
    const DataGroupHeader& header,
    size_t fileSize
) {
    CollisionMesh mesh;
    mesh.header = header;
    mesh.collisionType = header.collisionType;

    size_t pos = header.payloadOffset;
    const size_t payloadEnd = header.payloadOffset + header.payloadSize;

    while (pos + 12 <= fileSize && pos + 12 <= payloadEnd) {
        CollisionBlock block;
        block.enabled = readS16At(file, pos, fileSize);
        pos += 2;

        if (block.enabled != 1) {
            break;
        }

        block.polyCount = readS16At(file, pos, fileSize);
        pos += 2;

        for (int i = 0; i < 4; i++) {
            block.unk[i] = readS16At(file, pos, fileSize);
            pos += 2;
        }

        if (block.polyCount < 0) {
            throw std::runtime_error("Negative collision polygon count");
        }

        block.polygons.reserve(static_cast<size_t>(block.polyCount));

        for (int i = 0; i < block.polyCount; i++) {
            constexpr size_t Ts2CollisionPolygonBytes = 22 * 2;

            if (pos + Ts2CollisionPolygonBytes > fileSize || pos + Ts2CollisionPolygonBytes > payloadEnd) {
                throw std::runtime_error("Collision polygon exceeds file size");
            }

            block.polygons.push_back(parseCollisionPoly(file, pos, fileSize));
            pos += Ts2CollisionPolygonBytes;
        }

        mesh.blocks.push_back(std::move(block));
    }

    return mesh;
}

static const char* typeName(DataGroup type) { // for debug
    switch (type) {
    case DataGroup::GFX:          return "GFX";
    case DataGroup::Collision:         return "Collision";
    case DataGroup::DynamicCollision:  return "DynamicCollision";
    case DataGroup::GraphicsJointData:     return "GraphicsJointData";
    case DataGroup::TargetPosition:        return "TargetPosition";
    case DataGroup::InfiniteWallCollision: return "InfiniteWallCollision";
    case DataGroup::CollisionFooter:       return "CollisionFooter";
    default:                                   return "Unknown";
    }
}

AllFile readALL(std::ifstream& file) {
    auto start_time = std::chrono::high_resolution_clock::now();

    const size_t fileSize = getFileSize(file);

    if (fileSize < 8) throw std::runtime_error("too tiny to be an .all file");

    AllFile all;

    all.metadataOffset = readU32At(file, 0, fileSize) * 2;

    if (all.metadataOffset + 4 > fileSize) throw std::runtime_error("metadata offset is EOF");

    all.dataGroupCount = readU32At(file, all.metadataOffset, fileSize);

    size_t metadataEntryOffset = all.metadataOffset + 4;
    size_t currentPayloadOffset = 4;
    size_t lastPayloadOffset = 4;

    for (uint32_t i = 0; i < all.dataGroupCount; i++) {
        if (metadataEntryOffset + 0x4C > fileSize) {
            throw std::runtime_error("Metadata entry exceeds file size");
        }

        DataGroupHeader h;
        h.metadataOffset = static_cast<uint32_t>(metadataEntryOffset);

        uint32_t sizeWords = readU32At(file, metadataEntryOffset + 0x00, fileSize);
        h.payloadSize = sizeWords * 2;

        // The JS does:
        // pos = sizeWords ? currpos : lastpos
        if (sizeWords != 0) {
            h.payloadOffset = static_cast<uint32_t>(currentPayloadOffset);
        }
        else {
            h.payloadOffset = static_cast<uint32_t>(lastPayloadOffset);
        }

        h.pos = Vector3I{
            readS32At(file, metadataEntryOffset + 0x04, fileSize),
            readS32At(file, metadataEntryOffset + 0x08, fileSize),
            readS32At(file, metadataEntryOffset + 0x0C, fileSize)
        };

        h.typeRaw = readU32At(file, metadataEntryOffset + 0x10, fileSize);
        h.type = decodeGroupType(h.typeRaw);

        h.collisionType = readU8At(file, metadataEntryOffset + 0x28, fileSize);
        h.flags = readU8At(file, metadataEntryOffset + 0x29, fileSize);

        all.dataGroups.push_back(h);

        switch (h.type) {
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

        if (sizeWords != 0) {
            lastPayloadOffset = currentPayloadOffset;
            currentPayloadOffset += h.payloadSize;
        }

        metadataEntryOffset += 0x4C;
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    bool debug = true;
    if (debug == true) {
        std::cout << "Metadata offset: 0x"
            << std::hex << all.metadataOffset << std::dec << "\n";

        std::cout << "Datagroups: " << all.dataGroupCount << "\n";
        std::cout << "Graphics meshes: " << all.gfxMeshes.size() << "\n";
        std::cout << "Collision meshes: " << all.collisionMeshes.size() << "\n\n";

        for (const DataGroupHeader& h : all.dataGroups) {
            std::cout << "Datagroup @ metadata 0x"
                << std::hex << h.metadataOffset
                << ", payload 0x" << h.payloadOffset
                << std::dec
                << ", size " << h.payloadSize
                << ", type " << typeName(h.type)
                << " / raw 0x" << std::hex << h.typeRaw << std::dec
                << ", pos("
                << h.pos.x << ", "
                << h.pos.y << ", "
                << h.pos.z << ")";

            if (h.type == DataGroup::Collision ||
                h.type == DataGroup::DynamicCollision) {
                std::cout << ", collisionType " << static_cast<int>(h.collisionType)
                    << ", lodFlag " << h.LOD;
            }

            std::cout << "\n";
        }

        std::cout << "\nGraphics mesh details:\n";
        for (size_t i = 0; i < all.gfxMeshes.size(); i++) {
            const GFXMesh& m = all.gfxMeshes[i];
            std::cout << "  Mesh " << i
                << ": triangles=" << m.triangles.size()
                << ", LOD marker=" << (m.LOD ? "yes" : "no")
                << "\n";
        }

        std::cout << "\nCollision mesh details:\n";
        for (size_t i = 0; i < all.collisionMeshes.size(); i++) {
            const CollisionMesh& m = all.collisionMeshes[i];

            size_t polygonCount = 0;
            for (const CollisionBlock& b : m.blocks) {
                polygonCount += b.polygons.size();
            }

            std::cout << "  Collision " << i
                << ": type=" << static_cast<int>(m.collisionType)
                << ", blocks=" << m.blocks.size()
                << ", polygons=" << polygonCount
                << "\n";
        }
    }

	std::println("~Took {}ms to parse .all file", std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());
    return all;
}