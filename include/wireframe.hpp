#pragma once
#include <GLFW/glfw3.h>

struct CollisionWireframeRenderer {
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint shader = 0;

	GLsizei vertexCount = 0;

	float centerX = 0.0f;
	float centerY = 0.0f;
	float centerZ = 0.0f;

	float scale = 1.0f;
};

struct Vec3f {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

struct Camera {
	Vec3f position{ 0.0f, 0.0f, 3.0f };

	float yaw = -90.0f;
	float pitch = 0.0f;

	float moveSpeed = 10.0f;
	float lookSpeed = 90.0f;
};