#include <fstream>
#include <chrono>
#include <GLFW/glfw3.h>
#include "NGN.h"
#include <print>

const std::streamsize readUInt32 = 4;
const std::streamsize readUInt8 = 1;

static std::vector<Texture> textures;

std::vector<Texture> returnImages() {
    return textures;
}

void readTextureBlock(std::ifstream& file) {
    auto start_time = std::chrono::high_resolution_clock::now();

    textures.clear();
	auto start = std::chrono::high_resolution_clock::now();

    uint32_t textureCount;

    file.read(reinterpret_cast<char*>(&textureCount), readUInt32);

    textures.resize(textureCount);

    std::vector<GLuint> textureIDs(textureCount);
    glGenTextures(textureCount, textureIDs.data());

    for (uint32_t i = 0; i < textureCount; i++) {
        file.seekg(4, std::ios::cur); // Seek textureByteLength
        uint32_t textureNameByteLength; file.read(reinterpret_cast<char*>(&textureNameByteLength), readUInt32);
        std::string textureName(textureNameByteLength, '\0');
        
        file.read(&textureName[0], textureNameByteLength);

        file.seekg(18, std::ios::cur); // Seek unnecessary bytes

        uint32_t textureX; file.read(reinterpret_cast<char*>(&textureX), readUInt32);
        uint32_t textureY; file.read(reinterpret_cast<char*>(&textureY), readUInt32);

        file.seekg(28, std::ios::cur); // Seek unnecessary bytes

        std::vector<uint8_t> bgr(textureX * textureY * 3);
        file.read(reinterpret_cast<char*>(bgr.data()), bgr.size());

        std::vector<uint8_t> rgb(textureX * textureY * 3);
        for (uint32_t y = 0; y < textureY; y++) {
            memcpy(&rgb[y * textureX * 3],
                &bgr[(textureY - 1 - y) * textureX * 3],
                textureX * 3);
            // Also swap B<->R if needed
            for (uint32_t x = 0; x < textureX; x++) {
                std::swap(rgb[(y * textureX + x) * 3 + 0], rgb[(y * textureX + x) * 3 + 2]);
            }
        }

        GLuint texID = textureIDs[i];
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureX, textureY, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
        textures[i].name = textureName;
        textures[i].image = texID;
        textures[i].x = textureX;
        textures[i].y = textureY;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::println("~Extracted {} textures in {}ms", textures.size(), std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());
}