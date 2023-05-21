#ifndef VKOCCLUSIONTEST_INSTANCE_H
#define VKOCCLUSIONTEST_INSTANCE_H

#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>

class Swapchain;

class Instance {
private:
	vk::Instance _instance;
	vk::PhysicalDevice _physical_device;
	vk::Device _device;
	vk::Queue _graphics_queue;
	vk::Queue _present_queue;
	vk::Queue _compute_queue;
	vk::CommandPool _graphics_command_pool;
	vk::CommandPool _compute_command_pool;
	vk::PhysicalDeviceMemoryProperties _memory_properties;
	vk::DescriptorPool _descriptor_pool;
	uint32_t _graphics_index;
	uint32_t _present_index;
	uint32_t _compute_index;
public:
	Instance(SDL_Window* window);
	~Instance();

	void create_device(const Swapchain& swapchain);
	void wait_idle() const;
	std::vector<vk::DescriptorSet> create_descriptor_sets(const vk::ArrayProxy<vk::DescriptorSetLayout>& info);

	inline const vk::Instance& instance() const {
		return _instance;
	}

	inline operator const vk::Instance&() const {
		return _instance;
	}

	inline const vk::Device& device() const {
		return _device;
	}

	inline operator const vk::Device&() const {
		return _device;
	}

	inline const vk::PhysicalDevice& physical_device() const {
		return _physical_device;
	}

	inline operator const vk::PhysicalDevice&() const {
		return _physical_device;
	}

	inline const vk::Queue& graphics_queue() const {
		return _graphics_queue;
	}

	inline const vk::Queue& present_queue() const {
		return _present_queue;
	}

	inline const vk::Queue& compute_queue() const {
		return _compute_queue;
	}

	inline uint32_t graphics_queue_index() const {
		return _graphics_index;
	}

	inline uint32_t present_queue_index() const {
		return _present_index;
	}

	inline uint32_t compute_queue_index() const {
		return _compute_index;
	}

	inline const vk::CommandPool& graphics_command_pool() const {
		return _graphics_command_pool;
	}

	inline const vk::CommandPool& compute_command_pool() const {
		return _compute_command_pool;
	}

	inline const vk::PhysicalDeviceMemoryProperties& memory_properties() const {
		return _memory_properties;
	}
};

#endif //VKOCCLUSIONTEST_INSTANCE_H
