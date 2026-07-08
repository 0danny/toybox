#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <fstream>
#include <filesystem>
#include <print>
#include <vector>
#include <optional>
#include <array>

#include "Parsers/ALLParser.hpp"
#include "Parsers/RAWParser.hpp"
#include "Parsers/NGNParser.hpp"
#include "Parsers/TextureParser.hpp"

#include "FileUtils.hpp"
#include "Shader.hpp"
#include "Wireframe.hpp"
#include "Globals.hpp"

static std::vector<NGNParser::Texture> textures;
static std::vector<RawParser::rawTexture> texPackets;
static std::optional<ALLParser::AllFile> loadedAll;
static bool showCollisionWireframe = true;

using Mat4 = std::array<float, 16>;

void openNGN()
{
	if (ImGui::Button("Open NGN"))
	{
		std::filesystem::path ngnPath = FileUtils::OpenFileDialog(L".ngn");

		if (ngnPath.empty())
		{
			std::println("No NGN file selected");
			return;
		}

		std::ifstream file(ngnPath, std::ios::binary);

		if (! file)
		{
			std::println("Failed to open NGN file");
			return;
		}

		NGNParser::ReadNGN(file);
		textures = TextureParser::ReturnImages();

		//std::println("returned {} images", textures.size());
	}

	return;
}

void openRAW()
{
	if (ImGui::Button("Open RAW"))
	{
		std::filesystem::path rawPath = FileUtils::OpenFileDialog(L".raw");

		if (rawPath.empty())
		{
			std::println("No RAW file selected");
			return;
		}

		RawParser::RawReadResult raw = RawParser::ReadRAW(rawPath);

		texPackets = raw.texPackets;
		std::vector<RawParser::CreatureRam>& creatures = raw.creatures;
		std::vector<RawParser::RawPacket>& anmPackets = raw.anmPackets;
		std::vector<RawParser::RawPacket>& allPackets = raw.allPackets;

		std::println("\nreturned {} textures", texPackets.size());
		std::println("returned {} creatures", creatures.size());
		std::println("returned {} .anm files", anmPackets.size());
		std::println("returned {} .all files", allPackets.size());
	}

	return;
}

void openALL()
{
	if (ImGui::Button("Open ALL"))
	{
		std::filesystem::path allPath = FileUtils::OpenFileDialog(L".all");

		if (allPath.empty())
		{
			std::println("No ALL file selected");
			return;
		}

		std::ifstream file(allPath, std::ios::binary);

		if (! file)
		{
			std::println("Failed to open ALL file");
			return;
		}

		loadedAll = ALLParser::ReadALL(file);

		std::println("loaded ALL:");
		std::println("  collision meshes: {}", loadedAll->collisionMeshes.size());
		std::println("  graphics meshes: {}", loadedAll->gfxMeshes.size());

		Wireframe::UploadCollisionWireframe(*loadedAll);
	}

	return;
}

int main()
{
	if (! glfwInit())
	{
		return 1;
	}

	const char* glsl_version = "#version 130";

	GLFWwindow* window = glfwCreateWindow(1280, 1000, "ToyBox", nullptr, nullptr);

	if (! window)
	{
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (! gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		std::println("Failed to initialize GLAD");
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	glEnable(GL_DEPTH_TEST);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	double lastTime = glfwGetTime();

	while (! glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		double currentTime = glfwGetTime();
		float deltaTime = static_cast<float>(currentTime - lastTime);
		lastTime = currentTime;

		Wireframe::UpdateCamera(window, deltaTime);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		openNGN();
		openRAW();
		openALL();

		ImGui::Separator();

		ImGui::Checkbox("Show collision wireframe", &showCollisionWireframe);

		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		ImGui::Text("Camera: %.2f %.2f %.2f", camera.position.x, camera.position.y, camera.position.z);

		ImGui::Text("Yaw/Pitch: %.1f %.1f", camera.yaw, camera.pitch);

		ImGui::Text("Controls:");
		ImGui::Text("  W/S = move forward/back");
		ImGui::Text("  A/D = strafe left/right");
		ImGui::Text("  Q/E = move up/down");
		ImGui::Text("  Arrow keys = look around");

		if (loadedAll.has_value())
		{
			ImGui::Separator();

			ImGui::Text("ALL loaded");
			ImGui::Text("Collision meshes: %zu", loadedAll->collisionMeshes.size());
			ImGui::Text("Graphics meshes: %zu", loadedAll->gfxMeshes.size());
			ImGui::Text("Collision line vertices: %d", collisionRenderer.vertexCount);
			ImGui::Text("Collision lines: %d", collisionRenderer.vertexCount / 2);

			ImGui::Text("Collision center: %.2f %.2f %.2f", collisionRenderer.centerX, collisionRenderer.centerY, collisionRenderer.centerZ);

			ImGui::Text("Collision scale: %.6f", collisionRenderer.scale);
		}

		if (textures.size() > 0)
		{
			ImGui::Separator();
			ImGui::Text("NGN Textures:");

			for (int i = 0; i < static_cast<int>(textures.size()); i++)
			{
				ImGui::Text("%s - %dx%d", textures[i].name.c_str(), textures[i].x, textures[i].y);

				ImGui::Image(textures[i].image, ImVec2(textures[i].x * 1.5f, textures[i].y * 1.5f));
			}
		}

		if (texPackets.size() > 0)
		{
			ImGui::Separator();
			ImGui::Text("RAW Textures:");

			for (int i = 0; i < static_cast<int>(texPackets.size()); i++)
			{
				ImGui::Text("tex%d - %dx%d", i, texPackets[i].width, texPackets[i].height);

				ImGui::Image(texPackets[i].image, ImVec2(texPackets[i].width * 1.5f, texPackets[i].height * 1.5f));
			}
		}

		ImGui::Render();

		int32_t width, height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (showCollisionWireframe)
		{
			Wireframe::DrawCollisionWireframe(width, height);
		}

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	Shader::DestroyCollisionRenderer();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}