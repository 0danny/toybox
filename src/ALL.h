#pragma once
#include <cstdint>
#include <common.h>
#include <vector>
#include <array>

enum class DataGroup:uint32_t {
    GFX = 0x01,
    Collision = 0x06,
    DynamicCollision = 0x08,
    GraphicsJointData = 0x011F,
    TargetPosition = 0x09,
    InfiniteWallCollision = 0x0101,
    CollisionFooter = 0x0104,
    Unknown = 0xFFFFFFFF
};

struct DataGroupHeader {
    uint32_t metadataOffset = 0;
    uint32_t payloadOffset = 0;
    uint32_t payloadSize = 0;

    Vector3I pos;

    uint32_t typeRaw = 0;
    DataGroup type = DataGroup::Unknown;

    uint8_t collisionType = 0;
    uint8_t flags = 0;

    bool LOD = (flags & 0x04);
};

struct MeshVert {
    Vector3S pos;
    Vector2u8 UV;
    Vector2F UVN;

    RGBu8 colour;
};

struct MeshTri {
    MeshVert v0;
    MeshVert v1;
    MeshVert v2;

    uint8_t flag = 0;
    uint8_t texPageByte = 0;
    uint8_t texPage = 0;
    uint8_t matIndex = 0;
};

struct GFXMesh {
    DataGroupHeader header;
    std::vector<MeshTri> triangles;
    bool LOD = false;
    uint32_t lodOffset = 0;
};

struct CollisionPoly {
    std::array<int16_t, 4> activeArea{};
    
    Vector3S origin;
    Vector3S p2;
    Vector3S p3;
    Vector3S p4;

    std::array<int16_t, 2> inclination1{};
    int16_t bouncing1 = 0;

    std::array<int16_t, 2> inclination2{};
    int16_t bouncing2 = 0;

    bool isTriangle() const {
        return static_cast<uint16_t>(bouncing2) == 0x7FFF;
    }
};

struct CollisionBlock {
    int16_t enabled = 0;
    int16_t polyCount = 0;
    std::array<int16_t, 4> unk{};
    std::vector<CollisionPoly> polygons;
};

struct CollisionMesh {
    DataGroupHeader header;
    uint8_t collisionType = 0;
    std::vector<CollisionBlock> blocks;

};

struct AllFile {
    uint32_t metadataOffset = 0;
    uint32_t dataGroupCount = 0;

    std::vector<DataGroupHeader> dataGroups;
    std::vector<GFXMesh> gfxMeshes;
    std::vector<CollisionMesh> collisionMeshes;
};