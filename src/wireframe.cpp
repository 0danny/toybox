#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <print>
#include <vector>
#include <algorithm>
#include <limits>
#include <array>
#include <cmath>

#include "ALL.hpp"

#include "wireframe.hpp"
#include "globals.hpp"

GLuint createCollisionShader();

using Mat4 = std::array<float, 16>;

float radians(float degrees)
{
	return degrees * 3.1415926535f / 180.0f;
}

Vec3f add(Vec3f a, Vec3f b)
{
	return Vec3f { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec3f subtract(Vec3f a, Vec3f b)
{
	return Vec3f { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3f multiply(Vec3f v, float s)
{
	return Vec3f { v.x * s, v.y * s, v.z * s };
}

float dot(Vec3f a, Vec3f b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3f cross(Vec3f a, Vec3f b)
{
	return Vec3f { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

Vec3f normalize(Vec3f v)
{
	float len = std::sqrt(dot(v, v));

	if (len <= 0.00001f)
	{
		return Vec3f { 0.0f, 0.0f, 0.0f };
	}

	return Vec3f { v.x / len, v.y / len, v.z / len };
}

Vec3f getCameraForward()
{
	float yawRad = radians(camera.yaw);
	float pitchRad = radians(camera.pitch);

	Vec3f forward;

	forward.x = std::cos(yawRad) * std::cos(pitchRad);
	forward.y = std::sin(pitchRad);
	forward.z = std::sin(yawRad) * std::cos(pitchRad);

	return normalize(forward);
}

Vec3f getCameraRight()
{
	Vec3f forward = getCameraForward();
	Vec3f worldUp { 0.0f, 1.0f, 0.0f };

	return normalize(cross(forward, worldUp));
}
Mat4 identityMatrix()
{
	return Mat4 { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
}

Mat4 multiplyMatrix(const Mat4& a, const Mat4& b)
{
	Mat4 result {};

	// Column-major matrix multiplication for OpenGL.
	for (int col = 0; col < 4; col++)
	{
		for (int row = 0; row < 4; row++)
		{
			result[col * 4 + row] =
				a[0 * 4 + row] * b[col * 4 + 0] + a[1 * 4 + row] * b[col * 4 + 1] + a[2 * 4 + row] * b[col * 4 + 2] + a[3 * 4 + row] * b[col * 4 + 3];
		}
	}

	return result;
}

Mat4 perspectiveMatrix(float fovDegrees, float aspect, float nearPlane, float farPlane)
{
	float fovRad = radians(fovDegrees);
	float f = 1.0f / std::tan(fovRad * 0.5f);

	Mat4 m {};

	m[0] = f / aspect;
	m[5] = f;
	m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
	m[11] = -1.0f;
	m[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);

	return m;
}

Mat4 lookAtMatrix(Vec3f eye, Vec3f target, Vec3f up)
{
	Vec3f f = normalize(subtract(target, eye));
	Vec3f s = normalize(cross(f, up));
	Vec3f u = cross(s, f);

	Mat4 m = identityMatrix();

	m[0] = s.x;
	m[4] = s.y;
	m[8] = s.z;

	m[1] = u.x;
	m[5] = u.y;
	m[9] = u.z;

	m[2] = -f.x;
	m[6] = -f.y;
	m[10] = -f.z;

	m[12] = -dot(s, eye);
	m[13] = -dot(u, eye);
	m[14] = dot(f, eye);

	return m;
}

static Mat4 createModelMatrix()
{
	Mat4 model = identityMatrix();

	model[0] = collisionRenderer.scale;
	model[5] = -collisionRenderer.scale; // flip Y
	model[10] = -collisionRenderer.scale;

	model[12] = -collisionRenderer.centerX * collisionRenderer.scale;
	model[13] = collisionRenderer.centerY * collisionRenderer.scale; // sign changes because Y is flipped
	model[14] = collisionRenderer.centerZ * collisionRenderer.scale;

	return model;
}

static Mat4 createMVPMatrix(int32_t width, int32_t height)
{
	float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;

	Mat4 projection = perspectiveMatrix(70.0f, aspect, 0.01f, 100.0f);

	Vec3f forward = getCameraForward();
	Vec3f target = add(camera.position, forward);

	Mat4 view = lookAtMatrix(camera.position, target, Vec3f { 0.0f, 1.0f, 0.0f });

	Mat4 model = createModelMatrix();

	return multiplyMatrix(projection, multiplyMatrix(view, model));
}

void updateCamera(GLFWwindow* window, float deltaTime)
{
	Vec3f forward = getCameraForward();
	Vec3f right = getCameraRight();

	// Keep WASD movement horizontal.
	forward.y = 0.0f;
	forward = normalize(forward);

	float moveAmount = camera.moveSpeed * deltaTime;
	float lookAmount = camera.lookSpeed * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		camera.position = add(camera.position, multiply(forward, moveAmount));
	}

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera.position = subtract(camera.position, multiply(forward, moveAmount));
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		camera.position = subtract(camera.position, multiply(right, moveAmount));
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		camera.position = add(camera.position, multiply(right, moveAmount));
	}

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		camera.position.y = camera.position.y + moveAmount;
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		camera.position.y = camera.position.y - moveAmount;
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		camera.yaw -= lookAmount;
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		camera.yaw += lookAmount;
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		camera.pitch += lookAmount;
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		camera.pitch -= lookAmount;
	}

	camera.pitch = std::clamp(camera.pitch, -89.0f, 89.0f);
}

static void pushPoint(std::vector<float>& vertices, const Vector3S& p, const Vector3I& meshOffset)
{
	vertices.push_back(static_cast<float>(p.x) + static_cast<float>(meshOffset.x));
	vertices.push_back(static_cast<float>(p.y) + static_cast<float>(meshOffset.y));
	vertices.push_back(static_cast<float>(p.z) + static_cast<float>(meshOffset.z));
}

static void pushLine(std::vector<float>& vertices, const Vector3S& a, const Vector3S& b, const Vector3I& meshOffset)
{
	pushPoint(vertices, a, meshOffset);
	pushPoint(vertices, b, meshOffset);
}

static std::vector<float> buildCollisionWireframeVertices(const AllFile& all)
{
	std::vector<float> vertices;

	for (const CollisionMesh& mesh : all.collisionMeshes)
	{
		const Vector3I& meshOffset = mesh.header.pos;

		for (const CollisionBlock& block : mesh.blocks)
		{
			if (block.enabled != 1)
			{
				continue;
			}

			for (const CollisionPoly& poly : block.polygons)
			{
				const Vector3S& origin = poly.origin;
				const Vector3S& p2 = poly.p2;
				const Vector3S& p3 = poly.p3;
				const Vector3S& p4 = poly.p4;

				if (poly.isTriangle())
				{
					// entv.html triangle line order:
					// p2 -> origin -> p3 -> p2
					pushLine(vertices, p2, origin, meshOffset);
					pushLine(vertices, origin, p3, meshOffset);
					pushLine(vertices, p3, p2, meshOffset);
				}
				else
				{
					// entv.html quad split:
					// tri 1: [origin, p2, p3]
					// tri 2: [p4, p3, p2]
					//
					// Outer outline:
					// p2 -> origin
					// origin -> p3
					// p3 -> p4
					// p4 -> p2
					pushLine(vertices, p2, origin, meshOffset);
					pushLine(vertices, origin, p3, meshOffset);
					pushLine(vertices, p3, p4, meshOffset);
					pushLine(vertices, p4, p2, meshOffset);

					// Internal diagonal, equivalent to entv.html's grey line.
					// You can remove this if you only want the outer outline.
					pushLine(vertices, p3, p2, meshOffset);
				}
			}
		}
	}

	return vertices;
}

void uploadCollisionWireframe(const AllFile& all)
{
	std::vector<float> vertices = buildCollisionWireframeVertices(all);

	collisionRenderer.vertexCount = static_cast<GLsizei>(vertices.size() / 3);

	if (vertices.empty())
	{
		std::println("No collision wireframe vertices to upload");
		return;
	}

	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	float minZ = std::numeric_limits<float>::max();

	float maxX = std::numeric_limits<float>::lowest();
	float maxY = std::numeric_limits<float>::lowest();
	float maxZ = std::numeric_limits<float>::lowest();

	for (size_t i = 0; i < vertices.size(); i += 3)
	{
		float x = vertices[i + 0];
		float y = vertices[i + 1];
		float z = vertices[i + 2];

		minX = std::min(minX, x);
		minY = std::min(minY, y);
		minZ = std::min(minZ, z);

		maxX = std::max(maxX, x);
		maxY = std::max(maxY, y);
		maxZ = std::max(maxZ, z);
	}

	collisionRenderer.centerX = (minX + maxX) * 0.5f;
	collisionRenderer.centerY = (minY + maxY) * 0.5f;
	collisionRenderer.centerZ = (minZ + maxZ) * 0.5f;

	float sizeX = maxX - minX;
	float sizeY = maxY - minY;
	float sizeZ = maxZ - minZ;

	float maxSize = std::max(sizeX, std::max(sizeY, sizeZ));

	collisionRenderer.scale = 0.002f;

	if (collisionRenderer.shader == 0)
	{
		collisionRenderer.shader = createCollisionShader();
	}

	if (collisionRenderer.vao == 0)
	{
		glGenVertexArrays(1, &collisionRenderer.vao);
	}

	if (collisionRenderer.vbo == 0)
	{
		glGenBuffers(1, &collisionRenderer.vbo);
	}

	glBindVertexArray(collisionRenderer.vao);

	glBindBuffer(GL_ARRAY_BUFFER, collisionRenderer.vbo);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	std::println("Uploaded collision wireframe:");
	std::println("  vertices: {}", collisionRenderer.vertexCount);
	std::println("  lines: {}", collisionRenderer.vertexCount / 2);
	std::println("  center: {}, {}, {}", collisionRenderer.centerX, collisionRenderer.centerY, collisionRenderer.centerZ);
	std::println("  scale: {}", collisionRenderer.scale);
}

void drawCollisionWireframe(int32_t width, int32_t height)
{
	if (collisionRenderer.vao == 0 || collisionRenderer.shader == 0 || collisionRenderer.vertexCount == 0)
	{
		return;
	}

	Mat4 mvp = createMVPMatrix(width, height);

	glUseProgram(collisionRenderer.shader);

	GLint mvpLocation = glGetUniformLocation(collisionRenderer.shader, "uMVP");
	glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvp.data());

	glBindVertexArray(collisionRenderer.vao);

	glEnable(GL_DEPTH_TEST);
	glLineWidth(0.5f);

	glDrawArrays(GL_LINES, 0, collisionRenderer.vertexCount);

	glBindVertexArray(0);
	glUseProgram(0);
}