#ifndef VKOCCLUSIONTEST_HZBUFFER_H
#define VKOCCLUSIONTEST_HZBUFFER_H

#include <vulkan/vulkan.hpp>
#include "Instance.h"
#include "Texture.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

class HZBuffer {
private:
	std::shared_ptr<Instance> instance;
	Texture _texture;
	Texture _depth_texture;
	std::vector<glm::ivec2> _sizes;
	vk::ImageView _depth_view;
	std::vector<vk::ImageView> _level_views;
	std::vector<vk::DescriptorSet> _downsample_descriptor_sets;
public:
	HZBuffer(std::shared_ptr<Instance> inst, glm::ivec2 size, const vk::DescriptorSetLayout& downsampleLayout, const vk::Sampler& downsampleSampler);
	~HZBuffer();

	inline const Texture& texture() const {
		return _texture;
	}

	inline const Texture& depth_texture() const {
		return _depth_texture;
	}

	inline const vk::ImageView& depth_view() const {
		return _depth_view;
	}

	inline const std::vector<glm::ivec2>& sizes() const {
		return _sizes;
	}

	inline const std::vector<vk::ImageView>& level_views() const {
		return _level_views;
	}

	inline const std::vector<vk::DescriptorSet>& downsample_descriptor_sets() const {
		return _downsample_descriptor_sets;
	}
};


#endif //VKOCCLUSIONTEST_HZBUFFER_H
