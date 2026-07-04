#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <print>

#include "globals.hpp"

static GLuint compileShader(GLenum type, const char* source) {
	GLuint shader = glCreateShader(type);

	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		char log[1024];
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		std::println("Shader compile error: {}", log);
	}

	return shader;
}

GLuint createCollisionShader() {
	const char* vertexSource = R"(
		#version 130

		in vec3 aPos;

		uniform mat4 uMVP;

		void main() {
			gl_Position = uMVP * vec4(aPos, 1.0);
		}
	)";

	const char* fragmentSource = R"(
		#version 130

		out vec4 FragColor;

		void main() {
			FragColor = vec4(0.1, 1.0, 0.2, 1.0);
		}
	)";

	GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glBindAttribLocation(program, 0, "aPos");

	glLinkProgram(program);

	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char log[1024];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		std::println("Shader link error: {}", log);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

void destroyCollisionRenderer() {
	if (collisionRenderer.vbo != 0) {
		glDeleteBuffers(1, &collisionRenderer.vbo);
		collisionRenderer.vbo = 0;
	}

	if (collisionRenderer.vao != 0) {
		glDeleteVertexArrays(1, &collisionRenderer.vao);
		collisionRenderer.vao = 0;
	}

	if (collisionRenderer.shader != 0) {
		glDeleteProgram(collisionRenderer.shader);
		collisionRenderer.shader = 0;
	}

	collisionRenderer.vertexCount = 0;
}