#include "vulkan.hpp"
#include "window.hpp"
#include "render.hpp"

int main()
{
	LaunchVulkan();

	Window* window = new Window(1600, 900, false);
	GraphicsDevice* device = new GraphicsDevice(*window);
	Swapchain* swapchain = new Swapchain(*window, *device);
	GraphicsPipeline* pipeline;

	{
		GraphicsPipelineCreator creator(*device);
		creator.SetRenderFormat(swapchain->GetImageFormat());
		creator.AddShaderModule(VERTEX_SHADER, "shaders/shader.vert.spv");
		creator.AddShaderModule(FRAGMENT_SHADER, "shaders/shader.frag.spv");
		pipeline = new GraphicsPipeline(creator);
	}

	Framebuffers* framebuffers = new Framebuffers(*device, *pipeline, *swapchain);

	delete framebuffers;
	delete pipeline;
	delete swapchain;
	delete device;
	delete window;

	EndVulkan();
	return 0;
}