#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "imgui.h"
#include "vulkan/vulkan.h"

#include <View/Image.h>

void check_vk_result(VkResult err);

struct GLFWwindow;

namespace Walnut {

	class Application
	{
	public:
		Application(int width, int height, const char* title);
		~Application();

		void Run();
		void Close();

		virtual void MenuCallback() = 0;
		virtual void RenderCallback() = 0;

		static VkInstance GetInstance();
		static VkPhysicalDevice GetPhysicalDevice();
		static VkDevice GetDevice();

		static VkCommandBuffer GetCommandBuffer();
		static void FlushCommandBuffer(VkCommandBuffer commandBuffer);

		static void SubmitResourceFree(std::function<void()>&& func);

	protected:
		std::unique_ptr<Walnut::Image> m_Icon64;

	private:
		void Init(int width, int height, const char* title);
		void Shutdown();

	private:
		GLFWwindow* m_WindowHandle = nullptr;
		bool m_Running = false;
	};
}