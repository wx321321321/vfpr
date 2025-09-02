// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.
#include "model.hpp"


VModel model;
inline std::string findFolderName(const std::string& str)
{
	return str.substr(0, str.find_last_of("/\\"));
}


// uniform buffer object for model transformation
struct MaterialUbo
{
	int has_albedo_map;
	int has_normal_map;
};


struct MeshMaterialGroup // grouped by material
{
	std::vector<Vertex> vertices = {};
	std::vector<Vertex::index_t> vertex_indices = {};

	std::string albedo_map_path = "";
	std::string normal_map_path = "";
};

std::vector<MeshMaterialGroup> loadModel(const std::string& path)
{
	

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	std::string folder = findFolderName(path) + "/";
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str(), folder.c_str()))
	{
		throw std::runtime_error(err);
	}

	bool has_vertex_normal = attrib.normals.size() > 0;
	assert(has_vertex_normal);

	std::vector<MeshMaterialGroup> groups(materials.size() + 1); // group parts of the same material together, +1 for unknown material

	for (size_t i = 0; i < materials.size(); i++)
	{
		if (materials[i].diffuse_texname != "")
		{
			groups[i + 1].albedo_map_path = folder + materials[i].diffuse_texname;
		}
		if (materials[i].normal_texname != "")
		{
			groups[i + 1].normal_map_path = folder + materials[i].normal_texname;
		}
		else if (materials[i].bump_texname != "")
		{
			// CryEngine sponza scene uses keyword "bump" to store normal
			groups[i + 1].normal_map_path = folder + materials[i].bump_texname;
		}
	}

	std::vector<std::unordered_map <Vertex, size_t >> unique_vertices_per_group(materials.size() + 1);

	auto appendVertex = [&unique_vertices_per_group, &groups](const Vertex& vertex, int material_id)
		{
			// 0 for unknown material
			auto& unique_vertices = unique_vertices_per_group[material_id + 1];
			auto& group = groups[material_id + 1];
			if (unique_vertices.count(vertex) == 0)
			{
				unique_vertices[vertex] = group.vertices.size(); // auto incrementing size
				group.vertices.push_back(vertex);
			}
			group.vertex_indices.push_back(static_cast<Vertex::index_t>(unique_vertices[vertex]));
		};

	for (const auto& shape : shapes)
	{

		size_t indexOffset = 0;
		for (size_t n = 0; n < shape.mesh.num_face_vertices.size(); n++)
		{
			// per face
			auto ngon = shape.mesh.num_face_vertices[n];
			auto material_id = shape.mesh.material_ids[n];
			for (size_t f = 0; f < ngon; f++)
			{
				const auto& index = shape.mesh.indices[indexOffset + f];

				Vertex vertex;

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.tex_coord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				appendVertex(vertex, material_id);

			}
			indexOffset += ngon;
		}
	}

	return groups;
}

/**
* Load model from file and allocate vulkan resources needed
*/
VModel VModel::loadModelFromFile(const std::string& path, const vulkan::sampler& texture_sampler, const vulkan::descriptorPool& descriptor_pool,
	const vulkan::descriptorSetLayout& material_descriptor_set_layout)
{
	VModel model;

	auto device =vulkan::graphicsBase::Base().Device();
	//std::vector<util::Vertex> vertices, std::vector<util::Vertex::index_t> vertex_indices;
	auto groups = loadModel(path);
	VkDeviceSize vertex_buffer_size = 0;
	VkDeviceSize index_buffer_size = 0;
	for (const auto& group : groups)
	{
		if (group.vertex_indices.size() <= 0)
		{
			continue;
		}
		VkDeviceSize vertex_section_size = sizeof(group.vertices[0]) * group.vertices.size();
		VkDeviceSize index_section_size = sizeof(group.vertex_indices[0]) * group.vertex_indices.size();
		vertex_buffer_size += vertex_section_size;
		index_buffer_size += index_section_size;
	}

	model.vertex_buffer.Create(vertex_buffer_size);
	model.index_buffer.Create(index_buffer_size);
	VkDeviceSize tempvertexsize = 0;
	VkDeviceSize tempindexsize = 0;
	for (const auto& group : groups)
	{
		if (group.vertex_indices.size() <= 0)
		{
			continue;
		}

		VkDeviceSize vertex_section_size = sizeof(group.vertices[0]) * group.vertices.size();
		VkDeviceSize index_section_size = sizeof(group.vertex_indices[0]) * group.vertex_indices.size();

		model.vertex_buffer.TransferData(group.vertices.data(), vertex_section_size, tempvertexsize);
		model.index_buffer.TransferData(group.vertex_indices.data(), index_section_size, tempindexsize);

		VBufferSection vertex_buffer_section = { model.vertex_buffer, tempvertexsize, vertex_section_size };
		VBufferSection index_buffer_section = { model.index_buffer, tempindexsize, index_section_size };

		VMeshPart part = { vertex_buffer_section, index_buffer_section, group.vertex_indices.size() };

		if (!group.albedo_map_path.empty())
		{
			model.textures.emplace_back(group.albedo_map_path.c_str(), VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM);
			part.albedo_map.Create(group.albedo_map_path.c_str(), VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM);
		}
		if (!group.normal_map_path.empty())
		{
			model.textures.emplace_back(group.normal_map_path.c_str(), VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM);
			part.normal_map.Create(group.normal_map_path.c_str(), VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM);
		}
		model.mesh_parts.push_back(std::move(part));
	}

	auto createMaterialDescriptorSet = [ &device, &texture_sampler, &descriptor_pool, &material_descriptor_set_layout](
		VMeshPart& mesh_part
		, vulkan::uniformBuffer& uniform_buffer, VkDeviceSize offset, VkDeviceSize size
		)
		{
			VkDescriptorSetLayout layouts[] = { material_descriptor_set_layout };
			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			alloc_info.descriptorPool = descriptor_pool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = layouts;

			 
			descriptor_pool.AllocateSets(mesh_part.material_descriptor_set, material_descriptor_set_layout);

			MaterialUbo ubo{ 0, 0 };

			std::vector<VkWriteDescriptorSet> descriptor_writes = {};

		
			VkDescriptorBufferInfo uniform_buffer_info = {}; 
			uniform_buffer_info.buffer = uniform_buffer;
			uniform_buffer_info.offset = offset;
			uniform_buffer_info.range = size;
			mesh_part.material_descriptor_set.Write(
				arrayRef(&uniform_buffer_info, 1),          // ��װ��Ļ�������Ϣ��������
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // ���������ͣ�ԭ��Vkö�٣�
				0                                           // Ŀ��󶨵㣨��Ӧ��ɫ�� layout(binding=0)��
			);

			if (!mesh_part.albedo_map.texture2dEmpty())
			{
				ubo.has_albedo_map = 1; // ����UBO��������ڱ��

				VkDescriptorImageInfo albedo_map_info = {};
				albedo_map_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // ԭ��Vk����ö��
				albedo_map_info.imageView = mesh_part.albedo_map.ImageView();                  // ͼ����ͼ���
				albedo_map_info.sampler = texture_sampler;                         // ������������

				// �� arrayRef ��װ����������Ϣ������ Write д����������
				mesh_part.material_descriptor_set.Write(
					arrayRef(&albedo_map_info, 1),                  // ��װ���ͼ����Ϣ��������
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,       // ���������ͣ�������+ͼ����ͼ��ϣ�
					1                                               // Ŀ��󶨵㣨��Ӧ��ɫ�� layout(binding=1)��
				);
			}
			if (!mesh_part.normal_map.texture2dEmpty())
			{
				ubo.has_normal_map = 1; // ����UBO��������ڱ��
				VkDescriptorImageInfo normalmap_info = {};
				normalmap_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				normalmap_info.imageView = mesh_part.normal_map.ImageView();
				normalmap_info.sampler = texture_sampler;
				mesh_part.material_descriptor_set.Write(
					arrayRef(&normalmap_info, 1),                   // ��װ���ͼ����Ϣ��������
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,       // ����������
					2                                               // Ŀ��󶨵㣨��Ӧ��ɫ�� layout(binding=2)��
				);
			}

			uniform_buffer.TransferData(&ubo, offset, size);
		};

	const VkDeviceSize ubo_aligned_size = vulkan::uniformBuffer::CalculateAlignedSize(sizeof(MaterialUbo));
	const VkDeviceSize total_uniform_size = ubo_aligned_size * model.mesh_parts.size(); 

	model.uniform_buffer.Create(total_uniform_size);

	VkDeviceSize current_uniform_offset = 0;
	for (auto& mesh_part : model.mesh_parts)
	{
		createMaterialDescriptorSet(mesh_part, model.uniform_buffer,
			current_uniform_offset,
			sizeof(MaterialUbo));
		current_uniform_offset += ubo_aligned_size;
	}

	return model;
}

