#pragma once
#include "VKBase+.h"
#include "scene.hpp"

using namespace vulkan;
const VkExtent2D& windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;


struct renderPassWithFramebuffers {
        renderPass renderPass;
        std::vector<framebuffer> framebuffers;
	};
struct renderPassWithFramebuffer {
        renderPass renderPass;
        framebuffer framebuffer;
    };

struct cmdpools {
	commandPool graphics_command_pool;
	commandPool compute_command_pool;
};

depthStencilAttachment predepthAttachment;

struct Buffers {
	uniformBuffer object_uniform_buffer;
	//stagingBuffer object_staging_buffer;
	uniformBuffer camera_uniform_buffer;
	//stagingBuffer camera_staging_buffer;
	uniformBuffer pointlight_uniform_buffer;
	//stagingBuffer pointlight_staging_buffer;
	VkDeviceSize pointlight_buffer_size;
    uniformBuffer light_visibility_uniform_buffer;
	//stagingBuffer light_visibility_staging_buffer;
	VkDeviceSize light_visibility_buffer_size;
};

const auto& CreateRpwf_preDepth(VkFormat depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT) {
	static renderPassWithFramebuffer rpwf;
	static VkFormat _depthStencilFormat = depthStencilFormat;//因为一会儿需要用lambda定义重建交换链时的回调函数，把格式存到静态变量

	VkAttachmentDescription attachmentDescriptions = {
		.format = _depthStencilFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
	};
	VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 0;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0; // 0  refers to the subpass
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	
	VkRenderPassCreateInfo renderPassCreateInfo = {
			.attachmentCount = 1,
			.pAttachments = &attachmentDescriptions,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency
	};

	rpwf.renderPass.Create(renderPassCreateInfo);

	auto CreateFramebuffers = [] {
		predepthAttachment.Create(_depthStencilFormat, windowSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		VkFramebufferCreateInfo framebufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = rpwf.renderPass,
			.attachmentCount = 1,
			.width = windowSize.width,
			.height = windowSize.height,
			.layers = 1,
		};

		VkImageView attachments[1] = {predepthAttachment.ImageView() };
		framebufferCreateInfo.pAttachments = attachments;
			rpwf.framebuffer.Create(framebufferCreateInfo);
		
		};

	CreateFramebuffers();

	ExecuteOnce(rpwf); //防止需重建逻辑设备时，重复添加回调函数
	graphicsBase::Base().AddCallback_CreateSwapchain(CreateFramebuffers);
	return rpwf;
}

const auto& CreateRpwf_mainRender(VkFormat depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT) {
	static renderPassWithFramebuffers rpwf;
	
		VkAttachmentDescription color_attachment = {};
		color_attachment.format = graphicsBase::Base().SwapchainCreateInfo().imageFormat;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // before rendering
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // after rendering
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // no stencil
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // to be directly used in swap chain

		VkAttachmentDescription depth_attachment = {};
		depth_attachment.format = depthStencilFormat;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref = {};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0; // 0  refers to the subpass
		dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::vector<VkAttachmentDescription> attachments = { color_attachment, depth_attachment };
		
		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = (uint32_t)attachments.size();
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

	auto CreateFramebuffers = [] {
		rpwf.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
		VkFramebufferCreateInfo framebufferCreateInfo = {
			.renderPass = rpwf.renderPass,
			.attachmentCount = 2,
			.width = windowSize.width,
			.height = windowSize.height,
			.layers = 1
		};
		for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++) {
			std::vector<VkImageView> attachmentsview = { graphicsBase::Base().SwapchainImageView(i), predepthAttachment.ImageView() };
			framebufferCreateInfo.pAttachments = attachmentsview.data();
			rpwf.framebuffers[i].Create(framebufferCreateInfo);
		}
		};
	auto DestroyFramebuffers = [] {
		rpwf.framebuffers.clear();//清空vector中的元素时会逐一执行析构函数
		};
	graphicsBase::Base().AddCallback_CreateSwapchain(CreateFramebuffers);
	graphicsBase::Base().AddCallback_DestroySwapchain(DestroyFramebuffers);

	return rpwf;
}

const auto& CreateDescriptorPool() {
	static descriptorPool DescriptorPool;

	
	auto Create = [] {
		VkDescriptorPoolSize descriptorPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,3}
		};
		DescriptorPool.Create(200, descriptorPoolSizes);
		};
	auto Destroy = [] {
		DescriptorPool.~descriptorPool();//清空vector中的元素时会逐一执行析构函数
		};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
	return DescriptorPool;
}

struct _Dummy_VisibleLightsForTile
{
	uint32_t count;
	std::vector<uint32_t> lightindices{ MAX_POINT_LIGHT_PER_TILE };
};

const auto& CreateBuffers() {
	static Buffers buffers;
	auto Create = [] {

		buffers.object_uniform_buffer.Create(sizeof(SceneObjectUbo));
		SceneObjectUbo ubo = {};
		ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(getGlobalTestSceneConfiguration().scale));;
		buffers.object_uniform_buffer.TransferData(ubo);
		
		buffers.camera_uniform_buffer.Create(sizeof(CameraUbo));
		
		for (int i = 0; i < getGlobalTestSceneConfiguration().light_num; i++) {
			glm::vec3 color;
			do { color = { glm::linearRand(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)) }; } while (color.length() < 0.8f);
			pointlights.emplace_back(glm::linearRand(getGlobalTestSceneConfiguration().min_light_pos, getGlobalTestSceneConfiguration().max_light_pos), getGlobalTestSceneConfiguration().light_radius, color);
		}
		auto light_num = static_cast<int>(pointlights.size());
		buffers.pointlight_buffer_size = sizeof(PointLight) * MAX_POINT_LIGHT_COUNT + sizeof(glm::vec4);
		buffers.pointlight_uniform_buffer.Create(buffers.pointlight_buffer_size);

		tile_count_per_row = (windowSize.width - 1) / TILE_SIZE + 1;
		tile_count_per_col = (windowSize.height - 1) / TILE_SIZE + 1;
		buffers.light_visibility_buffer_size = sizeof(_Dummy_VisibleLightsForTile) * tile_count_per_row * tile_count_per_col;
		buffers.light_visibility_uniform_buffer.Create(buffers.light_visibility_buffer_size);


		};

	auto Destroy = [] {
		buffers.object_uniform_buffer.~uniformBuffer();
		buffers.camera_uniform_buffer.~uniformBuffer();
		buffers.pointlight_uniform_buffer.~uniformBuffer();
		buffers.light_visibility_uniform_buffer.~uniformBuffer();
		};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
	return buffers;
}

const auto& CreateTextureSampler() {
	static  sampler Sampler1;


	auto Create = [] {
		VkSamplerCreateInfo sampler_info = {};
		sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.magFilter = VK_FILTER_LINEAR;
		sampler_info.minFilter = VK_FILTER_LINEAR;

		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		sampler_info.anisotropyEnable = VK_TRUE;
		sampler_info.maxAnisotropy = 16;

		sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler_info.unnormalizedCoordinates = VK_FALSE;

		sampler_info.compareEnable = VK_FALSE;
		sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_info.mipLodBias = 0.0f;
		sampler_info.minLod = 0.0f;
		sampler_info.maxLod = 0.0f;

		Sampler1.Create(sampler_info);
		};
	auto Destroy = [] {
		Sampler1.~Sampler1();//清空vector中的元素时会逐一执行析构函数
		};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
	return Sampler1;
}

const auto& CreateCommandPool() {
	static cmdpools CmdPools;


	auto Create = [] {
		CmdPools.graphics_command_pool.Create(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		CmdPools.compute_command_pool.Create(graphicsBase::Base().QueueFamilyIndex_Compute(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		};
	auto Destroy = [] {
		
		CmdPools.graphics_command_pool.~commandPool();
		CmdPools.compute_command_pool.~commandPool();
		};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
	return CmdPools;
}
