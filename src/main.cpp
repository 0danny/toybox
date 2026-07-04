#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <fstream>
#include <filesystem>
#include <print>
#include <vector>

#include "RAW.h"
#include "NGN.h"
#include "All.h"

std::vector<Texture> returnImages();
std::filesystem::path OpenFileDialog(const std::wstring& extension);
void readNGN(std::ifstream& file);
AllFile readALL(std::ifstream& file);
RawReadResult readRAW(const std::filesystem::path& inputFilePath);

static std::vector<Texture> textures;
std::vector<rawTexture> texPackets;

void openNGN() {
	if (ImGui::Button("Open NGN")){
		std::filesystem::path ngnPath = OpenFileDialog(L".ngn");
		std::ifstream file(ngnPath, std::ios::binary);
		readNGN(file);
		textures = returnImages();
		std::println("returned {} images",textures.size());
	}
	return;
}

void openRAW() {
	if (ImGui::Button("Open RAW")){
	std::filesystem::path rawPath = OpenFileDialog(L".raw");

	RawReadResult raw = readRAW(rawPath);
	
	texPackets = raw.texPackets;
	std::vector<CreatureRam>& creatures = raw.creatures;
	std::vector<RawPacket>& anmPackets = raw.anmPackets;
	std::vector<RawPacket>& allPackets = raw.allPackets;
	
	
	std::println("\nreturned {} textures",texPackets.size());
	std::println("returned {} creatures",creatures.size());
	std::println("returned {} .anm files",anmPackets.size());
	std::println("returned {} .all files",allPackets.size());
	//for(int i=0; i< creatures.size(); i++){
	//	std::println("{}",creatures[i].creatureId);
	//}
	}
	return;
}

void openALL() {
	if (ImGui::Button("Open ALL")){
		std::filesystem::path allPath = OpenFileDialog(L".all");
		std::ifstream file(allPath, std::ios::binary);
		AllFile all = readALL(file);
	}
	return;
}

int main()
{
	if (! glfwInit())
		return 1;

	const char* glsl_version = "#version 130";

	GLFWwindow* window = glfwCreateWindow(1280, 1000, "ToyBox", nullptr, nullptr);

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
		openALL();

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		if(textures.size() > 0){
			for(int i = 0; i < textures.size(); i++){
				ImGui::Text("%s - %dx%d", textures[i].name.c_str(), textures[i].x, textures[i].y);
				ImGui::Image(textures[i].image, ImVec2(textures[i].x*1.5,textures[i].y*1.5)); // if textures have been extracted, display them all
			}
		}

		if(texPackets.size() > 0){
			for(int i = 0; i < texPackets.size(); i++){
				ImGui::Text("tex%d - %dx%d", i, texPackets[i].width, texPackets[i].height);
				ImGui::Image(texPackets[i].image, ImVec2(texPackets[i].width*1.5,texPackets[i].height*1.5)); // if textures have been extracted, display them all
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