#include "render.hpp"
#include "vulkan.hpp"
#include "window.hpp"
#include <fstream>

#define THISFILE "render.cpp"

GraphicsPipelineCreator::GraphicsPipelineCreator(GraphicsDevice const& device)
	: m_device(&device), m_render_format(VK_FORMAT_UNDEFINED)
{
	std::fill(m_shader_modules.begin(), m_shader_modules.end(), nullptr);
}

GraphicsPipelineCreator::~GraphicsPipelineCreator()
{
	for (VkShaderModule shader : m_shader_modules)
		vkDestroyShaderModule(m_device->GetLogical(), shader, nullptr);
}

void GraphicsPipelineCreator::AddShaderModule(ShaderType type, char const* filepath)
{
	std::ifstream ifs(filepath, std::ios::ate | std::ios::binary);
	VALIDATE(ifs.is_open());

	size_t fsize = ifs.tellg();
	std::vector<char> buffer(fsize);
	ifs.seekg(0);
	ifs.read(buffer.data(), fsize);
	ifs.close();

	VkShaderModuleCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = buffer.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

	VALIDATE(vkCreateShaderModule(m_device->GetLogical(), &create_info, nullptr, &m_shader_modules[type]) == VK_SUCCESS);
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipelineCreator const& creator)
	: m_device(creator.m_device)
{
	// Check if vertex shader and fragment shader present.
	ASSERT(creator.m_shader_modules[VERTEX_SHADER] && creator.m_shader_modules[FRAGMENT_SHADER]);

	VkPipelineShaderStageCreateInfo vert_stage{};
	vert_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_stage.module = creator.m_shader_modules[VERTEX_SHADER];
	vert_stage.pName = "main";

	VkPipelineShaderStageCreateInfo frag_stage{};
	frag_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_stage.module = creator.m_shader_modules[FRAGMENT_SHADER];
	frag_stage.pName = "main";

	// Shader stages
	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage, frag_stage };

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input.vertexBindingDescriptionCount = 0;
	vertex_input.pVertexBindingDescriptions = nullptr; // Optional
	vertex_input.vertexAttributeDescriptionCount = 0;
	vertex_input.pVertexAttributeDescriptions = nullptr; // Optional

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	// Dynamic states
	VkDynamicState constexpr dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dstate{};
	dstate.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dstate.dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState);
	dstate.pDynamicStates = dynamic_states;

	// Viewport states (since dynamic state is enabled, viewport and scissor will be set later)
	VkPipelineViewportStateCreateInfo viewport_state{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// Color blending
	VkPipelineColorBlendAttachmentState blend_func{};
	blend_func.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blend_func.blendEnable = VK_TRUE;
	blend_func.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_func.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_func.colorBlendOp = VK_BLEND_OP_ADD;
	blend_func.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_func.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blend_func.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo blending{};
	blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blending.logicOpEnable = VK_FALSE;
	blending.logicOp = VK_LOGIC_OP_COPY; // Optional
	blending.attachmentCount = 1;
	blending.pAttachments = &blend_func;
	blending.blendConstants[0] = 0.0f; // Optional
	blending.blendConstants[1] = 0.0f; // Optional
	blending.blendConstants[2] = 0.0f; // Optional
	blending.blendConstants[3] = 0.0f; // Optional

	CreatePipelineLayout();

	ASSERT(creator.m_render_format); // Check if defined.
	CreateRenderPass(creator.m_render_format);

	VkGraphicsPipelineCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.stageCount = 2;
	create_info.pStages = shader_stages;
	create_info.pVertexInputState = &vertex_input;
	create_info.pInputAssemblyState = &input_assembly;
	create_info.pViewportState = &viewport_state;
	create_info.pRasterizationState = &rasterizer;
	create_info.pMultisampleState = &multisampling;
	create_info.pColorBlendState = &blending;
	create_info.pDynamicState = &dstate;
	create_info.layout = m_layout;
	create_info.renderPass = m_render_pass;
	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;

	VALIDATE(vkCreateGraphicsPipelines(m_device->GetLogical(), VK_NULL_HANDLE, 1, &create_info, nullptr, &m_pipeline) == VK_SUCCESS);
}

GraphicsPipeline::~GraphicsPipeline()
{
	VkDevice ld = m_device->GetLogical();

	vkDestroyPipeline(ld, m_pipeline, nullptr);
	vkDestroyRenderPass(ld, m_render_pass, nullptr);
	vkDestroyPipelineLayout(ld, m_layout, nullptr);
}

void GraphicsPipeline::CreatePipelineLayout()
{
	VkPipelineLayoutCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.setLayoutCount = 0; // Optional
	create_info.pSetLayouts = nullptr; // Optional
	create_info.pushConstantRangeCount = 0; // Optional
	create_info.pPushConstantRanges = nullptr; // Optional

	VALIDATE(vkCreatePipelineLayout(m_device->GetLogical(), &create_info, nullptr, &m_layout) == VK_SUCCESS);
}

void GraphicsPipeline::CreateRenderPass(VkFormat format)
{
	VkAttachmentDescription attach{};
	attach.format = format;
	attach.samples = VK_SAMPLE_COUNT_1_BIT;
	attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference attach_ref{};
	attach_ref.attachment = 0;
	attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attach_ref;

	VkRenderPassCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.attachmentCount = 1;
	create_info.pAttachments = &attach;
	create_info.subpassCount = 1;
	create_info.pSubpasses = &subpass;

	VALIDATE(vkCreateRenderPass(m_device->GetLogical(), &create_info, nullptr, &m_render_pass) == VK_SUCCESS);
}

Framebuffers::Framebuffers(GraphicsDevice const& device, GraphicsPipeline const& pipeline, Swapchain const& swapchain)
	: m_device(&device)
{
	auto const& image_views = swapchain.GetImageViews();
	auto extent = swapchain.GetExtent();

	m_framebuffers.resize(image_views.size());

	for (int i = 0; i < m_framebuffers.size(); i++)
	{
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = pipeline.GetRenderPass();
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &image_views[i];
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		VALIDATE(vkCreateFramebuffer(device.GetLogical(), &framebufferInfo, nullptr, &m_framebuffers[i]) == VK_SUCCESS);
	}
}

Framebuffers::~Framebuffers()
{
	for (auto framebuffer : m_framebuffers)
		vkDestroyFramebuffer(m_device->GetLogical(), framebuffer, nullptr);
}