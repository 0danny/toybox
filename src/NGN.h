#pragma once
#include <string>
#include <vector>
#include <GLFW/glfw3.h>

struct Texture {
	std::string name;
    GLuint image;
	int x;
	int y;
};

struct Vertex {
	float x, y, z;       // Position
	float nx, ny, nz;    // Normal
	float u, v;          // UV
	float r, g, b, a;    // Color
};

struct Prim {
	std::vector<uint16_t> indices;
	GLuint vao, vbo, ebo;
	GLuint textureId; // set
};

struct Shape {
	std::string shapeName; // set
	std::vector<std::string> textureIndex; // set
	std::vector<int> materialIndex; // set
	std::vector<Vertex> vertices; // set
	std::vector<Prim> primitives; // "set"
};