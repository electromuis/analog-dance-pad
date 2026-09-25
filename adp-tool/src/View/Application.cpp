#include <View/Application.h>

#include <View/UserConfig.h>

// #define GLFW_INCLUDE_ES3

//#include <gl3.h>
// #define GLFW_INCLUDE_ES3
//#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>

// #define GLFW_INCLUDE_ES3
// #include <GLES3/gl3.h>
// #include <GLFW/glfw3.h>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#ifdef __EMSCRIPTEN__
int glfwGetGamepadState (int jid, GLFWgamepadstate *state)
{
	return 0;
}

#include <emscripten.h>
#endif

#include "stb_image.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <iostream>

// Emedded assets
#include <Assets/Roboto-Regular.inl>
#include <Assets/icon16.png.inl>
#include <Assets/icon32.png.inl>
#include <Assets/icon64.png.inl>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

namespace Walnut {

Application::Application(const char* title)
{
	if(!Init(title)) {
		std::cerr << "Init failed!\n";
	}
}

Application::~Application()
{
	Shutdown();
}

bool Application::Init(const char* title)
{
	adp::UserConfig::Init();
	int width = adp::UserConfig::WindowWidth;
	int height = adp::UserConfig::WindowHeight;

	// Setup GLFW window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		std::cerr << "Could not initalize GLFW!\n";
		return false;
	}
	
#ifdef __EMSCRIPTEN__
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
	const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

	m_WindowHandle = glfwCreateWindow(width, height, title, NULL, NULL);
	if(m_WindowHandle == NULL) {
		std::cerr << "Failed creating window!\n";
		return false;
	}

	glfwMakeContextCurrent(m_WindowHandle);

#ifndef __EMSCRIPTEN__
    glfwSwapInterval(1); // Enable vsync
#endif
	

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	// io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	if(!ImGui_ImplGlfw_InitForOpenGL(m_WindowHandle, true)) {
		std::cerr << "Could not initalize OpenGL (1)!\n";
		return false;
	}

	if(!ImGui_ImplOpenGL3_Init()) {
		std::cerr << "Could not initalize OpenGL (2)!\n";
		return false;
	}
	
	// Load icons
	GLFWimage icons[3] = {};
	icons[0].pixels = stbi_load_from_memory(Icon16, sizeof(Icon16), &icons[0].width, &icons[0].height, nullptr, 4);
	icons[1].pixels = stbi_load_from_memory(Icon32, sizeof(Icon32), &icons[1].width, &icons[1].height, nullptr, 4);
	icons[2].pixels = stbi_load_from_memory(Icon64, sizeof(Icon64), &icons[2].width, &icons[2].height, nullptr, 4);
	glfwSetWindowIcon(m_WindowHandle, 3, icons);

	 m_Icon64 = std::make_unique<Image>(
	 	icons[2].width,
	 	icons[2].height,
	 	Walnut::ImageFormat::RGBA,
	 	icons[2].pixels
	);

	stbi_image_free(icons[0].pixels);
	stbi_image_free(icons[1].pixels);
	stbi_image_free(icons[2].pixels);

	// Load default font
	ImFontConfig fontConfig;
	fontConfig.FontDataOwnedByAtlas = false;
	ImFont* robotoFont = io.Fonts->AddFontFromMemoryTTF((void*)g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &fontConfig);
	io.FontDefault = robotoFont;

	return true;
}

void Application::Shutdown()
{
	m_Icon64.reset();

	// Save the window size in user config so it will be
	// the same size on next launch
	glfwGetWindowSize(
			m_WindowHandle,
			&adp::UserConfig::WindowWidth,
			&adp::UserConfig::WindowHeight);
	adp::UserConfig::SaveToDisk();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(m_WindowHandle);
	glfwTerminate();
}

void Application::Loop()
{
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ImGuiIO& io = ImGui::GetIO();

	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	glfwPollEvents();

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_MenuBar;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("FSR Minipad", nullptr, window_flags);
		ImGui::PopStyleVar();

		ImGui::PopStyleVar(2);

		// Submit the DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("OpenGLAppDockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::BeginMenuBar())
		{
			MenuCallback();
			ImGui::EndMenuBar();
		}

		RenderCallback();

		ImGui::End();
	}

	// Rendering
	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(m_WindowHandle, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glfwSwapBuffers(m_WindowHandle);
}

void Application::Run()
{
	m_Running = true;

	// Main loop
	while (!glfwWindowShouldClose(m_WindowHandle) && m_Running)
	{
		Loop();
	}

}

void Application::Close()
{
	m_Running = false;
}

}; // namespace Walnut

// Entry point.

namespace adp { extern int Main(int argc, char** argv); }

#ifdef WL_DIST
#include <Windows.h>
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	return RunApplication(__argc, __argv);
}
#else
int main(int argc, char** argv)
{
	return adp::Main(argc, argv);
}
#endif
