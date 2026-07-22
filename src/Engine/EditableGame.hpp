#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define FE_EXCLUDE_GLFW

#include <iostream>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_vulkan.h>

#include "EditableGameBase.hpp"

namespace fe {
  
	class EditableGame : public EditableGameBase {

	public:
		EditableGame(int width, int height, bool vr = false, bool showWindow = true) : EditableGameBase(width, height, vr, showWindow) {
			Init();
		}

		EditableGame(XRGameOptions options) : EditableGameBase(options) {
			Init();
		}
		
	private:
		void Init() {
			InitImGUI();
			InitUI();
			bool themed = true;
			if (themed) ApplyBlackAndOrangeTheme();
		}

		ImGuiIO io;

		bool physicsGravityEnabled = true;

		void InitImGUI() {
			// rENDE
			auto renderer = (Renderer*)this;
			fe::SDLWindow *window = (fe::SDLWindow*)renderer->window.get();
			const char* glsl_version = "#version 330 core";
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

			ImGui::StyleColorsDark();

			float scale = SDL_GetWindowDisplayScale(window->GetWindow());
			if (scale > 1.0f) {
				io.FontGlobalScale = scale;
				ImGui::GetStyle().ScaleAllSizes(scale);
			}

			if (!useVulkan)ImGui_ImplSDL3_InitForOpenGL(window->GetWindow(), window->GetSDLGLContext());
			else ImGui_ImplSDL3_InitForVulkan(window->GetWindow());
			if (!useVulkan)ImGui_ImplOpenGL3_Init(glsl_version);
			else {
				auto* vkDevice = dynamic_cast<VulkanDevice*>(renderer->renderDevice.get());
				if (!vkDevice) return;

				VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
				VkDescriptorPoolCreateInfo poolInfo{};
				poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
				poolInfo.maxSets = 1;
				poolInfo.poolSizeCount = 1;
				poolInfo.pPoolSizes = &poolSize;

				VkDescriptorPool imguiPool = VK_NULL_HANDLE;
				if (vkCreateDescriptorPool(vkDevice->GetDevice(), &poolInfo, nullptr, &imguiPool) != VK_SUCCESS) {
					std::cerr << "[EditableGame] Failed to create ImGui descriptor pool" << std::endl;
					return;
				}

				ImGui_ImplVulkan_InitInfo init_info = {};
				init_info.ApiVersion = VK_API_VERSION_1_2; // Of VK_API_VERSION_1_3 afhankelijk van je setup
				init_info.Instance = vkDevice->GetInstance();
				init_info.PhysicalDevice = vkDevice->GetPhysicalDevice();
				init_info.Device = vkDevice->GetDevice();
				init_info.QueueFamily = vkDevice->GetGraphicsQueueFamily();
				init_info.Queue = vkDevice->GetGraphicsQueue();
				init_info.DescriptorPool = imguiPool;
				init_info.MinImageCount = static_cast<uint32_t>(vkDevice->GetSwapChainImageCount());
				init_info.ImageCount = static_cast<uint32_t>(vkDevice->GetSwapChainImageCount());

				init_info.PipelineInfoMain.RenderPass = vkDevice->GetRenderPass();
				init_info.PipelineInfoMain.Subpass = 0; 
				init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT; 

				ImGui_ImplVulkan_Init(&init_info);


				// ImGui_ImplVulkan_CreateFontsTexture();
			}
		}

	public:

		void OnDraw() override;

		void BeginFrame() {
			if (!useVulkan) ImGui_ImplOpenGL3_NewFrame();
			else ImGui_ImplVulkan_NewFrame();
			ImGui_ImplSDL3_NewFrame();
			ImGui::NewFrame();
		}

		void EndFrame() {
			ImGui::Render();
			if (useVulkan) {
				auto renderer = (Renderer*)this;
				auto* vkDevice = dynamic_cast<VulkanDevice*>(renderer->renderDevice.get());
				if (vkDevice)
					ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkDevice->GetCurrentCommandBuffer());
			}
			else ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		void DrawDebugUI();

	#ifdef USE_VISUALIZER
			void DrawAudioVisualizerUI() {
			}
	#endif

		void DrawNetworkDebugUI() {
			ImGui::Begin("Multiplayer");
			{
				static char usernameBuffer[32] = "Bill\0";
				static char addressBuffer[256] = "127.0.0.1\0";
				int port = 2130;

				ImGui::InputText("Username", usernameBuffer, IM_ARRAYSIZE(usernameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::InputText("Address", addressBuffer, IM_ARRAYSIZE(addressBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::InputInt("Port", &port);

				if (ImGui::Button("Join", ImVec2(60, 0))) {
					std::cout << "Connecting to server... " << addressBuffer << std::endl;
					this->connectToServer(addressBuffer, port, usernameBuffer);
				}

				fe::Object* model = this->player.get();
				ImGui::SliderFloat3("Position", &model->state.position.x, -10.0f, 10.0f);

				ImGui::Text("Players:");
				for (auto& [id, client] : this->client->clientClients) {
					ImGui::Text("Player #%i username: %s", id, client.username.c_str());
				}
			}
			ImGui::End();

		

			ImGui::Begin("Chat");
			{
				static char inputBuffer[256] = "";
				ImGui::BeginChild("ChatHistory", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), true, ImGuiWindowFlags_HorizontalScrollbar);

				for (const auto& msg : messages) {
					ImGui::TextWrapped("%s", msg.c_str());
				}

				// Auto-scroll to bottom if new messages
				if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
					ImGui::SetScrollHereY(1.0f);
				}

				ImGui::EndChild();

				ImGui::Separator();

				ImGui::PushItemWidth(-70);
				bool enter_pressed = ImGui::InputText("##Input", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::PopItemWidth();

				ImGui::SameLine();

				bool send_clicked = ImGui::Button("Send", ImVec2(60, 0));

				if (send_clicked || enter_pressed) {
					if (inputBuffer[0] != '\0') {
					messages.push_back(std::string("You: ") + inputBuffer);

		#ifdef FE_WIN32

				client->sendMessage(inputBuffer);
				#endif

				inputBuffer[0] = '\0';
				ImGui::SetKeyboardFocusHere(-1);
				}
			}
		}
		ImGui::End();
		}

	#ifdef USE_VISUALIZER
		void DrawAudioVisualizerUI() {
			ImGui::SetNextWindowSize(ImVec2(440, 240), ImGuiCond_FirstUseEver);
			ImGui::Begin("Audio Spectrum");

			ImVec2 canvasPos  = ImGui::GetCursorScreenPos();
			ImVec2 canvasSize = ImGui::GetContentRegionAvail();
			canvasSize.y = std::max(canvasSize.y, 80.0f);

			ImDrawList* draw = ImGui::GetWindowDrawList();
			draw->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(18, 18, 18, 255));

			float barGap   = 2.0f;
			float barWidth = (canvasSize.x - barGap * (NUM_BARS - 1)) / NUM_BARS;

			for (int i = 0; i < NUM_BARS; ++i) {
				float ah = visualizer.bandMagnitudesSmoothed[i] * visualizerScale;
				float normalized = std::clamp(ah, 0.0f, 1.0f);
				float barHeight  = normalized * canvasSize.y;

				float x0 = canvasPos.x + i * (barWidth + barGap);
				ImVec2 barMin(x0, canvasPos.y + canvasSize.y - barHeight);
				ImVec2 barMax(x0 + barWidth, canvasPos.y + canvasSize.y);

				ImU32 color = IM_COL32(60 + (int)(195 * normalized), 140, 255 - (int)(140 * normalized), 255);
				draw->AddRectFilled(barMin, barMax, color);
			}

			ImGui::Dummy(canvasSize);
			ImGui::End();
		}
	#endif

		void ApplyBlackAndOrangeTheme() {
			ImGuiStyle& style = ImGui::GetStyle();
			ImVec4* colors = style.Colors;

			// --- COLOR VARIABLES ---
			// Pure pitch blacks
			ImVec4 color_pure_black     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			ImVec4 color_trans_black    = ImVec4(0.00f, 0.00f, 0.00f, 0.95f); // Slight opacity for popups
			
			// Very dark grays for UI separation/depth
			ImVec4 color_dark_gray_1    = ImVec4(0.07f, 0.07f, 0.07f, 1.00f); 
			ImVec4 color_dark_gray_2    = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
			ImVec4 color_border_gray    = ImVec4(0.18f, 0.18f, 0.18f, 0.60f);

			// High-visibility Orange Palette
			ImVec4 color_orange_main    = ImVec4(1.00f, 0.40f, 0.00f, 1.00f); // Solid primary orange
			ImVec4 color_orange_hover   = ImVec4(1.00f, 0.50f, 0.10f, 1.00f); // Lighter orange for hover
			ImVec4 color_orange_active  = ImVec4(1.00f, 0.60f, 0.20f, 1.00f); // Brightest orange for clicks
			ImVec4 color_orange_low_a   = ImVec4(1.00f, 0.40f, 0.00f, 0.35f); // Low opacity orange for highlights

			// Text colors
			ImVec4 color_text_white     = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
			ImVec4 color_text_disabled  = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);


			// --- ASSIGNMENTS ---
			// Base Backgrounds (Actual Black)
			colors[ImGuiCol_WindowBg]             = color_pure_black;
			colors[ImGuiCol_ChildBg]              = color_pure_black;
			colors[ImGuiCol_PopupBg]              = color_trans_black;
			colors[ImGuiCol_Border]               = color_border_gray;
			colors[ImGuiCol_BorderShadow]         = color_pure_black;

			// Text & Headers
			colors[ImGuiCol_Text]                 = color_text_white;
			colors[ImGuiCol_TextDisabled]         = color_text_disabled;
			colors[ImGuiCol_Header]               = ImVec4(color_orange_main.x, color_orange_main.y, color_orange_main.z, 0.65f); 
			colors[ImGuiCol_HeaderHovered]        = color_orange_hover; 
			colors[ImGuiCol_HeaderActive]         = color_orange_active;

			// Buttons
			colors[ImGuiCol_Button]               = color_dark_gray_1; 
			colors[ImGuiCol_ButtonHovered]        = color_orange_main; 
			colors[ImGuiCol_ButtonActive]         = color_orange_active;

			// Frame Backgrounds (Inputs, Checkboxes, etc.)
			colors[ImGuiCol_FrameBg]              = color_dark_gray_2;
			colors[ImGuiCol_FrameBgHovered]       = color_orange_low_a;
			colors[ImGuiCol_FrameBgActive]        = color_orange_main;

			// Tabs
			colors[ImGuiCol_Tab]                  = color_dark_gray_1;
			colors[ImGuiCol_TabHovered]           = color_orange_hover;
			colors[ImGuiCol_TabActive]            = color_orange_main;
			colors[ImGuiCol_TabUnfocused]         = color_pure_black;
			colors[ImGuiCol_TabUnfocusedActive]  = color_dark_gray_1;

			// Title Bars
			colors[ImGuiCol_TitleBg]              = color_pure_black;
			colors[ImGuiCol_TitleBgActive]        = color_pure_black;
			colors[ImGuiCol_TitleBgCollapsed]     = color_pure_black;

			// Scrollbars & Sliders
			colors[ImGuiCol_ScrollbarBg]          = color_pure_black;
			colors[ImGuiCol_ScrollbarGrab]        = color_dark_gray_2;
			colors[ImGuiCol_ScrollbarGrabHovered] = color_orange_hover;
			colors[ImGuiCol_ScrollbarGrabActive]  = color_orange_active;
			colors[ImGuiCol_SliderGrab]           = color_orange_main;
			colors[ImGuiCol_SliderGrabActive]     = color_orange_active;

			// Widgets & Separators
			colors[ImGuiCol_CheckMark]            = color_orange_main;
			colors[ImGuiCol_ResizeGrip]           = color_dark_gray_2;
			colors[ImGuiCol_ResizeGripHovered]    = color_orange_hover;
			colors[ImGuiCol_ResizeGripActive]     = color_orange_active;
			colors[ImGuiCol_Separator]            = color_dark_gray_2;
			colors[ImGuiCol_SeparatorHovered]     = color_orange_hover;
			colors[ImGuiCol_SeparatorActive]      = color_orange_active;

			// Clean modern sizing
			style.WindowRounding = 4.0f;
			style.FrameRounding = 5.0f;
			style.GrabRounding = 3.0f;
			style.PopupRounding = 4.0f;
		}
	};
  
} // namespace fe