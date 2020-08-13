#include <quantumvk.hpp>

#include <iostream>
#include <thread>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

struct GLFWPlatform : public Vulkan::WSIPlatform
{

	GLFWPlatform()
	{
		width = 1280;
		height = 720;
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		window = glfwCreateWindow(width, height, "GLFW Window", nullptr, nullptr);
	}

	virtual ~GLFWPlatform() 
	{
		if (window)
			glfwDestroyWindow(window);
	}

	virtual VkSurfaceKHR CreateSurface(VkInstance instance, VkPhysicalDevice gpu)
	{
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
			return VK_NULL_HANDLE;

		int actual_width, actual_height;
		glfwGetFramebufferSize(window, &actual_width, &actual_height);
		width = unsigned(actual_width);
		height = unsigned(actual_height);
		return surface;
	}

	virtual std::vector<const char*> GetInstanceExtensions()
	{
		uint32_t count;
		const char** ext = glfwGetRequiredInstanceExtensions(&count);
		return { ext, ext + count };
	}

	virtual uint32_t GetSurfaceWidth()
	{
		return width;
	}

	virtual uint32_t GetSurfaceHeight()
	{
		return height;
	}

	virtual bool Alive(Vulkan::WSI& wsi)
	{
		return !glfwWindowShouldClose(window);
	}

	virtual void PollInput()
	{
		glfwPollEvents();
	}

	GLFWwindow* window = nullptr;
	unsigned width = 0;
	unsigned height = 0;

};

int main() 
{
	glfwInit();

	if (!Vulkan::Context::InitLoader(nullptr))
		QM_LOG_ERROR("Failed to load vulkan dynamic library");

	{
		GLFWPlatform platform;

		Vulkan::WSI wsi;
		wsi.SetPlatform(&platform);
		wsi.SetBackbufferSrgb(true);
		wsi.Init(1, nullptr, 0);

		{
			Vulkan::Device& device = wsi.GetDevice();


			const char* vertex_code = R"(
#version 450
			
layout(location = 0) in vec3 test;

void main()
{
	gl_Position = vec4(1.0, 0.0, 0.0, 1.0);
}

)";
			Vulkan::ShaderHandle vert_shader = device.CreateShaderGLSL(vertex_code, Vulkan::ShaderStage::Vertex);


			const char* frag_code = R"(
#version 450
			
layout(location = 0) in vec3 test;

void main()
{
	gl_Position = vec4(1.0, 0.0, 0.0, 1.0);
}

)";
			Vulkan::ShaderHandle frag_shader = device.CreateShaderGLSL(frag_code, Vulkan::ShaderStage::Fragment);

			Vulkan::GraphicsProgramShaders p_shaders;
			p_shaders.vertex = vert_shader;
			p_shaders.fragment = frag_shader;

			Vulkan::ProgramHandle program = device.CreateGraphicsProgram(p_shaders);

				Util::Timer timer;
			timer.start();


			uint64_t loops = 0;
			while (platform.Alive(wsi))
			{
				//std::this_thread::sleep_for(std::chrono::milliseconds(16));
				wsi.BeginFrame();
				{
					auto cmd = device.RequestCommandBuffer();

					// Just render a clear color to screen.
					// There is a lot of stuff going on in these few calls which will need its own sample to explore w.r.t. synchronization.
					// For now, you'll just get a blue-ish color on screen.
					Vulkan::RenderPassInfo rp = device.GetSwapchainRenderPass(Vulkan::SwapchainRenderPass::ColorOnly);
					rp.clear_color[0].float32[0] = 0.1f;
					rp.clear_color[0].float32[1] = 0.2f;
					rp.clear_color[0].float32[2] = 0.3f;
					cmd->BeginRenderPass(rp);

					//cmd->SetProgram(program);


					cmd->EndRenderPass();
					device.Submit(cmd);
				}

				wsi.EndFrame();

				loops++;
			}

			float time_milli = timer.end() / (float)loops * 1000;

			QM_LOG_INFO("Average Frame time (ms): %f\n", time_milli);
		}

	}

	glfwTerminate();
	
}