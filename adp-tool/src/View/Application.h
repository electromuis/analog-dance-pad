#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "imgui.h"

#include <View/Image.h>

struct GLFWwindow;

namespace Walnut {

	class Application
	{
	public:
		Application(const char* title);
		~Application();

		void Run();
		void RunOnce();
		void Close();
		void Loop();

		virtual void MenuCallback() = 0;
		virtual void RenderCallback() = 0;

		static void SubmitResourceFree(std::function<void()>&& func);

	protected:
		std::unique_ptr<Walnut::Image> m_Icon64;

	private:
		bool Init(const char* title);
		void Shutdown();

	private:
		GLFWwindow* m_WindowHandle = nullptr;
		bool m_Running = false;
	};
}
