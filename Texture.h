#ifndef VKOCCLUSIONTEST_TEXTURE_H
#define VKOCCLUSIONTEST_TEXTURE_H

#include <vulkan/vulkan.hpp>
#include "Instance.h"
#include <memory>
#include <glm/glm.hpp>
#include <filesystem>

#define TEXTURE_LEVELS_AUTO 0

class Texture {
private:
	std::shared_ptr<Instance> instance;
	vk::Image _image;
	vk::Format _format;
	vk::ImageUsageFlags _flags;
	vk::DeviceMemory _memory;
	glm::ivec2 _size;
	uint32_t _levels;
public:
	Texture(std::shared_ptr<Instance> instance, vk::Format format, vk::ImageUsageFlags flags, glm::ivec2 sz, uint32_t levels = TEXTURE_LEVELS_AUTO);
	~Texture();

	bool fill_from_file(const vk::CommandBuffer& cmd, const std::filesystem::path& path, uint32_t level, vk::ImageLayout targetLayout = vk::ImageLayout::eShaderReadOnlyOptimal);
	bool fill_from_data(const vk::CommandBuffer& cmd, const std::vector<uint8_t>& data, uint32_t level, uint32_t channels, vk::ImageLayout targetLayout = vk::ImageLayout::eShaderReadOnlyOptimal);

	inline const vk::Image& image() const {
		return _image;
	}

	inline operator const vk::Image&() const {
		return _image;
	}

	inline glm::ivec2 size() const {
		return _size;
	}

	inline vk::Format format() const {
		return _format;
	}

	inline uint32_t levels() const {
		return _levels;
	}
};


#endif //VKOCCLUSIONTEST_TEXTURE_H
