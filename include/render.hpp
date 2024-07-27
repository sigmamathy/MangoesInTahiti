#pragma once

#include "core.hpp"

class GraphicsDevice;
class Swapchain;

enum ShaderType
{
	VERTEX_SHADER,
	FRAGMENT_SHADER,

	SHADER_TYPE_COUNT
};

class GraphicsPipelineCreator
{
public:

	GraphicsPipelineCreator(GraphicsDevice const& device);
	~GraphicsPipelineCreator();

	void AddShaderModule(ShaderType type, char const* filepath);
	inline void SetRenderFormat(VkFormat format) { m_render_format = format; }

	GraphicsPipelineCreator(GraphicsPipelineCreator const&) = delete;
	GraphicsPipelineCreator& operator=(GraphicsPipelineCreator const&) = delete;

private:

	GraphicsDevice const* m_device;
	std::array<VkShaderModule, SHADER_TYPE_COUNT> m_shader_modules;
	VkFormat m_render_format;
	friend class GraphicsPipeline;
};

class GraphicsPipeline
{
public:

	GraphicsPipeline(GraphicsPipelineCreator const& creator);

	~GraphicsPipeline();

	inline VkRenderPass GetRenderPass() const { return m_render_pass; }

	GraphicsPipeline(GraphicsPipeline const&) = delete;
	GraphicsPipeline& operator=(GraphicsPipeline const&) = delete;

private:

	GraphicsDevice const* m_device;
	VkPipeline m_pipeline;
	VkPipelineLayout m_layout;
	VkRenderPass m_render_pass;

	void CreatePipelineLayout();
	void CreateRenderPass(VkFormat format);
};

class Framebuffers
{
public:

	Framebuffers(GraphicsDevice const& device, GraphicsPipeline const& pipeline, Swapchain const& swapchain);
	~Framebuffers();

	Framebuffers(Framebuffers const&) = delete;
	Framebuffers& operator=(Framebuffers const&) = delete;

private:

	GraphicsDevice const* m_device;
	std::vector<VkFramebuffer> m_framebuffers;
};

class CommandPool
{
public:

	CommandPool();
};