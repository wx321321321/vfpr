#pragma once
#include "VKBase+.h"
#include <functional> 

template <class T>
void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std {
	// 为glm::vec3添加哈希特化
	template<> struct hash<glm::vec3> {
		size_t operator()(const glm::vec3& v) const {
			size_t seed = 0;
			hash_combine(seed, v.x);
			hash_combine(seed, v.y);
			hash_combine(seed, v.z);
			return seed;
		}
	};
	// 为glm::vec2添加哈希特化
	template<> struct hash<glm::vec2> {
		size_t operator()(const glm::vec2& v) const {
			size_t seed = 0;
			hash_combine(seed, v.x);
			hash_combine(seed, v.y);
			return seed;
		}
	};
}

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 tex_coord;
	glm::vec3 normal;

	using index_t = uint32_t;

	bool operator==(const Vertex& other) const noexcept
	{
		return pos == other.pos && color == other.color && tex_coord == other.tex_coord && normal == other.normal;
	}

	size_t hash() const
	{
		size_t seed = 0;
		hash_combine(seed, pos);
		hash_combine(seed, color);
		hash_combine(seed, tex_coord);
		hash_combine(seed, normal);
		return seed;
	}
};


	namespace std {
		// hash function for Vertex
		template<> struct hash<Vertex>
		{
			size_t operator()(Vertex const& vertex) const
			{
				return vertex.hash();
			}
		};
	}


	class VContext;

	/**
	* A structure that points to a part of a buffer
	*/
	struct VBufferSection
	{
		VkBuffer buffer = {};  // just a handle, no ownership for this buffer
		VkDeviceSize offset = 0;
		VkDeviceSize size = 0;

		VBufferSection() = default;

		VBufferSection(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
			: buffer(buffer)
			, offset(offset)
			, size(size)
		{
		}
	};
	struct VMeshPart
	{
		// todo: separate mesh part with material?
		// (material as another global storage??)
		VBufferSection vertex_buffer_section = {};
		VBufferSection index_buffer_section = {};
		VBufferSection material_uniform_buffer_section = {};
		size_t index_count = 0;
		vulkan::descriptorSet material_descriptor_set = {};
		vulkan::texture2d albedo_map;
		vulkan::texture2d normal_map;



		VMeshPart(const VBufferSection& vertex_buffer_section, const VBufferSection& index_buffer_section, size_t index_count)
			: vertex_buffer_section(vertex_buffer_section)
			, index_buffer_section(index_buffer_section)
			, index_count(index_count)
		{
		}
	};

	/**
	* A class to manage resources of a static mesh.
	* Must be destructed before the vk::Device used to construct it
	*/
	class VModel
	{
	public:
		VModel() = default;
		~VModel() = default;
		VModel(VModel&&) = default;
		VModel& operator= (VModel&&) = default;

		const std::vector<VMeshPart>& getMeshParts() const
		{
			return mesh_parts;
		}

		static VModel loadModelFromFile(const std::string& path
			, const vulkan::sampler& texture_sampler, const vulkan::descriptorPool& descriptor_pool,
			const vulkan::descriptorSetLayout& material_descriptor_set_layout);

		VModel(const VModel&) = delete;
		VModel& operator= (const VModel&) = delete;

	private:
		vulkan::vertexBuffer vertex_buffer;
		vulkan::indexBuffer index_buffer;
		vulkan::uniformBuffer uniform_buffer;
		std::vector<vulkan::texture2d>textures;
		std::vector<VMeshPart> mesh_parts;

	};

