#include "GlfwGeneral.hpp"
#include "EasyVulkan.hpp"
#include "model.hpp"
#include"scene.hpp"


//fuzhu
void printMat4(const std::string& name, const glm::mat4& mat)
{
	std::cout << "---------- " << name << " ----------" << std::endl;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			// 格式化输出：固定小数位、左对齐、占12个字符宽度
			std::cout << std::fixed << std::setprecision(6)
				<< std::left << std::setw(12) << mat[i][j];
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

// 打印glm::vec3向量的辅助函数
void printVec3(const std::string& name, const glm::vec3& vec)
{
	std::cout << name << ": "
		<< "(" << std::fixed << std::setprecision(6)
		<< std::setw(10) << vec.x << ", "
		<< std::setw(10) << vec.y << ", "
		<< std::setw(10) << vec.z << ")" << std::endl;
}

// 打印CameraUbo所有数据的函数
void printCameraUbo(const CameraUbo& ubo)
{
	std::cout << "===== CameraUbo Data =====" << std::endl;

	// 打印各个矩阵
	printMat4("View Matrix", ubo.view);
	printMat4("Projection Matrix", ubo.proj);
	printMat4("ProjView Matrix", ubo.projview);

	// 打印相机位置向量
	printVec3("Camera Position", ubo.cam_pos);

	std::cout << "==========================" << std::endl << std::endl;
}

// 打印PointLight结构体的函数
void printPointLight(const PointLight& light, const std::string& label = "")
{
	// 如果提供了标签，打印标签（方便区分多个光源）
	if (!label.empty())
	{
		std::cout << "===== Point Light: " << label << " =====" << std::endl;
	}
	else
	{
		std::cout << "===== Point Light Data =====" << std::endl;
	}

	// 打印各成员变量
	printVec3("  Position", light.pos);                     // 位置
	std::cout << "  Radius:   " << std::fixed << std::setprecision(3)
		<< std::setw(8) << light.radius << std::endl;  // 影响半径
	printVec3("  Intensity", light.intensity);               // 强度（RGB）
	std::cout << "  Padding:  " << std::fixed << std::setprecision(3)
		<< std::setw(8) << light.padding << " (对齐用)" << std::endl;  // 对齐值

	std::cout << "=============================" << std::endl << std::endl;
}


//




inline bool isNearlyEqual(float a, float b, float tolerance = SMALL_NUMBER)
{
	return glm::abs(a - b) <= SMALL_NUMBER;
}


using namespace vulkan;


extern VModel model;
Camera camera;
extern depthStencilAttachment  predepthAttachment;

VkFormat depthStencilFormat;
const auto& RenderPassAndFramebuffers_Depth() {
	static const auto& rpwf = CreateRpwf_preDepth(depthStencilFormat);
	return rpwf;
}
const auto& RenderPassAndFramebuffer_mainRender() {
	static const auto& rpwf = CreateRpwf_mainRender(depthStencilFormat);
	return rpwf;
}
const auto& DescriptorPool(){
	static const auto& Descriptorpool = CreateDescriptorPool();
	return Descriptorpool;
}
const auto& Buffers1() {
	static const auto& buffers = CreateBuffers();
	return buffers;
}
const auto& Sampler() {
	static const auto& ssampler = CreateTextureSampler();
	return ssampler;
}
const auto& cmdPoolS() {
	static const auto& commandpools = CreateCommandPool();
	return commandpools;
}

//描述符布局：
descriptorSetLayout object_descriptor_set_layout;
descriptorSetLayout camera_descriptor_set_layout;
descriptorSetLayout light_culling_descriptor_set_layout;
descriptorSetLayout intermediate_descriptor_set_layout;
descriptorSetLayout material_descriptor_set_layout;
//描述符集
descriptorSet object_descriptor_set;
descriptorSet camera_descriptor_set;
descriptorSet light_culling_descriptor_set;
descriptorSet intermediate_descriptor_set;

void CreateDescriptorSetLayout() {
	//object_descriptor_set_layout:
	VkDescriptorSetLayoutBinding object_layout_binding = {};
	object_layout_binding.binding = 0;
	object_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	object_layout_binding.descriptorCount = 1;
	object_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // only referencing from vertex shader
	object_layout_binding.pImmutableSamplers = nullptr;
	VkDescriptorSetLayoutCreateInfo layout_info1 = {};
	layout_info1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info1.bindingCount = 1;
	layout_info1.pBindings = &object_layout_binding;
	object_descriptor_set_layout.Create(layout_info1);

	//camera_descriptor_set_layout
	VkDescriptorSetLayoutBinding camera_layout_binding = {};
	camera_layout_binding.binding = 0;
	camera_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	camera_layout_binding.descriptorCount = 1;
	camera_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT; // only referencing from vertex shader
	camera_layout_binding.pImmutableSamplers = nullptr;
	VkDescriptorSetLayoutCreateInfo layout_info2 = {};
	layout_info2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info2.bindingCount = 1;
	layout_info2.pBindings = &camera_layout_binding;
	camera_descriptor_set_layout.Create(layout_info2);

	//light_culling_descriptor_set_layout
	std::vector<VkDescriptorSetLayoutBinding> light_culling_set_layout_bindings = {};
	VkDescriptorSetLayoutBinding light_culling_layout_binding_result = {};
	light_culling_layout_binding_result.binding = 0;
	light_culling_layout_binding_result.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	light_culling_layout_binding_result.descriptorCount = 1;
	light_culling_layout_binding_result.stageFlags =  VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT; // only referencing from vertex shader
	light_culling_layout_binding_result.pImmutableSamplers = nullptr;
	VkDescriptorSetLayoutBinding light_culling_layout_binding_point_lights = {};
	light_culling_layout_binding_point_lights.binding = 1;
	light_culling_layout_binding_point_lights.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	light_culling_layout_binding_point_lights.descriptorCount = 1;
	light_culling_layout_binding_point_lights.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT; // only referencing from vertex shader
	light_culling_layout_binding_point_lights.pImmutableSamplers = nullptr;

	light_culling_set_layout_bindings.push_back(light_culling_layout_binding_result);
	light_culling_set_layout_bindings.push_back(light_culling_layout_binding_point_lights);
	VkDescriptorSetLayoutCreateInfo layout_info3 = {};
	layout_info3.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info3.bindingCount = 2;
	layout_info3.pBindings = light_culling_set_layout_bindings.data();
	light_culling_descriptor_set_layout.Create(layout_info3);

	//intermediate_descriptor_set_layout
	VkDescriptorSetLayoutBinding intermediate_layout_binding = {};
	intermediate_layout_binding.binding = 0;
	intermediate_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	intermediate_layout_binding.descriptorCount = 1;
	intermediate_layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // only referencing from vertex shader
	intermediate_layout_binding.pImmutableSamplers = nullptr;
	VkDescriptorSetLayoutCreateInfo layout_info4 = {};
	layout_info4.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info4.bindingCount = 1;
	layout_info4.pBindings = &intermediate_layout_binding;
	intermediate_descriptor_set_layout.Create(layout_info4);

	//material_descriptor_set_layout

	std::vector<VkDescriptorSetLayoutBinding> material_set_layout_bindings = {};
	VkDescriptorSetLayoutBinding uniform_layout_binding = {};
	uniform_layout_binding.binding = 0;
	uniform_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniform_layout_binding.descriptorCount = 1;
	uniform_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT ; // only referencing from vertex shader
	uniform_layout_binding.pImmutableSamplers = nullptr;
	VkDescriptorSetLayoutBinding albedo_map_layout_binding = {};
	albedo_map_layout_binding.binding = 1;
	albedo_map_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedo_map_layout_binding.descriptorCount = 1;
	albedo_map_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // only referencing from vertex shader
	albedo_map_layout_binding.pImmutableSamplers = nullptr;
	VkDescriptorSetLayoutBinding normap_layout_binding = {};
	normap_layout_binding.binding = 2;
	normap_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normap_layout_binding.descriptorCount = 1;
	normap_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT ; // only referencing from vertex shader
	normap_layout_binding.pImmutableSamplers = nullptr;

	material_set_layout_bindings.push_back(uniform_layout_binding);
	material_set_layout_bindings.push_back(albedo_map_layout_binding);
	material_set_layout_bindings.push_back(normap_layout_binding);
	VkDescriptorSetLayoutCreateInfo layout_info5 = {};
	layout_info5.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info5.bindingCount = 3;
	layout_info5.pBindings = material_set_layout_bindings.data();
	material_descriptor_set_layout.Create(layout_info5);



}

void CreateSceneObjectDescriptorSet() {

	const auto& Descriptorpool = DescriptorPool();
	DescriptorPool().AllocateSets(object_descriptor_set, object_descriptor_set_layout);
	VkDescriptorBufferInfo bufferInfos[] = {
		{ Buffers1().object_uniform_buffer, 0,sizeof(SceneObjectUbo)}
	};
	object_descriptor_set.Write(bufferInfos[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 0);
}

void CreateCameraDescriptorSet() {

	const auto& Descriptorpool = DescriptorPool();
	DescriptorPool().AllocateSets(camera_descriptor_set, camera_descriptor_set_layout);
	VkDescriptorBufferInfo bufferInfos[] = {
		{ Buffers1().camera_uniform_buffer, 0,sizeof(CameraUbo)}
	};
	camera_descriptor_set.Write(bufferInfos[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 0);
}

void createLigutCullingDescriptorSet()
{
	const auto& Descriptorpool = DescriptorPool();
	DescriptorPool().AllocateSets(light_culling_descriptor_set, light_culling_descriptor_set_layout);
	VkDescriptorBufferInfo bufferInfos[] = {
		{ Buffers1().light_visibility_storage_buffer, 0, Buffers1().light_visibility_buffer_size},
		{ Buffers1().pointlight_uniform_buffer, 0, Buffers1().pointlight_buffer_size}
	};
	light_culling_descriptor_set.Write(bufferInfos[0], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, 0);
	light_culling_descriptor_set.Write(bufferInfos[1], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, 0);

}


void CreateIntermediateDescriptorSet() {

	const auto& Descriptorpool = DescriptorPool();
	DescriptorPool().AllocateSets(intermediate_descriptor_set, intermediate_descriptor_set_layout);
}

void updateIntermediateDescriptorSet() {

	VkDescriptorImageInfo depth_image_info = {
		 Sampler() ,
		predepthAttachment.ImageView(),
		 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
	};
	intermediate_descriptor_set.Write(depth_image_info, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 0);
}


pipelineLayout pipelineLayout_depth;
pipelineLayout pipelineLayout_mainrender;
pipeline pipeline_depth;
pipeline pipeline_mainrender;
pipelineLayout pipelineLayout_Compute;
pipeline pipeline_Compute;

void CreateGraphicLayout() {
	//Depth
	std::vector<VkDescriptorSetLayout>  set_layouts_Depth = { object_descriptor_set_layout, camera_descriptor_set_layout };
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo_Depth = {
		.setLayoutCount = static_cast<uint32_t>(set_layouts_Depth.size()),
		.pSetLayouts = set_layouts_Depth.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};
	pipelineLayout_depth.Create(pipelineLayoutCreateInfo_Depth);
	//MainRender
	VkPushConstantRange push_constant_range = {};
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(PushConstantObject);
	push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkDescriptorSetLayout> set_layouts_mainRender = { object_descriptor_set_layout, camera_descriptor_set_layout, light_culling_descriptor_set_layout, intermediate_descriptor_set_layout, material_descriptor_set_layout };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo_mainRender = {
		.setLayoutCount = static_cast<uint32_t>(set_layouts_mainRender.size()),
		.pSetLayouts = set_layouts_mainRender.data(),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push_constant_range,
	};
	pipelineLayout_mainrender.Create(pipelineLayoutCreateInfo_mainRender);
}

void CreateGraphicPipeline() {
	//Depth
	static shaderModule vert_depth("shader/depth_vert.spv");
	static shaderModule frag_depth("shader/depth_frag.spv");
	//mainrender
	static shaderModule vert_mainrender("shader/forwardplus_vert.spv");
	static shaderModule frag_mainrender("shader/forwardplus_frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_depth[2] = {
		vert_depth.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_depth.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_mainrender[2] = {
		vert_mainrender.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_mainrender.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	auto Create = [] {
		//Depth
		graphicsPipelineCreateInfoPack pipelineCiPack0;
		pipelineCiPack0.createInfo.layout = pipelineLayout_depth;
		pipelineCiPack0.createInfo.renderPass = RenderPassAndFramebuffers_Depth().renderPass;
		pipelineCiPack0.vertexInputBindings.emplace_back(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
		pipelineCiPack0.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos));
		pipelineCiPack0.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
		pipelineCiPack0.vertexInputAttributes.emplace_back(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex_coord));
		pipelineCiPack0.vertexInputAttributes.emplace_back(3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal));
		pipelineCiPack0.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineCiPack0.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		pipelineCiPack0.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack0.rasterizationStateCi.polygonMode = VK_POLYGON_MODE_FILL;
		pipelineCiPack0.rasterizationStateCi.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineCiPack0.rasterizationStateCi.lineWidth = 1;
		pipelineCiPack0.rasterizationStateCi.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		pipelineCiPack0.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack0.depthStencilStateCi.depthTestEnable = VK_TRUE;
		pipelineCiPack0.depthStencilStateCi.depthCompareOp = VK_COMPARE_OP_LESS;
		pipelineCiPack0.depthStencilStateCi.depthWriteEnable = VK_TRUE;
		pipelineCiPack0.UpdateAllArrays();
		pipelineCiPack0.createInfo.stageCount = 2;
		pipelineCiPack0.createInfo.pStages = shaderStageCreateInfos_depth;
		pipeline_depth.Create(pipelineCiPack0);
		
		//mainrender
		graphicsPipelineCreateInfoPack pipelineCiPack1;
		pipelineCiPack1.createInfo.layout = pipelineLayout_mainrender;
		pipelineCiPack1.createInfo.renderPass = RenderPassAndFramebuffer_mainRender().renderPass;
		pipelineCiPack1.vertexInputBindings.emplace_back(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
		pipelineCiPack1.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos));
		pipelineCiPack1.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
		pipelineCiPack1.vertexInputAttributes.emplace_back(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex_coord));
		pipelineCiPack1.vertexInputAttributes.emplace_back(3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal));
		pipelineCiPack1.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineCiPack1.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		pipelineCiPack1.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack1.rasterizationStateCi.polygonMode = VK_POLYGON_MODE_FILL;
		pipelineCiPack1.rasterizationStateCi.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineCiPack1.rasterizationStateCi.lineWidth = 1;
		pipelineCiPack1.rasterizationStateCi.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		pipelineCiPack1.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack1.depthStencilStateCi.depthTestEnable = VK_TRUE;
		pipelineCiPack1.depthStencilStateCi.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		VkPipelineColorBlendAttachmentState color_blend_attachment = {};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_TRUE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
		pipelineCiPack1.colorBlendAttachmentStates.push_back(color_blend_attachment);
		pipelineCiPack1.UpdateAllArrays();
		pipelineCiPack1.createInfo.stageCount = 2;
		pipelineCiPack1.createInfo.pStages = shaderStageCreateInfos_mainrender;
		pipeline_mainrender.Create(pipelineCiPack1);
		};
	auto Destroy = [] {
		pipeline_depth.~pipeline();
		pipeline_mainrender.~pipeline();
		};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
}

void CreateComputePipeline() {

	VkPushConstantRange push_constant_range = {};
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(PushConstantObject);
	push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	
	std::vector<VkDescriptorSetLayout> set_layouts_Compute = { light_culling_descriptor_set_layout, camera_descriptor_set_layout, intermediate_descriptor_set_layout };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo_Compute = {
		.setLayoutCount = static_cast<uint32_t>(set_layouts_Compute.size()),
		.pSetLayouts = set_layouts_Compute.data(),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push_constant_range,
	};
	pipelineLayout_Compute.Create(pipelineLayoutCreateInfo_Compute);

	static shaderModule compute_shader("shader/light_culling_comp.spv");    
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_Compute[1] = {
	compute_shader.StageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT),
	};
	auto Create = [] {
		VkComputePipelineCreateInfo pipeline_create_info;
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_create_info.stage = shaderStageCreateInfos_Compute[0];
		pipeline_create_info.layout = pipelineLayout_Compute;
		pipeline_create_info.pNext = nullptr;
		pipeline_create_info.flags = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE; // not deriving from existing pipeline
		pipeline_create_info.basePipelineIndex = -1; // Optional
		pipeline_Compute.Create(pipeline_create_info);
		};
	auto Destroy = [] {
		pipeline_Compute.~pipeline();
		};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
}

void Updateuniform(float deltatime) {
	static auto start_time = std::chrono::high_resolution_clock::now();
	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0f;
	CameraUbo ubo = {};
	ubo.view = graphicsBase::Base().view_matrix;
	ubo.proj = glm::perspective(glm::radians(45.0f), windowSize.width / (float)windowSize.height, 0.5f, 100.0f);
	ubo.proj[1][1] *= -1; //since the Y axis of Vulkan NDC points down
	ubo.projview = ubo.proj * ubo.view;
	ubo.cam_pos = (graphicsBase::Base().cam_pos);
	//ubo.cam_pos = { 100,0,100 };
	Buffers1().camera_uniform_buffer.TransferData(ubo);
	//printCameraUbo(ubo);
	auto light_num = static_cast<int>(pointlights.size());
	VkDeviceSize bufferSize = sizeof(PointLight) * MAX_POINT_LIGHT_COUNT + sizeof(glm::vec4);
	for (int i = 0; i < light_num; i++) {
		pointlights[i].pos += glm::vec3(0, 3.0f, 0) * deltatime;
		if (pointlights[i].pos.y > getGlobalTestSceneConfiguration().max_light_pos.y) {
			pointlights[i].pos.y -= (getGlobalTestSceneConfiguration().max_light_pos.y - getGlobalTestSceneConfiguration().min_light_pos.y);
		}
	}
	auto pointlights_size = sizeof(PointLight) * pointlights.size();
	Buffers1().pointlight_uniform_buffer.TransferData(light_num);
	Buffers1().pointlight_uniform_buffer.TransferData(pointlights.data(), pointlights_size, sizeof(glm::vec4));
}

commandBuffer depth_prepass_command_buffer;
commandBuffer light_culling_command_buffer;
std::vector<commandBuffer> command_buffers; // buffers will be released when pool destroyed

void createDepthPrePassCommandBuffer() {
	if (depth_prepass_command_buffer != VK_NULL_HANDLE)
	{
		cmdPoolS().graphics_command_pool.FreeBuffers(depth_prepass_command_buffer);
	}
	cmdPoolS().graphics_command_pool.AllocateBuffers(depth_prepass_command_buffer);
	depth_prepass_command_buffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT); 
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 当前布局
	barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // 目标布局
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // 队列族不变时忽略
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = predepthAttachment.Image(); // 你的深度图像句柄
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // 深度附件
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // 无前置访问
	barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // 后续要写入深度

	// 插入屏障到命令缓冲区
	vkCmdPipelineBarrier(
		depth_prepass_command_buffer, // 你的命令缓冲区
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
	VkClearValue clearValues = {
	.depthStencil = {1.0f, 0}  // depth=1.0，stencil=0
	};
	RenderPassAndFramebuffers_Depth().renderPass.CmdBegin(depth_prepass_command_buffer, RenderPassAndFramebuffers_Depth().framebuffer, { {}, windowSize }, clearValues);
	int cnt = model.getMeshParts().size();
	for (const auto& part : model.getMeshParts())
	{
		vkCmdBindPipeline(depth_prepass_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_depth);
		std::vector<VkDescriptorSet> depth_descriptor_sets = { object_descriptor_set, camera_descriptor_set };
		vkCmdBindDescriptorSets(depth_prepass_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_depth, 0, 2, depth_descriptor_sets.data(), 0, nullptr);
		VkBuffer vertex_buffers[] = { part.vertex_buffer_section.buffer };
		VkDeviceSize vertex_offsets[] = { part.vertex_buffer_section.offset };
		vkCmdBindVertexBuffers(depth_prepass_command_buffer, 0, 1, vertex_buffers, vertex_offsets);
		vkCmdBindIndexBuffer(depth_prepass_command_buffer, part.index_buffer_section.buffer, part.index_buffer_section.offset, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(depth_prepass_command_buffer, static_cast<uint32_t>(part.index_count), 1,0, 0, 0);
	}
	RenderPassAndFramebuffers_Depth().renderPass.CmdEnd(depth_prepass_command_buffer);
	VkImageMemoryBarrier depthToSampledBarrier = {};
	depthToSampledBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	depthToSampledBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	depthToSampledBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthToSampledBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	depthToSampledBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	depthToSampledBarrier.image = predepthAttachment.Image();
	depthToSampledBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthToSampledBarrier.subresourceRange.baseMipLevel = 0;
	depthToSampledBarrier.subresourceRange.levelCount = 1;
	depthToSampledBarrier.subresourceRange.baseArrayLayer = 0;
	depthToSampledBarrier.subresourceRange.layerCount = 1;
	depthToSampledBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT; // 预渲染后的访问
	depthToSampledBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // 计算着色器的访问

	vkCmdPipelineBarrier(
		depth_prepass_command_buffer,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, // 前一阶段（深度测试）
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,       // 后一阶段（计算着色器采样）
		0,
		0, nullptr,
		0, nullptr,
		1, &depthToSampledBarrier
	);
	depth_prepass_command_buffer.End();
}

void createlightcullincommand_bufferCommandBuffer() {
	if (light_culling_command_buffer != VK_NULL_HANDLE)
	{
		cmdPoolS().compute_command_pool.FreeBuffers(light_culling_command_buffer);
	}
	cmdPoolS().compute_command_pool.AllocateBuffers(light_culling_command_buffer);
	light_culling_command_buffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
	std::vector<VkBufferMemoryBarrier> barriers_before;
	barriers_before.emplace_back
	(
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // 1. sType（固定值）
		nullptr,
		VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT,  // srcAccessMask：之前的访问类型（片段着色器读取）
		VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT,   // dstAccessMask：接下来的访问类型（计算着色器写入）
		0, // srcQueueFamilyIndex：源队列族（0表示无跨队列族，或未指定）
		0, // dstQueueFamilyIndex：目标队列族（0表示无跨队列族）
		Buffers1().light_visibility_storage_buffer,  // buffer：需要同步的缓冲区（光源可见性缓冲）
		0,  // offset：同步范围的起始偏移（0表示从开头）
		Buffers1().light_visibility_buffer_size  // size：同步范围的大小（整个缓冲区）
	);
	light_culling_command_buffer.pipelineBarrier(
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		0, nullptr,
		static_cast<uint32_t>(barriers_before.size()),
		barriers_before.data(),
		0, nullptr
	);
	std::vector<VkDescriptorSet> lightcull_descriptor_sets = { light_culling_descriptor_set, camera_descriptor_set, intermediate_descriptor_set };
	vkCmdBindDescriptorSets(light_culling_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout_Compute, 0, 3, lightcull_descriptor_sets.data(), 0, nullptr);
	PushConstantObject pco = { static_cast<int>(windowSize.width), static_cast<int>(windowSize.height), graphicsBase::Base().tile_count_per_row, graphicsBase::Base().tile_count_per_col };
	vkCmdPushConstants(light_culling_command_buffer, pipelineLayout_Compute,VK_SHADER_STAGE_COMPUTE_BIT,0, sizeof(pco), &pco);
	vkCmdBindPipeline(light_culling_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_Compute);

	std::vector<VkBufferMemoryBarrier> barriers_after;
	barriers_after.emplace_back(
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // 1. sType（固定值）
		nullptr,
		VK_ACCESS_SHADER_WRITE_BIT,  // srcAccessMask
		VK_ACCESS_SHADER_READ_BIT,   // dstAccessMask
		0,                           // srcQueueFamilyIndex
		0,                           // dstQueueFamilyIndex
		Buffers1().light_visibility_storage_buffer,  // buffer
		0,                           // offset
		Buffers1().light_visibility_buffer_size      // size
	);
	light_culling_command_buffer.pipelineBarrier(
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		static_cast<uint32_t>(barriers_after.size()),
		barriers_after.data(),
		0, nullptr
	);
	VkImageMemoryBarrier sampledToDepthBarrier = {};
	sampledToDepthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	sampledToDepthBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sampledToDepthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	sampledToDepthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	sampledToDepthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	sampledToDepthBarrier.image = predepthAttachment.Image();
	sampledToDepthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	sampledToDepthBarrier.subresourceRange.baseMipLevel = 0;
	sampledToDepthBarrier.subresourceRange.levelCount = 1;
	sampledToDepthBarrier.subresourceRange.baseArrayLayer = 0;
	sampledToDepthBarrier.subresourceRange.layerCount = 1;
	sampledToDepthBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;  // 计算着色器的读取
	sampledToDepthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;  // 主渲染的深度读取

	light_culling_command_buffer.pipelineBarrier(
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // 前一阶段（计算着色器）
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,  // 后一阶段（主渲染的深度测试）
		0,
		0, nullptr,
		0, nullptr,
		1, &sampledToDepthBarrier
	);
	int tile_count_x = (windowSize.width + TILE_SIZE - 1) / TILE_SIZE;
	int tile_count_y = (windowSize.height + TILE_SIZE - 1) / TILE_SIZE;
	uint32_t group_x = tile_count_x ; // 向上取整，因为 local_size_x = 32
	uint32_t group_y = tile_count_y;
	uint32_t group_z = 1; // 三维工作组的 z 维度为 1（仅二维 Tile）
	vkCmdDispatch(light_culling_command_buffer, group_x, group_y, group_z);
	light_culling_command_buffer.End();
	

}

void createGraphicsCommandBuffer() {
	if (command_buffers.size()>0)
	{
		for (int i = 0; i < command_buffers.size(); i++)
		{
			cmdPoolS().graphics_command_pool.FreeBuffers(command_buffers[i]);
		}
	}
	command_buffers.clear();
	command_buffers.resize(graphicsBase::Base().SwapchainImageCount());
	for (int i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++)
	{
		cmdPoolS().graphics_command_pool.AllocateBuffers(command_buffers[i]);
	}
	for (size_t i = 0; i < command_buffers.size(); i++)
	{
		command_buffers[i].Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
		VkClearValue clearValues = {
	.color = {0.0f, 0.0f, 0.0f, 1.0f}  // 例如：红色，不透明
		};
		RenderPassAndFramebuffer_mainRender().renderPass.CmdBegin(command_buffers[i], RenderPassAndFramebuffer_mainRender().framebuffers[i], {{}, windowSize}, clearValues);
		PushConstantObject pco = {
			static_cast<int>(windowSize.width),
			static_cast<int>(windowSize.height),
			graphicsBase::Base().tile_count_per_row, graphicsBase::Base().tile_count_per_col,
			graphicsBase::Base().debug_view_index
		};
		vkCmdPushConstants(command_buffers[i], pipelineLayout_mainrender, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pco), &pco);
		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_mainrender);
		std::vector<VkDescriptorSet> descriptor_sets = { object_descriptor_set, camera_descriptor_set, light_culling_descriptor_set, intermediate_descriptor_set };
		vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS
			, pipelineLayout_mainrender, 0, static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), 0, nullptr);
		for (const auto& part : model.getMeshParts())
		{
			VkBuffer vertex_buffers[] = { part.vertex_buffer_section.buffer };
			VkDeviceSize offsets[] = { part.vertex_buffer_section.offset };
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], part.index_buffer_section.buffer, part.index_buffer_section.offset, VK_INDEX_TYPE_UINT32);

			std::vector<VkDescriptorSet> mesh_descriptor_sets = { part.material_descriptor_set };
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS
				, pipelineLayout_mainrender, static_cast<uint32_t>(descriptor_sets.size()), static_cast<uint32_t>(mesh_descriptor_sets.size()), mesh_descriptor_sets.data(), 0, nullptr);
			vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(part.index_count), 1, 0, 0, 0);
		}
		vkCmdEndRenderPass(command_buffers[i]);

		auto record_result = vkEndCommandBuffer(command_buffers[i]);
		if (record_result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}
}

void prepareAll() {
	CreateDescriptorSetLayout();
	CreateGraphicLayout();
	CreateGraphicPipeline();
	CreateComputePipeline();
	model.loadModelFromFile(
		getGlobalTestSceneConfiguration().model_file,
		Sampler(),
		DescriptorPool(),
		material_descriptor_set_layout
	);
	CreateSceneObjectDescriptorSet();
	CreateCameraDescriptorSet();
	createLigutCullingDescriptorSet();
	CreateIntermediateDescriptorSet();
	updateIntermediateDescriptorSet();
	createDepthPrePassCommandBuffer();
	createlightcullincommand_bufferCommandBuffer();
	createGraphicsCommandBuffer();
}

void tick(float delta_time)
{
	if (AdjustSign.rmb_down)
	{
		if (isNearlyEqual(windowSize.width, 0.f) || isNearlyEqual(windowSize.height, 0.f))
		{
			return;
		}

		auto cursor_delta = (AdjustSign.cursor_pos - AdjustSign.prev_cursor_pos) / static_cast<float>(glm::min(windowSize.width, windowSize.height)) * 2.0f;

		if (!isNearlyEqual(cursor_delta.x, 0))
		{
			camera.rotation = glm::angleAxis(camera.rotation_speed * -cursor_delta.x, vec_up) * camera.rotation; // world up
		}

		if (!isNearlyEqual(cursor_delta.y, 0))
		{
			camera.rotation = camera.rotation * glm::angleAxis(camera.rotation_speed * -cursor_delta.y, vec_right); // local right
		}

		camera.rotation = glm::normalize(camera.rotation);

		AdjustSign.prev_cursor_pos = AdjustSign.cursor_pos;
	}

	if (AdjustSign.w_down)
	{
		// forward
		camera.position += camera.rotation * vec_forward * camera.move_speed * delta_time;
	}

	if (AdjustSign.s_down)
	{
		// back
		camera.position -= camera.rotation * vec_forward * camera.move_speed * delta_time;
	}

	if (AdjustSign.a_down)
	{
		// left
		camera.position -= camera.rotation *vec_right * camera.move_speed * delta_time;
	}

	if (AdjustSign.d_down)
	{
		// right
		camera.position += camera.rotation * vec_right * camera.move_speed * delta_time;
	}

	if (AdjustSign.q_down)
	{
		// up
		camera.position -= vec_up * camera.move_speed * delta_time;
	}

	if (AdjustSign.e_down)
	{
		// down
		camera.position += vec_up * camera.move_speed * delta_time;
	}
}

struct semfence {
		fence fence ; 
		semaphore render_finished_semaphore;
		semaphore lightculling_completed_semaphore;
		semaphore depth_prepass_finished_semaphore;
	};


int main() {

	
	if (!InitializeWindow({ 1280, 720 }))
		return -1;
	depthStencilFormat = graphicsBase::Plus().findDepthFormat();
	std::vector<semfence>semfences(graphicsBase::Base().SwapchainImageCount());
	graphicsBase::Base().cam_pos = camera.getpos();
	graphicsBase::Base().view_matrix = camera.getViewMatrix();
	predepthAttachment.Create(
		VK_FORMAT_D32_SFLOAT,             
		windowSize,                           
		1,                                           
		VK_SAMPLE_COUNT_1_BIT,                     
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT  
	);
	
	
	prepareAll();
	camera.position = getGlobalTestSceneConfiguration().camera_position;
	camera.rotation = getGlobalTestSceneConfiguration().camera_rotation;
	graphicsBase::Base().cam_pos = camera.getpos();
	graphicsBase::Base().view_matrix = camera.getViewMatrix();
	auto start_time = std::chrono::high_resolution_clock::now();
	auto previous = std::chrono::high_resolution_clock::now();
	decltype(previous) current;
	float delta_time;
	uint32_t frameObjectIndex = 0;
	

	
	while (!glfwWindowShouldClose(pWindow)) {
		while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
		{
			glfwWaitEvents();
		}
		current = std::chrono::high_resolution_clock::now();
		delta_time = std::chrono::duration<float>(current - previous).count();
		glfwPollEvents();
		TitleFps();
		if (AdjustSign.z_pressed) // change debug view
		{
			AdjustSign.z_pressed = false;
			graphicsBase::Base().changeDebugViewIndex();
		}

		if (delta_time >= MIN_DELTA_TIME) //prevent underflow
		{
			tick(delta_time);
			previous = current;
		}
		graphicsBase::Base().cam_pos = camera.getpos();
		graphicsBase::Base().view_matrix = camera.getViewMatrix();
		Updateuniform(delta_time);



		semaphore semaphore_imageIsAvailable;
		frameObjectIndex = (frameObjectIndex + 1) % graphicsBase::Base().SwapchainImageCount();
		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		uint32_t imageIndex = graphicsBase::Base().CurrentImageIndex();
		const auto& [fence, render_finished_semaphore, lightculling_completed_semaphore, depth_prepass_finished_semaphore] = semfences[imageIndex];
		//深度：
		graphicsBase::Base().SubmitCommandBuffer_Graphics(depth_prepass_command_buffer, VK_NULL_HANDLE, depth_prepass_finished_semaphore);
		//光剔除
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = depth_prepass_finished_semaphore.Address(); // 等待的信号量数组（如深度预渲染完成信号量）
		submit_info.pWaitDstStageMask = &wait_stage;   // 等待的管线阶段（如计算着色器阶段）
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = light_culling_command_buffer.Address(); // 光剔除命令缓冲区
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = lightculling_completed_semaphore.Address(); // 光剔除完成信号量
		graphicsBase::Base().SubmitCommandBuffer_Compute(submit_info);
		//main
		VkSubmitInfo submit_info1 = {};
		submit_info1.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore wait_semaphores[] = { semaphore_imageIsAvailable , lightculling_completed_semaphore }; // which semaphore to wait
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT }; // which stage to execute
		submit_info1.waitSemaphoreCount = 2;
		submit_info1.pWaitSemaphores = wait_semaphores;
		submit_info1.pWaitDstStageMask = wait_stages;
		submit_info1.commandBufferCount = 1;
		submit_info1.pCommandBuffers = command_buffers[imageIndex].Address();
		VkSemaphore signal_semaphores[] = { render_finished_semaphore };
		submit_info1.signalSemaphoreCount = 1;
		submit_info1.pSignalSemaphores = signal_semaphores;
		graphicsBase::Base().SubmitCommandBuffer_Graphics(submit_info1,fence);
		//present
		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		VkSemaphore present_wait_semaphores[] = { render_finished_semaphore };
		present_info.pWaitSemaphores = present_wait_semaphores;
		VkSwapchainKHR swapChains[] = { graphicsBase::Base().Swapchain() };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapChains;
		present_info.pImageIndices = &imageIndex;
		present_info.pResults = nullptr;
		graphicsBase::Base().PresentImage(present_info);
		fence.WaitAndReset();
	}

	TerminateWindow();
	return 0;
	
}