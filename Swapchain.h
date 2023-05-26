#ifndef VKOCCLUSIONTEST_SWAPCHAIN_H
#define VKOCCLUSIONTEST_SWAPCHAIN_H

#include "Instance.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class Swapchain {
private:
	std::shared_ptr<Instance> instance;
	vk::SurfaceKHR _surface;
	vk::SwapchainKHR _swapchain;
	vk::SurfaceFormatKHR _surface_format;
	std::vector<vk::ImageView> _image_views;
	glm::ivec2 _size;
public:
	Swapchain(SDL_Window* window, std::shared_ptr<Instance> instance);

	bool create_swapchain(glm::ivec2 size);
	bool destroy_swapchain();

	~Swapchain();

	inline const vk::SurfaceKHR& surface() const {
		return _surface;
	}

	inline const vk::SwapchainKHR& swapchain() const {
		return _swapchain;
	}

	inline const std::vector<vk::ImageView>& image_views() const& {
		return _image_views;
	}

	inline vk::SurfaceFormatKHR surface_format() const {
		return _surface_format;
	}

	inline glm::ivec2 size() const {
		return _size;
	}
};


#endif //VKOCCLUSIONTEST_SWAPCHAIN_H
