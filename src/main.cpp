#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <fstream>
#include <filesystem>
#include <print>
#include <vector>
#include "structs.h"

std::vector<Texture> returnImages();
std::filesystem::path OpenFileDialog();
void readNGN(std::ifstream& file);
RawReadResult readRAW(const std::filesystem::path& inputFilePath);

static std::vector<Texture> textures;

void openNGN() {
	if (ImGui::Button("Open NGN")){
		std::filesystem::path ngnPath = OpenFileDialog();
		std::ifstream file(ngnPath, std::ios::binary);
		readNGN(file);
		textures = returnImages();
		std::println("returned {} images",textures.size());
	}
	return;
}

void openRAW() {
	if (ImGui::Button("Open RAW")){
	std::filesystem::path rawPath = OpenFileDialog();

	RawReadResult raw = readRAW(rawPath);

	std::vector<CreatureRam>& creatures = raw.creatures;
	std::vector<RawPacket>& rawPackets = raw.rawPackets;

	std::println("returned {} creatures",creatures.size());
	//for(int i=0; i< creatures.size(); i++){
	//	std::println("{}",creatures[i].creatureId);
	//}
	}
	return;
}

int main()
{
	if (! glfwInit())
		return 1;

	const char* glsl_version = "#version 130";

	GLFWwindow* window = glfwCreateWindow(1280, 720, "ToyBox", nullptr, nullptr);

	if (! window)
	{
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	while (! glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		openNGN(); // select NGN and parse
		openRAW();

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		if(textures.size() > 0){
			for(int i = 0; i < textures.size(); i++){
				//std::println("{}",i);
				ImGui::Image(textures[i].image, ImVec2(textures[i].x,textures[i].y)); // if textures have been extracted, display them all
			}
			
		}		

		// Render
		ImGui::Render();

		int32_t width, height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}